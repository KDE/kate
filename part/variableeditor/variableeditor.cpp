/* This file is part of the KDE project

   Copyright (C) 2011 Dominik Haumann <dhaumann kde org>

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

#include "variableeditor.h"
#include "variableitem.h"

#include <QtGui/QPainter>
#include <QtCore/QVariant>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>

#include <kiconloader.h>
#include <klocale.h>

#include <QtCore/QDebug>

//BEGIN VariableEditor
VariableEditor::VariableEditor(VariableItem* item, QWidget* parent)
  : QWidget(parent)
  , m_item(item)
{
  setAutoFillBackground(true);
  QGridLayout* l = new QGridLayout(this);
  l->setMargin(10);

  m_checkBox = new QCheckBox(this);
  m_variable = new QLabel(item->variable(), this);
  m_variable->setFocusPolicy(Qt::ClickFocus);
  m_variable->setFocusProxy(m_checkBox);
  m_pixmap = new QLabel(this);
  m_pixmap->setPixmap(SmallIcon("flag"));
  m_helpText = new QLabel(item->helpText(), this);

  l->addWidget(m_checkBox, 0, 0, Qt::AlignLeft);
  l->addWidget(m_variable, 0, 1, Qt::AlignLeft);
  l->addWidget(m_pixmap, 0, 3, Qt::AlignRight);
  l->addWidget(m_helpText, 1, 1, 1, 3, Qt::AlignLeft);
  
  l->setColumnStretch(0, 0);
  l->setColumnStretch(1, 1);
  l->setColumnStretch(2, 1);
  l->setColumnStretch(3, 0);

  setLayout(l);

  connect(m_checkBox, SIGNAL(toggled(bool)), this, SLOT(itemEnabled(bool)));
  m_checkBox->setChecked(item->isActive());

  connect(m_checkBox, SIGNAL(toggled(bool)), this, SIGNAL(valueChanged()));
}

VariableEditor::~VariableEditor()
{
}

void VariableEditor::itemEnabled(bool enabled)
{
  if (enabled) {
    m_variable->setText("<b>" + m_item->variable() + "</b>");
  } else {
    m_variable->setText(m_item->variable());
  }
  m_item->setActive(enabled);
}

void VariableEditor::activateItem()
{
  m_checkBox->setChecked(true);
}

VariableItem* VariableEditor::item() const
{
  return m_item;
}
//END VariableEditor




//BEGIN VariableUintEditor
VariableUintEditor::VariableUintEditor(VariableUintItem* item, QWidget* parent)
  : VariableEditor(item, parent)
{
  QGridLayout* l = (QGridLayout *) layout();

  m_spinBox = new QSpinBox(this);
  m_spinBox->setValue(item->value());
  l->addWidget(m_spinBox, 0, 2, Qt::AlignLeft);

  connect(m_spinBox, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged()));
  connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(activateItem()));
  connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(setItemValue(int)));
}
void VariableUintEditor::setItemValue(int newValue)
{
  static_cast<VariableUintItem*>(item())->setValue(newValue);
}

VariableBoolEditor::VariableBoolEditor(VariableBoolItem* item, QWidget* parent)
  : VariableEditor(item, parent)
{
  QGridLayout* l = (QGridLayout *) layout();
  
  m_comboBox = new QComboBox(this);
  m_comboBox->addItem(i18n("true"));
  m_comboBox->addItem(i18n("false"));
  m_comboBox->setCurrentIndex(item->value() ? 0 : 1);
  l->addWidget(m_comboBox, 0, 2, Qt::AlignLeft);
  
  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(valueChanged()));
  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(activateItem()));
  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setItemValue(int)));
}

void VariableBoolEditor::setItemValue(int enabled)
{
  static_cast<VariableBoolItem*>(item())->setValue(enabled == 0);
}
//END VariableUintEditor




//BEGIN VariableStringListEditor
VariableStringListEditor::VariableStringListEditor(VariableStringListItem* item, QWidget* parent)
  : VariableEditor(item, parent)
{
  QGridLayout* l = (QGridLayout *) layout();
  
  m_comboBox = new QComboBox(this);
  m_comboBox->addItems(item->stringList());
  int index = 0;
  for (int i = 0; i < item->stringList().size(); ++i) {
    if (item->stringList().at(i) == item->value()) {
      index = i;
      break;
    }
  }
  m_comboBox->setCurrentIndex(index);
  l->addWidget(m_comboBox, 0, 2, Qt::AlignLeft);
  
  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(valueChanged()));
  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(activateItem()));
  connect(m_comboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setItemValue(const QString&)));
}

void VariableStringListEditor::setItemValue(const QString& newValue)
{
  static_cast<VariableStringListItem*>(item())->setValue(newValue);
}
//END VariableStringListEditor

// kate: indent-width 2; replace-tabs on;
