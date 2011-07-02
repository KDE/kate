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

#include "testwidget.h"
#include "variableitem.h"
#include "variablelistview.h"

#include <QtCore/QDebug>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QSizeGrip>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>

TestWidget::TestWidget(QWidget* parent)
  : QWidget(parent)
{
  QHBoxLayout* hl = new QHBoxLayout();
  hl->setMargin(0);
  hl->setSpacing(0);

  QVBoxLayout* vl = new QVBoxLayout(this);
  vl->setMargin(30);
  vl->setSpacing(6);
  setLayout(vl);


  m_lineedit = new QLineEdit(this);
  m_button= new QPushButton("Edit...", this);

  QComboBox* test = new QComboBox(this);
  test->addItem("Eintrag 1");
  test->addItem("Eintrag 2");
  test->addItem("Eintrag 3");
  test->addItem("Eintrag 4");
  test->addItem("Eintrag 5");

  hl->addWidget(m_lineedit);
  hl->addWidget(m_button);

  vl->addLayout(hl);
  vl->addWidget(test);
  vl->addStretch();


  m_popup = new QFrame(0, Qt::Popup);
  m_popup->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  QVBoxLayout* l = new QVBoxLayout(m_popup);
  l->setSpacing(0);
  l->setMargin(0);
  m_popup->setLayout(l);

  connect(m_button, SIGNAL(clicked()), this, SLOT(editVariables()));
}

TestWidget::~TestWidget()
{
}

void TestWidget::editVariables()
{
  static VariableListView* listview = 0;
  if (listview != 0) {
    m_popup->layout()->removeWidget(listview);
    delete listview;
  }
  listview = new VariableListView(m_lineedit->text(), m_popup);
  addKateItems(listview);
  connect(listview, SIGNAL(editingDone(const QString&)), m_lineedit, SLOT(setText(const QString&)));
  connect(listview, SIGNAL(changed()), this, SLOT(somethingChanged()));

  m_popup->layout()->addWidget(listview);

  QPoint topLeft = mapToGlobal(m_lineedit->geometry().bottomLeft());
  const int w = m_button->geometry().right() - m_lineedit->geometry().left();
  const int h = 300; //(w * 2) / 4;
  m_popup->setGeometry(QRect(topLeft, QSize(w, h)));
  m_popup->show();
}

