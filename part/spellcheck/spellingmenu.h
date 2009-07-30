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

#ifndef SPELLINGMENU_H
#define SPELLINGMENU_H

#include <QObject>
#include <QSignalMapper>

#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <ktexteditor/range.h>
#include <ktexteditor/smartrangewatcher.h>
#include <ktexteditor/view.h>

class KateDocument;
class KateView;

class KateSpellingMenu : public QObject, private KTextEditor::SmartRangeWatcher {
  Q_OBJECT

  public:
    KateSpellingMenu(KateView *view);
    virtual ~KateSpellingMenu();

    void createActions(KActionCollection *ac);

    void enteredMisspelledRange(KTextEditor::SmartRange *range);
    void exitedMisspelledRange(KTextEditor::SmartRange *range);

  public Q_SLOTS:
    void setEnabled(bool b);
    void setVisible(bool b);

  protected:
    KateView *m_view;
    KActionMenu *m_spellingMenuAction;
    KMenu *m_spellingMenu;
    KTextEditor::SmartRange *m_currentMisspelledRange;
    QStringList m_currentSuggestions;
    QSignalMapper *m_suggestionsSignalMapper;

    void rangeDeleted(KTextEditor::SmartRange *range);

  protected Q_SLOTS:
    void populateSuggestionsMenu();
    void replaceWordBySuggestion(const QString& suggestion);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
