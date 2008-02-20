/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

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

#ifndef __KATE_FILESELECTOR_H__
#define __KATE_FILESELECTOR_H__


#include <ktexteditor/document.h>
#include <ktexteditor/configpage.h>
#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <kate/pluginconfigpageinterface.h>
#include <kdiroperator.h>
#include <kurlcombobox.h>

#include <kvbox.h>
//Added by qt3to4:
#include <QShowEvent>
#include <QFocusEvent>
#include <QEvent>
#include <kfile.h>
#include <kurl.h>
#include <ktoolbar.h>


class KActionCollection;
class KActionSelector;
class KHistoryComboBox;
class QToolButton;
class QCheckBox;
class QSpinBox;

class KateFileSelector;

class KateFileSelectorPlugin: public Kate::Plugin, public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)
  public:
    explicit KateFileSelectorPlugin( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~KateFileSelectorPlugin()
    {}

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    uint configPages() const;
    Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0);
    QString configPageName (uint number = 0) const;
    QString configPageFullName (uint number = 0) const;
    KIcon configPageIcon (uint number = 0) const;
    KateFileSelector *m_fileSelector;
};

class KateFileSelectorPluginView : public Kate::PluginView
{
    Q_OBJECT

  public:
    /**
      * Constructor.
      */
    KateFileSelectorPluginView (Kate::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateFileSelectorPluginView ();

    virtual void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

    KateFileSelector * kateFileSelector() const { return m_fileSelector; }

  private:
    KateFileSelector *m_fileSelector;
};


/*
    The kate file selector presents a directory view, in which the default action is
    to open the activated file.
    Additionally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/

class KateFileSelectorToolBar: public KToolBar
{
    Q_OBJECT
  public:
    KateFileSelectorToolBar(QWidget *parent);
    virtual ~KateFileSelectorToolBar();
};

class KateFileSelector : public KVBox
{
    Q_OBJECT

    friend class KFSConfigPage;

  public:
    /* When to sync to current document directory */
    enum AutoSyncEvent { DocumentChanged = 1, GotVisible = 2 };

    explicit KateFileSelector( Kate::MainWindow *mainWindow = 0,
                      QWidget * parent = 0, const char * name = 0 );
    ~KateFileSelector();

    virtual void readSessionConfig( KConfigBase *, const QString & );
    virtual void writeSessionConfig( KConfigBase *, const QString & );
    void readConfig();
    void writeConfig();
    void setupToolbar( QStringList actions );
    void setView( KFile::FileView );
    KDirOperator *dirOperator()
    {
      return dir;
    }
    KActionCollection *actionCollection()
    {
      return mActionCollection;
    }

  public Q_SLOTS:
    void slotFilterChange(const QString&);
    void setDir(KUrl);
    void setDir( const QString& url )
    {
      setDir( KUrl( url ) );
    }
    void kateViewChanged();
    void selectorViewChanged( QAbstractItemView * );

  private Q_SLOTS:
    void fileSelected(const KFileItem & /*file*/);
    void cmbPathActivated( const KUrl& u );
    void cmbPathReturnPressed( const QString& u );
    void dirUrlEntered( const KUrl& u );
    void dirFinishedLoading();
    void setActiveDocumentDir();
    void btnFilterClick();

  protected:
    KUrl activeDocumentUrl();
    void focusInEvent( QFocusEvent * );
    void showEvent( QShowEvent * );
    bool eventFilter( QObject *, QEvent * );
    void initialDirChangeHack();
    void openSelectedFiles();

  public:
    Kate::MainWindow* mainWindow()
    {
      return mainwin;
    }
  private:
    class KateFileSelectorToolBar *toolbar;
    KActionCollection *mActionCollection;
    class KBookmarkHandler *bookmarkHandler;
    KUrlComboBox *cmbPath;
    KDirOperator * dir;
    class QAction *acSyncDir;
    KHistoryComboBox * filter;
    QToolButton *btnFilter;

    Kate::MainWindow *mainwin;

    QString lastFilter;
    int autoSyncEvents; // enabled autosync events
    QString waitingUrl; // maybe display when we gets visible
    QString waitingDir;
};

/*  TODO anders
    KFSFilterHelper
    A popup widget presenting a listbox with checkable items
    representing the mime types available in the current directory, and
    providing a name filter based on those.
*/

/*
    Config page for file selector.
    Allows for configuring the toolbar, the history length
    of the path and file filter combos, and how to handle
    user closed session.
*/
class KFSConfigPage : public Kate::PluginConfigPage
{
    Q_OBJECT
  public:
    explicit KFSConfigPage( QWidget* parent = 0, const char *name = 0, KateFileSelector *kfs = 0);
    virtual ~KFSConfigPage()
    {}

    virtual void apply();
    virtual void reset();
    virtual void defaults()
    {}

  private Q_SLOTS:
    void slotMyChanged();

  private:
    void init();

    KateFileSelector *fileSelector;
    KActionSelector *acSel;
    QSpinBox *sbPathHistLength, *sbFilterHistLength;
    QCheckBox *cbSyncActive, *cbSyncShow;
    QCheckBox *cbSesLocation, *cbSesFilter;

    bool m_changed;
};

#endif //__KATE_FILESELECTOR_H__

// kate: space-indent on; indent-width 2; replace-tabs on;
