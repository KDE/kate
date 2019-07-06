/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *   Copyright (C) 2019 by Mark Nauwelaerts <mark.nauwelaerts@gmail.com>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "lspclientplugin.h"
#include "lspclientpluginview.h"
#include "lspclientconfigpage.h"

#include <KConfigGroup>
#include <KDirWatch>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDir>

K_PLUGIN_FACTORY_WITH_JSON(LSPClientPluginFactory, "lspclientplugin.json", registerPlugin<LSPClientPlugin>();)

LSPClientPlugin::LSPClientPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
    readConfig();
}

LSPClientPlugin::~LSPClientPlugin()
{
}

QObject *LSPClientPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return LSPClientPluginView::new_(this, mainWindow);
}

int LSPClientPlugin::configPages() const
{
    return 1;
}

KTextEditor::ConfigPage *LSPClientPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }

    return new LSPClientConfigPage(parent, this);
}

void LSPClientPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("lspclient"));
    m_symbolDetails = config.readEntry(QStringLiteral("SymbolDetails"), false);
    m_symbolTree = config.readEntry(QStringLiteral("SymbolTree"), true);
    m_symbolExpand = config.readEntry(QStringLiteral("SymbolExpand"), true);
    m_symbolSort = config.readEntry(QStringLiteral("SymbolSort"), false);
    m_complDoc = config.readEntry(QStringLiteral("CompletionDocumentation"), true);
    m_configPath = config.readEntry(QStringLiteral("ServerConfiguration"), QUrl());

    emit update();
}

void LSPClientPlugin::writeConfig() const
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("lspclient"));
    config.writeEntry(QStringLiteral("SymbolDetails"), m_symbolDetails);
    config.writeEntry(QStringLiteral("SymbolTree"), m_symbolTree);
    config.writeEntry(QStringLiteral("SymbolExpand"), m_symbolExpand);
    config.writeEntry(QStringLiteral("SymbolSort"), m_symbolSort);
    config.writeEntry(QStringLiteral("CompletionDocumentation"), m_complDoc);
    config.writeEntry(QStringLiteral("ServerConfiguration"), m_configPath);

    emit update();
}

#include "lspclientplugin.moc"
