/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "breakpointview.h"

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QAbstractTableModel>
#include <QLoggingCategory>
#include <QTreeView>
#include <QVBoxLayout>

#include <ktexteditor_utils.h>

#include <memory_resource>
#include <span>

namespace
{
Q_LOGGING_CATEGORY(kateBreakpoint, "kate-breakpoint", QtDebugMsg)

// [x] Set initial breakpoints that get set when debugging starts
// [x] Breakpoint events support
// [x] Checkable breakpoints
// [x] Breakpoints should be sorted by linenumber
// [x] Clear all breakpoints
// [x] Fix KatePluginGDBView::prepareDocumentBreakpoints, it should work whether or not debugger is running. Perhaps remove it altogether because we shouldn't
// call that function when a view is created. Listening on view creation is useless here
// [x] Document loads its initial mark using us, not backend
// [x] add a test for this
// [x] Fix run to cursor
// [x] Double clicking on a breakpoint takes us to the location?
// [] Cleanup, add a header to the model, path item can span multiple columns?

static QString printBreakpoint(const QUrl &sourceId, const dap::SourceBreakpoint &def, const std::optional<dap::Breakpoint> &bp, const int bId)
{
    QString txtId = QStringLiteral("%1.").arg(bId);
    if (!bp) {
        txtId += QStringLiteral(" ");
    } else {
        if (bp->verified) {
            txtId += bp->id ? QString::number(bp->id.value()) : QStringLiteral("?");
        } else {
            txtId += QStringLiteral("!");
        }
    }

    QStringList out = {QStringLiteral("[%1] %2: %3").arg(txtId).arg(Utils::formatUrl(sourceId)).arg(def.line)};
    if (def.column) {
        out << QStringLiteral(", %1").arg(def.column.value());
    }
    if (bp) {
        if (bp->line) {
            out << QStringLiteral("->%1").arg(bp->line.value());
            if (bp->endLine) {
                out << QStringLiteral("-%1").arg(bp->endLine.value());
            }
            if (bp->column) {
                out << QStringLiteral(",%1").arg(bp->column.value());
                if (bp->endColumn) {
                    out << QStringLiteral("-%1").arg(bp->endColumn.value());
                }
            }
        }
    }
    if (def.condition) {
        out << QStringLiteral(" when {%1}").arg(def.condition.value());
    }
    if (def.hitCondition) {
        out << QStringLiteral(" hitcount {%1}").arg(def.hitCondition.value());
    }
    if (bp && bp->message) {
        out << QStringLiteral(" (%1)").arg(bp->message.value());
    }

    return out.join(QString());
}

struct FileBreakpoint {
    QUrl url;
    dap::SourceBreakpoint sourceBreakpoint;
    std::optional<dap::Breakpoint> breakpoint;
    Qt::CheckState checkState = Qt::Checked;
    bool isOneShot = false;

    bool isEnabled() const
    {
        return checkState == Qt::Checked;
    }

    int line() const
    {
        if (breakpoint && breakpoint->line) {
            return breakpoint->line.value();
        }
        return sourceBreakpoint.line;
    }

    bool operator==(const FileBreakpoint &r) const
    {
        return url == r.url && breakpoint == r.breakpoint && checkState == r.checkState;
    }
};
}

class BreakpointModel : public QAbstractItemModel
{
    Q_OBJECT

    enum Columns {
        Column0,

        Columns_Count
    };

    static constexpr int LineBreakpointsItem = 0;
    static constexpr quintptr Root = 0xFFFFFFFF;

    QList<FileBreakpoint> m_lineBreakpoints;

public:
    explicit BreakpointModel(QObject *parent)
        : QAbstractItemModel(parent)
    {
        beginInsertRows({}, 0, 0);
        endInsertRows();
    }

    int columnCount(const QModelIndex &) const override
    {
        return Columns_Count;
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (!parent.isValid()) {
            return 1;
        }
        if (parent.internalId() == Root) {
            switch (parent.row()) {
            case LineBreakpointsItem:
                return m_lineBreakpoints.size();
            }
            Q_ASSERT(false);
        }
        return 0;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        auto rootIndex = Root;
        if (parent.isValid()) {
            if (parent.internalId() == Root) {
                rootIndex = parent.row();
            } else {
                Q_ASSERT(false);
            }
        }
        return createIndex(row, column, rootIndex);
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (child.isValid()) {
            if (child.internalId() == Root) {
                return {};
            }
            const int row = child.internalId();
            return createIndex(row, 0, Root);
        }
        return {};
    }

