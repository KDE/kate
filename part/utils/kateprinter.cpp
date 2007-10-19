/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001-2002 Michael Goffioul <kdeprint@swing.be>
 *  Complete rewrite on Sat Jun 15 2002 (c) Anders Lund <anders@alweb.dk>
 *  Copyright (c) 2002, 2003 Anders Lund <anders@alweb.dk>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "kateprinter.h"

#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katetextlayout.h"
#include "katerenderer.h"
#include "kateschema.h"
#include "katetextline.h"
#include "kateview.h"

#include <kapplication.h>
#include <kcolorbutton.h>
#include <kdebug.h>
#include <kdialog.h> // for spacingHint()
#include <kfontdialog.h>
#include <klocale.h>
#include <kdeprintdialog.h>
#include <kurl.h>
#include <kuser.h> // for loginName

#include <QtGui/QPainter>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrinter>

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QSpinBox>
#include <QtCore/QStringList>
#include <kvbox.h>

//BEGIN KatePrinter
bool KatePrinter::print (KateDocument *doc)
{

  QPrinter printer;

  // docname is now always there, including the right Untitled name
  printer.setDocName(doc->documentName());

  KatePrintTextSettings *kpts = new KatePrintTextSettings;
  KatePrintHeaderFooter *kphf = new KatePrintHeaderFooter;
  KatePrintLayout *kpl = new KatePrintLayout;

  QList<QWidget*> tabs;
  tabs << kpts;
  tabs << kphf;
  tabs << kpl;

  QPrintDialog *printDialog = KdePrint::createPrintDialog(&printer, tabs, doc->widget());

  if ( doc->activeView()->selection() )
    printDialog->addEnabledOption(QAbstractPrintDialog::PrintSelection);

  if ( printDialog->exec() )
  {
    KateRenderer renderer(doc, doc->activeKateView());
    //renderer.config()->setSchema (1);
    renderer.setPrinterFriendly(true);

    QPainter paint( &printer );
    /*
     *        We work in tree cycles:
     *        1) initialize variables and retrieve print settings
     *        2) prepare data according to those settings
     *        3) draw to the printer
     */
    uint pdmWidth = printer.width();
    uint pdmHeight = printer.height();
    uint y = 0;
    uint xstart = 0; // beginning point for painting lines
    uint lineCount = 0;
    uint maxWidth = pdmWidth;
    uint headerWidth = pdmWidth;
    int startCol = 0;
    int endCol = 0;
    bool pageStarted = true;
    int remainder = 0; // remaining sublines from a wrapped line (for the top of a new page)

    // Text Settings Page
    bool selectionOnly = (printDialog->printRange() == QAbstractPrintDialog::Selection);

    int selStartCol = 0;
    int selEndCol = 0;

    bool useGuide = kpts->printGuide();
    int guideHeight = 0;
    int guideCols = 0;

    bool printLineNumbers = kpts->printLineNumbers();
    uint lineNumberWidth( 0 );

    // Header/Footer Page
    QFont headerFont(kphf->font()); // used for header/footer

    bool useHeader = kphf->useHeader();
    QColor headerBgColor(kphf->headerBackground());
    QColor headerFgColor(kphf->headerForeground());
    uint headerHeight( 0 ); // further init only if needed
    QStringList headerTagList; // do
    bool headerDrawBg = false; // do

    bool useFooter = kphf->useFooter();
    QColor footerBgColor(kphf->footerBackground());
    QColor footerFgColor(kphf->footerForeground());
    uint footerHeight( 0 ); // further init only if needed
    QStringList footerTagList; // do
    bool footerDrawBg = false; // do

    // Layout Page
    renderer.config()->setSchema( kpl->colorScheme() );
    bool useBackground = kpl->useBackground();
    bool useBox = kpl->useBox();
    int boxWidth(kpl->boxWidth());
    QColor boxColor(kpl->boxColor());
    int innerMargin = useBox ? kpl->boxMargin() : 6;

    // Post initialization
    uint maxHeight = (useBox ? pdmHeight-innerMargin : pdmHeight);
    uint currentPage( 1 );
    uint lastline = doc->lastLine(); // necessary to print selection only
    uint firstline( 0 );
    int fontHeight = renderer.fontHeight();
    KTextEditor::Range selectionRange;

    QList<KateExtendedAttribute::Ptr> ilist;

    if (useGuide)
      doc->highlight()->getKateExtendedAttributeListCopy (renderer.config()->schema(), ilist);

    /*
    *        Now on for preparations...
    *        during preparations, variable names starting with a "_" means
    *        those variables are local to the enclosing block.
    */
    {
      if ( selectionOnly )
      {
        // set a line range from the first selected line to the last
        selectionRange = doc->activeView()->selectionRange();
        firstline = selectionRange.start().line();
        lastline = selectionRange.end().line();
        lineCount = firstline;
      }

      if ( printLineNumbers )
      {
        // figure out the horiizontal space required
        QString s( QString("%1 ").arg( doc->lines() ) );
        s.fill('5', -1); // some non-fixed fonts haven't equally wide numbers
        // FIXME calculate which is actually the widest...
        lineNumberWidth = renderer.currentFontMetrics().width( s );
        // a small space between the line numbers and the text
        int _adj = renderer.currentFontMetrics().width( "5" );
        // adjust available width and set horizontal start point for data
        maxWidth -= (lineNumberWidth + _adj);
        xstart += lineNumberWidth + _adj;
      }

      if ( useHeader || useFooter )
      {
        // Set up a tag map
        // This retrieves all tags, ued or not, but
        // none of theese operations should be expensive,
        // and searcing each tag in the format strings is avoided.
        QDateTime dt = QDateTime::currentDateTime();
        QMap<QString,QString> tags;

        KUser u (KUser::UseRealUserID);
        tags["u"] = u.loginName();

        tags["d"] = KGlobal::locale()->formatDateTime(dt, KLocale::ShortDate);
        tags["D"] =  KGlobal::locale()->formatDateTime(dt, KLocale::LongDate);
        tags["h"] =  KGlobal::locale()->formatTime(dt.time(), false);
        tags["y"] =  KGlobal::locale()->formatDate(dt.date(), KLocale::ShortDate);
        tags["Y"] =  KGlobal::locale()->formatDate(dt.date(), KLocale::LongDate);
        tags["f"] =  doc->url().fileName();
        tags["U"] =  doc->url().prettyUrl();
        if ( selectionOnly )
        {
          QString s( i18n("(Selection of) ") );
          tags["f"].prepend( s );
          tags["U"].prepend( s );
        }

        QRegExp reTags( "%([dDfUhuyY])" ); // TODO tjeck for "%%<TAG>"

        if (useHeader)
        {
          headerDrawBg = kphf->useHeaderBackground();
          headerHeight = QFontMetrics( headerFont ).height();
          if ( useBox || headerDrawBg )
            headerHeight += innerMargin * 2;
          else
            headerHeight += 1 + QFontMetrics( headerFont ).leading();

          QString headerTags = kphf->headerFormat();
          int pos = reTags.indexIn( headerTags );
          QString rep;
          while ( pos > -1 )
          {
            rep = tags[reTags.cap( 1 )];
            headerTags.replace( (uint)pos, 2, rep );
            pos += rep.length();
            pos = reTags.indexIn( headerTags, pos );
          }
          headerTagList = headerTags.split('|');

          if (!headerBgColor.isValid())
            headerBgColor = Qt::lightGray;
          if (!headerFgColor.isValid())
            headerFgColor = Qt::black;
        }

        if (useFooter)
        {
          footerDrawBg = kphf->useFooterBackground();
          footerHeight = QFontMetrics( headerFont ).height();
          if ( useBox || footerDrawBg )
            footerHeight += 2*innerMargin;
          else
            footerHeight += 1; // line only

            QString footerTags = kphf->footerFormat();
          int pos = reTags.indexIn( footerTags );
          QString rep;
          while ( pos > -1 )
          {
            rep = tags[reTags.cap( 1 )];
            footerTags.replace( (uint)pos, 2, rep );
            pos += rep.length();
            pos = reTags.indexIn( footerTags, pos );
          }

          footerTagList = footerTags.split('|');
          if (!footerBgColor.isValid())
            footerBgColor = Qt::lightGray;
          if (!footerFgColor.isValid())
            footerFgColor = Qt::black;
          // adjust maxheight, so we can know when/where to print footer
          maxHeight -= footerHeight;
        }
      } // if ( useHeader || useFooter )

      if ( useBackground )
      {
        if ( ! useBox )
        {
          xstart += innerMargin;
          maxWidth -= innerMargin * 2;
        }
      }

      if ( useBox )
      {
        if (!boxColor.isValid())
          boxColor = Qt::black;
        if (boxWidth < 1) // shouldn't be pssible no more!
          boxWidth = 1;
        // set maxwidth to something sensible
        maxWidth -= ( ( boxWidth + innerMargin )  * 2 );
        xstart += boxWidth + innerMargin;
        // maxheight too..
        maxHeight -= boxWidth;
      }
      else
        boxWidth = 0;

  #if 0
      if ( useGuide )
      {
        // calculate the height required
        // the number of columns is a side effect, saved for drawing time
        // first width is needed
        int _w = pdmWidth - innerMargin * 2;
        if ( useBox )
          _w -= boxWidth * 2;
        else
        {
          if ( useBackground )
            _w -= ( innerMargin * 2 );
          _w -= 2; // 1 px line on each side
        }

        // base of height: margins top/bottom, above and below tetle sep line
        guideHeight = ( innerMargin * 4 ) + 1;

        // get a title and add the height required to draw it
        QString _title = i18n("Typographical Conventions for %1", doc->highlight()->name());
        guideHeight += paint.boundingRect( 0, 0, _w, 1000, Qt::AlignTop|Qt::AlignHCenter, _title ).height();

        // see how many columns we can fit in
        int _widest( 0 );

        int _items ( 0 );
        for (int i = 0; i < ilist.size(); ++i)
        {
          KateExtendedAttribute::Ptr _d = ilist[i];
          _widest = qMax( _widest, ((QFontMetrics)(
          _d->fontWeight() == QFont::Bold ?
          _d->fontItalic() ?
          renderer.config()->fontStruct()->myFontMetricsBI :
          renderer.config()->fontStruct()->myFontMetricsBold :
          _d->fontItalic() ?
          renderer.config()->fontStruct()->myFontMetricsItalic :
          renderer.config()->fontStruct()->myFontMetrics
          ) ).width( _d->name() ) );
          _items++;
        }
        guideCols = _w/( _widest + innerMargin );
        // add height for required number of lines needed given columns
        guideHeight += fontHeight * ( _items/guideCols );
        if ( _items%guideCols )
          guideHeight += fontHeight;
      }
  #endif

      // now that we know the vertical amount of space needed,
      // it is possible to calculate the total number of pages
      // if needed, that is if any header/footer tag contains "%P".
      if ( !headerTagList.filter("%P").isEmpty() || !footerTagList.filter("%P").isEmpty() )
      {
        kDebug(13020)<<"'%P' found! calculating number of pages...";
        uint _pages = 0;
        uint _ph = maxHeight;
        if ( useHeader )
          _ph -= ( headerHeight + innerMargin );
        if ( useFooter )
          _ph -= innerMargin;
        int _lpp = _ph / fontHeight;
        uint _lt = 0, _c=0;

        // add space for guide if required
        if ( useGuide )
          _lt += (guideHeight + (fontHeight /2)) / fontHeight;
        long _lw;
        for ( uint i = firstline; i < lastline; i++ )
        {
          //FIXME: _lw = renderer.textWidth( doc->kateTextLine( i ), -1 );
          _lw = 80 * renderer.spaceWidth(); //FIXME: just a stand-in
          while ( _lw >= 0 )
          {
            _c++;
            _lt++;
            if ( (int)_lt  == _lpp )
            {
              _pages++;
              _lt = 0;
            }
            _lw -= maxWidth;
            if ( ! _lw ) _lw--; // skip lines matching exactly!
          }
        }
        if ( _lt ) _pages++; // last page

        // substitute both tag lists
        QString re("%P");
        QStringList::Iterator it;
        for ( it=headerTagList.begin(); it!=headerTagList.end(); ++it )
          (*it).replace( re, QString( "%1" ).arg( _pages ) );
        for ( it=footerTagList.begin(); it!=footerTagList.end(); ++it )
          (*it).replace( re, QString( "%1" ).arg( _pages ) );
      }
    } // end prepare block

     /*
        On to draw something :-)
     */
    while (  lineCount <= lastline  )
    {
      startCol = 0;
      endCol = 0;

      if ( y + fontHeight >= (uint)(maxHeight) )
      {
        kDebug(13020)<<"Starting new page,"<<lineCount<<"lines up to now.";
        printer.newPage();
        paint.resetTransform();
        currentPage++;
        pageStarted = true;
        y=0;
      }

      if ( pageStarted )
      {

        if ( useHeader )
        {
          paint.setPen(headerFgColor);
          paint.setFont(headerFont);
          if ( headerDrawBg )
            paint.fillRect(0, 0, headerWidth, headerHeight, headerBgColor);
          if (headerTagList.count() == 3)
          {
            int valign = ( (useBox||headerDrawBg||useBackground) ?
            Qt::AlignVCenter : Qt::AlignTop );
            int align = valign|Qt::AlignLeft;
            int marg = ( useBox || headerDrawBg ) ? innerMargin : 0;
            if ( useBox ) marg += boxWidth;
            QString s;
            for (int i=0; i<3; i++)
            {
              s = headerTagList[i];
              if (s.indexOf("%p") != -1) s.replace("%p", QString::number(currentPage));
              paint.drawText(marg, 0, headerWidth-(marg*2), headerHeight, align, s);
              align = valign|(i == 0 ? Qt::AlignHCenter : Qt::AlignRight);
            }
          }
          if ( ! ( headerDrawBg || useBox || useBackground ) ) // draw a 1 px (!?) line to separate header from contents
          {
            paint.drawLine( 0, headerHeight-1, headerWidth, headerHeight-1 );
            //y += 1; now included in headerHeight
          }
          y += headerHeight + innerMargin;
        }

        if ( useFooter )
        {
          if ( ! ( footerDrawBg || useBox || useBackground ) ) // draw a 1 px (!?) line to separate footer from contents
            paint.drawLine( 0, maxHeight + innerMargin - 1, headerWidth, maxHeight + innerMargin - 1 );
          if ( footerDrawBg )
            paint.fillRect(0, maxHeight+innerMargin+boxWidth, headerWidth, footerHeight, footerBgColor);
          if (footerTagList.count() == 3)
          {
            int align = Qt::AlignVCenter|Qt::AlignLeft;
            int marg = ( useBox || footerDrawBg ) ? innerMargin : 0;
            if ( useBox ) marg += boxWidth;
            QString s;
            for (int i=0; i<3; i++)
            {
              s = footerTagList[i];
              if (s.indexOf("%p") != -1) s.replace("%p", QString::number(currentPage));
              paint.drawText(marg, maxHeight+innerMargin, headerWidth-(marg*2), footerHeight, align, s);
              align = Qt::AlignVCenter|(i == 0 ? Qt::AlignHCenter : Qt::AlignRight);
            }
          }
        } // done footer

        if ( useBackground )
        {
          // If we have a box, or the header/footer has backgrounds, we want to paint
          // to the border of those. Otherwise just the contents area.
          int _y = y, _h = maxHeight - y;
          if ( useBox )
          {
            _y -= innerMargin;
            _h += 2 * innerMargin;
          }
          else
          {
            if ( headerDrawBg )
            {
              _y -= innerMargin;
              _h += innerMargin;
            }
            if ( footerDrawBg )
            {
              _h += innerMargin;
            }
          }
          paint.fillRect( 0, _y, pdmWidth, _h, renderer.config()->backgroundColor());
        }

        if ( useBox )
        {
          paint.setPen(QPen(boxColor, boxWidth));
          paint.drawRect(0, 0, pdmWidth, pdmHeight);
          if (useHeader)
            paint.drawLine(0, headerHeight, headerWidth, headerHeight);
          else
            y += innerMargin;

          if ( useFooter ) // drawline is not trustable, grr.
            paint.fillRect( 0, maxHeight+innerMargin, headerWidth, boxWidth, boxColor );
        }

#if 0
        if ( useGuide && currentPage == 1 )
        {  // FIXME - this may span more pages...
          // draw a box unless we have boxes, in which case we end with a box line

          // use color of dsNormal for the title string and the hline
          KateAttributeList _dsList;
          KateHlManager::self()->getDefaults ( renderer.config()->schema(), _dsList );
          paint.setBrush ( _dsList.at(0)->foreground() );
          int _marg = 0; // this could be available globally!??
          if ( useBox )
          {
            _marg += (2*boxWidth) + (2*innerMargin);
            paint.fillRect( 0, y+guideHeight-innerMargin-boxWidth, headerWidth, boxWidth, boxColor );
          }
          else
          {
            if ( useBackground )
              _marg += 2*innerMargin;
            paint.drawRect( _marg, y, pdmWidth-(2*_marg), guideHeight );
            _marg += 1;
            y += 1 + innerMargin;
          }
          // draw a title string
          paint.setFont( renderer.config()->fontStruct()->myFontBold );
          QRect _r;
          paint.drawText( _marg, y, pdmWidth-(2*_marg), maxHeight - y,
            Qt::AlignTop|Qt::AlignHCenter,
            i18n("Typographical Conventions for %1", doc->highlight()->name()), -1, &_r );
          int _w = pdmWidth - (_marg*2) - (innerMargin*2);
          int _x = _marg + innerMargin;
          y += _r.height() + innerMargin;
          paint.drawLine( _x, y, _x + _w, y );
          y += 1 + innerMargin;
          // draw attrib names using their styles

          QListIterator<KateExtendedAttribute> _it( ilist );
          KateExtendedAttribute::Ptr _d;
          int _cw = _w/guideCols;
          int _i(0);

          while ( ( _d = _it.current() ) != 0 )
          {
            paint.setPen( renderer.attribute(_i)->foreground() );
            paint.setFont( renderer.attribute(_i)->font( *renderer.currentFont() ) );
            paint.drawText(( _x + ((_i%guideCols)*_cw)), y, _cw, fontHeight,
                    Qt::AlignVCenter|Qt::AlignLeft, _d->name, -1, &_r );
            _i++;
            if ( _i && ! ( _i%guideCols ) ) y += fontHeight;
            ++_it;
          }
          if ( _i%guideCols ) y += fontHeight;// last row not full
          y += ( useBox ? boxWidth : 1 ) + (innerMargin*2);
        }
#endif
        paint.translate(xstart,y);
        pageStarted = false;
      } // pageStarted; move on to contents:)

      if ( printLineNumbers /*&& ! startCol*/ ) // don't repeat!
      {
        paint.setFont( renderer.config()->font() );
        paint.setPen( renderer.config()->lineNumberColor() );
        paint.drawText( (( useBox || useBackground ) ? innerMargin : 0)-xstart, 0,
                    lineNumberWidth, fontHeight,
                    Qt::AlignRight, QString("%1").arg( lineCount + 1 ) );
      }

      // HA! this is where we print [part of] a line ;]]
      // FIXME Convert this function + related functionality to a separate KatePrintView
      KateLineLayout range(doc);
      range.setLine(lineCount);
      KateLineLayoutPtr *rangeptr = new KateLineLayoutPtr(&range);
      renderer.layoutLine(*rangeptr, (int)maxWidth, false);

      // selectionOnly: clip non-selection parts and adjust painter position if needed
      int _xadjust = 0;
      if (selectionOnly) {
        if (doc->activeView()->blockSelection()) {
          int _x = renderer.cursorToX((*rangeptr)->viewLine(0), selectionRange.start());
          int _x1 = renderer.cursorToX((*rangeptr)->viewLine((*rangeptr)->viewLineCount()-1), selectionRange.end());
           _xadjust = _x;
           paint.translate(-_xadjust, 0);
          paint.setClipRegion(QRegion( _x, 0, _x1 - _x, (*rangeptr)->viewLineCount()*fontHeight));
        }

        else if (lineCount == firstline || lineCount == lastline) {
          QRegion region(0, 0, maxWidth, (*rangeptr)->viewLineCount()*fontHeight);

          if ( lineCount == firstline) {
            region = region.subtracted(QRegion(0, 0, renderer.cursorToX((*rangeptr)->viewLine(0), selectionRange.start()), fontHeight));
          }

          if (lineCount == lastline) {
            int _x = renderer.cursorToX((*rangeptr)->viewLine((*rangeptr)->viewLineCount()-1), selectionRange.end());
            region = region.subtracted(QRegion(_x, 0, maxWidth-_x, fontHeight));
          }

          paint.setClipRegion(region);
        }
      }

      // If the line is too long (too many 'viewlines') to fit the remaining vertical space,
      // clip and adjust the painter position as nessecary
      int _lines = (*rangeptr)->viewLineCount()-remainder; // number of "sublines" to paint.
      int _yadjust = remainder * fontHeight; // if we need to clip at the start of the line, it's this much.
      bool _needWrap = (fontHeight*_lines > maxHeight-y);

      if (remainder || _needWrap) {
        remainder = _needWrap? _lines - ((maxHeight-y)/fontHeight) : 0;
        paint.translate(0, -_yadjust);
        paint.setClipRect(0,_yadjust,maxWidth,((_lines-remainder)*fontHeight)-1); //### drop the crosspatch in printerfriendly mode???
      }

      renderer.paintTextLine(paint, *rangeptr, 0, (int)maxWidth);

      paint.setClipping(false);
      paint.translate(_xadjust, (fontHeight * _lines)+_yadjust);

      y += fontHeight*_lines;

      if ( ! remainder )
      lineCount++;
    } // done lineCount <= lastline

    paint.end();
    return true;
  }
  return false;
}
//END KatePrinter

