/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "filehistorywidget.h"
#include "commitfilesview.h"
#include "diffparams.h"
#include "hostprocess.h"
#include "ktexteditor_utils.h"
#include <bytearraysplitter.h>
#include <gitprocess.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QPointer>
#include <QProcess>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>

struct Commit {
    QByteArray hash;
    QString authorName;
    QString email;
    qint64 authorDate;
    qint64 commitDate;
    QByteArray parentHash;
    QString msg;
    QByteArray fileName;
};

static std::vector<Commit> parseCommits(const QByteArray &raw)
{
    std::vector<Commit> commits;

    const auto splitted = ByteArraySplitter(raw, '\0');
    for (auto it = splitted.begin(); it != splitted.end(); ++it) {
        const auto commitDetails = *it;
        if (commitDetails.empty()) {
            continue;
        }

        const auto lines = ByteArraySplitter(commitDetails, '\n').toContainer<QVarLengthArray<strview, 7>>();
        if (lines.length() < 7) {
            continue;
        }

        QByteArray hash = lines.at(0).toByteArray();
        QString author = lines.at(1).toString();
        QString email = lines.at(2).toString();

        std::optional<long long> authDate = lines.at(3).to<qint64>();
        if (!authDate.has_value()) {
            continue;
        }
        qint64 authorDate = authDate.value();

        std::optional<long long> commtDate = lines.at(4).to<qint64>();
        if (!commtDate.has_value()) {
            continue;
        }
        qint64 commitDate = commtDate.value();

        QByteArray parent = lines.at(5).toByteArray();
        QString msg = lines.at(6).toString();

        QByteArray file;
        ++it;
        if (it != splitted.end()) {
            file = (*it).toByteArray().trimmed();
        }

        Commit c{.hash = hash,
                 .authorName = author,
                 .email = email,
                 .authorDate = authorDate,
                 .commitDate = commitDate,
                 .parentHash = parent,
                 .msg = msg,
                 .fileName = file};
        commits.push_back(std::move(c));
    }

    return commits;
}

class CommitListModel : public QAbstractListModel
{
public:
    CommitListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    enum Role {
        CommitRole = Qt::UserRole + 1
    };

    int rowCount(const QModelIndex &) const override
    {
        return (int)m_rows.size();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) {
            return {};
        }
        auto row = index.row();
        switch (role) {
        case Role::CommitRole:
            return QVariant::fromValue(m_rows.at(row));
        case Qt::ToolTipRole: {
            QString ret = m_rows.at(row).authorName + QStringLiteral("<br>") + m_rows.at(row).email;
            return ret;
        }
        }

        return {};
    }

    void addCommits(std::vector<Commit> &&cmts)
    {
        beginInsertRows(QModelIndex(), (int)m_rows.size(), (int)m_rows.size() + (int)cmts.size() - 1);
        m_rows.insert(m_rows.end(), std::make_move_iterator(cmts.begin()), std::make_move_iterator(cmts.end()));
        endInsertRows();
    }

    const Commit &commit(int row)
    {
        return m_rows[row];
    }

private:
    std::vector<Commit> m_rows;
};

struct Filter {
    int id;
    QString text;
};

class CommitProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &) const override
    {
        bool accept = true;
        auto model = static_cast<CommitListModel *>(sourceModel());
        const auto &commit = model->commit(sourceRow);

        if (!m_authorFilters.empty()) {
            accept = std::any_of(m_authorFilters.begin(), m_authorFilters.end(), [&commit](const Filter &f) {
                return commit.authorName.contains(f.text, Qt::CaseInsensitive);
            });
        }

        if (accept && !m_messageFilters.empty()) {
            accept = std::all_of(m_messageFilters.begin(), m_messageFilters.end(), [&commit](const Filter &f) {
                return commit.msg.contains(f.text, Qt::CaseInsensitive);
            });
        }

        return accept;
    }

    int appendFilter(const QString &data)
    {
        beginResetModel();
        int id = m_filterIdCounter++;

        if (data.startsWith(u"a:")) {
            m_authorFilters.push_back(Filter{id, data.mid(2)});
        } else {
            m_messageFilters.push_back(Filter{id, data});
        }

        endResetModel();

        return id;
    }

    void removeFilter(int id)
    {
        auto pred = [id](const Filter &f) {
            return f.id == id;
        };
        beginResetModel();

        std::erase_if(m_authorFilters, pred);
        std::erase_if(m_messageFilters, pred);

        endResetModel();
    }

