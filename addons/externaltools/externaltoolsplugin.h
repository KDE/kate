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
#ifndef KTEXTEDITOR_EXTERNALTOOLS_PLUGIN_H
#define KTEXTEDITOR_EXTERNALTOOLS_PLUGIN_H

#include <QVector>

#include <KXMLGUIClient>
#include <KTextEditor/Plugin>

namespace KTextEditor { class View; }

class KateExternalToolsMenuAction;
class KateExternalToolsPluginView;
class KateExternalToolsCommand;
class KateExternalTool;
class KateToolRunner;

class KateExternalToolsPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateExternalToolsPlugin(QObject* parent = nullptr, const QList<QVariant>& = QList<QVariant>());
    virtual ~KateExternalToolsPlugin();

    /**
     * Reimplemented to return the number of config pages, in this case 1.
     */
    int configPages() const override;

    /**
     * Reimplemented to return the KateExternalToolConfigWidget for number==0.
     */
    KTextEditor::ConfigPage* configPage(int number = 0, QWidget* parent = nullptr) override;

    /**
     * Reimplemented to instanciate a PluginView for each MainWindow.
     */
    QObject* createView(KTextEditor::MainWindow* mainWindow) override;

    /**
     * Reloads the external tools from disk.
     */
    void reload();

    /**
     * Returns a list of KTextEDitor::Command strings. This is needed by
     * the KateExternalToolsCommand constructor to pass the list of commands to
     * the KTextEditor::Editor.
     */
    QStringList commands() const;

    /**
     * Returns the KateExternalTool for a specific command line command 'cmd.
     */
    const KateExternalTool* toolForCommand(const QString& cmd) const;

    /**
     * Returns a list of all existing external tools.
     */
    const QVector<KateExternalTool*> & tools() const;

    /**
     * Executes the tool based on the view as current document.
     */
    void runTool(const KateExternalTool& tool, KTextEditor::View* view);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the external tools change.
     * This is typically the case when external tools were modified,
     * added, or removed via the config page.
     */
    void externalToolsChanged();

private:
    QVector<KateExternalTool*> m_tools;
    QStringList m_commands;
    KateExternalToolsCommand* m_command = nullptr;

private Q_SLOT:
    /**
     * Called whenever an external tool is done.
     */
    void handleToolFinished(KateToolRunner* runner);
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

private:
    KateExternalToolsPlugin* m_plugin;
    KTextEditor::MainWindow* m_mainWindow;
    KateExternalToolsMenuAction* m_externalToolsMenu = nullptr;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
