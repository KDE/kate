/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "RBQLPlugin.h"
#include "RBQLWidget.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(RBQLPluginFactory, "RBQLPlugin.json", registerPlugin<RBQLPlugin>();)

RBQLPlugin::RBQLPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

QObject *RBQLPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new RBQLPluginView(this, mainWindow);
}

RBQLPluginView::RBQLPluginView(RBQLPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_mainWindow(mainWin)
{
    auto toolview = mainWin->createToolView(plugin,
                                            QStringLiteral("rainbow_csv_toolview"),
                                            KTextEditor::MainWindow::Bottom,
                                            QIcon::fromTheme(QStringLiteral("text-csv")),
                                            i18n("RBQL"));
    m_toolview.reset(toolview);
    new RBQLWidget(mainWin, toolview);
    m_mainWindow->guiFactory()->addClient(this);
}

RBQLPluginView::~RBQLPluginView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

#include "RBQLPlugin.moc"