    bool hasChildren(const QModelIndex &parent) const override
    {
        if (parent.isValid()) {
            if (parent.internalId() == Root) {
                switch (parent.row()) {
                case LineBreakpointsItem:
                    return !m_lineBreakpoints.empty();
                }
                qCWarning(kateBreakpoint, "Invalid top level item: %d", parent.row());
                Q_ASSERT(false);
            } else {
                return false;
            }
        }
        return true;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid()) {
            return {};
        }

        const int row = index.row();

        if (index.internalId() == Root) {
            if (row == LineBreakpointsItem && role == Qt::DisplayRole) {
                return i18n("Line Breakpoints");
            }
            if (role == Qt::FontRole) {
                QFont font;
                font.setBold(true);
                return font;
            }
        } else {
            int rootIndex = index.internalId();
            if (rootIndex > LineBreakpointsItem) {
                Q_ASSERT(false);
                return {};
            }

            const int col = index.column();
            if (rootIndex == LineBreakpointsItem) {
                if (row >= m_lineBreakpoints.size()) {
                    qCWarning(kateBreakpoint, "Invalid row: %d", row);
                    return {};
                }
                const auto &item = m_lineBreakpoints[row].breakpoint;

                if (role == Qt::DisplayRole) {
                    if (col == Column0) {
                        if (item && item->source.has_value()) {
                            QString name = item->source.value().name;
                            if (item->line.has_value()) {
                                return QStringLiteral("%1:%2").arg(name).arg(item->line.value());
                            }
                            return QStringLiteral("%1:??").arg(name);
                        } else {
                            return QStringLiteral("%1:%2").arg(m_lineBreakpoints[row].url.fileName()).arg(m_lineBreakpoints[row].sourceBreakpoint.line);
                        }
                    }
                }

                if (role == Qt::CheckStateRole) {
                    return m_lineBreakpoints[row].checkState;
                }

                if (role == Qt::ToolTipRole) {
                    if (col == Column0) {
                        if (item && item->source.has_value()) {
                            return item->source.value().path.toDisplayString();
                        }
                    }
                }
            }
        }

        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (role == Qt::CheckStateRole && index.isValid() && !hasChildren(index)) {
            if (value.isValid() && index.row() >= 0 && index.row() < m_lineBreakpoints.size()) {
                auto &bp = m_lineBreakpoints[index.row()];
                bp.checkState = value.value<Qt::CheckState>();
                Q_EMIT dataChanged(index, index, {role});

                const int line = bp.breakpoint ? bp.breakpoint->line.value_or(-1) : bp.sourceBreakpoint.line;
                if (line >= 0) {
                    Q_EMIT breakpointEnabledChanged(bp.url, line, bp.isEnabled());
                }
                return true;
            }
            Q_ASSERT(false);
        }
        return false;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        auto flags = QAbstractItemModel::flags(index);
        if (hasChildren(index)) {
            return flags;
        }

