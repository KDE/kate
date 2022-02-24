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

#include <KAcceleratorManager>
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
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMimeDatabase>
#include <QPainter>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>
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
        m_list.setModel(&m_proxyModel);
        m_list.setResizeMode(QListView::Adjust);
        m_list.setViewMode(QListView::ListMode);
        m_list.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_list.setFrameStyle(QFrame::NoFrame);

        auto *l = new QVBoxLayout(this);
        l->setContentsMargins({});
        l->addWidget(&m_lineEdit);
        l->addWidget(&m_list);

        m_list.installEventFilter(this);
        m_list.viewport()->installEventFilter(this);
        setFocusProxy(&m_list);

        connect(qApp, &QApplication::paletteChanged, this, &DirFilesList::updatePalette, Qt::QueuedConnection);

        m_lineEdit.setReadOnly(true);
        m_lineEdit.hide();

        m_proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel.setSourceModel(&m_model);

        connect(&m_lineEdit, &QLineEdit::textChanged, &m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);
    }

    void updatePalette()
    {
        auto p = m_list.palette();
        p.setBrush(QPalette::Base, palette().alternateBase());
        m_list.setPalette(p);
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
        m_proxyModel.setSourceModel(&m_model);

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
            m_lineEdit.clear();
            m_lineEdit.hide();
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
            onClicked(m_list.currentIndex(), ke->modifiers());
            return;
        } else if (ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right) {
            hide();
            Q_EMIT navigateLeftRight(ke->key());
        } else if (ke->key() == Qt::Key_Escape) {
            hide();
            ke->accept();
            return;
        } else if (ke->key() == Qt::Key_Backspace) {
            if (m_lineEdit.text().isEmpty()) {
                auto dir = m_model.dir();
                if (dir.cdUp()) {
                    setDir(dir, QString());
                }
            } else {
                m_lineEdit.setText(m_lineEdit.text().chopped(1));
                if (m_lineEdit.text().isEmpty()) {
                    m_lineEdit.hide();
                }
            }
        }
        QMenu::keyPressEvent(ke);
    }

    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (e->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);
            if (me->button() == Qt::LeftButton) {
                const QModelIndex idx = m_list.indexAt(m_list.viewport()->mapFromGlobal(me->globalPos()));
                if (!idx.isValid()) {
                    return QMenu::eventFilter(o, me);
                }
                onClicked(idx, me->modifiers());
            }
        }

        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            if ((keyEvent->modifiers() == Qt::NoModifier || keyEvent->modifiers() == Qt::SHIFT) && !keyEvent->text().isEmpty()) {
                QChar c = keyEvent->text().front();
                if (c.isPrint()) {
                    m_lineEdit.setText(m_lineEdit.text() + keyEvent->text());
                    if (!m_lineEdit.isVisible()) {
                        m_lineEdit.show();
                    }
                }
            }
        }

        return QMenu::eventFilter(o, e);
    }

Q_SIGNALS:
    void openUrl(const QUrl &url, Qt::KeyboardModifiers);
    void navigateLeftRight(int key);

private:
    QListView m_list;
    QLineEdit m_lineEdit;
    DirFilesModel m_model;
    QSortFilterProxyModel m_proxyModel;
};

class SymbolsTreeView : public QMenu
{
    Q_OBJECT
public:
    // Copied from LSPClientSymbolView
    enum Role {
        SymbolRange = Qt::UserRole,
        ScoreRole, //> Unused here
        IsPlaceholder
    };
    SymbolsTreeView(QWidget *parent)
        : QMenu(parent)
        , m_tree(new QTreeView(this))
    {
        m_tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_tree->setFrameStyle(QFrame::NoFrame);
        m_tree->setUniformRowHeights(true);
        m_tree->setHeaderHidden(true);
        m_tree->setTextElideMode(Qt::ElideRight);
        m_tree->setRootIsDecorated(false);

        auto *l = new QVBoxLayout(this);
        l->setContentsMargins({});
        l->addWidget(m_tree);
        setFocusProxy(m_tree);
        m_tree->installEventFilter(this);
        m_tree->viewport()->installEventFilter(this);

        connect(m_tree, &QTreeView::clicked, this, &SymbolsTreeView::onClicked);
        connect(qApp, &QApplication::paletteChanged, this, &SymbolsTreeView::updatePalette, Qt::QueuedConnection);
    }

