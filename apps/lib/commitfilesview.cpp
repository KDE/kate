/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "commitfilesview.h"

#include "hostprocess.h"
#include <bytearraysplitter.h>
#include <diffparams.h>
#include <gitprocess.h>
#include <ktexteditor_utils.h>

#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMimeDatabase>
#include <QPainter>
#include <QProcess>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

#include <KColorScheme>
#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>

#include <optional>

struct GitFileItem {
    QByteArray oldFileName;
    QByteArray file;
    int linesAdded;
    int linesRemoved;
};

/**
 * Class representing a item inside the treeview
 * Copied from KateProject with modifications as needed
 */
class FileItem : public QStandardItem
{
public:
    enum Type {
        Directory = 1,
        File = 2
    };
    enum Role {
        Path = Qt::UserRole,
        TypeRole,
        LinesAdded,
        LinesRemoved,
        OldPath,
        NewPath,
    };

    FileItem(Type type, const QString &text)
        : QStandardItem(text)
        , m_type(type)
    {
    }

    QVariant data(int role = Qt::UserRole + 1) const override
    {
        if (role == Qt::DecorationRole) {
            return icon();
        }

        if (role == TypeRole) {
            return QVariant(m_type);
        }

        return QStandardItem::data(role);
    }

    /**
     * We want case-insensitive sorting and directories first!
     */
    bool operator<(const QStandardItem &other) const override
    {
        // let directories stay first
        const auto thisType = data(TypeRole).toInt();
        const auto otherType = other.data(TypeRole).toInt();
        if (thisType != otherType) {
            return thisType < otherType;
        }

        // case-insensitive compare of the filename
        return data(Qt::DisplayRole).toString().compare(other.data(Qt::DisplayRole).toString(), Qt::CaseInsensitive) < 0;
    }

    QIcon icon() const
    {
        if (!m_icon.isNull()) {
            return m_icon;
        }

        if (m_type == Directory) {
            m_icon = QIcon::fromTheme(QStringLiteral("folder"));
        } else if (m_type == File) {
            QIcon icon = QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(data(Path).toString(), QMimeDatabase::MatchExtension).iconName());
            if (icon.isNull()) {
                icon = QIcon::fromTheme(QStringLiteral("unknown"));
            }
            m_icon = icon;
        } else {
            Q_UNREACHABLE();
        }
        return m_icon;
    }

private:
    const Type m_type;
    mutable QIcon m_icon;
};

class DiffStyleDelegate : public QStyledItemDelegate
{
public:
    explicit DiffStyleDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.data(FileItem::TypeRole).toInt() == FileItem::Directory) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        painter->save();

        // paint background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(option.rect, option.palette.base());
        }

        int add = index.data(FileItem::LinesAdded).toInt();
        int sub = index.data(FileItem::LinesRemoved).toInt();
        QString adds = QString(QStringLiteral("+") + QString::number(add));
        QString subs = QString(QStringLiteral(" -") + QString::number(sub));
        QString file = options.text;

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        QRect r = options.rect;

        // don't draw over icon
        r.setX(r.x() + option.decorationSize.width() + 5);

        const QFontMetrics &fm = options.fontMetrics;

        // adds width
        int aw = fm.horizontalAdvance(adds);
        // subs width
        int sw = fm.horizontalAdvance(subs);

        // subtract this from total width of rect
        int totalw = r.width();
        totalw = totalw - (aw + sw);

        // get file name, elide if necessary
        QString filename = fm.elidedText(file, Qt::ElideRight, totalw);

        painter->drawText(r, Qt::AlignVCenter, filename);

        KColorScheme c;
        const QColor red = KColorScheme::shade(c.foreground(KColorScheme::NegativeText).color(), KColorScheme::MidlightShade, 1);
        const QColor green = KColorScheme::shade(c.foreground(KColorScheme::PositiveText).color(), KColorScheme::MidlightShade, 1);

        r.setX(r.x() + totalw);
        painter->setPen(green);
        painter->drawText(r, Qt::AlignVCenter, adds);

        painter->setPen(red);
        r.setX(r.x() + aw);
        painter->drawText(r, Qt::AlignVCenter, subs);

        painter->restore();
    }
};

