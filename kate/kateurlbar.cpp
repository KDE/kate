/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kateurlbar.h"
#include "kateviewmanager.h"
#include "kateviewspace.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QAbstractListModel>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMimeDatabase>
#include <QPainter>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
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
        m_fileInfos.clear();
        m_currentDir = dir;

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

    QDir dir() const
    {
        return m_currentDir;
    }

private:
    QList<QFileInfo> m_fileInfos;
    QDir m_currentDir;
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

        setFocusProxy(&m_list);
    }

    void setDir(const QDir &d, const QString &currentItemName)
    {
        m_model.setDir(d);
        updateGeometry();
        auto firstIndex = m_model.index(0, 0);
        if (!currentItemName.isEmpty() && firstIndex.isValid()) {
            const auto idxesToSelect = m_model.match(firstIndex, Qt::DisplayRole, currentItemName);
            if (!idxesToSelect.isEmpty() && idxesToSelect.constFirst().isValid()) {
                m_list.setCurrentIndex(idxesToSelect.constFirst());
            }
        } else {
            m_list.setCurrentIndex(firstIndex);
        }
    }

    void updateGeometry()
    {
        auto s = m_list.sizeHintForRow(0);
        auto c = m_model.rowCount();
        const auto h = s * c + (s / 2);
        const auto vScroll = m_list.verticalScrollBar();
        int w = m_list.sizeHintForColumn(0) + (vScroll ? vScroll->height() / 2 : 0);

        setFixedSize(qMin(w, 500), qMin(h, 600));
    }

    void onClicked(const QModelIndex &idx)
    {
        if (!idx.isValid()) {
            return;
        }
        const auto fi = idx.data(DirFilesModel::FileInfo).value<QFileInfo>();
        if (fi.isDir()) {
            setDir(QDir(fi.absoluteFilePath()), QString());
        } else if (fi.isFile()) {
            const QUrl url = QUrl::fromLocalFile(fi.absoluteFilePath());
            hide();
            Q_EMIT openUrl(url);
        }
    }

    void keyPressEvent(QKeyEvent *ke) override
    {
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            onClicked(m_list.currentIndex());
            return;
        } else if (ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right) {
            hide();
            Q_EMIT navigateLeftRight(ke->key());
        } else if (ke->key() == Qt::Key_Escape) {
            hide();
            ke->accept();
            return;
        } else if (ke->key() == Qt::Key_Backspace) {
            auto dir = m_model.dir();
            if (dir.cdUp()) {
                setDir(dir, QString());
            }
        }
        QMenu::keyPressEvent(ke);
    }

Q_SIGNALS:
    void openUrl(const QUrl &url);
    void navigateLeftRight(int key);

private:
    QListView m_list;
    DirFilesModel m_model;
};

enum BreadCrumbRole {
    PathRole = Qt::UserRole + 1,
    IsSeparator = Qt::UserRole + 2,
};

class BreadCrumbDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *o, const QModelIndex &idx) const override
    {
        QStyledItemDelegate::initStyleOption(o, idx);
        o->decorationAlignment = Qt::AlignCenter;
        o->decorationSize = QSize(16, 16);
    }

    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        const auto str = idx.data(Qt::DisplayRole).toString();
        const int margin = opt.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
        if (!str.isEmpty()) {
            const int hMargin = margin + 1;
            auto size = QStyledItemDelegate::sizeHint(opt, idx);
            const int w = opt.fontMetrics.horizontalAdvance(str) + (2 * hMargin);
            size.rwidth() = w;

            if (!idx.data(Qt::DecorationRole).isNull()) {
                size.rwidth() += 16 + (2 * margin);
            }

            return size;
        } else if (!idx.data(Qt::DecorationRole).isNull()) {
            QSize s(16, 16);
            s = s.grownBy({margin, 0, margin, 0});
            return s;
        }
        return QStyledItemDelegate::sizeHint(opt, idx);
    }
};

class BreadCrumbView : public QListView
{
    Q_OBJECT
public:
    BreadCrumbView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        setFlow(QListView::LeftToRight);
        setModel(&m_model);
        setFrameStyle(QFrame::NoFrame);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setItemDelegate(new BreadCrumbDelegate(this));
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setTextElideMode(Qt::ElideNone);

        connect(qApp, &QApplication::paletteChanged, this, &BreadCrumbView::updatePalette, Qt::QueuedConnection);
        auto font = this->font();
        font.setPointSize(font.pointSize() - 1);
        setFont(font);
        updatePalette();

