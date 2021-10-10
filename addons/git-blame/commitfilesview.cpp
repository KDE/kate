/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "commitfilesview.h"

#include <gitprocess.h>

#include <QDir>
#include <QMimeDatabase>
#include <QPainter>
#include <QProcess>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QVBoxLayout>

#include <KColorScheme>
#include <KLocalizedString>

#include <optional>

/**
 * Class representing a item inside the treeview
 * Copied from KateProject with modifications as needed
 */
class FileItem : public QStandardItem
{
public:
    enum Type { Directory = 1, File = 2 };
    enum Role {
        Path = Qt::UserRole,
        TypeRole,
        LinesAdded,
        LinesRemoved,
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
    DiffStyleDelegate(QObject *parent)
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
        const auto red = c.shade(c.foreground(KColorScheme::NegativeText).color(), KColorScheme::MidlightShade, 1);
        const auto green = c.shade(c.foreground(KColorScheme::PositiveText).color(), KColorScheme::MidlightShade, 1);

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
static void createFileTree(QStandardItem *parent, const QString &basePath, const QVector<GitFileItem> &files)
{
    QDir dir(basePath);
    const QString dirPath = dir.path() + QLatin1Char('/');
    QHash<QString, QStandardItem *> dir2Item;
    dir2Item[QString()] = parent;
    for (const auto &file : qAsConst(files)) {
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
        FileItem *fileItem = new FileItem(FileItem::File, fileName);
        fileItem->setData(fullFilePath, FileItem::Path);
        fileItem->setData(file.linesAdded, FileItem::LinesAdded);
        fileItem->setData(file.linesRemoved, FileItem::LinesRemoved);

        // put in our item to the right directory parent
        directoryParent(dir, dir2Item, filePathName)->appendRow(fileItem);
    }
}

static std::optional<QString> getDotGitPath(const QString &repo)
{
    /* This call is intentionally blocking because we need git path for everything else */
    QProcess git;
    setupGitProcess(git, repo, {QStringLiteral("rev-parse"), QStringLiteral("--absolute-git-dir")});
    git.start(QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return std::nullopt;
        }
        QString dotGitPath = QString::fromUtf8(git.readAllStandardOutput());
        if (dotGitPath.endsWith(QLatin1String("\n"))) {
            dotGitPath.remove(QLatin1String(".git\n"));
        } else {
            dotGitPath.remove(QLatin1String(".git"));
        }
        return dotGitPath;
    }
    return std::nullopt;
}

static bool getNum(const QByteArray &numBytes, int *num)
{
    bool res = false;
    *num = numBytes.toInt(&res);
    return res;
}

static QVector<GitFileItem> parseNumStat(const QByteArray &raw)
{
    QVector<GitFileItem> items;

    const auto lines = raw.split(0x00);
    for (const auto &line : lines) {
        // format: 12(adds)\t10(subs)\tFileName
        const auto cols = line.split('\t');
        if (cols.length() < 3) {
            continue;
        }

        int add = 0;
        if (!getNum(cols.at(0), &add)) {
            continue;
        }
        int sub = 0;
        if (!getNum(cols.at(1), &sub)) {
            continue;
        }

        const auto file = cols.at(2);

        items << GitFileItem{file, add, sub};
    }

    return items;
}

CommitDiffTreeView::CommitDiffTreeView(QWidget *parent)
    : QWidget(parent)
{
    setLayout(new QVBoxLayout);

    m_backBtn.setText(i18n("Close"));
    m_backBtn.setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
    connect(&m_backBtn, &QPushButton::clicked, this, &CommitDiffTreeView::closeRequested);
    layout()->addWidget(&m_backBtn);

    m_tree.setModel(&m_model);
    layout()->addWidget(&m_tree);

    m_tree.setHeaderHidden(true);
    m_tree.setEditTriggers(QTreeView::NoEditTriggers);
    m_tree.setItemDelegate(new DiffStyleDelegate(this));

    connect(&m_tree, &QTreeView::clicked, this, &CommitDiffTreeView::showDiff);
}

void CommitDiffTreeView::openCommit(const QString &hash, const QString &filePath)
{
    m_commitHash = hash;

    QProcess *git = new QProcess(this);
    setupGitProcess(*git,
                    QFileInfo(filePath).absolutePath(),
                    {QStringLiteral("show"), hash, QStringLiteral("--numstat"), QStringLiteral("--pretty=oneline"), QStringLiteral("-z")});
    connect(git, &QProcess::finished, this, [this, git, filePath](int e, QProcess::ExitStatus s) {
        git->deleteLater();
        if (e != 0 || s != QProcess::NormalExit) {
            return;
        }
        auto contents = git->readAllStandardOutput();
        int firstNull = contents.indexOf(char(0x00));
        if (firstNull == -1) {
            return;
        }
        QByteArray numstat = contents.mid(firstNull + 1);
        createFileTreeForCommit(filePath, numstat);
    });
    git->start();
}

void CommitDiffTreeView::createFileTreeForCommit(const QString &filePath, const QByteArray &rawNumStat)
{
    QFileInfo fi(filePath);
    QString path = fi.absolutePath();
    auto value = getDotGitPath(path);
    if (value.has_value()) {
        m_gitDir = value.value();
    }

    QStandardItem root;
    createFileTree(&root, m_gitDir, parseNumStat(rawNumStat));

    // Remove nodes that have only one item. i.e.,
    // - kate
    // -- addons
    //    -- file 1
    // kate will be removed since it has only item
    // The tree will start from addons instead.
    QList<QStandardItem *> tree = root.takeColumn(0);
    while (tree.size() == 1) {
        auto subRoot = tree.takeFirst();
        auto subTree = subRoot->takeColumn(0);

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
    auto file = idx.data(FileItem::Path).toString().remove(m_gitDir + QLatin1Char('/'));
    QProcess git;
    setupGitProcess(git, m_gitDir, {QStringLiteral("show"), m_commitHash, QStringLiteral("--"), file});
    git.start(QProcess::ReadOnly);

    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return;
        }
    }

    Q_EMIT showDiffRequested(git.readAllStandardOutput());
}
