/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

   Based on work of:
     Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

//BEGIN Really??
// Copyright (c) 2000-2001 Charles Samuels <charles@kde.org>
// Copyright (c) 2000-2001 Neil Stevens <multivac@fcmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIAB\ILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//END

// $Id$

//BEGIN Includes
#include "katedialogs.h"
#include "katedialogs.moc"

#include "katesyntaxdocument.h"
#include "katedocument.h"
#include "katehighlightdownload.h"
#include "katefactory.h"
#include "kateconfig.h"

#include <kapplication.h>
#include <kspell.h>
#include <kbuttonbox.h>
#include <kcharsets.h>
#include <kcolorcombo.h>
#include <kcolordialog.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kstandarddirs.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qheader.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmap.h>
#include <qpainter.h>
#include <qpointarray.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>
//END

// FIXME THE isSomethingSet() calls should partly be replaced by itemSet(XYZ) and
// there is a need for an itemUnset(XYZ)
// a) is done. itemUnset(N) == !itemSet(N); right???
// there is a unsetItem(), just (not all logic) called "KateAttribute::clearAttribute(int)";

using namespace Kate;

#define TAG_DETECTCHAR "DetectChar"
#define TAG_DETECT2CHARS "Detect2Chars"
#define TAG_RANGEDETECT "RangeDetect"
#define TAG_STRINGDETECT "StringDetect"
#define TAG_ANYCHAR "AnyChar"
#define TAG_REGEXPR "RegExpr"
#define TAG_INT "Int"
#define TAG_FLOAT "Float"
#define TAG_KEYWORD "keyword"

//BEGIN PluginListItem
PluginListItem::PluginListItem(const bool _exclusive, bool _checked, PluginInfo *_info, QListView *_parent)
  : QCheckListItem(_parent, _info->service->name(), CheckBox)
  , mInfo(_info)
  , silentStateChange(false)
  , exclusive(_exclusive)
{
  setChecked(_checked);
  if(_checked) static_cast<PluginListView *>(listView())->count++;
}


void PluginListItem::setChecked(bool b)
{
  silentStateChange = true;
  setOn(b);
  silentStateChange = false;
}

void PluginListItem::stateChange(bool b)
{
  if(!silentStateChange)
    static_cast<PluginListView *>(listView())->stateChanged(this, b);
}

void PluginListItem::paintCell(QPainter *p, const QColorGroup &cg, int a, int b, int c)
{
  if(exclusive) myType = RadioButton;
  QCheckListItem::paintCell(p, cg, a, b, c);
  if(exclusive) myType = CheckBox;
}
//END

//BEGIN PluginListView
PluginListView::PluginListView(unsigned _min, unsigned _max, QWidget *_parent, const char *_name)
  : KListView(_parent, _name)
  , hasMaximum(true)
  , max(_max)
  , min(_min <= _max ? _min : _max)
  , count(0)
{
}

PluginListView::PluginListView(unsigned _min, QWidget *_parent, const char *_name)
  : KListView(_parent, _name)
  , hasMaximum(false)
  , min(_min)
  , count(0)
{
}

PluginListView::PluginListView(QWidget *_parent, const char *_name)
  : KListView(_parent, _name)
  , hasMaximum(false)
  , min(0)
  , count(0)
{
}

void PluginListView::clear()
{
  count = 0;
  KListView::clear();
}

void PluginListView::stateChanged(PluginListItem *item, bool b)
{
  if(b)
  {
    count++;
    emit stateChange(item, b);

    if(hasMaximum && count > max)
    {
      // Find a different one and turn it off

      QListViewItem *cur = firstChild();
      PluginListItem *curItem = dynamic_cast<PluginListItem *>(cur);

      while(cur == item || !curItem || !curItem->isOn())
      {
        cur = cur->nextSibling();
        curItem = dynamic_cast<PluginListItem *>(cur);
      }

      curItem->setOn(false);
    }
  }
  else
  {
    if(count == min)
    {
      item->setChecked(true);
    }
    else
    {
      count--;
      emit stateChange(item, b);
    }
  }
}
//END

