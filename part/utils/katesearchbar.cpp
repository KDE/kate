/* ##################################################################
##
##  TODO:
##  * Highlight all (with background thread?)
##
################################################################## */

/* This file is part of the KDE libraries
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>
   Copyright (C) 2007 Matthew Woehlke <mw_triad@users.sourceforge.net>

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
#include "kateview.h"
#include "katedocument.h"
#include "kateglobal.h"

#include "ui_searchbarincremental.h"
#include "ui_searchbarpower.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QStringListModel>


using namespace KTextEditor;



KateSearchBar::KateSearchBar(KateViewBar * viewBar, bool initAsPower)
        : KateViewBarWidget(viewBar),
        m_view(viewBar->view()),
        m_topRange(NULL),
        m_layout(new QVBoxLayout()),
        m_widget(NULL),
        m_incUi(NULL),
        m_incMenu(NULL),
        m_incMenuMatchCase(NULL),
        m_incMenuFromCursor(NULL),
        m_incMenuHighlightAll(NULL),
        m_incInitCursor(0, 0),
        m_powerUi(NULL),
        m_incHighlightAll(false),
        m_incFromCursor(true),
        m_incMatchCase(false),
        m_powerMatchCase(true),
        m_powerFromCursor(false),
        m_powerHighlightAll(false),
        m_powerUsePlaceholders(false),
        m_powerMode(0) {
    // Modify parent
    QWidget * const widget = centralWidget();
    widget->setLayout(m_layout);
    m_layout->setMargin(2);

    // Init highlight
    m_topRange = m_view->doc()->newSmartRange(m_view->doc()->documentRange());
    m_topRange->setInsertBehavior(SmartRange::ExpandRight);
    enableHighlights(true);


    // Copy global to local config backup
    KateViewConfig * const globalConfig = KateGlobal::self()->viewConfig();
    const long searchFlags = globalConfig->searchFlags();
    m_incHighlightAll = (searchFlags & KateViewConfig::IncHighlightAll) != 0;
    m_incFromCursor = (searchFlags & KateViewConfig::IncFromCursor) != 0;
    m_incMatchCase = (searchFlags & KateViewConfig::IncMatchCase) != 0;
    m_powerMatchCase = (searchFlags & KateViewConfig::PowerMatchCase) != 0;
    m_powerFromCursor = (searchFlags & KateViewConfig::PowerFromCursor) != 0;
    m_powerHighlightAll = (searchFlags & KateViewConfig::PowerHighlightAll) != 0;
    m_powerUsePlaceholders = (searchFlags & KateViewConfig::PowerUsePlaceholders) != 0;
    m_powerMode = ((searchFlags & KateViewConfig::PowerModeRegularExpression) != 0)
            ? MODE_REGEX
            : (((searchFlags & KateViewConfig::PowerModeEscapeSequences) != 0)
                ? MODE_ESCAPE_SEQUENCES
                : (((searchFlags & KateViewConfig::PowerModeWholeWords) != 0)
                    ? MODE_WHOLE_WORDS
                    : MODE_PLAIN_TEXT));
    kDebug() << "GLOBAL SEARCH CONFIG COPIED TO LOCAL" << "past" << searchFlags;


    // Load one of either dialogs
    if (initAsPower) {
        onMutatePower();
    } else {
        onMutateIncremental();
    }
}



KateSearchBar::~KateSearchBar() {
    delete m_topRange;
    delete m_layout;
    delete m_widget;
    delete m_incUi;
    delete m_incMenu;
    delete m_powerUi;
}



void KateSearchBar::findNext() {
    if (m_incUi != NULL) {
        onIncNext();
    } else {
        onPowerFindNext();
    }
}



void KateSearchBar::findPrevious() {
    if (m_incUi != NULL) {
        onIncPrev();
    } else {
        onPowerFindPrev();
    }
}



void KateSearchBar::highlight(const Range & range, const QColor & color) {
    SmartRange * const highlight = m_view->doc()->newSmartRange(range, m_topRange);
    highlight->setInsertBehavior(SmartRange::ExpandRight);
    Attribute::Ptr attribute(new Attribute());
    attribute->setBackground(color);
    highlight->setAttribute(attribute);
}



void KateSearchBar::highlightMatch(const Range & range) {
    highlight(range, Qt::yellow); // TODO make this part of the color scheme
}



void KateSearchBar::highlightReplacement(const Range & range) {
    highlight(range, Qt::green); // TODO make this part of the color scheme
}



inline void KateSearchBar::adjustBackground(QPalette & palette, KColorScheme::BackgroundRole newRole) {
    const QPalette::ColorRole role = QPalette::Base;
    const KColorScheme::ColorSet set = KColorScheme::View;
    palette.setBrush(QPalette::Active,   role, KColorScheme(QPalette::Active,   set).background(newRole));
    palette.setBrush(QPalette::Inactive, role, KColorScheme(QPalette::Inactive, set).background(newRole));
    palette.setBrush(QPalette::Disabled, role, KColorScheme(QPalette::Disabled, set).background(newRole));
}



void KateSearchBar::indicateMatch(bool wrapped) {
    if (m_incUi != NULL) {
        // Green background for line edit
        QPalette background(m_incUi->pattern->palette());
        adjustBackground(background, KColorScheme::PositiveBackground);
        m_incUi->pattern->setPalette(background);

        // Update status label
        m_incUi->status->setText(wrapped
                ? i18n("Reached bottom, continued from top")
                : "");
    } else {
        // TODO
    }
}



void KateSearchBar::indicateMismatch() {
    if (m_incUi != NULL) {
        // Red background for line edit
        QPalette background(m_incUi->pattern->palette());
        adjustBackground(background, KColorScheme::NegativeBackground);
        m_incUi->pattern->setPalette(background);

        // Update status label
        m_incUi->status->setText(i18n("Not found"));
    } else {
        // TODO
    }
}



void KateSearchBar::indicateNothing() {
    if (m_incUi != NULL) {
        // Reset background of line edit
        QColor color = QPalette().color(QPalette::Base);
        QPalette background(m_incUi->pattern->palette());
        background.setBrush(QPalette::Active, QPalette::Base, QPalette().brush(QPalette::Active, QPalette::Base));
        background.setBrush(QPalette::Inactive, QPalette::Base, QPalette().brush(QPalette::Inactive, QPalette::Base));
        background.setBrush(QPalette::Disabled, QPalette::Base, QPalette().brush(QPalette::Disabled, QPalette::Base));
        m_incUi->pattern->setPalette(background);

        // Update status label
        m_incUi->status->setText("");
    } else {
        // TODO
    }
}



void KateSearchBar::selectRange(const KTextEditor::Range & range) {
    m_view->setCursorPositionInternal(range.start(), 1);
    m_view->setSelection(range);
}



void KateSearchBar::buildReplacement(QString & output, QList<ReplacementPart> & parts, const QVector<Range> & details) {
    const int MIN_REF_INDEX = 0;
    const int MAX_REF_INDEX = details.count() - 1;

    output.clear();
    for (QList<ReplacementPart>::iterator iter = parts.begin(); iter != parts.end(); iter++) {
        ReplacementPart & curPart = *iter;
        if (curPart.isReference) {
            if ((curPart.index < MIN_REF_INDEX) || (curPart.index > MAX_REF_INDEX)) {
                // Insert just the number to be consistent with QRegExp ("\c" becomes "c")
                output.append(QString::number(curPart.index));
            } else {
                const Range & captureRange = details[curPart.index];
                if (captureRange.isValid()) {
                    // Copy capture content
                    const bool blockMode = m_view->blockSelection();
                    output.append(m_view->doc()->text(captureRange, blockMode));
                }
            }
        } else {
            output.append(curPart.text);
        }
    }
}



void KateSearchBar::replaceMatch(const QVector<Range> & match, const QString & replacement) {
    const bool usePlaceholders = isChecked(m_powerUi->usePlaceholders);
    const Range & targetRange = match[0];

    QString finalReplacement;
    if (usePlaceholders) {
        // Resolve references and escape sequences
        QList<ReplacementPart> parts;
        QString writableHack(replacement);
        KateDocument::escapePlaintext(writableHack, &parts);
        buildReplacement(finalReplacement, parts, match);
    } else {
        // Plain text replacement
        finalReplacement = replacement;
    }

    const bool blockMode = (m_view->blockSelection() && !targetRange.onSingleLine());
    m_view->doc()->replaceText(targetRange, finalReplacement, blockMode);
}



void KateSearchBar::onIncPatternChanged(const QString & pattern) {
    if (pattern.isEmpty()) {
        // Kill selection
        m_view->setSelection(Range::invalid());

        // Reset edit color
        indicateNothing();

        // Disable next/prev
        m_incUi->next->setDisabled(true);
        m_incUi->prev->setDisabled(true);
        return;
    }

    // Enable next/prev
    m_incUi->next->setDisabled(false);
    m_incUi->prev->setDisabled(false);


    // How to find?
    Search::SearchOptions enabledOptions(KTextEditor::Search::Default);
    const bool matchCase = isChecked(m_incMenuMatchCase);
    if (!matchCase) {
        enabledOptions |= Search::CaseInsensitive;
    }


    // Where to find?
    Range inputRange;
    const bool fromCursor = isChecked(m_incMenuFromCursor);
    if (fromCursor) {
        inputRange.setRange(m_incInitCursor, m_view->doc()->documentEnd());
    } else {
        inputRange = m_view->doc()->documentRange();
    }

    // Find, first try
    QVector<Range> resultRanges = m_view->doc()->searchText(inputRange, pattern, enabledOptions);
    const Range & match = resultRanges[0];

    if (match.isValid()) {
        resetHighlights();
        highlightMatch(match);
        selectRange(match);
        const bool NOT_WRAPPED = false;
        indicateMatch(NOT_WRAPPED);
    } else {
        // Wrap if it makes sense
        if (fromCursor) {
            // Find, second try
            inputRange = m_view->doc()->documentRange();
            QVector<Range> resultRanges2 = m_view->doc()->searchText(inputRange, pattern, enabledOptions);
            const Range & match2 = resultRanges2[0];
            if (match2.isValid()) {
                resetHighlights();
                highlightMatch(match2);
                selectRange(match2);
                const bool WRAPPED = true;
                indicateMatch(WRAPPED);
            } else {
                indicateMismatch();
            }
        } else {
            indicateMismatch();
        }
    }
}



void KateSearchBar::onIncNext() {
    const bool FIND = false;
    onStep(FIND);
}



void KateSearchBar::onIncPrev() {
    const bool FIND = false;
    const bool BACKWARDS = false;
    onStep(FIND, BACKWARDS);
}



void KateSearchBar::onIncMatchCaseToggle(bool invokedByUserAction) {
    if (invokedByUserAction) {
        sendConfig();
    }
}



void KateSearchBar::onIncHighlightAllToggle(bool invokedByUserAction) {
    if (invokedByUserAction) {
        sendConfig();
    }
}



void KateSearchBar::onIncFromCursorToggle(bool invokedByUserAction) {
    if (invokedByUserAction) {
        sendConfig();
    }
}



void KateSearchBar::fixForSingleLine(Range & range, bool forwards) {
    kDebug() << "Single-line workaround checking" << range;
    if (forwards) {
        const int line = range.start().line();
        const int col = range.start().column();
        const int maxColWithNewline = m_view->doc()->lineLength(line);
        if (col == maxColWithNewline) {
            kDebug() << "Starting on a newline" << range;
            const int maxLine = m_view->doc()->lines() - 1;
            if (line < maxLine) {
                range.setRange(Cursor(line + 1, 0), range.end());                
                kDebug() << "Search range fixed to " << range;
            } else {
                // Already at last line
                range = Range::invalid();
            }
        }
    } else {
        const int col = range.end().column();
        if (col == 0) {
            kDebug() << "Ending after a newline" << range;
            const int line = range.end().line();
            if (line > 0) {
                const int maxColWithNewline = m_view->doc()->lineLength(line - 1);
                range.setRange(range.start(), Cursor(line - 1, maxColWithNewline));
                kDebug() << "Search range fixed to " << range;
            } else {
                // Already at first line
                range = Range::invalid();
            }
        }
    }
}



void KateSearchBar::onStep(bool replace, bool forwards) {
    // What to find?
    const QString pattern = (m_powerUi != NULL)
            ? m_powerUi->pattern->currentText()
            : m_incUi->pattern->displayText();


    // How to find?
    Search::SearchOptions enabledOptions(KTextEditor::Search::Default);
    const bool matchCase = (m_powerUi != NULL)
            ? isChecked(m_powerUi->matchCase)
            : isChecked(m_incMenuMatchCase);
    if (!matchCase) {
        enabledOptions |= Search::CaseInsensitive;
    }

    if (!forwards) {
        enabledOptions |= Search::Backwards;
    }

    bool multiLinePattern = false;
    bool regexMode = false;
    if (m_powerUi != NULL) {
        switch (m_powerUi->searchMode->currentIndex()) {
        case MODE_WHOLE_WORDS:
            enabledOptions |= Search::WholeWords;
            break;

        case MODE_ESCAPE_SEQUENCES:
            enabledOptions |= Search::EscapeSequences;
            break;

        case MODE_REGEX:
            {
                // Check if pattern multi-line
                QString patternCopy(pattern);
                KateDocument::repairPattern(patternCopy, multiLinePattern);
                regexMode = true;
            }
            enabledOptions |= Search::Regex;
            break;

        case MODE_PLAIN_TEXT: // FALLTHROUGH
        default:
            break;

        }
    }


    // Where to find?
    Range inputRange;
    Range selection;
    const bool selected = m_view->selection();
    const bool selectionOnly = (m_powerUi != NULL)
            ? isChecked(m_powerUi->selectionOnly)
            : false;
    if (selected) {
        selection = m_view->selectionRange();
        if (selectionOnly) {
            // First match in selection
            inputRange = selection;
        } else {
            // Next match after/before selection if a match was selected before
            if (forwards) {
                inputRange.setRange(selection.start(), m_view->doc()->documentEnd());
            } else {
                inputRange.setRange(Cursor(0, 0), selection.end());
            }
        }
    } else {
        // No selection
        const bool fromCursor = (m_powerUi != NULL)
                ? isChecked(m_powerUi->fromCursor)
                : isChecked(m_incMenuFromCursor);
        if (fromCursor) {
            const Cursor cursorPos = m_view->cursorPosition();
            if (forwards) {
                inputRange.setRange(cursorPos, m_view->doc()->documentEnd());
            } else {
                inputRange.setRange(Cursor(0, 0), cursorPos);
            }
        } else {
            inputRange = m_view->doc()->documentRange();
        }
    }
    kDebug() << "Search range is" << inputRange;

    // Single-line pattern workaround
    if (regexMode && !multiLinePattern) {
        fixForSingleLine(inputRange, forwards);
    }


    // Find, first try
    const QVector<Range> resultRanges = m_view->doc()->searchText(inputRange, pattern, enabledOptions);
    const Range & match = resultRanges[0];
    bool wrap = false;
    if (match.isValid()) {
        // Previously selected match again?
        if (selected && !selectionOnly && (match == selection)) {
            // Same match again
            if (replace) {
                // Selection is match -> replace
                const QString replacement = m_powerUi->replacement->currentText();
                resetHighlights();
                highlightReplacement(match);
                replaceMatch(resultRanges, replacement);
            } else {
                // Find, second try after old selection
                if (forwards) {
                    inputRange.setRange(selection.end(), inputRange.end());
                } else {
                    inputRange.setRange(inputRange.start(), selection.start());
                }

                // Single-line pattern workaround
                fixForSingleLine(inputRange, forwards);

                const QVector<Range> resultRanges2 = m_view->doc()->searchText(inputRange, pattern, enabledOptions);
                const Range & match2 = resultRanges2[0];
                if (match2.isValid()) {
                    resetHighlights();
                    highlightMatch(match2);
                    selectRange(match2);
                    const bool NOT_WRAPPED = false;
                    indicateMatch(NOT_WRAPPED);
                } else {
                    // Find, third try from doc start on
                    wrap = true;
                }
            }
        } else {
            resetHighlights();
            highlightMatch(match);
            selectRange(match);
            const bool NOT_WRAPPED = false;
            indicateMatch(NOT_WRAPPED);
        }
    } else if (!selected || !selectionOnly) {
        // Find, second try from doc start on
        wrap = true;
    }

    // Wrap around
    if (wrap) {
        inputRange = m_view->doc()->documentRange();
        const QVector<Range> resultRanges3 = m_view->doc()->searchText(inputRange, pattern, enabledOptions);
        const Range & match3 = resultRanges3[0];
        if (match3.isValid()) {
            // Previously selected match again?
            if (selected && !selectionOnly && (match3 == selection)) {
                // NOOP, same match again
            } else {
                resetHighlights();
                highlightMatch(match3);
                selectRange(match3);
            }
            const bool WRAPPED = true;
            indicateMatch(WRAPPED);
        } else {
            indicateMismatch();
        }
    }
}



void KateSearchBar::onPowerPatternChanged(const QString & pattern) {
    if (pattern.isEmpty()) {
        // Disable next/prev and replace next/all
        m_powerUi->findNext->setDisabled(true);
        m_powerUi->findPrev->setDisabled(true);
        m_powerUi->replaceNext->setDisabled(true);
        m_powerUi->replaceAll->setDisabled(true);
    } else {
        // Enable next/prev and replace next/all
        m_powerUi->findNext->setDisabled(false);
        m_powerUi->findPrev->setDisabled(false);
        m_powerUi->replaceNext->setDisabled(false);
        m_powerUi->replaceAll->setDisabled(false);
    }
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
        m_powerFromCursor = isChecked(m_powerUi->fromCursor);
        m_powerHighlightAll = isChecked(m_powerUi->highlightAll);
        m_powerUsePlaceholders = isChecked(m_powerUi->usePlaceholders);
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
            | (m_powerUsePlaceholders ? KateViewConfig::PowerUsePlaceholders : 0)
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
                    | KateViewConfig::PowerUsePlaceholders
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
    kDebug() << "LOCAL SEARCH CONFIG COPIED TO GLOBAL" << "past" << pastFlags << "future" << futureFlags;
}



void KateSearchBar::onPowerFindNext() {
    const bool FIND = false;
    onStep(FIND);

    // Add to search history
    addCurrentTextToHistory(m_powerUi->pattern);
}



void KateSearchBar::onPowerFindPrev() {
    const bool FIND = false;
    const bool BACKWARDS = false;
    onStep(FIND, BACKWARDS);

    // Add to search history
    addCurrentTextToHistory(m_powerUi->pattern);
}



void KateSearchBar::onPowerReplaceNext() {
    const bool REPLACE = true;
    onStep(REPLACE);

    // Add to search history
    addCurrentTextToHistory(m_powerUi->pattern);

    // Add to replace history
    addCurrentTextToHistory(m_powerUi->replacement);
}



void KateSearchBar::onPowerReplaceAll() {
    // What to find/replace?
    const QString pattern = m_powerUi->pattern->currentText();
    const QString replacement = m_powerUi->replacement->currentText();


    // How to find?
    Search::SearchOptions enabledOptions(KTextEditor::Search::Default);
    const bool matchCase = isChecked(m_powerUi->matchCase);
    if (!matchCase) {
        enabledOptions |= Search::CaseInsensitive;
    }

    bool multiLinePattern = false;
    bool regexMode = false;
    if (m_powerUi != NULL) {
        switch (m_powerUi->searchMode->currentIndex()) {
        case MODE_WHOLE_WORDS:
            enabledOptions |= Search::WholeWords;
            break;

        case MODE_ESCAPE_SEQUENCES:
            enabledOptions |= Search::EscapeSequences;
            break;

        case MODE_REGEX:
            {
                // Check if pattern multi-line
                QString patternCopy(pattern);
                KateDocument::repairPattern(patternCopy, multiLinePattern);
                regexMode = true;
            }
            enabledOptions |= Search::Regex;
            break;

        case MODE_PLAIN_TEXT: // FALLTHROUGH
        default:
            break;

        }
    }


    // Where to replace?
    Range selection;
    const bool selected = m_view->selection();
    const bool selectionOnly = isChecked(m_powerUi->selectionOnly);
    Range inputRange = (selected && selectionOnly)
            ? m_view->selectionRange()
            : m_view->doc()->documentRange();


    // Collect matches
    QList<QVector<KTextEditor::Range> > replacementJobs;
    for (;;) {
        const QVector<Range> resultRanges = m_view->doc()->searchText(inputRange, pattern, enabledOptions);
        const Range & match = resultRanges[0];
        if (!match.isValid()) {
            break;
        }

        replacementJobs.append(resultRanges);

        // Continue after match
        if (match.start() == match.end()) {
            // Can happen for regex patterns like "^".
            // If we don't advance here we will loop forever...
            const int line = match.end().line();
            const int col = match.end().column();
            const int maxColWithNewline = m_view->doc()->lineLength(line);
            if (col < maxColWithNewline) {
                // Advance on same line
                inputRange.setRange(Cursor(line, col + 1), inputRange.end());
            } else {
                // Advance to next line
                const int maxLine = m_view->doc()->lines() - 1;
                if (line < maxLine) {
                    // Next line
                    inputRange.setRange(Cursor(line + 1, 0), inputRange.end());
                } else {
                    // Already at last line
                    break;
                }
            }
        } else {
            inputRange.setRange(match.end(), inputRange.end());
        }

        // Single-line pattern workaround
        if (regexMode && !multiLinePattern) {
            const bool FORWARDS = true;
            fixForSingleLine(inputRange, FORWARDS);
        }

        if (!inputRange.isValid()) {
            break;
        }
    }


    // Replace (backwards)
    if (!replacementJobs.isEmpty()) {
        resetHighlights();
        m_view->doc()->editBegin(); // Group to single undo job
        QList<QVector<Range> >::iterator iter = replacementJobs.end();
        while (iter != replacementJobs.begin()) {
            --iter;
            const QVector<Range> & targetDetails = *iter;
            const Range & targetRange = targetDetails[0];
            highlightReplacement(targetRange);
            replaceMatch(targetDetails, replacement);
        }
        m_view->doc()->editEnd();
    }


    // Add to search history
    addCurrentTextToHistory(m_powerUi->pattern);

    // Add to replace history
    addCurrentTextToHistory(m_powerUi->replacement);
}



void KateSearchBar::addMenuEntry(QMenu * menu, QVector<QString> & insertBefore, QVector<QString> & insertAfter,
        uint & walker, const QString & before, const QString after, const QString description,
        const QString & realBefore, const QString & realAfter) {
    QAction * const action = menu->addAction(before + after + "\t" + description);
    insertBefore[walker] = QString(realBefore.isEmpty() ? before : realBefore);
    insertAfter[walker] = QString(realAfter.isEmpty() ? after : realAfter);
    action->setData(QVariant(walker++));
}



void KateSearchBar::showAddMenu(bool forPattern) {
    QVector<QString> insertBefore(35);
    QVector<QString> insertAfter(35);
    uint walker = 0;

    // Build menu
    QMenu * const popupMenu = new QMenu();
    const bool regexMode = (m_powerUi->searchMode->currentIndex() == MODE_REGEX);

    if (forPattern) {
        if (regexMode) {
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "^", "", i18n("Beginning of line"));
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "$", "", i18n("End of line"));
            popupMenu->addSeparator();
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, ".", "", i18n("Any single character (excluding line breaks)"));
            popupMenu->addSeparator();
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "+", "", i18n("One or more occurences"));
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "*", "", i18n("Zero or more occurences"));
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "?", "", i18n("Zero or one occurences"));
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "{a", ",b}", i18n("<a> through <b> occurences"), "{", ",}");
            popupMenu->addSeparator();
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "(", ")", i18n("Group, capturing"));
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "|", "", i18n("Or"));
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "[", "]", i18n("Set of characters"));
            addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "[^", "]", i18n("Negative set of characters"));
            popupMenu->addSeparator();
        }
    } else {
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\0", "", i18n("Whole match reference"));
        popupMenu->addSeparator();
        // TODO Add pattern text for each capture as requested by bug #148359
        // http://bugs.kde.org/show_bug.cgi?id=148359
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\1", "", i18n("Capture reference 1"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\2", "", i18n("Capture reference 2"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\3", "", i18n("Capture reference 3"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\4", "", i18n("Capture reference 4"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\5", "", i18n("Capture reference 5"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\6", "", i18n("Capture reference 6"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\7", "", i18n("Capture reference 7"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\8", "", i18n("Capture reference 8"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\9", "", i18n("Capture reference 9"));
        popupMenu->addSeparator();
    }

    addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\n", "", i18n("Line break"));
    addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\t", "", i18n("Tab"));

    if (forPattern && regexMode) {
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\b", "", i18n("Word boundary"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\B", "", i18n("Not word boundary"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\d", "", i18n("Digit"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\D", "", i18n("Non-digit"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\s", "", i18n("Whitespace (excluding line breaks)"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\S", "", i18n("Non-whitespace (excluding line breaks)"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\w", "", i18n("Word character (alphanumerics plus '_')"));
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\W", "", i18n("Non-word character"));
    }

    addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\0???", "", i18n("Octal character 000 to 377 (2^8-1)"), "\\0");
    addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\x????", "", i18n("Hex character 0000 to FFFF (2^16-1)"), "\\x");
    addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "\\\\", "", i18n("Backslash"));

    if (forPattern && regexMode) {
        popupMenu->addSeparator();
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "(?:E", ")", i18n("Group, non-capturing"), "(?:");
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "(?=E", ")", i18n("Lookahead"), "(?=");
        addMenuEntry(popupMenu, insertBefore, insertAfter, walker, "(?!E", ")", i18n("Negative lookahead"), "(?!");
    }


    // Show menu
    const QPoint topLeftGlobal = m_powerUi->patternAdd->mapToGlobal(QPoint(0, 0));
    QAction * const result = popupMenu->exec(topLeftGlobal);
    if (result != NULL) {
        QLineEdit * const lineEdit = forPattern
                ? m_powerUi->pattern->lineEdit()
                : m_powerUi->replacement->lineEdit();
        Q_ASSERT(lineEdit != NULL);
        const int cursorPos = lineEdit->cursorPosition();
        const int index = result->data().toUInt();
        const QString & before = insertBefore[index];
        const QString & after = insertAfter[index];
        lineEdit->insert(before + after);
        lineEdit->setCursorPosition(cursorPos + before.count());
        lineEdit->setFocus();
    }


    // Kill menu
    delete popupMenu;
}



void KateSearchBar::onPowerAddToPatternClicked() {
    const bool FOR_PATTERN = true;
    showAddMenu(FOR_PATTERN);
}



void KateSearchBar::onPowerAddToReplacementClicked() {
    const bool FOR_REPLACEMENT = false;
    showAddMenu(FOR_REPLACEMENT);
}



void KateSearchBar::onPowerUsePlaceholdersToggle(int state, bool invokedByUserAction) {
    const bool disabled = (state != Qt::Checked);
    m_powerUi->replacementAdd->setDisabled(disabled);

    if (invokedByUserAction) {
        sendConfig();
    }
}



void KateSearchBar::onPowerMatchCaseToggle(bool invokedByUserAction) {
    if (invokedByUserAction) {
        sendConfig();
    }
}



void KateSearchBar::onPowerHighlightAllToggle(bool invokedByUserAction) {
    if (invokedByUserAction) {
        sendConfig();
    }
}



void KateSearchBar::onPowerFromCursorToggle(bool invokedByUserAction) {
    if (invokedByUserAction) {
        sendConfig();
    }
}



void KateSearchBar::onPowerModeChanged(int index, bool invokedByUserAction) {
    const bool disabled = (index == MODE_PLAIN_TEXT)
            || (index == MODE_WHOLE_WORDS);
    m_powerUi->patternAdd->setDisabled(disabled);

    if (invokedByUserAction) {
        switch (index) {
        case MODE_REGEX:
            setChecked(m_powerUi->matchCase, true);
            // FALLTROUGH

        case MODE_ESCAPE_SEQUENCES:
            setChecked(m_powerUi->usePlaceholders, true);
            break;

        case MODE_WHOLE_WORDS: // FALLTROUGH
        case MODE_PLAIN_TEXT: // FALLTROUGH
        default:
            ; // NOOP
        }

        sendConfig();
    }
}



void KateSearchBar::onMutatePower() {
    // Coming from power search?
    const bool fromReplace = (m_powerUi != NULL) && (m_widget->isVisible());
    if (fromReplace) {
        m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
        return;
    }

    // Coming from incremental search?
    const bool fromIncremental = (m_incUi != NULL) && (m_widget->isVisible());
    QString initialPattern;
    if (fromIncremental) {
        initialPattern = m_incUi->pattern->displayText();
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
            delete m_widget;
        }

        // Add power widget
        m_widget = new QWidget;
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

        // Icons
        m_powerUi->mutate->setIcon(KIcon("arrow-down-double"));
        m_powerUi->findNext->setIcon(KIcon("go-down"));
        m_powerUi->findPrev->setIcon(KIcon("go-up"));
        m_powerUi->patternAdd->setIcon(KIcon("list-add"));
        m_powerUi->replacementAdd->setIcon(KIcon("list-add"));

        // Disable still to implement controls
        // TODO
        m_powerUi->highlightAll->setDisabled(true);

        // Focus proxy
        centralWidget()->setFocusProxy(m_powerUi->pattern);
    }

    // Guess settings from context
    const bool selected = m_view->selection();
    bool selectionOnly = false;
    if (!fromIncremental) {
        // Init pattern with current selection
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
    }
    setChecked(m_powerUi->selectionOnly, selectionOnly);

    // Restore previous settings
    if (create) {
        setChecked(m_powerUi->matchCase, m_powerMatchCase);
        setChecked(m_powerUi->highlightAll, m_powerHighlightAll);
        setChecked(m_powerUi->usePlaceholders, m_powerUsePlaceholders);
        setChecked(m_powerUi->fromCursor, m_powerFromCursor);
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
    const bool NOT_INVOKED_BY_USER_ACTION = false;
    onPowerUsePlaceholdersToggle(m_powerUi->usePlaceholders->checkState(), NOT_INVOKED_BY_USER_ACTION);
    onPowerModeChanged(m_powerUi->searchMode->currentIndex(), NOT_INVOKED_BY_USER_ACTION);

    if (create) {
        // Slots
        connect(m_powerUi->mutate, SIGNAL(clicked()), this, SLOT(onMutateIncremental()));
        connect(m_powerUi->pattern, SIGNAL(textChanged(const QString &)), this, SLOT(onPowerPatternChanged(const QString &)));
        connect(m_powerUi->findNext, SIGNAL(clicked()), this, SLOT(onPowerFindNext()));
        connect(m_powerUi->findPrev, SIGNAL(clicked()), this, SLOT(onPowerFindPrev()));
        connect(m_powerUi->replaceNext, SIGNAL(clicked()), this, SLOT(onPowerReplaceNext()));
        connect(m_powerUi->replaceAll, SIGNAL(clicked()), this, SLOT(onPowerReplaceAll()));
        connect(m_powerUi->searchMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onPowerModeChanged(int)));
        connect(m_powerUi->patternAdd, SIGNAL(clicked()), this, SLOT(onPowerAddToPatternClicked()));
        connect(m_powerUi->usePlaceholders, SIGNAL(stateChanged(int)), this, SLOT(onPowerUsePlaceholdersToggle(int)));
        connect(m_powerUi->matchCase, SIGNAL(stateChanged(int)), this, SLOT(onPowerMatchCaseToggle()));
        connect(m_powerUi->highlightAll, SIGNAL(stateChanged(int)), this, SLOT(onPowerHighlightAllToggle()));
        connect(m_powerUi->fromCursor, SIGNAL(stateChanged(int)), this, SLOT(onPowerFromCursorToggle()));
        connect(m_powerUi->replacementAdd, SIGNAL(clicked()), this, SLOT(onPowerAddToReplacementClicked()));

        // Make [return] in pattern line edit trigger <find next> action
        connect(patternLineEdit, SIGNAL(returnPressed()), this, SLOT(onPowerFindNext()));
    }

    // Focus
    m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
}



void KateSearchBar::onMutateIncremental() {
    // Coming from incremental search?
    const bool fromIncremental = (m_incUi != NULL) && (m_widget->isVisible());
    QString initialPattern;
    if (fromIncremental) {
        m_incUi->pattern->setFocus(Qt::MouseFocusReason);
        return;
    }

    // Coming from power search?
    const bool fromReplace = (m_powerUi != NULL) && (m_widget->isVisible());
    if (fromReplace) {
        initialPattern = m_powerUi->pattern->currentText();
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
            delete m_widget;
        }

        // Add incremental widget
        m_widget = new QWidget;
        m_incUi = new Ui::IncrementalSearchBar;
        m_incUi->setupUi(m_widget);
        m_layout->addWidget(m_widget);

        // Fill options menu
        m_incMenu = new QMenu();
        m_incUi->options->setMenu(m_incMenu);
        m_incMenuMatchCase = m_incMenu->addAction(i18n("&Match case"));
        m_incMenuMatchCase->setCheckable(true);
        m_incMenuFromCursor = m_incMenu->addAction(i18n("From &cursor"));
        m_incMenuFromCursor->setCheckable(true);
        m_incMenuHighlightAll = m_incMenu->addAction(i18n("Hi&ghlight all"));
        m_incMenuHighlightAll->setCheckable(true);
        m_incMenuHighlightAll->setDisabled(true);

        // Icons
        m_incUi->mutate->setIcon(KIcon("arrow-up-double"));
        m_incUi->next->setIcon(KIcon("go-down"));
        m_incUi->prev->setIcon(KIcon("go-up"));

        // Focus proxy
        centralWidget()->setFocusProxy(m_incUi->pattern);
    }

    // Guess settings from context
    // NOOP

    // Restore previous settings
    if (create) {
        setChecked(m_incMenuHighlightAll, m_incHighlightAll);
        setChecked(m_incMenuFromCursor, m_incFromCursor);
        setChecked(m_incMenuMatchCase, m_incMatchCase);
    }

    // Set initial search pattern
    m_incUi->pattern->setText(initialPattern);
    m_incUi->pattern->selectAll();

    // Propagate settings (slots are still inactive on purpose)
    onIncPatternChanged(initialPattern);

    if (create) {
        // Slots
        connect(m_incUi->mutate, SIGNAL(clicked()), this, SLOT(onMutatePower()));
        connect(m_incUi->pattern, SIGNAL(returnPressed()), this, SLOT(onIncNext()));
        connect(m_incUi->pattern, SIGNAL(textChanged(const QString &)), this, SLOT(onIncPatternChanged(const QString &)));
        connect(m_incUi->next, SIGNAL(clicked()), this, SLOT(onIncNext()));
        connect(m_incUi->prev, SIGNAL(clicked()), this, SLOT(onIncPrev()));
        connect(m_incMenuMatchCase, SIGNAL(changed()), this, SLOT(onIncMatchCaseToggle()));
        connect(m_incMenuFromCursor, SIGNAL(changed()), this, SLOT(onIncFromCursorToggle()));
        connect(m_incMenuHighlightAll, SIGNAL(changed()), this, SLOT(onIncHighlightAllToggle()));

        // Make button click open the menu as well. IMHO with the dropdown arrow present the button
        // better shows his nature than in instant popup mode.
        connect(m_incUi->options, SIGNAL(clicked()), m_incUi->options, SLOT(showMenu()));
    }

    // Focus
    m_incUi->pattern->setFocus(Qt::MouseFocusReason);
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



void KateSearchBar::enableHighlights(bool enable) {
    if (enable) {
        m_view->addInternalHighlight(m_topRange);
    } else {
        m_view->removeInternalHighlight(m_topRange);
        m_topRange->deleteChildRanges();
    }
}



void KateSearchBar::resetHighlights() {
    enableHighlights(false);
    enableHighlights(true);
}



void KateSearchBar::showEvent(QShowEvent * event) {
    // Update init cursor
    if (m_incUi != NULL) {
        m_incInitCursor = m_view->cursorPosition();
    }

    enableHighlights(true);
    KateViewBarWidget::showEvent(event);
}



void KateSearchBar::hideEvent(QHideEvent * event) {
    enableHighlights(false);
    KateViewBarWidget::hideEvent(event);
}



#include "katesearchbar.moc"

