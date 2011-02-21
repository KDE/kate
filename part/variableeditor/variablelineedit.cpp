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

#include "variablelineedit.h"

#include "variableitem.h"
#include "variablelistview.h"
#include "kateautoindent.h"
#include "katesyntaxmanager.h"
#include "kateschema.h"

#include <QtCore/QDebug>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QSizeGrip>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

#include <kdialog.h>
#include <klocale.h>

VariableLineEdit::VariableLineEdit(QWidget* parent)
  : QWidget(parent)
{
  m_listview = 0;

  QHBoxLayout* hl = new QHBoxLayout();
  hl->setMargin(0);
  hl->setSpacing(KDialog::spacingHint());
  setLayout(hl);

  m_lineedit = new QLineEdit(this);
  m_button= new QToolButton(this);
  m_button->setIcon(KIcon("tools-wizard"));
  m_button->setToolTip(i18n("Show list of valid variables."));

  hl->addWidget(m_lineedit);
  hl->addWidget(m_button);

  m_popup = new QFrame(0, Qt::Popup);
  m_popup->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  QVBoxLayout* l = new QVBoxLayout(m_popup);
  l->setSpacing(0);
  l->setMargin(0);
  m_popup->setLayout(l);

  // forward text changed signal
  connect(m_lineedit, SIGNAL(textChanged(const QString&)), this, SIGNAL(textChanged(const QString&)));
  
  // open popup on button click
  connect(m_button, SIGNAL(clicked()), this, SLOT(editVariables()));
}

VariableLineEdit::~VariableLineEdit()
{
}

void VariableLineEdit::editVariables()
{
  m_listview = new VariableListView(m_lineedit->text(), m_popup);
  addKateItems(m_listview);
  connect(m_listview, SIGNAL(aboutToHide()), this, SLOT(updateVariableLine()));

  m_popup->layout()->addWidget(m_listview);

  QPoint topLeft = mapToGlobal(m_lineedit->geometry().bottomLeft());
  const int w = m_button->geometry().right() - m_lineedit->geometry().left();
  const int h = 300; //(w * 2) / 4;
  m_popup->setGeometry(QRect(topLeft, QSize(w, h)));
  m_popup->show();
}

void VariableLineEdit::updateVariableLine()
{
  QString variables = m_listview->variableLine();
  m_lineedit->setText(variables);

  m_popup->layout()->removeWidget(m_listview);
  m_listview->deleteLater();
  m_listview = 0;
}

