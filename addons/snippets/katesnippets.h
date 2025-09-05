/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>

#include "katesnippetglobal.h"

class SnippetView;
class KateSnippetsPluginView;

class KateSnippetsPlugin : public KTextEditor::Plugin
{
    friend class KateSnippetsPluginView;

public:
    explicit KateSnippetsPlugin(QObject *parent);
    ~KateSnippetsPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    KateSnippetGlobal *m_snippetGlobal;
};

class KateSnippetsPluginView : public QObject, public KXMLGUIClient
{
public:
    /**
     * Constructor.
     */
    KateSnippetsPluginView(KateSnippetsPlugin *plugin, KTextEditor::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateSnippetsPluginView() override;

    void readConfig();

private:
    /**
     * New view got created, we need to update our connections
     * @param view new created view
     */
    void slotViewCreated(KTextEditor::View *view);

    void createSnippet();

private:
    KateSnippetsPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    QPointer<QWidget> m_toolView;
    SnippetView *m_snippets;

    /**
     * remember for which text views we might need to cleanup stuff
     */
    QList<QPointer<KTextEditor::View>> m_textViews;
};
