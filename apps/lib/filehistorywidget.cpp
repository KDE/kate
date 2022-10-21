/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "filehistorywidget.h"
#include "diffparams.h"
#include "ktexteditor_utils.h"
#include <bytearraysplitter.h>
#include <gitprocess.h>

#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QListView>
#include <QPainter>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <QWidget>

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>

#include <charconv>

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
Q_DECLARE_METATYPE(Commit)

static QVector<Commit> parseCommits(const QByteArray raw)
{
    QVector<Commit> commits;

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

        auto authDate = lines.at(3).to<qint64>();
        if (!authDate.has_value()) {
            continue;
        }
        qint64 authorDate = authDate.value();

        auto commtDate = lines.at(4).to<qint64>();
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

        commits << Commit{hash, author, email, authorDate, commitDate, parent, msg, file};
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

    enum Role { CommitRole = Qt::UserRole + 1 };

    int rowCount(const QModelIndex &) const override
    {
        return m_rows.count();
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

    void refresh(const QVector<Commit> &cmts)
    {
        beginResetModel();
        m_rows = cmts;
        endResetModel();
    }

    void addCommit(Commit cmt)
    {
        beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size());
        m_rows.push_back(cmt);
        endInsertRows();
    }

    void addCommits(const QVector<Commit> &cmts)
    {
        for (const auto &commit : cmts) {
            addCommit(commit);
        }
    }

private:
    QVector<Commit> m_rows;
};

class CommitDelegate : public QStyledItemDelegate
{
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

        constexpr int lineHeight = 2;
        QFontMetrics fm = opt.fontMetrics;

        QRect prect = opt.rect;

        // padding
        prect.setX(prect.x() + 5);
        prect.setY(prect.y() + lineHeight);

        // draw author on left
        QFont f = opt.font;
        f.setBold(true);
        painter->setFont(f);
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
        painter->setPen(Qt::gray);
        prect.setY(prect.y() + fm.height() + lineHeight);
        painter->drawText(prect, Qt::AlignLeft, QString::fromUtf8(commit.hash.left(7)));
        painter->setPen(fg);

        // draw msg
        prect.setY(prect.y() + fm.height() + lineHeight);
        auto elidedMsg = opt.fontMetrics.elidedText(commit.msg, Qt::ElideRight, prect.width());
        painter->drawText(prect, Qt::AlignLeft, elidedMsg);

        // draw separator
        painter->setPen(opt.palette.button().color());
        painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());
        painter->setPen(fg);
    }

    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &) const override
    {
        auto height = opt.fontMetrics.height();
        return QSize(0, height * 3 + (3 * 2));
    }
};

class FileHistoryWidget : public QWidget
{
    Q_OBJECT
public:
    void itemClicked(const QModelIndex &idx);

    void getFileHistory(const QString &file);
    explicit FileHistoryWidget(const QString &gitDir, const QString &file, KTextEditor::MainWindow *mw, QWidget *parent = nullptr);
    ~FileHistoryWidget() override;

    QPushButton m_backBtn;
    QListView *m_listView;
    QProcess m_git;
    const QString m_file;
    const QString m_gitDir;
    const QPointer<QWidget> m_toolView;
    const QPointer<KTextEditor::MainWindow> m_mainWindow;

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
    m_listView = new QListView;
    m_listView->setModel(model);
    getFileHistory(file);

    setLayout(new QVBoxLayout);

    m_backBtn.setText(i18n("Close"));
    m_backBtn.setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    connect(&m_backBtn, &QPushButton::clicked, this, [this] {
        deleteLater();
        m_mainWindow->hideToolView(m_toolView);
        m_toolView->deleteLater();
    });
    connect(m_listView, &QListView::clicked, this, &FileHistoryWidget::itemClicked);

    m_listView->setItemDelegate(new CommitDelegate(this));

    layout()->addWidget(&m_backBtn);
    layout()->addWidget(m_listView);
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
        auto commits = parseCommits(m_git.readAllStandardOutput());
        if (!commits.isEmpty()) {
            static_cast<CommitListModel *>(m_listView->model())->addCommits(commits);
        }
    });

    connect(&m_git, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus s) {
        if (exitCode != 0 || s != QProcess::NormalExit) {
            Q_EMIT errorMessage(i18n("Failed to get file history: %1", QString::fromUtf8(m_git.readAllStandardError())), true);
        }
    });

    startHostProcess(m_git, QProcess::ReadOnly);
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

    const auto repoBase = getRepoBasePath(fi.absolutePath());
    if (!repoBase.has_value()) {
        Utils::showMessage(i18n("%1 doesn't exist in a git repo.", file), gitIcon(), i18n("Git"), i18n("Error"), mainWindow);
        return;
    }

    if (!mainWindow) {
        mainWindow = KTextEditor::Editor::instance()->application()->activeMainWindow();
    }

    auto toolView = mainWindow->createToolView(nullptr, QStringLiteral("git_file_history"), KTextEditor::MainWindow::Left, gitIcon(), i18n("File History"));
    new FileHistoryWidget(repoBase.value(), file, mainWindow, toolView);
    mainWindow->showToolView(toolView);
}

#include "filehistorywidget.moc"
