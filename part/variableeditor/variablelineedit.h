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

#ifndef VARIABLE_LINE_EDIT_H
#define VARIABLE_LINE_EDIT_H

#include <QWidget>

class QFrame;
class QLineEdit;
class QToolButton;
class VariableListView;

class VariableLineEdit : public QWidget
{
  Q_OBJECT

public:
  VariableLineEdit(QWidget* parent = 0);
  virtual ~VariableLineEdit();

  void addKateItems(VariableListView* listview);
  QString text();

public Q_SLOTS:
  void editVariables();
  void setText(const QString &text);
  void clear();
  void updateVariableLine();

Q_SIGNALS:
  void textChanged(const QString&);

private:
  QFrame* m_popup;
  QLineEdit* m_lineedit;
  QToolButton* m_button;
  VariableListView* m_listview;
};

#endif

// kate: indent-width 2; replace-tabs on;
