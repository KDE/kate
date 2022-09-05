/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef FILEHISTORYWIDGET_H
#define FILEHISTORYWIDGET_H

#include "kateprivate_export.h"

class QString;
namespace KTextEditor
{
class MainWindow;
}

class FileHistory
{
public:
    /**
     * @brief shows git file history of @p file
     * @param file the file whose history you want to see
     * @param mainWindow the mainWindow where the toolview with history will open. If null, active mainWindow
     * will be used
     */
    static KATE_PRIVATE_EXPORT void showFileHistory(const QString &file, KTextEditor::MainWindow *mainWindow = nullptr);
};

#endif // FILEHISTORYWIDGET_H
