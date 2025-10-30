/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#include "diagnosticview.h"

#include "diagnosticitem.h"
#include "drawing_utils.h"
#include "kateapp.h"
#include "session_diagnostic_suppression.h"
#include "texthint/KateTextHintManager.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KColorScheme>
#include <KMessageWidget>
#include <KXMLGUIFactory>
#include <QClipboard>
#include <QComboBox>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <memory_resource>

class ProviderListModel final : public QAbstractListModel
{
public:
    explicit ProviderListModel(DiagnosticsView *parent)
        : QAbstractListModel(parent)
        , m_diagView(parent)
    {
    }

    int rowCount(const QModelIndex &) const override
    {
        return (int)m_providers.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (size_t(index.row()) >= m_providers.size()) {
            return {};
        }
        if (role == Qt::DisplayRole) {
            if (index.row() == 0) {
                return i18n("All");
            }
            return m_providers.at(index.row())->name;
        }
        if (role == Qt::UserRole) {
            return QVariant::fromValue(m_providers.at(index.row()));
        }
        return {};
    }

    void update(const std::vector<DiagnosticsProvider *> &providerList)
    {
        beginResetModel();
        m_providers.clear();
        m_providers.push_back(nullptr);
        m_providers.insert(m_providers.end(), providerList.begin(), providerList.end());
        endResetModel();
    }

    DiagnosticsView *m_diagView;
    std::vector<DiagnosticsProvider *> m_providers;
};

class DiagnosticsProxyModel final : public QSortFilterProxyModel
{
public:
    explicit DiagnosticsProxyModel(QObject *parent)
        : QSortFilterProxyModel(parent)
    {
    }

    void setActiveProvider(DiagnosticsProvider *provider)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
#endif
        activeProvider = provider;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
        invalidateFilter();
#endif
    }

    void setActiveSeverity(DiagnosticSeverity s)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        beginFilterChange();
#endif
        severity = s;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
        invalidateFilter();
#endif
    }

    DiagnosticSeverity activeSeverity() const
    {
        return this->severity;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        bool ret = true;
        auto model = static_cast<QStandardItemModel *>(sourceModel());
        if (activeProvider) {
            auto index = sourceModel()->index(sourceRow, 0, sourceParent);
            if (index.isValid()) {
                const auto item = model->itemFromIndex(index);
                if (item->type() == DiagnosticItem_File) {
                    ret = static_cast<DocumentDiagnosticItem *>(item)->providers().contains(activeProvider);
                } else {
                    // check parent item if the item is not diag
                    if (item->type() != DiagnosticItem_Diag && item->parent()) {
                        Q_ASSERT(item->parent()->type() == DiagnosticItem_Diag);
                        index = item->parent()->index();
                    }
                    ret = index.data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>() == activeProvider;
                }
            }
        }

        if (ret && severity != DiagnosticSeverity::Unknown) {
            auto index = sourceModel()->index(sourceRow, 0, sourceParent);
            const auto item = model->itemFromIndex(index);
            if (item && item->type() == DiagnosticItem_Diag) {
                auto castedItem = static_cast<DiagnosticItem *>(item);
                ret = castedItem->m_diagnostic.severity == severity;
            } else if (item && item->type() == DiagnosticItem_File) {
                // Hide parent if all childs hidden
                int rc = item->rowCount();
                int count = 0;
                for (int i = 0; i < rc; ++i) {
                    auto child = item->child(i);
                    if (child && child->type() == DiagnosticItem_Diag) {
                        count += static_cast<DiagnosticItem *>(child)->m_diagnostic.severity == severity;
                    }
                }
                ret = count > 0;
            } else if (item) {
                ret = false;
                if (item->parent() && item->parent()->type() == DiagnosticItem_Diag) {
                    auto castedItem = static_cast<DiagnosticItem *>(item);
                    ret = castedItem->m_diagnostic.severity == severity;
                }
            } else {
                ret = false;
            }
        }

        if (ret) {
            ret = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
        }
        return ret;
    }

private:
    DiagnosticsProvider *activeProvider = nullptr;
    DiagnosticSeverity severity = DiagnosticSeverity::Unknown;
};

DiagnosticsProvider::DiagnosticsProvider(KTextEditor::MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(mainWindow);
    DiagnosticsView::instance(mainWindow)->registerDiagnosticsProvider(this);
}

void DiagnosticsProvider::showDiagnosticsView(DiagnosticsProvider *filterTo)
{
    if (diagnosticView && !diagnosticView->isVisible()) {
        diagnosticView->showToolview(filterTo);
    }
}

void DiagnosticsProvider::filterDiagnosticsViewTo(DiagnosticsProvider *filterTo)
{
    if (diagnosticView) {
        diagnosticView->filterViewTo(filterTo);
    }
}

static constexpr KTextEditor::Document::MarkTypes markTypeDiagError = KTextEditor::Document::Error;
static constexpr KTextEditor::Document::MarkTypes markTypeDiagWarning = KTextEditor::Document::Warning;
static constexpr KTextEditor::Document::MarkTypes markTypeDiagOther = KTextEditor::Document::markType30;
static constexpr KTextEditor::Document::MarkTypes markTypeDiagAll =
    KTextEditor::Document::MarkTypes(markTypeDiagError | markTypeDiagWarning | markTypeDiagOther);

struct DiagModelIndex {
    int row;
    int parentRow;
    bool autoApply;
};
Q_DECLARE_METATYPE(DiagModelIndex)

class DiagnosticsLocationTreeDelegate : public QStyledItemDelegate
{
public:
    DiagnosticsLocationTreeDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
        , m_monoFont(Utils::editorFont())
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const bool docItem = !index.parent().isValid();
        if (!docItem) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        auto options = option;
        initStyleOption(&options, index);

        painter->save();

        QString text = index.data().toString();
        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        QList<QTextLayout::FormatRange> formats;
        int lastSlash = text.lastIndexOf(QLatin1Char('/'));
        if (lastSlash != -1) {
            QTextCharFormat fmt;
            fmt.setFontWeight(QFont::Bold);
            formats.append({.start = lastSlash + 1, .length = int(text.length() - (lastSlash + 1)), .format = fmt});
        }

        /** There might be an icon? Make sure to not draw over it **/
        auto textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options, options.widget);
        auto width = textRect.x() - options.rect.x();
        painter->translate(width, 0);
        Utils::paintItemViewText(painter, text, options, formats);

        painter->restore();
    }

private:
    QFont m_monoFont;
};

static QIcon diagnosticsIcon(DiagnosticSeverity severity)
{
    switch (severity) {
    case DiagnosticSeverity::Error: {
        static QIcon icon(QIcon::fromTheme(QStringLiteral("data-error"), QIcon::fromTheme(QStringLiteral("dialog-error"))));
        return icon;
    }
    case DiagnosticSeverity::Warning: {
        static QIcon icon(QIcon::fromTheme(QStringLiteral("data-warning"), QIcon::fromTheme(QStringLiteral("dialog-warning"))));
        return icon;
    }
    case DiagnosticSeverity::Information:
    case DiagnosticSeverity::Hint: {
        static QIcon icon(QIcon::fromTheme(QStringLiteral("data-information"), QIcon::fromTheme(QStringLiteral("dialog-information"))));
        return icon;
    }
    default:
        break;
    }
    return QIcon();
}