private:
    int m_filterIdCounter = 0;
    std::vector<Filter> m_authorFilters;
    std::vector<Filter> m_messageFilters;
};

class CommitDelegate : public QStyledItemDelegate
{
    static constexpr int Margin = 5; // Is there a pixel metric for this?
    static constexpr int LineHeight = 2;

public:
    CommitDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const override
    {
        auto commit = index.data(CommitListModel::CommitRole).value<Commit>();
        if (commit.hash.isEmpty()) {
            return;
        }

        QStyleOptionViewItem options = opt;
        initStyleOption(&options, index);

        options.text = QString();
        QStyledItemDelegate::paint(painter, options, index);

        QFontMetrics fm = opt.fontMetrics;
        QRect prect = opt.rect;

        // padding
        prect.setX(prect.x() + Margin);
        prect.setY(prect.y() + Margin);
        prect.setWidth(prect.width() - Margin);

        auto primaryColor = options.palette.color(QPalette::ColorRole::Text);
        auto secondaryColor = QColor(Qt::gray);
        if (options.state.testFlag(QStyle::State_Selected) && options.state.testFlag(QStyle::State_Active)) {
            primaryColor = options.palette.color(QPalette::ColorRole::HighlightedText);
            secondaryColor = primaryColor;
        }

        // draw author on left
        QFont f = opt.font;
        f.setBold(true);
        painter->setFont(f);
        painter->setPen(primaryColor);
        painter->drawText(prect, Qt::AlignLeft, commit.authorName);
        painter->setFont(opt.font);

        // draw author date on right
        auto dt = QDateTime::fromSecsSinceEpoch(commit.authorDate);
        QLocale l;
        const bool isToday = dt.date() == QDate::currentDate();
        QString timestamp = isToday ? l.toString(dt.time(), QLocale::ShortFormat) : l.toString(dt.date(), QLocale::ShortFormat);
        painter->drawText(prect, Qt::AlignRight, timestamp);

        // draw commit hash
        auto fg = painter->pen();
        painter->setPen(secondaryColor);
        prect.setY(prect.y() + fm.height() + LineHeight);
        painter->drawText(prect, Qt::AlignLeft, QString::fromUtf8(commit.hash.left(7)));
        painter->setPen(fg);

        // draw msg
        prect.setY(prect.y() + fm.height() + LineHeight);
        QString elidedMsg = opt.fontMetrics.elidedText(commit.msg, Qt::ElideRight, prect.width());
        painter->drawText(prect, Qt::AlignLeft, elidedMsg);

        // draw separator
        painter->setPen(opt.palette.button().color());
        painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());
        painter->setPen(fg);
    }

    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &) const override
    {
        auto height = opt.fontMetrics.height();
        return QSize(0, (height * 3) + (Margin * 2) + (LineHeight * 2));
    }
};

class FileHistoryWidget : public QWidget
{
    Q_OBJECT
public:
    void itemClicked(const QModelIndex &idx);
    void onContextMenu(QPoint pos);
    void onFilterReturnPressed(CommitProxyModel *proxy);

    void getFileHistory(const QString &file);
    explicit FileHistoryWidget(const QString &gitDir, const QString &file, KTextEditor::MainWindow *mw, QWidget *parent = nullptr);
    ~FileHistoryWidget() override;

    QLabel m_icon;
    QLabel m_label;

    QToolButton m_backBtn;
    QListView m_listView;
    QLineEdit m_filterLineEdit;
    QListWidget m_filtersListWidget;
    QProcess m_git;
    const QString m_file;
    const QString m_gitDir;
    const QPointer<QWidget> m_toolView;
    const QPointer<KTextEditor::MainWindow> m_mainWindow;
    QPointer<QWidget> m_lastActiveToolview;

Q_SIGNALS:
    void backClicked();
    void errorMessage(const QString &msg, bool warn);
};