    void updatePalette()
    {
        auto p = m_tree->palette();
        p.setBrush(QPalette::Base, palette().alternateBase());
        m_tree->setPalette(p);
    }

    void setSymbolsModel(QAbstractItemModel *model, KTextEditor::View *v, const QString &text)
    {
        m_activeView = v;
        m_tree->setModel(model);
        m_tree->expandAll();
        const auto idxToSelect = model->match(model->index(0, 0), 0, text, 1, Qt::MatchExactly);
        if (!idxToSelect.isEmpty()) {
            m_tree->setCurrentIndex(idxToSelect.constFirst());
        }
        updateGeometry();
    }

    bool eventFilter(QObject *o, QEvent *e) override
    {
        // Handling via event filter is necessary to bypass
        // QTreeView's own key event handling
        if (e->type() == QEvent::KeyPress) {
            if (handleKeyPressEvent(static_cast<QKeyEvent *>(e))) {
                return true;
            }
        }

        return QMenu::eventFilter(o, e);
    }

    bool handleKeyPressEvent(QKeyEvent *ke)
    {
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            onClicked(m_tree->currentIndex());
            return true;
        } else if (ke->key() == Qt::Key_Left) {
            hide();
            Q_EMIT navigateLeftRight(ke->key());
            return false;
        } else if (ke->key() == Qt::Key_Escape) {
            hide();
            ke->accept();
            return true;
        }
        return false;
    }

    void updateGeometry()
    {
        const auto *model = m_tree->model();
        const int rows = rowCount(model, {});
        const int rowHeight = m_tree->sizeHintForRow(0);
        const int maxHeight = rows * rowHeight;

        setFixedSize(350, qMin(600, maxHeight));
    }

    void onClicked(const QModelIndex &idx)
    {
        const auto range = idx.data(SymbolRange).value<KTextEditor::Range>();
        if (range.isValid()) {
            m_activeView->setCursorPosition(range.start());
        }
        hide();
    }

    static QModelIndex symbolForCurrentLine(QAbstractItemModel *model, const QModelIndex &index, int line)
    {
        const int rowCount = model->rowCount(index);
        for (int i = 0; i < rowCount; ++i) {
            const auto idx = model->index(i, 0, index);
            if (idx.data(SymbolRange).value<KTextEditor::Range>().overlapsLine(line)) {
                return idx;
            } else if (model->hasChildren(idx)) {
                const auto childIdx = symbolForCurrentLine(model, idx, line);
                if (childIdx.isValid()) {
                    return childIdx;
                }
            }
        }
        return {};
    }

private:
    // row count that counts top level + 1 level down rows
    // needed to ensure we don't get strange heights for
    // cases where there are only a couple of top level symbols
    int rowCount(const QAbstractItemModel *model, const QModelIndex &index)
    {
        int rows = model->rowCount(index);
        int child_rows = 0;
        for (int i = 0; i < rows; ++i) {
            child_rows += rowCount(model, model->index(i, 0, index));
        }
        return rows + child_rows;
    }

    QTreeView *m_tree;
    QPointer<KTextEditor::View> m_activeView;

Q_SIGNALS:
    void navigateLeftRight(int key);
};

enum BreadCrumbRole {
    PathRole = Qt::UserRole + 1,
    IsSeparator,
    IsSymbolCrumb,
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
        // We always want this icon size and nothing bigger
        if (idx.data(BreadCrumbRole::IsSeparator).toBool()) {
            o->decorationSize = QSize(8, 8);
        } else {
            o->decorationSize = QSize(16, 16);
        }

        if (o->state & QStyle::State_MouseOver) {
            // No hover feedback for separators
            if (idx.data(BreadCrumbRole::IsSeparator).toBool()) {
                o->state.setFlag(QStyle::State_MouseOver, false);
                o->state.setFlag(QStyle::State_Active, false);
            } else {
                o->palette.setBrush(QPalette::Text, o->palette.brightText());
            }
        }
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
            QSize s(8, 8);
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
        auto textColor = pal.text().color();
        textColor = textColor.lightness() > 127 ? textColor.darker(150) : textColor.lighter(150);
        pal.setBrush(QPalette::Inactive, QPalette::Text, textColor);
        pal.setBrush(QPalette::Active, QPalette::Text, textColor);
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
        m_symbolsModel = nullptr;

