/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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
#ifndef KTEXTEDITOR_EXTERNALTOOLS_H
#define KTEXTEDITOR_EXTERNALTOOLS_H

namespace KTextEditor { class MainWindow; }
namespace KTextEditor { class View; }

#include <KActionMenu>
#include <KMacroExpander>
#include <KXMLGUIClient>

class KActionCollection;
class KateExternalToolsPlugin;
class KateExternalTool;
namespace Ui { class ToolView; }

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

class KateExternalToolsPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    KateExternalToolsPluginView(KTextEditor::MainWindow* mainWindow, KateExternalToolsPlugin* plugin);

    /**
     * Virtual destructor.
     */
    ~KateExternalToolsPluginView();

    /**
     * Returns the associated mainWindow
     */
    KTextEditor::MainWindow* mainWindow() const;

public Q_SLOTS:
    /**
     * Called by the plugin view to reload the menu
     */
    void rebuildMenu();

    /**
     * Shows the tool view. The toolview will be created, if not yet existing.
     */
    void showToolView();

    /**
     * Shows the External Tools toolview and porints the error message along with
     * some more info about the tool.
     */
    void reportToolError(const QString& message, KateExternalTool* tool);

private:
    KateExternalToolsPlugin* m_plugin;
    KTextEditor::MainWindow* m_mainWindow;
    KateExternalToolsMenuAction* m_externalToolsMenu = nullptr;
    QWidget* m_toolView = nullptr;
    Ui::ToolView* m_ui = nullptr;
};

#endif // KTEXTEDITOR_EXTERNALTOOLS_H

// kate: space-indent on; indent-width 4; replace-tabs on;
