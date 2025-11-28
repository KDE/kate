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

    static constexpr quintptr PathItem = 0xFFFFFFFF;

public:
    explicit BreakpointModel(QObject *parent);

    int columnCount(const QModelIndex &parent) const override
    {
        return parent.isValid() ? 0 : Columns_Count;
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (!parent.isValid()) {
            return m_breakpoints.size();
        }
        if (parent.internalId() == PathItem) {
            if (parent.row() < m_breakpoints.size()) {
                return m_breakpoints[parent.row()].breakpoints.size();
            } else {
                qCWarning(kateBreakpoint, "Unexpected parent.row out of bounds");
            }
        }
        return 0;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        auto rootIndex = PathItem;
        if (parent.isValid()) {
            if (parent.internalId() == PathItem) {
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
            if (child.internalId() == PathItem) {
                return {};
            }

            const int row = child.internalId();
            return createIndex(row, 0, PathItem);
        }
        return {};
    }

    bool hasChildren(const QModelIndex &parent) const override
    {
        if (parent.isValid()) {
            if (parent.internalId() == PathItem)
                return true;
        } else {
            return !m_breakpoints.empty();
        }
        return false;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole) {
            const int row = index.row();
            const int col = index.column();

            if (index.internalId() == PathItem) {
                if (row >= m_breakpoints.size()) {
                    return {};
                }

                const auto &data = m_breakpoints[row];
                switch (col) {
                case Column0:
                    return data.url.toLocalFile();
                }
            } else {
                const int parent = index.internalId();
                if (parent >= m_breakpoints.size()) {
                    qCWarning(kateBreakpoint, "Unexpected parent row is out of bounds in data()");
                    return {};
                }

                const auto &fileBreakpoints = m_breakpoints[parent];
                if (index.row() >= fileBreakpoints.breakpoints.size()) {
                    qCWarning(kateBreakpoint, "Unexpected index.row is out of bounds in data()");
                    return {};
                }

                const auto &data = fileBreakpoints.breakpoints[index.row()];

                switch (col) {
                case Column0:
                    if (data.line.has_value()) {
                        return QString::number(data.line.value());
                    }
                }
            }
        }

        return {};
    }

    void toggleBreakpoint(const QUrl &url, int line);

    static QList<dap::SourceBreakpoint> toSourceBreakpoints(const QList<dap::Breakpoint> &breakpoints)
    {
        QList<dap::SourceBreakpoint> ret;
        ret.reserve(breakpoints.size());
        for (const auto &bp : breakpoints) {
            if (bp.line.has_value()) {
                ret.push_back(dap::SourceBreakpoint(bp.line.value()));
            }
        }
        return ret;
    }

    QList<dap::SourceBreakpoint> sourceBreakpointsForPath(const QUrl &url)
    {
        auto it = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [url](const FileBreakpoints &fp) {
            return url == fp.url;
        });

        if (it != m_breakpoints.end()) {
            return toSourceBreakpoints(it->breakpoints);
        }

        return {};
    }

    void fileBreakpointsSet(const QUrl &url, const QList<dap::Breakpoint> &breakpoints)
    {
        auto it = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [url](const FileBreakpoints &fp) {
            return url == fp.url;
        });

        int fileIndex = 0;
        if (it == m_breakpoints.end()) {
            // If there are no breakpoints, and we don't have an entry for this url
            // there is nothing to do
            if (breakpoints.empty()) {
                return;
            }

            fileIndex = m_breakpoints.size();

            beginInsertRows(QModelIndex(), fileIndex, fileIndex);
            m_breakpoints.push_back(FileBreakpoints{.url = url, .breakpoints = {}});
            endInsertRows();
        } else {
            fileIndex = it - m_breakpoints.begin();
        }

        auto &fileBreakpoints = m_breakpoints[fileIndex].breakpoints;
        const auto parent = index(fileIndex, 0, QModelIndex());

        // Remove existing breakpoints of this file
        if (!fileBreakpoints.empty()) {
            const int oldSize = fileBreakpoints.size();
            beginRemoveRows(parent, 0, oldSize - 1);
            fileBreakpoints.clear();
            endRemoveRows();
        }

        if (!breakpoints.empty()) {
            // Add new breakpoints
            const int newSize = breakpoints.size();
            beginInsertRows(parent, 0, newSize - 1);
            fileBreakpoints = breakpoints;
            endInsertRows();
        } else {
            // If there was nothing to add, then remove the file from model
            beginRemoveRows(QModelIndex(), fileIndex, fileIndex);
            m_breakpoints.remove(fileIndex);
            endRemoveRows();
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

        auto it = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [url](const FileBreakpoints &fp) {
            return url == fp.url;
        });

        int fileIndex = m_breakpoints.size();
        if (it != m_breakpoints.end()) {
            fileIndex = it - m_breakpoints.begin();
        } else {
            beginInsertRows(QModelIndex(), fileIndex, fileIndex);
            m_breakpoints.push_back(FileBreakpoints{.url = url, .breakpoints = {}});
            endInsertRows();
        }

        auto &fileBreakpoints = m_breakpoints[fileIndex].breakpoints;
        const auto parent = index(fileIndex, 0, QModelIndex());

        beginInsertRows(parent, fileBreakpoints.size(), fileBreakpoints.size());
        fileBreakpoints.push_back(bp);
        endInsertRows();
    }

    void onBreakpointChanged(const dap::Breakpoint &bp)
    {
        qDebug() << " breakpoint changed";
        if (!bp.source || !bp.id) {
            // ignore if no source or id
            qDebug() << "ignored, no source or id";
            return;
        }
        const auto url = bp.source.value().path;
        auto it = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [url](const FileBreakpoints &fp) {
            return url == fp.url;
        });

        // If we don't find a file for this, add it as a new breakpoint
        if (it == m_breakpoints.end()) {
            addNewBreakpoint(bp);
            return;
        }
        const int fileIndex = it - m_breakpoints.begin();
        auto &fileBreakpoints = m_breakpoints[fileIndex].breakpoints;
        const auto breakpointId = bp.id.value();

        auto fit = std::find_if(fileBreakpoints.begin(), fileBreakpoints.end(), [breakpointId](const dap::Breakpoint &n) {
            return n.id == breakpointId;
        });
        // add as a new breakpoint
        if (fit == fileBreakpoints.end()) {
            addNewBreakpoint(bp);
            return;
        }

        if (*fit == bp) {
            qDebug() << "ignoring no change";
            return;
        }

        *fit = bp;

        const auto parent = index(fileIndex, 0, QModelIndex());
        const auto left = index(fit - fileBreakpoints.begin(), 0, parent);
        const auto right = index(fit - fileBreakpoints.begin(), columnCount({}), parent);

        dataChanged(left, right);

        if (bp.line) {
            Q_EMIT breakpointChanged(url, bp.line.value(), BackendInterface::BreakpointEventKind::Changed);
        }
    }

    void onBreakpointRemoved(const dap::Breakpoint &bp)
    {
        if (!bp.source || !bp.id) {
            // ignore if no source or id
            return;
        }
        const auto url = bp.source.value().path;
        auto it = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [url](const FileBreakpoints &fp) {
            return url == fp.url;
        });

        if (it == m_breakpoints.end()) {
            return;
        }

        const int fileIndex = it - m_breakpoints.begin();
        auto &fileBreakpoints = m_breakpoints[fileIndex].breakpoints;

        const auto breakpointId = bp.id.value();

        auto fit = std::find_if(fileBreakpoints.begin(), fileBreakpoints.end(), [breakpointId](const dap::Breakpoint &n) {
            return n.id == breakpointId;
        });
        if (fit == fileBreakpoints.end()) {
            return;
        }

        const auto parent = index(fileIndex, 0, QModelIndex());
        const auto index = fit - fileBreakpoints.begin();
        beginRemoveRows(parent, index, index);
        fileBreakpoints.clear();
        endRemoveRows();

        if (bp.line) {
            Q_EMIT breakpointChanged(url, bp.line.value(), BackendInterface::BreakpointEventKind::Removed);
        }
    }

    std::map<QUrl, QList<dap::SourceBreakpoint>> allBreakpoints() const
    {
        std::map<QUrl, QList<dap::SourceBreakpoint>> ret;
        for (const auto &fileBreakpoints : m_breakpoints) {
            ret[fileBreakpoints.url] = toSourceBreakpoints(fileBreakpoints.breakpoints);
        }
        return ret;
    }

