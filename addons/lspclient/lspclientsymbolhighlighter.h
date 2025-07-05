/*
 *    SPDX-FileCopyrightText: 2025 Yuri Timenkov <yuri@timenkov.pro>
 *
 *    SPDX-License-Identifier: MIT
 */

#pragma once

#include "lspclientserver.h"

#include <KTextEditor/Attribute>

namespace KTextEditor
{
class Cursor;
class Editor;
class MovingRange;
class View;
}
class KActionCollection;

// Highlights all occurrences of symbol under the cursor
class LSPClientSymbolHighlighter : public QObject
{
public:
    explicit LSPClientSymbolHighlighter(KActionCollection *actions);

    void attach(KTextEditor::View *view, std::shared_ptr<LSPClientServer> server);

private:
    using MovingRangeList = std::vector<std::unique_ptr<KTextEditor::MovingRange>>;

    void cursorPositionChanged(KTextEditor::View *view, const KTextEditor::Cursor &newPosition);
    void themeChange(KTextEditor::Editor *e);

    void highlight();
    void rangesInvalidated();
    void cancelRequest();

    void gotoNextHighlight();
    void gotoPrevHighlight();

    MovingRangeList::iterator findRange(const KTextEditor::Cursor &position);
    void goToRange(const KTextEditor::MovingRange &range);

private:
    KTextEditor::Attribute::Ptr m_highlightAttribute;
    QAction *m_nextSymbolHighlight;
    QAction *m_prevSymbolHighlight;

    QPointer<KTextEditor::View> m_currentView;
    std::shared_ptr<LSPClientServer> m_currentServer;

    KTextEditor::Range m_currentWord;
    MovingRangeList m_ranges;

    QTimer m_highlightDelayTimer;
    QTimer m_requestTimeout;
    LSPClientServer::RequestHandle m_requestHandle;
};
