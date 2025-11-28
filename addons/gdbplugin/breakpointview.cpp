/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "breakpointview.h"
#include "backend.h"

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QAbstractTableModel>
#include <QLoggingCategory>
#include <QTreeView>
#include <QVBoxLayout>

#include <span>

Q_LOGGING_CATEGORY(kateBreakpoint, "kate-breakpoint", QtDebugMsg)

// [x] Set initial breakpoints that get set when debugging starts
// [x] Breakpoint events support
// [] Fix run to cursor
// [] Cleanup, add a header to the model, path item can span multiple columns?
// [] Breakpoints should be sorted by linenumber

class BreakpointModel : public QAbstractItemModel
{
    Q_OBJECT

    enum Columns {
        Column0,

        Columns_Count
    };

    static constexpr int LineBreakpointsItem = 0;
    static constexpr quintptr Root = 0xFFFFFFFF;

    struct FileBreakpoint {
        QUrl url;
        dap::Breakpoint breakpoint;
    };
    QList<FileBreakpoint> m_lineBreakpoints;

public:
    explicit BreakpointModel(QObject *parent);

    int columnCount(const QModelIndex &parent) const override
    {
        return parent.isValid() ? 0 : Columns_Count;
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (!parent.isValid()) {
            return 1;
        }
        if (parent.internalId() == Root) {
            return m_lineBreakpoints.size();
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
                        if (item.source.has_value()) {
                            QString name = item.source.value().name;
                            if (item.line.has_value()) {
                                return QStringLiteral("%1:%2").arg(name).arg(item.line.value());
                            }
                            return QStringLiteral("%1:??").arg(name);
                        }
                    }
                }

                if (role == Qt::ToolTipRole) {
                    if (col == Column0) {
                        if (item.source.has_value()) {
                            return item.source.value().path.toDisplayString();
                        }
                    }
                }
            }
        }

        return {};
    }

    void toggleBreakpoint(const QUrl &url, int line);

    static QList<dap::SourceBreakpoint> toSourceBreakpoints(const QList<FileBreakpoint> &breakpoints)
    {
        QList<dap::SourceBreakpoint> ret;
        ret.reserve(breakpoints.size());
        for (const auto &bp : breakpoints) {
            if (bp.breakpoint.line.has_value()) {
                ret.push_back(dap::SourceBreakpoint(bp.breakpoint.line.value()));
            }
        }
        return ret;
    }

    QList<dap::SourceBreakpoint> sourceBreakpointsForPath(const QUrl &url)
    {
        QList<FileBreakpoint> breakpoints;
        for (const auto &b : m_lineBreakpoints) {
            if (b.url == url) {
                breakpoints.push_back(b);
            }
        }
        return toSourceBreakpoints(breakpoints);
    }

    void fileBreakpointsSet(const QUrl &url, const QList<dap::Breakpoint> &newBreakpoints)
    {
        auto oldRangeStart = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [url](const FileBreakpoint &bp) {
            return bp.url == url;
        });

        auto oldRangeEnd = std::find_if(oldRangeStart, m_lineBreakpoints.end(), [url](const FileBreakpoint &bp) {
            return bp.url != url;
        });

        std::span oldBreakpoints{oldRangeStart, oldRangeEnd};
        const auto startIndex = static_cast<int>(oldRangeStart - m_lineBreakpoints.begin());

        if (!oldBreakpoints.empty()) {
            beginRemoveRows(QModelIndex(), startIndex, static_cast<int>((startIndex + oldBreakpoints.size()) - 1));
            m_lineBreakpoints.erase(oldRangeStart, oldRangeEnd);
            endRemoveRows();
        }

        if (!newBreakpoints.empty()) {
            beginInsertRows(QModelIndex(), startIndex, startIndex + newBreakpoints.size() - 1);

            // preallocate
            m_lineBreakpoints.insert(startIndex, newBreakpoints.size(), FileBreakpoint{});
            for (int i = 0; i < newBreakpoints.size(); i++) {
                m_lineBreakpoints[startIndex + i] = FileBreakpoint{.url = url, .breakpoint = newBreakpoints[i]};
            }

            endInsertRows();
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
        auto it = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [url](const FileBreakpoint &fp) {
            return url == fp.url;
        });

        while (it != m_lineBreakpoints.end() && it->url == url) {
            ++it;
        }

        int pos = m_lineBreakpoints.size();
        if (it != m_lineBreakpoints.end()) {
            pos = it - m_lineBreakpoints.begin();
        }

        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());
        beginInsertRows(parent, pos, pos);
        m_lineBreakpoints.insert(pos, FileBreakpoint{.url = url, .breakpoint = bp});
        endInsertRows();
    }

    void onBreakpointChanged(const dap::Breakpoint &bp)
    {
        if (!bp.id) {
            // ignore if no source or id
            return;
        }
        const auto id = bp.id.value();
        auto it = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [id](const FileBreakpoint &fp) {
            return fp.breakpoint.id == id;
        });

        // If we don't find an id for this, add it as a new breakpoint
        if (it == m_lineBreakpoints.end()) {
            addNewBreakpoint(bp);
            return;
        }

        if (it->breakpoint == bp) {
            return;
        }

        it->breakpoint = bp;

        const int pos = it - m_lineBreakpoints.begin();
        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());

        Q_EMIT dataChanged(index(pos, 0, parent), index(pos, columnCount({}), parent));

        if (bp.line && bp.source.has_value() && !bp.source.value().path.isEmpty()) {
            Q_EMIT breakpointChanged(bp.source.value().path, bp.line.value(), BackendInterface::BreakpointEventKind::Changed);
        }
    }

    void onBreakpointRemoved(const dap::Breakpoint &bp)
    {
        if (!bp.id) {
            return;
        }
        const auto id = bp.id.value();
        auto it = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [id](const FileBreakpoint &fp) {
            return fp.breakpoint.id == id;
        });

        if (it == m_lineBreakpoints.end()) {
            return;
        }

        const int pos = it - m_lineBreakpoints.begin();

        const auto parent = index(LineBreakpointsItem, 0, QModelIndex());
        beginRemoveRows(parent, pos, pos);
        m_lineBreakpoints.remove(pos);
        endRemoveRows();

        if (bp.line && bp.source.has_value() && !bp.source.value().path.isEmpty()) {
            Q_EMIT breakpointChanged(bp.source.value().path, bp.line.value(), BackendInterface::BreakpointEventKind::Removed);
        }
    }

    std::map<QUrl, QList<dap::SourceBreakpoint>> allBreakpoints() const
    {
        std::map<QUrl, QList<dap::SourceBreakpoint>> ret;
        for (const auto &fileBreakpoint : m_lineBreakpoints) {
            if (fileBreakpoint.breakpoint.line.has_value()) {
                ret[fileBreakpoint.url] << dap::SourceBreakpoint(fileBreakpoint.breakpoint.line.value());
            }
        }
        return ret;
    }