        flags |= Qt::ItemIsUserCheckable;
        return flags;
    }

    static QList<dap::SourceBreakpoint> toSourceBreakpoints(QList<FileBreakpoint> &breakpoints)
    {
        QList<dap::SourceBreakpoint> ret;
        ret.reserve(breakpoints.size());
        for (auto &bp : breakpoints) {
            auto &sourceBreakpoint = bp.sourceBreakpoint;
            if (bp.breakpoint && bp.breakpoint->line.has_value()) {
                // use dap::Breakpoint line if available
                sourceBreakpoint.line = bp.breakpoint->line.value();
            }
            ret.push_back(sourceBreakpoint);
        }
        return ret;
    }

    QList<dap::SourceBreakpoint> sourceBreakpointsForPath(const QUrl &url)
    {
        QList<FileBreakpoint> breakpoints;
        for (const auto &b : m_lineBreakpoints) {
            if (b.isEnabled() && b.url == url) {
                breakpoints.push_back(b);
            }
        }
        return toSourceBreakpoints(breakpoints);
    }

    QList<dap::SourceBreakpoint> toggleBreakpoint(const QUrl &url, int line, bool isOneShot, std::optional<bool> breakpointEnabledChange)
    {
        if (breakpointEnabledChange) {
            return sourceBreakpointsForPath(url);
        }

        auto bp = dap::SourceBreakpoint(line);
        auto existing = getFileBreakpoints(url);
        auto it = std::lower_bound(existing.begin(), existing.end(), bp, [](const FileBreakpoint &l, const dap::SourceBreakpoint &val) {
            return l.line() < val.line;
        });
        const auto fileStart = existing.empty() ? m_lineBreakpoints.size() : existing.data() - m_lineBreakpoints.data();
        const auto pos = fileStart + static_cast<int>(std::distance(existing.begin(), it));

        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());
        if (it == existing.end() || it->line() != line) {
            beginInsertRows(parent, pos, pos);
            m_lineBreakpoints.insert(pos,
                                     FileBreakpoint{
                                         .url = url,
                                         .sourceBreakpoint = bp,
                                         .breakpoint = {},
                                         .isOneShot = isOneShot,
                                     });
            endInsertRows();
        } else {
            if (isOneShot) {
                // For one shot, don't remove instead force check + mark it one shot
                m_lineBreakpoints[pos].isOneShot = true;
                m_lineBreakpoints[pos].checkState = Qt::Checked;
                auto thisIdx = index(pos, 0, parent);
                Q_EMIT dataChanged(thisIdx, thisIdx);
            } else {
                beginRemoveRows(parent, pos, pos);
                m_lineBreakpoints.remove(pos);
                endRemoveRows();
            }
        }

        return sourceBreakpointsForPath(url);
    }

    void fileBreakpointsSet(const QUrl &url, QList<dap::Breakpoint> newDapBreakpoints)
    {
        std::byte memory[sizeof(FileBreakpoint) * 30];
        std::pmr::monotonic_buffer_resource allocator(memory, sizeof(memory));

        std::span oldBreakpoints = getFileBreakpoints(url);

        // ensure elements are sorted
        std::sort(newDapBreakpoints.begin(), newDapBreakpoints.end(), [](const dap::Breakpoint &l, const dap::Breakpoint &r) {
            return l.line < r.line;
        });

        std::pmr::vector<FileBreakpoint> newBreakpoints(&allocator);
        newBreakpoints.reserve(newDapBreakpoints.size());
        for (auto &&b : newDapBreakpoints) {
            newBreakpoints.push_back(FileBreakpoint{.url = url, .sourceBreakpoint = {INT_MAX}, .breakpoint = std::move(b)});
        }

        auto sortedUniqueInsert = [&newBreakpoints](const FileBreakpoint &b) {
            auto it = std::lower_bound(newBreakpoints.begin(), newBreakpoints.end(), b, [](const FileBreakpoint &l, const FileBreakpoint &val) {
                return l.breakpoint->line < val.line();
            });
            if (it == newBreakpoints.end() || it->breakpoint->line != b.breakpoint->line) {
                newBreakpoints.insert(it, b);
            }
        };

        // merge all old disabled breakpoints into new ones
        for (const auto &b : oldBreakpoints) {
            if (!b.isEnabled()) {
                sortedUniqueInsert(b);
            }
        }

        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());

        if (oldBreakpoints.size() != static_cast<size_t>(newBreakpoints.size())) {
            auto setDifference = [&allocator](const auto &list1, const auto &list2) {
                std::pmr::vector<FileBreakpoint> ret(&allocator);
                auto lessThanByLine = [](const FileBreakpoint &l, const FileBreakpoint &r) {
                    return l.line() < r.line();
                };
                std::set_difference(list1.begin(), list1.end(), list2.begin(), list2.end(), std::back_inserter(ret), lessThanByLine);
                return ret;
            };

            std::pmr::vector<FileBreakpoint> newlyAddedBreakpoints = setDifference(newBreakpoints, oldBreakpoints);
            std::pmr::vector<FileBreakpoint> removedBreakpoints = setDifference(oldBreakpoints, newBreakpoints);

            if (!removedBreakpoints.empty()) {
                const auto startIndex = oldBreakpoints.empty() ? m_lineBreakpoints.size() : static_cast<int>(oldBreakpoints.data() - m_lineBreakpoints.data());
                for (const auto &b : removedBreakpoints) {
                    int i = m_lineBreakpoints.indexOf(b, startIndex);
                    Q_ASSERT(i >= 0);
                    beginRemoveRows(parent, i, i);
                    m_lineBreakpoints.remove(i);
                    endRemoveRows();
                }
            }

            if (!newlyAddedBreakpoints.empty()) {
                // get breakpoints again as they might be invalidated above
                oldBreakpoints = getFileBreakpoints(url);
                const auto startIndex = oldBreakpoints.empty() ? m_lineBreakpoints.size() : static_cast<int>(oldBreakpoints.data() - m_lineBreakpoints.data());
                auto lessThanByLine = [](const FileBreakpoint &l, const FileBreakpoint &r) {
                    return l.line() < r.line();
                };

                for (const auto &b : newlyAddedBreakpoints) {
                    auto it = std::lower_bound(oldBreakpoints.begin(), oldBreakpoints.end(), b, lessThanByLine);
                    Q_ASSERT(it == oldBreakpoints.end() || it->line() != b.line());
                    const auto pos = startIndex + static_cast<int>(std::distance(oldBreakpoints.begin(), it));
                    beginInsertRows(parent, pos, pos);
                    m_lineBreakpoints.insert(pos, b);
                    endInsertRows();
                }
            }

            oldBreakpoints = getFileBreakpoints(url);
            Q_ASSERT(oldBreakpoints.size() == static_cast<size_t>(newBreakpoints.size()));
            // replace the old list below, important as breakpoint ids might have changed even though line/url info matches
        }

        // just replace the old list and emit data changed
        if (!newBreakpoints.empty()) {
            const auto startIndex = oldBreakpoints.empty() ? m_lineBreakpoints.size() : static_cast<int>(oldBreakpoints.data() - m_lineBreakpoints.data());
            auto it = newBreakpoints.begin();
            for (auto &oldBP : oldBreakpoints) {
                oldBP.breakpoint = std::move(it->breakpoint);
                it++;
            }
            Q_EMIT dataChanged(index(startIndex, 0, parent), index(startIndex + newBreakpoints.size() - 1, columnCount(parent) - 1, parent));
        }
    }

    void onBreakpointEvent(const dap::Breakpoint &bp, BackendInterface::BreakpointEventKind kind)
    {
        switch (kind) {
        case BackendInterface::New:
            addNewBreakpoint(bp);
            break;
        case BackendInterface::Changed:
            onBreakpointChanged(bp);
            break;
        case BackendInterface::Removed:
            onBreakpointRemoved(bp);
            break;
        }
    }

    void addNewBreakpoint(const dap::Breakpoint &bp)
    {
        if (!bp.source) {
            // ignore
            return;
        }
        const auto url = bp.source.value().path;
        const auto fileBreakpoints = getFileBreakpoints(url);
        auto it = std::lower_bound(fileBreakpoints.begin(), fileBreakpoints.end(), bp, [](const FileBreakpoint &l, const dap::Breakpoint &val) {
            return l.line() < val.line;
        });
        const auto fileStart = fileBreakpoints.empty() ? m_lineBreakpoints.size() : fileBreakpoints.data() - m_lineBreakpoints.data();
        const auto pos = fileStart + static_cast<int>(std::distance(fileBreakpoints.begin(), it));

        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());
        beginInsertRows(parent, pos, pos);
        m_lineBreakpoints.insert(pos,
                                 FileBreakpoint{
                                     .url = url,
                                     .sourceBreakpoint = dap::SourceBreakpoint(bp.line.has_value() ? bp.line.value() : INT_MAX),
                                     .breakpoint = bp,
                                 });
        endInsertRows();

        if (bp.line && bp.source.has_value() && !bp.source.value().path.isEmpty()) {
            Q_EMIT breakpointChanged(bp.source.value().path, std::nullopt, bp.line.value(), BackendInterface::BreakpointEventKind::New);
        }
    }

    void onBreakpointChanged(const dap::Breakpoint &bp)
    {
        if (!bp.id) {
            // ignore if no source or id
            return;
        }

        if (!bp.source) {
            // can we do better?
            qCWarning(kateBreakpoint, "breakpoint-update: ignoring because breakpoint has no path set");
            return;
        }

        const auto id = bp.id.value();
        auto it = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [id](const FileBreakpoint &fp) {
            return fp.breakpoint && fp.breakpoint->id == id;
        });

        // If we don't find an id for this, ignore
        if (it == m_lineBreakpoints.end()) {
            qCWarning(kateBreakpoint, "breakpoint-update: Unexpected didn't find a breakpoint in the model for id: %d", id);
            return;
        }

        if (it->breakpoint == bp) {
            return;
        }

        const auto url = bp.source.value().path;
        const auto fileBreakpoints = getFileBreakpoints(url);
        Q_ASSERT(!fileBreakpoints.empty());
        const int fileStartIdx = fileBreakpoints.data() - m_lineBreakpoints.data();

        auto fit = std::lower_bound(fileBreakpoints.begin(), fileBreakpoints.end(), bp, [](const FileBreakpoint &l, const dap::Breakpoint &val) {
            return l.line() < val.line;
        });
        const auto newPos = fileStartIdx + std::distance(fileBreakpoints.begin(), fit);
        const auto oldPos = std::distance(m_lineBreakpoints.begin(), it);
        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());
        const auto oldLine = it->line();

        if (newPos == oldPos) {
            it->breakpoint = bp;

            const int pos = it - m_lineBreakpoints.begin();

            Q_EMIT dataChanged(index(pos, 0, parent), index(pos, columnCount({}), parent));
        } else {
            beginMoveRows(parent, oldPos, oldPos, parent, newPos);
            m_lineBreakpoints.insert(newPos, m_lineBreakpoints[oldPos]);
            m_lineBreakpoints[newPos].breakpoint = bp;
            if (newPos > oldPos) {
                m_lineBreakpoints.remove(oldPos);
            } else {
                m_lineBreakpoints.remove(oldPos + 1);
            }
            endMoveRows();
        }

        if (bp.line && bp.source.has_value() && !bp.source.value().path.isEmpty()) {
            Q_EMIT breakpointChanged(bp.source.value().path, oldLine, bp.line.value(), BackendInterface::BreakpointEventKind::Changed);
        }
    }

    void onBreakpointRemoved(const dap::Breakpoint &bp)
    {
        if (!bp.id) {
            return;
        }
        const auto id = bp.id.value();
        auto it = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [id](const FileBreakpoint &fp) {
            return fp.breakpoint && fp.breakpoint->id == id;
        });

        if (it == m_lineBreakpoints.end()) {
            qCWarning(kateBreakpoint, "breakpoint-remove: Unexpected didn't find a breakpoint in the model for id: %d", id);
            return;
        }

        const int pos = it - m_lineBreakpoints.begin();
        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());
        beginRemoveRows(parent, pos, pos);
        m_lineBreakpoints.remove(pos);
        endRemoveRows();

        if (bp.line && bp.source.has_value() && !bp.source.value().path.isEmpty()) {
            Q_EMIT breakpointChanged(bp.source.value().path, std::nullopt, bp.line.value(), BackendInterface::BreakpointEventKind::Removed);
        }
    }

    std::map<QUrl, QList<dap::SourceBreakpoint>> allBreakpoints() const
    {
        std::map<QUrl, QList<dap::SourceBreakpoint>> ret;
        for (const auto &fileBreakpoint : m_lineBreakpoints) {
            if (fileBreakpoint.isEnabled()) {
                auto sourceBreakpoint = fileBreakpoint.sourceBreakpoint;
                if (fileBreakpoint.breakpoint && fileBreakpoint.breakpoint->line.has_value()) {
                    sourceBreakpoint.line = fileBreakpoint.breakpoint->line.value();
                }
                ret[fileBreakpoint.url] << sourceBreakpoint;
            }
        }
        return ret;
    }

    QString printBreakpoints()
    {
        QString out;
        int bId = 0;
        for (const auto &breakpoint : m_lineBreakpoints) {
            out += printBreakpoint(breakpoint.url, breakpoint.sourceBreakpoint, breakpoint.breakpoint, bId);
            out += QStringLiteral("\n");
            ++bId;
        }
        return out;
    }

    std::span<FileBreakpoint> getFileBreakpoints(const QUrl &url)
    {
        auto it = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [url](const FileBreakpoint &fp) {
            return url == fp.url;
        });
        auto start = it;

        if (start == m_lineBreakpoints.end()) {
            return {};
        }

        while (it != m_lineBreakpoints.end() && it->url == url) {
            ++it;
        }
        return std::span{start, it};
    }

    void clearLineBreakpoints()
    {
        if (m_lineBreakpoints.empty()) {
            return;
        }
        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());
        beginRemoveRows(parent, 0, m_lineBreakpoints.size() - 1);
        m_lineBreakpoints.clear();
        endRemoveRows();
    }

    bool hasSingleShotBreakpointAtLine(const QUrl &url, int line)
    {
        const auto breakpoints = getFileBreakpoints(url);
        auto it = std::find_if(breakpoints.begin(), breakpoints.end(), [line](const FileBreakpoint &n) {
            return n.breakpoint.has_value() && n.breakpoint->line == line;
        });
        if (it != breakpoints.end() && it->isOneShot && it->breakpoint) {
            return true;
        }
        return false;
    }

    std::pair<QUrl, int> fileAndLineForIndex(const QModelIndex &index)
    {
        Q_ASSERT(index.isValid() && index.model() == this);
        if (!index.parent().isValid()) {
            return {{}, -1};
        }
        if (index.row() >= 0 && index.row() < m_lineBreakpoints.size()) {
            const auto &item = m_lineBreakpoints[index.row()];
            return {item.url, item.line()};
        }
        return {{}, -1};
    }