private:
    struct FileBreakpoints {
        QUrl url;
        QList<dap::Breakpoint> breakpoints;
    };
    QList<FileBreakpoints> m_breakpoints;

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
}

void BreakpointModel::toggleBreakpoint(const QUrl &url, int line)
{
    auto it = std::find_if(m_breakpoints.begin(), m_breakpoints.end(), [url](const FileBreakpoints &fp) {
        return url == fp.url;
    });

    int fileIndex = 0;
    if (it == m_breakpoints.end()) {
        fileIndex = m_breakpoints.size();

        beginInsertRows(QModelIndex(), fileIndex, fileIndex);
        m_breakpoints.push_back(FileBreakpoints{.url = url, .breakpoints = {}});
        endInsertRows();
    } else {
        fileIndex = it - m_breakpoints.begin();
    }

    auto &fileBreakpoints = m_breakpoints[fileIndex].breakpoints;

    auto fit = std::find_if(fileBreakpoints.begin(), fileBreakpoints.end(), [line](const dap::Breakpoint &n) {
        return n.line.has_value() && n.line.value() == line;
    });

    if (fit != fileBreakpoints.end()) {
        const int pos = static_cast<int>(fit - fileBreakpoints.begin());

        const auto parent = index(fileIndex, 0, QModelIndex());
        beginRemoveRows(parent, pos, pos);
        fileBreakpoints.erase(fit);
        endRemoveRows();

        qCDebug(kateBreakpoint, "removed breakpoint %ls:%d", qUtf16Printable(url.toLocalFile()), line);

        if (fileBreakpoints.empty()) {
            // remove this path
            beginRemoveRows(QModelIndex(), fileIndex, fileIndex);
            m_breakpoints.remove(fileIndex);
            endRemoveRows();
        }
    } else {
        const auto parent = index(fileIndex, 0, QModelIndex());

        dap::Breakpoint bp;
        bp.source = dap::Source(url);
        bp.line = line;

        beginInsertRows(parent, fileBreakpoints.size(), fileBreakpoints.size());
        fileBreakpoints.push_back(bp);
        endInsertRows();

        qCDebug(kateBreakpoint, "added breakpoint %ls:%d", qUtf16Printable(url.toLocalFile()), line);
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
    m_treeview->setItemsExpandable(true);
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
