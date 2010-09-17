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
#include <kate/pluginconfigpageinterface.h>

class KateFileTree;
class KateFileTreeModel;
class KateFileTreeProxyModel;
class KateFileTreeConfigPage;
class KateFileTreePluginView;

#include <QHash>

class KateFileTreePlugin: public Kate::Plugin, public Kate::PluginConfigPageInterface
{
  Q_OBJECT
  Q_INTERFACES(Kate::PluginConfigPageInterface)

  public:
    explicit KateFileTreePlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateFileTreePlugin()
    {}

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    virtual uint configPages() const;
    
    virtual QString configPageName (uint number = 0) const;
    virtual QString configPageFullName (uint number = 0) const;
    virtual KIcon configPageIcon (uint number = 0) const;
    virtual Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0 );
    
  private:
    QHash<Kate::MainWindow *, KateFileTreePluginView *> m_view;
    KateFileTreeConfigPage *m_confPage;
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

    KateFileTreeModel *model();
    KateFileTreeProxyModel *proxy();

    void setListMode(bool listMode);
    
  private:
    KateFileTree *m_fileTree;
    KateFileTreeProxyModel *m_proxyModel;
    KateFileTreeModel *m_documentModel;
    
  private Q_SLOTS:
    void showActiveDocument();
    void activateDocument(KTextEditor::Document *);
    void viewChanged();
    void documentOpened(KTextEditor::Document *);
    void documentClosed(KTextEditor::Document *);
    void viewModeChanged(bool);
    void sortRoleChanged(int);
};

#endif //KATE_FILETREE_PLUGIN_H

// kate: space-indent on; indent-width 2; replace-tabs on;
