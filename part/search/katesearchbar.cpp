/* This file is part of the KDE libraries
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
#include "kateregexp.h"
#include "kateview.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateconfig.h"

#include "ui_searchbarincremental.h"
#include "ui_searchbarpower.h"

#include <kactioncollection.h>
#include <ktexteditor/rangefeedback.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QKeySequence>
#include <QtGui/QShortcut>
#include <QtGui/QCursor>
#include <QStringListModel>
#include <QCompleter>
#include <QMutexLocker>

using namespace KTextEditor;



// Turn debug messages on/off here
// #define FAST_DEBUG_ENABLE

#ifdef FAST_DEBUG_ENABLE
# define FAST_DEBUG(x) kDebug() << x
#else
# define FAST_DEBUG(x)
#endif



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



KateSearchBar::KateSearchBar(bool initAsPower, KateView* view)
        : KateViewBarWidget(true, view, view),
        m_topRange(NULL),
        m_rangeNotifier(new KTextEditor::SmartRangeNotifier),
        m_layout(new QVBoxLayout()),
        m_widget(NULL),
        m_incUi(NULL),
        m_incMenu(NULL),
        m_incMenuMatchCase(NULL),
        m_incMenuFromCursor(NULL),
        m_incMenuHighlightAll(NULL),
        m_incInitCursor(0, 0),
        m_powerUi(NULL),
        m_powerMenu(NULL),
        m_powerMenuFromCursor(NULL),
        m_powerMenuHighlightAll(NULL),
        m_powerMenuSelectionOnly(NULL),
        m_incHighlightAll(false),
        m_incFromCursor(true),
        m_incMatchCase(false),
        m_powerMatchCase(true),
        m_powerFromCursor(false),
        m_powerHighlightAll(false),
        m_powerMode(0) {

    connect(m_rangeNotifier,SIGNAL(rangeContentsChanged(KTextEditor::SmartRange*)),
      this,SLOT(onRangeContentsChanged(KTextEditor::SmartRange*)));

    // Modify parent
    QWidget * const widget = centralWidget();
    widget->setLayout(m_layout);
    m_layout->setMargin(0);

    // Init highlight
    {
      QMutexLocker lock(view->doc()->smartMutex());
      
      m_topRange = view->doc()->newSmartRange(view->doc()->documentRange());
      static_cast<KateSmartRange*>(m_topRange)->setInternal();
      m_topRange->setInsertBehavior(SmartRange::ExpandLeft | SmartRange::ExpandRight);
      disableHighlights();
    }


    // Copy global to local config backup
    KateViewConfig * const globalConfig = KateGlobal::self()->viewConfig();
    const long searchFlags = globalConfig->searchFlags();
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
        onMutatePower();
    } else {
        onMutateIncremental();
    }
}



KateSearchBar::~KateSearchBar() {
//  delete m_topRange; this gets deleted somewhere else (bug #176027)
    delete m_layout;
    delete m_widget;

    delete m_incUi;
    delete m_incMenu;

    delete m_powerUi;
    delete m_powerMenu;
}



void KateSearchBar::findNext() {
    const bool found = find();

    if (found && m_powerUi != NULL) {
        // Add to search history
        addCurrentTextToHistory(m_powerUi->pattern);
    }
}



void KateSearchBar::findPrevious() {
    const bool found = find(SearchBackward);

    if (found && m_powerUi != NULL) {
        // Add to search history
        addCurrentTextToHistory(m_powerUi->pattern);
    }
}



void KateSearchBar::highlight(const Range & range, const QColor & color) {
    SmartRange * const highlight = view()->doc()->newSmartRange(range, m_topRange);
    highlight->setInsertBehavior(SmartRange::DoNotExpand);
    Attribute::Ptr attribute(new Attribute());
    attribute->setBackground(color);
    highlight->setAttribute(attribute);
    highlight->addNotifier(m_rangeNotifier);
}



void KateSearchBar::highlightMatch(const Range & range) {
    highlight(range, Qt::yellow); // TODO make this part of the color scheme
}



void KateSearchBar::highlightReplacement(const Range & range) {
    highlight(range, Qt::green); // TODO make this part of the color scheme
}



void KateSearchBar::highlightAllMatches() {
    findAll(view()->doc()->documentRange(), NULL);
}

void KateSearchBar::onRangeContentsChanged(KTextEditor::SmartRange* range) {
  indicateMatch(MatchNeutral);
  Attribute::Ptr attribute(new Attribute());
  //attribute->setBackground(color);
  range->setAttribute(attribute);

}

void KateSearchBar::indicateMatch(MatchResult matchResult) {
    QLineEdit * const lineEdit = isPower() ? m_powerUi->pattern->lineEdit()
                                           : m_incUi->pattern;
    QPalette background(lineEdit->palette());

    switch (matchResult) {
    case MatchFound:  // FALLTHROUGH
    case MatchWrapped:
        // Green background for line edit
        KColorScheme::adjustBackground(background, KColorScheme::PositiveBackground);
        break;
    case MatchMismatch:
        // Red background for line edit
        KColorScheme::adjustBackground(background, KColorScheme::NegativeBackground);
        break;
    case MatchNothing:
        // Reset background of line edit
        if (m_incUi != NULL) {
            background = QPalette();
        } else {
            // ### this is fragile (depends on knowledge of QPalette::ColorGroup)
            // ...would it better to cache the original palette?
            QColor color = QPalette().color(QPalette::Base);
            background.setBrush(QPalette::Active, QPalette::Base, QPalette().brush(QPalette::Active, QPalette::Base));
            background.setBrush(QPalette::Inactive, QPalette::Base, QPalette().brush(QPalette::Inactive, QPalette::Base));
            background.setBrush(QPalette::Disabled, QPalette::Base, QPalette().brush(QPalette::Disabled, QPalette::Base));
        }
        break;
    case MatchNeutral:
        KColorScheme::adjustBackground(background, KColorScheme::NeutralBackground);
        break;
    }

    // Update status label
    if (m_incUi != NULL) {
        switch (matchResult) {
        case MatchFound: // FALLTHROUGH
        case MatchNothing:
            m_incUi->status->setText("");
            break;
        case MatchWrapped:
            m_incUi->status->setText(i18n("Reached bottom, continued from top"));
            break;
        case MatchMismatch:
            m_incUi->status->setText(i18n("Not found"));
            break;
        case MatchNeutral:
            /* do nothing */
            break;
        }
    }

    lineEdit->setPalette(background);
}



