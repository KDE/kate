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
#include <klocalizedstring.h>
#include <KSharedConfig>

#include <qboxlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

KateSQLConfigPage::KateSQLConfigPage( QWidget* parent )
: KTextEditor::ConfigPage( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  m_box = new QCheckBox(i18nc("@option:check", "Save and restore connections in Kate session"), this);

  QGroupBox *stylesGroupBox = new QGroupBox(i18nc("@title:group", "Output Customization"), this);
  QVBoxLayout *stylesLayout = new QVBoxLayout(stylesGroupBox);

  m_outputStyleWidget = new OutputStyleWidget(this);

  stylesLayout->addWidget(m_outputStyleWidget);

  layout->addWidget(m_box);
  layout->addWidget(stylesGroupBox, 1);

  setLayout(layout);

  reset();

  connect(m_box, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
  connect(m_outputStyleWidget, &OutputStyleWidget::changed, this, &KateSQLConfigPage::changed);
}


KateSQLConfigPage::~KateSQLConfigPage()
{
}

QString KateSQLConfigPage::name() const
{
    return i18nc("@title", "SQL");
}

QString KateSQLConfigPage::fullName() const
{
    return i18nc("@title:window", "SQL ConfigPage Settings");
}

QIcon KateSQLConfigPage::icon() const
{
    return QIcon::fromTheme(QLatin1String ("server-database"));
}

void KateSQLConfigPage::apply()
{
  KConfigGroup config(KSharedConfig::openConfig(), "KateSQLPlugin");

  config.writeEntry("SaveConnections", m_box->isChecked());

  m_outputStyleWidget->writeConfig();

  config.sync();

  emit settingsChanged();
}


void KateSQLConfigPage::reset()
{
  KConfigGroup config(KSharedConfig::openConfig(), "KateSQLPlugin");

  m_box->setChecked(config.readEntry("SaveConnections", true));

  m_outputStyleWidget->readConfig();
}


void KateSQLConfigPage::defaults()
{
  KConfigGroup config(KSharedConfig::openConfig(), "KateSQLPlugin");

  config.revertToDefault("SaveConnections");
  config.revertToDefault("OutputCustomization");
}
