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

#include <kconfiggroup.h>
#include <kcolorbutton.h>
#include <klocale.h>

#include <qlayout.h>
#include <qformlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qradiobutton.h>

KateSQLConfigPage::KateSQLConfigPage( QWidget* parent )
: Kate::PluginConfigPage( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  m_box = new QCheckBox(i18n(
    "Save and restore connections in Kate session (WARNING: passwords will be stored in plain text format)"), this);

  m_nullColorButton = new KColorButton(this);
  m_blobColorButton = new KColorButton(this);

  m_nullColorButton->setDefaultColor(QColor::fromRgb(255,255,191));
  m_blobColorButton->setDefaultColor(QColor::fromRgb(255,255,191));

  m_nullColorButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
  m_blobColorButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  QGroupBox *colorsGroupBox = new QGroupBox(i18n("Output Customization"), this);
  QFormLayout *colorsLayout = new QFormLayout(colorsGroupBox);

//   colorsLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  colorsLayout->addRow(i18n("Background color of NULL values:"), m_nullColorButton);
  colorsLayout->addRow(i18n("Background color of BLOB values:"), m_blobColorButton);

  layout->addWidget(m_box);
  layout->addWidget(colorsGroupBox);

  layout->addStretch(1);

  setLayout(layout);

  reset();

  connect(m_box, SIGNAL(stateChanged(int)), this, SIGNAL(changed()));
  connect(m_nullColorButton, SIGNAL(changed(const QColor&)), this, SIGNAL(changed()));
  connect(m_blobColorButton, SIGNAL(changed(const QColor&)), this, SIGNAL(changed()));
}


KateSQLConfigPage::~KateSQLConfigPage()
{
}


void KateSQLConfigPage::apply()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  config.writeEntry("SaveConnections", m_box->isChecked());

  config.writeEntry("NullBackgroundColor", m_nullColorButton->color());
  config.writeEntry("BlobBackgroundColor", m_blobColorButton->color());

  config.sync();

  emit settingsChanged();
}


void KateSQLConfigPage::reset()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  m_box->setChecked(config.readEntry("SaveConnections", true));

  m_nullColorButton->setColor(config.readEntry("NullBackgroundColor", m_nullColorButton->defaultColor()));
  m_blobColorButton->setColor(config.readEntry("BlobBackgroundColor", m_blobColorButton->defaultColor()));
}


void KateSQLConfigPage::defaults()
{
  KConfigGroup config(KGlobal::config(), "KateSQLPlugin");

  config.revertToDefault("SaveConnections");
  config.revertToDefault("NullBackgroundColor");
  config.revertToDefault("BlobBackgroundColor");
/*
  m_box->setChecked(true);

  m_nullColorButton->setColor(m_nullColorButton->defaultColor());
  m_blobColorButton->setColor(m_blobColorButton->defaultColor());
*/
}
