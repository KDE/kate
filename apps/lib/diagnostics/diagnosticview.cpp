/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#include "diagnosticview.h"

#include "diagnosticitem.h"
#include "drawing_utils.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "session_diagnostic_suppression.h"
#include "texthint/KateTextHintManager.h"

#include <KActionCollection>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KTextEditor/MarkInterface>
#include <KTextEditor/MovingInterface>
#include <KTextEditor/TextHintInterface>
#endif

#include <KColorScheme>
#include <KXMLGUIFactory>
#include <QClipboard>
#include <QComboBox>
#include <QDebug>
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
        return m_providers.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (index.row() >= m_providers.size()) {
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

    void update(const QVector<DiagnosticsProvider *> &providerList)
    {
        beginResetModel();
        m_providers.clear();
        m_providers.push_back(nullptr);
        m_providers.append(providerList);
        endResetModel();
    }

    DiagnosticsView *m_diagView;
    QVector<DiagnosticsProvider *> m_providers;
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
        activeProvider = provider;
        invalidateFilter();
    }

    void setActiveSeverity(DiagnosticSeverity s)
    {
        severity = s;
        invalidateFilter();
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
            } else if (item && item->type() == DiagnosticItem_Fix) {
                if (item->parent() && item->parent()->type() == DiagnosticItem_Diag) {
                    auto castedItem = static_cast<DiagnosticItem *>(item);
                    ret = castedItem->m_diagnostic.severity == severity;
                }
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

class DiagTabOverlay : public QWidget
{
public:
    DiagTabOverlay(QWidget *parent)
        : QWidget(parent)
        , m_tabButton(parent)
    {
        if (!parent) {
            hide();
            return;
        }
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setGeometry(parent->geometry());
        show();
        raise();
    }

    void setActive(bool a)
    {
        if (m_tabButton && (m_active != a)) {
            m_active = a;
            if (m_tabButton->size() != size()) {
                resize(m_tabButton->size());
            }
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (m_active) {
            QPainter p(this);
            p.setOpacity(0.25);
            p.setBrush(KColorScheme().foreground(KColorScheme::NeutralText));
            p.setPen(Qt::NoPen);
            p.drawRect(rect().adjusted(1, 1, -1, -1));
        }
    }

private:
    bool m_active = false;
    QWidget *m_tabButton = nullptr;
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
static constexpr KTextEditor::Document::MarkTypes markTypeDiagError = KTextEditor::Document::Error;
static constexpr KTextEditor::Document::MarkTypes markTypeDiagWarning = KTextEditor::Document::Warning;
static constexpr KTextEditor::Document::MarkTypes markTypeDiagOther = KTextEditor::Document::markType30;
static constexpr KTextEditor::Document::MarkTypes markTypeDiagAll =
    KTextEditor::Document::MarkTypes(markTypeDiagError | markTypeDiagWarning | markTypeDiagOther);
#else
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagError = KTextEditor::MarkInterface::Error;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagWarning = KTextEditor::MarkInterface::Warning;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagOther = KTextEditor::MarkInterface::markType30;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagAll =
    KTextEditor::MarkInterface::MarkTypes(markTypeDiagError | markTypeDiagWarning | markTypeDiagOther);
#endif

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

        QVector<QTextLayout::FormatRange> formats;
        int lastSlash = text.lastIndexOf(QLatin1Char('/'));
        if (lastSlash != -1) {
            QTextCharFormat fmt;
            fmt.setFontWeight(QFont::Bold);
            formats.append({lastSlash + 1, int(text.length() - (lastSlash + 1)), fmt});
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

DiagnosticsView::DiagnosticsView(QWidget *parent, KTextEditor::MainWindow *mainWindow, QWidget *tabButton)
    : QWidget(parent)
    , KXMLGUIClient()
    , m_mainWindow(mainWindow)
    , m_diagnosticsTree(new QTreeView(this))
    , m_clearButton(new QToolButton(this))
    , m_filterLineEdit(new QLineEdit(this))
    , m_providerCombo(new QComboBox(this))
    , m_errFilterBtn(new QToolButton(this))
    , m_warnFilterBtn(new QToolButton(this))
    , m_proxy(new DiagnosticsProxyModel(this))
    , m_sessionDiagnosticSuppressions(std::make_unique<SessionDiagnosticSuppressions>())
    , m_tabButtonOverlay(new DiagTabOverlay(tabButton))
    , m_posChangedTimer(new QTimer(this))
    , m_filterChangedTimer(new QTimer(this))
    , m_textHintProvider(new KateTextHintProvider(mainWindow, this))
{
    auto l = new QVBoxLayout(this);
    l->setContentsMargins({});
    setupDiagnosticViewToolbar(l);
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
    auto *a = ac->addAction(QStringLiteral("kate_quick_fix"), this, &DiagnosticsView::quickFix);
    a->setText(i18n("Quick Fix"));
    ac->setDefaultShortcut(a, QKeySequence((Qt::CTRL | Qt::Key_Period)));

    a = ac->addAction(QStringLiteral("goto_next_diagnostic"), this, &DiagnosticsView::nextItem);
    a->setText(i18n("Next Item"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    ac->setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_Right);

    a = ac->addAction(QStringLiteral("goto_prev_diagnostic"), this, &DiagnosticsView::previousItem);
    a->setText(i18n("Previous Item"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    ac->setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_Left);

    a = ac->addAction(QStringLiteral("diagnostics_clear_filter"), this, [this]() {
        DiagnosticsView::filterViewTo(nullptr);
    });
    a->setText(i18n("Clear Diagnostics Filter"));
    ac->setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_C);

    m_posChangedTimer->setInterval(500);
    m_posChangedTimer->setSingleShot(true);
    m_posChangedTimer->callOnTimeout(this, [this] {
        auto v = m_mainWindow->activeView();
        if (v) {
            if (auto doc = v->document()) {
                syncDiagnostics(doc, v->cursorPosition().line(), true, false);
            }
        }
    });

    connect(m_textHintProvider.get(), &KateTextHintProvider::textHintRequested, this, &DiagnosticsView::onTextHint);
    connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &DiagnosticsView::handleEsc);
    mainWindow->guiFactory()->addClient(this);
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
        dv = new DiagnosticsView(tv, mainWindow, Utils::tabForToolView(tv, mainWindow));
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

    m_mainWindow->guiFactory()->removeClient(this);
}

void DiagnosticsView::setupDiagnosticViewToolbar(QVBoxLayout *mainLayout)
{
    mainLayout->setSpacing(2);
    auto l = new QHBoxLayout();
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
    m_filterLineEdit->setPlaceholderText(i18n("Filter..."));
    m_filterLineEdit->setClearButtonEnabled(true);
    connect(m_filterLineEdit, &QLineEdit::textChanged, m_filterChangedTimer, [this] {
        m_filterChangedTimer->start();
    });

    m_clearButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-all")));
    connect(m_clearButton, &QToolButton::clicked, this, [this] {
        std::vector<KTextEditor::Document *> docs(m_diagnosticsMarks.begin(), m_diagnosticsMarks.end());
        for (auto d : docs) {
            clearAllMarks(d);
        }
        m_model.clear();
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
        connect(v->document(), &KTextEditor::Document::documentUrlChanged, this, &DiagnosticsView::onDocumentUrlChanged, Qt::UniqueConnection);
    }
}

void DiagnosticsView::registerDiagnosticsProvider(DiagnosticsProvider *provider)
{
    if (m_providers.contains(provider)) {
        qWarning() << provider << " already registred, ignoring!";
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
    if (!m_providers.contains(provider)) {
        return;
    }
    disconnect(provider, &DiagnosticsProvider::diagnosticsAdded, this, &DiagnosticsView::onDiagnosticsAdded);
    disconnect(provider, &DiagnosticsProvider::fixesAvailable, this, &DiagnosticsView::onFixesAvailable);
    disconnect(provider, &DiagnosticsProvider::requestClearDiagnostics, this, &DiagnosticsView::clearDiagnosticsFromProvider);
    disconnect(provider, &DiagnosticsProvider::requestClearSuppressions, this, &DiagnosticsView::clearSuppressionsFromProvider);
    m_providers.removeOne(provider);

    m_providerModel->update(m_providers);

    // clear diagnostics
    clearDiagnosticsFromProvider(provider);
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
    m_tabButtonOverlay->setActive(false);
    QWidget::showEvent(e);
}

void DiagnosticsView::handleEsc(QEvent *event)
{
    if (event->type() != QEvent::ShortcutOverride)
        return;

    auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent && keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier) {
        if (parentWidget() && parentWidget()->isVisible()) {
            m_mainWindow->hideToolView(parentWidget());
            event->accept();
        }
    }
}

static DocumentDiagnosticItem *getItem(const QStandardItemModel &model, const QUrl &url)
{
    // local file in custom role, Qt::DisplayRole might have additional elements
    auto l = model.match(model.index(0, 0, QModelIndex()), Qt::UserRole, url.toString(QUrl::PreferLocalFile | QUrl::RemovePassword), 1, Qt::MatchExactly);
    if (l.length()) {
        return static_cast<DocumentDiagnosticItem *>(model.itemFromIndex(l.at(0)));
    }
    return nullptr;
}

static QStandardItem *getItem(const QStandardItem *topItem, KTextEditor::Cursor pos, bool onlyLine)
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
                int middle = first + (last - first) / 2;
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

void DiagnosticsView::onFixesAvailable(const QVector<DiagnosticFix> &fixes, const QVariant &data)
{
    if (fixes.empty() || data.isNull()) {
        return;
    }
    const auto diagModelIdx = data.value<DiagModelIndex>();
    if (diagModelIdx.parentRow == -1) {
        qWarning() << "Unexpected -1 parentRow";
        return;
    }
    const auto parentIdx = m_model.index(diagModelIdx.parentRow, 0);
    const auto idx = m_model.index(diagModelIdx.row, 0, parentIdx);
    if (!idx.isValid()) {
        qWarning() << Q_FUNC_INFO << "Unexpected invalid idx";
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

void DiagnosticsView::showFixesInMenu(const QVector<DiagnosticFix> &fixes)
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
            QVector<DiagnosticFix> fixes;
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
        qWarning() << "invalid item clicked";
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
        topItem->setData(prettyUri, Qt::UserRole);
    } else {
        // try to retain current position
        auto currentIndex = m_proxy->mapToSource(m_diagnosticsTree->currentIndex());
        if (currentIndex.parent() == topItem->index()) {
            row = currentIndex.row();
        }

        // Remove old diagnostics of this provider
        if (!provider->m_persistentDiagnostics) {
            topItem->removeItemsForProvider(provider);
        }
    }
    topItem->addProvider(provider);

    for (const auto &diag : diagnostics.diagnostics) {
        auto item = new DiagnosticItem(diag);
        topItem->appendRow(item);
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
        item->setText(source + (lines.size() > 0 ? lines[0] : QString()));
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
        m_diagnosticsTree->setExpanded(toProxyIndex(item->index()), true);
    }

    // TODO perhaps add some custom delegate that only shows 1 line
    // and only the whole text when item selected ??
    m_diagnosticsTree->setExpanded(toProxyIndex(topItem->index()), true);

    // topItem might be removed after this
    updateDiagnosticsState(topItem);
    // also sync updated diagnostic to current position
    auto currentView = m_mainWindow->activeView();
    if (topItem && currentView && currentView->document()) {
        if (!syncDiagnostics(currentView->document(), currentView->cursorPosition().line(), false, false)) {
            // avoid jitter; only restore previous if applicable
            if (row >= 0 && row < topItem->rowCount()) {
                m_diagnosticsTree->scrollTo(toProxyIndex(topItem->child(row)->index()));
            }
        }
    }
    m_tabButtonOverlay->setActive(!isVisible() && m_model.rowCount() > 0);
}

static auto getProvider(QStandardItem *item)
{
    return item->data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>();
}

void DiagnosticsView::clearDiagnosticsForStaleDocs(const QVector<QString> &filesToKeep, DiagnosticsProvider *provider)
{
    auto all_diags_from_provider = [provider](DocumentDiagnosticItem *file) {
        if (file->rowCount() == 0) {
            return true;
        }
        if (provider && file->providers().size() == 1 && file->providers().contains(provider)) {
            return true;
        }
        if (!provider) {
            const QVector<DiagnosticsProvider *> &providers = file->providers();
            return std::all_of(providers.begin(), providers.end(), [](DiagnosticsProvider *p) {
                return !p->persistentDiagnostics();
            });
        }
        return false;
    };

    auto bulk_remove = [](QStandardItemModel *model, int &start, int &count, int &i) {
        if (start > -1 && count != 0) {
            model->removeRows(start, count);
            i = start - 1; // reset i
        }
        start = -1;
        count = 0;
    };

    int start = -1, count = 0;

    for (int i = 0; i < m_model.rowCount(); ++i) {
        auto fileItem = static_cast<DocumentDiagnosticItem *>(m_model.item(i));
        if (!filesToKeep.contains(fileItem->data(Qt::UserRole).toString())) {
            if (!all_diags_from_provider(fileItem)) {
                // If the diagnostics of this file item are from multiple providers
                // delete the ones from @p provider
                bulk_remove(&m_model, start, count, i);
                fileItem->removeItemsForProvider(provider);
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
        m_model.removeRows(start, count);
    }

    updateMarks();

    m_tabButtonOverlay->setActive(!isVisible() && m_model.rowCount() > 0);
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

    auto url = item->data(DiagnosticModelRole::FileUrlRole).toUrl();
    // document url could end up empty while in intermediate reload state
    // (and then it might match a parent item with no RangeData at all)
    if (url != doc->url() || url.isEmpty()) {
        return;
    }

    KTextEditor::Range range = item->data(DiagnosticModelRole::RangeRole).value<KTextEditor::Range>();
    if (!range.isValid()) {
        return;
    }

    KTextEditor::Attribute::Ptr attr;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    KTextEditor::Document::MarkTypes markType = markTypeDiagWarning;
#else
    KTextEditor::MarkInterface::MarkTypes markType = markTypeDiagWarning;
#endif

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
        qWarning() << "Unknown diagnostic severity";
        return;
    }

    if (!attr) {
        qWarning() << "Unexpected null attr";
    }

    // highlight the range
    if (attr && !range.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        KTextEditor::MovingRange *mr = doc->newMovingRange(range);
#else
        KTextEditor::MovingInterface *miface = qobject_cast<KTextEditor::MovingInterface *>(doc);
        Q_ASSERT(miface);
        KTextEditor::MovingRange *mr = miface->newMovingRange(range);
#endif
        mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
        mr->setAttribute(attr);
        mr->setAttributeOnlyForViews(true);
        m_diagnosticsRanges[doc].push_back(mr);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    auto iface = doc;
#else
    KTextEditor::MarkInterfaceV2 *iface = qobject_cast<KTextEditor::MarkInterfaceV2 *>(doc);
    Q_ASSERT(iface);
#endif
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    using Doc = KTextEditor::Document;
    connect(doc, &Doc::aboutToInvalidateMovingInterfaceContent, this, &DiagnosticsView::clearAllMarks, Qt::UniqueConnection);
    connect(doc, &Doc::aboutToDeleteMovingInterfaceContent, this, &DiagnosticsView::clearAllMarks, Qt::UniqueConnection);
    // reload might save/restore marks before/after above signals, so let's clear before that
    connect(doc, &Doc::aboutToReload, this, &DiagnosticsView::clearAllMarks, Qt::UniqueConnection);
    connect(doc, &Doc::markClicked, this, &DiagnosticsView::onMarkClicked, Qt::UniqueConnection);
#else
    // clang-format off
    connect(doc,
            SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            this,
            SLOT(clearAllMarks(KTextEditor::Document*)),
            Qt::UniqueConnection);
    connect(doc,
            SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)),
            this,
            SLOT(clearAllMarks(KTextEditor::Document*)),
            Qt::UniqueConnection);
    // reload might save/restore marks before/after above signals, so let's clear before that
    connect(doc, &KTextEditor::Document::aboutToReload, this, &DiagnosticsView::clearAllMarks, Qt::UniqueConnection);

    connect(doc,
            SIGNAL(markClicked(KTextEditor::Document*,KTextEditor::Mark,bool&)),
            this,
            SLOT(onMarkClicked(KTextEditor::Document*,KTextEditor::Mark,bool&)),
            Qt::UniqueConnection);
    // clang-format on
#endif
}

void DiagnosticsView::addMarksRec(KTextEditor::Document *doc, QStandardItem *item)
{
    Q_ASSERT(item);
    // We only care about @p doc items
    if (item->type() == DiagnosticItem_File && QUrl::fromLocalFile(item->data(Qt::UserRole).toString()) != doc->url()) {
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_diagnosticsMarks.contains(doc)) {
        const QHash<int, KTextEditor::Mark *> marks = doc->marks();
        for (auto mark : marks) {
            if (mark->type & markTypeDiagAll) {
                doc->removeMark(mark->line, markTypeDiagAll);
            }
        }
        m_diagnosticsMarks.remove(doc);
    }
#else
    KTextEditor::MarkInterface *iface = m_diagnosticsMarks.contains(doc) ? qobject_cast<KTextEditor::MarkInterface *>(doc) : nullptr;
    if (iface) {
        const QHash<int, KTextEditor::Mark *> marks = iface->marks();
        for (auto mark : marks) {
            if (mark->type & markTypeDiagAll) {
                iface->removeMark(mark->line, markTypeDiagAll);
            }
        }
        m_diagnosticsMarks.remove(doc);
    }
#endif

    auto it = m_diagnosticsRanges.find(doc);
    if (it != m_diagnosticsRanges.end()) {
        for (auto range : it.value()) {
            delete range;
        }
        it = m_diagnosticsRanges.erase(it);
    }
}

void DiagnosticsView::updateMarks(const QList<QUrl> &urls)
{
    std::vector<KTextEditor::Document *> docs;
    if (!urls.isEmpty()) {
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

    const int totalCount = topItem->rowCount();
    int count = 0;
    for (int i = 0; i < totalCount; ++i) {
        auto item = topItem->child(i);
        auto hide = suppressions && item && suppressions->match(*item);
        // mark accordingly as flag and (un)hide
        auto flags = item->flags();
        const auto ENABLED = Qt::ItemFlag::ItemIsEnabled;
        if ((flags & ENABLED) != !hide) {
            flags = hide ? (flags & ~ENABLED) : (flags | ENABLED);
            item->setFlags(flags);
            const auto proxyIdx = m_proxy->mapFromSource(item->index());
            m_diagnosticsTree->setRowHidden(proxyIdx.row(), proxyIdx.parent(), hide);
        }
        count += hide ? 0 : 1;
    }
    // adjust file item level text
    auto suppressed = totalCount - count;
    const QString path = topItem->data(Qt::UserRole).toString();
    topItem->setText(suppressed ? i18nc("@info", "%1 [suppressed: %2]", path, suppressed) : path);
    // only hide if really nothing below
    const auto proxyIdx = m_proxy->mapFromSource(topItem->index());
    m_diagnosticsTree->setRowHidden(proxyIdx.row(), proxyIdx.parent(), totalCount == 0);
    if (topItem->rowCount() == 0) {
        m_model.removeRow(topItem->row());
        topItem = nullptr;
    }

    updateMarks({QUrl::fromLocalFile(path)});
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

    int line = start.start().line();
    int column = start.start().column();
    KTextEditor::View *activeView = m_mainWindow->activeView();
    if (line < 0 || column < 0) {
        return;
    }

    KTextEditor::Document *document = activeView ? activeView->document() : nullptr;
    KTextEditor::Cursor cdef(line, column);

    KTextEditor::View *targetView = nullptr;
    if (document && url == document->url()) {
        targetView = activeView;
    } else {
        targetView = m_mainWindow->openUrl(url);
    }
    if (targetView) {
        // save current position for location history
        if (activeView) {
            Utils::addPositionToHistory(activeView->document()->url(), activeView->cursorPosition(), m_mainWindow);
        }
        // save the position to which we are jumping in location history
        Utils::addPositionToHistory(targetView->document()->url(), cdef, m_mainWindow);
        targetView->setCursorPosition(cdef);
    }
}

void DiagnosticsView::onMarkClicked(KTextEditor::Document *document, KTextEditor::Mark mark, bool &handled)
{
    // no action if no mark was sprinkled here
    if (m_diagnosticsMarks.contains(document) && syncDiagnostics(document, mark.line, false, true)) {
        handled = true;
    }
}

void DiagnosticsView::showToolview(DiagnosticsProvider *filterTo)
{
    m_mainWindow->showToolView(parentWidget());
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

bool DiagnosticsView::syncDiagnostics(KTextEditor::Document *document, int line, bool allowTop, bool doShow)
{
    auto hint = QAbstractItemView::PositionAtTop;
    DocumentDiagnosticItem *topItem = getItem(m_model, document->url());
    updateDiagnosticsSuppression(topItem, document);
    QStandardItem *targetItem = getItem(topItem, {line, 0}, true);
    if (targetItem) {
        hint = QAbstractItemView::PositionAtCenter;
    }
    if (!targetItem && allowTop) {
        targetItem = topItem;
    }
    if (targetItem) {
        m_diagnosticsTree->blockSignals(true);
        const auto idx = m_proxy->mapFromSource(targetItem->index());
        m_diagnosticsTree->scrollTo(idx, hint);
        m_diagnosticsTree->setCurrentIndex(idx);
        m_diagnosticsTree->blockSignals(false);
        if (doShow) {
            m_mainWindow->showToolView(parentWidget());
        }
    }
    return targetItem != nullptr;
}

void DiagnosticsView::updateDiagnosticsSuppression(DocumentDiagnosticItem *diagTopItem, KTextEditor::Document *doc, bool force)
{
    if (!diagTopItem || !doc) {
        return;
    }

    auto &suppressions = diagTopItem->diagnosticSuppression;
    if (!suppressions || force) {
        QVector<QJsonObject> providerSupressions;
        const QVector<DiagnosticsProvider *> &providers = diagTopItem->providers();
        for (auto p : providers) {
            auto suppressions = p->suppressions(doc);
            if (!suppressions.isEmpty()) {
                providerSupressions.push_back(suppressions);
            }
        }
        const auto sessionSuppressions = m_sessionDiagnosticSuppressions->getSuppressions(doc->url().toLocalFile());
        if (!providerSupressions.isEmpty() || !sessionSuppressions.isEmpty()) {
            auto supp = new DiagnosticSuppression(doc, providerSupressions, sessionSuppressions);
            suppressions.reset(supp);
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
    QStandardItem *item = m_model.itemFromIndex(index);
    if (item->type() == DiagnosticItem_Diag) {
        auto diagText = index.data().toString();
        menu->addAction(QIcon::fromTheme(QLatin1String("edit-copy")), i18n("Copy to Clipboard"), [diagText]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(diagText);
        });
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
            updateDiagnosticsSuppression(docDiagItem, docDiagItem->diagnosticSuppression->document(), true);
        };
        using namespace std::placeholders;
        const auto empty = QString();
        if (m_sessionDiagnosticSuppressions->hasSuppression(empty, diagText)) {
            menu->addAction(i18n("Remove Global Suppression"), this, std::bind(h, false, empty, diagText));
        } else {
            menu->addAction(i18n("Add Global Suppression"), this, std::bind(h, true, empty, diagText));
        }
        auto file = parent.data(Qt::UserRole).toString();
        if (m_sessionDiagnosticSuppressions->hasSuppression(file, diagText)) {
            menu->addAction(i18n("Remove Local Suppression"), this, std::bind(h, false, file, diagText));
        } else {
            menu->addAction(i18n("Add Local Suppression"), this, std::bind(h, true, file, diagText));
        }
    } else if (item->type() == DiagnosticItem_File) {
        // track validity of raw pointer
        QPersistentModelIndex pindex(index);
        auto docDiagItem = static_cast<DocumentDiagnosticItem *>(item);
        auto h = [this, &docDiagItem, pindex](bool enabled) {
            if (pindex.isValid()) {
                docDiagItem->enabled = enabled;
            }
            updateDiagnosticsState(docDiagItem);
        };
        if (docDiagItem->enabled) {
            menu->addAction(i18n("Disable Suppression"), this, std::bind(h, false));
        } else {
            menu->addAction(i18n("Enable Suppression"), this, std::bind(h, true));
        }
    }
    menu->popup(m_diagnosticsTree->viewport()->mapToGlobal(pos));
}

void DiagnosticsView::onTextHint(KTextEditor::View *view, const KTextEditor::Cursor &position) const
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
            result += QStringLiteral("  \n"); // markdown 2 spaces = newline
            result += item->text();
        }
        // but let's not get carried away too far
        constexpr int maxsize = 1000;
        if (result.size() > maxsize) {
            result.resize(maxsize);
            result.append(QStringLiteral("..."));
        }
    }

    m_textHintProvider->textHintAvailable(result.toHtmlEscaped(), TextHintMarkupKind::MarkDown, position);
}

void DiagnosticsView::onDocumentUrlChanged()
{
    // remove lingering diagnostics
    // collect active urls
    QSet<QString> fpaths;
    const auto views = m_mainWindow->views();
    for (const auto view : views) {
        if (auto doc = view->document()) {
            fpaths.insert(doc->url().toLocalFile());
        }
    }
    clearDiagnosticsForStaleDocs({fpaths.begin(), fpaths.end()}, nullptr);
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

#include "moc_diagnosticview.cpp"
