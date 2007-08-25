/* ##################################################################
## 
##  PLEASE LEAVE THE OLD CODE BELOW: I WILL DO CLEANUP LATER,
##  I PROMISE ;-)
##
##  Sebastian Pipping (sping)
##
################################################################## */


/* This file is part of the KDE libraries
   Copyright (C) 2006 Andreas Kling <kling@impul.se>

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
#include <QtGui/QVBoxLayout>
#include <QtGui/QCheckBox>

using namespace KTextEditor;



KateSearchBar::KateSearchBar(KateViewBar * viewBar)
        : KateViewBarWidget(viewBar),
        m_view(viewBar->view()),
        m_layout(new QVBoxLayout()),
        m_widget(NULL),
        m_incUi(NULL),
        m_incMenu(NULL),
        m_incMenuMatchCase(NULL),
        m_incMenuFromCursor(NULL),
        m_incMenuHighlightAll(NULL),
        m_incInitCursor(0, 0),
        m_powerUi(NULL),
        m_topRange(NULL) {
    QWidget * const widget = centralWidget();
    widget->setLayout(m_layout);

    // Start in incremental mode
    mutateIncremental();
    // mutatePower();

    m_layout->setMargin(2);

    enableHighlights(true);
}



KateSearchBar::~KateSearchBar() {
    // TODO
}



void KateSearchBar::findNext() {
    // TODO remove need for this function?
}



void KateSearchBar::findPrevious() {
    // TODO remove need for this function?
}



void KateSearchBar::onMutate() {
    if (m_incUi == NULL) {
        mutateIncremental();
    } else {
        mutatePower();
    }
}



void KateSearchBar::highlightMatch(const Range & range) {
// TODO Highlight code does not work as expected!?
/*
    SmartRange * const highlight = m_view->doc()->newSmartRange(range, m_topRange);
    Attribute::Ptr color(new Attribute()); // TODO
    color->setBackground(QColor("yellow")); // TODO make this part of the color scheme
    highlight->setAttribute(color);
*/
}



void KateSearchBar::highlightReplacement(const Range & range) {
// TODO Highlight code does not work as expected!?
/*
    SmartRange * const highlight = m_view->doc()->newSmartRange(range, m_topRange);
    Attribute::Ptr color(new Attribute()); // TODO
    color->setBackground(QColor("green")); // TODO make this part of the color scheme
    highlight->setAttribute(color);
*/
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
// TODO indicate wrap match
            } else {
// TODO indicate wrap error
            }
        } else {
// TODO indicate wrap error
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

        case 3: // Regegular expression
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
                } else {
                    // Find, third try from doc start on
                    wrap = true;
                }
            }
        } else {
            resetHighlights();
            highlightMatch(match);
            selectRange(match);
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
// TODO indicate wrap match
        } else {
// TODO indicate wrap error
        }
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

        case 3: // Regegular expression
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



void KateSearchBar::mutatePower() {
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

    // Icons
    m_powerUi->mutate->setIcon(KIcon("arrow-down-double"));
    m_powerUi->findNext->setIcon(KIcon("go-down"));
    m_powerUi->findPrev->setIcon(KIcon("go-up"));
    m_powerUi->patternAdd->setIcon(KIcon("list-add"));
    m_powerUi->replacementAdd->setIcon(KIcon("list-add"));

    // Slots
    connect(m_powerUi->mutate, SIGNAL(clicked()), this, SLOT(onMutate()));
    connect(m_powerUi->findNext, SIGNAL(clicked()), this, SLOT(onPowerFindNext()));
    connect(m_powerUi->findPrev, SIGNAL(clicked()), this, SLOT(onPowerFindPrev()));
    connect(m_powerUi->replaceNext, SIGNAL(clicked()), this, SLOT(onPowerReplaceNext()));
    connect(m_powerUi->replaceAll, SIGNAL(clicked()), this, SLOT(onPowerReplaceAll()));

    // Disable still to implement controls
    // TODO
    m_powerUi->patternAdd->setDisabled(true);
    m_powerUi->replacementAdd->setDisabled(true);
    m_powerUi->highlightAll->setDisabled(true);

    // Focus
    centralWidget()->setFocusProxy(m_powerUi->pattern);
    m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
}



