/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

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

#ifndef KATE_FILETREE_PLUGIN_H
#define KATE_FILETREE_PLUGIN_H

#include <KIcon>
#include <KXMLGUIClient>

#include <ktexteditor/document.h>
#include <kate/plugin.h>
#include <kate/mainwindow.h>

class KateFileTree;
class KateFileTreeModel;
class KateFileTreeProxyModel;

class KateFileTreePlugin: public Kate::Plugin
{
    Q_OBJECT

  public:
    explicit KateFileTreePlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateFileTreePlugin()
    {}

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    virtual uint configPages() const;
    
    virtual QString configPageName (uint number = 0) const;
    virtual QString configPageFullName (uint number = 0) const;
    virtual KIcon configPageIcon (uint number = 0) const;

  private:
    KateFileTree *m_fileTree;
};

class KateFileTreePluginView : public Kate::PluginView, public KXMLGUIClient
{
    Q_OBJECT

  public:
    /**
      * Constructor.
      */
    KateFileTreePluginView (Kate::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateFileTreePluginView ();

    virtual void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

  private:
    KateFileTree *m_fileTree;
    friend class KateFileTreePlugin;
    KateFileTreeProxyModel *m_proxyModel;
    KateFileTreeModel *m_documentModel;

  private Q_SLOTS:
    void showActiveDocument();
    void activateDocument(KTextEditor::Document *);
    void viewChanged();
    void documentOpened(KTextEditor::Document *);
    void documentClosed(KTextEditor::Document *);
};

#endif //KATE_FILETREE_PLUGIN_H

// kate: space-indent on; indent-width 2; replace-tabs on;
