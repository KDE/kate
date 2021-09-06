/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "cmaketoolsplugin.h"

#include <KPluginFactory>
#include <KTextEditor/MainWindow>
#include <KXMLGUIFactory>
#include <ktexteditor/codecompletioninterface.h>

#include <QDebug>

K_PLUGIN_FACTORY_WITH_JSON(CMakeToolsPluginFactory, "cmaketoolsplugin.json", registerPlugin<CMakeToolsPlugin>();)

CMakeToolsPlugin::CMakeToolsPlugin(QObject *parent, const QList<QVariant> &)
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

    qWarning() << "Registering code completion model for view <<" << v << v->document()->url();

    KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(v);
    if (cci) {
        cci->registerCompletionModel(&m_completion);
    }
}

#include "cmaketoolsplugin.moc"