//BEGIN KatePrintTextSettings
KatePrintTextSettings::KatePrintTextSettings( QWidget *parent )
  : QWidget( parent )
{
  setWindowTitle( i18n("Te&xt Settings") );

  QVBoxLayout *lo = new QVBoxLayout ( this );
  lo->setSpacing( KDialog::spacingHint() );

//   cbSelection = new QCheckBox( i18n("Print &selected text only"), this );
//   lo->addWidget( cbSelection );

  cbLineNumbers = new QCheckBox( i18n("Print &line numbers"), this );
  lo->addWidget( cbLineNumbers );

  cbGuide = new QCheckBox( i18n("Print syntax &guide"), this );
  lo->addWidget( cbGuide );

  lo->addStretch( 1 );

  // set defaults - nothing to do :-)

  // whatsthis
//   cbSelection->setWhatsThis(i18n(
//         "<p>This option is only available if some text is selected in the document.</p>"
//         "<p>If available and enabled, only the selected text is printed.</p>") );
  cbLineNumbers->setWhatsThis(i18n(
        "<p>If enabled, line numbers will be printed on the left side of the page(s).</p>") );
  cbGuide->setWhatsThis(i18n(
        "<p>Print a box displaying typographical conventions for the document type, as "
        "defined by the syntax highlighting being used.</p>") );
}