FileHistoryWidget::FileHistoryWidget(const QString &gitDir, const QString &file, KTextEditor::MainWindow *mw, QWidget *parent)
    : QWidget(parent)
    , m_file(file)
    , m_gitDir(gitDir)
    , m_toolView(parent)
    , m_mainWindow(mw)
{
    auto model = new CommitListModel(this);
    auto proxy = new CommitProxyModel(this);
    proxy->setSourceModel(model);

    m_listView.setModel(proxy);
    m_listView.setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::TopEdge}));
    getFileHistory(file);

    m_filtersListWidget.setFlow(QListView::LeftToRight);
    m_filtersListWidget.setWrapping(true);
    m_filtersListWidget.setVisible(false);
    m_filtersListWidget.setSpacing(2);
    m_filtersListWidget.setSizeAdjustPolicy(QListView::SizeAdjustPolicy::AdjustToContents);

    auto vLayout = new QVBoxLayout;
    setLayout(vLayout);
    layout()->setContentsMargins({});
    layout()->setSpacing(0);

    m_backBtn.setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    m_backBtn.setAutoRaise(true);
    m_backBtn.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    connect(&m_backBtn, &QAbstractButton::clicked, this, [this] {
        deleteLater();
        m_mainWindow->hideToolView(m_toolView);
        m_toolView->deleteLater();
        if (m_lastActiveToolview) {
            m_mainWindow->showToolView(m_lastActiveToolview.data());
        }
    });
    connect(&m_listView, &QListView::clicked, this, &FileHistoryWidget::itemClicked);
    connect(&m_listView, &QListView::activated, this, &FileHistoryWidget::itemClicked);

    m_listView.setItemDelegate(new CommitDelegate(this));
    m_listView.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&m_listView, &QListView::customContextMenuRequested, this, &FileHistoryWidget::onContextMenu);

    m_filterLineEdit.setPlaceholderText(i18n("Filter…"));
    auto helpAction = new QAction(this);
    helpAction->setIcon(QIcon::fromTheme(QStringLiteral("info")));
    connect(helpAction, &QAction::triggered, this, [this] {
        const auto pos = mapToGlobal(m_filterLineEdit.rect().bottomRight());
        const QString text = i18nc("Help text",
                                   "Filter the log."
                                   "<ul>"
                                   "<li>Use \"a:\" prefix to filter by author name. For e.g., <b>a:john doe</b></li>"
                                   "<li>To filter messages, just type the search term</li>"
                                   "</ul>"
                                   "To trigger the filter, press the <b>Enter</b> key. ");
        QToolTip::showText(pos, text, this);
    });
    m_filterLineEdit.addAction(helpAction, QLineEdit::TrailingPosition);
    connect(&m_filterLineEdit, &QLineEdit::returnPressed, this, [this, proxy]() {
        onFilterReturnPressed(proxy);
    });

    // Label
    QFileInfo fi(file);
    m_label.setText(fi.fileName());
    m_label.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_icon.setPixmap(QIcon::fromTheme(QStringLiteral("view-history")).pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize)));

    // Horiz layout
    auto *hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin), 0, 0, 0);
    hLayout->addWidget(&m_icon);
    hLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2);
    hLayout->addWidget(&m_label);
    hLayout->addWidget(&m_backBtn);

    vLayout->addLayout(hLayout);
    vLayout->addWidget(&m_filterLineEdit);
    vLayout->addWidget(&m_filtersListWidget);
    vLayout->addWidget(&m_listView, 99);
}

FileHistoryWidget::~FileHistoryWidget()
{
    m_git.kill();
    m_git.waitForFinished();
}

// git log --format=%H%n%aN%n%aE%n%at%n%ct%n%P%n%B --author-date-order
void FileHistoryWidget::getFileHistory(const QString &file)
{
    if (!setupGitProcess(m_git,
                         m_gitDir,
                         {QStringLiteral("log"),
                          QStringLiteral("--follow"), // get history accross renames
                          QStringLiteral("--name-only"), // get file name also, it could be different if renamed
                          QStringLiteral("--format=%H%n%aN%n%aE%n%at%n%ct%n%P%n%B"),
                          QStringLiteral("-z"),
                          file})) {
        Q_EMIT errorMessage(i18n("Failed to get file history: git executable not found in PATH"), true);
        return;
    }

    connect(&m_git, &QProcess::readyReadStandardOutput, this, [this] {
        std::vector<Commit> commits = parseCommits(m_git.readAllStandardOutput());
        if (!commits.empty()) {
            auto proxy = static_cast<CommitProxyModel *>(m_listView.model());
            static_cast<CommitListModel *>(proxy->sourceModel())->addCommits(std::move(commits));
        }
    });

    connect(&m_git, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus s) {
        if (exitCode != 0 || s != QProcess::NormalExit) {
            Q_EMIT errorMessage(i18n("Failed to get file history: %1", QString::fromUtf8(m_git.readAllStandardError())), true);
        }
    });

    startHostProcess(m_git, QProcess::ReadOnly);
}