// Copied from KateProjectWorker
static QStandardItem *directoryParent(const QDir &base, QHash<QString, QStandardItem *> &dir2Item, QString path)
{
    /**
     * throw away simple /
     */
    if (path == QLatin1String("/")) {
        path = QString();
    }

    /**
     * quick check: dir already seen?
     */
    const auto existingIt = dir2Item.find(path);
    if (existingIt != dir2Item.end()) {
        return existingIt.value();
    }

    /**
     * else: construct recursively
     */
    const int slashIndex = path.lastIndexOf(QLatin1Char('/'));

    /**
     * no slash?
     * simple, no recursion, append new item toplevel
     */
    if (slashIndex < 0) {
        const auto item = new FileItem(FileItem::Directory, path);
        item->setData(base.absoluteFilePath(path), Qt::UserRole);
        dir2Item[path] = item;
        dir2Item[QString()]->appendRow(item);
        return item;
    }

    /**
     * else, split and recurse
     */
    const QString leftPart = path.left(slashIndex);
    const QString rightPart = path.right(path.size() - (slashIndex + 1));

    /**
     * special handling if / with nothing on one side are found
     */
    if (leftPart.isEmpty() || rightPart.isEmpty()) {
        return directoryParent(base, dir2Item, leftPart.isEmpty() ? rightPart : leftPart);
    }

    /**
     * else: recurse on left side
     */
    const auto item = new FileItem(FileItem::Directory, rightPart);
    item->setData(base.absoluteFilePath(path), Qt::UserRole);
    dir2Item[path] = item;
    directoryParent(base, dir2Item, leftPart)->appendRow(item);
    return item;
}

// Copied from CompareBranchView in KateProject plugin
static void createFileTree(QStandardItem *parent, const QString &basePath, const std::vector<GitFileItem> &files)
{
    QDir dir(basePath);
    const QString dirPath = dir.path() + QLatin1Char('/');
    QHash<QString, QStandardItem *> dir2Item;
    dir2Item[QString()] = parent;
    for (const GitFileItem &file : files) {
        const QString filePath = QString::fromUtf8(file.file);
        /**
         * cheap file name computation
         * we do this A LOT, QFileInfo is very expensive just for this operation
         */
        const int slashIndex = filePath.lastIndexOf(QLatin1Char('/'));
        const QString fileName = (slashIndex < 0) ? filePath : filePath.mid(slashIndex + 1);
        const QString filePathName = (slashIndex < 0) ? QString() : filePath.left(slashIndex);
        const QString fullFilePath = dirPath + filePath;

        /**
         * construct the item with right directory prefix
         * already hang in directories in tree
         */
        auto *fileItem = new FileItem(FileItem::File, fileName);
        fileItem->setData(fullFilePath, FileItem::Path);
        fileItem->setData(file.linesAdded, FileItem::LinesAdded);
        fileItem->setData(file.linesRemoved, FileItem::LinesRemoved);
        if (!file.oldFileName.isEmpty()) {
            const auto oldFileName = QString::fromUtf8(file.oldFileName);
            fileItem->setData(oldFileName, FileItem::OldPath);
            fileItem->setData(filePath, FileItem::NewPath);
            // we just show name here
            fileItem->setText(QStringLiteral("%1 => %2").arg(oldFileName, fileName));
        }

        // put in our item to the right directory parent
        directoryParent(dir, dir2Item, filePathName)->appendRow(fileItem);
    }
}

static void parseNumStat(const QByteArray &raw, std::vector<GitFileItem> *items)
{
    const auto splitter = ByteArraySplitter(raw, '\0');
    for (auto it = splitter.begin(); it != splitter.end(); ++it) {
        const auto line = *it;
        // format: 12(adds)\t10(subs)\tFileName
        auto lineSplitter = ByteArraySplitter(line, '\t');
        const auto cols = lineSplitter.toContainer<QVarLengthArray<strview, 3>>();
        if (cols.length() < 3) {
            continue;
        }

        int add = 0;
        int sub = 0;
        if (cols[0] == "-") {
            add = sub = 0;
        } else {
            auto col0 = cols[0].to<int>();
            auto col1 = cols[1].to<int>();
            if (!col0 || !col1) {
                continue;
            }
            add = col0.value();
            sub = col1.value();
        }

        if (cols[2].empty()) {
            it++; // skip this
            if (it == splitter.end()) {
                qWarning() << "Unexpected EOF when parsing numstat";
                break;
            }
            auto oldFileName = (*it).toByteArray().trimmed();
            it++;
            if (it == splitter.end()) {
                qWarning() << "Unexpected EOF when parsing numstat";
                break;
            }
            auto file = (*it).toByteArray().trimmed();
            items->push_back(GitFileItem{
                .oldFileName = oldFileName,
                .file = file,
                .linesAdded = add,
                .linesRemoved = sub,
            });
        } else {
            items->push_back(GitFileItem{
                .oldFileName = {},
                .file = cols[2].toByteArray(),
                .linesAdded = add,
                .linesRemoved = sub,
            });
        }
    }
}

