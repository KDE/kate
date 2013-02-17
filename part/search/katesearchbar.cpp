/* This file is part of the KDE libraries
   Copyright (C) 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>
   Copyright (C) 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>
   Copyright (C) 2007 Thomas Friedrichsmeier <thomas.friedrichsmeier@ruhr-uni-bochum.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesearchbar.h"
#include "katesearchbar.moc"

#include "kateregexp.h"
#include "katematch.h"
#include "kateview.h"
#include "katedocument.h"
#include "kateundomanager.h"
#include "kateconfig.h"
#include "katerenderer.h"

#include <ktexteditor/movingcursor.h>
#include <ktexteditor/movingrange.h>
#include <kte5/messageinterface.h> // KDE5 rename kte5 to ktexteditor

#include "ui_searchbarincremental.h"
#include "ui_searchbarpower.h"

#include <kcolorscheme.h>
#include <kstandardaction.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QShortcut>

// Turn debug messages on/off here
// #define FAST_DEBUG_ENABLE

#ifdef FAST_DEBUG_ENABLE
# define FAST_DEBUG(x) kDebug() << x
#else
# define FAST_DEBUG(x)
#endif

using namespace KTextEditor;

namespace {

class AddMenuManager {

private:
    QVector<QString> m_insertBefore;
    QVector<QString> m_insertAfter;
    QSet<QAction *> m_actionPointers;
    uint m_indexWalker;
    QMenu * m_menu;

public:
    AddMenuManager(QMenu * parent, int expectedItemCount)
            : m_insertBefore(QVector<QString>(expectedItemCount)),
            m_insertAfter(QVector<QString>(expectedItemCount)),
            m_indexWalker(0),
            m_menu(NULL) {
        Q_ASSERT(parent != NULL);
        m_menu = parent->addMenu(i18n("Add..."));
        if (m_menu == NULL) {
            return;
        }
        m_menu->setIcon(KIcon("list-add"));
    }

    void enableMenu(bool enabled) {
        if (m_menu == NULL) {
            return;
        }
        m_menu->setEnabled(enabled);
    }

    void addEntry(const QString & before, const QString after,
            const QString description, const QString & realBefore = QString(),
            const QString & realAfter = QString()) {
        if (m_menu == NULL) {
            return;
        }
        QAction * const action = m_menu->addAction(before + after + '\t' + description);
        m_insertBefore[m_indexWalker] = QString(realBefore.isEmpty() ? before : realBefore);
        m_insertAfter[m_indexWalker] = QString(realAfter.isEmpty() ? after : realAfter);
        action->setData(QVariant(m_indexWalker++));
        m_actionPointers.insert(action);
    }

    void addSeparator() {
        if (m_menu == NULL) {
            return;
        }
        m_menu->addSeparator();
    }

    void handle(QAction * action, QLineEdit * lineEdit) {
        if (!m_actionPointers.contains(action)) {
            return;
        }

        const int cursorPos = lineEdit->cursorPosition();
        const int index = action->data().toUInt();
        const QString & before = m_insertBefore[index];
        const QString & after = m_insertAfter[index];
        lineEdit->insert(before + after);
        lineEdit->setCursorPosition(cursorPos + before.count());
        lineEdit->setFocus();
    }
};

} // anon namespace



KateSearchBar::KateSearchBar(bool initAsPower, KateView* view, KateViewConfig *config)
        : KateViewBarWidget(true, view),
        m_view(view),
        m_config(config),
        m_layout(new QVBoxLayout()),
        m_widget(NULL),
        m_incUi(NULL),
        m_incInitCursor(view->cursorPosition()),
        m_powerUi(NULL),
        highlightMatchAttribute (new Attribute()),
        highlightReplacementAttribute (new Attribute()),
        m_incHighlightAll(false),
        m_incFromCursor(true),
        m_incMatchCase(false),
        m_powerMatchCase(true),
        m_powerFromCursor(false),
        m_powerHighlightAll(false),
        m_powerMode(0)
{

    connect(view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)),
            this, SLOT(updateIncInitCursor()));

    // init match attribute
    Attribute::Ptr mouseInAttribute(new Attribute());
    mouseInAttribute->setFontBold(true);
    highlightMatchAttribute->setDynamicAttribute (Attribute::ActivateMouseIn, mouseInAttribute);

    Attribute::Ptr caretInAttribute(new Attribute());
    caretInAttribute->setFontItalic(true);
    highlightMatchAttribute->setDynamicAttribute (Attribute::ActivateCaretIn, caretInAttribute);

    updateHighlightColors();

    // Modify parent
    QWidget * const widget = centralWidget();
    widget->setLayout(m_layout);
    m_layout->setMargin(0);

    // allow to have small size, for e.g. Kile
    setMinimumWidth (100);

    // Copy global to local config backup
    const long searchFlags = m_config->searchFlags();
    m_incHighlightAll = (searchFlags & KateViewConfig::IncHighlightAll) != 0;
    m_incFromCursor = (searchFlags & KateViewConfig::IncFromCursor) != 0;
    m_incMatchCase = (searchFlags & KateViewConfig::IncMatchCase) != 0;
    m_powerMatchCase = (searchFlags & KateViewConfig::PowerMatchCase) != 0;
    m_powerFromCursor = (searchFlags & KateViewConfig::PowerFromCursor) != 0;
    m_powerHighlightAll = (searchFlags & KateViewConfig::PowerHighlightAll) != 0;
    m_powerMode = ((searchFlags & KateViewConfig::PowerModeRegularExpression) != 0)
            ? MODE_REGEX
            : (((searchFlags & KateViewConfig::PowerModeEscapeSequences) != 0)
                ? MODE_ESCAPE_SEQUENCES
                : (((searchFlags & KateViewConfig::PowerModeWholeWords) != 0)
                    ? MODE_WHOLE_WORDS
                    : MODE_PLAIN_TEXT));


    // Load one of either dialogs
    if (initAsPower) {
        enterPowerMode();
    } else {
        enterIncrementalMode();
    }

    updateSelectionOnly();
    connect(view, SIGNAL(selectionChanged(KTextEditor::View*)),
            this, SLOT(updateSelectionOnly()));
}



KateSearchBar::~KateSearchBar() {
    clearHighlights();
    delete m_layout;
    delete m_widget;

    delete m_incUi;
    delete m_powerUi;
}

void KateSearchBar::closed()
{
    // remove search from the view bar, because it vertically bloats up the
    // stacked layout in KateViewBar.
    if (viewBar()) {
        viewBar()->removeBarWidget(this);
    }
}


void KateSearchBar::setReplacementPattern(const QString &replacementPattern) {
    Q_ASSERT(isPower());

    if (this->replacementPattern() == replacementPattern)
        return;

    m_powerUi->replacement->setEditText(replacementPattern);
}



QString KateSearchBar::replacementPattern() const {
    Q_ASSERT(isPower());

    return m_powerUi->replacement->currentText();
}



void KateSearchBar::setSearchMode(KateSearchBar::SearchMode mode) {
    Q_ASSERT(isPower());

    m_powerUi->searchMode->setCurrentIndex(mode);
}



void KateSearchBar::findNext() {
    const bool found = find();

    if (found) {
        QComboBox *combo = m_powerUi != 0 ? m_powerUi->pattern : m_incUi->pattern;

        // Add to search history
        addCurrentTextToHistory(combo);
    }
}



void KateSearchBar::findPrevious() {
    const bool found = find(SearchBackward);

    if (found) {
        QComboBox *combo = m_powerUi != 0 ? m_powerUi->pattern : m_incUi->pattern;

        // Add to search history
        addCurrentTextToHistory(combo);
    }
}

void KateSearchBar::showInfoMessage(const QString& text)
{
    delete m_infoMessage;

    m_infoMessage = new KTextEditor::Message(KTextEditor::Message::Positive, text);
    m_infoMessage->setPosition(KTextEditor::Message::BelowView);
    m_infoMessage->setAutoHide(0);
    m_infoMessage->setView(m_view);

    QAction* closeAction = new QAction(KIcon("window-close"), i18n("&Close"), 0);
    closeAction->setToolTip(i18n("Close message (Escape)"));

    QAction* hlAction = new QAction(i18n("&Keep highlighting"), 0);
    hlAction->setToolTip(i18n("Keep search and replace highlighting marks"));

    m_infoMessage->addAction(hlAction);
    m_infoMessage->addAction(closeAction);

    // the closed() signal is emitted in the Message::destructor, clearHighlights()
    // calls delete m_infoMessage, which leads to a double delete. Thus, use a
    // Qt::QueuedConnection.
    connect(m_infoMessage, SIGNAL(closed(KTextEditor::Message*)), SLOT(clearHighlights()), Qt::QueuedConnection);
    connect(hlAction, SIGNAL(triggered()), SLOT(keepHighlights()));

    m_view->doc()->postMessage(m_infoMessage);
}

void KateSearchBar::keepHighlights()
{
    if (m_infoMessage) {
        // kill the Message::closed() <-> clearHighlights() connection
        disconnect(m_infoMessage, 0, this, 0);
    }
}

void KateSearchBar::highlightMatch(const Range & range) {
    KTextEditor::MovingRange* const highlight = m_view->doc()->newMovingRange(range, Kate::TextRange::DoNotExpand);
    highlight->setView(m_view); // show only in this view
    highlight->setAttributeOnlyForViews(true);
    // use z depth defined in moving ranges interface
    highlight->setZDepth (-10000.0);
    highlight->setAttribute(highlightMatchAttribute);
    m_hlRanges.append(highlight);
}

void KateSearchBar::highlightReplacement(const Range & range) {
    KTextEditor::MovingRange* const highlight = m_view->doc()->newMovingRange(range, Kate::TextRange::DoNotExpand);
    highlight->setView(m_view); // show only in this view
    highlight->setAttributeOnlyForViews(true);
    // use z depth defined in moving ranges interface
    highlight->setZDepth (-10000.0);
    highlight->setAttribute(highlightReplacementAttribute);
    m_hlRanges.append(highlight);
}

void KateSearchBar::indicateMatch(MatchResult matchResult) {
    QLineEdit * const lineEdit = isPower() ? m_powerUi->pattern->lineEdit()
                                           : m_incUi->pattern->lineEdit();
    QPalette background(lineEdit->palette());

    switch (matchResult) {
    case MatchFound:  // FALLTHROUGH
    case MatchWrappedForward:
    case MatchWrappedBackward:
        // Green background for line edit
        KColorScheme::adjustBackground(background, KColorScheme::PositiveBackground);
        break;
    case MatchMismatch:
        // Red background for line edit
        KColorScheme::adjustBackground(background, KColorScheme::NegativeBackground);
        break;
    case MatchNothing:
        // Reset background of line edit
        background = QPalette();
        break;
    case MatchNeutral:
        KColorScheme::adjustBackground(background, KColorScheme::NeutralBackground);
        break;
    }

    // Update status label
    if (m_incUi != NULL) {
        QPalette foreground(m_incUi->status->palette());
        switch (matchResult) {
        case MatchFound: // FALLTHROUGH
        case MatchNothing:
            KColorScheme::adjustForeground(foreground, KColorScheme::NormalText, QPalette::WindowText, KColorScheme::Window);
            m_incUi->status->clear();
            break;
        case MatchWrappedForward:
        case MatchWrappedBackward:
            KColorScheme::adjustForeground(foreground, KColorScheme::NormalText, QPalette::WindowText, KColorScheme::Window);
            if (matchResult == MatchWrappedBackward) {
                m_incUi->status->setText(i18n("Reached top, continued from bottom"));
            } else {
                m_incUi->status->setText(i18n("Reached bottom, continued from top"));
            }
            break;
        case MatchMismatch:
            KColorScheme::adjustForeground(foreground, KColorScheme::NegativeText, QPalette::WindowText, KColorScheme::Window);
            m_incUi->status->setText(i18n("Not found"));
            break;
        case MatchNeutral:
            /* do nothing */
            break;
        }
        m_incUi->status->setPalette(foreground);
    }

    lineEdit->setPalette(background);
}



