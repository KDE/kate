/* This file is part of the KDE project
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef FILETREE_MODEL_TEST_H
#define FILETREE_MODEL_TEST_H

#include <QObject>

class KateFileTreeModel;
class ResultNode;

class FileTreeModelTest : public QObject
{
  Q_OBJECT

  private Q_SLOTS:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();

    void basic();

    void buildTree_data();
    void buildTree();
    void buildTreeFullPath_data();
    void buildTreeFullPath();
    void buildTreeBatch_data();
    void buildTreeBatch();
    void buildTreeBatchPrefill_data();
    void buildTreeBatchPrefill();
    
    void listMode_data();
    void listMode();

    void deleteDocument_data();
    void deleteDocument();
    void deleteDocumentBatch_data();
    void deleteDocumentBatch();

    void rename_data();
    void rename();

  private:
    void walkTree(KateFileTreeModel &model, const QModelIndex &i, ResultNode &node);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
