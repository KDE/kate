/* This file is part of the KDE project
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katemailfilesdialog.h"

#include <KLocale>
#include <KVBox>
#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <QLabel>
#include <QList>

/* a private check list item, that can store a KTextEditor::Document*.  */
class KateMailDocItem : public QTreeWidgetItem
{
  public:
    KateMailDocItem( QTreeWidget *parent, KTextEditor::Document *doc )
        : QTreeWidgetItem( parent ), mdoc(doc)
    {
      setText(0, doc->documentName());
      setText(1, doc->url().pathOrUrl());
      setCheckState(0, Qt::Unchecked);
    }
    KTextEditor::Document *doc()
    {
      return mdoc;
    }
  private:
    KTextEditor::Document *mdoc;
};

///////////////////////////////////////////////////////////////////////////
// KateMailDialog implementation
///////////////////////////////////////////////////////////////////////////
KateMailDialog::KateMailDialog( QWidget *parent, Kate::MainWindow  *mainwin )
    : KDialog( parent ),
    mainWindow( mainwin )
{
  setCaption( i18n("Email Files") );
  setButtons( Ok | Cancel | User1 );
  setButtonGuiItem( User1, KGuiItem( i18n("&Show All Documents >>") ) );
  setObjectName( "kate mail dialog" );
  setModal( true );

  setButtonGuiItem( KDialog::Ok, KGuiItem( i18n("&Mail..."), "mail-send") );

  mw = new KVBox(this);
  setMainWidget(mw);
  mw->installEventFilter( this );

  lInfo = new QLabel( i18n(
                        "<p>Press <strong>Mail...</strong> to email the current document.</p>"
                        "<p>To select more documents to send, press <strong>Show All Documents&nbsp;&gt;&gt;</strong>.</p>"), mw );
  // TODO avoid untill needed - later
  list = new QTreeWidget( mw );
  QStringList header;
  header << i18n("Name");
  header << i18n("URL");
  list->setHeaderLabels(header);

  KTextEditor::Document *currentDoc = mainWindow->activeView()->document();
  uint n = Kate::documentManager()->documents().size();
  uint i = 0;
  QTreeWidgetItem *item;
  while ( i < n )
  {
    KTextEditor::Document *doc = Kate::documentManager()->documents().at( i );
    if ( doc )
    {
      item = new KateMailDocItem( list, doc );
      if ( doc == currentDoc )
      {
        list->setCurrentItem(item);
        item->setCheckState(0, Qt::Checked);
      }
    }
    i++;
  }
  list->hide();
  connect( this, SIGNAL(user1Clicked()), this, SLOT(slotShowButton()) );
  mw->setMinimumSize( lInfo->sizeHint() );
}

QList<KTextEditor::Document *> KateMailDialog::selectedDocs()
{
  QList<KTextEditor::Document *> l;
  KateMailDocItem *item = NULL;
  for(int i = 0; i < list->topLevelItemCount(); i++)
  {
    item = static_cast<KateMailDocItem *>( list->topLevelItem(i) );
    if ( item->checkState(0) == Qt::Checked )
      l.append( item->doc() );
  }
  return l;
}

void KateMailDialog::slotShowButton()
{
  if ( list->isVisible() )
  {
    setButtonText( User1, i18n("&Show All Documents >>") );
    list->hide();
    mw->setMinimumSize( QSize( lInfo->sizeHint().width(), lInfo->sizeHint().height()) );
    setMinimumSize( mw->width(), sizeHint().height() - list->sizeHint().height());
  }
  else
  {
    list->show();
    setButtonText( User1, i18n("&Hide Document List <<") );
    lInfo->setText( i18n("Press <strong>Mail...</strong> to send selected documents") );
    mw->setMinimumSize( QSize( lInfo->sizeHint().width(), list->sizeHint().height() + lInfo->sizeHint().height()) );
    setMinimumSize( mw->width(), sizeHint().height());
  }

  resize( width(), minimumHeight() );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
