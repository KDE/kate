/*
   This file is part of the Kate text editor of the KDE project.
   It describes a "external tools" action for kate and provides a dialog
   page to configure that.

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
   Copyright (C) 2004 Anders Lund <anders@alweb.dk>
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
*/

#ifndef KTEXTEDITOR_EXTERNALTOOLS_H
#define KTEXTEDITOR_EXTERNALTOOLS_H

#include <KTextEditor/MainWindow>

#include <KActionMenu>
#include <KMacroExpander>
#include <QDialog>

#include <QHash>
#include <QPixmap>
#include <QProcess>

class KActionCollection;
class KateExternalToolsPlugin;
class KateExternalTool;

/**
 * The external tools action
 * This action creates a menu, in which each item will launch a process
 * with the provided arguments, which may include the following macros:
 * - %URLS: the URLs of all open documents.
 * - %URL: The URL of the active document.
 * - %filedir: The directory of the current document, if that is a local file.
 * - %selection: The selection of the active document.
 * - %text: The text of the active document.
 * - %line: The line number of the cursor in the active view.
 * - %column: The column of the cursor in the active view.
 *
 * Each item has the following properties:
 * - Name: The friendly text for the menu
 * - Exec: The command to execute, including arguments.
 * - TryExec: the name of the executable, if not available, the
 *       item will not be displayed.
 * - MimeTypes: An optional list of mimetypes. The item will be disabled or
 *       hidden if the current file is not of one of the indicated types.
 *
 */
class KateExternalToolsMenuAction : public KActionMenu
{
    Q_OBJECT
public:
    KateExternalToolsMenuAction(const QString& text, KActionCollection* collection, KateExternalToolsPlugin* plugin,
                                class KTextEditor::MainWindow* mw = nullptr);
    virtual ~KateExternalToolsMenuAction();

    /**
     * This will load all the configured services.
     */
    void reload();

    KActionCollection* actionCollection() { return m_actionCollection; }

private Q_SLOTS:
    void slotViewChanged(KTextEditor::View* view);

private:
    KateExternalToolsPlugin* m_plugin;
    KTextEditor::MainWindow* m_mainwindow; // for the actions to access view/doc managers
    KActionCollection* m_actionCollection;
};

#endif // KTEXTEDITOR_EXTERNALTOOLS_H

// kate: space-indent on; indent-width 4; replace-tabs on;