class CommitDiffTreeView : public QWidget
{
public:
    explicit CommitDiffTreeView(const QString &repoBase, const QString &hash, KTextEditor::MainWindow *mainWindow, QWidget *parent);

    /**
     * open treeview for commit with @p hash
     * @filePath can be path of any file in the repo
     */
    void openCommit(const QString &filePath);

public:
    void createFileTreeForCommit(const QByteArray &rawNumStat);

private:
    void showDiff(const QModelIndex &idx);
    void openContextMenu(QPoint pos);

private:
    KTextEditor::MainWindow *m_mainWindow;
    QToolButton m_backBtn;
    QTreeView m_tree;
    QStandardItemModel m_model;
    QString m_gitDir;
    QString m_commitHash;
    QLabel m_icon;
    QLabel m_label;
};

CommitDiffTreeView::CommitDiffTreeView(const QString &repoBase, const QString &hash, KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_gitDir(repoBase)
{
    auto vLayout = new QVBoxLayout;
    setLayout(vLayout);
    layout()->setContentsMargins({});
    layout()->setSpacing(0);

    m_backBtn.setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    m_backBtn.setAutoRaise(true);
    m_backBtn.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    connect(&m_backBtn, &QAbstractButton::clicked, this, [parent, mainWindow] {
        Q_ASSERT(parent);
        parent->deleteLater();
        mainWindow->hideToolView(parent);
    });

    // Label
    m_label.setText(hash.left(7));
    m_label.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_icon.setPixmap(QIcon::fromTheme(QStringLiteral("vcs-commit")).pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize)));

    // Horiz layout
    auto *hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin), 0, 0, 0);
    hLayout->addWidget(&m_icon);
    hLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2);
    hLayout->addWidget(&m_label);
    hLayout->addWidget(&m_backBtn);

    vLayout->addLayout(hLayout);

    m_tree.setModel(&m_model);
    layout()->addWidget(&m_tree);

    m_tree.setHeaderHidden(true);
    m_tree.setEditTriggers(QTreeView::NoEditTriggers);
    m_tree.setItemDelegate(new DiffStyleDelegate(this));
    m_tree.setIndentation(10);
    m_tree.setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::TopEdge}));

    m_tree.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&m_tree, &QTreeView::customContextMenuRequested, this, &CommitDiffTreeView::openContextMenu);

    connect(&m_tree, &QTreeView::clicked, this, &CommitDiffTreeView::showDiff);
    openCommit(hash);
}

void CommitDiffTreeView::openContextMenu(QPoint pos)
{
    const auto idx = m_tree.indexAt(pos);
    if (!idx.isValid()) {
        return;
    }

    const auto file = idx.data(FileItem::Path).toString();
    QFileInfo fi(file);

    QMenu menu(this);
    if (fi.exists() && fi.isFile()) {
        menu.addAction(i18n("Open File"), this, [this, fi] {
            m_mainWindow->openUrl(QUrl::fromLocalFile(fi.absoluteFilePath()));
        });
    }

    if (qApp->clipboard()) {
        menu.addAction(i18n("Copy Location"), this, [fi] {
            qApp->clipboard()->setText(fi.absoluteFilePath());
        });
    }

    menu.exec(m_tree.viewport()->mapToGlobal(pos));
}

void CommitDiffTreeView::openCommit(const QString &hash)
{
    m_commitHash = hash;

    auto *git = new QProcess(this);
    if (!setupGitProcess(*git,
                         m_gitDir,
                         {QStringLiteral("show"), hash, QStringLiteral("--numstat"), QStringLiteral("--pretty=oneline"), QStringLiteral("-z")})) {
        delete git;
        return;
    }

    connect(git, &QProcess::finished, this, [this, git](int e, QProcess::ExitStatus s) {
        git->deleteLater();
        if (e != 0 || s != QProcess::NormalExit) {
            Utils::showMessage(QString::fromUtf8(git->readAllStandardError()), gitIcon(), i18n("Git"), MessageType::Error, m_mainWindow);
            m_backBtn.click();
            return;
        }
        QByteArray contents = git->readAllStandardOutput();
        int firstNull = contents.indexOf(char(0x00));
        if (firstNull == -1) {
            return;
        }
        QByteArray numstat = contents.mid(firstNull + 1);
        createFileTreeForCommit(numstat);
    });
    startHostProcess(*git);
}

