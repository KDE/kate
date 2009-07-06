/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

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

#ifndef KATE_FILEBROWSER_H
#define KATE_FILEBROWSER_H

#include <kate/mainwindow.h>

#include <KVBox>
#include <KFile>
#include <KUrl>

class QAbstractItemView;
class KateBookmarkHandler;

class KActionCollection;
class KActionSelector;
class KDirOperator;
class KFileItem;
class KHistoryComboBox;
class KToolBar;
class QToolButton;
class QCheckBox;
class QSpinBox;

class KUrlNavigator;
/*
    The kate file selector presents a directory view, in which the default action is
    to open the activated file.
    Additionally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/

class KateFileBrowser : public KVBox
{
    Q_OBJECT

  public:
    explicit KateFileBrowser( Kate::MainWindow *mainWindow = 0,
                      QWidget * parent = 0, const char * name = 0 );
    ~KateFileBrowser();

    virtual void readSessionConfig( KConfigBase *, const QString & );
    virtual void writeSessionConfig( KConfigBase *, const QString & );

    void setupToolbar();
    void setView( KFile::FileView );
    KDirOperator *dirOperator() { return m_dirOperator; }

    KActionCollection* actionCollection()
    { return m_actionCollection; }

  public Q_SLOTS:
    void slotFilterChange(const QString&);
    void setDir(KUrl);
    void setDir( const QString& url ) { setDir( KUrl( url ) ); }
    void selectorViewChanged( QAbstractItemView * );

  private Q_SLOTS:
    void fileSelected(const KFileItem & /*file*/);
    void updateDirOperator( const KUrl& u );
    void updateUrlNavigator( const KUrl& u );
    void setActiveDocumentDir();

  protected:
    KUrl activeDocumentUrl();
    void focusInEvent( QFocusEvent * );
    void openSelectedFiles();

  public:
    Kate::MainWindow* mainWindow()
    {
      return m_mainWindow;
    }
  private:
    KToolBar *m_toolbar;
    KActionCollection *m_actionCollection;
    KateBookmarkHandler *m_bookmarkHandler;
    KUrlNavigator *m_urlNavigator;
    KDirOperator * m_dirOperator;
    KHistoryComboBox * m_filter;

    Kate::MainWindow *m_mainWindow;
};

#endif //KATE_FILEBROWSER_H

// kate: space-indent on; indent-width 2; replace-tabs on;
