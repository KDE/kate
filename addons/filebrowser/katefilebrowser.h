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

#include <ktexteditor/mainwindow.h>

#include <KFile>

#include <QUrl>
#include <QWidget>

class KateBookmarkHandler;
class KActionCollection;
class KDirOperator;
class KFileItem;
class KHistoryComboBox;
class KToolBar;
class KConfigGroup;
class KUrlNavigator;

class QAbstractItemView;
class QAction;

/*
    The kate file selector presents a directory view, in which the default action is
    to open the activated file.
    Additionally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/

class KateFileBrowser : public QWidget
{
    Q_OBJECT

  public:
    explicit KateFileBrowser( KTextEditor::MainWindow *mainWindow = nullptr,
                      QWidget * parent = nullptr);
    ~KateFileBrowser() override;

    void readSessionConfig (const KConfigGroup& config);
    void writeSessionConfig (KConfigGroup& config);

    void setupToolbar();
    void setView( KFile::FileView );
    KDirOperator *dirOperator() { return m_dirOperator; }

    KActionCollection* actionCollection()
    { return m_actionCollection; }

  public Q_SLOTS:
    void slotFilterChange(const QString&);
    void setDir(QUrl);
    void setDir( const QString &url ) { setDir( QUrl( url ) ); }
    void selectorViewChanged( QAbstractItemView * );

  private Q_SLOTS:
    void fileSelected(const KFileItem & /*file*/);
    void updateDirOperator( const QUrl &u );
    void updateUrlNavigator( const QUrl &u );
    void setActiveDocumentDir();
    void autoSyncFolder();

  protected:
    QUrl activeDocumentUrl();
    void openSelectedFiles();
    void setupActions();

  public:
    KTextEditor::MainWindow* mainWindow()
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
    QAction *m_autoSyncFolder;

    KTextEditor::MainWindow *m_mainWindow;
};

#endif //KATE_FILEBROWSER_H

// kate: space-indent on; indent-width 2; replace-tabs on;
