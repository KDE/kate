/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "cmaketoolsplugin.h"

#include <KPluginFactory>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KXMLGUIFactory>

#include <QDebug>

K_PLUGIN_FACTORY_WITH_JSON(CMakeToolsPluginFactory, "cmaketoolsplugin.json", registerPlugin<CMakeToolsPlugin>();)

CMakeToolsPlugin::CMakeToolsPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

CMakeToolsPlugin::~CMakeToolsPlugin() = default;

QObject *CMakeToolsPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new CMakeToolsPluginView(this, mainWindow);
}

CMakeToolsPluginView::CMakeToolsPluginView(CMakeToolsPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : QObject(plugin)
    , m_mainWindow(mainwindow)
{
    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, &CMakeToolsPluginView::onViewCreated);

    /**
     * connect for all already existing views
     */
    const auto views = m_mainWindow->views();
    for (KTextEditor::View *view : views) {
        onViewCreated(view);
    }
}

CMakeToolsPluginView::~CMakeToolsPluginView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

void CMakeToolsPluginView::onViewCreated(KTextEditor::View *v)
{
    if (!CMakeCompletion::isCMakeFile(v->document()->url()))
        return;

    v->registerCompletionModel(&m_completion);
}

#include "cmaketoolsplugin.moc"
