/* This file is part of the KDE project
 * Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>
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

#ifndef _SNIPPET_EDITOR_WINDOW_
#define _SNIPPET_EDITOR_WINDOW_

#include "ui_snippeteditorview.h"
#include <kmainwindow.h>
#include <qwidget.h>
#include <qstringlist.h>

class QAbstractButton;
namespace JoWenn {
  class KateSnippetCompletionModel;
  class KateSnippetSelectorModel;
}

class SnippetEditorWindow: public KMainWindow, private Ui::SnippetEditorView
{
  Q_OBJECT
  public:
    SnippetEditorWindow(const QStringList &modes, const KUrl& url);
    virtual ~SnippetEditorWindow();
  private Q_SLOTS:
    void slotClose(QAbstractButton*);
    void currentChanged(const QModelIndex& current, const QModelIndex&);
    void modified();
    void deleteSnippet();
    void newSnippet();
  private:
    bool m_modified;
    KUrl m_url;
    JoWenn::KateSnippetCompletionModel *m_snippetData;
    JoWenn::KateSnippetSelectorModel *m_selectorModel;
};

#endif