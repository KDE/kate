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

#include "katemdi.h"
#include "kateviewmanager.h"

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/mainwindow.h>

#include <KParts/Part>

#include <QDragEnterEvent>
#include <QEvent>
#include <QDropEvent>
#include <QVBoxLayout>
#include <QModelIndex>
#include <QHash>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QUrl>

class QMenu;

namespace KIO
{
  class UDSEntry;
  typedef class QList<UDSEntry> UDSEntryList;
}

namespace Kate
{
  class MainWindow;
  class Plugin;
  class PluginView;
  class PluginConfigPageInterface;
}

class KFileItem;
class KRecentFilesAction;

class KateViewManager;
class KateMwModOnHdDialog;
class KateQuickOpen;

// Helper layout class to always provide minimum size
class KateContainerStackedLayout : public QStackedLayout
{
  Q_OBJECT
public:
  KateContainerStackedLayout(QWidget* parent);
  virtual QSize sizeHint() const;
  virtual QSize minimumSize() const;
};


class KateMainWindow : public KateMDI::MainWindow, virtual public KParts::PartBase
{
    Q_OBJECT

    friend class KateConfigDialog;
    friend class KateViewManager;

  public:
    /**
     * Construct the window and restore its state from given config if any
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
    
    /**
     * KTextEditor::MainWindow wrapper
     * @return KTextEditor::MainWindow wrapper.
     */
    KTextEditor::MainWindow *wrapper ()
    {
      return m_wrapper;
    }

    /**
     * get a plugin view with identifier \p name.
     * \param name the plugin's name
     * \return pointer to the plugin view if a plugin with \p name is loaded and has a view for this mainwindow,
     *         otherwise null
     */
    Kate::PluginView *pluginView (const QString &name);

  public:
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    QUrl activeDocumentUrl();

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

    /**
     * save the settings, size and state of this window in
     * the provided config group
     */
    void saveWindowConfig(const KConfigGroup &);
    /**
     * restore the settings, size and state of this window from
     * the provided config group.
     */
    void restoreWindowConfig(const KConfigGroup &);

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

  public Q_SLOTS:
    void slotFileClose();
    void slotFileQuit();
    void queueModifiedOnDisc(KTextEditor::Document *doc);

    /**
     * slots used for actions in the menus/toolbars
     * or internal signal connections
     */
  private Q_SLOTS:
    void newWindow ();

    void slotConfigure();

    void slotOpenWithMenuAction(QAction* a);

    void slotEditToolbars();
    void slotNewToolbarConfig();
    void slotUpdateOpenWith();
    void slotOpenDocument(QUrl);

    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();

    /**
     * Show quick open
     */
    void slotQuickOpen ();
    
    void tipOfTheDay();

    /* to update the caption */
    void slotDocumentCreated (KTextEditor::Document *doc);
    void updateCaption (KTextEditor::Document *doc);
    // calls updateCaption(doc) with the current document
    void updateCaption ();

    void pluginHelp ();
    void aboutEditor();
    void slotFullScreen(bool);

    void slotListRecursiveEntries(KIO::Job *job, const KIO::UDSEntryList &list);

  private Q_SLOTS:
    void toggleShowStatusBar ();

  public:
    bool showStatusBar ();

  Q_SIGNALS:
    void statusBarToggled ();
    void unhandledShortcutOverride (QEvent *e);

  public:
    void openUrl (const QString &name = QString());

    QHash<Kate::Plugin*, Kate::PluginView*> &pluginViews ()
    {
      return m_pluginViews;
    }

    inline QWidget *bottomViewBarContainer() {return m_bottomViewBarContainer;}
    inline void addToBottomViewBarContainer(KTextEditor::View *view,QWidget *bar){m_bottomContainerStack->addWidget (bar); m_bottomViewBarMapping[view]=BarState(bar);}
    inline void hideBottomViewBarForView(KTextEditor::View *view) {QWidget *bar; BarState state=m_bottomViewBarMapping.value(view); bar=state.bar(); if (bar) {m_bottomContainerStack->setCurrentWidget(bar); bar->hide(); state.setState(false); m_bottomViewBarMapping[view]=state;} m_bottomViewBarContainer->hide();}
    inline void showBottomViewBarForView(KTextEditor::View *view) {QWidget *bar; BarState state=m_bottomViewBarMapping.value(view); bar=state.bar();  if (bar) {m_bottomContainerStack->setCurrentWidget(bar); bar->show(); state.setState(true); m_bottomViewBarMapping[view]=state;  m_bottomViewBarContainer->show();}}
    inline void deleteBottomViewBarForView(KTextEditor::View *view) {QWidget *bar; BarState state=m_bottomViewBarMapping.take(view); bar=state.bar();  if (bar) {if (m_bottomContainerStack->currentWidget()==bar) m_bottomViewBarContainer->hide(); delete bar;}}

