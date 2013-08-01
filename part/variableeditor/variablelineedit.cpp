/* This file is part of the KDE project

   Copyright (C) 2011 Dominik Haumann <dhaumann kde org>
   Copyright (C) 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>

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
#include "kateview.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katerenderer.h"

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
#include <sonnet/speller.h>

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
  connect(m_lineedit, SIGNAL(textChanged(QString)), this, SIGNAL(textChanged(QString)));

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

  if (layoutDirection() == Qt::LeftToRight) {
    QPoint topLeft = mapToGlobal(m_lineedit->geometry().bottomLeft());
    const int w = m_button->geometry().right() - m_lineedit->geometry().left();
    const int h = 300; //(w * 2) / 4;
    m_popup->setGeometry(QRect(topLeft, QSize(w, h)));
  } else {
    QPoint topLeft = mapToGlobal(m_button->geometry().bottomLeft());
    const int w = m_lineedit->geometry().right() - m_button->geometry().left();
    const int h = 300; //(w * 2) / 4;
    m_popup->setGeometry(QRect(topLeft, QSize(w, h)));
  }
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

  // If a current active doc is available
  KateView* activeView = 0;
  KateDocument* activeDoc = 0;

  KateDocumentConfig* docConfig = KateDocumentConfig::global();
  KateViewConfig* viewConfig = KateViewConfig::global();
  KateRendererConfig *rendererConfig = KateRendererConfig::global();

  KTextEditor::MdiContainer *iface = qobject_cast<KTextEditor::MdiContainer*>(KateGlobal::self()->container());
  if (iface) {
    activeView = qobject_cast<KateView*>(iface->activeView());

    if (activeView) {
      activeDoc = activeView->doc();
      viewConfig = activeView->config();
      docConfig = activeDoc->config();
      rendererConfig = activeView->renderer()->config();
    }
  }

  // Add 'auto-center-lines' to list
  item = new VariableIntItem("auto-center-lines", viewConfig->autoCenterLines());
  static_cast<VariableIntItem*>(item)->setRange(1, 100);
  item->setHelpText(i18nc("short translation please", "Set the number of autocenter lines."));
  listview->addItem(item);

  // Add 'auto-insert-doxygen' to list
  item = new VariableBoolItem("auto-insert-doxygen", false);
  item->setHelpText(i18nc("short translation please", "Auto insert asterisk in doxygen comments."));
  listview->addItem(item);

  // Add 'background-color' to list
  item = new VariableColorItem("background-color", rendererConfig->backgroundColor());
  item->setHelpText(i18nc("short translation please", "Set the document background color."));
  listview->addItem(item);

  // Add 'backspace-indents' to list
  item = new VariableBoolItem("backspace-indents", docConfig->backspaceIndents());
  item->setHelpText(i18nc("short translation please", "Pressing backspace in leading whitespace unindents."));
  listview->addItem(item);

  // Add 'block-selection' to list
  item = new VariableBoolItem("block-selection", false);
  if (activeView) static_cast<VariableBoolItem*>(item)->setValue(activeView->blockSelection());
  item->setHelpText(i18nc("short translation please", "Enable block selection mode."));
  listview->addItem(item);

  // Add 'byte-order-marker' (bom) to list
  item = new VariableBoolItem("byte-order-marker", docConfig->bom());
  item->setHelpText(i18nc("short translation please", "Enable the byte order marker when saving unicode files."));
  listview->addItem(item);

  // Add 'bracket-highlight-color' to list
  item = new VariableColorItem("bracket-highlight-color", rendererConfig->highlightedBracketColor());
  item->setHelpText(i18nc("short translation please", "Set the color for the bracket highlight."));
  listview->addItem(item);

  // Add 'current-line-color' to list
  item = new VariableColorItem("current-line-color", rendererConfig->highlightedLineColor());
  item->setHelpText(i18nc("short translation please", "Set the background color for the current line."));
  listview->addItem(item);

  // Add 'default-dictionary' to list
  Sonnet::Speller speller;
  item = new VariableSpellCheckItem("default-dictionary", speller.defaultLanguage());
  item->setHelpText(i18nc("short translation please", "Set the default dictionary used for spell checking."));
  listview->addItem(item);

  // Add 'dynamic-word-wrap' to list
  item = new VariableBoolItem("dynamic-word-wrap", viewConfig->dynWordWrap());
  item->setHelpText(i18nc("short translation please", "Enable dynamic word wrap of long lines."));
  listview->addItem(item);

  // Add 'end-of-line' (eol) to list
  item = new VariableStringListItem("end-of-line", QStringList() << "unix" << "mac" << "dos", docConfig->eolString());
  item->setHelpText(i18nc("short translation please", "Sets the end of line mode."));
  listview->addItem(item);

  // Add 'folding-markers' to list
  item = new VariableBoolItem("folding-markers", viewConfig->foldingBar());
  item->setHelpText(i18nc("short translation please", "Enable folding markers in the editor border."));
  listview->addItem(item);

  // Add 'font-size' to list
  item = new VariableIntItem("font-size", rendererConfig->font().pointSize());
  static_cast<VariableIntItem*>(item)->setRange(4, 128);
  item->setHelpText(i18nc("short translation please", "Set the point size of the document font."));
  listview->addItem(item);

  // Add 'font' to list
  item = new VariableFontItem("font", rendererConfig->font());
  item->setHelpText(i18nc("short translation please", "Set the font of the document."));
  listview->addItem(item);

  // Add 'syntax' (hl) to list
  /* Prepare list of highlighting modes */
  const int count = KateHlManager::self()->highlights();
  QStringList hl;
  for (int z = 0; z < count; ++z)
    hl << KateHlManager::self()->hlName(z);

  item = new VariableStringListItem("syntax", hl, hl.at(0));
  if (activeDoc) static_cast<VariableStringListItem*>(item)->setValue(activeDoc->highlightingMode());
  item->setHelpText(i18nc("short translation please", "Set the syntax highlighting."));
  listview->addItem(item);

  // Add 'icon-bar-color' to list
  item = new VariableColorItem("icon-bar-color", rendererConfig->iconBarColor());
  item->setHelpText(i18nc("short translation please", "Set the icon bar color."));
  listview->addItem(item);

  // Add 'icon-border' to list
  item = new VariableBoolItem("icon-border", viewConfig->iconBar());
  item->setHelpText(i18nc("short translation please", "Enable the icon border in the editor view."));
  listview->addItem(item);

  // Add 'indent-mode' to list
  item = new VariableStringListItem("indent-mode", KateAutoIndent::listIdentifiers(), docConfig->indentationMode());
  item->setHelpText(i18nc("short translation please", "Set the auto indentation style."));
  listview->addItem(item);

  // Add 'indent-pasted-text' to list
  item = new VariableBoolItem("indent-pasted-text", docConfig->indentPastedText());
  item->setHelpText(i18nc("short translation please", "Adjust indentation of text pasted from the clipboard."));
  listview->addItem(item);

  // Add 'indent-width' to list
  item = new VariableIntItem("indent-width", docConfig->indentationWidth());
  static_cast<VariableIntItem*>(item)->setRange(1, 16);
  item->setHelpText(i18nc("short translation please", "Set the indentation depth for each indent level."));
  listview->addItem(item);

  // Add 'keep-extra-spaces' to list
  item = new VariableBoolItem("keep-extra-spaces", docConfig->keepExtraSpaces());
  item->setHelpText(i18nc("short translation please", "Allow odd indentation level (no multiple of indent width)."));
  listview->addItem(item);

  // Add 'line-numbers' to list
  item = new VariableBoolItem("line-numbers", viewConfig->lineNumbers());
  item->setHelpText(i18nc("short translation please", "Show line numbers."));
  listview->addItem(item);

  // Add 'newline-at-eof' to list
  item = new VariableBoolItem("newline-at-eof", docConfig->ovr());
  item->setHelpText(i18nc("short translation please", "Insert newline at end of file on save."));
  listview->addItem(item);

  // Add 'overwrite-mode' to list
  item = new VariableBoolItem("overwrite-mode", docConfig->ovr());
  item->setHelpText(i18nc("short translation please", "Enable overwrite mode in the document."));
  listview->addItem(item);

  // Add 'persistent-selection' to list
  item = new VariableBoolItem("persistent-selection", viewConfig->persistentSelection());
  item->setHelpText(i18nc("short translation please", "Enable persistent text selection."));
  listview->addItem(item);

  // Add 'replace-tabs-save' to list
  item = new VariableBoolItem("replace-tabs-save", false);
  item->setHelpText(i18nc("short translation please", "Replace tabs with spaces when saving the document."));
  listview->addItem(item);

  // Add 'replace-tabs' to list
  item = new VariableBoolItem("replace-tabs", docConfig->replaceTabsDyn());
  item->setHelpText(i18nc("short translation please", "Replace tabs with spaces."));
  listview->addItem(item);

  // Add 'remove-trailing-spaces' to list
  item = new VariableRemoveSpacesItem("remove-trailing-spaces", docConfig->removeSpaces());
  item->setHelpText(i18nc("short translation please", "Remove trailing spaces when saving the document."));
  listview->addItem(item);

  // Add 'scheme' to list
  QStringList schemas;
  Q_FOREACH (const KateSchema &schema, KateGlobal::self()->schemaManager()->list())
    schemas.append (schema.rawName);
  item = new VariableStringListItem("scheme", schemas, rendererConfig->schema());
  item->setHelpText(i18nc("short translation please", "Set the color scheme."));
  listview->addItem(item);

  // Add 'selection-color' to list
  item = new VariableColorItem("selection-color", rendererConfig->selectionColor());
  item->setHelpText(i18nc("short translation please", "Set the text selection color."));
  listview->addItem(item);

  // Add 'show-tabs' to list
  item = new VariableBoolItem("show-tabs", docConfig->showTabs());
  item->setHelpText(i18nc("short translation please", "Visualize tabs and trailing spaces."));
  listview->addItem(item);

  // Add 'smart-home' to list
  item = new VariableBoolItem("smart-home", docConfig->smartHome());
  item->setHelpText(i18nc("short translation please", "Enable smart home navigation."));
  listview->addItem(item);

  // Add 'tab-indents' to list
  item = new VariableBoolItem("tab-indents", docConfig->tabIndentsEnabled());
  item->setHelpText(i18nc("short translation please", "Pressing TAB key indents."));
  listview->addItem(item);

  // Add 'tab-width' to list
  item = new VariableIntItem("tab-width", docConfig->tabWidth());
  static_cast<VariableIntItem*>(item)->setRange(1, 16);
  item->setHelpText(i18nc("short translation please", "Set the tab display width."));
  listview->addItem(item);

  // Add 'undo-steps' to list
  item = new VariableIntItem("undo-steps", 0);
  static_cast<VariableIntItem*>(item)->setRange(0, 100);
  item->setHelpText(i18nc("short translation please", "Set the number of undo steps to remember (0 equals infinity)."));
  listview->addItem(item);

  // Add 'word-wrap-column' to list
  item = new VariableIntItem("word-wrap-column", docConfig->wordWrapAt());
  static_cast<VariableIntItem*>(item)->setRange(20, 200);
  item->setHelpText(i18nc("short translation please", "Set the word wrap column."));
  listview->addItem(item);

  // Add 'word-wrap-marker-color' to list
  item = new VariableColorItem("word-wrap-marker-color", rendererConfig->wordWrapMarkerColor());
  item->setHelpText(i18nc("short translation please", "Set the word wrap marker color."));
  listview->addItem(item);

  // Add 'word-wrap' to list
  item = new VariableBoolItem("word-wrap", docConfig->wordWrap());
  item->setHelpText(i18nc("short translation please", "Enable word wrap while typing text."));
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
