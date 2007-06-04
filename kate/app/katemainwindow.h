/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_MAINWINDOW_H__
#define __KATE_MAINWINDOW_H__

#include "katemain.h"
#include "katemdi.h"

#include <KTextEditor/View>
#include <KTextEditor/Document>

#include <KParts/Part>

#include <KAction>

#include <QDragEnterEvent>
#include <QEvent>
#include <QDropEvent>
#include <QVBoxLayout>
#include <QActionGroup>
#include <QModelIndex>
#include <QHash>

class QMenu;

class QListView;

namespace Kate
{
  class MainWindow;
  class Plugin;
  class PluginView;
}

class KFileItem;
class KRecentFilesAction;

class KateExternalToolsMenuAction;
class KateViewDocumentProxyModel;
class KateViewManager;

class KateMainWindow : public KateMDI::MainWindow, virtual public KParts::PartBase
{
    Q_OBJECT

    friend class KateConfigDialog;
    friend class KateViewManager;

  public:
    /**
     * Construct the window and restore it's state from given config if any
     * @param sconfig session config for this window, 0 if none
     * @param sgroup session config group to use
     */
    KateMainWindow (KConfig *sconfig, const QString &sgroup);

    /**
     * Destruct the nice window
     */
    ~KateMainWindow();

    /**
     * Accessor methodes for interface and child objects
     */
  public:
    Kate::MainWindow *mainWindow ()
    {
      return m_mainWindow;
    }

    KateViewManager *viewManager ()
    {
      return m_viewManager;
    }

    QString dbusObjectPath() const
    {
      return m_dbusObjectPath;
    }
    /**
     * various methodes to get some little info out of this
     */
  public:
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KUrl activeDocumentUrl();

    uint mainWindowNumber () const
    {
      return myID;
    }

    /**
     * Prompts the user for what to do with files that are modified on disk if any.
     * This is optionally run when the window receives focus, and when the last
     * window is closed.
     * @return true if no documents are modified on disk, or all documents were
     * handled by the dialog; otherwise (the dialog was canceled) false.
     */
    bool showModOnDiskPrompt();

  public:
    /*reimp*/ void readProperties(const KConfigGroup& config);
    /*reimp*/ void saveProperties(KConfigGroup& config);
    /*reimp*/ void saveGlobalProperties( KConfig* sessionConfig );

  public:
    bool queryClose_internal(KTextEditor::Document *doc = NULL);

  private:
    /**
     * Setup actions which pointers are needed already in setupMainWindow
     */
    void setupImportantActions ();

    void setupMainWindow();
    void setupActions();
    bool queryClose();

    /**
     * read some global options from katerc
     */
    void readOptions();

    /**
     * save some global options to katerc
     */
    void saveOptions();

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

    /**
     * slots used for actions in the menus/toolbars
     * or internal signal connections
     */
  private Q_SLOTS:
    void newWindow ();

    void slotConfigure();

    void slotOpenWithMenuAction(QAction* a);

    void slotFileQuit();
    void slotEditToolbars();
    void slotNewToolbarConfig();
    void slotWindowActivated ();
    void slotUpdateOpenWith();

    void documentMenuAboutToShow();
    void activateDocumentFromDocMenu (QAction *action);

    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();

    //void fileSelected(const KFileItem *file);

    void tipOfTheDay();

    /* to update the caption */
    void slotDocumentCreated (KTextEditor::Document *doc);
    void updateCaption (KTextEditor::Document *doc);

    void pluginHelp ();
    void aboutEditor();
    void slotFullScreen(bool);

  private Q_SLOTS:
    void toggleShowStatusBar ();
    void toggleShowFullPath ();

  public:
    bool showStatusBar ();

  Q_SIGNALS:
    void statusBarToggled ();

  public:
    void openUrl (const QString &name = 0L);
    QModelIndex modelIndexForDocument(KTextEditor::Document *document);

    QHash<Kate::Plugin*, Kate::PluginView*> &pluginViews ()
    {
      return m_pluginViews;
    }

  private Q_SLOTS:
    void showFileListPopup(const QPoint& pos);
  protected:
    bool event( QEvent * );

  private Q_SLOTS:
    void slotDocumentCloseAll();
    void slotDocumentCloseOther();
    void slotDocModified(KTextEditor::Document *document);
  private:
    static uint uniqueID;
    uint myID;

    Kate::MainWindow *m_mainWindow;

    bool modNotification;

    // management items
    KateViewManager *m_viewManager;

    KRecentFilesAction *fileOpenRecent;

    QListView *m_fileList;

    KActionMenu* documentOpenWith;

    QMenu *documentMenu;
    QActionGroup *documentsGroup;

    KToggleAction* settingsShowFilelist;
    KToggleAction* settingsShowFileselector;

    KateExternalToolsMenuAction *externalTools;
    bool m_modignore, m_grrr;

    QString m_dbusObjectPath;
    KateViewDocumentProxyModel *m_documentModel;

    // all plugin views for this mainwindow, used by the pluginmanager
    QHash<Kate::Plugin*, Kate::PluginView*> m_pluginViews;

    // options: show statusbar + show path
    KToggleAction *m_paShowPath;
    KToggleAction *m_paShowStatusBar;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
