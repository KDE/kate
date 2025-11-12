/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katesqlplugin.h"
#include "katesqlconfigpage.h"
#include "katesqlview.h"

#include <KTextEditor/Document>

#include <KLocalizedString>

K_PLUGIN_FACTORY_WITH_JSON(KateSQLFactory, "katesql.json", registerPlugin<KateSQLPlugin>();)

// BEGIN KateSQLPLugin
KateSQLPlugin::KateSQLPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

KateSQLPlugin::~KateSQLPlugin() = default;

QObject *KateSQLPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    auto *view = new KateSQLView(this, mainWindow);

    connect(this, &KateSQLPlugin::globalSettingsChanged, view, &KateSQLView::slotGlobalSettingsChanged);

    return view;
}

KTextEditor::ConfigPage *KateSQLPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }

    auto *page = new KateSQLConfigPage(parent);
    connect(page, &KateSQLConfigPage::settingsChanged, this, &KateSQLPlugin::globalSettingsChanged);

    return page;
}

// END KateSQLPlugin

#include "katesqlplugin.moc"
#include "moc_katesqlplugin.cpp"