Q_SIGNALS:
    /**
     * Breakpoint at file:line changed
     * line is 1 based index
     */
    void breakpointChanged(const QUrl &url, std::optional<int> oldline, int line, BackendInterface::BreakpointEventKind);
    void breakpointEnabledChanged(const QUrl &url, int line, bool enabled);
};

BreakpointView::BreakpointView(KTextEditor::MainWindow *mainWindow, BackendInterface *backend, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_backend(backend)
    , m_treeview(new QTreeView(this))
    , m_breakpointModel(new BreakpointModel(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_treeview);

    m_treeview->setUniformRowHeights(true);
    m_treeview->setHeaderHidden(true);
    m_treeview->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeview->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_treeview, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        if (index.isValid()) {
            const auto [url, line] = m_breakpointModel->fileAndLineForIndex(index);
            if (url.isValid()) {
                if (auto view = m_mainWindow->openUrl(url)) {
                    view->setCursorPosition({line - 1, 0});
                }
            }
        }
    });

    QItemSelectionModel *m = m_treeview->selectionModel();
    m_treeview->setModel(m_breakpointModel);
    delete m;

    connect(m_backend, &BackendInterface::breakPointsSet, this, &BreakpointView::slotBreakpointsSet);
    connect(m_backend, &BackendInterface::breakpointEvent, this, &BreakpointView::onBreakpointEvent);

    connect(m_backend, &BackendInterface::removeBreakpointRequested, this, &BreakpointView::onRemoveBreakpointRequested);
    connect(m_backend, &BackendInterface::addBreakpointRequested, this, &BreakpointView::onAddBreakpointRequested);
    connect(m_backend, &BackendInterface::listBreakpointsRequested, this, &BreakpointView::onListBreakpointsRequested);
    connect(m_backend, &BackendInterface::runToLineRequested, this, &BreakpointView::runToPosition);

    connect(m_breakpointModel,
            &BreakpointModel::breakpointChanged,
            this,
            [this](const QUrl &url, std::optional<int> oldLine, int line, BackendInterface::BreakpointEventKind kind) {
                auto app = KTextEditor::Editor::instance()->application();
                if (auto doc = app->findUrl(url)) {
                    disconnect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints);

                    if (kind == BackendInterface::BreakpointEventKind::Changed) {
                        if (oldLine.has_value()) {
                            doc->removeMark(oldLine.value() - 1, KTextEditor::Document::BreakpointActive);
                        }
                        // Ensure there is a red dot for this line
                        if ((doc->mark(line - 1) & KTextEditor::Document::BreakpointActive) == 0) {
                            doc->addMark(line - 1, KTextEditor::Document::BreakpointActive);
                        }
                    } else if (kind == BackendInterface::BreakpointEventKind::Removed) {
                        doc->removeMark(line - 1, KTextEditor::Document::BreakpointActive);
                    } else if (kind == BackendInterface::BreakpointEventKind::New) {
                        doc->addMark(line - 1, KTextEditor::Document::BreakpointActive);
                    }

                    connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);
                }
            });

    connect(m_breakpointModel, &BreakpointModel::breakpointEnabledChanged, this, [this](const QUrl &url, int line, bool enabled) {
        auto app = KTextEditor::Editor::instance()->application();

        if (auto doc = app->findUrl(url)) {
            disconnect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints);
            if (enabled) {
                doc->addMark(line - 1, KTextEditor::Document::BreakpointActive);
            } else {
                doc->removeMark(line - 1, KTextEditor::Document::BreakpointActive);
            }
            connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);
        }
        setBreakpoint(url, line, enabled);
    });

    const auto documents = KTextEditor::Editor::instance()->application()->documents();
    for (auto doc : documents) {
        enableBreakpointMarks(doc);
    }
    connect(KTextEditor::Editor::instance()->application(), &KTextEditor::Application::documentCreated, this, &BreakpointView::enableBreakpointMarks);
}

