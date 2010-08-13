/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesqlconfigpage.h"
#include "outputstylewidget.h"

#include <kconfiggroup.h>
#include <klocale.h>

#include <qboxlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

KateSQLConfigPage::KateSQLConfigPage( QWidget* parent )
: Kate::PluginConfigPage( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  m_box = new QCheckBox(i18n("Save and restore connections in Kate session"), this);

  QGroupBox *stylesGroupBox = new QGroupBox(i18n("Output Customization"), this);
  QVBoxLayout *stylesLayout = new QVBoxLayout(stylesGroupBox);

  m_outputStyleWidget = new OutputStyleWidget(this);

  stylesLayout->addWidget(m_outputStyleWidget);

  layout->addWidget(m_box);
  layout->addWidget(stylesGroupBox, 1);

  setLayout(layout);

  reset();

  connect(m_box, SIGNAL(stateChanged(int)), this, SIGNAL(changed()));
  connect(m_outputStyleWidget, SIGNAL(changed()), this, SIGNAL(changed()));
}


KateSQLConfigPage::~KateSQLConfigPage()
{
}


void KateSQLConfigPage::apply()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  config.writeEntry("SaveConnections", m_box->isChecked());

  m_outputStyleWidget->writeConfig();

  config.sync();

  emit settingsChanged();
}


void KateSQLConfigPage::reset()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  m_box->setChecked(config.readEntry("SaveConnections", true));

  m_outputStyleWidget->readConfig();
}


void KateSQLConfigPage::defaults()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  config.revertToDefault("SaveConnections");
  config.revertToDefault("OutputCustomization");
}
