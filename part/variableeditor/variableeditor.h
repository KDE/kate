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

#ifndef VARIABLE_EDITOR_H
#define VARIABLE_EDITOR_H

#include <QtGui/QWidget>

class VariableItem;
class VariableUintItem;
class VariableBoolItem;
class VariableStringListItem;
class QLabel;
class QSpinBox;
class QCheckBox;
class QComboBox;

class VariableEditor : public QWidget
{
  Q_OBJECT

public:
  VariableEditor(VariableItem* item, QWidget* parent = 0);
  virtual ~VariableEditor();

  VariableItem* item() const;
  
Q_SIGNALS:
  void valueChanged();

protected Q_SLOTS:
  void itemEnabled(bool enabled);
  void activateItem();

private:
  VariableItem* m_item;
  
  QCheckBox* m_checkBox;
  QLabel* m_variable;
  QLabel* m_helpText;
  QLabel* m_pixmap;
};

class VariableUintEditor : public VariableEditor
{
  Q_OBJECT
public:
  VariableUintEditor(VariableUintItem* item, QWidget* parent);  
  
protected Q_SLOTS:
  void setItemValue(int newValue);

private:
  QSpinBox* m_spinBox;
};

class VariableBoolEditor : public VariableEditor
{
  Q_OBJECT
public:
  VariableBoolEditor(VariableBoolItem* item, QWidget* parent);  

protected Q_SLOTS:
  void setItemValue(int enabled);

private:
  QComboBox* m_comboBox;
};

class VariableStringListEditor : public VariableEditor
{
  Q_OBJECT
public:
  VariableStringListEditor(VariableStringListItem* item, QWidget* parent);  

protected Q_SLOTS:
  void setItemValue(const QString& newValue);

private:
  QComboBox* m_comboBox;
};

#endif

// kate: indent-width 2; replace-tabs on;