//BEGIN PluginConfigPage
 PluginConfigPage::PluginConfigPage (QWidget *parent, KateDocument *doc) : Kate::ConfigPage (parent, "")
{
  m_doc = doc;

  // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  PluginListView* listView = new PluginListView(0, this);
  listView->addColumn(i18n("Name"));
  listView->addColumn(i18n("Description"));
  listView->addColumn(i18n("Author"));
  listView->addColumn(i18n("License"));
  connect(listView, SIGNAL(stateChange(PluginListItem *, bool)), this, SLOT(stateChange(PluginListItem *, bool)));

  grid->addWidget( listView, 0, 0);

  for (uint i=0; i<m_doc->s_plugins.count(); i++)
  {
    PluginListItem *item = new PluginListItem(false, m_doc->s_plugins.at(i)->load, m_doc->s_plugins.at(i), listView);
    item->setText(0, m_doc->s_plugins.at(i)->service->name());
    item->setText(1, m_doc->s_plugins.at(i)->service->comment());
    item->setText(2, "");
    item->setText(3, "");
  }
}

PluginConfigPage::~PluginConfigPage ()
{
}


 void PluginConfigPage::stateChange(PluginListItem *item, bool b)
{
  if(b)
    loadPlugin(item);
  else
    unloadPlugin(item);
  emit changed();
}

void PluginConfigPage::loadPlugin (PluginListItem *item)
{
  item->info()->load = true;
  for (uint z=0; z < KateFactory::self()->documents()->count(); z++)
    KateFactory::self()->documents()->at(z)->loadAllEnabledPlugins ();

  item->setOn(true);
}

void PluginConfigPage::unloadPlugin (PluginListItem *item)
{
  item->info()->load = false;
  for (uint z=0; z < KateFactory::self()->documents()->count(); z++)
    KateFactory::self()->documents()->at(z)->loadAllEnabledPlugins ();

  item->setOn(false);
}
//END

//BEGIN HlConfigPage
HlConfigPage::HlConfigPage (QWidget *parent)
 : Kate::ConfigPage (parent, "")
 , hlData (0)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  QHBox *hbHl = new QHBox( this );
  layout->add (hbHl);
  
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("H&ighlight:"), hbHl );
  hlCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( hlCombo );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );

  for( int i = 0; i < HlManager::self()->highlights(); i++) {
    if (HlManager::self()->hlSection(i).length() > 0)
      hlCombo->insertItem(HlManager::self()->hlSection(i) + QString ("/") + HlManager::self()->hlName(i));
    else
      hlCombo->insertItem(HlManager::self()->hlName(i));
  }
  hlCombo->setCurrentItem(0);

  QGroupBox *gbProps = new QGroupBox( 1, Qt::Horizontal, i18n("Properties"), this );
  layout->add (gbProps);

  // file & mime types
  QHBox *hbFE = new QHBox( gbProps);
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), hbFE );
  wildcards  = new QLineEdit( hbFE );
  lFileExts->setBuddy( wildcards );
  connect( wildcards, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  QHBox *hbMT = new QHBox( gbProps );
  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), hbMT);
  mimetypes = new QLineEdit( hbMT );
  connect( mimetypes, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  lMimeTypes->setBuddy( mimetypes );

  QHBox *hbMT2 = new QHBox( gbProps );
  QLabel *lprio = new QLabel( i18n("Prio&rity:"), hbMT2);
  priority = new KIntNumInput( hbMT2 );
  connect( priority, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );
  lprio->setBuddy( priority );

  QToolButton *btnMTW = new QToolButton(hbMT);
  btnMTW->setIconSet(QIconSet(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));

  // download/new buttons
  QHBox *hbBtns = new QHBox( this );
  layout->add (hbBtns);
  
  ((QBoxLayout*)hbBtns->layout())->addStretch(1); // hmm.
  hbBtns->setSpacing( KDialog::spacingHint() );
  QPushButton *btnDl = new QPushButton(i18n("Do&wnload..."), hbBtns);
  connect( btnDl, SIGNAL(clicked()), this, SLOT(hlDownload()) );

  hlCombo->setCurrentItem( 0 );
  hlChanged(0);

  QWhatsThis::add( hlCombo,   i18n("Choose a <em>Syntax Highlight mode</em> from this list to view its properties below.") );
  QWhatsThis::add( wildcards, i18n("The list of file extensions used to determine which files to highlight using the current syntax highlight mode.") );
  QWhatsThis::add( mimetypes, i18n("The list of Mime Types used to determine which files to highlight using the current highlight mode.<p>Click the wizard button on the left of the entry field to display the MimeType selection dialog.") );
  QWhatsThis::add( btnMTW,    i18n("Display a dialog with a list of all available mime types to choose from.<p>The <strong>File Extensions</strong> entry will automatically be edited as well.") );
  QWhatsThis::add( btnDl,     i18n("Click this button to download new or updated syntax highlight descriptions from the Kate website.") );
  
  layout->addStretch ();
}