void BreakpointView::toggleBreakpoint()
{
    // qCDebug(kateBreakpoint, "%s", __FUNCTION__);

    if (m_backend->debuggerRunning() && !m_backend->canContinue()) {
        m_backend->slotInterrupt();
    } else {
        if (!m_mainWindow) {
            return;
        }

        KTextEditor::View *editView = m_mainWindow->activeView();
        if (!editView) {
            return;
        }
        QUrl currURL = editView->document()->url();
        if (!currURL.isValid()) {
            return;
        }

        int line = editView->cursorPosition().line() + 1;
        setBreakpoint(currURL, line, {});
    }
}

void BreakpointView::slotBreakpointsSet(const QUrl &file, const QList<dap::Breakpoint> &breakpoints)
{
    qCDebug(kateBreakpoint, "%s %ls numBreakpoints: %lld", __FUNCTION__, qUtf16Printable(file.fileName()), breakpoints.size());
    auto app = KTextEditor::Editor::instance()->application();
    if (auto doc = app->findUrl(file)) {
        disconnect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints);

        for (const auto &bp : breakpoints) {
            if (bp.line) {
                const int line = bp.line.value() - 1;
                const auto mark = doc->mark(line);
                if ((mark & KTextEditor::Document::BreakpointActive) == 0) {
                    doc->addMark(line, KTextEditor::Document::BreakpointActive);
                    qCDebug(kateBreakpoint) << "add mark" << line;
                }
            }
        }

        const auto marks = doc->marks();
        for (const auto &[line, mark] : marks.asKeyValueRange()) {
            if (mark->type == KTextEditor::Document::BreakpointActive) {
                auto it = std::find_if(breakpoints.begin(), breakpoints.end(), [line](const dap::Breakpoint &bp) {
                    return bp.line.has_value() && line == bp.line.value() - 1;
                });
                if (it == breakpoints.end()) {
                    doc->removeMark(line, KTextEditor::Document::BreakpointActive);
                    qCDebug(kateBreakpoint) << "remove mark" << line;
                }
            }
        }

        connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);
    }

    m_breakpointModel->fileBreakpointsSet(file, breakpoints);
}