DiagnosticsView::DiagnosticsView(QWidget *parent, KTextEditor::MainWindow *mainWindow)
    : QWidget(parent)
    , KXMLGUIClient()
    , m_mainWindow(mainWindow)
    , m_diagnosticsTree(new QTreeView(this))
    , m_clearButton(new QToolButton(this))
    , m_filterLineEdit(new QLineEdit(this))
    , m_providerCombo(new QComboBox(this))
    , m_errFilterBtn(new QToolButton(this))
    , m_warnFilterBtn(new QToolButton(this))
    , m_diagLimitReachedWarning(new KMessageWidget(this))
    , m_proxy(new DiagnosticsProxyModel(this))
    , m_sessionDiagnosticSuppressions(std::make_unique<SessionDiagnosticSuppressions>())
    , m_posChangedTimer(new QTimer(this))
    , m_filterChangedTimer(new QTimer(this))
    , m_urlChangedTimer(new QTimer(this))
    , m_textHintProvider(new KateTextHintProvider(mainWindow, this))
{
    setComponentName(QStringLiteral("kate_diagnosticsview"), i18n("Diagnostics View"));
    setXMLFile(QStringLiteral("ui.rc"));

    auto l = new QVBoxLayout(this);
    l->setContentsMargins({});
    setupDiagnosticViewToolbar(l);

    m_diagLimitReachedWarning->setText(i18n("Diagnostics limit reached, ignoring further diagnostics. The limit can be configured in settings."));
    m_diagLimitReachedWarning->setWordWrap(true);
    m_diagLimitReachedWarning->setCloseButtonVisible(false);
    m_diagLimitReachedWarning->setMessageType(KMessageWidget::Warning);
    l->addWidget(m_diagLimitReachedWarning);
    m_diagLimitReachedWarning->hide(); // hidden by default

    l->addWidget(m_diagnosticsTree);

    m_filterChangedTimer->setInterval(400);
    m_filterChangedTimer->callOnTimeout(this, [this] {
        m_proxy->setFilterRegularExpression(m_filterLineEdit->text());
    });

    m_model.setColumnCount(1);

    m_diagnosticsTree->setHeaderHidden(true);
    m_diagnosticsTree->setFocusPolicy(Qt::NoFocus);
    m_diagnosticsTree->setLayoutDirection(Qt::LeftToRight);
    m_diagnosticsTree->setSortingEnabled(false);
    m_diagnosticsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_diagnosticsTree->setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::TopEdge}));
    m_diagnosticsTree->setUniformRowHeights(true);
    m_diagnosticsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_diagnosticsTree->setItemDelegate(new DiagnosticsLocationTreeDelegate(this));

    m_proxy->setSourceModel(&m_model);
    m_proxy->setFilterKeyColumn(0);
    m_proxy->setRecursiveFilteringEnabled(true);
    m_proxy->setFilterRole(Qt::DisplayRole);

    m_diagnosticsTree->setModel(m_proxy);

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &DiagnosticsView::onViewChanged);

    connect(m_diagnosticsTree, &QTreeView::customContextMenuRequested, this, &DiagnosticsView::onContextMenuRequested);
    connect(m_diagnosticsTree, &QTreeView::clicked, this, &DiagnosticsView::goToItemLocation);
    connect(m_diagnosticsTree, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        onDoubleClicked(m_proxy->mapToSource(index));
    });

    auto *ac = actionCollection();

    auto *diagMenu = new KActionMenu(i18nc("@action:inmenu Diagnostics View actions menu", "Diagnostics View"), this);
    ac->addAction(QStringLiteral("tools_diagnosticsview"), diagMenu);

    auto *a = ac->addAction(QStringLiteral("kate_quick_fix"), this, &DiagnosticsView::quickFix);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setText(i18n("Quick Fix"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("quickopen")));
    KActionCollection::setDefaultShortcut(a, QKeySequence((Qt::CTRL | Qt::Key_Period)));

    a = ac->addAction(QStringLiteral("goto_next_diagnostic"), this, &DiagnosticsView::nextItem);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setText(i18n("Next Item"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    KActionCollection::setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_Right);

    a = ac->addAction(QStringLiteral("goto_prev_diagnostic"), this, &DiagnosticsView::previousItem);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setText(i18n("Previous Item"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    KActionCollection::setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_Left);

    a = ac->addAction(QStringLiteral("diagnostics_clear_filter"), this, [this]() {
        DiagnosticsView::filterViewTo(nullptr);
    });
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setText(i18n("Clear Diagnostics Filter"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-all")));
    KActionCollection::setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_C);

    a = ac->addAction(QStringLiteral("diagnostics_show_hover"), this, [this]() {
        if (auto v = m_mainWindow->activeView()) {
            onTextHint(v, v->cursorPosition(), true);
        }
    });
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setText(i18n("Show Hover Info"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-all")));

    m_posChangedTimer->setInterval(500);
    m_posChangedTimer->setSingleShot(true);
    m_posChangedTimer->callOnTimeout(this, [this] {
        auto v = m_mainWindow->activeView();
        if (v) {
            if (auto doc = v->document()) {
                syncDiagnostics(doc, v->cursorPosition(), true, false);
            }
        }
    });

    // collapse url changed events
    m_urlChangedTimer->setInterval(100);
    m_urlChangedTimer->setSingleShot(true);
    m_urlChangedTimer->callOnTimeout(this, &DiagnosticsView::onDocumentUrlChanged);

    // handle tab button creation
    connect(mainWindow->window(), SIGNAL(tabForToolViewAdded(QWidget *, QWidget *)), this, SLOT(tabForToolViewAdded(QWidget *, QWidget *)));

    connect(m_textHintProvider.get(), &KateTextHintProvider::textHintRequested, this, [this](KTextEditor::View *v, KTextEditor::Cursor pos) {
        onTextHint(v, pos, /*manual=*/false);
    });
    connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &DiagnosticsView::handleEsc);

    mainWindow->guiFactory()->addClient(this);

    // We don't want our shortcuts available in the whole mainwindow, as it can conflict
    // with other KParts' shortcus (e.g. Konsole). The KateViewManager will be added to
    // the asscioted widgets in KateMainWindow::setupDiagnosticsView(), after the view
    // manager has been initialized.
    ac->clearAssociatedWidgets();
    ac->addAssociatedWidget(this);

    auto readConfig = [this] {
        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
        m_diagnosticLimit = cgGeneral.readEntry("Diagnostics Limit", 12000);

        const bool diagnosticsAreLimited = m_diagnosticLimit > -1;
        if (!diagnosticsAreLimited && m_diagLimitReachedWarning->isVisible()) {
            m_diagLimitReachedWarning->hide();
        }
    };
    connect(KateApp::self(), &KateApp::configurationChanged, this, readConfig);
    readConfig();
}

DiagnosticsView *DiagnosticsView::instance(KTextEditor::MainWindow *mainWindow)
{
    Q_ASSERT(mainWindow);
    auto dv = static_cast<DiagnosticsView *>(mainWindow->property("diagnosticsView").value<QObject *>());
    if (!dv) {
        auto tv = mainWindow->createToolView(nullptr /* toolview has no plugin it belongs to */,
                                             QStringLiteral("diagnostics"),
                                             KTextEditor::MainWindow::Bottom,
                                             QIcon::fromTheme(QStringLiteral("dialog-warning-symbolic")),
                                             i18n("Diagnostics"));
        dv = new DiagnosticsView(tv, mainWindow);
        mainWindow->setProperty("diagnosticsView", QVariant::fromValue(dv));
    }
    return dv;
}

DiagnosticsView::~DiagnosticsView()
{
    // clear the model first so that destruction is faster
    m_model.clear();
    onViewChanged(nullptr);
    const auto providers = m_providers;
    for (auto *p : providers) {
        unregisterDiagnosticsProvider(p);
    }
    Q_ASSERT(m_providers.empty());

    // KDevelop's main window is destroyed before plugins are unloaded
    if (m_mainWindow) {
        m_mainWindow->guiFactory()->removeClient(this);
    }
}

void DiagnosticsView::setupDiagnosticViewToolbar(QVBoxLayout *mainLayout)
{
    mainLayout->setSpacing(0);
    auto l = new QHBoxLayout();
    l->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                          style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                          style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                          style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
    l->setSpacing(style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));
    mainLayout->addLayout(l);

    l->addWidget(m_providerCombo);
    m_providerModel = new ProviderListModel(this);
    m_providerCombo->setModel(m_providerModel);
    m_providerCombo->setCurrentIndex(0);
    connect(m_providerCombo, &QComboBox::currentIndexChanged, this, [this] {
        auto proxy = static_cast<DiagnosticsProxyModel *>(m_proxy);
        proxy->setActiveProvider(m_providerCombo->currentData().value<DiagnosticsProvider *>());
        m_diagnosticsTree->expandAll();
    });

    m_errFilterBtn->setIcon(QIcon::fromTheme(QStringLiteral("data-error")));
    m_errFilterBtn->setText(i18n("Errors"));
    m_errFilterBtn->setCheckable(true);
    m_errFilterBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    l->addWidget(m_errFilterBtn);
    connect(m_errFilterBtn, &QToolButton::clicked, this, [this](bool c) {
        if (m_warnFilterBtn->isChecked()) {
            const QSignalBlocker b(m_warnFilterBtn);
            m_warnFilterBtn->setChecked(false);
        }
        auto proxy = static_cast<DiagnosticsProxyModel *>(m_proxy);
        proxy->setActiveSeverity(c ? DiagnosticSeverity::Error : DiagnosticSeverity::Unknown);
        QTimer::singleShot(200, m_diagnosticsTree, &QTreeView::expandAll);
    });

    m_warnFilterBtn->setIcon(QIcon::fromTheme(QStringLiteral("data-warning")));
    m_warnFilterBtn->setCheckable(true);
    m_warnFilterBtn->setText(i18n("Warnings"));
    m_warnFilterBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    l->addWidget(m_warnFilterBtn);
    connect(m_warnFilterBtn, &QToolButton::clicked, this, [this](bool c) {
        if (m_errFilterBtn->isChecked()) {
            const QSignalBlocker b(m_errFilterBtn);
            m_errFilterBtn->setChecked(false);
        }
        auto proxy = static_cast<DiagnosticsProxyModel *>(m_proxy);
        proxy->setActiveSeverity(c ? DiagnosticSeverity::Warning : DiagnosticSeverity::Unknown);
        QTimer::singleShot(200, m_diagnosticsTree, &QTreeView::expandAll);
    });

    l->addWidget(m_filterLineEdit);
    m_filterLineEdit->setPlaceholderText(i18n("Filter…"));
    m_filterLineEdit->setClearButtonEnabled(true);
    connect(m_filterLineEdit, &QLineEdit::textChanged, m_filterChangedTimer, [this] {
        m_filterChangedTimer->start();
    });

    m_clearButton->setToolTip(i18nc("@info:tooltip", "Clear diagnostics"));
    m_clearButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-all")));
    connect(m_clearButton, &QToolButton::clicked, this, [this] {
        std::vector<KTextEditor::Document *> docs(m_diagnosticsMarks.begin(), m_diagnosticsMarks.end());
        for (auto d : docs) {
            clearAllMarks(d);
        }
        m_model.clear();
        m_diagnosticsCount = 0;
        m_diagLimitReachedWarning->hide();
    });
    l->addWidget(m_clearButton);
}

void DiagnosticsView::onViewChanged(KTextEditor::View *v)
{
    disconnect(posChangedConnection);
    m_posChangedTimer->stop();
    if (v) {
        posChangedConnection = connect(v, &KTextEditor::View::cursorPositionChanged, this, [this] {
            m_posChangedTimer->start();
        });
        m_posChangedTimer->start();
    }

    if (v && v->document()) {
        connect(v->document(), &KTextEditor::Document::documentUrlChanged, m_urlChangedTimer, qOverload<>(&QTimer::start), Qt::UniqueConnection);
    }
}

void DiagnosticsView::registerDiagnosticsProvider(DiagnosticsProvider *provider)
{
    if (std::find(m_providers.begin(), m_providers.end(), provider) != m_providers.end()) {
        qWarning("already registered provider, ignoring!");
        return;
    }

    provider->diagnosticView = this;
    connect(provider, &DiagnosticsProvider::diagnosticsAdded, this, &DiagnosticsView::onDiagnosticsAdded);
    connect(provider, &DiagnosticsProvider::fixesAvailable, this, &DiagnosticsView::onFixesAvailable);
    connect(provider, &DiagnosticsProvider::requestClearDiagnostics, this, &DiagnosticsView::clearDiagnosticsFromProvider);
    connect(provider, &DiagnosticsProvider::requestClearSuppressions, this, &DiagnosticsView::clearSuppressionsFromProvider);
    connect(provider, &QObject::destroyed, this, [this](QObject *o) {
        unregisterDiagnosticsProvider(static_cast<DiagnosticsProvider *>(o));
    });
    m_providers.push_back(provider);

    m_providerModel->update(m_providers);
}

void DiagnosticsView::unregisterDiagnosticsProvider(DiagnosticsProvider *provider)
{
    auto it = std::find(m_providers.begin(), m_providers.end(), provider);
    if (it == m_providers.end()) {
        return;
    }
    disconnect(provider, &DiagnosticsProvider::diagnosticsAdded, this, &DiagnosticsView::onDiagnosticsAdded);
    disconnect(provider, &DiagnosticsProvider::fixesAvailable, this, &DiagnosticsView::onFixesAvailable);
    disconnect(provider, &DiagnosticsProvider::requestClearDiagnostics, this, &DiagnosticsView::clearDiagnosticsFromProvider);
    disconnect(provider, &DiagnosticsProvider::requestClearSuppressions, this, &DiagnosticsView::clearSuppressionsFromProvider);
    m_providers.erase(it);

    // save some work during shutdown
    if (!m_inShutdown) {
        m_providerModel->update(m_providers);

        // clear diagnostics
        clearDiagnosticsFromProvider(provider);
    }
}

void DiagnosticsView::readSessionConfig(const KConfigGroup &config)
{
    m_sessionDiagnosticSuppressions->readSessionConfig(config);
}

void DiagnosticsView::writeSessionConfig(KConfigGroup &config)
{
    m_sessionDiagnosticSuppressions->writeSessionConfig(config);
}

void DiagnosticsView::showEvent(QShowEvent *e)
{
    if (m_tabButtonOverlay) {
        m_tabButtonOverlay->setActive(false);
    }
    QWidget::showEvent(e);
}

void DiagnosticsView::handleEsc(QEvent *event)
{
    if (event->type() != QEvent::ShortcutOverride)
        return;

    auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent && keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier) {
        if (auto p = qobject_cast<QWidget *>(parent()); p && p->isVisible()) {
            m_mainWindow->hideToolView(p);
            event->accept();
        }
    }
}

void DiagnosticsView::tabForToolViewAdded(QWidget *toolView, QWidget *tab)
{
    if (parent() == toolView) {
        m_tabButtonOverlay = new DiagTabOverlay(tab);
        m_tabButtonOverlay->setActive(!isVisible() && m_model.rowCount() > 0);
    }
}

static DocumentDiagnosticItem *getItem(const QStandardItemModel &model, const QUrl &url)
{
    // url in custom role, Qt::DisplayRole might have additional elements
    auto l = model.match(model.index(0, 0, QModelIndex()), Qt::UserRole, url, 1, Qt::MatchExactly);
    if (!l.empty()) {
        return static_cast<DocumentDiagnosticItem *>(model.itemFromIndex(l.at(0)));
    }
    return nullptr;
}

static QStandardItem *getItem(const QStandardItem *topItem, KTextEditor::Cursor pos, bool onlyLine, DiagnosticSeverity severity = DiagnosticSeverity::Unknown)
{
    QStandardItem *targetItem = nullptr;
    if (topItem) {
        int count = topItem->rowCount();
        int first = 0, last = count;
        // let's not run wild on a linear search in a flood of diagnostics
        if (count > 50) {
            // instead, let's *assume* sorted and use binary search to get closer
            // it probably is sorted, so it should work out
            // if not, at least we tried (without spending/wasting more on sorting)
            auto getLine = [topItem, count](int index) {
                Q_ASSERT(index >= 0);
                Q_ASSERT(index < count);
                auto range = topItem->child(index)->data(DiagnosticModelRole::RangeRole).value<KTextEditor::Range>();
                return range.start().line();
            };
            int first = 0, last = count - 1;
            int target = pos.line();
            while (first + 1 < last) {
                int middle = first + ((last - first) / 2);
                Q_ASSERT(first != middle);
                Q_ASSERT(middle != last);
                if (getLine(middle) < target) {
                    first = middle;
                } else {
                    last = middle;
                }
            }
        }
        for (int i = first; i < last; ++i) {
            auto item = topItem->child(i);
            if (!(item->flags() & Qt::ItemIsEnabled)) {
                continue;
            }

            auto range = item->data(DiagnosticModelRole::RangeRole).value<KTextEditor::Range>();
            if ((onlyLine && pos.line() == range.start().line()) || (range.contains(pos))) {
                // the severity must match if it was specified
                if (severity != DiagnosticSeverity::Unknown && item->type() == DiagnosticItem_Diag
                    && static_cast<DiagnosticItem *>(item)->m_diagnostic.severity != severity) {
                    continue;
                }
                targetItem = item;
                break;
            }
        }
    }
    return targetItem;
}

static void fillItemRoles(QStandardItem *item, const QUrl &url, const KTextEditor::Range _range, DiagnosticSeverity kind)
{
    // auto range = snapshot ? transformRange(url, *snapshot, _range) : _range;
    item->setData(QVariant(url), DiagnosticModelRole::FileUrlRole);
    QVariant vrange;
    vrange.setValue(_range);
    item->setData(vrange, DiagnosticModelRole::RangeRole);
    item->setData(QVariant::fromValue(kind), DiagnosticModelRole::KindRole);
}

void DiagnosticsView::onFixesAvailable(const QList<DiagnosticFix> &fixes, const QVariant &data)
{
    if (fixes.empty() || data.isNull()) {
        return;
    }
    const auto diagModelIdx = data.value<DiagModelIndex>();
    if (diagModelIdx.parentRow == -1) {
        qWarning("Unexpected -1 parentRow");
        return;
    }
    const auto parentIdx = m_model.index(diagModelIdx.parentRow, 0);
    const auto idx = m_model.index(diagModelIdx.row, 0, parentIdx);
    if (!idx.isValid()) {
        qWarning("%s, Unexpected invalid idx", Q_FUNC_INFO);
        return;
    }
    const auto item = m_model.itemFromIndex(idx);
    if (!item) {
        return;
    }
    bool autoApply = diagModelIdx.autoApply;
    if (autoApply) {
        if (fixes.size() == 1) {
            fixes.constFirst().fixCallback();
        } else {
            showFixesInMenu(fixes);
        }
        return;
    }
    for (const auto &fix : fixes) {
        if (!fix.fixCallback) {
            continue;
        }
        auto f = new DiagnosticFixItem;
        f->fix = fix;
        f->setText(fix.fixTitle);
        item->appendRow(f);
    }
}

void DiagnosticsView::showFixesInMenu(const QList<DiagnosticFix> &fixes)
{
    auto av = m_mainWindow->activeView();
    if (av) {
        auto pos = av->cursorPositionCoordinates();
        QMenu menu(this);
        for (const auto &fix : fixes) {
            menu.addAction(fix.fixTitle, fix.fixCallback);
        }
        menu.exec(av->mapToGlobal(pos));
    }
}

void DiagnosticsView::quickFix()
{
    KTextEditor::View *activeView = m_mainWindow->activeView();
    if (!activeView) {
        return;
    }
    KTextEditor::Document *document = activeView->document();

    if (!document) {
        return;
    }

    QStandardItem *topItem = getItem(m_model, document->url());

    // try to find current diagnostic based on cursor position
    auto pos = activeView->cursorPosition();
    QStandardItem *targetItem = getItem(topItem, pos, false);
    if (!targetItem) {
        // match based on line position only
        targetItem = getItem(topItem, pos, true);
    }

    if (targetItem && targetItem->type() == DiagnosticItem_Diag) {
        if (static_cast<DiagnosticItem *>(targetItem)->hasFixes()) {
            QList<DiagnosticFix> fixes;
            for (int i = 0; i < targetItem->rowCount(); ++i) {
                auto item = targetItem->child(i);
                if (item && item->type() == DiagnosticItem_Fix) {
                    fixes << static_cast<DiagnosticFixItem *>(item)->fix;
                }
            }
            if (fixes.size() > 1) {
                showFixesInMenu(fixes);
            } else if (fixes.size() == 1 && fixes[0].fixCallback) {
                fixes[0].fixCallback();
            }
        } else {
            onDoubleClicked(m_model.indexFromItem(targetItem), true);
        }
    }
}

void DiagnosticsView::onDoubleClicked(const QModelIndex &index, bool quickFix)
{
    auto itemFromIndex = m_model.itemFromIndex(index);
    if (!itemFromIndex) {
        qWarning("invalid item clicked");
        return;
    }

    if (itemFromIndex->type() == DiagnosticItem_Diag) {
        if (static_cast<DiagnosticItem *>(itemFromIndex)->hasFixes()) {
            return;
        }
        auto provider = index.data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>();
        if (!provider) {
            return;
        }
        auto item = static_cast<DiagnosticItem *>(itemFromIndex);
        DiagModelIndex idx;
        idx.row = item->row();
        idx.parentRow = item->parent() ? item->parent()->row() : -1;
        idx.autoApply = quickFix;
        QVariant data = QVariant::fromValue(idx);
        Q_EMIT provider->requestFixes(item->data(DiagnosticModelRole::FileUrlRole).toUrl(), item->m_diagnostic, data);
    }

    if (itemFromIndex->type() == DiagnosticItem_Fix) {
        static_cast<DiagnosticFixItem *>(itemFromIndex)->fix.fixCallback();
    }
}

void DiagnosticsView::onDiagnosticsAdded(const FileDiagnostics &diagnostics)
{
    auto view = m_mainWindow->activeView();
    auto doc = view ? view->document() : nullptr;
    // We allow diagnostics for the active document always because it might be that diagnostic limit is reached
    // and the user doesn't know about it and thus won't get any further diagnostics at all while typing
    const bool diagnosticsAreForActiveDoc = doc ? diagnostics.uri == doc->url() : false;

    const bool diagnosticsAreLimited = m_diagnosticLimit > -1;
    if (diagnosticsAreLimited && m_diagnosticsCount > m_diagnosticLimit && !diagnosticsAreForActiveDoc) {
        return;
    }

    auto diagWarnShowGuard = qScopeGuard([this] {
        const bool diagnosticsAreLimited = m_diagnosticLimit > -1;
        if (!diagnosticsAreLimited) {
            return;
        }

        if (m_diagnosticsCount > m_diagnosticLimit && !m_diagLimitReachedWarning->isVisible()) {
            m_diagLimitReachedWarning->show();
        } else if (m_diagnosticsCount < m_diagnosticLimit && m_diagLimitReachedWarning->isVisible()) {
            m_diagLimitReachedWarning->hide();
        }
    });

    auto *provider = qobject_cast<DiagnosticsProvider *>(sender());
    Q_ASSERT(provider);

    QStandardItemModel *model = &m_model;
    DocumentDiagnosticItem *topItem = getItem(m_model, diagnostics.uri);

    auto toProxyIndex = [this](const QModelIndex &index) {
        return m_proxy->mapFromSource(index);
    };

    // current diagnostics row, if one of incoming diagnostics' document
    int row = -1;
    if (!topItem) {
        // no need to create an empty one
        if (diagnostics.diagnostics.empty()) {
            return;
        }
        topItem = new DocumentDiagnosticItem();
        model->appendRow(topItem);
        QString prettyUri = diagnostics.uri.toString(QUrl::PreferLocalFile | QUrl::RemovePassword);
        topItem->setText(prettyUri);
        topItem->setData(diagnostics.uri, Qt::UserRole);
    } else {
        // try to retain current position
        auto currentIndex = m_proxy->mapToSource(m_diagnosticsTree->currentIndex());
        if (currentIndex.parent() == topItem->index()) {
            row = currentIndex.row();
        }

        // Remove old diagnostics of this provider
        if (!provider->m_persistentDiagnostics) {
            int removedCount = topItem->removeItemsForProvider(provider);
            m_diagnosticsCount -= removedCount;
        }
    }
    topItem->addProvider(provider);
    // emit so that proxy can update
    m_model.dataChanged(topItem->index(), topItem->index());

    QList<QStandardItem *> diagItems;
    diagItems.reserve(diagnostics.diagnostics.size());
    for (const auto &diag : diagnostics.diagnostics) {
        auto item = new DiagnosticItem(diag);
        diagItems.push_back(item);
        QString source;
        if (diag.source.length()) {
            source = QStringLiteral("[%1] ").arg(diag.source);
        }
        if (diag.code.length()) {
            source += QStringLiteral("(%1) ").arg(diag.code);
        }
        item->setData(diagnosticsIcon(diag.severity), Qt::DecorationRole);
        // rendering of lines with embedded newlines does not work so well
        // so ... split message by lines
        auto lines = diag.message.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        item->setText(source + (!lines.empty() ? lines[0] : QString()));
        fillItemRoles(item, diagnostics.uri, diag.range, diag.severity);
        item->setData(QVariant::fromValue(provider), DiagnosticModelRole::ProviderRole);
        // add subsequent lines to subitems
        // no metadata is added to these,
        // as it can be taken from the parent (for marks and ranges)
        for (int l = 1; l < lines.size(); ++l) {
            auto subitem = new QStandardItem();
            subitem->setText(lines[l]);
            item->appendRow(subitem);
        }
        const auto &relatedInfo = diag.relatedInformation;
        for (const auto &related : relatedInfo) {
            if (related.location.uri.isEmpty()) {
                continue;
            }
            auto relatedItemMessage = new QStandardItem();

            fillItemRoles(relatedItemMessage, related.location.uri, related.location.range, DiagnosticSeverity::Information);

            auto basename = QFileInfo(related.location.uri.toLocalFile()).fileName();
            // display line number is 1-based (as opposed to internal 0-based)
            auto location = QStringLiteral("%1:%2").arg(basename).arg(related.location.range.start().line() + 1);
            relatedItemMessage->setText(QStringLiteral("[%1] %2").arg(location).arg(related.message));
            relatedItemMessage->setData(diagnosticsIcon(DiagnosticSeverity::Information), Qt::DecorationRole);
            item->appendRow(relatedItemMessage);
        }
    }
    m_diagnosticsCount += diagItems.size();
    topItem->appendRows(diagItems);

    // TODO perhaps add some custom delegate that only shows 1 line
    // and only the whole text when item selected ??
    m_diagnosticsTree->expandRecursively(toProxyIndex(topItem->index()), 1);

    // topItem might be removed after this
    updateDiagnosticsState(topItem);
    // also sync updated diagnostic to current position
    auto currentView = m_mainWindow->activeView();
    if (topItem && currentView && currentView->document()) {
        if (!syncDiagnostics(currentView->document(), currentView->cursorPosition(), false, false)) {
            // avoid jitter; only restore previous if applicable
            if (row >= 0 && row < topItem->rowCount()) {
                m_diagnosticsTree->scrollTo(toProxyIndex(topItem->child(row)->index()));
            }
        }
    }
    if (m_tabButtonOverlay) {
        m_tabButtonOverlay->setActive(!isVisible() && m_model.rowCount() > 0);
    }
}

static auto getProvider(QStandardItem *item)
{
    return item->data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>();
}

void DiagnosticsView::clearDiagnosticsFromProvider(DiagnosticsProvider *provider)
{
    clearDiagnosticsForStaleDocs({}, provider);
}

void DiagnosticsView::clearDiagnosticsForStaleDocs(const std::pmr::unordered_set<QUrl, QUrlHash> &filesToKeep, DiagnosticsProvider *provider)
{
    auto diagWarnShowGuard = qScopeGuard([this] {
        const bool diagnosticsAreLimited = m_diagnosticLimit > -1;
        if (!diagnosticsAreLimited) {
            return;
        }

        if (m_diagnosticsCount < m_diagnosticLimit && m_diagLimitReachedWarning->isVisible()) {
            m_diagLimitReachedWarning->hide();
        }
    });

    auto all_diags_from_provider = [provider](DocumentDiagnosticItem *file) {
        if (file->rowCount() == 0) {
            return true;
        }
        if (provider && file->providers().size() == 1 && file->providers().contains(provider)) {
            return true;
        }
        if (!provider) {
            const QList<DiagnosticsProvider *> &providers = file->providers();
            return std::all_of(providers.begin(), providers.end(), [](DiagnosticsProvider *p) {
                return !p->persistentDiagnostics();
            });
        }
        return false;
    };

    auto bulk_remove = [this](QStandardItemModel *model, int &start, int &count, int &i) {
        if (start > -1 && count != 0) {
            for (int r = start; r < (start + count); r++) {
                m_diagnosticsCount -= model->item(r)->rowCount();
            }
            model->removeRows(start, count);
            i = start - 1; // reset i
        }
        start = -1;
        count = 0;
    };

    int start = -1, count = 0;

    for (int i = 0; i < m_model.rowCount(); ++i) {
        auto fileItem = static_cast<DocumentDiagnosticItem *>(m_model.item(i));
        if (!filesToKeep.contains(fileItem->data(Qt::UserRole).toUrl())) {
            if (!all_diags_from_provider(fileItem)) {
                // If the diagnostics of this file item are from multiple providers
                // delete the ones from @p provider
                bulk_remove(&m_model, start, count, i);
                int removedCount = fileItem->removeItemsForProvider(provider);
                m_diagnosticsCount -= removedCount;
            } else {
                if (start == -1) {
                    start = i;
                }
                count += 1;
            }
        } else {
            bulk_remove(&m_model, start, count, i);
        }
    }

    if (start != -1 && count != 0) {
        for (int r = start; r < (start + count); r++) {
            m_diagnosticsCount -= m_model.item(r)->rowCount();
        }
        m_model.removeRows(start, count);
    }

    updateMarks();

    if (m_tabButtonOverlay) {
        m_tabButtonOverlay->setActive(!isVisible() && m_model.rowCount() > 0);
    }
}

void DiagnosticsView::clearSuppressionsFromProvider(DiagnosticsProvider *provider)
{
    // need to clear suppressions
    // will be filled again at suitable time by re-requesting provider
    for (int i = 0; i < m_model.rowCount(); ++i) {
        auto item = m_model.item(i);
        if (item->type() != DiagnosticItem_File) {
            continue;
        }
        auto fileItem = static_cast<DocumentDiagnosticItem *>(item);
        if (getProvider(fileItem) == provider) {
            fileItem->diagnosticSuppression.reset();
        }
    }
}

void DiagnosticsView::addMarks(KTextEditor::Document *doc, QStandardItem *item)
{
    Q_ASSERT(item);
    using Style = KSyntaxHighlighting::Theme::TextStyle;

    // only consider enabled items
    if (!(item->flags() & Qt::ItemIsEnabled)) {
        return;
    }

    const auto url = item->data(DiagnosticModelRole::FileUrlRole).toUrl().toDisplayString(QUrl::PreferLocalFile | QUrl::PrettyDecoded | QUrl::RemovePassword);
    const auto docUrl = doc->url().toDisplayString(QUrl::PreferLocalFile | QUrl::PrettyDecoded | QUrl::RemovePassword);
    // document url could end up empty while in intermediate reload state
    // (and then it might match a parent item with no RangeData at all)
    if (url != docUrl || url.isEmpty()) {
        return;
    }

    const auto range = item->data(DiagnosticModelRole::RangeRole).value<KTextEditor::Range>();
    if (!range.isValid()) {
        return;
    }

    KTextEditor::Attribute::Ptr attr;
    KTextEditor::Document::MarkTypes markType = markTypeDiagWarning;

    const auto kind = item->data(DiagnosticModelRole::KindRole).value<DiagnosticSeverity>();
    switch (kind) {
    // use underlining for diagnostics to avoid lots of fancy flickering
    case DiagnosticSeverity::Error: {
        static KTextEditor::Attribute::Ptr errorAttr;
        if (!errorAttr) {
            const auto theme = KTextEditor::Editor::instance()->theme();
            errorAttr = new KTextEditor::Attribute();
            errorAttr->setUnderlineColor(theme.textColor(Style::Error));
            errorAttr->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        }
        attr = errorAttr;
        markType = markTypeDiagError;
        break;
    }
    case DiagnosticSeverity::Warning: {
        static KTextEditor::Attribute::Ptr warnAttr;
        if (!warnAttr) {
            const auto theme = KTextEditor::Editor::instance()->theme();
            warnAttr = new KTextEditor::Attribute();
            warnAttr->setUnderlineColor(theme.textColor(Style::Warning));
            warnAttr->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        }
        attr = warnAttr;
        markType = markTypeDiagWarning;
        break;
    }
    case DiagnosticSeverity::Information:
    case DiagnosticSeverity::Hint: {
        static KTextEditor::Attribute::Ptr infoAttr;
        if (!infoAttr) {
            const auto theme = KTextEditor::Editor::instance()->theme();
            infoAttr = new KTextEditor::Attribute();
            infoAttr->setUnderlineColor(theme.textColor(Style::Information));
            infoAttr->setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        }
        attr = infoAttr;
        markType = markTypeDiagOther;
        break;
    }
    case DiagnosticSeverity::Unknown:
        qWarning("Unknown diagnostic severity");
        return;
    }

    if (!attr) {
        qWarning("Unexpected null attr");
    }

    // highlight the range
    if (attr) {
        KTextEditor::Range highlightedRange = range;
        if (range.isEmpty()) {
            auto end = highlightedRange.end();
            end.setColumn(doc->lineLength(highlightedRange.start().line()));
            highlightedRange.setEnd(end);
        }
        KTextEditor::MovingRange *mr = doc->newMovingRange(highlightedRange);
        mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
        mr->setAttribute(attr);
        mr->setAttributeOnlyForViews(true);
        m_diagnosticsRanges[doc].push_back(mr);
    }

    auto iface = doc;
    // add match mark for range
    switch (markType) {
    case markTypeDiagError:
        iface->setMarkDescription(markType, i18n("Error"));
        iface->setMarkIcon(markType, diagnosticsIcon(DiagnosticSeverity::Error));
        break;
    case markTypeDiagWarning:
        iface->setMarkDescription(markType, i18n("Warning"));
        iface->setMarkIcon(markType, diagnosticsIcon(DiagnosticSeverity::Warning));
        break;
    case markTypeDiagOther:
        iface->setMarkDescription(markType, i18n("Information"));
        iface->setMarkIcon(markType, diagnosticsIcon(DiagnosticSeverity::Information));
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    const auto line = range.start().line();
    iface->addMark(line, markType);
    m_diagnosticsMarks.insert(doc);

    // ensure runtime match
    using Doc = KTextEditor::Document;
    connect(doc, &Doc::aboutToInvalidateMovingInterfaceContent, this, &DiagnosticsView::clearAllMarks, Qt::UniqueConnection);
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 9, 0)
    connect(doc, &Doc::aboutToDeleteMovingInterfaceContent, this, &DiagnosticsView::clearAllMarks, Qt::UniqueConnection);
#endif
    // reload might save/restore marks before/after above signals, so let's clear before that
    connect(doc, &Doc::aboutToReload, this, &DiagnosticsView::clearAllMarks, Qt::UniqueConnection);
    connect(doc, &Doc::markClicked, this, &DiagnosticsView::onMarkClicked, Qt::UniqueConnection);
}

void DiagnosticsView::addMarksRec(KTextEditor::Document *doc, QStandardItem *item)
{
    Q_ASSERT(item);
    // We only care about @p doc items
    if (item->type() == DiagnosticItem_File && item->data(Qt::UserRole).toUrl() != doc->url()) {
        return;
    }
    addMarks(doc, item);
    for (int i = 0; i < item->rowCount(); ++i) {
        addMarksRec(doc, item->child(i));
    }
}

void DiagnosticsView::addMarks(KTextEditor::Document *doc)
{
    // check if already added
    if (m_diagnosticsRanges.contains(doc) && m_diagnosticsMarks.contains(doc)) {
        return;
    }
    addMarksRec(doc, m_model.invisibleRootItem());
}

void DiagnosticsView::clearAllMarks(KTextEditor::Document *doc)
{
    if (m_diagnosticsMarks.contains(doc)) {
        const QHash<int, KTextEditor::Mark *> marks = doc->marks();
        for (auto mark : marks) {
            if (mark->type & markTypeDiagAll) {
                doc->removeMark(mark->line, markTypeDiagAll);
            }
        }
        m_diagnosticsMarks.remove(doc);
    }

    auto it = m_diagnosticsRanges.find(doc);
    if (it != m_diagnosticsRanges.end()) {
        for (auto range : it.value()) {
            delete range;
        }
        it = m_diagnosticsRanges.erase(it);
    }
}

void DiagnosticsView::updateMarks(const std::span<QUrl> &urls)
{
    std::vector<KTextEditor::Document *> docs;
    if (!urls.empty()) {
        auto app = KTextEditor::Editor::instance()->application();
        for (const auto &url : urls) {
            if (auto doc = app->findUrl(url)) {
                docs.push_back(doc);
            }
        }
    } else {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (activeView && activeView->document()) {
            docs.push_back(activeView->document());
        }
    }

    for (auto doc : docs) {
        clearAllMarks(doc);
        addMarks(doc);
    }
}

void DiagnosticsView::updateDiagnosticsState(DocumentDiagnosticItem *&topItem)
{
    if (!topItem) {
        return;
    }

    auto diagTopItem = static_cast<DocumentDiagnosticItem *>(topItem);
    auto enabled = diagTopItem->enabled;
    auto suppressions = enabled ? diagTopItem->diagnosticSuppression.get() : nullptr;
    auto url = topItem->data(Qt::UserRole).toUrl();
    auto doc = KTextEditor::Editor::instance()->application()->findUrl(url);

    const int totalCount = topItem->rowCount();
    int count = 0;
    for (int i = 0; i < totalCount; ++i) {
        auto item = topItem->child(i);
        if (!item) {
            continue;
        }

        auto hide = suppressions && suppressions->match(*item, doc);
        // mark accordingly as flag and (un)hide
        Qt::ItemFlags flags = item->flags();
        const bool itemIsEnabled = flags.testFlag(Qt::ItemIsEnabled);
        if (itemIsEnabled != !hide) {
            flags.setFlag(Qt::ItemIsEnabled, !hide);
            if (item->flags() != flags) {
                item->setFlags(flags);
            }
            const auto proxyIdx = m_proxy->mapFromSource(item->index());
            if (m_diagnosticsTree->isRowHidden(proxyIdx.row(), proxyIdx.parent()) != hide) {
                m_diagnosticsTree->setRowHidden(proxyIdx.row(), proxyIdx.parent(), hide);
            }
        }
        count += hide ? 0 : 1;
    }
    // adjust file item level text
    auto suppressed = totalCount - count;
    const QString path = url.toString(QUrl::PreferLocalFile | QUrl::RemovePassword);
    topItem->setText(suppressed ? i18nc("@info", "%1 [suppressed: %2]", path, suppressed) : path);
    // only hide if really nothing below
    const auto proxyIdx = m_proxy->mapFromSource(topItem->index());
    m_diagnosticsTree->setRowHidden(proxyIdx.row(), proxyIdx.parent(), totalCount == 0);
    if (topItem->rowCount() == 0) {
        m_model.removeRow(topItem->row());
        topItem = nullptr;
    }

    QUrl urls[] = {QUrl::fromLocalFile(path)};
    updateMarks(urls);
}

void DiagnosticsView::goToItemLocation(QModelIndex index)
{
    auto getPrimaryModelIndex = [](QModelIndex index) {
        // in case of a multiline diagnostics item, a split secondary line has no data set
        // so we need to go up to the primary parent item
        if (!index.data(DiagnosticModelRole::RangeRole).isValid() && index.parent().data(DiagnosticModelRole::RangeRole).isValid()) {
            return index.parent();
        }
        return index;
    };

    index = getPrimaryModelIndex(index);
    auto url = index.data(DiagnosticModelRole::FileUrlRole).toUrl();
    auto start = index.data(DiagnosticModelRole::RangeRole).value<KTextEditor::Range>();
    if (url.isEmpty()) {
        return;
    }

    Utils::goToDocumentLocation(m_mainWindow, url, start);
}

void DiagnosticsView::onMarkClicked(KTextEditor::Document *document, KTextEditor::Mark mark, bool &handled)
{
    // no action if no mark was sprinkled here
    if (m_diagnosticsMarks.contains(document) && syncDiagnostics(document, {mark.line, 0}, false, true)) {
        handled = true;
    }
}

void DiagnosticsView::showToolview(DiagnosticsProvider *filterTo)
{
    m_mainWindow->showToolView(qobject_cast<QWidget *>(parent()));
    filterViewTo(filterTo);
}

void DiagnosticsView::filterViewTo(DiagnosticsProvider *provider)
{
    if (provider) {
        auto index = m_providerCombo->findData(QVariant::fromValue(provider));
        if (index != -1) {
            m_providerCombo->setCurrentIndex(index);
        }
    } else {
        m_providerCombo->setCurrentIndex(0);
    }
}

bool DiagnosticsView::syncDiagnostics(KTextEditor::Document *document, KTextEditor::Cursor pos, bool allowTop, bool doShow)
{
    auto hint = QAbstractItemView::PositionAtTop;
    DocumentDiagnosticItem *topItem = getItem(m_model, document->url());
    updateDiagnosticsSuppression(topItem, document);
    auto proxy = static_cast<DiagnosticsProxyModel *>(m_proxy);
    auto severity = proxy->activeSeverity();
    QStandardItem *targetItem = getItem(topItem, pos, /*onlyLine=*/pos.column() == 0, severity);
    if (targetItem) {
        hint = QAbstractItemView::PositionAtCenter;
    }
    if (!targetItem && allowTop) {
        targetItem = topItem;
    }
    if (targetItem) {
        if (const auto idx = m_proxy->mapFromSource(targetItem->index()); idx.isValid()) {
            m_diagnosticsTree->blockSignals(true);
            m_diagnosticsTree->scrollTo(idx, hint);
            m_diagnosticsTree->setCurrentIndex(idx);
            m_diagnosticsTree->blockSignals(false);

            if (doShow) {
                m_mainWindow->showToolView(qobject_cast<QWidget *>(parent()));
            }

            return true;
        }
    }
    return false;
}

void DiagnosticsView::updateDiagnosticsSuppression(DocumentDiagnosticItem *diagTopItem, KTextEditor::Document *doc, bool force)
{
    if (!diagTopItem) {
        return;
    }

    auto &suppressions = diagTopItem->diagnosticSuppression;
    if (!suppressions || force) {
        std::vector<QJsonObject> providerSupressions;
        const QList<DiagnosticsProvider *> &providers = diagTopItem->providers();
        if (doc) {
            for (auto p : providers) {
                auto suppressions = p->suppressions(doc);
                if (!suppressions.isEmpty()) {
                    providerSupressions.push_back(suppressions);
                }
            }
        }

        const auto docUrl = diagTopItem->data(Qt::UserRole).toUrl();
        const auto sessionSuppressions = m_sessionDiagnosticSuppressions->getSuppressions(docUrl.toLocalFile());
        auto supp = new DiagnosticSuppression(docUrl, providerSupressions, sessionSuppressions);
        const bool hadSuppression = suppressions != nullptr;
        suppressions.reset(supp);
        if (!providerSupressions.empty() || !sessionSuppressions.empty() || hadSuppression) {
            updateDiagnosticsState(diagTopItem);
        }
    }
}

void DiagnosticsView::onContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);

    auto menu = new QMenu(m_diagnosticsTree);
    menu->addAction(i18n("Expand All"), m_diagnosticsTree, &QTreeView::expandAll);
    menu->addAction(i18n("Collapse All"), m_diagnosticsTree, &QTreeView::collapseAll);
    menu->addSeparator();

    QModelIndex index = m_proxy->mapToSource(m_diagnosticsTree->currentIndex());
    if (QStandardItem *item = m_model.itemFromIndex(index)) {
        auto diagText = index.data().toString();
        menu->addAction(QIcon::fromTheme(QLatin1String("edit-copy")), i18n("Copy to Clipboard"), [diagText]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(diagText);
        });

        if (item->type() == DiagnosticItem_Diag) {
            menu->addSeparator();
            auto parent = index.parent();
            auto docDiagItem = static_cast<DocumentDiagnosticItem *>(m_model.itemFromIndex(parent));
            // track validity of raw pointer
            QPersistentModelIndex pindex(parent);
            auto h = [this, pindex, diagText, docDiagItem](bool add, const QString &file, const QString &diagnostic) {
                if (!pindex.isValid()) {
                    return;
                }
                if (add) {
                    m_sessionDiagnosticSuppressions->add(file, diagnostic);
                } else {
                    m_sessionDiagnosticSuppressions->remove(file, diagnostic);
                }

                auto app = KTextEditor::Editor::instance()->application();
                if (file.isEmpty()) {
                    // global
                    const int rows = m_model.rowCount();
                    for (int i = 0; i < rows; ++i) {
                        auto item = m_model.item(i);
                        if (item->type() == DiagnosticItem_File && item->rowCount() > 0) {
                            auto docItem = static_cast<DocumentDiagnosticItem *>(item);
                            auto url = docItem->data(Qt::UserRole).toUrl();
                            updateDiagnosticsSuppression(docItem, app->findUrl(url), true);
                        }
                    }
                } else {
                    // local
                    auto url = docDiagItem->data(Qt::UserRole).toUrl();
                    updateDiagnosticsSuppression(docDiagItem, app->findUrl(url), true);
                }
            };
            using namespace std::placeholders;
            const auto empty = QString();
            if (m_sessionDiagnosticSuppressions->hasSuppression(empty, diagText)) {
                menu->addAction(i18n("Remove Global Suppression"), this, std::bind(h, false, empty, diagText));
            } else {
                menu->addAction(i18n("Add Global Suppression"), this, std::bind(h, true, empty, diagText));
            }
            auto file = parent.data(Qt::UserRole).toUrl().toLocalFile();
            if (m_sessionDiagnosticSuppressions->hasSuppression(file, diagText)) {
                menu->addAction(i18n("Remove Local Suppression"), this, std::bind(h, false, file, diagText));
            } else {
                menu->addAction(i18n("Add Local Suppression"), this, std::bind(h, true, file, diagText));
            }
        } else if (item->type() == DiagnosticItem_File) {
            // track validity of raw pointer
            QPersistentModelIndex pindex(index);
            auto docDiagItem = static_cast<DocumentDiagnosticItem *>(item);
            auto h = [this, item, pindex](bool enabled) {
                if (!pindex.isValid()) {
                    return;
                }
                auto docDiagItem = static_cast<DocumentDiagnosticItem *>(item);
                docDiagItem->enabled = enabled;
                updateDiagnosticsState(docDiagItem);
            };
            if (docDiagItem->enabled) {
                menu->addAction(i18n("Disable Suppression"), this, std::bind(h, false));
            } else {
                menu->addAction(i18n("Enable Suppression"), this, std::bind(h, true));
            }
        }
    }
    menu->popup(m_diagnosticsTree->viewport()->mapToGlobal(pos));
}

