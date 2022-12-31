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

#include <KActionCollection>
#include <KTextEditor/Editor>
#include <KTextEditor/MarkInterface>
#include <KTextEditor/MovingInterface>
#include <KTextEditor/TextHintInterface>

#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QTimer>
#include <QVBoxLayout>

bool DiagnosticsProvider::hasTooltipForPos(KTextEditor::View *v, KTextEditor::Cursor pos) const
{
    if (!diagnosticView->onTextHint(v, pos).isEmpty())
        return true;
    return false;
}

static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagError = KTextEditor::MarkInterface::Error;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagWarning = KTextEditor::MarkInterface::Warning;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagOther = KTextEditor::MarkInterface::markType30;
static constexpr KTextEditor::MarkInterface::MarkTypes markTypeDiagAll =
    KTextEditor::MarkInterface::MarkTypes(markTypeDiagError | markTypeDiagWarning | markTypeDiagOther);

struct DiagModelIndex {
    int row;
    int parentRow;
    bool autoApply;
};
Q_DECLARE_METATYPE(DiagModelIndex)

class LocationTreeDelegate : public QStyledItemDelegate
{
public:
    LocationTreeDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
        , m_monoFont(Utils::editorFont())
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto options = option;
        initStyleOption(&options, index);

        painter->save();

        QString text = index.data().toString();
        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        QVector<QTextLayout::FormatRange> formats;
        if (!index.parent().isValid()) {
            int lastSlash = text.lastIndexOf(QLatin1Char('/'));
            if (lastSlash != -1) {
                QTextCharFormat fmt;
                fmt.setFontWeight(QFont::Bold);
                formats.append({lastSlash + 1, int(text.length() - (lastSlash + 1)), fmt});
            }
        }

        /** There might be an icon? Make sure to not draw over it **/
        auto textRectX = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options, options.widget).x();
        auto width = textRectX - options.rect.x();
        painter->translate(width, 0);
        Utils::paintItemViewText(painter, text, options, formats);

        painter->restore();
    }

private:
    QFont m_monoFont;
};

class ForwardingTextHintProvider : public KTextEditor::TextHintProvider
{
    DiagnosticsView *m_parent;
    QPointer<KTextEditor::View> m_view;

public:
    ForwardingTextHintProvider(DiagnosticsView *parent)
        : m_parent(parent)
    {
        Q_ASSERT(m_parent);
    }

    void setView(KTextEditor::View *v)
    {
        if (m_view) {
            auto iface = qobject_cast<KTextEditor::TextHintInterface *>(m_view);
            iface->unregisterTextHintProvider(this);
        }
        if (v) {
            m_view = v;
            auto iface = qobject_cast<KTextEditor::TextHintInterface *>(m_view);
            iface->registerTextHintProvider(this);
        }
    }

    virtual QString textHint(KTextEditor::View *view, const KTextEditor::Cursor &position) override
    {
        return m_parent->onTextHint(view, position);
    }
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

DiagnosticsView::DiagnosticsView(QWidget *parent, KateMainWindow *mainWindow)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_diagnosticsTree(new QTreeView(this))
    , m_sessionDiagnosticSuppressions(std::make_unique<SessionDiagnosticSuppressions>())
    , m_textHintProvider(new ForwardingTextHintProvider(this))
    , m_posChangedTimer(new QTimer(this))
{
    auto l = new QVBoxLayout(this);
    l->setContentsMargins({});
    l->addWidget(m_diagnosticsTree);

    m_model.setColumnCount(1);

    m_diagnosticsTree->setHeaderHidden(true);
    m_diagnosticsTree->setFocusPolicy(Qt::NoFocus);
    m_diagnosticsTree->setLayoutDirection(Qt::LeftToRight);
    m_diagnosticsTree->setSortingEnabled(false);
    m_diagnosticsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_diagnosticsTree->setUniformRowHeights(true);
    m_diagnosticsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_diagnosticsTree->setItemDelegate(new LocationTreeDelegate(this));

    m_diagnosticsTree->setModel(&m_model);

    connect(m_mainWindow->wrapper(), &KTextEditor::MainWindow::viewChanged, this, &DiagnosticsView::onViewChanged);

    connect(m_diagnosticsTree, &QTreeView::customContextMenuRequested, this, &DiagnosticsView::onContextMenuRequested);
    connect(m_diagnosticsTree, &QTreeView::clicked, this, &DiagnosticsView::goToItemLocation);
    connect(m_diagnosticsTree, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        onDoubleClicked(index);
    });

