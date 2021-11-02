/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "filehistorywidget.h"

#include <gitprocess.h>

#include <QDate>
#include <QFileInfo>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <optional>

#include <KLocalizedString>

struct Commit {
    QByteArray hash;
    QString authorName;
    QString email;
    qint64 authorDate;
    qint64 commitDate;
    QByteArray parentHash;
    QString msg;
};
Q_DECLARE_METATYPE(Commit)

static std::optional<qint64> toLong(const QByteArray &input)
{
    bool ok = false;
    qint64 ret = input.toLongLong(&ok);
    if (ok) {
        return ret;
    }
    return std::nullopt;
}

static QVector<Commit> parseCommits(const QList<QByteArray> &raw)
{
    QVector<Commit> commits;
    commits.reserve(raw.size());

    for (const auto &r : raw) {
        const auto lines = r.split('\n');
        if (lines.length() < 7) {
            continue;
        }
        QByteArray hash = lines.at(0);
        QString author = QString::fromUtf8(lines.at(1));
        QString email = QString::fromUtf8(lines.at(2));

        auto authDate = toLong(lines.at(3));
        if (!authDate.has_value()) {
            continue;
        }
        qint64 authorDate = authDate.value();

        auto commtDate = toLong(lines.at(4));
        if (!commtDate.has_value()) {
            continue;
        }
        qint64 commitDate = commtDate.value();

        QByteArray parent = lines.at(5);
        QString msg = QString::fromUtf8(lines.at(6));
        commits << Commit{hash, author, email, authorDate, commitDate, parent, msg};
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

    enum Role { CommitRole = Qt::UserRole + 1, CommitHash };

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
        case Role::CommitHash:
            return m_rows.at(row).hash;
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

FileHistoryWidget::FileHistoryWidget(const QString &file, QWidget *parent)
    : QWidget(parent)
    , m_file(file)
{
    auto model = new CommitListModel(this);
    m_listView = new QListView;
    m_listView->setModel(model);
    getFileHistory(file);

    setLayout(new QVBoxLayout);

    m_backBtn.setText(i18n("Back"));
    m_backBtn.setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    connect(&m_backBtn, &QPushButton::clicked, this, &FileHistoryWidget::backClicked);

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
    setupGitProcess(m_git,
                    QFileInfo(file).absolutePath(),
                    {QStringLiteral("log"), QStringLiteral("--format=%H%n%aN%n%aE%n%at%n%ct%n%P%n%B"), QStringLiteral("-z"), file});

    connect(&m_git, &QProcess::readyReadStandardOutput, this, [this] {
        auto commits = parseCommits(m_git.readAllStandardOutput().split(0x00));
        if (!commits.isEmpty()) {
            static_cast<CommitListModel *>(m_listView->model())->addCommits(commits);
        }
    });

    connect(&m_git, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus s) {
        if (exitCode != 0 || s != QProcess::NormalExit) {
            Q_EMIT errorMessage(i18n("Failed to get file history: %1", QString::fromUtf8(m_git.readAllStandardError())), true);
        }
    });

    m_git.start(QProcess::ReadOnly);
}

void FileHistoryWidget::itemClicked(const QModelIndex &idx)
{
    QProcess git;
    QFileInfo fi(m_file);

    const auto commit = idx.data(CommitListModel::CommitRole).value<Commit>();

    setupGitProcess(git, fi.absolutePath(), {QStringLiteral("show"), QString::fromUtf8(commit.hash), QStringLiteral("--"), m_file});
    git.start(QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return;
        }
        QByteArray contents(git.readAllStandardOutput());
        // we send this signal to the parent, which will pass it on to
        // the GitWidget from where a temporary file is opened
        Q_EMIT commitClicked(contents);
    }
}
