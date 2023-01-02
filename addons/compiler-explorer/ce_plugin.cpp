/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "ce_plugin.h"
#include "ce_widget.h"
#include "ktexteditor_utils.h"

#include <QAction>

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(CEPluginFactory, "ce_plugin.json", registerPlugin<CEPlugin>();)

CEPlugin::CEPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
}

CEPlugin::~CEPlugin() = default;

QObject *CEPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new CEPluginView(this, mainWindow);
}

CEPluginView::CEPluginView(CEPlugin *, KTextEditor::MainWindow *mainwindow)
    : QObject(mainwindow)
    , m_mainWindow(mainwindow)
{
    setComponentName(QStringLiteral("compilerexplorer"), i18n("Compiler Explorer"));
    // create our one and only action
    QAction *a = actionCollection()->addAction(QStringLiteral("kate_open_ce_tab"));
    a->setText(i18n("&Open Current File in Compiler Explorer"));
    connect(a, &QAction::triggered, this, &CEPluginView::openANewTab);

    // register us at the UI
    mainwindow->guiFactory()->addClient(this);
}

CEPluginView::~CEPluginView()
{
    // remove us from the UI again
    m_mainWindow->guiFactory()->removeClient(this);
}

void CEPluginView::openANewTab()
{
    if (!m_mainWindow->activeView() || !m_mainWindow->activeView()->document()) {
        Utils::showMessage(i18n("No file open"), {}, i18nc("error category title", "CompilerExplorer"), QStringLiteral("Error"));
        return;
    }

    m_mainWidget = new CEWidget(this, m_mainWindow);

    QWidget *mw = m_mainWindow->window();
    // clang-format off
    QMetaObject::invokeMethod(mw, "addWidget", Q_ARG(QWidget*, m_mainWidget));
    // clang-format on
}

// required for TextFilterPluginFactory vtable
#include "ce_plugin.moc"
