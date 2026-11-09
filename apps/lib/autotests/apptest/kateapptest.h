/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.ahmed@kdab.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Cursor>

#include <QObject>
#include <QSize>
#include <QStringList>

#include <memory>

class KateApp;
class KateMainWindow;
class QAction;
class QCommandLineParser;
class QTemporaryDir;
class QWidget;

namespace KTextEditor
{
class MainWindow;
class View;
}

/**
 * A complete Kate (KateApp + KateMainWindow) for whole app tests, with chosen plugins and its own
 * temporary config, sessions and files. Tests drive it the way a user would, e.g. open files,
 * trigger actions, read tool views.
 *
 */
class KateAppTest
{
public:
    /**
     * @param plugins plugins to load, by their save name, e.g. "katesearchplugin"
     */
    explicit KateAppTest(const QStringList &plugins = {});
    ~KateAppTest();
    Q_DISABLE_COPY_MOVE(KateAppTest)

    /**
     * @return the main window
     */
    [[nodiscard]] KTextEditor::MainWindow *mainWindow() const;

    /**
     * @return the view of @p plugin in the main window
     */
    [[nodiscard]] QObject *pluginView(const QString &plugin) const;

    /**
     * @return the action @p name from the action collection of the view of @p plugin
     */
    [[nodiscard]] QAction *action(const QString &plugin, const QString &name) const;

    /**
     * @return the action @p name of the main window
     */
    [[nodiscard]] QAction *appAction(const QString &name) const;

    /**
     * Open the file at @p path in the active split and activate it.
     * @param cursor cursor position to set, if valid
     * @return the view of the file, nullptr if it couldn't be opened
     */
    [[nodiscard]] KTextEditor::View *openFile(const QString &path, KTextEditor::Cursor cursor = KTextEditor::Cursor::invalid());

    /**
     * Write @p text into the file @p fileName in the temporary directory, subdirectories are created.
     * @return the absolute path of the file, empty on failure
     */
    [[nodiscard]] QString writeFile(const QString &fileName, const QString &text) const;

    /**
     * Close all documents without asking, e.g. to start each test with a clean state.
     * @return false if a document couldn't be closed
     */
    [[nodiscard]] bool closeAllDocuments();

    /**
     * @return the tool view with the given identifier, as passed to createToolView(), nullptr if not there
     */
    [[nodiscard]] QWidget *toolView(const QString &identifier) const;

    /**
     * Show the tool view with the given identifier.
     * @return false if there is no such tool view
     */
    [[nodiscard]] bool showToolView(const QString &identifier);

    /**
     * @return the temporary directory, removed when this is destroyed
     */
    [[nodiscard]] QString tempDir() const;

private:
    // order matters: destroyed in reverse order, the app before what it uses
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<QCommandLineParser> m_parser;
    std::unique_ptr<KateApp> m_app;
    KateMainWindow *m_mainWindow = nullptr;
};