HlConfigPage::~HlConfigPage ()
{
}

void HlConfigPage::apply ()
{
  writeback();
  
  for ( QIntDictIterator<HlData> it( hlDataDict ); it.current(); ++it )
    HlManager::self()->getHl( it.currentKey() )->setData( it.current() );
    
  HlManager::self()->getKConfig()->sync ();
}

void HlConfigPage::reload ()
{
}

void HlConfigPage::hlChanged(int z)
{
  writeback();

  if ( ! hlDataDict.find( z ) )
    hlDataDict.insert( z, HlManager::self()->getHl( z )->getData() );

  hlData = hlDataDict.find( z );
  wildcards->setText(hlData->wildcards);
  mimetypes->setText(hlData->mimetypes);
  priority->setValue(hlData->priority);
}

void HlConfigPage::writeback()
{
  if (hlData)
  {
    hlData->wildcards = wildcards->text();
    hlData->mimetypes = mimetypes->text();
    hlData->priority = priority->value();
  }
}

void HlConfigPage::hlDownload()
{
  HlDownloadDialog diag(this,"hlDownload",true);
  diag.exec();
}

void HlConfigPage::showMTDlg()
{
  QString text = i18n("Select the MimeTypes you want highlighted using the '%1' syntax highlight rules.\nPlease note that this will automatically edit the associated file extensions as well.").arg( hlCombo->currentText() );
  QStringList list = QStringList::split( QRegExp("\\s*;\\s*"), mimetypes->text() );
  KMimeTypeChooserDlg *d = new KMimeTypeChooserDlg( this, i18n("Select Mime Types"), text, list );
  
  if ( d->exec() == KDialogBase::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    wildcards->setText(d->patterns().join(";"));
    mimetypes->setText(d->mimeTypes().join(";"));
  }
}
//END HlConfigPage

//BEGIN StyleListView
/*********************************************************************/
/*                  StyleListView Implementation                     */
/*********************************************************************/
StyleListView::StyleListView( QWidget *parent, bool showUseDefaults, QColor textcol )
    : QListView( parent ),
      normalcol( textcol )
{
  addColumn( i18n("Context") );
  addColumn( SmallIconSet("text_bold"), QString::null/*i18n("Bold")*/ );
  addColumn( SmallIconSet("text_italic"), QString::null/*i18n("Italic")*/ );
  addColumn( SmallIconSet("text_under"), QString::null );
  addColumn( SmallIconSet("text_strike"), QString::null );
  addColumn( i18n("Normal") );
  addColumn( i18n("Selected") );
  addColumn( i18n("Background") );
  addColumn( i18n("Background Selected") );
  if ( showUseDefaults )
    addColumn( i18n("Use Default Style") );
  connect( this, SIGNAL(mouseButtonPressed(int, QListViewItem*, const QPoint&, int)),
           this, SLOT(slotMousePressed(int, QListViewItem*, const QPoint&, int)) );
  connect( this, SIGNAL(spacePressed(QListViewItem*)),
           this, SLOT(showPopupMenu(QListViewItem*)) );
  // grap the bg color, selected color and default font
  bgcol = *KateRendererConfig::global()->backgroundColor();
  selcol = *KateRendererConfig::global()->selectionColor();
  docfont = *KateRendererConfig::global()->font();

  viewport()->setPaletteBackgroundColor( bgcol );
}

