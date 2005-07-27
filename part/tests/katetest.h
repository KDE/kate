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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KATE_TEST_H__
#define __KATE_TEST_H__

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <kparts/mainwindow.h>

#include <kdialogbase.h>

#include <Q3PtrList>

namespace KTextEditor { class EditorChooser; }

class KAction;
class KToggleAction;
class KSelectAction;
class KRecentFilesAction;

class KWrite : public KParts::MainWindow
{
  Q_OBJECT

  public:
    KWrite(KTextEditor::Document * = 0L);
    ~KWrite();

    void loadURL(const KURL &url);

    KTextEditor::View *view() const { return m_view; }

    static bool noWindows () { return winList.isEmpty(); }

  private:
    void setupActions();
    void setupStatusBar();

    bool queryClose();

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

  public slots:
    void slotNew();
    void slotFlush ();
    void slotOpen();
    void slotOpen( const KURL& url);
    void newView();
    void toggleStatusBar();
    void editKeys();
    void editToolbars();
    void changeEditor();

  public slots:
    void printNow();
    void printDlg();

    void newStatus(const QString &msg);
    void newCaption();

    void slotDropEvent(QDropEvent *);

    void slotEnableActions( bool enable );

    /**
     * adds a changed URL to the recent files
     */
    void slotFileNameChanged();

  //config file functions
  public:
    void readConfig (KConfig *);
    void writeConfig (KConfig *);

    void readConfig ();
    void writeConfig ();

  //session management
  public:
    void restore(KConfig *,int);
    static void restore();

  private:
    void readProperties(KConfig *);
    void saveProperties(KConfig *);
    void saveGlobalProperties(KConfig *);

  private:
    KTextEditor::View * m_view;

    KRecentFilesAction * m_recentFiles;
    KToggleAction * m_paShowPath;
    KToggleAction * m_paShowStatusBar;

    QString encoding;

    static Q3PtrList<KTextEditor::Document> docList;
    static Q3PtrList<KWrite> winList;
};

class KWriteEditorChooser: public KDialogBase
{
  Q_OBJECT

  public:
    KWriteEditorChooser(QWidget *parent);
    virtual ~KWriteEditorChooser();

  private:
    KTextEditor::EditorChooser *m_chooser;

  protected slots:
    void slotOk();
};

#endif