/*static*/ void KateSearchBar::selectRange(KateView * view, const KTextEditor::Range & range) {
    view->setCursorPositionInternal(range.end());

    // don't make a selection if the vi input mode is used
    if (!view->viInputMode())
        view->setSelection(range);
}



void KateSearchBar::selectRange2(const KTextEditor::Range & range) {
    disconnect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(updateSelectionOnly()));
    selectRange(m_view, range);
    connect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(updateSelectionOnly()));
}



void KateSearchBar::onIncPatternChanged(const QString & pattern)
{
    if (!m_incUi)
        return;

    // clear prior highlightings (deletes info message if present)
    clearHighlights();

    m_incUi->next->setDisabled(pattern.isEmpty());
    m_incUi->prev->setDisabled(pattern.isEmpty());

    KateMatch match(m_view->doc(), searchOptions());

    if (!pattern.isEmpty()) {
        // Find, first try
        const Range inputRange = KTextEditor::Range(m_incInitCursor, m_view->document()->documentEnd());
        match.searchText(inputRange, pattern);
    }

    const bool wrap = !match.isValid() && !pattern.isEmpty();

    if (wrap) {
        // Find, second try
        const KTextEditor::Range inputRange = m_view->document()->documentRange();
        match.searchText(inputRange, pattern);
    }

    const MatchResult matchResult = match.isValid()   ? (wrap ? MatchWrappedForward : MatchFound) :
                                    pattern.isEmpty() ? MatchNothing :
                                                        MatchMismatch;

    const Range selectionRange = pattern.isEmpty() ? Range(m_incInitCursor, m_incInitCursor) :
                                 match.isValid()   ? match.range() :
                                                     Range::invalid();

    // don't update m_incInitCursor when we move the cursor
    disconnect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)),
               this, SLOT(updateIncInitCursor()));
    selectRange2(selectionRange);
    connect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)),
            this, SLOT(updateIncInitCursor()));

    indicateMatch(matchResult);
}