void StyleListView::showPopupMenu( StyleListItem *i, const QPoint &globalPos, bool showtitle )
{
  KPopupMenu m( this );
  KateAttribute *is = i->style();
  int id;
  // the title is used, because the menu obscures the context name when
  // displayed on behalf of spacePressed().
  QPixmap cl(16,16);
  cl.fill( i->style()->textColor() );
  QPixmap scl(16,16);
  scl.fill( i->style()->selectedTextColor() );
  if ( showtitle )
    m.insertTitle( i->contextName(), StyleListItem::ContextName );
  id = m.insertItem( i18n("&Bold"), this, SLOT(mSlotPopupHandler(int)), 0, StyleListItem::Bold );
  m.setItemChecked( id, is->bold() );
  id = m.insertItem( i18n("&Italic"), this, SLOT(mSlotPopupHandler(int)), 0, StyleListItem::Italic );
  m.setItemChecked( id, is->italic() );
  m.insertItem( QIconSet(cl), i18n("Normal &Color..."), this, SLOT(mSlotPopupHandler(int)), 0, StyleListItem::Color );
  m.insertItem( QIconSet(scl), i18n("&Selected Color..."), this, SLOT(mSlotPopupHandler(int)), 0, StyleListItem::SelColor );
  if ( ! i->isDefault() ) {
    id = m.insertItem( i18n("Use &Default Style"), this, SLOT(mSlotPopupHandler(int)), 0, StyleListItem::UseDefStyle );
    m.setItemChecked( id, i->defStyle() );
  }
  m.exec( globalPos );
}

void StyleListView::showPopupMenu( QListViewItem *i )
{
  showPopupMenu( (StyleListItem*)i, viewport()->mapToGlobal(itemRect(i).topLeft()), true );
}

void StyleListView::mSlotPopupHandler( int z )
{
  ((StyleListItem*)currentItem())->changeProperty( (StyleListItem::Property)z );
}

// Because QListViewItem::activatePos() is going to become deprecated,
// and also because this attempt offers more control, I connect mousePressed to this.
void StyleListView::slotMousePressed(int btn, QListViewItem* i, const QPoint& pos, int c)
{
  if ( i ) {
    if ( btn == Qt::RightButton ) {
      showPopupMenu( (StyleListItem*)i, /*mapToGlobal(*/pos/*)*/ );
    }
    else if ( btn == Qt::LeftButton && c > 0 ) {
      // map pos to item/column and call StyleListItem::activate(col, pos)
      ((StyleListItem*)i)->activate( c, viewport()->mapFromGlobal( pos ) - QPoint( 0, itemRect(i).top() ) );
    }
  }
}

/* broken ?!
#include <kstdaccel.h>
#include <qcursor.h>
void StyleListView::keyPressEvent( QKeyEvent *e )
{
  if ( ! currentItem() ) return;
  if ( isVisible() && KStdAccel::isEqual( e, KStdAccel::key(KStdAccel::PopupMenuContext) ) ) {
    QPoint p = QCursor::pos();
    if( ! itemRect( currentItem() ).contains( mapFromGlobal(p)  ) )
      p = viewport()->mapToGlobal( itemRect(currentItem()).topLeft() );
    showPopupMenu( (StyleListItem*)currentItem(), p, true );
  }
  else
    QListView::keyPressEvent( e );
}
*/
//END

//BEGIN StyleListItem
/*********************************************************************/
/*                  StyleListItem Implementation                     */
/*********************************************************************/
static const int BoxSize = 16;
static const int ColorBtnWidth = 32;

StyleListItem::StyleListItem( QListView *parent, const QString & stylename,
                              KateAttribute *style, ItemData *data )
        : QListViewItem( parent, stylename ),
          /*styleName( stylename ),*/
          ds( style ),
          st( data )
{
  if ( st )
  {
    KateAttribute shit( *ds );
    is = new KateAttribute( shit += *dynamic_cast<KateAttribute*>(st) );
  }
  else
    is = ds;
}