// bool KatePrintTextSettings::printSelection()
// {
//     return cbSelection->isChecked();
// }

bool KatePrintTextSettings::printLineNumbers()
{
  return cbLineNumbers->isChecked();
}

bool KatePrintTextSettings::printGuide()
{
  return cbGuide->isChecked();
}

// void KatePrintTextSettings::enableSelection( bool enable )
// {
//   cbSelection->setEnabled( enable );
// }

//END KatePrintTextSettings

//BEGIN KatePrintHeaderFooter
KatePrintHeaderFooter::KatePrintHeaderFooter( QWidget *parent )
  : QWidget( parent )
{
  setWindowTitle( i18n("Hea&der && Footer") );

  QVBoxLayout *lo = new QVBoxLayout ( this );
  uint sp = KDialog::spacingHint();
  lo->setSpacing( sp );

  // enable
  QHBoxLayout *lo1 = new QHBoxLayout ();
  lo->addLayout( lo1 );
  cbEnableHeader = new QCheckBox( i18n("Pr&int header"), this );
  lo1->addWidget( cbEnableHeader );
  cbEnableFooter = new QCheckBox( i18n("Pri&nt footer"), this );
  lo1->addWidget( cbEnableFooter );

  // font
  QHBoxLayout *lo2 = new QHBoxLayout();
  lo->addLayout( lo2 );
  lo2->addWidget( new QLabel( i18n("Header/footer font:"), this ) );
  lFontPreview = new QLabel( this );
  lFontPreview->setFrameStyle( QFrame::Panel|QFrame::Sunken );
  lo2->addWidget( lFontPreview );
  lo2->setStretchFactor( lFontPreview, 1 );
  QPushButton *btnChooseFont = new QPushButton( i18n("Choo&se Font..."), this );
  lo2->addWidget( btnChooseFont );
  connect( btnChooseFont, SIGNAL(clicked()), this, SLOT(setHFFont()) );

  // header
  gbHeader = new QGroupBox( this );
  gbHeader->setTitle(i18n("Header Properties"));
  QGridLayout* grid = new QGridLayout(gbHeader);
  lo->addWidget( gbHeader );

  QLabel *lHeaderFormat = new QLabel( i18n("&Format:"), gbHeader );
  grid->addWidget(lHeaderFormat, 0, 0);

  KHBox *hbHeaderFormat = new KHBox( gbHeader );
  grid->addWidget(hbHeaderFormat, 0, 1);

  hbHeaderFormat->setSpacing( sp );
  leHeaderLeft = new QLineEdit( hbHeaderFormat );
  leHeaderCenter = new QLineEdit( hbHeaderFormat );
  leHeaderRight = new QLineEdit( hbHeaderFormat );
  lHeaderFormat->setBuddy( leHeaderLeft );

  grid->addWidget(new QLabel( i18n("Colors:"), gbHeader ), 1, 0);

  KHBox *hbHeaderColors = new KHBox( gbHeader );
  grid->addWidget(hbHeaderColors, 1, 1);

  hbHeaderColors->setSpacing( sp );
  QLabel *lHeaderFgCol = new QLabel( i18n("Foreground:"), hbHeaderColors );
  kcbtnHeaderFg = new KColorButton( hbHeaderColors );
  lHeaderFgCol->setBuddy( kcbtnHeaderFg );
  cbHeaderEnableBgColor = new QCheckBox( i18n("Bac&kground"), hbHeaderColors );
  kcbtnHeaderBg = new KColorButton( hbHeaderColors );

  gbFooter = new QGroupBox( this );
  gbFooter->setTitle(i18n("Footer Properties"));
  grid = new QGridLayout(gbFooter);
  lo->addWidget( gbFooter );

  // footer
  QLabel *lFooterFormat = new QLabel( i18n("For&mat:"), gbFooter );
  grid->addWidget(lFooterFormat, 0, 0);

  KHBox *hbFooterFormat = new KHBox( gbFooter );
  grid->addWidget(hbFooterFormat, 0, 1);

  hbFooterFormat->setSpacing( sp );
  leFooterLeft = new QLineEdit( hbFooterFormat );
  leFooterCenter = new QLineEdit( hbFooterFormat );
  leFooterRight = new QLineEdit( hbFooterFormat );
  lFooterFormat->setBuddy( leFooterLeft );

  grid->addWidget(new QLabel( i18n("Colors:"), gbFooter ), 1, 0);

  KHBox *hbFooterColors = new KHBox( gbFooter );
  grid->addWidget(hbFooterColors, 1, 1);
  hbFooterColors->setSpacing( sp );

  QLabel *lFooterBgCol = new QLabel( i18n("Foreground:"), hbFooterColors );
  kcbtnFooterFg = new KColorButton( hbFooterColors );
  lFooterBgCol->setBuddy( kcbtnFooterFg );
  cbFooterEnableBgColor = new QCheckBox( i18n("&Background"), hbFooterColors );
  kcbtnFooterBg = new KColorButton( hbFooterColors );

  lo->addStretch( 1 );

  // user friendly
  connect( cbEnableHeader, SIGNAL(toggled(bool)), gbHeader, SLOT(setEnabled(bool)) );
  connect( cbEnableFooter, SIGNAL(toggled(bool)), gbFooter, SLOT(setEnabled(bool)) );
  connect( cbHeaderEnableBgColor, SIGNAL(toggled(bool)), kcbtnHeaderBg, SLOT(setEnabled(bool)) );
  connect( cbFooterEnableBgColor, SIGNAL(toggled(bool)), kcbtnFooterBg, SLOT(setEnabled(bool)) );

  // set defaults
  cbEnableHeader->setChecked( true );
  leHeaderLeft->setText( "%y" );
  leHeaderCenter->setText( "%f" );
  leHeaderRight->setText( "%p" );
  kcbtnHeaderFg->setColor( QColor("black") );
  cbHeaderEnableBgColor->setChecked( true );
  kcbtnHeaderBg->setColor( QColor("lightgrey") );

  cbEnableFooter->setChecked( true );
  leFooterRight->setText( "%U" );
  kcbtnFooterFg->setColor( QColor("black") );
  cbFooterEnableBgColor->setChecked( true );
  kcbtnFooterBg->setColor( QColor("lightgrey") );

  // whatsthis
  QString  s = i18n("<p>Format of the page header. The following tags are supported:</p>");
  QString s1 = i18n(
      "<ul><li><tt>%u</tt>: current user name</li>"
      "<li><tt>%d</tt>: complete date/time in short format</li>"
      "<li><tt>%D</tt>: complete date/time in long format</li>"
      "<li><tt>%h</tt>: current time</li>"
      "<li><tt>%y</tt>: current date in short format</li>"
      "<li><tt>%Y</tt>: current date in long format</li>"
      "<li><tt>%f</tt>: file name</li>"
      "<li><tt>%U</tt>: full URL of the document</li>"
      "<li><tt>%p</tt>: page number</li>"
      "</ul><br />"
      "<u>Note:</u> Do <b>not</b> use the '|' (vertical bar) character.");
  leHeaderRight->setWhatsThis(s + s1 );
  leHeaderCenter->setWhatsThis(s + s1 );
  leHeaderLeft->setWhatsThis(s + s1 );
  s = i18n("<p>Format of the page footer. The following tags are supported:</p>");
  leFooterRight->setWhatsThis(s + s1 );
  leFooterCenter->setWhatsThis(s + s1 );
  leFooterLeft->setWhatsThis(s + s1 );
}