void KateSearchBar::mutateIncremental() {
    // Kill power widget
    delete m_powerUi;
    m_powerUi = NULL;
    delete m_widget;

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
    m_incMenuFromCursor->setChecked(true);
    m_incMenuHighlightAll = m_incMenu->addAction(i18n("Hi&ghlight all"));
    m_incMenuHighlightAll->setCheckable(true);
    m_incMenuHighlightAll->setDisabled(true);

    // Icons
    m_incUi->mutate->setIcon(KIcon("arrow-up-double"));
    m_incUi->next->setIcon(KIcon("go-down"));
    m_incUi->prev->setIcon(KIcon("go-up"));

    // Slots
    connect(m_incUi->mutate, SIGNAL(clicked()), this, SLOT(onMutate()));
    connect(m_incUi->pattern, SIGNAL(returnPressed()), this, SLOT(onIncNext()));
    connect(m_incUi->pattern, SIGNAL(textChanged(const QString &)), this, SLOT(onIncPatternChanged(const QString &)));
    connect(m_incUi->next, SIGNAL(clicked()), this, SLOT(onIncNext()));
    connect(m_incUi->prev, SIGNAL(clicked()), this, SLOT(onIncPrev()));

    // Make button click open the menu as well. IMHO with the dropdown arrow present the button
    // better shows his nature than in instant popup mode.
    connect(m_incUi->options, SIGNAL(clicked()), m_incUi->options, SLOT(showMenu()));

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



void KateSearchBar::enableHighlights(bool enable) {
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



#include "kateviewinternal.h"
#include "katesmartrange.h"

#include <kglobalsettings.h>
#include <kiconloader.h>

#include <QtGui/QBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QKeyEvent>
#include <QtGui/QPushButton> // just for replace buttons, remove if class changes
#include <QtCore/QList>

#include <kdebug.h>

class OLD_KateSearchBar_OLD::Private
{
public:
    KTextEditor::Cursor startCursor;
    KTextEditor::Range match;
    KTextEditor::Range lastMatch;
    OLD_KateSearchBar_OLDEdit *expressionEdit;
    KLineEdit * replaceEdit;

    QCheckBox *caseSensitiveBox;
    //QCheckBox *wholeWordsBox;
    QComboBox *regExpBox;

    QCheckBox *fromCursorBox;
    QCheckBox *selectionOnlyBox;
    QCheckBox *highlightAllBox;

    bool searching;
    bool wrapAround;

    KTextEditor::SmartRange* topRange;

    QString lastExpression;
    QVector<KTextEditor::Range> lastResultRanges; // TODO reset on document change so we don't get wrong data when asking for the ranges content?
};

OLD_KateSearchBar_OLD::OLD_KateSearchBar_OLD(KateViewBar *viewBar)
    : KateViewBarWidget (viewBar),
      m_view(viewBar->view()),
      d(new Private)
{
    centralWidget()->setFont(KGlobalSettings::toolBarFont());

    d->searching = false;
    d->wrapAround = true;

    d->expressionEdit = new OLD_KateSearchBar_OLDEdit;
    connect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));
    connect(d->expressionEdit, SIGNAL(returnPressed()), this, SLOT(slotSearch()));
    connect(d->expressionEdit, SIGNAL(findNext()), this, SLOT(findNext()));
    connect(d->expressionEdit, SIGNAL(findPrevious()), this, SLOT(findPrevious()));

    d->replaceEdit = new KLineEdit();

    d->caseSensitiveBox = new QCheckBox(i18n("&Case sensitive"));
    connect(d->caseSensitiveBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    //d->wholeWordsBox = new QCheckBox(i18n("&Whole words"));
    //connect(d->wholeWordsBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->regExpBox = new QComboBox();
    
    d->regExpBox->addItem (i18n("Plain Text"), KTextEditor::Search::Default);
    d->regExpBox->addItem (i18n("Whole Words"), KTextEditor::Search::WholeWords);
    d->regExpBox->addItem (i18n("Escape Sequences"), KTextEditor::Search::EscapeSequences);
    d->regExpBox->addItem (i18n("Regular Expression"), KTextEditor::Search::Regex);
    
    connect(d->regExpBox, SIGNAL(currentIndexChanged (int)), this, SLOT(slotSearch()));

    d->fromCursorBox = new QCheckBox(i18n("&From cursor"));
    connect(d->fromCursorBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->selectionOnlyBox = new QCheckBox(i18n("&Selection only"));
    connect(d->selectionOnlyBox, SIGNAL(stateChanged(int)), this, SLOT(slotSpecialOptionTogled()));

    d->highlightAllBox = new QCheckBox(i18n("&Highlight all"));
    connect(d->highlightAllBox, SIGNAL(stateChanged(int)), this, SLOT(slotSpecialOptionTogled()));

    QVBoxLayout *topLayout = new QVBoxLayout ();
    centralWidget()->setLayout(topLayout);

    // NOTE: Here be cosmetics.
    topLayout->setMargin(2);


    // first line, search field + next/prev
    QToolButton *nextButton = new QToolButton();
    nextButton->setAutoRaise(true);
    nextButton->setIcon(SmallIcon("find-next"));
    nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    nextButton->setText(i18n("Next"));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(findNext()));

    QToolButton *prevButton = new QToolButton();
    prevButton->setAutoRaise(true);
    prevButton->setIcon(SmallIcon("find-previous"));
    prevButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    prevButton->setText(i18n("Previous"));
    connect(prevButton, SIGNAL(clicked()), this, SLOT(findPrevious()));


    QPushButton * const replaceOnceButton = new QPushButton();
#ifdef __GNUC__
#warning TRANSLATE HERE
#endif
    replaceOnceButton->setText("Once");
    connect(replaceOnceButton, SIGNAL(clicked()), this, SLOT(replaceOnce()));

    QPushButton * const replaceAllButton = new QPushButton();
#ifdef __GNUC__
#warning TRANSLATE HERE
#endif
    replaceAllButton->setText("All");
    replaceAllButton->setDisabled(true);
    connect(replaceAllButton, SIGNAL(clicked()), this, SLOT(replaceAll()));

    // first line: lineedit + next + prev
    QHBoxLayout *layoutFirstLine = new QHBoxLayout;
    topLayout->addLayout (layoutFirstLine);
    layoutFirstLine->addWidget(d->expressionEdit);
    layoutFirstLine->addWidget(prevButton);
    layoutFirstLine->addWidget(nextButton);

    // second line: lineedit
    // TODO: better in grid with first line?
    QHBoxLayout * layoutSecondLine = new QHBoxLayout;
    layoutSecondLine->addWidget(d->replaceEdit);
    layoutSecondLine->addWidget(replaceOnceButton);
    layoutSecondLine->addWidget(replaceAllButton);
    topLayout->addLayout(layoutSecondLine);

    QGridLayout *gridLayout = new QGridLayout;
    topLayout->addLayout (gridLayout);

    // third line: casesensitive + whole words + regexp
    gridLayout->addWidget(d->regExpBox, 0, 0);
    gridLayout->addWidget(d->caseSensitiveBox, 0, 1);
    //gridLayout->addWidget(d->wholeWordsBox, 0, 1);
    gridLayout->addItem (new QSpacerItem(0,0), 0, 3);

    // fourth line: from cursor + selection only + highlight all
    gridLayout->addWidget(d->fromCursorBox, 1, 0);
    gridLayout->addWidget(d->selectionOnlyBox, 1, 1);
    gridLayout->addWidget(d->highlightAllBox, 1, 2);
    gridLayout->addItem (new QSpacerItem(0,0), 1, 3);

    gridLayout->setColumnStretch (3, 10);

    // set right focus proxy
    centralWidget()->setFocusProxy(d->expressionEdit);

    d->topRange = m_view->doc()->newSmartRange( m_view->doc()->documentRange() );
    d->topRange->setInsertBehavior(KTextEditor::SmartRange::ExpandRight);

    // result range vector must contain one element minimum
    d->lastResultRanges.append(KTextEditor::Range::invalid());
}

OLD_KateSearchBar_OLD::~OLD_KateSearchBar_OLD()
{
    delete d->topRange;
    delete d;
}

void OLD_KateSearchBar_OLD::doSearch(const QString &_expression, bool init, bool backwards)
{
    QString expression = _expression;

    const bool selectionOnly = d->selectionOnlyBox->checkState() == Qt::Checked;
    const bool fromCursor = d->fromCursorBox->checkState() == Qt::Checked;

    // If we're starting search for the first time, begin at the current cursor position.
    // ### there may be cases when this should happen, but does not
    if ( init )
    {
        d->startCursor = fromCursor ? m_view->cursorPosition()
                                    : selectionOnly ? m_view->selectionRange().start()
                                                    : KTextEditor::Cursor(0, 0);
        d->searching = true;

        // clear
        d->topRange->deleteChildRanges();
    }

    // combine options
    KTextEditor::Search::SearchOptions enabledOptions(KTextEditor::Search::Default);

    // which mode?
    const bool regexChecked
      = d->regExpBox->itemData (d->regExpBox->currentIndex()).toUInt() & KTextEditor::Search::Regex;
    if (regexChecked)
    {
      // regex
      enabledOptions |= KTextEditor::Search::Regex;

      // TODO
      // dot matches newline?
      const bool dotMatchesNewline = false;
      if (dotMatchesNewline)
      {
        enabledOptions |= KTextEditor::Search::DotMatchesNewline;
      }
    }
    else
    {
      // plaintext

      // whole words?
      const bool wholeWordsChecked
        = d->regExpBox->itemData (d->regExpBox->currentIndex()).toUInt() & KTextEditor::Search::WholeWords;
      if (wholeWordsChecked)
      {
        enabledOptions |= KTextEditor::Search::WholeWords;
        kDebug() << "doSearch | whole words only";
      }

      // escape sequences?    
      const bool escapeSequences = d->regExpBox->itemData(d->regExpBox->currentIndex()).toUInt()
        & KTextEditor::Search::EscapeSequences;
      if (escapeSequences)
      {
        enabledOptions |= KTextEditor::Search::EscapeSequences;
        kDebug() << "doSearch | parse escapes";
      }
    }

    // case insensitive?
    const bool caseSensitiveChecked = d->caseSensitiveBox->checkState() == Qt::Checked;
    if (!caseSensitiveChecked)
    {
      enabledOptions |= KTextEditor::Search::CaseInsensitive;
    }

    // backwards?
    if (backwards)
    {
      enabledOptions |= KTextEditor::Search::Backwards;
    }

    // find input range
    KTextEditor::Range inputRange;
    if (selectionOnly)
    {
      // inside selection only checked
      // is text selected?
      if (m_view->selection())
      {
        // selection present
        inputRange = m_view->selectionRange();

        // exclude part before cursor
        const KTextEditor::Cursor cursorPos = m_view->cursorPosition();
        if (inputRange.contains(cursorPos))
        {
          inputRange.setRange(cursorPos, inputRange.end());
        }

        // block input range?
        if (m_view->blockSelection() && !inputRange.onSingleLine())
        {
          enabledOptions |= KTextEditor::Search::BlockInputRange;
        }
      }
      else
      {
        // no selection
        if (fromCursor)
        {
          // from cursor on
          inputRange.setRange(d->startCursor, m_view->doc()->documentEnd());
        }
        else
        {
          // whole document
          inputRange = m_view->doc()->documentRange();
        }
      }
    }
    else
    {
      // whole document
      kDebug() << "doSearch | whole document";

      // is text selected?
      if (m_view->selection())
      {
        // selection present
        const KTextEditor::Range selRange = m_view->selectionRange();
        if (expression == d->lastExpression)
        {
          // prev/next was pressed -> exclude selection from input range
          if (backwards)
          {
            kDebug() << "doSearch | searching [backwards] before [selected text]";
            inputRange.setRange(d->startCursor, selRange.start());
          }
          else
          {
            kDebug() << "doSearch | searching [forwards] after [selected text]";
            inputRange.setRange(selRange.end(), m_view->doc()->documentEnd());
          }
        }
        else
        {
          // pattern has changed -> new search from original starting position
          kDebug() << "doSearch | searching RESET";
          inputRange.setRange(d->startCursor, m_view->doc()->documentEnd());
        }
      }
      else
      {
        if (backwards)
        {
          // we checked above that no selection is present which
          // also means there was no match after the starting point.
          // if the user hits "prev" now (that is why <backwards> is true here)
          // he probably wants to find a match before his original starting point.
          kDebug() << "doSearch | searching [backwards] before [cursor]";
          d->startCursor = KTextEditor::Cursor(0, 0);
          inputRange.setRange(d->startCursor, m_view->doc()->documentEnd());
        }
        else
        {
          // find first match from starting position on
          kDebug() << "doSearch | searching [forwards] after [cursor]";
          inputRange.setRange(d->startCursor, m_view->doc()->documentEnd());
        }
      }
    }
    kDebug() << "doSearch | range is " << inputRange.start() << ".." << inputRange.end();


    // run engine
    QVector<KTextEditor::Range> resultRanges = m_view->doc()->searchText(inputRange, expression, enabledOptions);
    d->match = resultRanges[0];

    bool foundMatch = d->match.isValid() && d->match != d->lastMatch && d->match != m_view->selectionRange();
    bool wrapped = false;

    if (d->wrapAround && !foundMatch)
    {
        // We found nothing, so wrap.
        d->startCursor = selectionOnly
          ? backwards ? m_view->selectionRange().end()
                      : m_view->selectionRange().start()
          : // backwards ? m_view->doc()->documentEnd() : KTextEditor::Cursor(0, 0);
            KTextEditor::Cursor(0, 0);

        // d->match = m_view->doc()->searchText(d->startCursor, regexp, backwards);
        QVector<KTextEditor::Range> resultRanges = m_view->doc()->searchText(inputRange, expression, enabledOptions);
        d->match = resultRanges[0];

        foundMatch = d->match.isValid() && d->match != d->lastMatch;
        wrapped = true;
    }

    if ( foundMatch && selectionOnly )
        foundMatch = m_view->selectionRange().contains( d->match );

    if (foundMatch)
    {
      m_view->setCursorPositionInternal(d->match.start(), 1);
      m_view->setSelection(d->match);
      d->lastMatch = d->match;

      // it makes no sense to have this enabled after a match
      if (selectionOnly)
        d->selectionOnlyBox->setCheckState(Qt::Unchecked);

      // highlight all matches
      if (d->highlightAllBox->checkState() == Qt::Checked)
      {
        // add highlight to the current match
        KTextEditor::SmartRange* sr = m_view->doc()->newSmartRange(d->match, d->topRange);
        KTextEditor::Attribute::Ptr a(new KTextEditor::Attribute());
        a->setBackground( QColor("yellow") ); //TODO make this part of the color scheme
        sr->setAttribute( a );

        // find other matches and put each in a smartrange
        bool another = true;
        bool highlightWrapped = false;
        KTextEditor::Cursor nextStart = d->match.end();
        do {
          // KTextEditor::Range r = m_view->doc()->searchText( d->startCursor, regexp, false );
          const KTextEditor::Range localRange(nextStart, m_view->doc()->documentEnd());
          QVector<KTextEditor::Range> resultRanges = m_view->doc()->searchText(localRange, expression, enabledOptions);
          const KTextEditor::Range r = resultRanges[0];

          if (r.isValid())
          {
            sr = m_view->doc()->newSmartRange(r, d->topRange);
            sr->setAttribute(a);
            nextStart = r.end();
            if ((nextStart >= m_view->doc()->documentEnd()) && !highlightWrapped)
            {
              nextStart = KTextEditor::Cursor(0, 0);
              highlightWrapped = true;
            }
            else
            {
              another = false;
            }
            //kDebug()<<"MATCH: "<<r;
          }
          else
          {
            if (!highlightWrapped)
            {
              nextStart = KTextEditor::Cursor(0, 0);
              highlightWrapped = true;
            }
            else
            {
              another = false;
            }
          }
        } while (another);
      }
    }

  d->expressionEdit->setStatus(foundMatch
    ? (wrapped
      ? OLD_KateSearchBar_OLDEdit::SearchWrapped
      : OLD_KateSearchBar_OLDEdit::Normal)
    : OLD_KateSearchBar_OLDEdit::NotFound);

  d->lastExpression = expression;
  d->lastResultRanges = resultRanges; // copy over whole vector
}

void OLD_KateSearchBar_OLD::slotSpecialOptionTogled()
{
  if ( d->selectionOnlyBox->checkState() == Qt::Checked || d->highlightAllBox->checkState() == Qt::Checked )
      disconnect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));
  else
      connect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));

  d->expressionEdit->setFocus( Qt::OtherFocusReason );
}