void DiagnosticsView::onTextHint(KTextEditor::View *view, const KTextEditor::Cursor &position, bool manual) const
{
    QString result;
    auto document = view->document();

    QStandardItem *topItem = getItem(m_model, document->url());
    QStandardItem *targetItem = getItem(topItem, position, false);
    if (targetItem) {
        result = targetItem->text();
        // also include related info
        int count = targetItem->rowCount();
        for (int i = 0; i < count; ++i) {
            auto item = targetItem->child(i);
            result += QStringLiteral("\n");
            result += item->text();
        }
        // but let's not get carried away too far
        constexpr int maxsize = 1000;
        if (result.size() > maxsize) {
            result.resize(maxsize);
            result.append(QStringLiteral("..."));
        }
    }

    if (manual) {
        if (!result.isEmpty()) {
            m_textHintProvider->showTextHint(result, TextHintMarkupKind::PlainText, position);
        }
    } else {
        m_textHintProvider->textHintAvailable(result, TextHintMarkupKind::PlainText, position);
    }
}

void DiagnosticsView::onDocumentUrlChanged()
{
    // remove lingering diagnostics
    // collect active urls
    std::byte buffer[128 * 1000];
    std::pmr::monotonic_buffer_resource memory(buffer, sizeof(buffer));
    std::pmr::unordered_set<QUrl, QUrlHash> fpaths(&memory);

    const auto views = m_mainWindow->views();
    for (const auto view : views) {
        if (auto doc = view->document()) {
            fpaths.insert(doc->url());
        }
    }
    clearDiagnosticsForStaleDocs(fpaths, nullptr);
}

