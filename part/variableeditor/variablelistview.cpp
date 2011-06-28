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
#include "variablelistview.h"
#include "variableitem.h"

#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

class ContainerProxyWidget : public QWidget
{
public:
  ContainerProxyWidget(VariableListView* parent = 0)
    : QWidget(parent), m_listView(parent)
  {
  }

  virtual QSize sizeHint() const
  {
    int h = 0;
    foreach (QWidget* w, m_listView->m_editors) {
      h += w->sizeHint().height();
    }

    return QSize(-1, h); // the width is not used
  }
  
private:
  VariableListView* m_listView;
};

VariableListView::VariableListView(QWidget* parent)
  : QScrollArea(parent)
{
  setBackgroundRole(QPalette::Base);

  QWidget* top = new QWidget(this);
  setWidget(top);
  
  addKateItems();
}

VariableListView::~VariableListView()
{
}

void VariableListView::addKateItems()
{
  QWidget* top = widget();
  VariableItem* item = 0;

  item = new VariableBoolItem("auto-brackets", false);
  item->setHelpText("Set auto insertion of brackets on or off.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableUintItem("auto-center-lines", 0);
  item->setHelpText("Set the number of autocenter lines.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("auto-insert-doxygen", false);
  item->setHelpText("Auto insert asterisk in doxygen comments.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("background-color TODO", false);
  item->setHelpText("Set the document background color.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("backspace-indents", false);
  item->setHelpText("Pressing backspace in leading whitespace unindents.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("block-selection", false);
  item->setHelpText("Enable block selection mode.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("byte-order-marker", false);
  item->setHelpText("Enable the byte order marker when saving unicode files.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("bracket-highlight-color TODO", false);
  item->setHelpText("Set the color for the bracket highlight.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("current-line-color TODO", false);
  item->setHelpText("Set the background color for the current line.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("default-dictionary TODO", false);
  item->setHelpText("Set the default dictionary used for spell checking.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("dynamic-word-wrap", false);
  item->setHelpText("Enable dynamic word wrap of long lines.");
  m_editors << item->createEditor(top);
  m_items << item;
 
  item = new VariableStringListItem("end-of-line", QStringList() << "unix" << "mac" << "dos", "unix");
  item->setHelpText("Sets the end of line mode.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("folding-markers", false);
  item->setHelpText("Enable folding markers in the editor border.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableUintItem("font-size", 12);
  item->setHelpText("Set the point size of the document font.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableUintItem("font TODO", 12);
  item->setHelpText("Set the font of the document.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableStringListItem("syntax TODO", QStringList() << "C++", "C++");
  item->setHelpText("Set the syntax highlighting.");
  m_editors << item->createEditor(top);
  m_items << item;  
  
  item = new VariableBoolItem("icon-bar-color TODO", false);
  item->setHelpText("Set the icon bar color.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("icon-border", true);
  item->setHelpText("Enable the icon border in the editor view.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableStringListItem("indent-mode TODO", QStringList() << "cstyle" << "lisp" << "python" << "none", "none");
  item->setHelpText("Set the auto indentation style.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableUintItem("indent-width", 4);
  item->setHelpText("Set the indentation depth for each indent level.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("keep-extra-spaces", false);
  item->setHelpText("Allow odd indentation level (no multiple of indent width).");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("keep-indent-profile: gibts das noch?", false);
  item->setHelpText("Prevents unindenting a block if at least one line has no indentation.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("line-numbers", false);
  item->setHelpText("Show line numbers.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("overwrite-mode", false);
  item->setHelpText("Enable overwrite mode in the document.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("persistent-selection", false);
  item->setHelpText("Enable persistent text selection.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("remove-trailing-space", false);
  item->setHelpText("Remove trailing spaces when editing a line.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("replace-tabs-save", false);
  item->setHelpText("Replace tabs with spaces when saving the document.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("replace-tabs", true);
  item->setHelpText("Replace tabs with spaces.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableBoolItem("replace-trailing-space-save", true);
  item->setHelpText("Remove trailing spaces when saving the document.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("scheme TODO", false);
  item->setHelpText("Set the color scheme.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("selection-color TODO", false);
  item->setHelpText("Set the text selection color.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("show-tabs", false);
  item->setHelpText("Visualize tabs and trailing spaces.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("smart-home", false);
  item->setHelpText("Enable smart home navigation.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("tab-indents", false);
  item->setHelpText("Pressing TAB key indents.");
  m_editors << item->createEditor(top);
  m_items << item;

  item = new VariableUintItem("tab-width", 8);
  item->setHelpText("Set the tab display width.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableUintItem("undo-steps", 0);
  item->setHelpText("Set the number of undo steps to remember (0 equals infinity).");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableUintItem("word-wrap-column", 78);
  item->setHelpText("Set the word wrap column.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableUintItem("word-wrap-marker-color TODO", 78);
  item->setHelpText("Set the work wrap marker color.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("word-wrap", false);
  item->setHelpText("Enable word wrap while typing text.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  item = new VariableBoolItem("wrap-cursor", true);
  item->setHelpText("Wrap the text cursor at the end of a line.");
  m_editors << item->createEditor(top);
  m_items << item;
  
  for (int i = 0; i < m_editors.size(); ++i) {
    m_editors[i]->setBackgroundRole((i % 2) ? QPalette::AlternateBase : QPalette::Base);
    m_editors[i]->show();

    connect(m_editors[i], SIGNAL(valueChanged()), this, SLOT(somethingChanged()));
  }
}

QVector<VariableItem *> VariableListView::items()
{
  return m_items;
}

void VariableListView::resizeEvent(QResizeEvent* event)
{
  QScrollArea::resizeEvent(event);
  
  // calculate sum of all editor heights
  int listHeight = 0;
  foreach (QWidget* w, m_editors) {
    listHeight += w->sizeHint().height();
  }

  // resize scroll area widget
  QWidget* top = widget();
  top->resize(event->size().width(), listHeight);

  // set client geometries correctly
  int h = 0;
  foreach (QWidget* w, m_editors) {
    w->setGeometry(0, h, top->width(), w->sizeHint().height());
    h += w->sizeHint().height();
  }
}

void VariableListView::somethingChanged()
{
  qDebug() << "CHANGED";
}

// kate: indent-width 2; replace-tabs on;