QFont KatePrintHeaderFooter::font()
{
    return lFontPreview->font();
}

bool KatePrintHeaderFooter::useHeader()
{
  return cbEnableHeader->isChecked();
}

QString KatePrintHeaderFooter::headerFormat()
{
  return leHeaderLeft->text() + '|' +
  leHeaderCenter->text() + '|' +
  leHeaderRight->text();
}

QColor KatePrintHeaderFooter::headerForeground()
{
  return kcbtnHeaderFg->color();
}

QColor KatePrintHeaderFooter::headerBackground()
{
  return kcbtnHeaderBg->color();
}

bool KatePrintHeaderFooter::useHeaderBackground()
{
  return cbHeaderEnableBgColor->isChecked();
}

bool KatePrintHeaderFooter::useFooter()
{
  return cbEnableFooter->isChecked();
}

QString KatePrintHeaderFooter::footerFormat()
{
  return leFooterLeft->text() + '|' +
  leFooterCenter->text() + '|' +
  leFooterRight->text();
}

QColor KatePrintHeaderFooter::footerForeground()
{
  return kcbtnFooterFg->color();
}

QColor KatePrintHeaderFooter::footerBackground()
{
  return kcbtnFooterBg->color();
}

bool KatePrintHeaderFooter::useFooterBackground()
{
  return cbFooterEnableBgColor->isChecked();
}

