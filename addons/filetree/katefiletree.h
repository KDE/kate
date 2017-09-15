/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

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

#ifndef KATE_FILETREE_H
#define KATE_FILETREE_H

#include <QUrl>
#include <QIcon>
#include <QTreeView>

namespace KTextEditor
{
class Document;
}

class QActionGroup;

class KateFileTree: public QTreeView
{
    Q_OBJECT

public:

    KateFileTree(QWidget *parent);
    ~KateFileTree() override;

    void setModel(QAbstractItemModel *model) override;

public Q_SLOTS:
    void slotDocumentClose();
    void slotExpandRecursive();
    void slotCollapseRecursive();
    void slotDocumentCloseOther();
    void slotDocumentReload();
    void slotCopyFilename();
    void slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void slotDocumentFirst();
    void slotDocumentLast();
    void slotDocumentNext();
    void slotDocumentPrev();
    void slotPrintDocument();
    void slotPrintDocumentPreview();
    void slotResetHistory();
    void slotDocumentDelete();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

Q_SIGNALS:
    void closeDocument(KTextEditor::Document *);
    void activateDocument(KTextEditor::Document *);

    void openDocument(QUrl);

    void viewModeChanged(bool treeMode);
    void sortRoleChanged(int);

private Q_SLOTS:
    void mouseClicked(const QModelIndex &index);

    void slotTreeMode();
    void slotListMode();

    void slotSortName();
    void slotSortPath();
    void slotSortOpeningOrder();
    void slotFixOpenWithMenu();
    void slotOpenWithMenuAction(QAction *a);

    void slotRenameFile();

private:
    QAction *setupOption(QActionGroup *group, const QIcon &, const QString &, const QString &, const char *slot, bool checked = false);

private:
    QAction *m_filelistCloseDocument;
    QAction *m_filelistExpandRecursive;
    QAction *m_filelistCollapseRecursive;
    QAction *m_filelistCloseOtherDocument;
    QAction *m_filelistReloadDocument;
    QAction *m_filelistCopyFilename;
    QAction *m_filelistRenameFile;
    QAction *m_filelistPrintDocument;
    QAction *m_filelistPrintDocumentPreview;
    QAction *m_filelistDeleteDocument;

    QAction *m_treeModeAction;
    QAction *m_listModeAction;

    QAction *m_sortByFile;
    QAction *m_sortByPath;
    QAction *m_sortByOpeningOrder;
    QAction *m_resetHistory;

    QPersistentModelIndex m_previouslySelected;
    QPersistentModelIndex m_indexContextMenu;
};

#endif // KATE_FILETREE_H

