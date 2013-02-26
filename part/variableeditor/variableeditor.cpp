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
#include "katehelpbutton.h"

#include <QtCore/QVariant>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPainter>
#include <QtGui/QSpinBox>

#include <kfontcombobox.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kcolorcombo.h>
#include <sonnet/dictionarycombobox.h>

//BEGIN VariableEditor
VariableEditor::VariableEditor(VariableItem* item, QWidget* parent)
  : QWidget(parent)
  , m_item(item)
{
  setAttribute(Qt::WA_Hover);

  setAutoFillBackground(true);
  QGridLayout* l = new QGridLayout(this);
  l->setMargin(10);

  m_checkBox = new QCheckBox(this);
  m_variable = new QLabel(item->variable(), this);
  m_variable->setFocusPolicy(Qt::ClickFocus);
  m_variable->setFocusProxy(m_checkBox);
  m_btnHelp = new KateHelpButton(this);
  m_btnHelp->setIconState(KateHelpButton::IconHidden);
  m_btnHelp->setEnabled(false);
  m_btnHelp->setSection(QLatin1String("variable-") + item->variable());

  m_helpText = new QLabel(item->helpText(), this);
  m_helpText->setWordWrap(true);

  l->addWidget(m_checkBox, 0, 0, Qt::AlignLeft);
  l->addWidget(m_variable, 0, 1, Qt::AlignLeft);
  l->addWidget(m_btnHelp, 0, 3, Qt::AlignRight);
  l->addWidget(m_helpText, 1, 1, 1, 3);

  l->setColumnStretch(0, 0);
  l->setColumnStretch(1, 1);
  l->setColumnStretch(2, 1);
  l->setColumnStretch(3, 0);

  setLayout(l);

  connect(m_checkBox, SIGNAL(toggled(bool)), this, SLOT(itemEnabled(bool)));
  m_checkBox->setChecked(item->isActive());

  connect(m_checkBox, SIGNAL(toggled(bool)), this, SIGNAL(valueChanged()));
  setMouseTracking(true);
}

VariableEditor::~VariableEditor()
{
}

void VariableEditor::enterEvent(QEvent* event)
{
  QWidget::enterEvent(event);
  m_btnHelp->setIconState(KateHelpButton::IconColored);
  m_btnHelp->setEnabled(true);

  update();
}

void VariableEditor::leaveEvent(QEvent* event)
{
  QWidget::leaveEvent(event);
  m_btnHelp->setIconState(KateHelpButton::IconHidden);
  m_btnHelp->setEnabled(false);

  update();
}

void VariableEditor::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);

  // draw highlighting rect like in plasma
  if (underMouse()) {
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    QColor cornerColor = palette().color(QPalette::Highlight);
    cornerColor.setAlphaF(0.2);

    QColor midColor = palette().color(QPalette::Highlight);
    midColor.setAlphaF(0.5);

    QRect highlightRect = rect().adjusted(2, 2, -2, -2);

    QPen outlinePen;
    outlinePen.setWidth(2);

    QLinearGradient gradient(highlightRect.topLeft(), highlightRect.topRight());
    gradient.setColorAt(0, cornerColor);
    gradient.setColorAt(0.3, midColor);
    gradient.setColorAt(1, cornerColor);
    outlinePen.setBrush(gradient);
    painter.setPen(outlinePen);

    const int radius = 5;
    painter.drawRoundedRect(highlightRect, radius, radius);
  }
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
VariableIntEditor::VariableIntEditor(VariableIntItem* item, QWidget* parent)
  : VariableEditor(item, parent)
{
  QGridLayout* l = (QGridLayout *) layout();

  m_spinBox = new QSpinBox(this);
  m_spinBox->setValue(item->value());
  m_spinBox->setMinimum(item->minValue());
  m_spinBox->setMaximum(item->maxValue());

  l->addWidget(m_spinBox, 0, 2, Qt::AlignLeft);

  connect(m_spinBox, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged()));
  connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(activateItem()));
  connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(setItemValue(int)));
}

void VariableIntEditor::setItemValue(int newValue)
{
  static_cast<VariableIntItem*>(item())->setValue(newValue);
}
//END VariableUintEditor



//BEGIN VariableBoolEditor
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
//END VariableBoolEditor




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
  connect(m_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setItemValue(QString)));
}

void VariableStringListEditor::setItemValue(const QString& newValue)
{
  static_cast<VariableStringListItem*>(item())->setValue(newValue);
}
//END VariableStringListEditor



