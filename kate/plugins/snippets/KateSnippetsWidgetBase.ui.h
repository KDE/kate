/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

#ifndef KATESNIPPETSWIDGETBASE_UI_H
#define KATESNIPPETSWIDGETBASE_UI_H

void KateSnippetsWidgetBase::init()
{
}

Q3ListViewItem* KateSnippetsWidgetBase::insertItem( const QString& name, bool rename )
{
  Q3ListViewItem *item = new Q3ListViewItem( lvSnippets, name );
  item->setRenameEnabled( 0, true );
  lvSnippets->setSelected( item, true );
  if ( rename ) {
    teSnippetText->clear();
    item->startRename (0);
  }
  return item;
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