void BreakpointView::clearLineBreakpoints()
{
    qCDebug(kateBreakpoint, "%s", __FUNCTION__);

    if (m_backend->debuggerRunning()) {
        auto allBreakpoints = m_breakpointModel->allBreakpoints();
        for (auto &[url, breakpoints] : allBreakpoints) {
            breakpoints.clear();
            m_backend->setBreakpoints(url, breakpoints);
        }
    }

    // clear the model
    m_breakpointModel->clearLineBreakpoints();

    // remove all breakpoint marks in open files
    auto app = KTextEditor::Editor::instance()->application();
    const auto documents = app->documents();
    for (KTextEditor::Document *doc : documents) {
        const QHash<int, KTextEditor::Mark *> marks = doc->marks();
        if (!marks.isEmpty()) {
            disconnect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints);
            QHashIterator<int, KTextEditor::Mark *> i(marks);
            while (i.hasNext()) {
                i.next();
                if (i.value()->type & KTextEditor::Document::BreakpointActive) {
                    doc->removeMark(i.value()->line, i.value()->type);
                }
            }
            connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);
        }
    }
}

void BreakpointView::runToCursor()
{
    KTextEditor::View *editView = m_mainWindow->activeView();
    QUrl currURL = editView->document()->url();
    if (!currURL.isValid()) {
        return;
    }
    KTextEditor::Cursor cursor = editView->cursorPosition();

    runToPosition(currURL, cursor.line() + 1);
}

