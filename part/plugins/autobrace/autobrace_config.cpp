/**
  * This file is part of the KDE libraries
  * Copyright (C) 2010 André Stein <andre.stein@rwth-aachen.de>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "autobrace_config.h"
#include "autobrace.h"

#include <QtGui/QBoxLayout>
#include <QtGui/QCheckBox>

#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klineedit.h>
#include <kconfiggroup.h>

AutoBraceConfig::AutoBraceConfig(QWidget *parent, const QVariantList &args)
    : KCModule(AutoBracePluginFactory::componentData(), parent, args)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_autoBrackets = new QCheckBox(i18n("Automatically add closing brackets ) and ]"), this);
    m_autoQuotations = new QCheckBox(i18n("Automatically add closing quotation marks"), this);

    layout->addWidget(m_autoBrackets);
    layout->addWidget(m_autoQuotations);

    setLayout(layout);

    load();

    // Changed slots
    connect(m_autoBrackets, SIGNAL(toggled(bool)), this, SLOT(slotChanged(bool)));
    connect(m_autoQuotations, SIGNAL(toggled(bool)), this, SLOT(slotChanged(bool)));
}

AutoBraceConfig::~AutoBraceConfig()
{
}

void AutoBraceConfig::save()
{
    if (AutoBracePlugin::self())
    {
        AutoBracePlugin::self()->setAutoBrackets(m_autoBrackets->isChecked());
        AutoBracePlugin::self()->setAutoQuotations(m_autoQuotations->isChecked());
        AutoBracePlugin::self()->writeConfig();
    }
    else
    {
        KConfigGroup cg(KGlobal::config(), "AutoBrace Plugin");
        cg.writeEntry("autobrackets", m_autoBrackets->isChecked());
        cg.writeEntry("autoquotations", m_autoQuotations->isChecked());
    }

    emit changed(false);
}

void AutoBraceConfig::load()
{
    if (AutoBracePlugin::self())
    {
        AutoBracePlugin::self()->readConfig();
        m_autoBrackets->setChecked(AutoBracePlugin::self()->autoBrackets());
        m_autoQuotations->setChecked(AutoBracePlugin::self()->autoQuotations());
    }
    else
    {
        KConfigGroup cg(KGlobal::config(), "AutoBrace Plugin" );
        m_autoBrackets->setChecked(cg.readEntry("autobrackets", QVariant(true)).toBool());
        m_autoQuotations->setChecked(cg.readEntry("autoquotations", QVariant(true)).toBool());
    }

    emit changed(false);
}

void AutoBraceConfig::defaults()
{
    m_autoBrackets->setChecked(true);
    m_autoQuotations->setChecked(true);

    emit changed(true);
}

void AutoBraceConfig::slotChanged(bool)
{
    emit changed(true);
}

#include "autobrace_config.moc"