//BEGIN VariableColorEditor
VariableColorEditor::VariableColorEditor(VariableColorItem* item, QWidget* parent)
  : VariableEditor(item, parent)
{
  QGridLayout* l = (QGridLayout *) layout();

  m_comboBox = new KColorCombo(this);
  m_comboBox->setColor(item->value());
  l->addWidget(m_comboBox, 0, 2, Qt::AlignLeft);

  connect(m_comboBox, SIGNAL(activated(QColor)), this, SIGNAL(valueChanged()));
  connect(m_comboBox, SIGNAL(activated(QColor)), this, SLOT(activateItem()));
  connect(m_comboBox, SIGNAL(activated(QColor)), this, SLOT(setItemValue(QColor)));
}

void VariableColorEditor::setItemValue(const QColor& newValue)
{
  static_cast<VariableColorItem*>(item())->setValue(newValue);
}
//END VariableColorEditor



//BEGIN VariableFontEditor
VariableFontEditor::VariableFontEditor(VariableFontItem* item, QWidget* parent)
  : VariableEditor(item, parent)
{
  QGridLayout* l = (QGridLayout *) layout();

  m_comboBox = new KFontComboBox(this);
  m_comboBox->setCurrentFont(item->value());
  l->addWidget(m_comboBox, 0, 2, Qt::AlignLeft);

  connect(m_comboBox, SIGNAL(currentFontChanged(QFont)), this, SIGNAL(valueChanged()));
  connect(m_comboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(activateItem()));
  connect(m_comboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setItemValue(QFont)));
}

void VariableFontEditor::setItemValue(const QFont& newValue)
{
  static_cast<VariableFontItem*>(item())->setValue(newValue);
}
//END VariableFontEditor



//BEGIN VariableStringEditor
VariableStringEditor::VariableStringEditor(VariableStringItem *item, QWidget *parent)
  :VariableEditor(item, parent)
{
  QGridLayout *l = (QGridLayout*) layout();

  m_lineEdit = new QLineEdit(this);
  m_lineEdit->setText(item->value());
  l->addWidget(m_lineEdit, 0, 2, Qt::AlignLeft);

  connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(valueChanged()));
  connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(activateItem()));
  connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(setItemValue(QString)));
}

void VariableStringEditor::setItemValue(const QString &newValue)
{
  static_cast <VariableStringItem*>(item())->setValue(newValue);
}
//END VariableStringEditor



//BEGIN VariableSpellCheckEditor
VariableSpellCheckEditor::VariableSpellCheckEditor(VariableSpellCheckItem *item, QWidget *parent)
  : VariableEditor(item, parent)
{
  QGridLayout *l = (QGridLayout*) layout();

  m_dictionaryCombo = new Sonnet::DictionaryComboBox(this);
  m_dictionaryCombo->setCurrentByDictionary(item->value());
  l->addWidget(m_dictionaryCombo, 0, 2, Qt::AlignLeft);

  connect(m_dictionaryCombo, SIGNAL(dictionaryNameChanged(QString)), this, SIGNAL(valueChanged()));
  connect(m_dictionaryCombo, SIGNAL(dictionaryNameChanged(QString)), this, SLOT(activateItem()));
  connect(m_dictionaryCombo, SIGNAL(dictionaryChanged(QString)), this, SLOT(setItemValue(QString)));
}

void VariableSpellCheckEditor::setItemValue(const QString &newValue)
{
  static_cast <VariableSpellCheckItem*>(item())->setValue(newValue);
}

//END VariableSpellCheckEditor



//BEGIN VariableRemoveSpacesEditor
VariableRemoveSpacesEditor::VariableRemoveSpacesEditor(VariableRemoveSpacesItem* item, QWidget* parent)
  : VariableEditor(item, parent)
{
  QGridLayout* l = (QGridLayout *) layout();

  m_comboBox = new QComboBox(this);
  m_comboBox->addItem(i18nc("value for variable remove-trailing-spaces", "none"));
  m_comboBox->addItem(i18nc("value for variable remove-trailing-spaces", "modified"));
  m_comboBox->addItem(i18nc("value for variale remove-trailing-spaces", "all"));
  m_comboBox->setCurrentIndex(item->value());
  l->addWidget(m_comboBox, 0, 2, Qt::AlignLeft);

  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(valueChanged()));
  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(activateItem()));
  connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setItemValue(int)));
}

void VariableRemoveSpacesEditor::setItemValue(int enabled)
{
  static_cast<VariableRemoveSpacesItem*>(item())->setValue(enabled == 0);
}
//END VariableRemoveSpacesEditor

// kate: indent-width 2; replace-tabs on;