void VariableLineEdit::addKateItems(VariableListView* listview)
{
  VariableItem* item = 0;

  item = new VariableBoolItem("auto-brackets", false);
  item->setHelpText(i18nc("short translation please", "Set auto insertion of brackets on or off."));
  listview->addItem(item);
  
  item = new VariableIntItem("auto-center-lines", 0);
  ((VariableIntItem*)item)->setRange(1, 100);
  item->setHelpText(i18nc("short translation please", "Set the number of autocenter lines."));
  listview->addItem(item);

  item = new VariableBoolItem("auto-insert-doxygen", false);
  item->setHelpText(i18nc("short translation please", "Auto insert asterisk in doxygen comments."));
  listview->addItem(item);

  item = new VariableColorItem("background-color", Qt::white);
  item->setHelpText(i18nc("short translation please", "Set the document background color."));
  listview->addItem(item);

  item = new VariableBoolItem("backspace-indents", false);
  item->setHelpText(i18nc("short translation please", "Pressing backspace in leading whitespace unindents."));
  listview->addItem(item);

  item = new VariableBoolItem("block-selection", false);
  item->setHelpText(i18nc("short translation please", "Enable block selection mode."));
  listview->addItem(item);

  item = new VariableBoolItem("byte-order-marker", false);
  item->setHelpText(i18nc("short translation please", "Enable the byte order marker when saving unicode files."));
  listview->addItem(item);
  
  item = new VariableColorItem("bracket-highlight-color", Qt::yellow);
  item->setHelpText(i18nc("short translation please", "Set the color for the bracket highlight."));
  listview->addItem(item);

  item = new VariableColorItem("current-line-color", Qt::magenta);
  item->setHelpText(i18nc("short translation please", "Set the background color for the current line."));
  listview->addItem(item);

  item = new VariableStringItem("default-dictionary", "English(US)");
  item->setHelpText(i18nc("short translation please", "Set the default dictionary used for spell checking."));
  listview->addItem(item);

  item = new VariableBoolItem("dynamic-word-wrap", false);
  item->setHelpText(i18nc("short translation please", "Enable dynamic word wrap of long lines."));
  listview->addItem(item);
 
  item = new VariableStringListItem("end-of-line", QStringList() << "unix" << "mac" << "dos", "unix");
  item->setHelpText(i18nc("short translation please", "Sets the end of line mode."));
  listview->addItem(item);
  
  item = new VariableBoolItem("folding-markers", false);
  item->setHelpText(i18nc("short translation please", "Enable folding markers in the editor border."));
  listview->addItem(item);
  
  item = new VariableIntItem("font-size", 12);
  ((VariableIntItem*)item)->setRange(4, 128);
  item->setHelpText(i18nc("short translation please", "Set the point size of the document font."));
  listview->addItem(item);
  
  item = new VariableFontItem("font", QFont());
  item->setHelpText(i18nc("short translation please", "Set the font of the document."));
  listview->addItem(item);
  
  /* Prepare list of highlighting modes */
  int count = KateHlManager::self()->highlights();
  QStringList hl;
  for (int z=0;z<count;z++)
    hl<<KateHlManager::self()->hlNameTranslated (z);
  
  item = new VariableStringListItem("syntax", hl, hl.at(0));
  item->setHelpText(i18nc("short translation please", "Set the syntax highlighting."));
  listview->addItem(item);
  
  item = new VariableColorItem("icon-bar-color", Qt::gray);
  item->setHelpText(i18nc("short translation please", "Set the icon bar color."));
  listview->addItem(item);

  item = new VariableBoolItem("icon-border", true);
  item->setHelpText(i18nc("short translation please", "Enable the icon border in the editor view."));
  listview->addItem(item);

  item = new VariableStringListItem("indent-mode", KateAutoIndent::listModes(),"none");
  item->setHelpText(i18nc("short translation please", "Set the auto indentation style."));
  listview->addItem(item);
  
  item = new VariableIntItem("indent-width", 4);
  ((VariableIntItem*)item)->setRange(1, 16);
  item->setHelpText(i18nc("short translation please", "Set the indentation depth for each indent level."));
  listview->addItem(item);

  item = new VariableBoolItem("keep-extra-spaces", false);
  item->setHelpText(i18nc("short translation please", "Allow odd indentation level (no multiple of indent width)."));
  listview->addItem(item);
  
  item = new VariableBoolItem("line-numbers", false);
  item->setHelpText(i18nc("short translation please", "Show line numbers."));
  listview->addItem(item);

  item = new VariableBoolItem("overwrite-mode", false);
  item->setHelpText(i18nc("short translation please", "Enable overwrite mode in the document."));
  listview->addItem(item);

  item = new VariableBoolItem("persistent-selection", false);
  item->setHelpText(i18nc("short translation please", "Enable persistent text selection."));
  listview->addItem(item);
  
  item = new VariableBoolItem("remove-trailing-space", false);
  item->setHelpText(i18nc("short translation please", "Remove trailing spaces when editing a line."));
  listview->addItem(item);
  
  item = new VariableBoolItem("replace-tabs-save", false);
  item->setHelpText(i18nc("short translation please", "Replace tabs with spaces when saving the document."));
  listview->addItem(item);
  
  item = new VariableBoolItem("replace-tabs", true);
  item->setHelpText(i18nc("short translation please", "Replace tabs with spaces."));
  listview->addItem(item);

  item = new VariableBoolItem("replace-trailing-space-save", true);
  item->setHelpText(i18nc("short translation please", "Remove trailing spaces when saving the document."));
  listview->addItem(item);
  
  KateSchemaManager *schemaManager = new KateSchemaManager();
  QStringList schemas = schemaManager->list();
  item = new VariableStringListItem("scheme", schemas, schemas.at(0));
  item->setHelpText(i18nc("short translation please", "Set the color scheme."));
  listview->addItem(item);
  
  item = new VariableColorItem("selection-color", Qt::blue);
  item->setHelpText(i18nc("short translation please", "Set the text selection color."));
  listview->addItem(item);
  
  item = new VariableBoolItem("show-tabs", false);
  item->setHelpText(i18nc("short translation please", "Visualize tabs and trailing spaces."));
  listview->addItem(item);
  
  item = new VariableBoolItem("smart-home", false);
  item->setHelpText(i18nc("short translation please", "Enable smart home navigation."));
  listview->addItem(item);
  
  item = new VariableBoolItem("tab-indents", false);
  item->setHelpText(i18nc("short translation please", "Pressing TAB key indents."));
  listview->addItem(item);

  item = new VariableIntItem("tab-width", 8);
  ((VariableIntItem*)item)->setRange(1, 16);
  item->setHelpText(i18nc("short translation please", "Set the tab display width."));
  listview->addItem(item);
  
  item = new VariableIntItem("undo-steps", 0);
  ((VariableIntItem*)item)->setRange(0, 100);
  item->setHelpText(i18nc("short translation please", "Set the number of undo steps to remember (0 equals infinity)."));
  listview->addItem(item);
  
  item = new VariableIntItem("word-wrap-column", 78);
  ((VariableIntItem*)item)->setRange(20, 200);
  item->setHelpText(i18nc("short translation please", "Set the word wrap column."));
  listview->addItem(item);
  
  item = new VariableColorItem("word-wrap-marker-color", Qt::black);
  item->setHelpText(i18nc("short translation please", "Set the work wrap marker color."));
  listview->addItem(item);
  
  item = new VariableBoolItem("word-wrap", false);
  item->setHelpText(i18nc("short translation please", "Enable word wrap while typing text."));
  listview->addItem(item);
  
  item = new VariableBoolItem("wrap-cursor", true);
  item->setHelpText(i18nc("short translation please", "Wrap the text cursor at the end of a line."));
  listview->addItem(item);
}

void VariableLineEdit::setText(const QString &text)
{
  m_lineedit->setText(text);
}

void VariableLineEdit::clear()
{
  m_lineedit->clear();
}

QString VariableLineEdit::text()
{
  return m_lineedit->text();
}

// kate: indent-width 2; replace-tabs on;
