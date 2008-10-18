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

#ifndef KWRITE_MAIN_H
#define KWRITE_MAIN_H

#include <QtGui/QKeyEvent>
#include <QtCore/QList>

#include <kdialog.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <kparts/mainwindow.h>

class QLabel;

namespace KTextEditor { class EditorChooser; }

class KToggleAction;
class KRecentFilesAction;
class KSqueezedTextLabel;

class KWrite : public KParts::MainWindow
{
  Q_OBJECT

  public:
    KWrite(KTextEditor::Document * = 0L);
    ~KWrite();

    void loadURL(const KUrl &url);

    KTextEditor::View *view() const { return m_view; }

    static bool noWindows () { return winList.isEmpty(); }

  private:
    void setupActions();
    void setupStatusBar();

    bool queryClose();

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

  public Q_SLOTS:
    void slotNew();
    void slotFlush ();
    void slotOpen();
    void slotOpen( const KUrl& url);
    void newView();
    void toggleStatusBar();
    void editKeys();
    void editToolbars();
    void changeEditor();
    void aboutEditor();

  public Q_SLOTS:
    void slotDropEvent(QDropEvent *);

    void slotEnableActions( bool enable );

    /**
     * adds a changed URL to the recent files
     */
    void slotFileNameChanged();

  //config file functions
  public:
    void readConfig (const KConfigGroup &);
    void writeConfig (KConfigGroup &);

    void readConfig ();
    void writeConfig ();

  //session management
  public:
    void restore(KConfig *,int);
    static void restore();

  private:
    void readProperties(const KConfigGroup &);
    void saveProperties(KConfigGroup &);
    void saveGlobalProperties(KConfig *);

  private:
    KTextEditor::View * m_view;

    KRecentFilesAction * m_recentFiles;
    KToggleAction * m_paShowPath;
    KToggleAction * m_paShowStatusBar;

    QString encoding;

    static QList<KTextEditor::Document*> docList;
    static QList<KWrite*> winList;

  /**
   * Stuff for the status bar
   */
  public Q_SLOTS:
    void updateStatus ();

    void viewModeChanged ( KTextEditor::View *view );

    void cursorPositionChanged ( KTextEditor::View *view );

    void selectionChanged (KTextEditor::View *view);

    void modifiedChanged();

    void documentNameChanged ();

    void informationMessage (KTextEditor::View *view, const QString &message);

   private:
      QLabel* m_lineColLabel;
      QLabel* m_modifiedLabel;
      QLabel* m_insertModeLabel;
      QLabel* m_selectModeLabel;
      KSqueezedTextLabel* m_fileNameLabel;
      QPixmap m_modPm; //, m_modDiscPm, m_modmodPm;
};

class KWriteEditorChooser: public KDialog
{
  Q_OBJECT

  public:
    KWriteEditorChooser(QWidget *parent);
    virtual ~KWriteEditorChooser();

  private:
    KTextEditor::EditorChooser *m_chooser;

  protected Q_SLOTS:
    void slotOk();
};

#endif