void StyleListItem::updateStyle()
{
  //kdDebug()<<text(0)<<": set: "<<st->itemsSet()<<endl;
  /*
  save a atom if
      it is set and has changed and
      the setting is different from the default
  */
  if ( is->itemSet(KateAttribute::Weight) )
  {
    if ( is->weight() != st->weight() &&
        ( !ds->itemSet(KateAttribute::Weight) || is->weight() != ds->weight() ) )
      st->setWeight( is->weight() );
    else
      st->clearAttribute(KateAttribute::Weight);
  }
  if ( is->itemSet(KateAttribute::Italic) )
  {
    if ( is->italic() != st->italic() &&
        ( !ds->itemSet(KateAttribute::Italic) || is->italic() != ds->italic() ) )
      st->setItalic( is->italic() );
    else
      st->clearAttribute(KateAttribute::Italic);
  }
  if ( is->itemSet(KateAttribute::StrikeOut) )
  {
    if ( is->strikeOut() != st->strikeOut() &&
        ( !ds->itemSet(KateAttribute::StrikeOut) || is->strikeOut() != ds->strikeOut() ) )
      st->setStrikeOut( is->strikeOut() );
    else
      st->clearAttribute(KateAttribute::StrikeOut);
  }
  if ( is->itemSet(KateAttribute::Underline) )
  {
    if ( is->underline() != st->underline() &&
        ( !ds->itemSet(KateAttribute::Underline) || is->underline() != ds->underline() ) )
      st->setUnderline( is->underline() );
    else
      st->clearAttribute(KateAttribute::Underline);
  }
  if ( is->itemSet(KateAttribute::Outline) )
  {
    if ( is->outline() != st->outline() &&
        ( !ds->itemSet(KateAttribute::Outline) || is->outline() != ds->outline() ) )
      st->setOutline( is->outline() );
    else
      st->clearAttribute(KateAttribute::Outline);
  }

  if ( is->itemSet(KateAttribute::TextColor) )
  {
    if ( is->textColor() != st->textColor() &&
        is->textColor() != ds->textColor() )
      st->setTextColor( is->textColor() );
    else
      st->clearAttribute(KateAttribute::TextColor);
  }
  if ( is->itemSet(KateAttribute::SelectedTextColor) )
  {
    if ( is->selectedTextColor() != st->selectedTextColor() &&
        is->selectedTextColor() != ds->selectedTextColor() )
      st->setSelectedTextColor( is->selectedTextColor() );
    else
      st->clearAttribute(KateAttribute::SelectedTextColor);
  }
  if ( is->itemSet(KateAttribute::BGColor) )
  {
    if ( is->bgColor() != st->bgColor() &&
        ( !ds->itemSet(KateAttribute::BGColor) || is->bgColor() != ds->bgColor() ) )
      st->setBGColor( is->bgColor() );
    else
      st->clearAttribute(KateAttribute::BGColor);
  }
  if ( is->itemSet(KateAttribute::SelectedBGColor) )
  {
    if ( is->selectedBGColor() != st->selectedBGColor() &&
        ( ! ds->itemSet(KateAttribute::SelectedBGColor) || is->selectedBGColor() != ds->selectedBGColor() ) )
      st->setSelectedBGColor( is->selectedBGColor() );
    else
      st->clearAttribute(KateAttribute::SelectedBGColor);
  }
  //kdDebug()<<"after update: "<<st->itemsSet()<<endl;
  //kdDebug()<<"bold: "<<st->bold()<<" ("<<is->bold()<<")"<<endl;
}

/* only true for a hl mode item using it's default style */
bool StyleListItem::defStyle() { return st && st->isSomethingSet(); }

/* true for default styles */
bool StyleListItem::isDefault() { return st ? false : true; }

int StyleListItem::width( const QFontMetrics & /*fm*/, const QListView * lv, int col ) const
{
  int m = lv->itemMargin() * 2;
  switch ( col ) {
    case ContextName:
      // FIXME: width for name column should reflect bold/italic
      // (relevant for non-fixed fonts only - nessecary?)
      return QFontMetrics( ((StyleListView*)lv)->docfont).width( text(0) ) + m;
    case Bold:
    case Italic:
    case UseDefStyle:
      return BoxSize + m;
    case Color:
    case SelColor:
    case BgColor:
    case SelBgColor:
      return ColorBtnWidth +m;
    default:
      return 0;
  }
}

void StyleListItem::activate( int column, const QPoint &localPos )
{
  QListView *lv = listView();
  int x = 0;
  for( int c = 0; c < column-1; c++ )
    x += lv->columnWidth( c );
  int w;
  switch( column ) {
    case Bold:
    case Italic:
    case Underline:
    case Strikeout:
    case UseDefStyle:
      w = BoxSize;
      break;
    case Color:
    case SelColor:
    case BgColor:
    case SelBgColor:
      w = ColorBtnWidth;
      break;
    default:
      return;
  }
  if ( !QRect( x, 0, w, BoxSize ).contains( localPos ) )
  changeProperty( (Property)column );
}

void StyleListItem::changeProperty( Property p )
{
  bool ch( true );
  if ( p == Bold )
    is->setBold( ! is->bold() );
  else if ( p == Italic )
    is->setItalic( ! is->italic() );
  else if ( p == Underline )
    is->setUnderline( ! is->underline() );
  else if ( p == Strikeout )
    is->setStrikeOut( ! is->strikeOut() );
  else if ( p == UseDefStyle )
    toggleDefStyle();
  else
  {
    setColor( p );
    ch = false;
  }
  if ( ch )
    ((StyleListView*)listView())->emitChanged();
}

