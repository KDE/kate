/* This file is part of the KDE project
   Copyright (C) 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "replicodeconfigpage.h"
#include "replicodeconfig.h"
#include <KUrlRequester>
#include <KConfigGroup>
#include <KConfig>
#include <KSharedConfig>

#include <QGridLayout>
#include <QLayout>
#include <klocalizedstring.h>
#include <QLabel>
#include <QTabWidget>

ReplicodeConfigPage::ReplicodeConfigPage(QWidget *parent) : KTextEditor::ConfigPage(parent), m_config(new ReplicodeConfig(this))
{
    QGridLayout *gridlayout = new QGridLayout;
    setLayout(gridlayout);
    gridlayout->addWidget(new QLabel(i18n("Path to replicode executor:")), 0, 0);

    m_requester = new KUrlRequester;
    m_requester->setMode(KFile::File | KFile::ExistingOnly);
    gridlayout->addWidget(m_requester, 0, 1);

    gridlayout->addWidget(m_config, 1, 0, 1, 2);

    reset();

    connect(m_requester, &KUrlRequester::textChanged, this, &ReplicodeConfigPage::changed);
}

QString ReplicodeConfigPage::name() const
{
    return i18n("Replicode");
}

QString ReplicodeConfigPage::fullName() const
{
    return i18n("Replicode configuration");
}

void ReplicodeConfigPage::apply()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Replicode"));
    config.writeEntry("replicodePath", m_requester->text());
    m_config->save();
}

void ReplicodeConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Replicode"));
    m_requester->setText(config.readEntry<QString>("replicodePath", QString()));
    m_config->load();
}

void ReplicodeConfigPage::defaults()
{
    m_requester->setText(QString());
    m_config->reset();
}