        connect(this, &QListView::clicked, this, &BreadCrumbView::onClicked);
    }

    void updatePalette()
    {
        auto pal = palette();
        pal.setBrush(QPalette::Base, parentWidget()->palette().window());
        setPalette(pal);
    }

    void setUrl(const QUrl &url)
    {
        if (url.isEmpty()) {
            return;
        }

        const QString s = url.toString(QUrl::NormalizePathSegments | QUrl::PreferLocalFile);
        const auto res = splittedUrl(s);

        const auto &dirs = res;

        m_model.clear();
        int i = 0;
        for (const auto &dir : dirs) {
            auto item = new QStandardItem(dir.name);
            item->setData(dir.path, BreadCrumbRole::PathRole);
            m_model.appendRow(item);

            if (i < dirs.size() - 1) {
                auto sep = new QStandardItem(QIcon::fromTheme(QStringLiteral("arrow-right")), {});
                sep->setSelectable(false);
                sep->setData(true, BreadCrumbRole::IsSeparator);
                m_model.appendRow(sep);
            } else {
                // last item, which is the filename, show icon with it
                const auto icon = QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(s, QMimeDatabase::MatchExtension).iconName());
                item->setIcon(icon);
            }
            i++;
        }
    }

    bool IsSeparator(const QModelIndex &idx) const
    {
        return idx.data(BreadCrumbRole::IsSeparator).toBool();
    }

    void keyPressEvent(QKeyEvent *ke) override
    {
        const auto key = ke->key();
        auto current = currentIndex();

        if (key == Qt::Key_Left || key == Qt::Key_Right) {
            onNavigateLeftRight(key, false);
        } else if (key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Down) {
            Q_EMIT clicked(current);
        } else if (key == Qt::Key_Escape) {
            Q_EMIT unsetFocus();
        }
    }

    void onClicked(const QModelIndex &idx)
    {
        auto path = idx.data(BreadCrumbRole::PathRole).toString();
        if (path.isEmpty()) {
            return;
        }

        const auto pos = mapToGlobal(rectForIndex(idx).bottomLeft());

        QDir d(path);
        DirFilesList m(this);
        auto par = static_cast<KateUrlBar *>(parentWidget());
        connect(&m, &DirFilesList::openUrl, par, &KateUrlBar::openUrlRequested);
        connect(&m, &DirFilesList::openUrl, this, &BreadCrumbView::unsetFocus);
        connect(&m, &DirFilesList::navigateLeftRight, this, [this](int k) {
            onNavigateLeftRight(k, true);
        });
        m.setDir(d, idx.data().toString());
        m.setFocus();
        m.exec(pos);
    }

protected:
    void focusInEvent(QFocusEvent *f) override
    {
        if (f->reason() == Qt::OtherFocusReason) {
            const auto last = m_model.index(m_model.rowCount() - 1, 0);
            if (last.isValid()) {
                setCurrentIndex(last);
                QMetaObject::invokeMethod(
                    this,
                    [this, last] {
                        clicked(last);
                    },
                    Qt::QueuedConnection);
            }
        }
        QListView::focusInEvent(f);
    }

private:
    QModelIndex lastIndex()
    {
        return m_model.index(m_model.rowCount() - 1, 0);
    }

    struct DirNamePath {
        QString name;
        QString path;
    };

    void onNavigateLeftRight(int key, bool open)
    {
        const auto current = currentIndex();
        const int step = IsSeparator(current) ? 1 : 2;
        const int nextRow = key == Qt::Key_Left ? current.row() - step : current.row() + step;
        auto nextIndex = current.sibling(nextRow, 0);
        if (nextIndex.isValid()) {
            setCurrentIndex(nextIndex);
            if (open) {
                Q_EMIT clicked(currentIndex());
            }
        }
    }

    QVector<DirNamePath> splittedUrl(const QString &s)
    {
        const int slashIndex = s.lastIndexOf(QLatin1Char('/'));
        if (slashIndex == -1) {
            return {};
        }

        QDir dir(s);
        const QString fileName = dir.dirName();
        dir.cdUp();
        const QString path = dir.absolutePath();

        QVector<DirNamePath> dirsList;
        dirsList << DirNamePath{fileName, path};

        QString dirName = dir.dirName();

        while (dir.cdUp()) {
            if (dirName.isEmpty()) {
                continue;
            }
            DirNamePath dnp{dirName, dir.absolutePath()};
            dirsList.push_back(dnp);

            dirName = dir.dirName();
        }
        std::reverse(dirsList.begin(), dirsList.end());
        return dirsList;
    }

    QStandardItemModel m_model;

Q_SIGNALS:
    void unsetFocus();
};

KateUrlBar::KateUrlBar(KateViewSpace *parent)
    : QWidget(parent)
{
    setFixedHeight(24);
    setContentsMargins({});

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    m_breadCrumbView = new BreadCrumbView(this);
    layout->addWidget(m_breadCrumbView);

    auto *vm = parent->viewManger();
    connect(vm, &KateViewManager::viewChanged, this, &KateUrlBar::onViewChanged);

    connect(vm, &KateViewManager::showUrlNavBarChanged, this, [this, vm](bool show) {
        setHidden(!show);
        if (show) {
            onViewChanged(vm->activeView());
        }
    });

    connect(m_breadCrumbView, &BreadCrumbView::unsetFocus, this, [vm] {
        vm->activeView()->setFocus();
    });

    setFocusProxy(m_breadCrumbView);

    setHidden(!vm->showUrlNavBar());
}

void KateUrlBar::onViewChanged(KTextEditor::View *v)
{
    if (!v) {
        updateForDocument(nullptr);
        hide();
        return;
    }

    updateForDocument(v->document());
}

void KateUrlBar::updateForDocument(KTextEditor::Document *doc)
{
    // always disconnect and perhaps set nullptr doc
    if (m_currentDoc) {
        disconnect(m_currentDoc, &KTextEditor::Document::documentUrlChanged, this, &KateUrlBar::updateForDocument);
    }
    m_currentDoc = doc;
    if (!doc) {
        return;
    }

    // we want to watch for url changed
    connect(m_currentDoc, &KTextEditor::Document::documentUrlChanged, this, &KateUrlBar::updateForDocument);

    auto *vm = static_cast<KateViewSpace *>(parentWidget())->viewManger();
    if (vm && !vm->showUrlNavBar()) {
        return;
    }

    const auto url = doc->url();
    if (url.isEmpty() || !url.isLocalFile()) {
        hide();
        return;
    }

    m_breadCrumbView->setUrl(url);

    if (isHidden())
        show();
}

#include "kateurlbar.moc"
