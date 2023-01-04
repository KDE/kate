/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2013 Dominik Haumann <dhaumann.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QPoint>
#include <QString>

class QModelIndex;
class KateProjectViewTree;

class KateProjectTreeViewContextMenu
{
public:
    /**
     * our project.
     * @return project
     */
    static void exec(const QString &filename, const QModelIndex &index, const QPoint &pos, KateProjectViewTree *parent);
};
