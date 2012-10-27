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

#ifndef VARIABLE_ITEM_H
#define VARIABLE_ITEM_H

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtCore/QString>
#include <QtCore/QStringList>

class VariableEditor;

//BEGIN class VariableItem
class VariableItem
{
public:
  VariableItem(const QString& variable);
  virtual ~VariableItem();

  QString variable() const;
  QString helpText() const;
  void setHelpText(const QString& text);

  bool isActive() const;
  void setActive(bool active);

  virtual void setValueByString(const QString& value) = 0;
  virtual QString valueAsString() const = 0;

  virtual VariableEditor* createEditor(QWidget* parent) = 0;

private:
  // not implemented: copy constructor and operator=
  VariableItem(const VariableItem& copy);
  VariableItem& operator=(const VariableItem& other);

  QString m_variable;
  QString m_helpText;
  bool m_active;
};
//END class VariableItem


//
// DERIVE A CLASS FOR EACH TYPE
//

//BEGIN class VariableIntItem
class VariableIntItem : public VariableItem
{
public:
  VariableIntItem(const QString& variable, int value);

  int value() const;
  void setValue(int newValue);
  void setRange(int minValue, int maxValue);
  int minValue() const;
  int maxValue() const;

public:
  virtual void setValueByString(const QString& value);
  virtual QString valueAsString() const;
  virtual VariableEditor* createEditor(QWidget* parent);

private:
  int m_value;
  int m_minValue;
  int m_maxValue;

};
//END class VariableIntItem


//BEGIN class VariableStringListItem
class VariableStringListItem : public VariableItem
{
public:
  VariableStringListItem(const QString& variable, const QStringList& slist, const QString& value);

  QStringList stringList() const;

  QString value() const;
  void setValue(const QString& newValue);

public:
  virtual void setValueByString(const QString& value);
  virtual QString valueAsString() const;
  virtual VariableEditor* createEditor(QWidget* parent);

private:
  QStringList m_list;
  QString m_value;
};
//END class VariableStringListItem



//BEGIN class VariableBoolItem
class VariableBoolItem : public VariableItem
{
public:
  VariableBoolItem(const QString& variable, bool value);

  bool value() const;
  void setValue(bool enabled);

public:
  virtual void setValueByString(const QString& value);
  virtual QString valueAsString() const;
  virtual VariableEditor* createEditor(QWidget* parent);

private:
  bool m_value;
};
//END class VariableBoolItem



//BEGIN class VariableColorItem
class VariableColorItem : public VariableItem
{
public:
  VariableColorItem(const QString& variable, const QColor& value);

  QColor value() const;
  void setValue(const QColor& color);

public:
  virtual void setValueByString(const QString& value);
  virtual QString valueAsString() const;
  virtual VariableEditor* createEditor(QWidget* parent);

private:
  QColor m_value;
};
//END class VariableColorItem



//BEGIN class VariableFontItem
class VariableFontItem : public VariableItem
{
public:
  VariableFontItem(const QString& variable, const QFont& value);

  QFont value() const;
  void setValue(const QFont& value);

public:
  virtual void setValueByString(const QString& value);
  virtual QString valueAsString() const;
  virtual VariableEditor* createEditor(QWidget* parent);

private:
  QFont m_value;
};
//END class VariableFontItem



//BEGIN class VariableStringItem
class VariableStringItem : public VariableItem
{
public:
  VariableStringItem(const QString &variable, const QString &value);

  QString value() const;
  void setValue(const QString& value);

public:
  virtual void setValueByString(const QString& value); // Same as setValue() in this case, implemented for uniformity
  virtual QString valueAsString() const;               // Same as value() in this case, implemented for uniformity
  virtual VariableEditor* createEditor(QWidget* parent);

private:
  QString m_value;
};

//END class VariableStringItem

//BEGIN class VariableSpellCheckItem
class VariableSpellCheckItem : public VariableItem
{
public:
  VariableSpellCheckItem(const QString &variable, const QString &value);

  QString value() const;
  void setValue(const QString& value);

public:
  virtual void setValueByString(const QString &value);  // Same as setValue() in this case, implemented for uniformity
  virtual QString valueAsString() const;                // Same as value() in this case, implemented for uniformity
  virtual VariableEditor* createEditor(QWidget *parent);

private:
  QString m_value;
};
//END class VariableSpellCheckItem

//BEGIN class VariableRemoveSpacesItem
class VariableRemoveSpacesItem : public VariableItem
{
public:
  VariableRemoveSpacesItem(const QString &variable, int value);

  int value() const;
  void setValue(int value);

public:
  virtual void setValueByString(const QString &value);  // Same as setValue() in this case, implemented for uniformity
  virtual QString valueAsString() const;                // Same as value() in this case, implemented for uniformity
  virtual VariableEditor* createEditor(QWidget *parent);

private:
  int m_value;
};
//END class VariableRemoveSpacesItem

#endif

// kate: indent-width 2; replace-tabs on;