void StyleListItem::toggleDefStyle()
{
  if ( *is == *ds ) {
    KMessageBox::information( listView(),
         i18n("\"Use Default Style\" will be automatically unset when you change any style properties."),
         i18n("Kate Styles"),
         "Kate hl config use defaults" );
  }
  else {
    delete is;
    is = new KateAttribute( *ds );
    repaint();
  }
}

void StyleListItem::setColor( int column )
{
  QColor c;
  if ( column == Color) c = is->textColor();
  else if ( column == SelColor ) c = is->selectedTextColor();
  else if ( column == BgColor ) c = is->bgColor();
  else if ( column == SelBgColor ) c = is->selectedBGColor();

  if ( KColorDialog::getColor( c, listView() ) != QDialog::Accepted) return;

  if (st && st->isSomethingSet()) setCustStyle();

  if ( column == Color) is->setTextColor( c );
  else if ( column == SelColor ) is->setSelectedTextColor( c );
  else if ( column == BgColor ) is->setBGColor( c );
  else if ( column == SelBgColor ) is->setSelectedBGColor( c );

  repaint();
}

void StyleListItem::setCustStyle()
{
//   is = st;
//   *is += *ds;
//  st->defStyle = 0;
}

void StyleListItem::paintCell( QPainter *p, const QColorGroup& cg, int col, int width, int align )
{

  if ( !p )
    return;

  QListView *lv = listView();
  if ( !lv )
    return;
  Q_ASSERT( lv ); //###

  p->fillRect( 0, 0, width, height(), QBrush( ((StyleListView*)lv)->bgcol ) );
  int marg = lv->itemMargin();

  // use a provate color group and set the text/highlighted text colors
  QColorGroup mcg = cg;
  QColor c;

  switch ( col )
  {
    case ContextName:
    {
      mcg.setColor(QColorGroup::Text, is->textColor());
      mcg.setColor(QColorGroup::HighlightedText, is->selectedTextColor());
      QFont f ( ((StyleListView*)lv)->docfont );
      p->setFont( is->font(f) );
      // FIXME - repainting when text is cropped, and the column is enlarged is buggy.
      // Maybe I need painting the string myself :(
      QListViewItem::paintCell( p, mcg, col, width, align );
    }
    break;
    case Bold:
    case Italic:
    case Underline:
    case Strikeout:
    case UseDefStyle:
    {
      // Bold/Italic/use default checkboxes
      // code allmost identical to QCheckListItem
      // I use the text color of defaultStyles[0], normalcol in parent listview
      mcg.setColor( QColorGroup::Text, ((StyleListView*)lv)->normalcol );
      int x = 0;
      if ( align == AlignCenter ) {
        QFontMetrics fm( lv->font() );
        x = (width - BoxSize - fm.width(text(0)))/2;
      }
      int y = (height() - BoxSize) / 2;

      if ( isEnabled() )
        p->setPen( QPen( mcg.text(), 2 ) );
      else
        p->setPen( QPen( lv->palette().color( QPalette::Disabled, QColorGroup::Text ), 2 ) );

      if ( isSelected() && lv->header()->mapToSection( 0 ) != 0 )
      {
        p->fillRect( 0, 0, x + marg + BoxSize + 4, height(),
              mcg.brush( QColorGroup::Highlight ) );
        if ( isEnabled() )
          p->setPen( QPen( mcg.highlightedText(), 2 ) ); // FIXME! - use defaultstyles[0].selecol. luckily not used :)
      }
      p->drawRect( x+marg, y+2, BoxSize-4, BoxSize-4 );
      x++;
      y++;
      if ( (col == Bold && is->bold()) ||
          (col == Italic && is->italic()) ||
          (col == Underline && is->underline()) ||
          (col == Strikeout && is->strikeOut()) ||
          (col == UseDefStyle && *is == *ds ) )
      {
        QPointArray a( 7*2 );
        int i, xx, yy;
        xx = x+1+marg;
        yy = y+5;
        for ( i=0; i<3; i++ ) {
          a.setPoint( 2*i,   xx, yy );
          a.setPoint( 2*i+1, xx, yy+2 );
          xx++; yy++;
        }
        yy -= 2;
        for ( i=3; i<7; i++ ) {
          a.setPoint( 2*i,   xx, yy );
          a.setPoint( 2*i+1, xx, yy+2 );
          xx++; yy--;
        }
        p->drawLineSegments( a );
      }
    }
    break;
    case Color:
    case SelColor:
    case BgColor:
    case SelBgColor:
    {
      if ( col == Color) c = is->textColor();
      else if ( col == SelColor ) c = is->selectedTextColor();
      else if ( col == BgColor ) c = is->itemSet(KateAttribute::BGColor) ? is->bgColor() : ((StyleListView*)lv)->bgcol;
      else if ( col == SelBgColor ) c = is->itemSet(KateAttribute::SelectedBGColor) ? is->selectedBGColor(): ((StyleListView*)lv)->bgcol;
      // color "buttons"
      mcg.setColor( QColorGroup::Text, ((StyleListView*)lv)->normalcol );
      int x = 0;
      int y = (height() - BoxSize) / 2;
      if ( isEnabled() )
        p->setPen( QPen( mcg.text(), 2 ) );
      else
        p->setPen( QPen( lv->palette().color( QPalette::Disabled, QColorGroup::Text ), 2 ) );

      p->drawRect( x+marg, y+2, ColorBtnWidth-4, BoxSize-4 );
      p->fillRect( x+marg+1,y+3,ColorBtnWidth-7,BoxSize-7,QBrush( c ) );
    }
    //case default: // no warning...
  }
}
//END

