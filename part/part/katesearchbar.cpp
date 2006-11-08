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

class KateSearchBar::Private
{
public:
    KTextEditor::Cursor startCursor;
    KTextEditor::Range match;
    KTextEditor::Range lastMatch;
    KateSearchBarEdit *expressionEdit;
    QCheckBox *caseSensitiveBox;
    QRegExp regExp;

    bool searching;
    bool wrapAround;
};

KateSearchBar::KateSearchBar(KateView *view)
    : QWidget(),
      m_view(view),
      d(new Private)
{
    setFont(KGlobalSettings::toolBarFont());

    d->searching = false;
    d->wrapAround = true;

    d->expressionEdit = new KateSearchBarEdit;
    connect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));
    connect(d->expressionEdit, SIGNAL(findNext()), this, SLOT(findNext()));
    connect(d->expressionEdit, SIGNAL(findPrevious()), this, SLOT(findPrevious()));
    connect(d->expressionEdit, SIGNAL(escapePressed()), this, SLOT(slotEscapePressed()));

    d->caseSensitiveBox = new QCheckBox(i18n("&Case Sensitive"));
    connect(d->caseSensitiveBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    QHBoxLayout *layout = new QHBoxLayout;

    // NOTE: Here be cosmetics.
    layout->setMargin(2);

    QToolButton *nextButton = new QToolButton();
    nextButton->setIcon(QIcon(SmallIcon("next")));
    nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    nextButton->setText(i18n("Find &Next"));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(findNext()));

    QToolButton *prevButton = new QToolButton();
    prevButton->setIcon(QIcon(SmallIcon("previous")));
    prevButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    prevButton->setText(i18n("Find &Previous"));
    connect(prevButton, SIGNAL(clicked()), this, SLOT(findPrevious()));

    layout->addWidget(d->expressionEdit);
    layout->addWidget(nextButton);
    layout->addWidget(prevButton);
    layout->addWidget(d->caseSensitiveBox);

    setLayout(layout);

    setFocusProxy(d->expressionEdit);
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

void KateSearchBar::slotEscapePressed()
{
    emit hidden();
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
            case Qt::Key_Escape:
                emit escapePressed();
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
