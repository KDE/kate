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

#include "variableitem.h"
#include "variableeditor.h"


//BEGIN class VariableItem
VariableItem::VariableItem(const QString& variable)
  : m_variable(variable)
  , m_active(false)
{
}

VariableItem::~VariableItem()
{
}

QString VariableItem::variable() const
{
  return m_variable;
}

QString VariableItem::helpText() const
{
  return m_helpText;
}

void VariableItem::setHelpText(const QString& text)
{
  m_helpText = text;
}

bool VariableItem::isActive() const
{
  return m_active;
}

void VariableItem::setActive(bool active)
{
  m_active = active;
}
//END class VariableItem



//BEGIN class VariableUintItem
VariableUintItem::VariableUintItem(const QString& variable, int value)
  : VariableItem(variable)
  , m_value(value)
{
}

int VariableUintItem::value() const
{
  return m_value;
}

void VariableUintItem::setValue(int newValue)
{
  m_value = newValue;
}

void VariableUintItem::setValueByString(const QString& value)
{
  setValue(value.toInt());
}

QString VariableUintItem::valueAsString() const
{
  return QString::number(value());
}

VariableEditor* VariableUintItem::createEditor(QWidget* parent)
{
  return new VariableUintEditor(this, parent);
}
//END class VariableUintItem








//BEGIN class VariableStringListItem
VariableStringListItem::VariableStringListItem(const QString& variable, const QStringList& slist, const QString& value)
  : VariableItem(variable)
  , m_list(slist)
  , m_value(value)
{
}

QStringList VariableStringListItem::stringList() const
{
  return m_list;
}

QString VariableStringListItem::value() const
{
  return m_value;
}

void VariableStringListItem::setValue(const QString& newValue)
{
  m_value = newValue;
}

void VariableStringListItem::setValueByString(const QString& value)
{
  setValue(value);
}

QString VariableStringListItem::valueAsString() const
{
  return value();
}

VariableEditor* VariableStringListItem::createEditor(QWidget* parent)
{
  return new VariableStringListEditor(this, parent);
}
//END class VariableStringListItem



//BEGIN class VariableBoolItem
VariableBoolItem::VariableBoolItem(const QString& variable, bool value)
  : VariableItem(variable)
  , m_value(value)
{
}

bool VariableBoolItem::value() const
{
  return m_value;
}

void VariableBoolItem::setValue(bool enabled)
{
  m_value = enabled;
}

void VariableBoolItem::setValueByString(const QString& value)
{
  QString tmp = value.trimmed().toLower();
  bool enabled = (tmp == "on") || (tmp == "1") || (tmp == "true");
  setValue(enabled);
}

QString VariableBoolItem::valueAsString() const
{
  return value() ? QString("true") : QString("false");
}

VariableEditor* VariableBoolItem::createEditor(QWidget* parent)
{
  return new VariableBoolEditor(this, parent);
}
//END class VariableBoolItem



//BEGIN class VariableColorItem
VariableColorItem::VariableColorItem(const QString& variable, const QColor& value)
  : VariableItem(variable)
  , m_value(value)
{
}

QColor VariableColorItem::value() const
{
  return m_value;
}

void VariableColorItem::setValue(const QColor& value)
{
  m_value = value;
}

void VariableColorItem::setValueByString(const QString& value)
{
  setValue(QColor(value));
}

QString VariableColorItem::valueAsString() const
{
  return value().name();
}

VariableEditor* VariableColorItem::createEditor(QWidget* parent)
{
  return new VariableColorEditor(this, parent);
}
//END class VariableColorItem



//BEGIN class VariableFontItem
VariableFontItem::VariableFontItem(const QString& variable, const QFont& value)
  : VariableItem(variable)
  , m_value(value)
{
}

QFont VariableFontItem::value() const
{
  return m_value;
}

void VariableFontItem::setValue(const QFont& value)
{
  m_value = value;
}

void VariableFontItem::setValueByString(const QString& value)
{
  setValue(QFont(value));
}

QString VariableFontItem::valueAsString() const
{
  return value().family();
}

VariableEditor* VariableFontItem::createEditor(QWidget* parent)
{
  return new VariableFontEditor(this, parent);
}
//END class VariableFontItem
// kate: indent-width 2; replace-tabs on;