/*static*/ void KateSearchBar::selectRange(KateView * view, const KTextEditor::Range & range) {
    view->setCursorPositionInternal(range.start(), 1);

    // don't make a selection if the vi input mode is used
    if (!view->viInputMode())
        view->setSelection(range);
}



void KateSearchBar::nonstatic_selectRange(KateView * view, const KTextEditor::Range & range) {
    // don't update m_incInitCursor when we move the cursor 
    disconnect(view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor const&)),
               this, SLOT(onCursorPositionChanged()));
    nonstatic_selectRange2(view, range);
    connect(view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor const&)),
            this, SLOT(onCursorPositionChanged()));
}



void KateSearchBar::nonstatic_selectRange2(KateView * view, const KTextEditor::Range & range) {
    disconnect(view, SIGNAL(selectionChanged(KTextEditor::View *)),
               this, SLOT(onSelectionChanged()));
    selectRange(view, range);
    connect(view, SIGNAL(selectionChanged(KTextEditor::View *)),
            this, SLOT(onSelectionChanged()));
}



void KateSearchBar::buildReplacement(QString & output, QList<ReplacementPart> & parts,
        const QVector<Range> & details, int replacementCounter) {
    const int MIN_REF_INDEX = 0;
    const int MAX_REF_INDEX = details.count() - 1;

    output.clear();
    ReplacementPart::Type caseConversion = ReplacementPart::KeepCase;
    for (QList<ReplacementPart>::iterator iter = parts.begin(); iter != parts.end(); iter++) {
        ReplacementPart & curPart = *iter;
        switch (curPart.type) {
        case ReplacementPart::Reference:
            if ((curPart.index < MIN_REF_INDEX) || (curPart.index > MAX_REF_INDEX)) {
                // Insert just the number to be consistent with QRegExp ("\c" becomes "c")
                output.append(QString::number(curPart.index));
            } else {
                const Range & captureRange = details[curPart.index];
                if (captureRange.isValid()) {
                    // Copy capture content
                    const bool blockMode = view()->blockSelection();
                    const QString content = view()->doc()->text(captureRange, blockMode);
                    switch (caseConversion) {
                    case ReplacementPart::UpperCase:
                        // Copy as uppercase
                        output.append(content.toUpper());
                        break;

                    case ReplacementPart::LowerCase:
                        // Copy as lowercase
                        output.append(content.toLower());
                        break;

                    case ReplacementPart::KeepCase: // FALLTHROUGH
                    default:
                        // Copy unmodified
                        output.append(content);
                        break;

                    }
                }
            }
            break;

        case ReplacementPart::UpperCase: // FALLTHROUGH
        case ReplacementPart::LowerCase: // FALLTHROUGH
        case ReplacementPart::KeepCase:
            caseConversion = curPart.type;
            break;

        case ReplacementPart::Counter:
            {
                // Zero padded counter value
                const int minWidth = curPart.index;
                const int number = replacementCounter;
                output.append(QString("%1").arg(number, minWidth, 10, QLatin1Char('0')));
            }
            break;

        case ReplacementPart::Text: // FALLTHROUGH
        default:
            switch (caseConversion) {
            case ReplacementPart::UpperCase:
                // Copy as uppercase
                output.append(curPart.text.toUpper());
                break;

            case ReplacementPart::LowerCase:
                // Copy as lowercase
                output.append(curPart.text.toLower());
                break;

            case ReplacementPart::KeepCase: // FALLTHROUGH
            default:
                // Copy unmodified
                output.append(curPart.text);
                break;

            }
            break;

        }
    }
}