void FileHistoryWidget::onContextMenu(QPoint pos)
{
    QMenu menu(m_listView.viewport());

    menu.addAction(i18nc("@menu:action", "Copy Commit Hash"), this, [this, pos] {
        const auto index = m_listView.indexAt(pos);
        const auto commit = index.data(CommitListModel::CommitRole).value<Commit>();
        if (!commit.hash.isEmpty()) {
            qApp->clipboard()->setText(QString::fromLatin1(commit.hash));
        }
    });

    menu.addAction(i18nc("@menu:action", "Show Full Commit"), this, [this, pos] {
        const auto index = m_listView.indexAt(pos);
        const auto commit = index.data(CommitListModel::CommitRole).value<Commit>();
        if (!commit.hash.isEmpty()) {
            const QString hash = QString::fromLatin1(commit.hash);
            CommitView::openCommit(hash, m_file, m_mainWindow);
        }
    });

    menu.exec(m_listView.viewport()->mapToGlobal(pos));
}

void FileHistoryWidget::onFilterReturnPressed(CommitProxyModel *proxy)
{
    int id = proxy->appendFilter(m_filterLineEdit.text());

    if (m_filtersListWidget.isHidden()) {
        m_filtersListWidget.setVisible(true);
    }

    auto widget = new QWidget(this);
    QPalette p = this->palette();
    p.setBrush(QPalette::Base, p.window());

    widget->setPalette(p);
    widget->setAutoFillBackground(true);
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    auto label = new QLabel;
    label->setContentsMargins(2, 0, 0, 0);
    label->setText(m_filterLineEdit.text());
    layout->addWidget(label);

    auto closeBtn = new QToolButton;
    closeBtn->setAutoRaise(true);
    closeBtn->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    layout->addWidget(closeBtn);

    auto item = new QListWidgetItem(m_filterLineEdit.text().append(QStringLiteral("XXX")));
    item->setData(Qt::UserRole + 1, id);
    m_filtersListWidget.addItem(item);

    connect(closeBtn, &QToolButton::clicked, this, [proxy, item, id] {
        delete item;
        proxy->removeFilter(id);
    });

    m_filtersListWidget.setItemWidget(item, widget);

    m_filterLineEdit.clear();
}

void FileHistoryWidget::itemClicked(const QModelIndex &idx)
{
    QProcess git;

    const auto commit = idx.data(CommitListModel::CommitRole).value<Commit>();
    const QString file = QString::fromUtf8(commit.fileName);

    if (!setupGitProcess(git, m_gitDir, {QStringLiteral("show"), QString::fromUtf8(commit.hash), QStringLiteral("--"), file})) {
        return;
    }

    startHostProcess(git, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return;
        }
        const QByteArray contents(git.readAllStandardOutput());

        DiffParams d;
        const QString shortCommit = QString::fromUtf8(commit.hash.mid(0, 7));
        d.tabTitle = QStringLiteral("%1[%2]").arg(Utils::fileNameFromPath(m_file), shortCommit);
        d.flags.setFlag(DiffParams::ShowCommitInfo);
        d.flags.setFlag(DiffParams::ShowFullContext);
        d.arguments = git.arguments();
        d.workingDir = m_gitDir;
        Utils::showDiff(contents, d, m_mainWindow);
    }
}

void FileHistory::showFileHistory(const QString &file, KTextEditor::MainWindow *mainWindow)
{
    QFileInfo fi(file);
    if (!fi.exists()) {
        qWarning() << "Unexpected non-existent file: " << file;
        return;
    }

    const std::optional<QString> repoBase = getRepoBasePath(fi.absolutePath());
    if (!repoBase.has_value()) {
        Utils::showMessage(i18n("%1 doesn't exist in a git repo.", file), gitIcon(), i18n("Git"), MessageType::Error, mainWindow);
        return;
    }

    if (!mainWindow) {
        mainWindow = KTextEditor::Editor::instance()->application()->activeMainWindow();
    }

    const QString identifier = QStringLiteral("git_file_history_%1").arg(file);
    const QString title = i18nc("@title:tab", "File History - %1", fi.fileName());
    auto toolView = Utils::toolviewForName(mainWindow, identifier);
    auto activeToolview = Utils::activeToolviewForSide(mainWindow, KTextEditor::MainWindow::Left);
    if (!toolView) {
        toolView = mainWindow->createToolView(nullptr, identifier, KTextEditor::MainWindow::Left, gitIcon(), title);
        new FileHistoryWidget(repoBase.value(), file, mainWindow, toolView);
    }
    if (auto w = toolView->findChild<FileHistoryWidget *>()) {
        w->m_lastActiveToolview = activeToolview;
    }

    mainWindow->showToolView(toolView);
}

#include "filehistorywidget.moc"
