#include "filehistorywidget.h"

#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QVBoxLayout>

#include <KLocalizedString>

// git log --format=%H%n%aN%n%aE%n%at%n%ct%n%P%n%B --author-date-order
static QList<QByteArray> getFileHistory(const QString &file)
{
    QProcess git;
    git.setWorkingDirectory(QFileInfo(file).absolutePath());
    QStringList args{QStringLiteral("log"),
                     QStringLiteral("--format=%H%n%aN%n%aE%n%at%n%ct%n%P%n%B"),
                     QStringLiteral("-z"),
                     QStringLiteral("--author-date-order"),
                     file};
    git.start(QStringLiteral("git"), args, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        return git.readAll().split(0x00);
    }
    return {};
}

struct Commit {
    QByteArray hash;
    QString authorName;
    QString email;
    qint64 authorDate;
    qint64 commitDate;
    QByteArray parentHash;
    QString msg;
};

static QVector<Commit> parseCommits(const QList<QByteArray> &raw)
{
    QVector<Commit> commits;
    commits.reserve(raw.size());
    std::transform(raw.cbegin(), raw.cend(), std::back_inserter(commits), [](const QByteArray &r) {
        const auto lines = r.split('\n');
        if (lines.length() < 7) {
            return Commit{};
        }
        auto hash = lines.at(0);
        //        qWarning() << hash;
        auto author = QString::fromUtf8(lines.at(1));
        //        qWarning() << author;
        auto email = QString::fromUtf8(lines.at(2));
        //        qWarning() << email;
        qint64 authorDate = lines.at(3).toLong();
        //        qWarning() << authorDate;
        qint64 commitDate = lines.at(4).toLong();
        //        qWarning() << commitDate;
        auto parent = lines.at(5);
        //        qWarning() << parent;
        auto msg = QString::fromUtf8(lines.at(6));
        //        qWarning() << msg;
        return Commit{hash, author, email, authorDate, commitDate, parent, msg};
    });

    return commits;
}

class CommitListModel : public QAbstractListModel
{
public:
    CommitListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    enum Role { Author = Qt::UserRole + 1, Email, Date, Hash };

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
        case Qt::DisplayRole:
            return m_rows[row].msg;
        case Role::Author:
            return m_rows[row].authorName;
        case Role::Email:
            return m_rows[row].email;
        case Role::Date:
            return m_rows[row].commitDate;
        case Role::Hash:
            return m_rows[row].hash;
        }

        return {};
    }

    void refresh(const QVector<Commit> &cmts)
    {
        beginResetModel();
        m_rows = cmts;
        endResetModel();
    }

private:
    QVector<Commit> m_rows;
};

FileHistoryWidget::FileHistoryWidget(const QString &file, QWidget *parent)
    : QWidget(parent)
    , m_file(file)
{
    setLayout(new QVBoxLayout);

    m_backBtn.setText(i18n("Back"));
    connect(&m_backBtn, &QPushButton::clicked, this, &FileHistoryWidget::backClicked);
    layout()->addWidget(&m_backBtn);

    m_listView = new QListView;
    layout()->addWidget(m_listView);

    auto model = new CommitListModel(this);
    model->refresh(parseCommits(getFileHistory(file)));

    m_listView->setModel(model);
    connect(m_listView, &QListView::clicked, this, &FileHistoryWidget::itemClicked);
}

void FileHistoryWidget::itemClicked(const QModelIndex &idx)
{
    QProcess git;
    QFileInfo fi(m_file);
    git.setWorkingDirectory(fi.absolutePath());
    QStringList args{QStringLiteral("diff"), QString::fromUtf8(idx.data(CommitListModel::Hash).toByteArray()), QStringLiteral("--"), m_file};
    git.start(QStringLiteral("git"), args, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return;
        }
        QByteArray contents = git.readAll();
        // we send this signal to the parent, which will pass it on to
        // the GitWidget from where a temporary file is opened
        Q_EMIT commitClicked(fi.fileName(), QStringLiteral("XXXXXX %1.diff"), contents);
    }
}