void OLD_KateSearchBar_OLDEdit::setStatus(Status status)
{
    m_status = status;

    QPalette pal;
    QColor col;
    switch (status)
    {
        case OLD_KateSearchBar_OLDEdit::Normal:
            col = QPalette().color(QPalette::Base);
            break;
        case OLD_KateSearchBar_OLDEdit::NotFound:
            col = QColor("lightsalmon");
            break;
        case OLD_KateSearchBar_OLDEdit::SearchWrapped:
            col = QColor("palegreen");
            break;
    }
    pal.setColor(QPalette::Base, col);
    setPalette(pal);
}

void OLD_KateSearchBar_OLD::slotSearch()
{
    // ### can we achieve this in a better way?
    if ( isVisible() )
        d->expressionEdit->setFocus( Qt::OtherFocusReason );

    if (d->expressionEdit->text().isEmpty())
    {
        kDebug()<<"reset!!";
        d->expressionEdit->setStatus(OLD_KateSearchBar_OLDEdit::Normal);
        d->searching = false;
    }
    else
        doSearch(d->expressionEdit->text(), d->highlightAllBox->checkState()==Qt::Checked||d->selectionOnlyBox->checkState()==Qt::Checked||!d->searching);

    if ( d->expressionEdit->status() == OLD_KateSearchBar_OLDEdit::NotFound)
    {
#if 0 // this hurts usability, if you try multiline search but get selected on typing the \ for test1\n for example...
        // nothing found, so select the non-matching part of the text
        if ( d->expressionEdit->text().startsWith( m_view->document()->text(d->lastMatch) ) )
            d->expressionEdit->setSelection( d->lastMatch.columnWidth(), d->expressionEdit->text().length() );
        else
            d->expressionEdit->selectAll();
#endif
    }

}

