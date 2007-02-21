/* This file is part of the KDE project
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>
 
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

#ifndef _KATE_SAVE_MODIFIED_DIALOG_
#define _KATE_SAVE_MODIFIED_DIALOG_

#include <KTextEditor/Document>

#include <KDialog>
#include <QList>

class QTreeWidget;
class QTreeWidgetItem;

class KateSaveModifiedDialog: public KDialog
{
    Q_OBJECT
  public:
    KateSaveModifiedDialog(QWidget *parent, QList<KTextEditor::Document*> documents);
    virtual ~KateSaveModifiedDialog();
    static bool queryClose(QWidget *parent, QList<KTextEditor::Document*> documents);
  protected:
    virtual void slotUser2();
    virtual void slotUser1();
    bool doSave(QTreeWidgetItem *root);
  protected Q_SLOTS:
    void slotSelectAll();
    void slotItemActivated(QTreeWidgetItem*, int);

  private:
    QTreeWidgetItem *m_projectRoot;
    QTreeWidgetItem *m_documentRoot;
    QTreeWidget *m_list;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