void KateSearchBar::setMatchCase(bool matchCase) {
    if (this->matchCase() == matchCase)
        return;

    if (isPower())
        m_powerUi->matchCase->setChecked(matchCase);
    else
        m_incUi->matchCase->setChecked(matchCase);
}



void KateSearchBar::onMatchCaseToggled(bool /*matchCase*/) {
    sendConfig();

    if (m_incUi != 0) {
       // Re-search with new settings
        const QString pattern = m_incUi->pattern->currentText();
        onIncPatternChanged(pattern);
    } else {
        indicateMatch(MatchNothing);
    }
}



bool KateSearchBar::matchCase() const
{
    return isPower() ? m_powerUi->matchCase->isChecked()
                     : m_incUi->matchCase->isChecked();
}



void KateSearchBar::fixForSingleLine(Range & range, SearchDirection searchDirection) {
    FAST_DEBUG("Single-line workaround checking BEFORE" << range);
    if (searchDirection == SearchForward) {
        const int line = range.start().line();
        const int col = range.start().column();
        const int maxColWithNewline = m_view->document()->lineLength(line) + 1;
        if (col == maxColWithNewline) {
            FAST_DEBUG("Starting on a newline" << range);
            const int maxLine = m_view->document()->lines() - 1;
            if (line < maxLine) {
                range.setRange(Cursor(line + 1, 0), range.end());
                FAST_DEBUG("Search range fixed to " << range);
            } else {
                FAST_DEBUG("Already at last line");
                range = Range::invalid();
            }
        }
    } else {
        const int col = range.end().column();
        if (col == 0) {
            FAST_DEBUG("Ending after a newline" << range);
            const int line = range.end().line();
            if (line > 0) {
                const int maxColWithNewline = m_view->document()->lineLength(line - 1);
                range.setRange(range.start(), Cursor(line - 1, maxColWithNewline));
                FAST_DEBUG("Search range fixed to " << range);
            } else {
                FAST_DEBUG("Already at first line");
                range = Range::invalid();
            }
        }
    }
    FAST_DEBUG("Single-line workaround checking  AFTER" << range);
}



void KateSearchBar::onReturnPressed() {
    const Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    const bool shiftDown = (modifiers & Qt::ShiftModifier) != 0;
    const bool controlDown = (modifiers & Qt::ControlModifier) != 0;

    // if vi input mode is active, the search box should be closed when hitting enter
    if (m_view->viInputMode()) {
        emit hideMe();
        return;
    }

    if (shiftDown) {
        // Shift down, search backwards
        findPrevious();
    } else {
        // Shift up, search forwards
        findNext();
    }

    if (controlDown) {
        emit hideMe();
    }
}



