/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2010 by Michel Ludwig <michel.ludwig@kdemail.net>
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
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
    m_ignoreWordAction(NULL),
    m_addToDictionaryAction(NULL),
    m_spellingMenu(NULL),
    m_currentMisspelledRange(NULL), // we have to use 'm_currentMisspelledRange'
                                    // as QSignalMapper doesn't work with pairs of objects;
                                    // it just points to the object pointed to by either
                                    // 'm_currentMouseMisspelledRange' or 'm_currentCaretMisspelledRange'
    m_currentMouseMisspelledRange(NULL),
    m_currentCaretMisspelledRange(NULL),
    m_useMouseForMisspelledRange(false),
    m_suggestionsSignalMapper(new QSignalMapper(this))
{
  connect(m_suggestionsSignalMapper, SIGNAL(mapped(const QString&)),
          this, SLOT(replaceWordBySuggestion(const QString&)));
}

KateSpellingMenu::~KateSpellingMenu()
{
  m_currentMisspelledRange = NULL; // it shouldn't be accessed anymore as it could
                                   // point to a non-existing object (bug 226724)
                                   // (for example, when it pointed to m_currentCaretMisspelledRange
                                   // and that range got deleted after the caret had left)
  m_currentCaretMisspelledRange = NULL;
  m_currentMouseMisspelledRange = NULL;
}

bool KateSpellingMenu::isEnabled() const
{
  if(!m_spellingMenuAction) {
    return false;
  }
  return m_spellingMenuAction->isEnabled();
}

void KateSpellingMenu::setEnabled(bool b)
{
  if(m_spellingMenuAction) {
    m_spellingMenuAction->setEnabled(b);
  }
  if(m_ignoreWordAction) {
    m_ignoreWordAction->setEnabled(b);
  }
  if(m_addToDictionaryAction) {
    m_addToDictionaryAction->setEnabled(b);
  }
}

void KateSpellingMenu::setVisible(bool b)
{
  if(m_spellingMenuAction) {
    m_spellingMenuAction->setVisible(b);
  }
  if(m_ignoreWordAction) {
    m_ignoreWordAction->setVisible(b);
  }
  if(m_addToDictionaryAction) {
    m_addToDictionaryAction->setVisible(b);
  }
}

void KateSpellingMenu::createActions(KActionCollection *ac)
{
  m_spellingMenuAction = new KActionMenu(i18n("Spelling"), this);
  ac->addAction("spelling_suggestions", m_spellingMenuAction);
  m_spellingMenu = m_spellingMenuAction->menu();
  connect(m_spellingMenu, SIGNAL(aboutToShow()), this, SLOT(populateSuggestionsMenu()));

  m_ignoreWordAction = new KAction(i18n("Ignore Word"), this);
  connect(m_ignoreWordAction, SIGNAL(triggered()), this, SLOT(ignoreCurrentWord()));

  m_addToDictionaryAction = new KAction(i18n("Add to Dictionary"), this);
  connect(m_addToDictionaryAction, SIGNAL(triggered()), this, SLOT(addCurrentWordToDictionary()));

  setEnabled(false);
  setVisible(false);
}

void KateSpellingMenu::caretEnteredMisspelledRange(KTextEditor::MovingRange *range)
{
  if(m_currentCaretMisspelledRange == range) {
      return;
  }
  m_currentCaretMisspelledRange = NULL;
  setEnabled(true);
  m_currentCaretMisspelledRange = range;
}

void KateSpellingMenu::caretExitedMisspelledRange(KTextEditor::MovingRange *range)
{
  if(range != m_currentCaretMisspelledRange) { // order of 'exit' and 'entered' signals
    return;                                    // was wrong
  }
  setEnabled(false);
  m_currentCaretMisspelledRange = NULL;
}

void KateSpellingMenu::mouseEnteredMisspelledRange(KTextEditor::MovingRange *range)
{
  if(m_currentMouseMisspelledRange == range) {
      return;
  }
  m_currentMouseMisspelledRange = range;
}

void KateSpellingMenu::mouseExitedMisspelledRange(KTextEditor::MovingRange *range)
{
  if(range != m_currentMouseMisspelledRange) { // order of 'exit' and 'entered' signals
    return;                                    // was wrong
  }
  m_currentMouseMisspelledRange = NULL;
}

void KateSpellingMenu::rangeDeleted(KTextEditor::MovingRange *range)
{
  if(m_currentCaretMisspelledRange == range) {
    m_currentCaretMisspelledRange = NULL;
  }
  if(m_currentMouseMisspelledRange == range) {
    m_currentMouseMisspelledRange = NULL;
  }
  if(m_currentMisspelledRange == range) {
    m_currentMisspelledRange = NULL;
  }
  setEnabled(m_currentCaretMisspelledRange != NULL);
}

void KateSpellingMenu::setUseMouseForMisspelledRange(bool b)
{
  m_useMouseForMisspelledRange = b;
  if(m_useMouseForMisspelledRange) {
    setEnabled(m_currentMouseMisspelledRange != NULL);
  }
  else if(!m_useMouseForMisspelledRange) {
    setEnabled(m_currentCaretMisspelledRange != NULL);
  }
}

void KateSpellingMenu::populateSuggestionsMenu()
{
  m_spellingMenu->clear();
  if((m_useMouseForMisspelledRange && !m_currentMouseMisspelledRange)
      || (!m_useMouseForMisspelledRange && !m_currentCaretMisspelledRange)) {
    return;
  }
  m_currentMisspelledRange = (m_useMouseForMisspelledRange ? m_currentMouseMisspelledRange
                                                           : m_currentCaretMisspelledRange);
  m_spellingMenu->addAction(m_ignoreWordAction);
  m_spellingMenu->addAction(m_addToDictionaryAction);
  m_spellingMenu->addSeparator();
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
  KateDocument *doc = m_view->doc();
  KateGlobal::self()->spellCheckManager()->replaceCharactersEncodedIfNecessary(suggestion, doc, *m_currentMisspelledRange);
}

void KateSpellingMenu::addCurrentWordToDictionary()
{
  if(!m_currentMisspelledRange) {
    return;
  }
  const QString& misspelledWord = m_view->doc()->text(*m_currentMisspelledRange);
  const QString dictionary = m_view->doc()->dictionaryForMisspelledRange(*m_currentMisspelledRange);
  KateGlobal::self()->spellCheckManager()->addToDictionary(misspelledWord, dictionary);
  m_view->doc()->clearMisspellingForWord(misspelledWord); // WARNING: 'm_currentMisspelledRange' is deleted here!
}

void KateSpellingMenu::ignoreCurrentWord()
{
  if(!m_currentMisspelledRange) {
    return;
  }
  const QString& misspelledWord = m_view->doc()->text(*m_currentMisspelledRange);
  const QString dictionary = m_view->doc()->dictionaryForMisspelledRange(*m_currentMisspelledRange);
  KateGlobal::self()->spellCheckManager()->ignoreWord(misspelledWord, dictionary);
  m_view->doc()->clearMisspellingForWord(misspelledWord); // WARNING: 'm_currentMisspelledRange' is deleted here!
}

// kate: space-indent on; indent-width 2; replace-tabs on;
