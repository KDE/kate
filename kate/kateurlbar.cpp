/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateurlbar.h"
#include "kateapp.h"
#include "kateviewmanager.h"
#include "kateviewspace.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KColorScheme>
#include <KLocalizedString>

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
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTimer>
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
        } else if (role == Qt::ForegroundRole) {
            // highlight already open documents
            if (KateApp::self()->documentManager()->findDocument(QUrl::fromLocalFile(fi.absoluteFilePath()))) {
                return KColorScheme().foreground(KColorScheme::PositiveText).color();
            }
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

        m_list.viewport()->installEventFilter(this);
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

    void onClicked(const QModelIndex &idx, Qt::KeyboardModifiers mod)
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
            Q_EMIT openUrl(url, /*newtab=*/mod);
        }
    }

    void keyPressEvent(QKeyEvent *ke) override
    {
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            onClicked(m_list.currentIndex(), Qt::NoModifier);
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

    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (e->type() != QEvent::Type::MouseButtonPress) {
            return QMenu::eventFilter(o, e);
        }
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        if (me->button() == Qt::LeftButton) {
            const QModelIndex idx = m_list.indexAt(m_list.viewport()->mapFromGlobal(me->globalPos()));
            if (!idx.isValid()) {
                return QMenu::eventFilter(o, me);
            }
            onClicked(idx, me->modifiers());
        }
        return QMenu::eventFilter(o, e);
    }

Q_SIGNALS:
    void openUrl(const QUrl &url, Qt::KeyboardModifiers);
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
    BreadCrumbView(QWidget *parent, KateUrlBar *urlBar)
        : QListView(parent)
        , m_urlBar(urlBar)
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

    static bool IsSeparator(const QModelIndex &idx)
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
        connect(&m, &DirFilesList::openUrl, m_urlBar, &KateUrlBar::openUrlRequested);
        connect(&m, &DirFilesList::openUrl, this, &BreadCrumbView::unsetFocus);
        connect(&m, &DirFilesList::navigateLeftRight, this, [this](int k) {
            onNavigateLeftRight(k, true);
        });
        m.setDir(d, idx.data().toString());
        m.setFocus();
        m.exec(pos);
    }

    void openLastIndex()
    {
        const auto last = m_model.index(m_model.rowCount() - 1, 0);
        if (last.isValid()) {
            setCurrentIndex(last);
            clicked(last);
        }
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

    static QVector<DirNamePath> splittedUrl(const QString &s)
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

    KateUrlBar *const m_urlBar;
    QStandardItemModel m_model;

Q_SIGNALS:
    void unsetFocus();
};

class UrlbarContainer : public QWidget
{
    Q_OBJECT
public:
    UrlbarContainer(KateUrlBar *parent)
        : QWidget(parent)
        , m_urlBar(parent)
        , m_breadCrumbView(new BreadCrumbView(this, parent))
        , m_currBranchBtn(new QToolButton(this))
        , m_infoLabel(new QLabel(this))
    {
        // UrlBar
        auto urlBarLayout = new QHBoxLayout(this);
        urlBarLayout->setSpacing(0);
        urlBarLayout->setContentsMargins({});
        urlBarLayout->addWidget(m_currBranchBtn);
        urlBarLayout->addSpacing(2);
        urlBarLayout->addWidget(m_breadCrumbView);
        urlBarLayout->addWidget(m_infoLabel);

        setFocusProxy(m_breadCrumbView);

        setupCurrentBranchButton();

        connect(m_breadCrumbView, &BreadCrumbView::unsetFocus, this, [this] {
            m_urlBar->viewManager()->activeView()->setFocus();
        });
    }

    void paintEvent(QPaintEvent *e) override
    {
        QWidget::paintEvent(e);

        const int topX = x() + m_currBranchBtn->width();
        const int topY = y();

        const int bottomX = topX;
        const int bottomY = topY + height();

        QPainter p(this);
        p.setPen(palette().color(QPalette::Disabled, QPalette::Text));
        p.drawLine(topX, topY, bottomX, bottomY);
    }

    void setupCurrentBranchButton()
    {
        m_currBranchBtn->setAutoRaise(true);
        m_currBranchBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        QTimer::singleShot(500, this, [this] {
            auto *mw = m_urlBar->viewManager()->mainWindow();
            const auto acs = mw->actionCollection()->allCollections();
            for (auto *ac : acs) {
                if (auto action = ac->action(QStringLiteral("current_branch"))) {
                    m_currBranchBtn->setDefaultAction(action);
                    connect(action, &QAction::changed, this, [this] {
                        if (m_currBranchBtn->defaultAction() && m_currBranchBtn->defaultAction()->text().isEmpty()) {
                            m_currBranchBtn->hide();
                        }
                    });
                }
            }
            if (!m_currBranchBtn->defaultAction())
                m_currBranchBtn->hide();
        });
    }

    void open()
    {
        if (m_breadCrumbView) {
            m_breadCrumbView->openLastIndex();
        }
    }

    void setUrl(const QUrl &url)
    {
        m_breadCrumbView->setUrl(url);
    }

private:
    KateUrlBar *m_urlBar;
    BreadCrumbView *const m_breadCrumbView;
    QToolButton *const m_currBranchBtn;
    QLabel *const m_infoLabel;
};

KateUrlBar::KateUrlBar(KateViewSpace *parent)
    : QWidget(parent)
    , m_stack(new QStackedWidget(this))
    , m_urlBarView(new UrlbarContainer(this))
    , m_untitledDocLabel(new QLabel(this))
    , m_parentViewSpace(parent)
{
    setContentsMargins({});

    setupLayout();

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

void KateUrlBar::open()
{
    if (m_stack->currentWidget() == m_urlBarView) {
        m_urlBarView->open();
    }
}

KateViewManager *KateUrlBar::viewManager()
{
    return m_parentViewSpace->viewManger();
}

void KateUrlBar::setupLayout()
{
    // Setup the stacked widget
    m_stack->addWidget(m_untitledDocLabel);
    m_stack->addWidget(m_urlBarView);

    // MainLayout
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_stack);
}

void KateUrlBar::onViewChanged(KTextEditor::View *v)
{
    if (!v) {
        updateForDocument(nullptr);
        m_untitledDocLabel->setText(i18n("Untitled"));
        m_stack->setCurrentWidget(m_untitledDocLabel);
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

    if (m_currentDoc->url().isEmpty() || !m_currentDoc->url().isLocalFile()) {
        m_untitledDocLabel->setText(m_currentDoc->documentName());
        m_stack->setCurrentWidget(m_untitledDocLabel);
        return;
    }

    if (m_stack->currentWidget() != m_urlBarView) {
        m_stack->setCurrentWidget(m_urlBarView);
    }

    auto *vm = viewManager();
    if (vm && !vm->showUrlNavBar()) {
        return;
    }
    m_urlBarView->setUrl(doc->url());
}

#include "kateurlbar.moc"
