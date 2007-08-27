/* ##################################################################
##
##  TODO:
##  * Text/icon area for match/mismatch/wrap indication
##  * Search/replace history
##  * Highlight all with background thread
##  * Fix regex backward search in KateDocument?
##  * Fix highlighting of matches/replacements?
##  * Proper loading/saving of search settings
##
################################################################## */

/* This file is part of the KDE libraries
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

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
#include "ui_searchbarincremental.h"
#include "ui_searchbarpower.h"
#include <kglobalsettings.h>
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>

using namespace KTextEditor;



KateSearchBar::KateSearchBar(KateViewBar * viewBar)
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
        m_powerUi(NULL) {
    QWidget * const widget = centralWidget();
    widget->setLayout(m_layout);

    // Start in incremental mode
    onMutateIncremental();
    // onMutatePower();

    m_layout->setMargin(2);

    // TODO Do we need this?
    centralWidget()->setFont(KGlobalSettings::toolBarFont());

    enableHighlights(true);
}



KateSearchBar::~KateSearchBar() {
    // TODO
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



void KateSearchBar::highlightMatch(const Range & /*range*/) {
// TODO Highlight code does not work as expected!?
/*
    SmartRange * const highlight = m_view->doc()->newSmartRange(range, m_topRange);
    Attribute::Ptr color(new Attribute()); // TODO
    color->setBackground(QColor("yellow")); // TODO make this part of the color scheme
    highlight->setAttribute(color);
*/
}



void KateSearchBar::highlightReplacement(const Range & /*range*/) {
// TODO Highlight code does not work as expected!?
/*
    SmartRange * const highlight = m_view->doc()->newSmartRange(range, m_topRange);
    Attribute::Ptr color(new Attribute()); // TODO
    color->setBackground(QColor("green")); // TODO make this part of the color scheme
    highlight->setAttribute(color);
*/
}



void KateSearchBar::indicateMatch(bool /*wrapped*/) {
    if (m_incUi != NULL) {
        // Green background for line edit
        QColor color("palegreen");
        QPalette background;
        background.setColor(QPalette::Base, color);
        m_incUi->pattern->setPalette(background);
    } else {
        // TODO
    }
}



void KateSearchBar::indicateMismatch() {
    if (m_incUi != NULL) {
        // Red background for line edit
        QColor color("lightsalmon");
        QPalette background;
        background.setColor(QPalette::Base, color);
        m_incUi->pattern->setPalette(background);
    } else {
        // TODO
    }
}



void KateSearchBar::indicateNothing() {
    if (m_incUi != NULL) {
        // Reset background of line edit
        QColor color = QPalette().color(QPalette::Base);
        QPalette background;
        background.setColor(QPalette::Base, color);
        m_incUi->pattern->setPalette(background);
    } else {
        // TODO
    }
}



