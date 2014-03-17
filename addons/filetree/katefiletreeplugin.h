/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <QIcon>

#include <ktexteditor/commandinterface.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <KTextEditor/Plugin>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/sessionconfiginterface.h>

#include "katefiletreepluginsettings.h"

#include <KXMLGUIClient>

class KateFileTree;
class KateFileTreeModel;
class KateFileTreeProxyModel;
class KateFileTreeConfigPage;
class KateFileTreePluginView;
class KateFileTreeCommand;

class KateFileTreePlugin: public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    explicit KateFileTreePlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateFileTreePlugin();

    QObject *createView (KTextEditor::MainWindow *mainWindow);

    virtual int configPages() const;
    virtual KTextEditor::ConfigPage *configPage (int number = 0, QWidget *parent = 0 );

    const KateFileTreePluginSettings &settings();

    void applyConfig(bool shadingEnabled, QColor viewShade, QColor editShade, bool listMode, int sortRole, bool showFulPath);

  public Q_SLOTS:
    void viewDestroyed(QObject* view);

  private:
    QList<KateFileTreePluginView *> m_views;
    KateFileTreeConfigPage *m_confPage;
    KateFileTreePluginSettings m_settings;
};

class KateFileTreePluginView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
  Q_OBJECT
    
  Q_INTERFACES(KTextEditor::SessionConfigInterface)

  public:
    /**
      * Constructor.
      */
    KateFileTreePluginView (KTextEditor::MainWindow *mainWindow, KateFileTreePlugin *plug);

    /**
     * Virtual destructor.
     */
    ~KateFileTreePluginView ();

    void readSessionConfig (const KConfigGroup& config);
    void writeSessionConfig (KConfigGroup& config);

    /**
     * The file tree model.
     * @return the file tree model
     */
    KateFileTreeModel *model();
    /**
     * The file tree proxy model.
     * @return the file tree proxy model
     */
    KateFileTreeProxyModel *proxy();
    /**
     * The file tree.
     * @return the file tree
     */
    KateFileTree *tree();

    void setListMode(bool listMode);

    bool hasLocalPrefs();
    void setHasLocalPrefs(bool);

  private:
    QWidget *m_toolView;
    KateFileTree *m_fileTree;
    KateFileTreeProxyModel *m_proxyModel;
    KateFileTreeModel *m_documentModel;
    bool m_hasLocalPrefs;
    bool m_loadingDocuments;
    KateFileTreePlugin *m_plug;
    KTextEditor::MainWindow *m_mainWindow;

  private Q_SLOTS:
    void showToolView();
    void hideToolView();
    void showActiveDocument();
    void activateDocument(KTextEditor::Document *);
    void viewChanged(KTextEditor::View * = nullptr);
    void documentOpened(KTextEditor::Document *);
    void documentClosed(KTextEditor::Document *);
    void viewModeChanged(bool);
    void sortRoleChanged(int);
    void slotAboutToCreateDocuments();
    void slotDocumentsCreated(const QList<KTextEditor::Document *> &);
};

#endif //KATE_FILETREE_PLUGIN_H

// kate: space-indent on; indent-width 2; replace-tabs on;
