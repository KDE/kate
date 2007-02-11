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
#include "katedocument.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <QHBoxLayout>
#include <QToolButton>
#include <QCheckBox>
#include <QKeyEvent>

class KateSearchBar::Private
{
public:
    KTextEditor::Cursor startCursor;
    KTextEditor::Range match;
    KTextEditor::Range lastMatch;
    KateSearchBarEdit *expressionEdit;

    QCheckBox *caseSensitiveBox;
    QCheckBox *wholeWordsBox;
    QCheckBox *regExpBox;

    QCheckBox *fromCursorBox;
    QCheckBox *selectionOnlyBox;
    QCheckBox *highlightAllBox;

    QRegExp regExp;

    bool searching;
    bool wrapAround;
};

KateSearchBar::KateSearchBar(KateViewBar *viewBar)
    : KateViewBarWidget (viewBar),
      m_view(viewBar->view()),
      d(new Private)
{
    centralWidget()->setFont(KGlobalSettings::toolBarFont());

    d->searching = false;
    d->wrapAround = true;

    d->expressionEdit = new KateSearchBarEdit;
    connect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));
    connect(d->expressionEdit, SIGNAL(findNext()), this, SLOT(findNext()));
    connect(d->expressionEdit, SIGNAL(findPrevious()), this, SLOT(findPrevious()));

    d->caseSensitiveBox = new QCheckBox(i18n("&Case Sensitive"));
    connect(d->caseSensitiveBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->wholeWordsBox = new QCheckBox(i18n("&Whole Words"));
    connect(d->wholeWordsBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->regExpBox = new QCheckBox(i18n("&Regular Expression"));
    connect(d->regExpBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->fromCursorBox = new QCheckBox(i18n("&From Cursor"));
    connect(d->fromCursorBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->selectionOnlyBox = new QCheckBox(i18n("&Selection Only"));
    connect(d->selectionOnlyBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->highlightAllBox = new QCheckBox(i18n("&Highlight all"));
    connect(d->highlightAllBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    QVBoxLayout *topLayout = new QVBoxLayout ();
    centralWidget()->setLayout(topLayout);

    // NOTE: Here be cosmetics.
    topLayout->setMargin(2);


    // first line, search field + next/prev
    QToolButton *nextButton = new QToolButton();
    nextButton->setAutoRaise(true);
    nextButton->setIcon(SmallIcon("next"));
    nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    nextButton->setText(i18n("Next"));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(findNext()));

    QToolButton *prevButton = new QToolButton();
    prevButton->setAutoRaise(true);
    prevButton->setIcon(SmallIcon("previous"));
    prevButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    prevButton->setText(i18n("Previous"));
    connect(prevButton, SIGNAL(clicked()), this, SLOT(findPrevious()));

    QHBoxLayout *layoutFirstLine = new QHBoxLayout;
    topLayout->addLayout (layoutFirstLine);

    // first line: lineedit + next + prev
    layoutFirstLine->addWidget(d->expressionEdit);
    layoutFirstLine->addWidget(prevButton);
    layoutFirstLine->addWidget(nextButton);

    QGridLayout *gridLayout = new QGridLayout;
    topLayout->addLayout (gridLayout);

    // second line: casesensitive + whole words + regexp
    gridLayout->addWidget(d->caseSensitiveBox, 0, 0);
    gridLayout->addWidget(d->wholeWordsBox, 0, 1);
    gridLayout->addWidget(d->regExpBox, 0, 2);
    gridLayout->addItem (new QSpacerItem(0,0), 0, 3);

    // third line: from cursor + selection only + highlight all
    gridLayout->addWidget(d->fromCursorBox, 1, 0);
    gridLayout->addWidget(d->selectionOnlyBox, 1, 1);
    gridLayout->addWidget(d->highlightAllBox, 1, 2);
    gridLayout->addItem (new QSpacerItem(0,0), 1, 3);

    gridLayout->setColumnStretch (3, 10);

    // set right focus proxy
    centralWidget()->setFocusProxy(d->expressionEdit);
}

KateSearchBar::~KateSearchBar()
{
    delete d;
}

void KateSearchBar::doSearch(const QString &expression, bool backwards)
{
    // If we're starting search for the first time, begin at the current cursor position.
    if (!d->searching)
    {
        d->startCursor = m_view->cursorPosition();
        d->searching = true;
    }

    // FIXME: Make regexp searching optional (off by default), setting goes in the "advanced" section.
    d->regExp.setPattern(expression);
    d->regExp.setCaseSensitivity(d->caseSensitiveBox->checkState() == Qt::Checked ? Qt::CaseSensitive : Qt::CaseInsensitive);

    d->match = m_view->doc()->searchText(d->startCursor, d->regExp, backwards);

    bool foundMatch = d->match.isValid() && d->match != d->lastMatch;
    bool wrapped = false;

    if (d->wrapAround && !foundMatch)
    {
        // We found nothing, so wrap.
        d->startCursor = backwards ? m_view->doc()->documentEnd() : KTextEditor::Cursor(0, 0);
        d->match = m_view->doc()->searchText(d->startCursor, d->regExp, backwards);
        foundMatch = d->match.isValid() && d->match != d->lastMatch;
        wrapped = true;
    }

    if (foundMatch)
    {
        m_view->setCursorPositionInternal(d->match.start(), 1);
        m_view->setSelection(d->match);
        d->lastMatch = d->match;
    }

    d->expressionEdit->setStatus(foundMatch ? (wrapped ? KateSearchBarEdit::SearchWrapped : KateSearchBarEdit::Normal) : KateSearchBarEdit::NotFound);
}

void KateSearchBarEdit::setStatus(Status status)
{
    QPalette pal;
    QColor col;
    switch (status)
    {
        case KateSearchBarEdit::Normal:
            col = QPalette().color(QPalette::Base);
            break;
        case KateSearchBarEdit::NotFound:
            col = QColor("lightsalmon");
            break;
        case KateSearchBarEdit::SearchWrapped:
            col = QColor("palegreen");
            break;
    }
    pal.setColor(QPalette::Base, col);
    setPalette(pal);
}

void KateSearchBar::slotSearch()
{
    if (d->expressionEdit->text().isEmpty())
        d->expressionEdit->setStatus(KateSearchBarEdit::Normal);
    else
        doSearch(d->expressionEdit->text());
}

void KateSearchBar::findNext()
{
    if (!d->searching)
        return;
    d->startCursor = d->lastMatch.end();
    slotSearch();
}

void KateSearchBar::findPrevious()
{
    if (!d->searching)
        return;
    d->startCursor = d->lastMatch.start();
    doSearch(d->expressionEdit->text(), true);
}

KateSearchBarEdit::KateSearchBarEdit(QWidget *parent)
    : KLineEdit(parent)
{
}

// NOTE: void keyPressEvent(QKeyEvent* ev) does not grab Qt::Key_Tab.
// No idea, why... but that's probably why we use ::event here.
bool KateSearchBarEdit::event(QEvent *e)
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
        }
    }
    return KLineEdit::event(e);
}

void KateSearchBarEdit::showEvent(QShowEvent *)
{
    selectAll();
}

#include "katesearchbar.moc"
