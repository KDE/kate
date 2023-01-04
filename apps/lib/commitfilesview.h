/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "kateprivate_export.h"

class QString;
namespace KTextEditor
{
class MainWindow;
}

/**
 * This class shows the diff tree for a commit.
 */
class CommitView
{
public:
    /**
     * open treeview for commit with @p hash
     * @filePath can be path of any file in the repo
     */
    static void KATE_PRIVATE_EXPORT openCommit(const QString &hash, const QString &path, KTextEditor::MainWindow *mainWindow);
};