void TestWidget::addKateItems(VariableListView* listview)
{
  VariableItem* item = 0;

  item = new VariableBoolItem("auto-brackets", false);
  item->setHelpText("Set auto insertion of brackets on or off.");
  listview->addItem(item);
  
  item = new VariableUintItem("auto-center-lines", 0);
  item->setHelpText("Set the number of autocenter lines.");
  listview->addItem(item);

  item = new VariableBoolItem("auto-insert-doxygen", false);
  item->setHelpText("Auto insert asterisk in doxygen comments.");
  listview->addItem(item);

  item = new VariableColorItem("background-color", Qt::white);
  item->setHelpText("Set the document background color.");
  listview->addItem(item);

  item = new VariableBoolItem("backspace-indents", false);
  item->setHelpText("Pressing backspace in leading whitespace unindents.");
  listview->addItem(item);

  item = new VariableBoolItem("block-selection", false);
  item->setHelpText("Enable block selection mode.");
  listview->addItem(item);

  item = new VariableBoolItem("byte-order-marker", false);
  item->setHelpText("Enable the byte order marker when saving unicode files.");
  listview->addItem(item);
  
  item = new VariableColorItem("bracket-highlight-color", Qt::yellow);
  item->setHelpText("Set the color for the bracket highlight.");
  listview->addItem(item);

  item = new VariableColorItem("current-line-color", Qt::magenta);
  item->setHelpText("Set the background color for the current line.");
  listview->addItem(item);

  item = new VariableBoolItem("default-dictionary TODO", false);
  item->setHelpText("Set the default dictionary used for spell checking.");
  listview->addItem(item);

  item = new VariableBoolItem("dynamic-word-wrap", false);
  item->setHelpText("Enable dynamic word wrap of long lines.");
  listview->addItem(item);
 
  item = new VariableStringListItem("end-of-line", QStringList() << "unix" << "mac" << "dos", "unix");
  item->setHelpText("Sets the end of line mode.");
  listview->addItem(item);
  
  item = new VariableBoolItem("folding-markers", false);
  item->setHelpText("Enable folding markers in the editor border.");
  listview->addItem(item);
  
  item = new VariableUintItem("font-size", 12);
  item->setHelpText("Set the point size of the document font.");
  listview->addItem(item);
  
  item = new VariableFontItem("font", QFont());
  item->setHelpText("Set the font of the document.");
  listview->addItem(item);
  
  item = new VariableStringListItem("syntax TODO", QStringList() << "C++", "C++");
  item->setHelpText("Set the syntax highlighting.");
  listview->addItem(item);
  
  item = new VariableColorItem("icon-bar-color", Qt::gray);
  item->setHelpText("Set the icon bar color.");
  listview->addItem(item);

  item = new VariableBoolItem("icon-border", true);
  item->setHelpText("Enable the icon border in the editor view.");
  listview->addItem(item);

  item = new VariableStringListItem("indent-mode TODO", QStringList() << "cstyle" << "lisp" << "python" << "none", "none");
  item->setHelpText("Set the auto indentation style.");
  listview->addItem(item);
  
  item = new VariableUintItem("indent-width", 4);
  item->setHelpText("Set the indentation depth for each indent level.");
  listview->addItem(item);

  item = new VariableBoolItem("keep-extra-spaces", false);
  item->setHelpText("Allow odd indentation level (no multiple of indent width).");
  listview->addItem(item);
  
  item = new VariableBoolItem("line-numbers", false);
  item->setHelpText("Show line numbers.");
  listview->addItem(item);

  item = new VariableBoolItem("overwrite-mode", false);
  item->setHelpText("Enable overwrite mode in the document.");
  listview->addItem(item);

  item = new VariableBoolItem("persistent-selection", false);
  item->setHelpText("Enable persistent text selection.");
  listview->addItem(item);
  
  item = new VariableBoolItem("remove-trailing-space", false);
  item->setHelpText("Remove trailing spaces when editing a line.");
  listview->addItem(item);
  
  item = new VariableBoolItem("replace-tabs-save", false);
  item->setHelpText("Replace tabs with spaces when saving the document.");
  listview->addItem(item);
  
  item = new VariableBoolItem("replace-tabs", true);
  item->setHelpText("Replace tabs with spaces.");
  listview->addItem(item);

  item = new VariableBoolItem("replace-trailing-space-save", true);
  item->setHelpText("Remove trailing spaces when saving the document.");
  listview->addItem(item);
  
  item = new VariableBoolItem("scheme TODO", false);
  item->setHelpText("Set the color scheme.");
  listview->addItem(item);
  
  item = new VariableColorItem("selection-color", Qt::blue);
  item->setHelpText("Set the text selection color.");
  listview->addItem(item);
  
  item = new VariableBoolItem("show-tabs", false);
  item->setHelpText("Visualize tabs and trailing spaces.");
  listview->addItem(item);
  
  item = new VariableBoolItem("smart-home", false);
  item->setHelpText("Enable smart home navigation.");
  listview->addItem(item);
  
  item = new VariableBoolItem("tab-indents", false);
  item->setHelpText("Pressing TAB key indents.");
  listview->addItem(item);

  item = new VariableUintItem("tab-width", 8);
  item->setHelpText("Set the tab display width.");
  listview->addItem(item);
  
  item = new VariableUintItem("undo-steps", 0);
  item->setHelpText("Set the number of undo steps to remember (0 equals infinity).");
  listview->addItem(item);
  
  item = new VariableUintItem("word-wrap-column", 78);
  item->setHelpText("Set the word wrap column.");
  listview->addItem(item);
  
  item = new VariableColorItem("word-wrap-marker-color", Qt::black);
  item->setHelpText("Set the work wrap marker color.");
  listview->addItem(item);
  
  item = new VariableBoolItem("word-wrap", false);
  item->setHelpText("Enable word wrap while typing text.");
  listview->addItem(item);
  
  item = new VariableBoolItem("wrap-cursor", true);
  item->setHelpText("Wrap the text cursor at the end of a line.");
  listview->addItem(item);
}

void TestWidget::somethingChanged()
{
  qDebug() << "list view changed";
}
// kate: indent-width 2; replace-tabs on;
