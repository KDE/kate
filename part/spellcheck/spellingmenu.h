/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 by Michel Ludwig <michel.ludwig@kdemail.net>
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

#ifndef SPELLINGMENU_H
#define SPELLINGMENU_H

#include <QObject>
#include <QSignalMapper>

#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>
#include <ktexteditor/range.h>
#include <ktexteditor/view.h>

class KateDocument;
class KateView;

class KateOnTheFlyChecker;

class KateSpellingMenu : public QObject {
  Q_OBJECT
  friend class KateOnTheFlyChecker;

  public:
    KateSpellingMenu(KateView *view);
    virtual ~KateSpellingMenu();

    bool isEnabled() const;

    void createActions(KActionCollection *ac);

    /**
     * This method has to be called before the menu is shown in response to a context
     * menu event. It will trigger that the misspelled range located under the mouse pointer
     * is considered for the spelling suggestions.
     **/
    void setUseMouseForMisspelledRange(bool b);

  public Q_SLOTS:
    void setEnabled(bool b);
    void setVisible(bool b);

  protected:
    KateView *m_view;
    KActionMenu *m_spellingMenuAction;
    KAction *m_ignoreWordAction, *m_addToDictionaryAction;
    KMenu *m_spellingMenu;
    KTextEditor::MovingRange *m_currentMisspelledRange;
    KTextEditor::MovingRange *m_currentMouseMisspelledRange;
    KTextEditor::MovingRange *m_currentCaretMisspelledRange;
    bool m_useMouseForMisspelledRange;
    QStringList m_currentSuggestions;
    QSignalMapper *m_suggestionsSignalMapper;

    // These methods are called from KateOnTheFlyChecker to inform about events involving
    // moving ranges.
    void rangeDeleted(KTextEditor::MovingRange *range);
    void caretEnteredMisspelledRange(KTextEditor::MovingRange *range);
    void caretExitedMisspelledRange(KTextEditor::MovingRange *range);
    void mouseEnteredMisspelledRange(KTextEditor::MovingRange *range);
    void mouseExitedMisspelledRange(KTextEditor::MovingRange *range);

  protected Q_SLOTS:
    void populateSuggestionsMenu();
    void replaceWordBySuggestion(const QString& suggestion);

    void addCurrentWordToDictionary();
    void ignoreCurrentWord();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
