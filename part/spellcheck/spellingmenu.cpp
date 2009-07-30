/*
 * Copyright (C) 2008-2009 by Michel Ludwig (michel.ludwig@kdemail.net)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/


#include "spellingmenu.h"
#include "spellingmenu.moc"

#include <QMutexLocker>

#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"
#include "spellcheck/spellcheck.h"

#include <kdebug.h>

KateSpellingMenu::KateSpellingMenu(KateView *view)
  : QObject(view),
    m_view (view),
    m_spellingMenuAction(NULL),
    m_suggestionsSignalMapper(new QSignalMapper(this))
{
  connect(m_suggestionsSignalMapper, SIGNAL(mapped(const QString&)),
          this, SLOT(replaceWordBySuggestion(const QString&)));
}

KateSpellingMenu::~KateSpellingMenu()
{
}

void KateSpellingMenu::setEnabled(bool b)
{
  if(m_spellingMenuAction) {
    m_spellingMenuAction->setEnabled(b);
  }
}

void KateSpellingMenu::setVisible(bool b)
{
  if(m_spellingMenuAction) {
    m_spellingMenuAction->setVisible(b);
  }
}

void KateSpellingMenu::createActions(KActionCollection *ac)
{
  m_spellingMenuAction = new KActionMenu(i18n("Spelling"), this);
  ac->addAction("spelling_suggestions", m_spellingMenuAction);
  m_spellingMenu = m_spellingMenuAction->menu();
  setEnabled(false);
  setVisible(false);

  connect(m_spellingMenu, SIGNAL(aboutToShow()), this, SLOT(populateSuggestionsMenu()));
}

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
void KateSpellingMenu::enteredMisspelledRange(KTextEditor::SmartRange *range)
{
  m_spellingMenuAction->setEnabled(true);
  m_currentMisspelledRange = range;
  m_currentMisspelledRange->addWatcher(this);
}

/**
 * WARNING: SmartInterface lock must have been obtained before entering this function!
 **/
void KateSpellingMenu::exitedMisspelledRange(KTextEditor::SmartRange *range)
{
  Q_UNUSED(range);
  m_spellingMenuAction->setEnabled(false);
  if(m_currentMisspelledRange) {
    m_currentMisspelledRange->removeWatcher(this);
    m_currentMisspelledRange = NULL;
  }
}

void KateSpellingMenu::rangeDeleted(KTextEditor::SmartRange *range)
{
  exitedMisspelledRange(range);
}

void KateSpellingMenu::populateSuggestionsMenu()
{
  QMutexLocker(m_view->doc()->smartMutex());
  m_spellingMenu->clear();
  if(!m_currentMisspelledRange) {
    return;
  }
  const QString& misspelledWord = m_view->doc()->text(*m_currentMisspelledRange);
  const QString dictionary = m_view->doc()->dictionaryForMisspelledRange(*m_currentMisspelledRange);
  m_currentSuggestions = KateGlobal::self()->spellCheckManager()->suggestions(misspelledWord, dictionary);

  int counter = 0;
  for(QStringList::iterator i = m_currentSuggestions.begin(); i != m_currentSuggestions.end() && counter < 10; ++i) {
    const QString& suggestion = *i;
    KAction *action = new KAction(suggestion, m_spellingMenu);
    connect(action, SIGNAL(triggered()), m_suggestionsSignalMapper, SLOT(map()));
    m_suggestionsSignalMapper->setMapping(action, suggestion);
    m_spellingMenu->addAction(action);
    ++counter;
  }
}

void KateSpellingMenu::replaceWordBySuggestion(const QString& suggestion)
{
  QMutexLocker(m_view->doc()->smartMutex());
  m_view->doc()->replaceText(*m_currentMisspelledRange, suggestion);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