void KateSearchBar::selectRange(const KTextEditor::Range & range) {
    m_view->setCursorPositionInternal(range.start(), 1); // TODO
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



void KateSearchBar::onStep(bool replace, bool forwards) {
    // kDebug() << "KateSearchBar::onStep" << forwards;

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

    if (m_powerUi != NULL) {
        switch (m_powerUi->searchMode->currentIndex()) {
        case 1: // Whole words
            enabledOptions |= Search::WholeWords;
            break;

        case 2: // Escape sequences
            enabledOptions |= Search::EscapeSequences;
            break;

        case 3: // Regular expression
            enabledOptions |= Search::Regex;
            break;

        default: // Plain text
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
    // kDebug() << "(1) input range is" << inputRange;

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
                kDebug() << "replace range" << match;
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
                // kDebug() << "(2) input range is" << inputRange;
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



void KateSearchBar::onPowerFindNext() {
    const bool FIND = false;
    onStep(FIND);
}



void KateSearchBar::onPowerFindPrev() {
    const bool FIND = false;
    const bool BACKWARDS = false;
    onStep(FIND, BACKWARDS);
}



void KateSearchBar::onPowerReplaceNext() {
    const bool REPLACE = true;
    onStep(REPLACE);
}



void KateSearchBar::onPowerReplaceAll() {
    // kDebug() << "KateSearchBar::onPowerReplaceAll";

    // What to find/replace?
    const QString pattern = m_powerUi->pattern->currentText();
    const QString replacement = m_powerUi->replacement->currentText();


    // How to find?
    Search::SearchOptions enabledOptions(KTextEditor::Search::Default);
    const bool matchCase = isChecked(m_powerUi->matchCase);
    if (!matchCase) {
        enabledOptions |= Search::CaseInsensitive;
    }

    if (m_powerUi != NULL) {
        switch (m_powerUi->searchMode->currentIndex()) {
        case 1: // Whole words
            enabledOptions |= Search::WholeWords;
            break;

        case 2: // Escape sequences
            enabledOptions |= Search::EscapeSequences;
            break;

        case 3: // Regular expression
            enabledOptions |= Search::Regex;
            break;

        default: // Plain text
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

        // Continue after match
        replacementJobs.append(resultRanges);
        inputRange.setRange(match.end(), inputRange.end());
        if (!inputRange.isValid()) {
            break;
        }
    }


    // Replace backwards
    if (!replacementJobs.isEmpty()) {
        resetHighlights();
        QList<QVector<Range> >::iterator iter = replacementJobs.end();
        while (iter != replacementJobs.begin()) {
            --iter;
            const QVector<Range> & targetDetails = *iter;
            const Range & targetRange = targetDetails[0];
            highlightReplacement(targetRange);
            replaceMatch(targetDetails, replacement);
        }
    }
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
    const bool regexMode = (m_powerUi->searchMode->currentIndex() == 3); // TODO

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
        // TODO count indexable captures in search pattern and only show available refs?
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



void KateSearchBar::onPowerUsePlaceholdersToggle(int state) {
    const bool disabled = (state != Qt::Checked);
    m_powerUi->replacementAdd->setDisabled(disabled);
}



void KateSearchBar::onPowerModeChanged(int index) {
    const bool disabled = (index < 2); // TODO
    m_powerUi->patternAdd->setDisabled(disabled);
}



void KateSearchBar::onMutatePower() {
    QString initialPattern;

    // Re-init if existing
    if (m_powerUi != NULL) {
        if (!m_widget->isVisible()) {
            // Init pattern
            if (m_view->selection()) {
                // ... with current selection
                initialPattern = m_view->selectionText(); // TODO multi-line selection?
            }
            QLineEdit * const lineEdit = m_powerUi->pattern->lineEdit();
            Q_ASSERT(lineEdit != NULL);
            lineEdit->setText(initialPattern);
            lineEdit->selectAll();

            // Enable/disable add buttons
            onPowerUsePlaceholdersToggle(m_powerUi->usePlaceholders->checkState());
            onPowerModeChanged(m_powerUi->searchMode->currentIndex());
        }
        return;
    }


    // Initial search pattern
    if (!m_widget->isVisible()) {
        // Init pattern with current selection
        const bool selected = m_view->selection();
        if (selected) {
            initialPattern = m_view->selectionText(); // TODO multi-line selection?
        }
    } else if (m_incUi != NULL) {
        // Init pattern with old pattern from incremental dialog
        initialPattern = m_incUi->pattern->displayText();
    }

    // Kill incremental widget
    delete m_incUi;
    delete m_incMenu;
/*
    // NOTE: The menu owns them!
    delete m_incMenuMatchCase;
    delete m_incMenuFromCursor;
    delete m_incMenuHighlightAll;
*/
    m_incUi = NULL;
    m_incMenu = NULL;
    m_incMenuMatchCase = NULL;
    m_incMenuFromCursor = NULL;
    m_incMenuHighlightAll = NULL;
    delete m_widget;

    // Add power widget
    m_widget = new QWidget;
    m_powerUi = new Ui::PowerSearchBar;
    m_powerUi->setupUi(m_widget);
    m_layout->addWidget(m_widget);

    // Disable next/prev and replace next/all
    m_powerUi->findNext->setDisabled(true);
    m_powerUi->findPrev->setDisabled(true);
    m_powerUi->replaceNext->setDisabled(true);
    m_powerUi->replaceAll->setDisabled(true);

    // Icons
    m_powerUi->mutate->setIcon(KIcon("arrow-down-double"));
    m_powerUi->findNext->setIcon(KIcon("go-down"));
    m_powerUi->findPrev->setIcon(KIcon("go-up"));
    m_powerUi->patternAdd->setIcon(KIcon("list-add"));
    m_powerUi->replacementAdd->setIcon(KIcon("list-add"));

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
    connect(m_powerUi->replacementAdd, SIGNAL(clicked()), this, SLOT(onPowerAddToReplacementClicked()));

    // Disable still to implement controls
    // TODO
    m_powerUi->highlightAll->setDisabled(true);

    // Initial search pattern
    if (!initialPattern.isEmpty()) {
        QLineEdit * const lineEdit = m_powerUi->pattern->lineEdit();
        Q_ASSERT(lineEdit != NULL);
        lineEdit->setText(initialPattern);
        lineEdit->selectAll();
    }

    // Enable/disable add buttons
    onPowerUsePlaceholdersToggle(m_powerUi->usePlaceholders->checkState());
    onPowerModeChanged(m_powerUi->searchMode->currentIndex());

    // Focus
    centralWidget()->setFocusProxy(m_powerUi->pattern);
    m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
}



void KateSearchBar::onMutateIncremental() {
    // Re-init if existing
    if (m_incUi != NULL) {
        if (!m_widget->isVisible()) {
            m_incUi->pattern->setText("");
        }
        return;
    }


    // Initial search pattern
    QString initialPattern;
    if ((m_powerUi != NULL) && m_widget->isVisible()) {
        initialPattern = m_powerUi->pattern->currentText();
    }

    // Kill power widget
    delete m_powerUi;
    m_powerUi = NULL;
    delete m_widget;

    // Add incremental widget
    m_widget = new QWidget;
    m_incUi = new Ui::IncrementalSearchBar;
    m_incUi->setupUi(m_widget);
    m_layout->addWidget(m_widget);

    // Disable next/prev
    m_incUi->next->setDisabled(true);
    m_incUi->prev->setDisabled(true);

    // Fill options menu
    m_incMenu = new QMenu();
    m_incUi->options->setMenu(m_incMenu);
    m_incMenuMatchCase = m_incMenu->addAction(i18n("&Match case"));
    m_incMenuMatchCase->setCheckable(true);
    m_incMenuFromCursor = m_incMenu->addAction(i18n("From &cursor"));
    m_incMenuFromCursor->setCheckable(true);
    m_incMenuFromCursor->setChecked(true);
    m_incMenuHighlightAll = m_incMenu->addAction(i18n("Hi&ghlight all"));
    m_incMenuHighlightAll->setCheckable(true);
    m_incMenuHighlightAll->setDisabled(true);

    // Icons
    m_incUi->mutate->setIcon(KIcon("arrow-up-double"));
    m_incUi->next->setIcon(KIcon("go-down"));
    m_incUi->prev->setIcon(KIcon("go-up"));

    // Slots
    connect(m_incUi->mutate, SIGNAL(clicked()), this, SLOT(onMutatePower()));
    connect(m_incUi->pattern, SIGNAL(returnPressed()), this, SLOT(onIncNext()));
    connect(m_incUi->pattern, SIGNAL(textChanged(const QString &)), this, SLOT(onIncPatternChanged(const QString &)));
    connect(m_incUi->next, SIGNAL(clicked()), this, SLOT(onIncNext()));
    connect(m_incUi->prev, SIGNAL(clicked()), this, SLOT(onIncPrev()));

    // Make button click open the menu as well. IMHO with the dropdown arrow present the button
    // better shows his nature than in instant popup mode.
    connect(m_incUi->options, SIGNAL(clicked()), m_incUi->options, SLOT(showMenu()));

    // Initial search pattern
    if (!initialPattern.isEmpty()) {
        m_incUi->pattern->setText(initialPattern);
        m_incUi->pattern->selectAll();
    }

    // Focus
    centralWidget()->setFocusProxy(m_incUi->pattern);
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



void KateSearchBar::enableHighlights(bool /*enable*/) {
// TODO Highlight code does not work as expected!?
/*
    if (enable) {
        if (m_topRange != NULL) {
            return;
        }
        m_topRange = m_view->doc()->newSmartRange(m_view->doc()->documentRange());
        m_topRange->setInsertBehavior(SmartRange::ExpandRight);
        m_view->addInternalHighlight(m_topRange);
    } else {
        if (m_topRange == NULL) {
            return;
        }
        m_view->removeInternalHighlight(m_topRange);
        m_view->doc()->unbindSmartRange(m_topRange);
        m_view->doc()->deleteRanges();
        m_topRange = NULL;

        // TODO or somethinbg like this?
        // m_topRange->deleteChildRanges();
    }
*/
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

