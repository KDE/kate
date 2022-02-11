/*
    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kateurlbar.h"
#include "kateviewmanager.h"
#include "kateviewspace.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QAbstractListModel>
#include <QAction>
#include <QDir>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMimeDatabase>
#include <QScrollBar>
#include <QToolButton>
#include <QUrl>

class DirFilesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    DirFilesModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    enum Role { FileInfo = Qt::UserRole + 1 };

    int rowCount(const QModelIndex & = {}) const override
    {
        return m_fileInfos.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid()) {
            return {};
        }

        const auto &fi = m_fileInfos.at(index.row());
        if (role == Qt::DisplayRole) {
            return fi.fileName();
        } else if (role == Qt::DecorationRole) {
            if (fi.isDir()) {
                return QIcon(QIcon::fromTheme(QStringLiteral("folder")));
            } else if (fi.isFile()) {
                return QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(fi).iconName());
            }
        } else if (role == FileInfo) {
            return QVariant::fromValue(fi);
        }

        return {};
    }

    void setDir(const QDir &dir)
    {
        beginResetModel();
        const auto fileInfos = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs | QDir::Hidden);
        for (const auto &fi : fileInfos) {
            if (fi.isDir()) {
                m_fileInfos << fi;
            } else if (QMimeDatabase().mimeTypeForFile(fi).inherits(QStringLiteral("text/plain"))) {
                m_fileInfos << fi;
            }
        }
        endResetModel();
    }

private:
    QList<QFileInfo> m_fileInfos;
};

class DirFilesList : public QMenu
{
    Q_OBJECT
public:
    DirFilesList(QWidget *parent)
        : QMenu(parent)
    {
        m_list.setModel(&m_model);
        m_list.setResizeMode(QListView::Adjust);
        m_list.setViewMode(QListView::ListMode);
        m_list.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_list.setFrameStyle(QFrame::NoFrame);

        auto *l = new QVBoxLayout(this);
        l->setContentsMargins({});
        l->addWidget(&m_list);

        connect(&m_list, &QListView::clicked, this, &DirFilesList::onClicked);
    }

    void setDir(const QDir &d)
    {
        m_model.setDir(d);
        updateGeometry();
    }

    void updateGeometry()
    {
        auto s = m_list.sizeHintForRow(0);
        auto c = m_model.rowCount();
        const auto h = s * c + (s / 2);
        const auto vScroll = m_list.verticalScrollBar();
        int w = m_list.sizeHintForColumn(0) + (vScroll ? vScroll->height() / 2 : 0);

        setFixedSize(qMin(w, 300), qMin(h, 600));
    }

    void onClicked(const QModelIndex &idx)
    {
        if (!idx.isValid()) {
            return;
        }
        const auto fi = idx.data(DirFilesModel::FileInfo).value<QFileInfo>();
        if (fi.isDir()) {
            setDir(QDir(fi.absoluteFilePath()));
        } else if (fi.isFile()) {
            const QUrl url = QUrl::fromLocalFile(fi.absoluteFilePath());
            hide();
            Q_EMIT openUrl(url);
        }
    }

Q_SIGNALS:
    void openUrl(const QUrl &url);

private:
    QListView m_list;
    DirFilesModel m_model;
};

KateUrlBar::KateUrlBar(KateViewSpace *parent)
    : QWidget(parent)
{
    setFixedHeight(25);
    setContentsMargins({});

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins({});
    m_layout->setSpacing(0);

    auto *vm = parent->viewManger();
    connect(vm, &KateViewManager::viewChanged, this, &KateUrlBar::onViewChanged);

    connect(vm, &KateViewManager::showUrlNavBarChanged, this, [this, vm](bool show) {
        setHidden(!show);
        if (show) {
            onViewChanged(vm->activeView());
        }
    });

    setHidden(!vm->showUrlNavBar());
}

void KateUrlBar::onViewChanged(KTextEditor::View *v)
{
    if (!v) {
        hide();
        return;
    }

    auto *vm = static_cast<KateViewSpace *>(parentWidget())->viewManger();
    if (vm && !vm->showUrlNavBar()) {
        return;
    }
    m_paths.clear();

    QLayoutItem *l;
    while ((l = m_layout->takeAt(0)) != nullptr) {
        delete l->widget();
        delete l;
    }

    const auto url = v->document()->url();
    if (url.isEmpty()) {
        hide();
        return;
    }

    auto res = splittedUrl(url);
    const auto &file = res.first;
    const auto &dirs = res.second;
    if (dirs.isEmpty() || file.isEmpty()) {
        hide();
        return;
    }

    for (const auto &dir : dirs) {
        m_layout->addWidget(dirButton(dir.name, dir.path));
        m_layout->addWidget(separatorLabel());
    }
    m_layout->addWidget(fileLabel(file));
    m_layout->addStretch();
    if (isHidden())
        show();
}

std::pair<QString, QVector<KateUrlBar::DirNamePath>> KateUrlBar::splittedUrl(const QUrl &u)
{
    QString s = u.toString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile);

    const int slashIndex = s.lastIndexOf(QLatin1Char('/'));
    if (slashIndex == -1) {
        return {};
    }

    const QString fileName = s.mid(slashIndex + 1);

    QDir dir(s);
    QVector<DirNamePath> dirsList;
    while (dir.cdUp()) {
        if (dir.dirName().isEmpty()) {
            continue;
        }
        DirNamePath dnp{dir.dirName(), dir.absolutePath()};
        dirsList.push_back(dnp);
    }
    std::reverse(dirsList.begin(), dirsList.end());
    return {fileName, dirsList};
}

QLabel *KateUrlBar::separatorLabel()
{
    QLabel *sep = new QLabel(this);
    sep->setPixmap(QIcon::fromTheme(QStringLiteral("arrow-right")).pixmap(QSize(16, 16)));
    return sep;
}

QToolButton *KateUrlBar::dirButton(const QString &dirName, const QString &path)
{
    QToolButton *tb = new QToolButton(this);
    tb->setText(dirName);
    tb->setArrowType(Qt::RightArrow);
    tb->setToolButtonStyle(Qt::ToolButtonTextOnly);
    tb->setAutoRaise(true);
    connect(tb, &QToolButton::clicked, this, &KateUrlBar::pathClicked);

    m_paths.push_back({tb, path});

    return tb;
}

QLabel *KateUrlBar::fileLabel(const QString &file)
{
    auto l = new QLabel(file);
    auto font = l->font();
    font.setPointSize(font.pointSize() - 1);
    l->setFont(font);
    l->setAlignment(Qt::AlignCenter);
    l->setContentsMargins({6, 0, 0, 0});
    return l;
}

void KateUrlBar::pathClicked()
{
    auto button = sender();
    if (!button) {
        return;
    }

    // Find the path that this toolbutton refers to
    auto it = std::find_if(m_paths.begin(), m_paths.end(), [button](const std::pair<QToolButton *, QString> &tbPath) {
        return tbPath.first == button;
    });
    // not found?
    if (it == m_paths.end()) {
        return;
    }

    const QToolButton *tb = it->first;
    const QString path = it->second;
    QDir d(path);
    DirFilesList m(this);
    connect(&m, &DirFilesList::openUrl, this, &KateUrlBar::openUrlRequested);
    m.setDir(d);
    auto pos = tb->mapToGlobal(tb->pos()) - tb->pos();
    pos.setY(pos.y() + tb->height());
    m.exec(pos);
}

#include "kateurlbar.moc"
