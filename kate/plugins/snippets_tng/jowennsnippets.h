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

#ifndef __JOWENN_SNIPPETS_H__
#define __JOWENN_SNIPPETS_H__

#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <kate/pluginconfigpageinterface.h>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/document.h>
#include <kxmlguiclient.h>
#include <qpointer.h>
#include <qsharedpointer.h>

#include "ui_snippet_repository.h"

namespace KParts
{
  class ReadOnlyPart;
}

namespace KateMDI
{
}

namespace JoWenn {
  
  class KateSnippetsPluginView;
  class KateSnippetRepositoryModel;
  class KateSnippetSelector;
  
  class KateSnippetCompletionModel;
  
  class KateSnippetsPlugin: public Kate::Plugin, public Kate::PluginConfigPageInterface
  {
      Q_OBJECT
      Q_INTERFACES(Kate::PluginConfigPageInterface)
    public:
      explicit KateSnippetsPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
      virtual ~KateSnippetsPlugin();

      Kate::PluginView *createView (Kate::MainWindow *mainWindow);

      // PluginConfigPageInterface
      uint configPages() const { return 1; };
      Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0);
      QString configPageName (uint number = 0) const;
      QString configPageFullName (uint number = 0) const;
      KIcon configPageIcon (uint number = 0) const;

      void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
      void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);
      KateSnippetRepositoryModel *repositoryData();
      
    public Q_SLOTS:
      void addDocument(KTextEditor::Document* document);
      void removeDocument(KTextEditor::Document* document);
      void addView(KTextEditor::Document *document, KTextEditor::View *view);
      void updateDocument(KTextEditor::Document *document);
      void slotTypeChanged(const QString& fileType);
    public:
      KateSnippetCompletionModel* modelForDocument(KTextEditor::Document *document);
    private:
      QList<KateSnippetsPluginView*> mViews;
      QHash<KTextEditor::Document*,QSharedPointer<KTextEditor::CodeCompletionModel2> > m_document_model_hash;
      QHash<QString,QWeakPointer<KTextEditor::CodeCompletionModel2> > m_mode_model_hash;
      KateSnippetRepositoryModel *m_repositoryData;
  };

  class KateSnippetsPluginView : public Kate::PluginView
  {
      Q_OBJECT

    public:
      /**
        * Constructor.
        */
      KateSnippetsPluginView (Kate::MainWindow *mainWindow,JoWenn::KateSnippetsPlugin *plugin);

      /**
      * Virtual destructor.
      */
      ~KateSnippetsPluginView ();

      void readConfig();

    private:
      KateSnippetSelector* m_snippetSelector;
  };


  class KateSnippetsConfigPage : public Kate::PluginConfigPage, private Ui::KateSnippetRepository {
      Q_OBJECT
    public:
      explicit KateSnippetsConfigPage( QWidget* parent = 0, KateSnippetsPlugin *plugin = 0 );
      virtual ~KateSnippetsConfigPage()
      {}

      virtual void apply();
      virtual void reset();
      virtual void defaults(){}
    public Q_SLOTS:
      void slotCopy();
    private:
//      class QCheckBox *cbAutoSyncronize;
      KateSnippetsPlugin *m_plugin;
  };

}
#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