void BreakpointView::onStoppedAtLine(const QUrl &url, int line)
{
    Q_ASSERT(m_backend->canContinue());
    if (m_breakpointModel->hasSingleShotBreakpointAtLine(url, line)) {
        setBreakpoint(url, line, std::nullopt);
    }
}

void BreakpointView::updateBreakpoints(const KTextEditor::Document *document, const KTextEditor::Mark mark)
{
    if (!document->url().isValid()) {
        return;
    }

    if (mark.type == KTextEditor::Document::MarkTypes::BreakpointActive) {
        if (m_backend->debuggerRunning() && !m_backend->canContinue()) {
            m_backend->slotInterrupt();
        }
        setBreakpoint(document->url(), mark.line + 1, {});
    }
}

void BreakpointView::setBreakpoint(const QUrl &file, int line, std::optional<bool> enabledStateChange, bool isOneShot)
{
    auto breakpoints = m_breakpointModel->toggleBreakpoint(file, line, isOneShot, enabledStateChange);
    if (m_backend->debuggerRunning()) {
        m_backend->setBreakpoints(file, breakpoints);
    }
}

void BreakpointView::enableBreakpointMarks(KTextEditor::Document *doc)
{
    if (doc) {
        doc->setEditableMarks(doc->editableMarks() | KTextEditor::Document::BreakpointActive);
        doc->setMarkDescription(KTextEditor::Document::BreakpointActive, i18n("Breakpoint"));
        doc->setMarkIcon(KTextEditor::Document::BreakpointActive, QIcon::fromTheme(QStringLiteral("media-record")));

        // Update breakpoints when they're added or removed to the debugger
        connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);

        // When a view is created, add breakpoint marks. We don't do it upfront to avoid wasteful work
        connect(doc, &KTextEditor::Document::viewCreated, this, [this, doc] {
            if (!doc->url().isValid()) {
                return;
            }
            const auto fileBreakpoints = m_breakpointModel->getFileBreakpoints(doc->url());
            if (fileBreakpoints.empty()) {
                return;
            }

            const int lines = doc->lines();
            disconnect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints);
            for (auto i = 0; i < lines; i++) {
                auto it = std::find_if(fileBreakpoints.begin(), fileBreakpoints.end(), [i](const FileBreakpoint &b) {
                    return b.line() - 1 == i;
                });
                if (it != fileBreakpoints.end()) {
                    doc->setMark(i, KTextEditor::Document::MarkTypes::BreakpointActive);
                }
            }
            connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);
        });
    }
}

