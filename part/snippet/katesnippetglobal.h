/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#ifndef __SNIPPETPLUGIN_H__
#define __SNIPPETPLUGIN_H__

#include <QtCore/QVariant>
#include <QtCore/QPointer>

#include <kateglobal.h>
#include <kateview.h>

#include "completionmodel.h"

namespace KTextEditor {
  namespace CodesnippetsCore {
    class SnippetRepositoryModel;
  }
}


/**
 * This is the main class of KDevelop's snippet plugin.
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 */
class KateSnippetGlobal : public QObject
{
    Q_OBJECT

public:
    KateSnippetGlobal(QObject *parent, const QVariantList &args = QVariantList() );
    ~KateSnippetGlobal ();


    static KateSnippetGlobal* self() { return KateGlobal::self()->snippetGlobal(); }

    KateView* getCurrentView();
    
    /**
     * Create a new snippet widget, to allow to manage and insert snippets
     * @return new snippet widget
     */
    QWidget *snippetWidget (QWidget *parent, KateView *initialView);
    
    class KTextEditor::CodesnippetsCore::SnippetRepositoryModel *repositoryModel ();
    
    KTextEditor::CodesnippetsCore::CategorizedSnippetModel* modelForDocument(KTextEditor::Document *document);
    
    
    enum Mode {FileModeBasedMode=0, SnippetFileBasedMode=1};
    
    enum Mode snippetsMode();
    void setSnippetsMode(enum Mode);
    
Q_SIGNALS:
      void typeHasChanged(KTextEditor::Document*);
      void snippetsModeChanged();
      
public Q_SLOTS:
    /**
     * Create snippet for given view, e.g. by using the selection
     * @param view view to create snippet for
     */
    void createSnippet (KateView *view);

    /**
     * Show the snippet dialog, used by most simple apps using just
     * KatePart.
     * @param view view to show dialog for
     */
    void showDialog (KateView *view);

    void insertSnippetFromActionData();

  
    void addDocument(KTextEditor::Document* document);
    void removeDocument(KTextEditor::Document* document);
    void addView(KTextEditor::Document *document, KTextEditor::View *view);
    void updateDocument(KTextEditor::Document *document);
    void slotTypeChanged(const QStringList& fileType);    
    
    
private:
    class KTextEditor::CodesnippetsCore::SnippetRepositoryModel* m_repositoryModel;
    QPointer<KateView> m_activeViewForDialog;
    
    QMultiHash<KTextEditor::Document*,QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> > m_document_model_multihash;
    QHash<QString,QWeakPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> > m_mode_model_hash;
    QHash<KTextEditor::Document*,KTextEditor::CodesnippetsCore::CategorizedSnippetModel*> m_document_categorized_hash;

    enum Mode m_snippetsMode;
    
};

#endif

