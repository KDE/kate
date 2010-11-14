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
#include "ui_filetypelistcreatorview.h"
#include <kmainwindow.h>
#include <qwidget.h>
#include <qstringlist.h>
#include <QStandardItemModel>

class QAbstractButton;
class QStandardItemModel;
class KLineEdit;
class KPushButton;

namespace KTextEditor {
  namespace CodesnippetsCore {
    namespace Editor {
      class SnippetCompletionModel;
      class SnippetSelectorModel;
    }
  }
}

class FiletypeListDropDown: public QWidget, private Ui::FiletypeListCreatorview
{
  Q_OBJECT
  public:
    FiletypeListDropDown(QWidget *parent,KLineEdit *lineEdit,KPushButton *btn,const QStringList &modes);
    virtual ~FiletypeListDropDown();
    virtual bool eventFilter(QObject *obj,QEvent *event);
  Q_SIGNALS:
    void modified();
  public Q_SLOTS:
    void parseAndShow();
  private Q_SLOTS:
    void addFileType();
  private:
    KLineEdit *m_lineEdit;
    KPushButton *m_btn;
    QStringList m_modes;
    QStandardItemModel m_model;
};

class SnippetEditorWindow: public KMainWindow, private Ui::SnippetEditorView
{
  Q_OBJECT
  public:
    SnippetEditorWindow(const QStringList &modes, const KUrl& url);
    virtual ~SnippetEditorWindow();
    bool ok() {return m_ok;}
  private Q_SLOTS:
    void slotClose(QAbstractButton*);
    void currentChanged(const QModelIndex& current, const QModelIndex&);
    void modified();
    void deleteSnippet();
    void newSnippet();
  private:      
    bool m_modified;
    KUrl m_url;    
    KTextEditor::CodesnippetsCore::Editor::SnippetCompletionModel *m_snippetData;
    KTextEditor::CodesnippetsCore::Editor::SnippetSelectorModel *m_selectorModel;
    bool m_ok;
    void notifyChange();
    QString m_snippetLicense;
    FiletypeListDropDown *m_filetypeDropDown;    
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
