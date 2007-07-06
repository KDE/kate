/*
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
 
    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#ifndef _KATE_MW_MODONHD_DIALOG_H_
#define _KATE_MW_MODONHD_DIALOG_H_

#include <KTextEditor/Document>

#include <KDialog>

#include <QVector>

#include <QTreeWidget>
#include <QTreeWidgetItem>

typedef  QVector<KTextEditor::Document*> DocVector;
class KProcess;
class KTemporaryFile;

/**
 * A dialog for handling multiple documents modified on disk
 * from within KateMainWindow
 */
class KateMwModOnHdDialog : public KDialog
{
    Q_OBJECT
  public:
    KateMwModOnHdDialog( DocVector docs, QWidget *parent = 0, const char *name = 0 );
    ~KateMwModOnHdDialog();

  protected Q_SLOTS:
    void slotUser1();
    void slotUser2();
    void slotUser3();

  private Q_SLOTS:
    void slotDiff();
    void slotSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *);
    void slotDataAvailable();
    void slotPDone();

  private:
    enum Action { Ignore, Overwrite, Reload };
    void handleSelected( int action );
    class QTreeWidget *twDocuments;
    class QPushButton *btnDiff;
    KProcess *m_proc;
    KTemporaryFile *m_diffFile;
};

#endif // _KATE_MW_MODONHD_DIALOG_H_
// kate: space-indent on; indent-width 2; replace-tabs on;