void BreakpointView::onRemoveBreakpointRequested(const QUrl &url, int line)
{
    auto existing = m_breakpointModel->sourceBreakpointsForPath(url);
    auto it = std::find_if(existing.begin(), existing.end(), [line](const dap::SourceBreakpoint &n) {
        return n.line == line;
    });
    if (it != existing.end()) {
        setBreakpoint(url, line, {});
    }
}

void BreakpointView::onAddBreakpointRequested(const QUrl &url, const dap::SourceBreakpoint &breakpoint)
{
    auto existing = m_breakpointModel->sourceBreakpointsForPath(url);
    const auto line = breakpoint.line;
    auto it = std::find_if(existing.begin(), existing.end(), [line](const dap::SourceBreakpoint &n) {
        return n.line == line;
    });
    if (it == existing.end()) {
        setBreakpoint(url, line, {});
    }
}

void BreakpointView::onListBreakpointsRequested()
{
    QString out = m_breakpointModel->printBreakpoints();
    if (out.isEmpty()) {
        m_backend->outputText(i18n("No breakpoints set"));
    } else {
        m_backend->outputText(out);
    }
}

void BreakpointView::runToPosition(const QUrl &url, int line)
{
    setBreakpoint(url, line, std::nullopt, /*isOneShot=*/true);
    if (m_backend->canContinue()) {
        m_backend->slotContinue();
    }
}

std::map<QUrl, QList<dap::SourceBreakpoint>> BreakpointView::allBreakpoints() const
{
    return m_breakpointModel->allBreakpoints();
}

void BreakpointView::onBreakpointEvent(const dap::Breakpoint &bp, BackendInterface::BreakpointEventKind kind)
{
    m_breakpointModel->onBreakpointEvent(bp, kind);
}

#include "breakpointview.moc"