void OLD_KateSearchBar_OLD::findNext()
{
    if (d->lastMatch.isEmpty())
        return;
    // XXX d->startCursor = d->lastMatch.end();
    doSearch(d->expressionEdit->text(), false, false);
}

void OLD_KateSearchBar_OLD::findPrevious()
{
    if (d->lastMatch.isEmpty())
        return;
    // XXX d->startCursor = d->lastMatch.start();
    doSearch(d->expressionEdit->text(), false, true);
}

void OLD_KateSearchBar_OLD::replaceOnce()
{
    // replace current selection in document
    // with text from <replaceEdit>
    // TODO do a new search and replace its reseult instead

    // text selected?
    KTextEditor::Range selRange = m_view->selectionRange();
    if (!selRange.isValid())
    {
      return;
    }

    // parse escape sequences in escape string if in
    // regexp or escape sequence search mode
    const uint comboData = d->regExpBox->itemData(d->regExpBox->currentIndex()).toUInt();
    const bool processEscapeSequence = comboData & (KTextEditor::Search::Regex | KTextEditor::Search::EscapeSequences);
    QString replaceText = d->replaceEdit->text();
    if (processEscapeSequence)
    {
      const int MIN_REF_INDEX = 0;
      const int MAX_REF_INDEX = d->lastResultRanges.count() - 1;

      QList<ReplacementPart> parts;
      const bool zeroCaptureOnly = (MAX_REF_INDEX == 0);
      KateDocument::escapePlaintext(replaceText, &parts, zeroCaptureOnly);

      // build replacement text with filled in references
      replaceText.clear();
      for (QList<ReplacementPart>::iterator iter = parts.begin(); iter != parts.end(); iter++)
      {
        ReplacementPart curPart = *iter;
        if (curPart.isReference)
        {
          if ((curPart.index < MIN_REF_INDEX) || (curPart.index > MAX_REF_INDEX))
          {
            // insert just the number since "\c" becomes "c" in QRegExp
            replaceText.append(QString::number(curPart.index));
          }
          else
          {
            const KTextEditor::Range & captureRange = d->lastResultRanges[curPart.index];
            if (captureRange.isValid())
            {
              // copy capture content
              const bool blockMode = m_view->blockSelection();
              replaceText.append(m_view->doc()->text(captureRange, blockMode));
            }
          }
        }
        else
        {
          replaceText.append(curPart.text);
        }
      }
    }

    // replace
    const bool blockMode = (m_view->blockSelection() && !selRange.onSingleLine());
    m_view->doc()->replaceText(selRange, replaceText, blockMode);
}

