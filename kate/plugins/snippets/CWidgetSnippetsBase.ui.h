/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

#ifndef CWIDGETSNIPPETSBASE_UI_H
#define CWIDGETSNIPPETSBASE_UI_H

void CWidgetSnippetsBase::init()
{

}


Q3ListViewItem* CWidgetSnippetsBase::insertItem( const QString& name, bool bRename )
{
    Q3ListViewItem *item = new Q3ListViewItem(lvSnippets, name);
    item->setRenameEnabled(0,true);
    lvSnippets->setSelected( item, true );
    if ( bRename ) {
	teSnippetText->clear();
	item->startRename (0);
    }
    return item;
}

#endif // CWIDGETSNIPPETSBASE_UI_H
