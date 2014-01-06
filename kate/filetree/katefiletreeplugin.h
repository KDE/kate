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

#include <ktexteditor/commandinterface.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <kate/pluginconfigpageinterface.h>

#include "katefiletreepluginsettings.h"

class KateFileTree;
class KateFileTreeModel;
class KateFileTreeProxyModel;
class KateFileTreeConfigPage;
class KateFileTreePluginView;
class KateFileTreeCommand;

class KateFileTreePlugin: public Kate::Plugin, public Kate::PluginConfigPageInterface
{
  Q_OBJECT
  Q_INTERFACES(Kate::PluginConfigPageInterface)

  public:
    explicit KateFileTreePlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateFileTreePlugin();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    virtual uint configPages() const;

    virtual QString configPageName (uint number = 0) const;
    virtual QString configPageFullName (uint number = 0) const;
    virtual KIcon configPageIcon (uint number = 0) const;
    virtual Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0 );

    const KateFileTreePluginSettings &settings();

    void applyConfig(bool shadingEnabled, QColor viewShade, QColor editShade, bool listMode, int sortRole, bool showFulPath);

  public Q_SLOTS:
    void viewDestroyed(QObject* view);

  private:
    QList<KateFileTreePluginView *> m_views;
    KateFileTreeConfigPage *m_confPage;
    KateFileTreePluginSettings m_settings;
    KateFileTreeCommand* m_fileCommand;
};

class KateFileTreePluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

  public:
    /**
      * Constructor.
      */
    KateFileTreePluginView (Kate::MainWindow *mainWindow, KateFileTreePlugin *plug);

    /**
     * Virtual destructor.
     */
    ~KateFileTreePluginView ();

    virtual void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

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

  private Q_SLOTS:
    void showToolView();
    void hideToolView();
    void switchDocument(const QString &doc);
    void showActiveDocument();
    void activateDocument(KTextEditor::Document *);
    void viewChanged();
    void documentOpened(KTextEditor::Document *);
    void documentClosed(KTextEditor::Document *);
    void viewModeChanged(bool);
    void sortRoleChanged(int);
    void slotAboutToLoadDocuments();
    void slotDocumentsLoaded(const QList<KTextEditor::Document *> &);
};

class KateFileTreeCommand : public QObject, public KTextEditor::Command
{
  Q_OBJECT

  public:
    KateFileTreeCommand(QObject *parent);

  Q_SIGNALS:
    void showToolView();
    void slotDocumentPrev();
    void slotDocumentNext();
    void slotDocumentFirst();
    void slotDocumentLast();
    void switchDocument(const QString&);

  public:
    const QStringList& cmds();
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg);
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg);
};

#endif //KATE_FILETREE_PLUGIN_H

// kate: space-indent on; indent-width 2; replace-tabs on;