void KateSearchBar::replaceMatch(const QVector<Range> & match, const QString & replacement,
        int replacementCounter) {
    // Placeholders depending on search mode
    const bool usePlaceholders = searchOptions().testFlag(Search::Regex) ||
                                 searchOptions().testFlag(Search::EscapeSequences);

    const Range & targetRange = match[0];
    QString finalReplacement;
    if (usePlaceholders) {
        // Resolve references and escape sequences
        QList<ReplacementPart> parts;
        const bool REPLACEMENT_GOODIES = true;
        KateEscapedTextSearch::escapePlaintext(replacement, &parts, REPLACEMENT_GOODIES);
        buildReplacement(finalReplacement, parts, match, replacementCounter);
    } else {
        // Plain text replacement
        finalReplacement = replacement;
    }

    const bool blockMode = (view()->blockSelection() && !targetRange.onSingleLine());
    view()->doc()->replaceText(targetRange, finalReplacement, blockMode);
}



void KateSearchBar::onIncPatternChanged(const QString & pattern) {
    if (pattern.isEmpty()) {
        // Kill selection
        nonstatic_selectRange(view(), Range(m_incInitCursor, m_incInitCursor));

        // Kill highlight
        resetHighlights();

        // Reset edit color
        indicateMatch(MatchNothing);

        // Disable next/prev
        m_incUi->next->setDisabled(true);
        m_incUi->prev->setDisabled(true);
        return;
    }

    // Enable next/prev
    m_incUi->next->setDisabled(false);
    m_incUi->prev->setDisabled(false);

    // Where to find?
    Range inputRange;
    if (fromCursor()) {
        inputRange.setRange(m_incInitCursor, view()->doc()->documentEnd());
    } else {
        inputRange = view()->doc()->documentRange();
    }

    // Find, first try
    const QVector<Range> resultRanges = view()->doc()->searchText(inputRange, pattern, searchOptions());
    const Range & match = resultRanges[0];

    bool found = false;
    if (match.isValid()) {
        nonstatic_selectRange(view(), match);
        indicateMatch(MatchFound);
        found = true;
    } else {
        // Wrap if it makes sense
        if (fromCursor()) {
            // Find, second try
            inputRange = view()->doc()->documentRange();
            const QVector<Range> resultRanges2 = view()->doc()->searchText(inputRange, pattern, searchOptions());
            const Range & match2 = resultRanges2[0];
            if (match2.isValid()) {
                nonstatic_selectRange(view(), match2);
                indicateMatch(MatchWrapped);
                found = true;
            } else {
                indicateMatch(MatchMismatch);
            }
        } else {
            indicateMatch(MatchMismatch);
        }
    }

    // Highlight all
    if (isChecked(m_incMenuHighlightAll)) {
        if (found ) {
            highlightAllMatches();
        } else {
            resetHighlights();
        }
    }
    if (!found) {
        view()->setSelection(Range::invalid());
    }
}



void KateSearchBar::onMatchCaseToggled(bool /*matchCase*/) {
    sendConfig();

    if (m_incUi != 0) {
       // Re-search with new settings
        const QString pattern = m_incUi->pattern->displayText();
        onIncPatternChanged(pattern);
    } else {
        indicateMatch(MatchNothing);
    }
}



void KateSearchBar::onHighlightAllToggled(bool checked) {
    sendConfig();

    if (checked) {
        if (!searchPattern().isEmpty()) {
            // Highlight them all
            resetHighlights();
            highlightAllMatches();
        }
    } else {
        resetHighlights();
    }
}



void KateSearchBar::onFromCursorToggled(bool /*fromCursor*/) {
        sendConfig();
}