//BEGIN KMimeTypeChooser
/*********************************************************************/
/*               KMimeTypeChooser Implementation                     */
/*********************************************************************/
KMimeTypeChooser::KMimeTypeChooser( QWidget *parent, const QString &text, const QStringList &selectedMimeTypes, bool editbutton, bool showcomment, bool showpatterns)
    : QVBox( parent )
{
  setSpacing( KDialogBase::spacingHint() );

/* HATE!!!! geometry management seems BADLY broken :(((((((((((
   Problem: if richtext is used (or Qt::WordBreak is on in the label),
   the list view is NOT resized when the parent box is.
   No richtext :(((( */
  if ( !text.isEmpty() ) {
    new QLabel( text, this );
  }

  lvMimeTypes = new QListView( this );
  lvMimeTypes->addColumn( i18n("Mime Type") );
  if ( showcomment )
    lvMimeTypes->addColumn( i18n("Comment") );
  if ( showpatterns )
    lvMimeTypes->addColumn( i18n("Patterns") );
  lvMimeTypes->setRootIsDecorated( true );

  //lvMimeTypes->clear(); WHY?!
  QMap<QString,QListViewItem*> groups;
  // thanks to kdebase/kcontrol/filetypes/filetypesview
  KMimeType::List mimetypes = KMimeType::allMimeTypes();
  QValueListIterator<KMimeType::Ptr> it(mimetypes.begin());

  QListViewItem *groupItem;
  bool agroupisopen = false;
  QListViewItem *idefault = 0; //open this, if all other fails
  for (; it != mimetypes.end(); ++it) {
    QString mimetype = (*it)->name();
    int index = mimetype.find("/");
    QString maj = mimetype.left(index);
    QString min = mimetype.right(mimetype.length() - (index+1));

    QMapIterator<QString,QListViewItem*> mit = groups.find( maj );
    if ( mit == groups.end() ) {
        groupItem = new QListViewItem( lvMimeTypes, maj );
        groups.insert( maj, groupItem );
        if (maj == "text")
          idefault = groupItem;
    }
    else
        groupItem = mit.data();

    QCheckListItem *item = new QCheckListItem( groupItem, min, QCheckListItem::CheckBox );
    item->setPixmap( 0, SmallIcon( (*it)->icon(QString::null,false) ) );
    int cl = 1;
    if ( showcomment ) {
      item->setText( cl, (*it)->comment(QString::null, false) );
      cl++;
    }
    if ( showpatterns )
      item->setText( cl, (*it)->patterns().join("; ") );
    if ( selectedMimeTypes.contains(mimetype) ) {
      item->setOn( true );
      groupItem->setOpen( true );
      agroupisopen = true;
      lvMimeTypes->ensureItemVisible( item );// actually, i should do this for the first item only.
    }
  }

  if (! agroupisopen)
    idefault->setOpen( true );

  if (editbutton) {
    QHBox *btns = new QHBox( this );
    ((QBoxLayout*)btns->layout())->addStretch(1); // hmm.
    btnEditMimeType = new QPushButton( i18n("&Edit..."), btns );
    connect( btnEditMimeType, SIGNAL(clicked()), this, SLOT(editMimeType()) );
    btnEditMimeType->setEnabled( false );
    connect( lvMimeTypes, SIGNAL( doubleClicked ( QListViewItem * )), this, SLOT( editMimeType()));
    connect( lvMimeTypes, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(slotCurrentChanged(QListViewItem*)) );
    QWhatsThis::add( btnEditMimeType, i18n("Click this button to display the familiar KDE File Type Editor.<p><strong>Warning:</strong> if you change the file extensions, you need to restart this dialog, as it will not be aware that they have changed.") );
  }
}