        int i = 0;
        QIcon seperator = m_urlBar->seperator();
        for (const auto &dir : dirs) {
            auto item = new QStandardItem(dir.name);
            item->setData(dir.path, BreadCrumbRole::PathRole);
            m_model.appendRow(item);

            if (i < dirs.size() - 1) {
                auto sep = new QStandardItem(seperator, {});
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

        auto *mainWindow = m_urlBar->viewManager()->mainWindow();
        QPointer<QObject> lsp = mainWindow->pluginView(QStringLiteral("lspclientplugin"));
        if (lsp) {
            addSymbolCrumb(lsp);
        }
    }

    void addSymbolCrumb(QObject *lsp)
    {
        QAbstractItemModel *model = nullptr;
        QMetaObject::invokeMethod(lsp, "documentSymbolsModel", Q_RETURN_ARG(QAbstractItemModel *, model));
        m_symbolsModel = model;
        if (!model) {
            return;
        }

        connect(m_symbolsModel, &QAbstractItemModel::modelReset, this, &BreadCrumbView::updateSymbolsCrumb);

        if (model->rowCount({}) == 0) {
            return;
        }

        const auto view = m_urlBar->viewManager()->activeView();
        disconnect(m_connToView);
        if (view) {
            m_connToView = connect(view, &KTextEditor::View::cursorPositionChanged, this, &BreadCrumbView::updateSymbolsCrumb);
        }

        const auto idx = getSymbolCrumbText();
        if (!idx.isValid()) {
            return;
        }

        // Add separator
        auto sep = new QStandardItem(QIcon(m_urlBar->seperator()), {});
        sep->setSelectable(false);
        sep->setData(true, BreadCrumbRole::IsSeparator);
        m_model.appendRow(sep);

        const auto icon = idx.data(Qt::DecorationRole).value<QIcon>();
        const auto text = idx.data().toString();
        auto *item = new QStandardItem(icon, text);
        item->setData(true, BreadCrumbRole::IsSymbolCrumb);
        m_model.appendRow(item);
    }

    void updateSymbolsCrumb()
    {
        QStandardItem *item = m_model.item(m_model.rowCount() - 1, 0);

        if (!m_urlBar->viewSpace()->isActiveSpace()) {
            if (item && item->data(BreadCrumbRole::IsSymbolCrumb).toBool()) {
                // we are not active viewspace, remove the symbol + separator from breadcrumb
                // This is important as LSP only gives us symbols for the current active view
                // which atm is in some other viewspace.
                // In future we might want to extend LSP to provide us models for documents
                // but for now this will do.
                qDeleteAll(m_model.takeRow(m_model.rowCount() - 1));
                qDeleteAll(m_model.takeRow(m_model.rowCount() - 1));
            }
            return;
        }

        const auto idx = getSymbolCrumbText();
        if (!idx.isValid()) {
            return;
        }

        if (!item || !item->data(BreadCrumbRole::IsSymbolCrumb).toBool()) {
            // Add separator
            auto sep = new QStandardItem(QIcon(m_urlBar->seperator()), {});
            sep->setSelectable(false);
            sep->setData(true, BreadCrumbRole::IsSeparator);
            m_model.appendRow(sep);

            item = new QStandardItem;
            m_model.appendRow(item);
            item->setData(true, BreadCrumbRole::IsSymbolCrumb);
        }

        const auto text = idx.data().toString();
        const auto icon = idx.data(Qt::DecorationRole).value<QIcon>();
        item->setText(text);
        item->setIcon(icon);
    }

    QModelIndex getSymbolCrumbText()
    {
        if (!m_symbolsModel) {
            return {};
        }

        QModelIndex first = m_symbolsModel->index(0, 0);
        if (first.data(SymbolsTreeView::IsPlaceholder).toBool()) {
            return {};
        }

        const auto view = m_urlBar->viewSpace()->currentView();
        int line = view ? view->cursorPosition().line() : 0;

        QModelIndex idx;
        if (line > 0) {
            idx = SymbolsTreeView::symbolForCurrentLine(m_symbolsModel, idx, line);
        } else {
            idx = first;
        }

        if (!idx.isValid()) {
            idx = first;
        }
        return idx;
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
        // Clicked on the symbol?
        if (m_symbolsModel && idx.data(BreadCrumbRole::IsSymbolCrumb).toBool()) {
            auto activeView = m_urlBar->viewSpace()->currentView();
            if (!activeView) {
                // View must be there
                return;
            }
            SymbolsTreeView t(this);
            connect(&t, &SymbolsTreeView::navigateLeftRight, this, [this](int k) {
                onNavigateLeftRight(k, true);
            });
            const QString symbolName = idx.data().toString();
            t.setSymbolsModel(m_symbolsModel, activeView, symbolName);
            const auto pos = mapToGlobal(rectForIndex(idx).bottomLeft());
            t.setFocus();
            t.exec(pos);
        }

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

    void updateSeperatorIcon()
    {
        auto newSeperator = m_urlBar->seperator();
        for (int i = 0; i < m_model.rowCount(); ++i) {
            auto item = m_model.item(i);
            if (item && item->data(BreadCrumbRole::IsSeparator).toBool()) {
                item->setIcon(newSeperator);
            }
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
    QPointer<QAbstractItemModel> m_symbolsModel;
    QMetaObject::Connection m_connToView; // Only one conn at a time

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
        , m_sepLabel(new QLabel(this))
    {
        // UrlBar
        auto urlBarLayout = new QHBoxLayout(this);
        urlBarLayout->setSpacing(0);
        urlBarLayout->setContentsMargins({});
        urlBarLayout->addWidget(m_currBranchBtn);
        urlBarLayout->addWidget(m_sepLabel);
        urlBarLayout->addWidget(m_breadCrumbView);

        m_sepLabel->setAlignment(Qt::AlignCenter);

        setFocusProxy(m_breadCrumbView);

        setupCurrentBranchButton();

        connect(m_breadCrumbView, &BreadCrumbView::unsetFocus, this, [this] {
            m_urlBar->viewManager()->activeView()->setFocus();
        });

        connect(
            qApp,
            &QApplication::paletteChanged,
            this,
            [this] {
                m_sepPixmap = QPixmap();
                m_sepLabel->setPixmap(separatorPixmap());
                m_breadCrumbView->updateSeperatorIcon();
            },
            Qt::QueuedConnection);
    }

    void setupCurrentBranchButton()
    {
        m_currBranchBtn->setAutoRaise(true);
        m_currBranchBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        KAcceleratorManager::setNoAccel(m_currBranchBtn);

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
            if (!m_currBranchBtn->defaultAction()) {
                m_currBranchBtn->hide();
                m_sepLabel->hide();
            } else {
                // Setup the icon now
                m_sepLabel->setPixmap(separatorPixmap());
            }
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

    QPixmap separatorPixmap()
    {
        if (m_sepPixmap.isNull()) {
            initSeparatorIcon();
        }
        return m_sepPixmap;
    }

private:
    void initSeparatorIcon()
    {
        Q_ASSERT(m_sepPixmap.isNull());
        const auto dpr = this->devicePixelRatioF();
        m_sepPixmap = QPixmap(8 * dpr, 8 * dpr);
        m_sepPixmap.fill(Qt::transparent);

        auto pal = palette();
        auto textColor = pal.text().color();
        textColor = textColor.lightness() > 127 ? textColor.darker(150) : textColor.lighter(150);
        pal.setColor(QPalette::ButtonText, textColor);
        pal.setColor(QPalette::WindowText, textColor);
        pal.setColor(QPalette::Text, textColor);

        QPainter p(&m_sepPixmap);
        QStyleOption o;
        o.rect.setRect(0, 0, 8, 8);
        o.palette = pal;
        style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &o, &p, this);
        m_sepPixmap.setDevicePixelRatio(dpr);
    }

    KateUrlBar *const m_urlBar;
    BreadCrumbView *const m_breadCrumbView;
    QToolButton *const m_currBranchBtn;
    QLabel *const m_sepLabel;
    QPixmap m_sepPixmap;
};

KateUrlBar::KateUrlBar(KateViewSpace *parent)
    : QWidget(parent)
    , m_stack(new QStackedWidget(this))
    , m_urlBarView(new UrlbarContainer(this))
    , m_untitledDocLabel(new QLabel(this))
    , m_parentViewSpace(parent)
{
    setContentsMargins({});
    setFixedHeight(22);

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

KateViewSpace *KateUrlBar::viewSpace()
{
    return m_parentViewSpace;
}

QIcon KateUrlBar::seperator()
{
    return m_urlBarView->separatorPixmap();
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
    // We are not active but we have a doc? => don't do anything
    // we check for a doc because we want to update the KateUrlBar
    // when kate starts
    if (!viewSpace()->isActiveSpace() && m_currentDoc) {
        return;
    }

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