    auto *ac = m_mainWindow->actionCollection();
    auto *a = ac->addAction(QStringLiteral("kate_quick_fix"), this, &DiagnosticsView::quickFix);
    a->setText(i18n("Quick Fix"));
    ac->setDefaultShortcut(a, QKeySequence((Qt::CTRL | Qt::Key_Period)));

    m_posChangedTimer->setInterval(500);
    m_posChangedTimer->setSingleShot(true);
    m_posChangedTimer->callOnTimeout(this, [this] {
        auto v = m_mainWindow->activeView();
        if (auto doc = v->document()) {
            syncDiagnostics(doc, v->cursorPosition().line(), true, false);
        }
    });
}

DiagnosticsView::~DiagnosticsView()
{
    m_textHintProvider->setView(nullptr);
}

void DiagnosticsView::onViewChanged(KTextEditor::View *v)
{
    m_textHintProvider->setView(v);

    disconnect(posChangedConnection);
    m_posChangedTimer->stop();
    if (v) {
        posChangedConnection = connect(v, &KTextEditor::View::cursorPositionChanged, this, [this] {
            m_posChangedTimer->start();
        });
        m_posChangedTimer->start();
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
    connect(provider, &DiagnosticsProvider::requestClearDiagnosticsForStaleDocs, this, &DiagnosticsView::clearDiagnosticsForStaleDocs);
    m_providers.push_back(provider);
}

void DiagnosticsView::unregisterDiagnosticsProvider(DiagnosticsProvider *provider)
{
    if (!m_providers.contains(provider)) {
        qWarning() << provider << "not registred but trying to unregister!";
        return;
    }
    disconnect(provider, &DiagnosticsProvider::diagnosticsAdded, this, &DiagnosticsView::onDiagnosticsAdded);
    disconnect(provider, &DiagnosticsProvider::fixesAvailable, this, &DiagnosticsView::onFixesAvailable);
    disconnect(provider, &DiagnosticsProvider::requestClearDiagnosticsForStaleDocs, this, &DiagnosticsView::clearDiagnosticsForStaleDocs);
    m_providers.removeOne(provider);
}

void DiagnosticsView::readSessionConfig(const KConfigGroup &config)
{
    m_sessionDiagnosticSuppressions->readSessionConfig(config);
}

void DiagnosticsView::writeSessionConfig(KConfigGroup &config)
{
    m_sessionDiagnosticSuppressions->writeSessionConfig(config);
}

static QStandardItem *getItem(const QStandardItemModel &model, const QUrl &url)
{
    // local file in custom role, Qt::DisplayRole might have additional elements
    auto l = model.match(model.index(0, 0, QModelIndex()), Qt::UserRole, url.toLocalFile(), 1, Qt::MatchExactly);
    if (l.length()) {
        return model.itemFromIndex(l.at(0));
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

    if (targetItem) {
        if (targetItem->hasChildren()) {
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
        return;
    }

    if (itemFromIndex->type() == DiagnosticItem_Diag) {
        if (itemFromIndex->hasChildren()) {
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
        provider->requestFixes(item->data(DiagnosticModelRole::FileUrlRole).toUrl(), item->m_diagnostic, data);
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
    QStandardItem *topItem = getItem(m_model, diagnostics.uri);

    // current diagnostics row, if one of incoming diagnostics' document
    int row = -1;
    if (!topItem) {
        // no need to create an empty one
        if (diagnostics.diagnostics.empty()) {
            return;
        }
        topItem = new DocumentDiagnosticItem();
        model->appendRow(topItem);
        topItem->setText(diagnostics.uri.toLocalFile());
        topItem->setData(diagnostics.uri.toLocalFile(), Qt::UserRole);
        topItem->setData(QVariant::fromValue(provider), DiagnosticModelRole::ProviderRole);
    } else {
        // try to retain current position
        auto currentIndex = m_diagnosticsTree->currentIndex();
        if (currentIndex.parent() == topItem->index()) {
            row = currentIndex.row();
        }

        // Remove diagnostics of this provider
        for (int i = 0; i < topItem->rowCount(); ++i) {
            auto item = topItem->child(i);
            if (item && item->data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>() == provider) {
                topItem->removeRow(i);
                i--;
            }
        }
    }

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
            auto location = QStringLiteral("%1:%2").arg(basename).arg(related.location.range.start().line());
            relatedItemMessage->setText(QStringLiteral("[%1] %2").arg(location).arg(related.message));
            relatedItemMessage->setData(diagnosticsIcon(DiagnosticSeverity::Information), Qt::DecorationRole);
            item->appendRow(relatedItemMessage);
        }
        m_diagnosticsTree->setExpanded(item->index(), true);
    }

    // TODO perhaps add some custom delegate that only shows 1 line
    // and only the whole text when item selected ??
    m_diagnosticsTree->setExpanded(topItem->index(), true);

    updateDiagnosticsState(topItem);
    // also sync updated diagnostic to current position
    auto currentView = m_mainWindow->activeView();
    if (currentView && currentView->document()) {
        if (!syncDiagnostics(currentView->document(), currentView->cursorPosition().line(), false, false)) {
            // avoid jitter; only restore previous if applicable
            if (row >= 0 && row < topItem->rowCount()) {
                m_diagnosticsTree->scrollTo(topItem->child(row)->index());
            }
        }
    }
}

void DiagnosticsView::clearDiagnosticsForStaleDocs(const QVector<QString> &filesToKeep, DiagnosticsProvider *provider)
{
    auto get_provider = [](QStandardItem *item) {
        return item->data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>();
    };
    auto all_diags_from_provider = [get_provider, provider](QStandardItem *file) {
        if (file->rowCount() == 0) {
            return true;
        }
        for (int i = 0; i < file->rowCount(); ++i) {
            auto diagItem = file->child(i);
            auto p = get_provider(diagItem);
            if (provider != p) {
                return false;
            }
        }
        return true;
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
        auto fileItem = m_model.item(i);
        if (!filesToKeep.contains(fileItem->data(Qt::UserRole).toString())) {
            if (!all_diags_from_provider(fileItem)) {
                // If the diagnostics of this file item are from multiple providers
                // delete the ones from @p provider
                bulk_remove(&m_model, start, count, i);
                for (int r = 0; r < fileItem->rowCount(); ++r) {
                    auto item = fileItem->child(r);
                    if (item && get_provider(item) == provider) {
                        fileItem->removeRow(r);
                        r--;
                    }
                }
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
}

void DiagnosticsView::addMarks(KTextEditor::Document *doc, QStandardItem *item, RangeCollection *ranges, DocumentCollection *docs)
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
    const auto line = range.start().line();
    const auto kind = item->data(DiagnosticModelRole::KindRole).value<DiagnosticSeverity>();

    KTextEditor::Attribute::Ptr attr;

    KTextEditor::MarkInterface::MarkTypes markType = markTypeDiagWarning;
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
    if (ranges && attr && !range.isEmpty()) {
        KTextEditor::MovingInterface *miface = qobject_cast<KTextEditor::MovingInterface *>(doc);
        Q_ASSERT(miface);
        KTextEditor::MovingRange *mr = miface->newMovingRange(range);
        mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
        mr->setAttribute(attr);
        mr->setAttributeOnlyForViews(true);
        ranges->insert(doc, mr);
    }

    KTextEditor::MarkInterfaceV2 *iface = qobject_cast<KTextEditor::MarkInterfaceV2 *>(doc);
    Q_ASSERT(iface);
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

    if (docs) {
        iface->addMark(line, markType);
        docs->insert(doc);
    }

    // ensure runtime match
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
}

void DiagnosticsView::addMarksRec(KTextEditor::Document *doc, QStandardItem *item, RangeCollection *ranges, DocumentCollection *docs)
{
    // We only care about @p doc items
    auto docItem = dynamic_cast<DocumentDiagnosticItem *>(item);
    if (docItem && QUrl::fromLocalFile(docItem->data(Qt::UserRole).toString()) != doc->url()) {
        return;
    }

    Q_ASSERT(item);
    addMarks(doc, item, ranges, docs);
    for (int i = 0; i < item->rowCount(); ++i) {
        addMarksRec(doc, item->child(i), ranges, docs);
    }
}

void DiagnosticsView::addMarks(KTextEditor::Document *doc, QStandardItemModel *treeModel, RangeCollection &ranges, DocumentCollection &docs)
{
    // check if already added
    auto oranges = ranges.contains(doc) ? nullptr : &ranges;
    auto odocs = docs.contains(doc) ? nullptr : &docs;

    if (!oranges && !odocs) {
        return;
    }

    Q_ASSERT(treeModel);
    addMarksRec(doc, treeModel->invisibleRootItem(), oranges, odocs);
}

static void clearMarks(KTextEditor::Document *doc,
                       QMultiHash<KTextEditor::Document *, KTextEditor::MovingRange *> &ranges,
                       QSet<KTextEditor::Document *> &docs,
                       uint markType)
{
    KTextEditor::MarkInterface *iface = docs.contains(doc) ? qobject_cast<KTextEditor::MarkInterface *>(doc) : nullptr;
    if (iface) {
        const QHash<int, KTextEditor::Mark *> marks = iface->marks();
        QHashIterator<int, KTextEditor::Mark *> i(marks);
        while (i.hasNext()) {
            i.next();
            if (i.value()->type & markType) {
                iface->removeMark(i.value()->line, markType);
            }
        }
        docs.remove(doc);
    }

    for (auto it = ranges.find(doc); it != ranges.end() && it.key() == doc;) {
        delete it.value();
        it = ranges.erase(it);
    }
}

void DiagnosticsView::clearAllMarks(KTextEditor::Document *doc)
{
    clearMarks(doc, m_diagnosticsRanges, m_diagnosticsMarks, markTypeDiagAll);
}

void DiagnosticsView::updateMarks()
{
    KTextEditor::View *activeView = m_mainWindow->activeView();
    KTextEditor::Document *doc = activeView ? activeView->document() : nullptr;

    if (doc) {
        clearMarks(doc, m_diagnosticsRanges, m_diagnosticsMarks, markTypeDiagAll);
        addMarks(doc, &m_model, m_diagnosticsRanges, m_diagnosticsMarks);
    }
}

void DiagnosticsView::updateDiagnosticsState(QStandardItem *topItem)
{
    if (!topItem) {
        return;
    }

    auto diagTopItem = static_cast<DocumentDiagnosticItem *>(topItem);
    auto enabled = diagTopItem->enabled;
    auto suppressions = enabled ? diagTopItem->diagnosticSuppression.get() : nullptr;

    int totalCount = topItem->rowCount();
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
            m_diagnosticsTree->setRowHidden(item->row(), topItem->index(), hide);
        }
        count += hide ? 0 : 1;
    }
    // adjust file item level text
    auto suppressed = totalCount - count;
    auto text = topItem->data(Qt::UserRole).toString();
    topItem->setText(suppressed ? i18nc("@info", "%1 [suppressed: %2]", text, suppressed) : text);
    // only hide if really nothing below
    m_diagnosticsTree->setRowHidden(topItem->row(), QModelIndex(), totalCount == 0);

    updateMarks();
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
            m_mainWindow->viewManager()->addPositionToHistory(activeView->document()->url(), activeView->cursorPosition());
        }
        // save the position to which we are jumping in location history
        m_mainWindow->viewManager()->addPositionToHistory(targetView->document()->url(), cdef);
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

bool DiagnosticsView::syncDiagnostics(KTextEditor::Document *document, int line, bool allowTop, bool doShow)
{
    auto hint = QAbstractItemView::PositionAtTop;
    QStandardItem *topItem = getItem(m_model, document->url());
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
        m_diagnosticsTree->scrollTo(targetItem->index(), hint);
        m_diagnosticsTree->setCurrentIndex(targetItem->index());
        m_diagnosticsTree->blockSignals(false);
        if (doShow) {
            m_mainWindow->showToolView(parentWidget());
        }
    }
    return targetItem != nullptr;
}

void DiagnosticsView::updateDiagnosticsSuppression(QStandardItem *topItem, KTextEditor::Document *doc, bool force)
{
    if (!topItem || !doc) {
        return;
    }

    auto diagTopItem = static_cast<DocumentDiagnosticItem *>(topItem);
    auto &suppressions = diagTopItem->diagnosticSuppression;
    if (!suppressions || force) {
        auto provider = diagTopItem->data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>();
        const auto sessionSuppressions = m_sessionDiagnosticSuppressions->getSuppressions(doc->url().toLocalFile());
        auto supp = new DiagnosticSuppression(doc, provider->suppressions(doc), sessionSuppressions);
        suppressions.reset(supp);
        updateDiagnosticsState(topItem);
    }
}

void DiagnosticsView::onContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);

    auto menu = new QMenu(m_diagnosticsTree);
    menu->addAction(i18n("Expand All"), m_diagnosticsTree, &QTreeView::expandAll);
    menu->addAction(i18n("Collapse All"), m_diagnosticsTree, &QTreeView::collapseAll);
    menu->addSeparator();

    QModelIndex index = m_diagnosticsTree->currentIndex();
    auto item = m_model.itemFromIndex(index);
    auto diagItem = dynamic_cast<DiagnosticItem *>(item);
    auto docDiagItem = dynamic_cast<DocumentDiagnosticItem *>(item);
    if (diagItem) {
        auto diagText = index.data().toString();
        menu->addAction(QIcon::fromTheme(QLatin1String("edit-copy")), i18n("Copy to Clipboard"), [diagText]() {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(diagText);
        });
        menu->addSeparator();
        auto parent = index.parent();
        docDiagItem = dynamic_cast<DocumentDiagnosticItem *>(m_model.itemFromIndex(parent));
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
    } else if (docDiagItem) {
        // track validity of raw pointer
        QPersistentModelIndex pindex(index);
        auto h = [this, docDiagItem, pindex](bool enabled) {
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

QString DiagnosticsView::onTextHint(KTextEditor::View *view, const KTextEditor::Cursor &position) const
{
    QString result;
    auto document = view->document();

    // bool diagHover = m_diagnostics && m_diagnostics->isChecked() && m_diagnosticsHover && m_diagnosticsHover->isChecked();

    QStandardItem *topItem = getItem(m_model, document->url());
    QStandardItem *targetItem = getItem(topItem, position, false);
    if (targetItem) {
        result = targetItem->text();
        // also include related info
        int count = targetItem->rowCount();
        for (int i = 0; i < count; ++i) {
            auto item = targetItem->child(i);
            result += QStringLiteral("\n<br>");
            result += item->text();
        }
        // but let's not get carried away too far
        constexpr int maxsize = 1000;
        if (result.size() > maxsize) {
            result.resize(maxsize);
            result.append(QStringLiteral("..."));
        }
    }
    return result;
}