void OLD_KateSearchBar_OLD::replaceAll()
{
    // TODO
}

void OLD_KateSearchBar_OLD::showEvent(QShowEvent *e)
{
    if ( e->spontaneous() ) return;

    d->searching = false;
    if ( d->selectionOnlyBox->checkState() == Qt::Checked && ! m_view->selection() )
      d->selectionOnlyBox->setCheckState( Qt::Unchecked );

    m_view->addInternalHighlight( d->topRange );
}

void OLD_KateSearchBar_OLD::hideEvent( QHideEvent * )
{
    m_view->removeInternalHighlight( d->topRange );
}

OLD_KateSearchBar_OLDEdit::OLD_KateSearchBar_OLDEdit(QWidget *parent)
    : KLineEdit(parent)
{
  m_status = Normal;
}

// NOTE: void keyPressEvent(QKeyEvent* ev) does not grab Qt::Key_Tab.
// No idea, why... but that's probably why we use ::event here.
bool OLD_KateSearchBar_OLDEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key())
        {
            case Qt::Key_Tab:
                emit findNext();
                return true;
            case Qt::Key_Backtab:
                emit findPrevious();
                return true;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                emit returnPressed();
                return true;
        }
    }
    return KLineEdit::event(e);
}

void OLD_KateSearchBar_OLDEdit::showEvent(QShowEvent *e)
{
    if ( e->spontaneous() ) return;

    selectAll();
}

#include "katesearchbar.moc"

