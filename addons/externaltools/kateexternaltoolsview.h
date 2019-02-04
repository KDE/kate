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

class QTextDocument;

class KActionCollection;
class KateExternalToolsPlugin;
class KateExternalTool;

namespace Ui { class ToolView; }

/**
 * Menu action that displays all KateExternalTool in a submenu.
 * Enables/disables the tool actions whenever the view changes, depending on the mimetype.
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

    KActionCollection* actionCollection() const { return m_actionCollection; }

private Q_SLOTS:
    /**
     * Called whenever the current view changed.
     * Required to enable/disable the tools that depend on specific mimetypes.
     */
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
     * Creates the tool view. If already existing, does nothing.
     */
    void createToolView();

    /**
     * Shows the tool view. The toolview will be created, if not yet existing.
     */
    void showToolView();

    /**
     * Clears the toolview data. If no toolview is around, nothing happens.
     */
    void clearToolView();

    /**
     * Shows the External Tools toolview and points the error message along with
     * some more info about the tool.
     */
    void addToolStatus(const QString& message, KateExternalTool* tool);

    /**
     * Sets the output data to data;
     */
    void setOutputData(const QString& data);

    /**
     * Deletes the tool view, if existing.
     */
    void deleteToolView();

private:
    KateExternalToolsPlugin* m_plugin;
    KTextEditor::MainWindow* m_mainWindow;
    KateExternalToolsMenuAction* m_externalToolsMenu = nullptr;
    QWidget* m_toolView = nullptr;
    Ui::ToolView* m_ui = nullptr;
    QTextDocument* m_outputDoc = nullptr;
    QTextDocument* m_statusDoc = nullptr;
};

#endif // KTEXTEDITOR_EXTERNALTOOLS_H

// kate: space-indent on; indent-width 4; replace-tabs on;
