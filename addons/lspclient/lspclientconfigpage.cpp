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

#include "lspclientconfigpage.h"
#include "lspclientplugin.h"

#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KUrlRequester>

LSPClientConfigPage::LSPClientConfigPage(QWidget *parent, LSPClientPlugin *plugin)
    : KTextEditor::ConfigPage(parent), m_plugin(plugin)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *outlineBox = new QGroupBox(i18n("Symbol Outline Options"), this);
    QVBoxLayout *top = new QVBoxLayout(outlineBox);
    m_symbolDetails = new QCheckBox(i18n("Display symbol details"));
    m_symbolTree = new QCheckBox(i18n("Tree mode outline"));
    m_symbolExpand = new QCheckBox(i18n("Automatically expand nodes in tree mode"));
    m_symbolSort = new QCheckBox(i18n("Sort symbols alphabetically"));
    top->addWidget(m_symbolDetails);
    top->addWidget(m_symbolTree);
    top->addWidget(m_symbolExpand);
    top->addWidget(m_symbolSort);
    layout->addWidget(outlineBox);

    outlineBox = new QGroupBox(i18n("General Options"), this);
    top = new QVBoxLayout(outlineBox);
    m_complDoc = new QCheckBox(i18n("Show selected completion documentation"));
    top->addWidget(m_complDoc);
    layout->addWidget(outlineBox);

    outlineBox = new QGroupBox(i18n("Server Configuration"), this);
    top = new QVBoxLayout(outlineBox);
    m_configPath = new KUrlRequester(this);
    top->addWidget(m_configPath);
    layout->addWidget(outlineBox);

    layout->addStretch(1);

    reset();

    for (const auto & cb : {m_symbolDetails, m_symbolExpand, m_symbolSort, m_symbolTree, m_complDoc})
        connect(cb, &QCheckBox::toggled, this, &LSPClientConfigPage::changed);
    connect(m_configPath, &KUrlRequester::textChanged, this, &LSPClientConfigPage::changed);
    connect(m_configPath, &KUrlRequester::urlSelected, this, &LSPClientConfigPage::changed);
}

QString LSPClientConfigPage::name() const
{
    return QString(i18n("LSP Client"));
}

QString LSPClientConfigPage::fullName() const
{
    return QString(i18n("LSP Client"));
}

QIcon LSPClientConfigPage::icon() const
{
    return QIcon::fromTheme(QLatin1String("code-context"));
}

void LSPClientConfigPage::apply()
{
    m_plugin->m_symbolDetails = m_symbolDetails->isChecked();
    m_plugin->m_symbolTree = m_symbolTree->isChecked();
    m_plugin->m_symbolExpand = m_symbolExpand->isChecked();
    m_plugin->m_symbolSort = m_symbolSort->isChecked();

    m_plugin->m_complDoc = m_complDoc->isChecked();

    m_plugin->m_configPath = m_configPath->url();

    m_plugin->writeConfig();
}

void LSPClientConfigPage::reset()
{
    m_symbolDetails->setChecked(m_plugin->m_symbolDetails);
    m_symbolTree->setChecked(m_plugin->m_symbolTree);
    m_symbolExpand->setChecked(m_plugin->m_symbolExpand);
    m_symbolSort->setChecked(m_plugin->m_symbolSort);

    m_complDoc->setChecked(m_plugin->m_complDoc);

    m_configPath->setUrl(m_plugin->m_configPath);
}

void LSPClientConfigPage::defaults()
{
    reset();
}