void DiagnosticsView::moveDiagnosticsSelection(bool forward)
{
    // If there's no items we just return
    if (m_diagnosticsTree->model()->rowCount() == 0) {
        return;
    }

    auto model = m_diagnosticsTree->model();
    auto index = m_diagnosticsTree->currentIndex();

    // Nothing is selected, select first visible item
    if (!index.isValid()) {
        index = model->index(0, 0);
    }

    auto isDiagItem = [](const QModelIndex &index) {
        // If parent is valid and parent of parent is invalid,
        // we are a level 2 item which means we are diagnostic item
        return index.parent().isValid() && !index.parent().parent().isValid();
    };

    auto getRow = [forward](const QModelIndex &index) {
        return forward ? 0 : index.model()->rowCount(index) - 1;
    };

    if (isDiagItem(index)) {
        auto next = forward ? m_diagnosticsTree->indexBelow(index) : m_diagnosticsTree->indexAbove(index);
        if (next.isValid() && isDiagItem(next)) {
            goToItemLocation(next);
        } else {
            // Next is not a diagnostic, are we at the end of current file's diagnostics?
            // If so, then jump to the next file
            auto parent = index.parent();
            auto nextFile = parent.siblingAtRow(parent.row() + (forward ? 1 : -1));
            if (nextFile.isValid()) {
                // Iterate and find valid first child and jump to that
                goToItemLocation(model->index(getRow(nextFile), 0, nextFile));
            }
            // If file is not valid, we are in the end of the list
        }
    } else {
        // Current is not a diagnostic item
        if (!index.parent().isValid()) {
            // Current is a file item, select it's first child
            goToItemLocation(model->index(0, 0, index));
        } else {
            // We are likely in third subitem, so we need to go back up
            goToItemLocation(model->index(getRow(index.parent()), 0, index));
        }
    }
}

void DiagnosticsView::nextItem()
{
    moveDiagnosticsSelection(true);
}

void DiagnosticsView::previousItem()
{
    moveDiagnosticsSelection(false);
}

void DiagTabOverlay::paintEvent(QPaintEvent *)
{
    if (m_active) {
        QPainter p(this);
        p.setOpacity(0.25);
        p.setBrush(KColorScheme().foreground(KColorScheme::NeutralText));
        p.setPen(Qt::NoPen);
        p.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

#include "moc_diagnosticview.cpp"
