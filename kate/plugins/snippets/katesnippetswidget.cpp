/* This file is part of the KDE project
   Copyright (C) 2004 Stephan Möres <Erdling@gmx.net>
   Copyright (C) 2008 Jakob Petsovits <jpetso@gmx.at>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "katesnippetswidget.h"
#include "katesnippetswidget.moc"

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <klocale.h>

#include <q3listview.h>
#include <qregexp.h>
#include <qtoolbutton.h>
#include <q3textedit.h>

#include <QDate>
#include <QTime>


KateSnippetsWidget::KateSnippetsWidget( Kate::MainWindow *mainWindow, QWidget* parent, const char* name, Qt::WFlags fl)
    : KateSnippetsWidgetBase( parent, name, fl ), m_mainWin( mainWindow )
{
  connect(
    lvSnippets, SIGNAL( selectionChanged(Q3ListViewItem *) ),
    this, SLOT( slotSnippetSelectionChanged(Q3ListViewItem *) )
  );
  connect(
    lvSnippets, SIGNAL( doubleClicked(Q3ListViewItem *) ),
    this, SLOT( slotSnippetItemClicked(Q3ListViewItem  *) )
  );
  connect(
    lvSnippets, SIGNAL( itemRenamed(Q3ListViewItem *, int, const QString &) ),
    this, SLOT( slotSnippetItemRenamed(Q3ListViewItem *, int, const QString &) )
  );

  connect( btnNew,    SIGNAL( clicked() ), this, SLOT( slotNewClicked() ) );
  connect( btnSave,   SIGNAL( clicked() ), this, SLOT( slotSaveClicked() ) );
  connect( btnDelete, SIGNAL( clicked() ), this, SLOT( slotDeleteClicked() ) );
}

KateSnippetsWidget::~KateSnippetsWidget()
{
  // The snippets are only stored as pointers, so we need to clean up.
  foreach ( KateSnippet *snippet, m_snippets ) {
    delete snippet;
  }
}

/**
 * Read existing snippets from the given config, or create default ones
 * if no snippets exist yet.
 */
void KateSnippetsWidget::readConfig( KConfigBase* config, const QString& groupPrefix )
{
  QString key, value;
  Q3ListViewItem *lvi;

  KConfigGroup cg( config, groupPrefix );
  int snippetCount = cg.readEntry( "NumberOfSnippets", "0" ).toInt();

  for ( int i = 0; i < snippetCount; i++ )
  {
    QStringList fields;
    fields = cg.readEntry( QString::number(i), QStringList() );

    key   = fields[0];
    value = fields[1];

    lvi = insertItem( key, false );

    m_snippets.append( new KateSnippet(key, value, lvi) );
  }

  // <defaults>
  if ( snippetCount == 0 ) {
    key  = "DEBUG variable";
    value = "## < DEBUG >\nout \"<pre>\\$<mark/> : \\\"$<mark/>\\\"\\n</pre>\"\n## </DEBUG >\n";
    lvi = insertItem( key, false );
    m_snippets.append( new KateSnippet(key, value, lvi) );

    key  = "proc-header";
    value  = "## [created : <date/>, <time/>]\n## Description:\n## ============\n## The function \"<mark/>\" ...\n##\n##\n##\n##\n## Input:\n## ======\n##\n##\n##\nproc <mark/> {args} {\n\n  ## add your code here\n\n return \"\"\n}\n";
    lvi = insertItem( key, false );
    m_snippets.append( new KateSnippet(key, value, lvi) );
  }
  // </defaults>

  // set text of selected item at startup
  slotSnippetSelectionChanged( lvSnippets->selectedItem() );
}

/**
 * Store all snippets to the given config.
 */
void KateSnippetsWidget::writeConfig( KConfigBase* config, const QString& groupPrefix )
{
  KConfigGroup cg( config, groupPrefix );

  int snippetCount = m_snippets.count();
  cg.writeEntry( "NumberOfSnippets", snippetCount );

  int i = 0;

  Q_FOREACH ( KateSnippet *snippet, m_snippets ) {
    QStringList fields;
    fields.append( snippet->key() );
    fields.append( snippet->value() );

    cg.writeEntry( QString::number(i), fields );
    i++;
  }
  // sync to disc...
  config->sync();
}

KateSnippet* KateSnippetsWidget::findSnippetByListViewItem( Q3ListViewItem *item )
{
  Q_FOREACH ( KateSnippet *snippet, m_snippets ) {
    if ( snippet->listViewItem() == item ) {
      return snippet;
    }
  }
  return 0;
}

void KateSnippetsWidget::slotSnippetSelectionChanged( Q3ListViewItem  *item )
{
  KateSnippet *snippet;
  if ( (snippet = findSnippetByListViewItem(item)) != 0 ) {
    teSnippetText->setText( snippet->value() );
  }
}

void KateSnippetsWidget::slotSnippetItemClicked( Q3ListViewItem  *item )
{
  KTextEditor::View *kv = mainWindow()->activeView();
  KateSnippet *snippet;

  if ( kv ) {
    if ( (snippet = findSnippetByListViewItem(item)) != 0 ) {
      QString text = snippet->value();
      QString selection = "";

      if ( kv->selection() ) {
        selection = kv->selectionText();
        // clear selection
        kv->removeSelectionText();
      }

      // <mark/>, <date/> and <time/> are special tokens and will be replaced.
      text.replace( "<mark/>", selection );
      text.replace( "<date/>", QDate::currentDate().toString(Qt::LocalDate) );
      text.replace( "<time/>", QTime::currentTime().toString(Qt::LocalDate) );
      kv->document()->insertText( kv->cursorPosition(), text );
    }
    kv->setFocus();
  }
}

void KateSnippetsWidget::slotSnippetItemRenamed( Q3ListViewItem *lvi, int /*col*/, const QString& text )
{
  KateSnippet *snippet;
  if ( (snippet = findSnippetByListViewItem(lvi)) != 0 ) {
    snippet->setKey( text );
    emit saveRequested();
  }
}

void KateSnippetsWidget::slotNewClicked()
{
  QString key = i18n("New Snippet");
  QString value = "";

  Q3ListViewItem *lvi = insertItem( key, true );
  m_snippets.append( new KateSnippet(key, value, lvi) );
}

void KateSnippetsWidget::slotSaveClicked()
{
  KateSnippet *snippet;
  Q3ListViewItem *lvi = lvSnippets->selectedItem();
  if ( (snippet = findSnippetByListViewItem(lvi)) != 0 ) {
    snippet->setValue( teSnippetText->text() );
    emit saveRequested();
  }
}

void KateSnippetsWidget::slotDeleteClicked()
{
  KateSnippet *snippet;
  Q3ListViewItem *lvi = lvSnippets->selectedItem();

  if ( (snippet = findSnippetByListViewItem(lvi)) != 0 ) {
    lvSnippets->takeItem( lvi );
    m_snippets.remove( snippet );
    delete snippet;
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
