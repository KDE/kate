/* This file is part of the KDE project
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <ktexteditor/document.h>

#include <QList>
#include <QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;

class KateSaveModifiedDialog: public QDialog
{
    Q_OBJECT
public:
    KateSaveModifiedDialog(QWidget *parent, QList<KTextEditor::Document *> documents);
    ~KateSaveModifiedDialog() override;
    static bool queryClose(QWidget *parent, QList<KTextEditor::Document *> documents);
protected:
    bool doSave();
protected Q_SLOTS:
    void slotSelectAll();
    void slotItemActivated(QTreeWidgetItem *, int);
    void slotSaveSelected();
    void slotDoNotSave();

private:
    QTreeWidgetItem *m_documentRoot;
    QTreeWidget *m_list;
    QPushButton *m_saveButton;
};

#endif