Q_SIGNALS:
    /**
     * Breakpoint at file:line changed
     * line is 1 based index
     */
    void breakpointChanged(const QUrl &url, int line, BackendInterface::BreakpointEventKind);
};

BreakpointModel::BreakpointModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    beginInsertRows({}, 0, 1);
    endInsertRows();
}

void BreakpointModel::toggleBreakpoint(const QUrl &url, int line)
{
    auto it = std::find_if(m_lineBreakpoints.begin(), m_lineBreakpoints.end(), [url](const FileBreakpoint &fp) {
        return url == fp.url;
    });

    dap::Breakpoint bp;
    bp.source = dap::Source(url);
    bp.source.value().name = url.fileName();
    bp.line = line;
    const auto parent = index(LineBreakpointsItem, 0, QModelIndex());

    if (it == m_lineBreakpoints.end()) {
        beginInsertRows(parent, m_lineBreakpoints.size(), m_lineBreakpoints.size());
        m_lineBreakpoints.push_back(FileBreakpoint{.url = url, .breakpoint = bp});
        endInsertRows();
        qCDebug(kateBreakpoint, "added breakpoint %ls:%d", qUtf16Printable(url.toLocalFile()), line);
    } else {
        bool remove = false;
        while (it != m_lineBreakpoints.end() && it->url == url) {
            if (it->breakpoint.line.value_or(-1) == line) {
                remove = true;
                break;
            }
            ++it;
        }

        const auto pos = static_cast<int>(std::distance(m_lineBreakpoints.begin(), it));

        if (remove) {
            qCDebug(kateBreakpoint, "removed breakpoint %ls:%d", qUtf16Printable(url.toLocalFile()), line);

            beginRemoveRows(parent, pos, pos);
            m_lineBreakpoints.erase(it);
            endRemoveRows();
        } else {
            beginInsertRows(parent, pos, pos);
            m_lineBreakpoints.insert(pos, FileBreakpoint{.url = url, .breakpoint = bp});
            endInsertRows();
            qCDebug(kateBreakpoint, "added breakpoint %ls:%d", qUtf16Printable(url.toLocalFile()), line);
        }
    }
}