void CommitDiffTreeView::createFileTreeForCommit(const QByteArray &rawNumStat)
{
    QStandardItem root;
    std::vector<GitFileItem> items;
    parseNumStat(rawNumStat, &items);
    createFileTree(&root, m_gitDir, items);

    // Remove nodes that have only one item. i.e.,
    // - kate
    // -- addons
    //    -- file 1
    // kate will be removed since it has only item
    // The tree will start from addons instead.
    QList<QStandardItem *> tree = root.takeColumn(0);
    while (tree.size() == 1) {
        QStandardItem *subRoot = tree.takeFirst();
        QList<QStandardItem *> subTree = subRoot->takeColumn(0);

        // if its just one file
        if (subTree.isEmpty()) {
            tree.append(subRoot);
            break;
        }

        if (subTree.size() > 1) {
            tree.append(subRoot);
            subRoot->insertColumn(0, subTree);
            break;
        } else {
            // Is the only child of this node a "File" item?
            if (subTree.first()->data(FileItem::TypeRole).toInt() == FileItem::File) {
                subRoot->insertColumn(0, subTree);
                tree.append(subRoot);
                break;
            }

            delete subRoot;
            tree = subTree;
        }
    }

    m_model.clear();
    m_model.invisibleRootItem()->appendColumn(tree);

    m_tree.expandAll();
}

void CommitDiffTreeView::showDiff(const QModelIndex &idx)
{
    const QString file = idx.data(FileItem::Path).toString();
    const QString oldFile = idx.data(FileItem::OldPath).toString();
    QProcess git;
    if (oldFile.isEmpty()) {
        if (!setupGitProcess(git, m_gitDir, {QStringLiteral("show"), m_commitHash, QStringLiteral("--"), file})) {
            return;
        }
    } else {
        if (!setupGitProcess(git, m_gitDir, {QStringLiteral("show"), m_commitHash, QStringLiteral("--"), oldFile, file})) {
            return;
        }
    }

    startHostProcess(git, QProcess::ReadOnly);

    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return;
        }
    }

    DiffParams d;
    if (!oldFile.isEmpty()) {
        d.tabTitle = QStringLiteral("Renamed %1 => %2").arg(oldFile, idx.data(FileItem::NewPath).toString());
    }
    d.srcFile = file;
    d.flags.setFlag(DiffParams::ShowCommitInfo);
    d.workingDir = m_gitDir;
    if (m_tree.model()->rowCount(idx) > 1) {
        d.flags.setFlag(DiffParams::ShowFileName);
    }
    Utils::showDiff(git.readAllStandardOutput(), d, m_mainWindow);
}

void CommitView::openCommit(const QString &hash, const QString &path, KTextEditor::MainWindow *mainWindow)
{
    QFileInfo fi(path);
    if (!fi.exists()) {
        qWarning("Unexpected non-existent file: %ls", qUtf16Printable(path));
        return;
    }

    if (hash.length() < 7 && hash != QStringLiteral("HEAD")) {
        Utils::showMessage(i18n("Invalid hash"), gitIcon(), i18n("Git"), MessageType::Error, mainWindow);
        return;
    }

    const std::optional<QString> repoBase = getRepoBasePath(fi.absolutePath());
    if (!repoBase.has_value()) {
        Utils::showMessage(i18n("%1 doesn't exist in a git repo.", path), gitIcon(), i18n("Git"), MessageType::Error, mainWindow);
        return;
    }

    if (!mainWindow) {
        mainWindow = KTextEditor::Editor::instance()->application()->activeMainWindow();
    }

    QWidget *toolView = Utils::toolviewForName(mainWindow, QStringLiteral("git_commit_view_%1").arg(hash));
    if (!toolView) {
        const auto icon = QIcon::fromTheme(QStringLiteral("vcs-commit"));
        toolView = mainWindow->createToolView(nullptr,
                                              QStringLiteral("git_commit_view_%1").arg(hash),
                                              KTextEditor::MainWindow::Left,
                                              icon,
                                              i18nc("@title:tab", "Commit %1", hash.mid(0, 7)));
        new CommitDiffTreeView(repoBase.value(), hash, mainWindow, toolView);
    }
    mainWindow->showToolView(toolView);
}