void KateSearchBar::fixForSingleLine(Range & range, SearchDirection searchDirection) {
    FAST_DEBUG("Single-line workaround checking BEFORE" << range);
    if (searchDirection == SearchForward) {
        const int line = range.start().line();
        const int col = range.start().column();
        const int maxColWithNewline = view()->doc()->lineLength(line) + 1;
        if (col == maxColWithNewline) {
            FAST_DEBUG("Starting on a newline" << range);
            const int maxLine = view()->doc()->lines() - 1;
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
                const int maxColWithNewline = view()->doc()->lineLength(line - 1);
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
    if (view()->viInputMode()) {
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
    Range selection;
    const bool selected = view()->selection();
    if (selected) {
        selection = view()->selectionRange();
        if (selectionOnly()) {
            // First match in selection
            inputRange = selection;
        } else {
            // Next match after/before selection if a match was selected before
            if (searchDirection == SearchForward) {
                inputRange.setRange(selection.start(), view()->doc()->documentEnd());
            } else {
                inputRange.setRange(Cursor(0, 0), selection.end());
            }
        }
    } else {
        // No selection
        if (fromCursor()) {
            const Cursor cursorPos = view()->cursorPosition();
            if (searchDirection == SearchForward) {
                // if the vi input mode is used, the cursor will stay a the first character of the
                // matched pattern (no selection will be made), so the next search should start from
                // match column + 1
                if (!view()->viInputMode()) {
                    inputRange.setRange(cursorPos, view()->doc()->documentEnd());
                } else {
                    inputRange.setRange(Cursor(cursorPos.line(), cursorPos.column()+1), view()->doc()->documentEnd());
                }
            } else {
                inputRange.setRange(Cursor(0, 0), cursorPos);
            }
        } else {
            inputRange = view()->doc()->documentRange();
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


    // Find, first try
    const QVector<Range> resultRanges = view()->doc()->searchText(inputRange, searchPattern(), enabledOptions);
    const Range & match = resultRanges[0];
    bool wrap = false;
    bool found = false;
    SmartRange * afterReplace = NULL;
    if (match.isValid()) {
        // Previously selected match again?
        if (selected && (match == selection) && (!selectionOnly() || replacement != 0)) {
            // Same match again
            if (replacement != 0) {
                // Selection is match -> replace
                afterReplace = view()->doc()->newSmartRange(match);
                afterReplace->setInsertBehavior(SmartRange::ExpandRight | SmartRange::ExpandLeft);
                replaceMatch(resultRanges, *replacement);

                // Find, second try after replaced text
                if (searchDirection == SearchForward) {
                    inputRange.setRange(afterReplace->end(), inputRange.end());
                } else {
                    inputRange.setRange(inputRange.start(), afterReplace->start());
                }
            } else {
                // Find, second try after old selection
                if (searchDirection == SearchForward) {
                    inputRange.setRange(selection.end(), inputRange.end());
                } else {
                    inputRange.setRange(inputRange.start(), selection.start());
                }
            }

            // Single-line pattern workaround
            fixForSingleLine(inputRange, searchDirection);

            const QVector<Range> resultRanges2 = view()->doc()->searchText(inputRange, searchPattern(), enabledOptions);
            const Range & match2 = resultRanges2[0];
            if (match2.isValid()) {
                nonstatic_selectRange2(view(), match2);
                found = true;
                indicateMatch(MatchFound);
            } else {
                // Find, third try from doc start on
                wrap = true;
            }
        } else {
            nonstatic_selectRange2(view(), match);
            found = true;
            indicateMatch(MatchFound);
        }
    } else if (!selected || !selectionOnly()) {
        // Find, second try from doc start on
        wrap = true;
    }

    // Wrap around
    if (wrap) {
        inputRange = view()->doc()->documentRange();
        const QVector<Range> resultRanges3 = view()->doc()->searchText(inputRange, searchPattern(), enabledOptions);
        const Range & match3 = resultRanges3[0];
        if (match3.isValid()) {
            // Previously selected match again?
            if (selected && !selectionOnly() && (match3 == selection)) {
                // NOOP, same match again
            } else {
                nonstatic_selectRange2(view(), match3);
                found = true;
            }
            indicateMatch(MatchWrapped);
        } else {
            indicateMatch(MatchMismatch);
        }
    }

    // Highlight all matches and/or replacement
    if ((found && highlightAll()) || (afterReplace != NULL)) {
        // Highlight all matches
        if (found && highlightAll()) {
            highlightAllMatches();
        }

        // Highlight replacement (on top if overlapping) if new match selected
        if (found && (afterReplace != NULL)) {
            // Note: highlightAllMatches already reset for us
            if (!(found && highlightAll())) {
                resetHighlights();
            }

            highlightReplacement(*afterReplace);
        }

    }

    delete afterReplace;

    return true; // == No pattern error
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
    if (index != -1) {
        combo->removeItem(index);
    }
    combo->insertItem(0, text);
    combo->setCurrentIndex(0);
}



void KateSearchBar::backupConfig(bool ofPower) {
    if (ofPower) {
        m_powerMatchCase = isChecked(m_powerUi->matchCase);
        m_powerFromCursor = isChecked(m_powerMenuFromCursor);
        m_powerHighlightAll = isChecked(m_powerMenuHighlightAll);
        m_powerMode = m_powerUi->searchMode->currentIndex();
    } else {
        m_incHighlightAll = isChecked(m_incMenuHighlightAll);
        m_incFromCursor = isChecked(m_incMenuFromCursor);
        m_incMatchCase = isChecked(m_incMenuMatchCase);
    }
}



void KateSearchBar::sendConfig() {
    KateViewConfig * const globalConfig = KateGlobal::self()->viewConfig();
    const long pastFlags = globalConfig->searchFlags();
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
    globalConfig->setSearchFlags(futureFlags);
}



void KateSearchBar::onPowerReplaceNext() {
    const QString replacement = m_powerUi->replacement->currentText();

    if (find(SearchForward, &replacement)) {
        // Add to search history
        addCurrentTextToHistory(m_powerUi->pattern);

        // Add to replace history
        addCurrentTextToHistory(m_powerUi->replacement);
    }
}



// replacement == NULL --> Highlight all matches
// replacement != NULL --> Replace and highlight all matches
void KateSearchBar::findAll(Range inputRange, const QString * replacement) {
    const Search::SearchOptions enabledOptions = searchOptions(SearchForward);

    const bool regexMode = enabledOptions.testFlag(Search::Regex);
    const bool multiLinePattern = regexMode ? KateRegExp(searchPattern()).isMultiLine() : false;

    // Before first match
    resetHighlights();

    SmartRange * const workingRange = view()->doc()->newSmartRange(inputRange);
    QList<Range> highlightRanges;
    int matchCounter = 0;
    for (;;) {
        const QVector<Range> resultRanges = view()->doc()->searchText(*workingRange, searchPattern(), enabledOptions);
        Range match = resultRanges[0];
        if (!match.isValid()) {
            break;
        }
        bool const originalMatchEmpty = match.isEmpty();

        // Work with the match
        if (replacement != NULL) {
            if (matchCounter == 0) {
                view()->doc()->editBegin();
            }

            // Track replacement operation
            SmartRange * const afterReplace = view()->doc()->newSmartRange(match);
            afterReplace->setInsertBehavior(SmartRange::ExpandRight | SmartRange::ExpandLeft);

            // Replace
            replaceMatch(resultRanges, *replacement, ++matchCounter);

            // Highlight and continue after adjusted match
            //highlightReplacement(*afterReplace);
            match = *afterReplace;
            highlightRanges << match;
            delete afterReplace;
        } else {
            // Highlight and continue after original match
            //highlightMatch(match);
            highlightRanges << match;
            matchCounter++;
        }

        // Continue after match
        SmartCursor & workingStart = workingRange->smartStart();
        workingStart.setPosition(match.end());
        if (originalMatchEmpty) {
            // Can happen for regex patterns like "^".
            // If we don't advance here we will loop forever...
            workingStart.advance(1);
        } else if (regexMode && !multiLinePattern && workingStart.atEndOfLine()) {
            // single-line regexps might match the naked line end
            // therefore we better advance to the next line
            workingStart.advance(1);
        }

        // Are we done?
        if (!workingRange->isValid() || workingStart.atEndOfDocument()) {
            break;
        }
    }

    // After last match
    if (matchCounter > 0) {
        if (replacement != NULL) {
            view()->doc()->editEnd();
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
}



void KateSearchBar::onPowerReplaceAll() {
    // What to find/replace?
    const QString replacement = m_powerUi->replacement->currentText();

    // Where to replace?
    Range selection;
    const bool selected = view()->selection();
    Range inputRange = (selected && selectionOnly())
            ? view()->selectionRange()
            : view()->doc()->documentRange();


    // Pass on the hard work
    findAll(inputRange, &replacement);


    // Add to search history
    addCurrentTextToHistory(m_powerUi->pattern);

    // Add to replace history
    addCurrentTextToHistory(m_powerUi->replacement);
}



QString KateSearchBar::searchPattern() const {
    return (m_powerUi != 0) ? m_powerUi->pattern->currentText()
                            : m_incUi->pattern->displayText();
}



bool KateSearchBar::fromCursor() const {
    return isPower() ? m_powerMenuFromCursor->isChecked()
                     : m_incMenuFromCursor->isChecked();
}



bool KateSearchBar::selectionOnly() const {
    return isPower() ? m_powerMenuSelectionOnly->isChecked()
                     : false;
}



bool KateSearchBar::highlightAll() const {
    return isPower() ? m_powerMenuHighlightAll->isChecked()
                     : m_incMenuHighlightAll->isChecked();
}



KTextEditor::Search::SearchOptions KateSearchBar::searchOptions(SearchDirection searchDirection) const {
    Search::SearchOptions enabledOptions = KTextEditor::Search::Default;

    const bool matchCase = (m_powerUi != NULL)
            ? m_powerUi->matchCase->isChecked()
            : m_incMenuMatchCase->isChecked();
    if (!matchCase) {
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

    AddMenuManager addMenuManager(contextMenu, 35);
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
        const QVector<Range> resultRanges = view->doc()->searchText(inputRange, pattern, enabledOptions);
        const Range & match = resultRanges[0];

        if (match.isValid()) {
            selectRange(view, match);
        } else {
            // Find, second try
            if (searchDirection == SearchForward) {
                inputRange.setRange(Cursor(0, 0), selRange.start());
            } else {
                inputRange.setRange(selRange.end(), view->doc()->documentEnd());
            }
            const QVector<Range> resultRanges2 = view->doc()->searchText(inputRange, pattern, enabledOptions);
            const Range & match2 = resultRanges2[0];
            if (match2.isValid()) {
                selectRange(view, match2);
            }
        }
    } else {
        // Select current word so we can search for that the next time
        const Cursor cursorPos = view->cursorPosition();
        view->selectWord(cursorPos);
    }
}



void KateSearchBar::onMutatePower() {
    QString initialPattern;
    bool selectionOnly = false;

    // Guess settings from context: init pattern with current selection
    const bool selected = view()->selection();
    if (selected) {
        const Range & selection = view()->selectionRange();
        if (selection.onSingleLine()) {
            // ... with current selection
            initialPattern = view()->selectionText();
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
            initialPattern = m_incUi->pattern->displayText();
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
            delete m_incMenu;
            m_incUi = NULL;
            m_incMenu = NULL;
            m_incMenuMatchCase = NULL;
            m_incMenuFromCursor = NULL;
            m_incMenuHighlightAll = NULL;
            m_layout->removeWidget(m_widget);
            m_widget->deleteLater(); // I didn't get a crash here but for symmetrie to the other mutate slot^
        }

        // Add power widget
        m_widget = new QWidget(this);
        m_powerUi = new Ui::PowerSearchBar;
        m_powerUi->setupUi(m_widget);
        m_layout->addWidget(m_widget);

        // Bind to shared history models
        const int MAX_HISTORY_SIZE = 100; // Please don't lower this value! Thanks, Sebastian
        QStringListModel * const patternHistoryModel = KateHistoryModel::getPatternHistoryModel();
        QStringListModel * const replacementHistoryModel = KateHistoryModel::getReplacementHistoryModel();
        m_powerUi->pattern->setMaxCount(MAX_HISTORY_SIZE);
        m_powerUi->pattern->setModel(patternHistoryModel);
        m_powerUi->replacement->setMaxCount(MAX_HISTORY_SIZE);
        m_powerUi->replacement->setModel(replacementHistoryModel);

        // Fill options menu
        m_powerMenu = new QMenu();
        m_powerUi->options->setMenu(m_powerMenu);
        m_powerMenuFromCursor = m_powerMenu->addAction(i18n("From &cursor"));
        m_powerMenuFromCursor->setCheckable(true);
        m_powerMenuHighlightAll = m_powerMenu->addAction(i18n("Hi&ghlight all"));
        m_powerMenuHighlightAll->setCheckable(true);
        m_powerMenuSelectionOnly = m_powerMenu->addAction(i18n("Selection &only"));
        m_powerMenuSelectionOnly->setCheckable(true);

        // Icons
        m_powerUi->mutate->setIcon(KIcon("arrow-down-double"));
        m_powerUi->findNext->setIcon(KIcon("go-down-search"));
        m_powerUi->findPrev->setIcon(KIcon("go-up-search"));

        // Focus proxy
        centralWidget()->setFocusProxy(m_powerUi->pattern);

        // Make completers case-sensitive
        QLineEdit * const patternLineEdit = m_powerUi->pattern->lineEdit();
        Q_ASSERT(patternLineEdit != NULL);
        patternLineEdit->completer()->setCaseSensitivity(Qt::CaseSensitive);

        QLineEdit * const replacementLineEdit = m_powerUi->replacement->lineEdit();
        Q_ASSERT(replacementLineEdit != NULL);
        replacementLineEdit->completer()->setCaseSensitivity(Qt::CaseSensitive);
    }

    setChecked(m_powerMenuSelectionOnly, selectionOnly);

    // Restore previous settings
    if (create) {
        setChecked(m_powerUi->matchCase, m_powerMatchCase);
        setChecked(m_powerMenuHighlightAll, m_powerHighlightAll);
        setChecked(m_powerMenuFromCursor, m_powerFromCursor);
        m_powerUi->searchMode->setCurrentIndex(m_powerMode);
    }

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
        connect(m_powerUi->mutate, SIGNAL(clicked()), this, SLOT(onMutateIncremental()));
        connect(patternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onPowerPatternChanged(const QString &)));
        connect(m_powerUi->findNext, SIGNAL(clicked()), this, SLOT(findNext()));
        connect(m_powerUi->findPrev, SIGNAL(clicked()), this, SLOT(findPrevious()));
        connect(m_powerUi->replaceNext, SIGNAL(clicked()), this, SLOT(onPowerReplaceNext()));
        connect(m_powerUi->replaceAll, SIGNAL(clicked()), this, SLOT(onPowerReplaceAll()));
        connect(m_powerUi->searchMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onPowerModeChanged(int)));
        connect(m_powerUi->matchCase, SIGNAL(toggled(bool)), this, SLOT(onMatchCaseToggled(bool)));
        connect(m_powerMenuHighlightAll, SIGNAL(toggled(bool)), this, SLOT(onHighlightAllToggled(bool)));
        connect(m_powerMenuFromCursor, SIGNAL(toggled(bool)), this, SLOT(onFromCursorToggled(bool)));

        // Make button click open the menu as well. IMHO with the dropdown arrow present the button
        // better shows his nature than in instant popup mode.
        connect(m_powerUi->options, SIGNAL(clicked()), m_powerUi->options, SLOT(showMenu()));

        // Make [return] in pattern line edit trigger <find next> action
        connect(patternLineEdit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
        connect(replacementLineEdit, SIGNAL(returnPressed()), this, SLOT(onPowerReplaceNext()));

        // Hook into line edit context menus
        m_powerUi->pattern->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_powerUi->pattern, SIGNAL(customContextMenuRequested(const QPoint&)), this,
                SLOT(onPowerPatternContextMenuRequest(const QPoint&)));
        m_powerUi->replacement->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_powerUi->replacement, SIGNAL(customContextMenuRequested(const QPoint&)), this,
                SLOT(onPowerReplacmentContextMenuRequest(const QPoint&)));
    }

    // Focus
    if (m_widget->isVisible()) {
        m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
    }
}



void KateSearchBar::onMutateIncremental() {
    QString initialPattern;

    // Guess settings from context: init pattern with current selection
    const bool selected = view()->selection();
    if (selected) {
        const Range & selection = view()->selectionRange();
        if (selection.onSingleLine()) {
            // ... with current selection
            initialPattern = view()->selectionText();
        }
    }

    // If there's no new selection, we'll use the existing pattern
    if (initialPattern.isNull()) {
        // Coming from incremental search?
        const bool fromIncremental = (m_incUi != NULL) && (m_widget->isVisible());
        if (fromIncremental) {
            m_incUi->pattern->selectAll();
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
        initialPattern = view()->currentWord();
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

        new QShortcut(KStandardShortcut::paste().primary(), m_incUi->pattern, SLOT(paste()), 0, Qt::WidgetWithChildrenShortcut);
        if (!KStandardShortcut::paste().alternate().isEmpty())
            new QShortcut(KStandardShortcut::paste().alternate(), m_incUi->pattern, SLOT(paste()), 0, Qt::WidgetWithChildrenShortcut);


        // Fill options menu
        m_incMenu = new QMenu();
        m_incUi->options->setMenu(m_incMenu);
        m_incMenuFromCursor = m_incMenu->addAction(i18n("From &cursor"));
        m_incMenuFromCursor->setCheckable(true);
        m_incMenuHighlightAll = m_incMenu->addAction(i18n("Hi&ghlight all"));
        m_incMenuHighlightAll->setCheckable(true);
        m_incMenuMatchCase = m_incMenu->addAction(i18n("&Match case"));
        m_incMenuMatchCase->setCheckable(true);

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
    }

    // Restore previous settings
    if (create) {
        setChecked(m_incMenuHighlightAll, m_incHighlightAll);
        setChecked(m_incMenuFromCursor, m_incFromCursor);
        setChecked(m_incMenuMatchCase, m_incMatchCase);
    }

    // Set initial search pattern
    if (!create)
        disconnect(m_incUi->pattern, SIGNAL(textChanged(const QString&)), this, SLOT(onIncPatternChanged(const QString&)));
    m_incUi->pattern->setText(initialPattern);
    connect(m_incUi->pattern, SIGNAL(textChanged(const QString&)), this, SLOT(onIncPatternChanged(const QString&)));
    m_incUi->pattern->selectAll();

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
        connect(m_incUi->mutate, SIGNAL(clicked()), this, SLOT(onMutatePower()));
        connect(m_incUi->pattern, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
        connect(m_incUi->pattern, SIGNAL(textChanged(const QString &)), this, SLOT(onIncPatternChanged(const QString &)));
        connect(m_incUi->next, SIGNAL(clicked()), this, SLOT(findNext()));
        connect(m_incUi->prev, SIGNAL(clicked()), this, SLOT(findPrevious()));
        connect(m_incMenuMatchCase, SIGNAL(toggled(bool)), this, SLOT(onMatchCaseToggled(bool)));
        connect(m_incMenuFromCursor, SIGNAL(toggled(bool)), this, SLOT(onFromCursorToggled(bool)));
        connect(m_incMenuHighlightAll, SIGNAL(toggled(bool)), this, SLOT(onHighlightAllToggled(bool)));

        // Make button click open the menu as well. IMHO with the dropdown arrow present the button
        // better shows his nature than in instant popup mode.
        connect(m_incUi->options, SIGNAL(clicked()), m_incUi->options, SLOT(showMenu()));
    }

    // Focus
    if (m_widget->isVisible()) {
        m_incUi->pattern->setFocus(Qt::MouseFocusReason);
    }
}



bool KateSearchBar::isChecked(QCheckBox * checkbox) {
    Q_ASSERT(checkbox != NULL);
    return checkbox->checkState() == Qt::Checked;
}



bool KateSearchBar::isChecked(QAction * menuAction) {
    Q_ASSERT(menuAction != NULL);
    return menuAction->isChecked();
}



void KateSearchBar::setChecked(QCheckBox * checkbox, bool checked) {
    Q_ASSERT(checkbox != NULL);
    checkbox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}



void KateSearchBar::setChecked(QAction * menuAction, bool checked) {
    Q_ASSERT(menuAction != NULL);
    menuAction->setChecked(checked);
}



void KateSearchBar::enableHighlights() {
    view()->addInternalHighlight(m_topRange);
}



void KateSearchBar::disableHighlights() {
    view()->removeInternalHighlight(m_topRange);
    m_topRange->deleteChildRanges();
}



void KateSearchBar::resetHighlights() {
    disableHighlights();
    enableHighlights();
}



void KateSearchBar::showEvent(QShowEvent * event) {
    // Update init cursor
    if (m_incUi != NULL) {
        m_incInitCursor = view()->cursorPosition();
    }

    connect(view(), SIGNAL(selectionChanged(KTextEditor::View *)),
            this, SLOT(onSelectionChanged()));
    connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View *, KTextEditor::Cursor const &)),
            this, SLOT(onCursorPositionChanged()));

    enableHighlights();
    KateViewBarWidget::showEvent(event);
}



void KateSearchBar::closed() {
    disconnect(view(), SIGNAL(selectionChanged(KTextEditor::View *)),
            this, SLOT(onSelectionChanged()));
    disconnect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View *, KTextEditor::Cursor const &)),
            this, SLOT(onCursorPositionChanged()));

    disableHighlights();
}



void KateSearchBar::onSelectionChanged() {
    if (m_powerUi == NULL) {
        return;
    }

    // Re-init "Selection only" checkbox if power search bar open
    const bool selected = view()->selection();
    bool selectionOnly = selected;
    if (selected) {
        Range const & selection = view()->selectionRange();
        selectionOnly = !selection.onSingleLine();
    }
    setChecked(m_powerMenuSelectionOnly, selectionOnly);
}


void KateSearchBar::onCursorPositionChanged() {
    if (m_incUi == NULL) {
        return;
    }

    // Update init cursor
    m_incInitCursor = view()->cursorPosition();
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

#include "katesearchbar.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
