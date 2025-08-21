/*
 *    SPDX-FileCopyrightText: 2025 Yuri Timenkov <yuri@timenkov.pro>
 *
 *    SPDX-License-Identifier: MIT
 */

#include "lspclientsymbolhighlighter.h"

#include "lspclientservermanager.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

LSPClientSymbolHighlighter::LSPClientSymbolHighlighter(KActionCollection *actions)
    : m_highlightAttribute(new KTextEditor::Attribute())
{
    m_highlightDelayTimer.setSingleShot(true);
    m_highlightDelayTimer.setInterval(100);
    connect(&m_highlightDelayTimer, &QTimer::timeout, this, &LSPClientSymbolHighlighter::highlight);

    m_requestTimeout.setSingleShot(true);
    m_requestTimeout.setInterval(1000);
    connect(&m_requestTimeout, &QTimer::timeout, this, &LSPClientSymbolHighlighter::cancelRequest);

    m_nextSymbolHighlight = actions->addAction(QStringLiteral("lspclient_next_symbol_highlight"), this, &LSPClientSymbolHighlighter::gotoNextHighlight);
    m_nextSymbolHighlight->setText(i18nc("@action", "Go to next symbol highlight"));
    m_prevSymbolHighlight = actions->addAction(QStringLiteral("lspclient_prev_symbol_highlight"), this, &LSPClientSymbolHighlighter::gotoPrevHighlight);
    m_prevSymbolHighlight->setText(i18nc("@action", "Go to previous symbol highlight"));

    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, &LSPClientSymbolHighlighter::themeChange);
    themeChange(KTextEditor::Editor::instance());
}

void LSPClientSymbolHighlighter::attach(KTextEditor::View *view, std::shared_ptr<LSPClientServer> server)
{
    bool actionsEnabled;
    if (!view || !server) {
        m_currentView = nullptr;
        m_currentServer = nullptr;
        actionsEnabled = false;
    } else {
        m_currentView = view;
        m_currentServer = std::move(server);
        actionsEnabled = true;
        connect(view, &KTextEditor::View::cursorPositionChanged, this, &LSPClientSymbolHighlighter::cursorPositionChanged, Qt::UniqueConnection);
        connect(view, &KTextEditor::View::selectionChanged, this, &LSPClientSymbolHighlighter::rangesInvalidated, Qt::UniqueConnection);
    }
    m_nextSymbolHighlight->setEnabled(actionsEnabled);
    m_prevSymbolHighlight->setEnabled(actionsEnabled);

    rangesInvalidated();
}

void LSPClientSymbolHighlighter::cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor &newPosition)
{
    if (!m_currentWord.contains(newPosition)) {
        rangesInvalidated();
    }
}

void LSPClientSymbolHighlighter::rangesInvalidated()
{
    m_ranges.clear();
    m_currentWord = KTextEditor::Range::invalid();
    cancelRequest();

    if (m_currentView && !m_currentView->selection())
        m_highlightDelayTimer.start();
    else
        m_highlightDelayTimer.stop();
}

void LSPClientSymbolHighlighter::themeChange(KTextEditor::Editor *e)
{
    const auto theme = e->theme();
    m_highlightAttribute->setBackground(QBrush(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight)));
}

void LSPClientSymbolHighlighter::cancelRequest()
{
    m_requestHandle.cancel() = {};
}

void LSPClientSymbolHighlighter::highlight()
{
    if (!m_currentView || !m_currentServer)
        return;

    m_requestTimeout.start();
    m_requestHandle.cancel() =
        m_currentServer->documentHighlight(m_currentView->document()->url(),
                                           m_currentView->cursorPosition(),
                                           this,
                                           [this](const QList<LSPDocumentHighlight> &locations) {
                                               // By the time we get response view may change.
                                               if (!m_currentView)
                                                   return;

                                               auto document = m_currentView->document();
                                               m_ranges.resize(locations.length());
                                               auto it = m_ranges.begin();

                                               for (const auto &location : locations) {
                                                   auto mr = std::unique_ptr<KTextEditor::MovingRange>(document->newMovingRange(location.range));
                                                   mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
                                                   mr->setAttribute(m_highlightAttribute);
                                                   mr->setAttributeOnlyForViews(true);
                                                   *it = std::move(mr);
                                                   it++;
                                               }

                                               if (auto it = findRange(m_currentView->cursorPosition()); it != m_ranges.end()) {
                                                   m_currentWord = **it;
                                               }
                                           });
}

LSPClientSymbolHighlighter::MovingRangeList::iterator LSPClientSymbolHighlighter::findRange(const KTextEditor::Cursor &position)
{
    return std::find_if(m_ranges.begin(), m_ranges.end(), [&position](auto &range) {
        return range->contains(position);
    });
}

void LSPClientSymbolHighlighter::gotoNextHighlight()
{
    if (auto it = findRange(m_currentWord.start()); it != m_ranges.end()) {
        if (++it == m_ranges.end()) {
            it = m_ranges.begin();
        }
        goToRange(**it);
    }
}

void LSPClientSymbolHighlighter::gotoPrevHighlight()
{
    if (auto it = findRange(m_currentWord.start()); it != m_ranges.end()) {
        if (it == m_ranges.begin()) {
            it = m_ranges.end();
        }
        --it;

        goToRange(**it);
    }
}

void LSPClientSymbolHighlighter::goToRange(const KTextEditor::MovingRange &range)
{
    if (!m_currentView)
        return;

    // Update the current word position to skip new query to LSP
    // when signal is received.
    m_currentWord = range;
    m_currentView->setCursorPosition(range.start().toCursor());
}
