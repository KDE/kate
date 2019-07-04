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

static const QString CONFIG_LSPCLIENT { QStringLiteral("lspclient") };
static const QString CONFIG_SYMBOL_DETAILS { QStringLiteral("SymbolDetails") };
static const QString CONFIG_SYMBOL_TREE { QStringLiteral("SymbolTree") };
static const QString CONFIG_SYMBOL_EXPAND { QStringLiteral("SymbolExpand") };
static const QString CONFIG_SYMBOL_SORT { QStringLiteral("SymbolSort") };
static const QString CONFIG_COMPLETION_DOC { QStringLiteral("CompletionDocumentation") };
static const QString CONFIG_SERVER_CONFIG { QStringLiteral("ServerConfiguration") };


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
    KConfigGroup config(KSharedConfig::openConfig(), CONFIG_LSPCLIENT);
    m_symbolDetails = config.readEntry(CONFIG_SYMBOL_DETAILS, false);
    m_symbolTree = config.readEntry(CONFIG_SYMBOL_TREE, true);
    m_symbolExpand = config.readEntry(CONFIG_SYMBOL_EXPAND, true);
    m_symbolSort = config.readEntry(CONFIG_SYMBOL_SORT, false);
    m_complDoc = config.readEntry(CONFIG_COMPLETION_DOC, true);
    m_configPath = config.readEntry(CONFIG_SERVER_CONFIG, QUrl());

    emit update();
}

void LSPClientPlugin::writeConfig() const
{
    KConfigGroup config(KSharedConfig::openConfig(), CONFIG_LSPCLIENT);
    config.writeEntry(CONFIG_SYMBOL_DETAILS, m_symbolDetails);
    config.writeEntry(CONFIG_SYMBOL_TREE, m_symbolTree);
    config.writeEntry(CONFIG_SYMBOL_EXPAND, m_symbolExpand);
    config.writeEntry(CONFIG_SYMBOL_SORT, m_symbolSort);
    config.writeEntry(CONFIG_COMPLETION_DOC, m_complDoc);
    config.writeEntry(CONFIG_SERVER_CONFIG, m_configPath);

    emit update();
}

#include "lspclientplugin.moc"