void KatePrintHeaderFooter::setHFFont()
{
  QFont fnt( lFontPreview->font() );
  // display a font dialog
  if ( KFontDialog::getFont( fnt, false, this ) == KFontDialog::Accepted )
  {
    // set preview
    lFontPreview->setFont( fnt );
    lFontPreview->setText( (fnt.family() + ", %1pt").arg( fnt.pointSize() ) );
  }
}

//END KatePrintHeaderFooter

//BEGIN KatePrintLayout

KatePrintLayout::KatePrintLayout( QWidget *parent)
  : QWidget( parent )
{
  setWindowTitle( i18n("L&ayout") );

  QVBoxLayout *lo = new QVBoxLayout ( this );
  lo->setSpacing( KDialog::spacingHint() );

  KHBox *hb = new KHBox( this );
  lo->addWidget( hb );
  QLabel *lSchema = new QLabel( i18n("&Schema:"), hb );
  cmbSchema = new QComboBox( hb );
  cmbSchema->setEditable( false );
  lSchema->setBuddy( cmbSchema );

  cbDrawBackground = new QCheckBox( i18n("Draw bac&kground color"), this );
  lo->addWidget( cbDrawBackground );

  cbEnableBox = new QCheckBox( i18n("Draw &boxes"), this );
  lo->addWidget( cbEnableBox );

  gbBoxProps = new QGroupBox( this );
  gbBoxProps->setTitle(i18n("Box Properties"));
  QGridLayout* grid = new QGridLayout(gbBoxProps);
  lo->addWidget( gbBoxProps );

  QLabel *lBoxWidth = new QLabel( i18n("W&idth:"), gbBoxProps );
  grid->addWidget(lBoxWidth, 0, 0);
  sbBoxWidth = new QSpinBox( gbBoxProps );
  sbBoxWidth->setRange( 1, 100 );
  sbBoxWidth->setSingleStep( 1 );
  grid->addWidget(sbBoxWidth, 0, 1);
  lBoxWidth->setBuddy( sbBoxWidth );

  QLabel *lBoxMargin = new QLabel( i18n("&Margin:"), gbBoxProps );
  grid->addWidget(lBoxWidth, 1, 0);
  sbBoxMargin = new QSpinBox( gbBoxProps );
  sbBoxMargin->setRange( 0, 100 );
  sbBoxMargin->setSingleStep( 1 );
  grid->addWidget(sbBoxMargin, 1, 1);
  lBoxMargin->setBuddy( sbBoxMargin );

  QLabel *lBoxColor = new QLabel( i18n("Co&lor:"), gbBoxProps );
  grid->addWidget(lBoxColor, 2, 0);
  kcbtnBoxColor = new KColorButton( gbBoxProps );
  grid->addWidget(kcbtnBoxColor, 2, 1);
  lBoxColor->setBuddy( kcbtnBoxColor );

  connect( cbEnableBox, SIGNAL(toggled(bool)), gbBoxProps, SLOT(setEnabled(bool)) );

  lo->addStretch( 1 );
  // set defaults:
  sbBoxMargin->setValue( 6 );
  gbBoxProps->setEnabled( false );
  cmbSchema->addItems (KateGlobal::self()->schemaManager()->list ());
  cmbSchema->setCurrentIndex( 1 );

  // whatsthis
  // FIXME uncomment when string freeze is over
//   QWhatsThis::add ( cmbSchema, i18n(
//         "Select the color scheme to use for the print." ) );
  cbDrawBackground->setWhatsThis(i18n(
        "<p>If enabled, the background color of the editor will be used.</p>"
        "<p>This may be useful if your color scheme is designed for a dark background.</p>") );
  cbEnableBox->setWhatsThis(i18n(
        "<p>If enabled, a box as defined in the properties below will be drawn "
        "around the contents of each page. The Header and Footer will be separated "
        "from the contents with a line as well.</p>") );
  sbBoxWidth->setWhatsThis(i18n(
        "The width of the box outline" ) );
  sbBoxMargin->setWhatsThis(i18n(
        "The margin inside boxes, in pixels") );
  kcbtnBoxColor->setWhatsThis(i18n(
        "The line color to use for boxes") );
}

QString KatePrintLayout::colorScheme()
{
  return cmbSchema->currentText();
}

bool KatePrintLayout::useBackground()
{
  return cbDrawBackground->isChecked();
}

bool KatePrintLayout::useBox()
{
  return cbEnableBox->isChecked();
}

int KatePrintLayout::boxWidth()
{
  return sbBoxWidth->value();
}

int KatePrintLayout::boxMargin()
{
  return sbBoxMargin->value();
}

QColor KatePrintLayout::boxColor()
{
  return kcbtnBoxColor->color();
}
//END KatePrintLayout

#include "kateprinter.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