bool KateSearchBar::find(SearchDirection searchDirection, const QString * replacement) {
    // What to find?
    if (searchPattern().isEmpty()) {
        return false; // == Pattern error
    }

    const Search::SearchOptions enabledOptions = searchOptions(searchDirection);

    // Where to find?
    Range inputRange;
    const Range selection = m_view->selection() ? m_view->selectionRange() : Range::invalid();
    if (selection.isValid()) {
        if (selectionOnly()) {
            // First match in selection
            inputRange = selection;
        } else {
            // Next match after/before selection if a match was selected before
            if (searchDirection == SearchForward) {
                inputRange.setRange(selection.start(), m_view->document()->documentEnd());
            } else {
                inputRange.setRange(Cursor(0, 0), selection.end());
            }
        }
    } else {
        // No selection
            const Cursor cursorPos = m_view->cursorPosition();
            if (searchDirection == SearchForward) {
                // if the vi input mode is used, the cursor will stay a the first character of the
                // matched pattern (no selection will be made), so the next search should start from
                // match column + 1
                if (!m_view->viInputMode()) {
                    inputRange.setRange(cursorPos, m_view->document()->documentEnd());
                } else {
                    inputRange.setRange(Cursor(cursorPos.line(), cursorPos.column()+1), m_view->document()->documentEnd());
                }
            } else {
                inputRange.setRange(Cursor(0, 0), cursorPos);
            }
    }
    FAST_DEBUG("Search range is" << inputRange);

    {
        const bool regexMode = enabledOptions.testFlag(Search::Regex);
        const bool multiLinePattern = regexMode ? KateRegExp(searchPattern()).isMultiLine() : false;

        // Single-line pattern workaround
        if (regexMode && !multiLinePattern) {
            fixForSingleLine(inputRange, searchDirection);
        }
    }

    KateMatch match(m_view->doc(), enabledOptions);
    Range afterReplace = Range::invalid();

    // Find, first try
    match.searchText(inputRange, searchPattern());
    if (match.isValid() && match.range() == selection) {
            // Same match again
            if (replacement != 0) {
                // Selection is match -> replace
                KTextEditor::MovingRange *smartInputRange = m_view->doc()->newMovingRange (inputRange, KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
                afterReplace = match.replace(*replacement, m_view->blockSelection());
                inputRange = *smartInputRange;
                delete smartInputRange;
            }

            if (!selectionOnly()) {
                // Find, second try after old selection
                if (searchDirection == SearchForward) {
                    const Cursor start = (replacement != 0) ? afterReplace.end() : selection.end();
                    inputRange.setRange(start, inputRange.end());
                } else {
                    const Cursor end = (replacement != 0) ? afterReplace.start() : selection.start();
                    inputRange.setRange(inputRange.start(), end);
                }
            }

            // Single-line pattern workaround
            fixForSingleLine(inputRange, searchDirection);

            match.searchText(inputRange, searchPattern());
    }

    const bool wrap = !match.isValid() && (!selection.isValid() || !selectionOnly());
    if (wrap) {
        inputRange = m_view->document()->documentRange();
        match.searchText(inputRange, searchPattern());
    }

    if (match.isValid()) {
        selectRange2(match.range());
    }

    const MatchResult matchResult = !match.isValid()                 ? MatchMismatch :
                                    !wrap                            ? MatchFound :
                                    searchDirection == SearchForward ? MatchWrappedForward :
                                                                       MatchWrappedBackward;
    indicateMatch(matchResult);

    // Reset highlighting for all matches and highlight replacement if there is one
    clearHighlights();
    if (afterReplace.isValid()) {
        highlightReplacement(afterReplace);
    }

    return true; // == No pattern error
}




void KateSearchBar::findAll()
{
    // clear highlightings of prior search&replace action
    clearHighlights();

    Range inputRange = (m_view->selection() && selectionOnly())
            ? m_view->selectionRange()
            : m_view->document()->documentRange();
    const int occurrences = findAll(inputRange, NULL);

    // send passive notification to view
    showInfoMessage(i18np("1 match found", "%1 matches found", occurrences));

    indicateMatch(occurrences > 0 ? MatchFound : MatchMismatch);
}



void KateSearchBar::onPowerPatternChanged(const QString & /*pattern*/) {
    givePatternFeedback();
    indicateMatch(MatchNothing);
}



bool KateSearchBar::isPatternValid() const {
    if (searchPattern().isEmpty())
        return false;

    return searchOptions().testFlag(Search::WholeWords) ? searchPattern().trimmed() == searchPattern() :
           searchOptions().testFlag(Search::Regex)      ? QRegExp(searchPattern()).isValid() :
                                                          true;
}



void KateSearchBar::givePatternFeedback() {
    // Enable/disable next/prev and replace next/all
    m_powerUi->findNext->setEnabled(isPatternValid());
    m_powerUi->findPrev->setEnabled(isPatternValid());
    m_powerUi->replaceNext->setEnabled(isPatternValid());
    m_powerUi->replaceAll->setEnabled(isPatternValid());
}



void KateSearchBar::addCurrentTextToHistory(QComboBox * combo) {
    const QString text = combo->currentText();
    const int index = combo->findText(text);

    if (index > 0)
        combo->removeItem(index);
    if (index != 0) {
        combo->insertItem(0, text);
        combo->setCurrentIndex(0);
    }
}



void KateSearchBar::backupConfig(bool ofPower) {
    if (ofPower) {
        m_powerMatchCase = m_powerUi->matchCase->isChecked();
        m_powerMode = m_powerUi->searchMode->currentIndex();
    } else {
        m_incMatchCase = m_incUi->matchCase->isChecked();
    }
}



void KateSearchBar::sendConfig() {
    const long pastFlags = m_config->searchFlags();
    long futureFlags = pastFlags;

    if (m_powerUi != NULL) {
        const bool OF_POWER = true;
        backupConfig(OF_POWER);

        // Update power search flags only
        const long incFlagsOnly = pastFlags
                & (KateViewConfig::IncHighlightAll
                    | KateViewConfig::IncFromCursor
                    | KateViewConfig::IncMatchCase);

        futureFlags = incFlagsOnly
            | (m_powerMatchCase ? KateViewConfig::PowerMatchCase : 0)
            | (m_powerFromCursor ? KateViewConfig::PowerFromCursor : 0)
            | (m_powerHighlightAll ? KateViewConfig::PowerHighlightAll : 0)
            | ((m_powerMode == MODE_REGEX)
                ? KateViewConfig::PowerModeRegularExpression
                : ((m_powerMode == MODE_ESCAPE_SEQUENCES)
                    ? KateViewConfig::PowerModeEscapeSequences
                    : ((m_powerMode == MODE_WHOLE_WORDS)
                        ? KateViewConfig::PowerModeWholeWords
                        : KateViewConfig::PowerModePlainText)));

    } else if (m_incUi != NULL) {
        const bool OF_INCREMENTAL = false;
        backupConfig(OF_INCREMENTAL);

        // Update incremental search flags only
        const long powerFlagsOnly = pastFlags
                & (KateViewConfig::PowerMatchCase
                    | KateViewConfig::PowerFromCursor
                    | KateViewConfig::PowerHighlightAll
                    | KateViewConfig::PowerModeRegularExpression
                    | KateViewConfig::PowerModeEscapeSequences
                    | KateViewConfig::PowerModeWholeWords
                    | KateViewConfig::PowerModePlainText);

        futureFlags = powerFlagsOnly
                | (m_incHighlightAll ? KateViewConfig::IncHighlightAll : 0)
                | (m_incFromCursor ? KateViewConfig::IncFromCursor : 0)
                | (m_incMatchCase ? KateViewConfig::IncMatchCase : 0);
    }

    // Adjust global config
    m_config->setSearchFlags(futureFlags);
}



void KateSearchBar::replaceNext() {
    const QString replacement = m_powerUi->replacement->currentText();

    if (find(SearchForward, &replacement)) {
        // Never merge replace actions with other replace actions/user actions
        m_view->doc()->undoManager()->undoSafePoint();

        // Add to search history
        addCurrentTextToHistory(m_powerUi->pattern);

        // Add to replace history
        addCurrentTextToHistory(m_powerUi->replacement);
    }
}



// replacement == NULL --> Highlight all matches
// replacement != NULL --> Replace and highlight all matches
int KateSearchBar::findAll(Range inputRange, const QString * replacement)
{
    const Search::SearchOptions enabledOptions = searchOptions(SearchForward);

    const bool regexMode = enabledOptions.testFlag(Search::Regex);
    const bool multiLinePattern = regexMode ? KateRegExp(searchPattern()).isMultiLine() : false;

    KTextEditor::MovingRange * workingRange = m_view->doc()->newMovingRange(inputRange);
    QList<Range> highlightRanges;
    int matchCounter = 0;

    bool block = m_view->selection() && m_view->blockSelection();
    int line = inputRange.start().line();
    do {
        if (block)
            workingRange = m_view->doc()->newMovingRange(m_view->doc()->rangeOnLine(inputRange, line));

        for (;;) {
            KateMatch match(m_view->doc(), enabledOptions);
            match.searchText(*workingRange, searchPattern());
            if (!match.isValid()) {
                break;
            }
            bool const originalMatchEmpty = match.isEmpty();

            // Work with the match
            if (replacement != NULL) {
                if (matchCounter == 0) {
                    m_view->document()->startEditing();
                }

                // Replace
                const Range afterReplace = match.replace(*replacement, false, ++matchCounter);

                // Highlight and continue after adjusted match
                //highlightReplacement(*afterReplace);
                highlightRanges << afterReplace;
            } else {
                // Highlight and continue after original match
                //highlightMatch(match);
                highlightRanges << match.range();
                matchCounter++;
            }

            // Continue after match
            if (highlightRanges.last().end() >= workingRange->end())
                break;
            KTextEditor::MovingCursor* workingStart =
                static_cast<KateDocument*>(m_view->document())->newMovingCursor(highlightRanges.last().end());
            if (originalMatchEmpty) {
                // Can happen for regex patterns like "^".
                // If we don't advance here we will loop forever...
                workingStart->move(1);
            } else if (regexMode && !multiLinePattern && workingStart->atEndOfLine()) {
                // single-line regexps might match the naked line end
                // therefore we better advance to the next line
                workingStart->move(1);
            }
            workingRange->setRange(*workingStart, workingRange->end());

            const bool atEndOfDocument = workingStart->atEndOfDocument();
            delete workingStart;
            // Are we done?
            if (!workingRange->toRange().isValid() || atEndOfDocument) {
                break;
            }
        }

    } while (block && ++line <= inputRange.end().line());

    // After last match
    if (matchCounter > 0) {
        if (replacement != NULL) {
            m_view->document()->endEditing();
        }
    }

    if (replacement == NULL)
        foreach (const Range &r, highlightRanges) {
            highlightMatch(r);
        }
    else
        foreach (const Range &r, highlightRanges) {
            highlightReplacement(r);
        }

    delete workingRange;

    return matchCounter;
}



void KateSearchBar::replaceAll()
{
    // clear prior highlightings (deletes info message if present)
    clearHighlights();

    // What to find/replace?
    const QString replacement = m_powerUi->replacement->currentText();

    // Where to replace?
    Range selection;
    const bool selected = m_view->selection();
    Range inputRange = (selected && selectionOnly())
            ? m_view->selectionRange()
            : m_view->document()->documentRange();


    // Pass on the hard work
    int replacementsDone=findAll(inputRange, &replacement);

    // send passive notification to view
    showInfoMessage(i18np("1 replacement has been made", "%1 replacements have been made", replacementsDone));

    // Never merge replace actions with other replace actions/user actions
    m_view->doc()->undoManager()->undoSafePoint();

    // Add to search history
    addCurrentTextToHistory(m_powerUi->pattern);

    // Add to replace history
    addCurrentTextToHistory(m_powerUi->replacement);
}



void KateSearchBar::setSearchPattern(const QString &searchPattern)
{
    if (searchPattern == this->searchPattern())
        return;

    if (isPower())
        m_powerUi->pattern->setEditText(searchPattern);
    else
        m_incUi->pattern->setEditText(searchPattern);
}



QString KateSearchBar::searchPattern() const {
    return (m_powerUi != 0) ? m_powerUi->pattern->currentText()
                            : m_incUi->pattern->currentText();
}



void KateSearchBar::setSelectionOnly(bool selectionOnly)
{
    if (this->selectionOnly() == selectionOnly)
        return;

    if (isPower())
        m_powerUi->selectionOnly->setChecked(selectionOnly);
}




bool KateSearchBar::selectionOnly() const {
    return isPower() ? m_powerUi->selectionOnly->isChecked()
                     : false;
}



KTextEditor::Search::SearchOptions KateSearchBar::searchOptions(SearchDirection searchDirection) const {
    Search::SearchOptions enabledOptions = KTextEditor::Search::Default;

    if (!matchCase()) {
        enabledOptions |= Search::CaseInsensitive;
    }

    if (searchDirection == SearchBackward) {
        enabledOptions |= Search::Backwards;
    }

    if (m_powerUi != NULL) {
        switch (m_powerUi->searchMode->currentIndex()) {
        case MODE_WHOLE_WORDS:
            enabledOptions |= Search::WholeWords;
            break;

        case MODE_ESCAPE_SEQUENCES:
            enabledOptions |= Search::EscapeSequences;
            break;

        case MODE_REGEX:
            enabledOptions |= Search::Regex;
            break;

        case MODE_PLAIN_TEXT: // FALLTHROUGH
        default:
            break;

        }
    }

    return enabledOptions;
}




struct ParInfo {
    int openIndex;
    bool capturing;
    int captureNumber; // 1..9
};



QVector<QString> KateSearchBar::getCapturePatterns(const QString & pattern) const {
    QVector<QString> capturePatterns;
    capturePatterns.reserve(9);
    QStack<ParInfo> parInfos;

    const int inputLen = pattern.length();
    int input = 0; // walker index
    bool insideClass = false;
    int captureCount = 0;

    while (input < inputLen) {
        if (insideClass) {
            // Wait for closing, unescaped ']'
            if (pattern[input].unicode() == L']') {
                insideClass = false;
            }
            input++;
        }
        else
        {
            switch (pattern[input].unicode())
            {
            case L'\\':
                // Skip this and any next character
                input += 2;
                break;

            case L'(':
                ParInfo curInfo;
                curInfo.openIndex = input;
                curInfo.capturing = (input + 1 >= inputLen) || (pattern[input + 1].unicode() != '?');
                if (curInfo.capturing) {
                    captureCount++;
                }
                curInfo.captureNumber = captureCount;
                parInfos.push(curInfo);

                input++;
                break;

            case L')':
                if (!parInfos.empty()) {
                    ParInfo & top = parInfos.top();
                    if (top.capturing && (top.captureNumber <= 9)) {
                        const int start = top.openIndex + 1;
                        const int len = input - start;
                        if (capturePatterns.size() < top.captureNumber) {
                            capturePatterns.resize(top.captureNumber);
                        }
                        capturePatterns[top.captureNumber - 1] = pattern.mid(start, len);
                    }
                    parInfos.pop();
                }

                input++;
                break;

            case L'[':
                input++;
                insideClass = true;
                break;

            default:
                input++;
                break;

            }
        }
    }

    return capturePatterns;
}



void KateSearchBar::showExtendedContextMenu(bool forPattern, const QPoint& pos) {
    // Make original menu
    QComboBox* comboBox = forPattern ? m_powerUi->pattern : m_powerUi->replacement;
    QMenu* const contextMenu = comboBox->lineEdit()->createStandardContextMenu();

    if (contextMenu == NULL) {
        return;
    }

    bool extendMenu = false;
    bool regexMode = false;
    switch (m_powerUi->searchMode->currentIndex()) {
    case MODE_REGEX:
        regexMode = true;
        // FALLTHROUGH

    case MODE_ESCAPE_SEQUENCES:
        extendMenu = true;
        break;

    default:
        break;
    }

    AddMenuManager addMenuManager(contextMenu, 37);
    if (!extendMenu) {
        addMenuManager.enableMenu(extendMenu);
    } else {
        // Build menu
        if (forPattern) {
            if (regexMode) {
                addMenuManager.addEntry("^", "", i18n("Beginning of line"));
                addMenuManager.addEntry("$", "", i18n("End of line"));
                addMenuManager.addSeparator();
                addMenuManager.addEntry(".", "", i18n("Any single character (excluding line breaks)"));
                addMenuManager.addSeparator();
                addMenuManager.addEntry("+", "", i18n("One or more occurrences"));
                addMenuManager.addEntry("*", "", i18n("Zero or more occurrences"));
                addMenuManager.addEntry("?", "", i18n("Zero or one occurrences"));
                addMenuManager.addEntry("{a", ",b}", i18n("<a> through <b> occurrences"), "{", ",}");
                addMenuManager.addSeparator();
                addMenuManager.addEntry("(", ")", i18n("Group, capturing"));
                addMenuManager.addEntry("|", "", i18n("Or"));
                addMenuManager.addEntry("[", "]", i18n("Set of characters"));
                addMenuManager.addEntry("[^", "]", i18n("Negative set of characters"));
                addMenuManager.addSeparator();
            }
        } else {
            addMenuManager.addEntry("\\0", "", i18n("Whole match reference"));
            addMenuManager.addSeparator();
            if (regexMode) {
                const QString pattern = m_powerUi->pattern->currentText();
                const QVector<QString> capturePatterns = getCapturePatterns(pattern);

                const int captureCount = capturePatterns.count();
                for (int i = 1; i <= 9; i++) {
                    const QString number = QString::number(i);
                    const QString & captureDetails = (i <= captureCount)
                            ? (QString(" = (") + capturePatterns[i - 1].left(30)) + QString(")")
                            : QString();
                    addMenuManager.addEntry("\\" + number, "",
                            i18n("Reference") + ' ' + number + captureDetails);
                }

                addMenuManager.addSeparator();
            }
        }

        addMenuManager.addEntry("\\n", "", i18n("Line break"));
        addMenuManager.addEntry("\\t", "", i18n("Tab"));

        if (forPattern && regexMode) {
            addMenuManager.addEntry("\\b", "", i18n("Word boundary"));
            addMenuManager.addEntry("\\B", "", i18n("Not word boundary"));
            addMenuManager.addEntry("\\d", "", i18n("Digit"));
            addMenuManager.addEntry("\\D", "", i18n("Non-digit"));
            addMenuManager.addEntry("\\s", "", i18n("Whitespace (excluding line breaks)"));
            addMenuManager.addEntry("\\S", "", i18n("Non-whitespace (excluding line breaks)"));
            addMenuManager.addEntry("\\w", "", i18n("Word character (alphanumerics plus '_')"));
            addMenuManager.addEntry("\\W", "", i18n("Non-word character"));
        }

        addMenuManager.addEntry("\\0???", "", i18n("Octal character 000 to 377 (2^8-1)"), "\\0");
        addMenuManager.addEntry("\\x????", "", i18n("Hex character 0000 to FFFF (2^16-1)"), "\\x");
        addMenuManager.addEntry("\\\\", "", i18n("Backslash"));

        if (forPattern && regexMode) {
            addMenuManager.addSeparator();
            addMenuManager.addEntry("(?:E", ")", i18n("Group, non-capturing"), "(?:");
            addMenuManager.addEntry("(?=E", ")", i18n("Lookahead"), "(?=");
            addMenuManager.addEntry("(?!E", ")", i18n("Negative lookahead"), "(?!");
        }

        if (!forPattern) {
            addMenuManager.addSeparator();
            addMenuManager.addEntry("\\L", "", i18n("Begin lowercase conversion"));
            addMenuManager.addEntry("\\U", "", i18n("Begin uppercase conversion"));
            addMenuManager.addEntry("\\E", "", i18n("End case conversion"));
            addMenuManager.addEntry("\\l", "", i18n("Lowercase first character conversion"));
            addMenuManager.addEntry("\\u", "", i18n("Uppercase first character conversion"));
            addMenuManager.addEntry("\\#[#..]", "", i18n("Replacement counter (for Replace All)"), "\\#");
        }
    }

    // Show menu
    QAction * const result = contextMenu->exec(comboBox->mapToGlobal(pos));
    if (result != NULL) {
        addMenuManager.handle(result, comboBox->lineEdit());
    }
}



void KateSearchBar::onPowerModeChanged(int /*index*/) {
    if (m_powerUi->searchMode->currentIndex() == MODE_REGEX) {
        m_powerUi->matchCase->setChecked(true);
    }

    sendConfig();
    indicateMatch(MatchNothing);

    givePatternFeedback();
}



/*static*/ void KateSearchBar::nextMatchForSelection(KateView * view, SearchDirection searchDirection) {
    const bool selected = view->selection();
    if (selected) {
        const QString pattern = view->selectionText();

        // How to find?
        Search::SearchOptions enabledOptions(KTextEditor::Search::Default);
        if (searchDirection == SearchBackward) {
            enabledOptions |= Search::Backwards;
        }

        // Where to find?
        const Range selRange = view->selectionRange();
        Range inputRange;
        if (searchDirection == SearchForward) {
            inputRange.setRange(selRange.end(), view->doc()->documentEnd());
        } else {
            inputRange.setRange(Cursor(0, 0), selRange.start());
        }

        // Find, first try
        KateMatch match(view->doc(), enabledOptions);
        match.searchText(inputRange, pattern);

        if (match.isValid()) {
            selectRange(view, match.range());
        } else {
            // Find, second try
            if (searchDirection == SearchForward) {
                inputRange.setRange(Cursor(0, 0), selRange.start());
            } else {
                inputRange.setRange(selRange.end(), view->doc()->documentEnd());
            }
            KateMatch match2(view->doc(), enabledOptions);
            match2.searchText(inputRange, pattern);
            if (match2.isValid()) {
                selectRange(view, match2.range());
            }
        }
    } else {
        // Select current word so we can search for that the next time
        const Cursor cursorPos = view->cursorPosition();
        view->selectWord(cursorPos);
    }
}



void KateSearchBar::enterPowerMode() {
    QString initialPattern;
    bool selectionOnly = false;

    // Guess settings from context: init pattern with current selection
    const bool selected = m_view->selection();
    if (selected) {
        const Range & selection = m_view->selectionRange();
        if (selection.onSingleLine()) {
            // ... with current selection
            initialPattern = m_view->selectionText();
        } else {
            // Enable selection only
            selectionOnly = true;
        }
    }

    // If there's no new selection, we'll use the existing pattern
    if (initialPattern.isNull()) {
        // Coming from power search?
        const bool fromReplace = (m_powerUi != NULL) && (m_widget->isVisible());
        if (fromReplace) {
            QLineEdit * const patternLineEdit = m_powerUi->pattern->lineEdit();
            Q_ASSERT(patternLineEdit != NULL);
            patternLineEdit->selectAll();
            m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
            return;
        }

        // Coming from incremental search?
        const bool fromIncremental = (m_incUi != NULL) && (m_widget->isVisible());
        if (fromIncremental) {
            initialPattern = m_incUi->pattern->currentText();
        }
    }

    // Create dialog
    const bool create = (m_powerUi == NULL);
    if (create) {
        // Kill incremental widget
        if (m_incUi != NULL) {
            // Backup current settings
            const bool OF_INCREMENTAL = false;
            backupConfig(OF_INCREMENTAL);

            // Kill widget
            delete m_incUi;
            m_incUi = NULL;
            m_layout->removeWidget(m_widget);
            m_widget->deleteLater(); // I didn't get a crash here but for symmetrie to the other mutate slot^
        }

        // Add power widget
        m_widget = new QWidget(this);
        m_powerUi = new Ui::PowerSearchBar;
        m_powerUi->setupUi(m_widget);
        m_layout->addWidget(m_widget);

        // Bind to shared history models
        m_powerUi->pattern->setDuplicatesEnabled(false);
        m_powerUi->pattern->setInsertPolicy(QComboBox::InsertAtTop);
        m_powerUi->pattern->setMaxCount(m_config->maxHistorySize());
        m_powerUi->pattern->setModel(m_config->patternHistoryModel());
        m_powerUi->replacement->setDuplicatesEnabled(false);
        m_powerUi->replacement->setInsertPolicy(QComboBox::InsertAtTop);
        m_powerUi->replacement->setMaxCount(m_config->maxHistorySize());
        m_powerUi->replacement->setModel(m_config->replacementHistoryModel());

        // Icons
        m_powerUi->mutate->setIcon(KIcon("arrow-down-double"));
        m_powerUi->findNext->setIcon(KIcon("go-down-search"));
        m_powerUi->findPrev->setIcon(KIcon("go-up-search"));
        m_powerUi->findAll->setIcon(KIcon("edit-find"));

        // Focus proxy
        centralWidget()->setFocusProxy(m_powerUi->pattern);

        // Make completers case-sensitive
        m_powerUi->pattern->completionObject()->setIgnoreCase(false);
        m_powerUi->replacement->completionObject()->setIgnoreCase(false);
    }

    m_powerUi->selectionOnly->setChecked(selectionOnly);

    // Restore previous settings
    if (create) {
        m_powerUi->matchCase->setChecked(m_powerMatchCase);
        m_powerUi->searchMode->setCurrentIndex(m_powerMode);
    }

    // force current index of -1 --> <cursor down> shows 1st completion entry instead of 2nd
    m_powerUi->pattern->setCurrentIndex(-1);
    m_powerUi->replacement->setCurrentIndex(-1);

    // Set initial search pattern
    QLineEdit * const patternLineEdit = m_powerUi->pattern->lineEdit();
    Q_ASSERT(patternLineEdit != NULL);
    patternLineEdit->setText(initialPattern);
    patternLineEdit->selectAll();

    // Set initial replacement text
    QLineEdit * const replacementLineEdit = m_powerUi->replacement->lineEdit();
    Q_ASSERT(replacementLineEdit != NULL);
    replacementLineEdit->setText("");

    // Propagate settings (slots are still inactive on purpose)
    onPowerPatternChanged(initialPattern);
    givePatternFeedback();

    if (create) {
        // Slots
        connect(m_powerUi->mutate, SIGNAL(clicked()), this, SLOT(enterIncrementalMode()));
        connect(patternLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onPowerPatternChanged(QString)));
        connect(m_powerUi->findNext, SIGNAL(clicked()), this, SLOT(findNext()));
        connect(m_powerUi->findPrev, SIGNAL(clicked()), this, SLOT(findPrevious()));
        connect(m_powerUi->replaceNext, SIGNAL(clicked()), this, SLOT(replaceNext()));
        connect(m_powerUi->replaceAll, SIGNAL(clicked()), this, SLOT(replaceAll()));
        connect(m_powerUi->searchMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onPowerModeChanged(int)));
        connect(m_powerUi->matchCase, SIGNAL(toggled(bool)), this, SLOT(onMatchCaseToggled(bool)));
        connect(m_powerUi->findAll, SIGNAL(clicked()), this, SLOT(findAll()));

        // Make [return] in pattern line edit trigger <find next> action
        connect(patternLineEdit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
        connect(replacementLineEdit, SIGNAL(returnPressed()), this, SLOT(replaceNext()));

        // Hook into line edit context menus
        m_powerUi->pattern->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_powerUi->pattern, SIGNAL(customContextMenuRequested(QPoint)), this,
                SLOT(onPowerPatternContextMenuRequest(QPoint)));
        m_powerUi->replacement->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_powerUi->replacement, SIGNAL(customContextMenuRequested(QPoint)), this,
                SLOT(onPowerReplacmentContextMenuRequest(QPoint)));
    }

    // Focus
    if (m_widget->isVisible()) {
        m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
    }
}