BreakpointView::BreakpointView(KTextEditor::MainWindow *mainWindow, Backend *backend, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_backend(backend)
    , m_treeview(new QTreeView(this))
    , m_breakpointModel(new BreakpointModel(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_treeview);

    m_treeview->setModel(m_breakpointModel);
    m_treeview->setUniformRowHeights(true);
    m_treeview->setHeaderHidden(true);
    // m_treeview->setRootIsDecorated(false);

    connect(m_backend, &BackendInterface::breakPointsSet, this, &BreakpointView::slotBreakpointsSet);
    connect(m_backend, &BackendInterface::clearBreakpointMarks, this, &BreakpointView::clearMarks);
    connect(m_backend, &BackendInterface::breakpointEvent, this, &BreakpointView::onBreakpointEvent);

    connect(m_breakpointModel, &BreakpointModel::breakpointChanged, this, [this](const QUrl &url, int line, BackendInterface::BreakpointEventKind kind) {
        auto app = KTextEditor::Editor::instance()->application();
        if (auto doc = app->findUrl(url)) {
            disconnect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints);

            if (kind == BackendInterface::BreakpointEventKind::Changed) {
                // Ensure there is a red dot for this line
                if ((doc->mark(line - 1) & KTextEditor::Document::BreakpointActive) == 0) {
                    doc->addMark(line - 1, KTextEditor::Document::BreakpointActive);
                }
            } else if (kind == BackendInterface::BreakpointEventKind::Removed) {
                doc->removeMark(line - 1, KTextEditor::Document::BreakpointActive);
            }

            connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);
        }
    });
}

void BreakpointView::toggleBreakpoint()
{
    // qCDebug(kateBreakpoint, "%s", __FUNCTION__);

    if (m_backend->debuggerRunning() && !m_backend->canContinue()) {
        m_backend->slotInterrupt();
    } else {
        KTextEditor::View *editView = m_mainWindow->activeView();
        if (!editView) {
            return;
        }
        QUrl currURL = editView->document()->url();
        if (!currURL.isValid()) {
            return;
        }

        int line = editView->cursorPosition().line() + 1;
        setBreakpoint(currURL, line);
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

        m_breakpointModel->fileBreakpointsSet(file, breakpoints);

        connect(doc, &KTextEditor::Document::markChanged, this, &BreakpointView::updateBreakpoints, Qt::UniqueConnection);
    }
}

void BreakpointView::clearMarks()
{
    // qCDebug(kateBreakpoint, "%s", __FUNCTION__);

    // TODO this is broken
    // auto app = KTextEditor::Editor::instance()->application();
    // const auto documents = app->documents();
    // for (KTextEditor::Document *doc : documents) {
    //     const QHash<int, KTextEditor::Mark *> marks = doc->marks();
    //     QHashIterator<int, KTextEditor::Mark *> i(marks);
    //     while (i.hasNext()) {
    //         i.next();
    //         if ((i.value()->type == KTextEditor::Document::Execution) || (i.value()->type == KTextEditor::Document::BreakpointActive)) {
    //             // m_backend->removeSavedBreakpoint(doc->url(), i.value()->line);
    //             doc->removeMark(i.value()->line, i.value()->type);
    //         }
    //     }
    // }
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
        setBreakpoint(document->url(), mark.line + 1);
    }
}

void BreakpointView::setBreakpoint(const QUrl &file, int line)
{
    if (m_backend->debuggerRunning()) {
        auto existing = m_breakpointModel->sourceBreakpointsForPath(file);
        auto it = std::find_if(existing.begin(), existing.end(), [line](const dap::SourceBreakpoint &n) {
            return n.line == line;
        });
        if (it == existing.end()) {
            existing.push_back(dap::SourceBreakpoint(line));
        } else {
            existing.erase(it);
        }

        m_backend->setBreakpoints(file, existing);
    } else {
        // update breakpoint in model
        m_breakpointModel->toggleBreakpoint(file, line);
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