  //
  // KTextEditor::MainWindow interface, get called by invokeMethod from our wrapper object!
  //
  public Q_SLOTS:
    /**
     * get the toplevel widget.
     * \return the real main window widget.
     */
    QWidget *window ()
    {
      return this;
    } 
    
    /**
     * Get a list of all views for this main window.
     * @return all views
     */
    QList<KTextEditor::View *> views ()
    {
      return viewManager()->viewList();
    }
    
    /**
     * Access the active view.
     * \return active view
     */
    KTextEditor::View *activeView ()
    {
      return viewManager()->activeView();
    }

    /**
     * Activate the view with the corresponding \p document.
     * If none exist for this document, create one
     * \param document the document
     * \return activated view of this document
     */
    KTextEditor::View *activateView (KTextEditor::Document *document)
    {
      return viewManager()->activateView (document);
    }

    /**
     * Open the document \p url with the given \p encoding.
     * \param url the document's url
     * \param encoding the preferred encoding. If encoding is QString() the
     *        encoding will be guessed or the default encoding will be used.
     * \return a pointer to the created view for the new document, if a document
     *         with this url is already existing, its view will be activated
     */
    KTextEditor::View *openUrl (const QUrl &url, const QString &encoding = QString())
    {
      return viewManager()->openUrlWithView (url, encoding);
    }
      
    /**
     * Try to create a view bar for the given view.
     * @param view view for which we want an view bar
     * @return suitable widget that can host view bars widgets or nullptr
    */
    QWidget *createViewBar (KTextEditor::View *)
    {
      return bottomViewBarContainer ();
    }

    /**
     * Delete the view bar for the given view.
     * @param view view for which we want an view bar
     */
    void deleteViewBar (KTextEditor::View *view)
    {
      deleteBottomViewBarForView (view);
    }

    /**
     * Add a widget to the view bar.
     * @param view view for which the view bar is used
     * @param bar bar widget, shall have the viewBarParent() as parent widget
     */
    void addWidgetToViewBar (KTextEditor::View *view, QWidget *bar)
    {
      addToBottomViewBarContainer (view, bar);
    }
    
    /**
     * Show the view bar for the given view
     * @param view view for which the view bar is used
     */
    void showViewBar (KTextEditor::View *view)
    {
      showBottomViewBarForView (view);
    }
    
    /**
     * Hide the view bar for the given view
     * @param view view for which the view bar is used
     */
    void hideViewBar (KTextEditor::View *view)
    {
      hideBottomViewBarForView (view);
    }
    
  private Q_SLOTS:
    void slotUpdateBottomViewBar();

  private Q_SLOTS:
    void slotDocumentCloseAll();
    void slotDocumentCloseOther();
    void slotDocumentCloseOther(KTextEditor::Document *document);
    void slotDocumentCloseSelected(const QList<KTextEditor::Document*>&);
  private:
    static uint uniqueID;
    uint myID;

    Kate::MainWindow *m_mainWindow;

    bool modNotification;

    /**
     * stacked widget containing the central area, aka view manager, quickopen, ...
     */
    QStackedWidget *m_mainStackedWidget;
    
    /**
     * quick open to fast switch documents
     */
    KateQuickOpen *m_quickOpen;
    
    /**
     * keeps track of views
     */
    KateViewManager *m_viewManager;

    KRecentFilesAction *fileOpenRecent;

    KActionMenu* documentOpenWith;

    KToggleAction* settingsShowFileselector;

    bool m_modignore;

    // all plugin views for this mainwindow, used by the pluginmanager
    QHash<Kate::Plugin*, Kate::PluginView*> m_pluginViews;

    // options: show statusbar + show path
    KToggleAction *m_paShowPath;
    KToggleAction *m_paShowStatusBar;
    QWidget *m_bottomViewBarContainer;
    KateContainerStackedLayout *m_bottomContainerStack;

    class BarState{
      public:
        BarState():m_bar(0),m_state(false){}
        BarState(QWidget* bar):m_bar(bar),m_state(false){}
        ~BarState(){}
        QWidget *bar(){return m_bar;}
        bool state(){return m_state;}
        void setState(bool state){m_state=state;}
      private:
        QWidget *m_bar;
        bool m_state;
    };
    QHash<KTextEditor::View*,BarState> m_bottomViewBarMapping;

  public:
    static void unsetModifiedOnDiscDialogIfIf(KateMwModOnHdDialog* diag) {
      if (s_modOnHdDialog==diag) s_modOnHdDialog=0;
    }
  private:
    static KateMwModOnHdDialog *s_modOnHdDialog;
    
    /**
     * Wrapper of main window for KTextEditor
     */
    KTextEditor::MainWindow *m_wrapper;

  public Q_SLOTS:
    void showPluginConfigPage(Kate::PluginConfigPageInterface *configpageinterface,uint id);

    void slotWindowActivated ();
    
  protected:
    virtual bool event( QEvent *e );
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