void KMimeTypeChooser::editMimeType()
{
  if ( !(lvMimeTypes->currentItem() && (lvMimeTypes->currentItem())->parent()) ) return;
  QString mt = (lvMimeTypes->currentItem()->parent())->text( 0 ) + "/" + (lvMimeTypes->currentItem())->text( 0 );
  // thanks to libkonq/konq_operations.cc
  QString keditfiletype = QString::fromLatin1("keditfiletype");
  KRun::runCommand( keditfiletype + " " + KProcess::quote(mt),
                    keditfiletype, keditfiletype /*unused*/);
}

void KMimeTypeChooser::slotCurrentChanged(QListViewItem* i)
{
  btnEditMimeType->setEnabled( i->parent() );
}

QStringList KMimeTypeChooser::selectedMimeTypesStringList()
{
  QStringList l;
  QListViewItemIterator it( lvMimeTypes );
  for (; it.current(); ++it) {
    if ( it.current()->parent() && ((QCheckListItem*)it.current())->isOn() )
      l << it.current()->parent()->text(0) + "/" + it.current()->text(0); // FIXME: uncecked, should be Ok unless someone changes mimetypes during this!
  }
  return l;
}

QStringList KMimeTypeChooser::patterns()
{
  QStringList l;
  KMimeType::Ptr p;
  QString defMT = KMimeType::defaultMimeType();
  QListViewItemIterator it( lvMimeTypes );
  for (; it.current(); ++it) {
    if ( it.current()->parent() && ((QCheckListItem*)it.current())->isOn() ) {
      p = KMimeType::mimeType( it.current()->parent()->text(0) + "/" + it.current()->text(0) );
      if ( p->name() != defMT )
        l += p->patterns();
    }
  }
  return l;
}
//END

//BEGIN KMimeTypeChooserDlg
/*********************************************************************/
/*               KMimeTypeChooserDlg Implementation                  */
/*********************************************************************/
KMimeTypeChooserDlg::KMimeTypeChooserDlg(QWidget *parent,
                         const QString &caption, const QString& text,
                         const QStringList &selectedMimeTypes,
                         bool editbutton, bool showcomment, bool showpatterns )
    : KDialogBase(parent, 0, true, caption, Cancel|Ok, Ok)
{
  KConfig *config = kapp->config();

  chooser = new KMimeTypeChooser( this, text, selectedMimeTypes, editbutton, showcomment, showpatterns);
  setMainWidget(chooser);

  config->setGroup("KMimeTypeChooserDlg");
  resize( config->readSizeEntry("size", new QSize(400,300)) );
}

KMimeTypeChooserDlg::~KMimeTypeChooserDlg()
{
  KConfig *config = kapp->config();
  config->setGroup("KMimeTypeChooserDlg");
  config->writeEntry("size", size());
}

QStringList KMimeTypeChooserDlg::mimeTypes()
{
  return chooser->selectedMimeTypesStringList();
}

QStringList KMimeTypeChooserDlg::patterns()
{
  return chooser->patterns();
}

SpellConfigPage::SpellConfigPage( QWidget* parent )
  : Kate::ConfigPage( parent)
{
  QVBoxLayout* l = new QVBoxLayout( this );
  cPage = new KSpellConfig( this, 0L, 0L, false );
  l->addWidget( cPage );
  connect( cPage, SIGNAL( configChanged() ), this, SLOT( slotChanged() ) );
}

void SpellConfigPage::apply ()
{
  // kspell
  cPage->writeGlobalSettings ();
}
//END

// kate: space-indent on; indent-width 2; replace-tabs on;