void KateSearchBar::enterIncrementalMode() {
    QString initialPattern;

    // Guess settings from context: init pattern with current selection
    const bool selected = m_view->selection();
    if (selected) {
        const Range & selection = m_view->selectionRange();
        if (selection.onSingleLine()) {
            // ... with current selection
            initialPattern = m_view->selectionText();
        }
    }

    // If there's no new selection, we'll use the existing pattern
    if (initialPattern.isNull()) {
        // Coming from incremental search?
        const bool fromIncremental = (m_incUi != NULL) && (m_widget->isVisible());
        if (fromIncremental) {
            m_incUi->pattern->lineEdit()->selectAll();
            m_incUi->pattern->setFocus(Qt::MouseFocusReason);
            return;
        }

        // Coming from power search?
        const bool fromReplace = (m_powerUi != NULL) && (m_widget->isVisible());
        if (fromReplace) {
            initialPattern = m_powerUi->pattern->currentText();
        }
    }

    // Still no search pattern? Use the word under the cursor
    if (initialPattern.isNull()) {
        const KTextEditor::Cursor cursorPosition = m_view->cursorPosition();
        initialPattern = m_view->doc()->getWord( cursorPosition );
    }

    // Create dialog
    const bool create = (m_incUi == NULL);
    if (create) {
        // Kill power widget
        if (m_powerUi != NULL) {
            // Backup current settings
            const bool OF_POWER = true;
            backupConfig(OF_POWER);

            // Kill widget
            delete m_powerUi;
            m_powerUi = NULL;
            m_layout->removeWidget(m_widget);
            m_widget->deleteLater(); //deleteLater, because it's not a good idea too delete the widget and there for the button triggering this slot
        }

        // Add incremental widget
        m_widget = new QWidget(this);
        m_incUi = new Ui::IncrementalSearchBar;
        m_incUi->setupUi(m_widget);
        m_layout->addWidget(m_widget);

//         new QShortcut(KStandardShortcut::paste().primary(), m_incUi->pattern, SLOT(paste()), 0, Qt::WidgetWithChildrenShortcut);
//         if (!KStandardShortcut::paste().alternate().isEmpty())
//             new QShortcut(KStandardShortcut::paste().alternate(), m_incUi->pattern, SLOT(paste()), 0, Qt::WidgetWithChildrenShortcut);


        // Icons
        m_incUi->mutate->setIcon(KIcon("arrow-up-double"));
        m_incUi->next->setIcon(KIcon("go-down-search"));
        m_incUi->prev->setIcon(KIcon("go-up-search"));

        // Ensure minimum size
        m_incUi->pattern->setMinimumWidth(12 * m_incUi->pattern->fontMetrics().height());

	// Customize status area
	m_incUi->status->setTextElideMode(Qt::ElideLeft);

        // Focus proxy
        centralWidget()->setFocusProxy(m_incUi->pattern);

        m_incUi->pattern->setDuplicatesEnabled(false);
        m_incUi->pattern->setInsertPolicy(QComboBox::InsertAtTop);
        m_incUi->pattern->setMaxCount(m_config->maxHistorySize());
        m_incUi->pattern->setModel(m_config->patternHistoryModel());
        m_incUi->pattern->setAutoCompletion(false);
    }

    // Restore previous settings
    if (create) {
        m_incUi->matchCase->setChecked(m_incMatchCase);
    }

    // force current index of -1 --> <cursor down> shows 1st completion entry instead of 2nd
    m_incUi->pattern->setCurrentIndex(-1);

    // Set initial search pattern
    if (!create)
        disconnect(m_incUi->pattern, SIGNAL(editTextChanged(QString)), this, SLOT(onIncPatternChanged(QString)));
    m_incUi->pattern->setEditText(initialPattern);
    connect(m_incUi->pattern, SIGNAL(editTextChanged(QString)), this, SLOT(onIncPatternChanged(QString)));
    m_incUi->pattern->lineEdit()->selectAll();

    // Propagate settings (slots are still inactive on purpose)
    if (initialPattern.isEmpty()) {
        // Reset edit color
        indicateMatch(MatchNothing);
    }

    // Enable/disable next/prev
    m_incUi->next->setDisabled(initialPattern.isEmpty());
    m_incUi->prev->setDisabled(initialPattern.isEmpty());

    if (create) {
        // Slots
        connect(m_incUi->mutate, SIGNAL(clicked()), this, SLOT(enterPowerMode()));
        connect(m_incUi->pattern->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
        connect(m_incUi->next, SIGNAL(clicked()), this, SLOT(findNext()));
        connect(m_incUi->prev, SIGNAL(clicked()), this, SLOT(findPrevious()));
        connect(m_incUi->matchCase, SIGNAL(toggled(bool)), this, SLOT(onMatchCaseToggled(bool)));
    }

    // Focus
    if (m_widget->isVisible()) {
        m_incUi->pattern->setFocus(Qt::MouseFocusReason);
    }
}

bool KateSearchBar::clearHighlights()
{
    if (m_infoMessage)
        delete m_infoMessage;

    if (m_hlRanges.isEmpty()) {
        return false;
    }
    qDeleteAll(m_hlRanges);
    m_hlRanges.clear();
    return true;
}

void KateSearchBar::updateHighlightColors()
{
    const QColor& searchColor = m_view->renderer()->config()->searchHighlightColor();
    const QColor& replaceColor = m_view->renderer()->config()->replaceHighlightColor();

    // init match attribute
    highlightMatchAttribute->setBackground(searchColor);
    highlightMatchAttribute->dynamicAttribute (Attribute::ActivateMouseIn)->setBackground(searchColor);
    highlightMatchAttribute->dynamicAttribute (Attribute::ActivateCaretIn)->setBackground(searchColor);

    // init replacement attribute
    highlightReplacementAttribute->setBackground(replaceColor);
}


void KateSearchBar::showEvent(QShowEvent * event) {
    // Update init cursor
    if (m_incUi != NULL) {
        m_incInitCursor = m_view->cursorPosition();
    }

    updateSelectionOnly();
    KateViewBarWidget::showEvent(event);
}


void KateSearchBar::updateSelectionOnly() {
    if (m_powerUi == NULL) {
        return;
    }

    // Re-init "Selection only" checkbox if power search bar open
    const bool selected = m_view->selection();
    bool selectionOnly = selected;
    if (selected) {
        Range const & selection = m_view->selectionRange();
        selectionOnly = !selection.onSingleLine();
    }
    m_powerUi->selectionOnly->setChecked(selectionOnly);
}


void KateSearchBar::updateIncInitCursor() {
    if (m_incUi == NULL) {
        return;
    }

    // Update init cursor
    m_incInitCursor = m_view->cursorPosition();
}


void KateSearchBar::onPowerPatternContextMenuRequest(const QPoint& pos) {
    const bool FOR_PATTERN = true;
    showExtendedContextMenu(FOR_PATTERN, pos);
}

void KateSearchBar::onPowerPatternContextMenuRequest() {
    onPowerPatternContextMenuRequest(m_powerUi->pattern->mapFromGlobal(QCursor::pos()));
}


void KateSearchBar::onPowerReplacmentContextMenuRequest(const QPoint& pos) {
    const bool FOR_REPLACEMENT = false;
    showExtendedContextMenu(FOR_REPLACEMENT, pos);
}

void KateSearchBar::onPowerReplacmentContextMenuRequest() {
    onPowerReplacmentContextMenuRequest(m_powerUi->replacement->mapFromGlobal(QCursor::pos()));
}


bool KateSearchBar::isPower() const {
    return m_powerUi != 0;
}

void KateSearchBar::slotReadWriteChanged ()
{
    if (!KateSearchBar::isPower())
        return;
    
    // perhaps disable/enable
    m_powerUi->replaceNext->setEnabled (m_view->doc()->isReadWrite() && isPatternValid());
    m_powerUi->replaceAll->setEnabled (m_view->doc()->isReadWrite() && isPatternValid());
}

// kate: space-indent on; indent-width 4; replace-tabs on;
