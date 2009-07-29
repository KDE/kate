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


#include "spellingsuggestionsmenu.h"
#include "spellingsuggestionsmenu.moc"

#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"
#include "spellcheck/spellcheck.h"

#include <kdebug.h>

KateSpellingSuggestionsMenu::KateSpellingSuggestionsMenu(KateView *view)
  : QObject(view),
    m_view (view),
    m_suggestionsMenuAction(NULL),
    m_suggestionsSignalMapper(new QSignalMapper(this))
{
  connect(m_suggestionsSignalMapper, SIGNAL(mapped(const QString&)),
          this, SLOT(replaceWordBySuggestion(const QString&)));
}

KateSpellingSuggestionsMenu::~KateSpellingSuggestionsMenu()
{
}

void KateSpellingSuggestionsMenu::createActions(KActionCollection *ac)
{
    m_suggestionsMenuAction = new KActionMenu(i18n("Spelling Suggestions"), this);
    ac->addAction("spelling_suggestions", m_suggestionsMenuAction);
    m_suggestionsMenu = m_suggestionsMenuAction->menu();

    connect(m_suggestionsMenu, SIGNAL(aboutToShow()), this, SLOT(populateSuggestionsMenu()));
}

/*
 * WARNING: we assume that 'updateContextMenuActionStatus' is called before this method!
 */
void KateSpellingSuggestionsMenu::populateSuggestionsMenu()
{
  m_suggestionsMenu->clear();
  int counter = 0;
  for(QStringList::iterator i = m_currentSuggestions.begin(); i != m_currentSuggestions.end() && counter < 10; ++i) {
    const QString& suggestion = *i;
    KAction *action = new KAction(suggestion, m_suggestionsMenu);
    connect(action, SIGNAL(triggered()), m_suggestionsSignalMapper, SLOT(map()));
    m_suggestionsSignalMapper->setMapping(action, suggestion);
    m_suggestionsMenu->addAction(action);
    ++counter;
  }
}

void KateSpellingSuggestionsMenu::updateContextMenuActionStatus(KTextEditor::View *view, QMenu *menu)
{
  Q_ASSERT(view == m_view);
  Q_UNUSED(menu);
  if(!m_suggestionsMenuAction) {
    return;
  }
  QPair<KTextEditor::Range, QString> item = m_view->doc()->onTheFlyMisspelledItem(m_view->cursorPosition());
  m_currentMisspelledRange = item.first;
  const QString dictionary = item.second;
  if(!m_currentMisspelledRange.isValid()) {
    m_suggestionsMenuAction->setVisible(false);
    return;
  }
  m_suggestionsMenuAction->setVisible(true);
  const QString& misspelledWord = m_view->doc()->text(m_currentMisspelledRange);
  m_currentSuggestions = KateGlobal::self()->spellCheckManager()->suggestions(misspelledWord, dictionary);
  if(m_currentSuggestions.length() == 0) {
      m_suggestionsMenuAction->setEnabled(false);
      return;
  }
  m_suggestionsMenuAction->setEnabled(true);
}

void KateSpellingSuggestionsMenu::replaceWordBySuggestion(const QString& suggestion)
{
  m_view->doc()->replaceText(m_currentMisspelledRange, suggestion);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
