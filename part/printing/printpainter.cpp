/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2002-2010 Anders Lund <anders@alweb.dk>
 *
 *  Rewritten based on code of Copyright (c) 2002 Michael Goffioul <kdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
 */

#include "printpainter.h"

#include "katetextfolding.h"
#include "katedocument.h"
#include "katebuffer.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katehighlight.h"
#include "katepartdebug.h"
#include "katetextlayout.h"

#include <KLocalizedString>
#include <KUser>

#include <QPainter>
#include <QPrinter>

using namespace KatePrinter;

PrintPainter::PrintPainter(KateDocument *doc)
  : m_doc(doc)
  , m_colorScheme()
  , m_printGuide(false)
  , m_printLineNumbers(false)
  , m_useHeader(false)
  , m_useFooter(false)
  , m_useBackground(false)
  , m_useBox(false)
  , m_useHeaderBackground(false)
  , m_useFooterBackground(false)
  , m_boxMargin(0)
  , m_boxWidth(1)
  , m_boxColor()
  , m_headerBackground()
  , m_headerForeground()
  , m_footerBackground()
  , m_footerForeground()
  , m_fhFont()
  , m_headerFormat()
  , m_footerFormat()
{
}

void PrintPainter::paint(QPrinter *printer) const
{
  Kate::TextFolding folding (m_doc->buffer());
  KateRenderer renderer(m_doc, folding, m_doc->activeKateView());
  renderer.config()->setSchema (m_colorScheme);
  renderer.setPrinterFriendly(true);

  QPainter paint(printer);
  /*
    *        We work in three cycles:
    *        1) initialize variables and retrieve print settings
    *        2) prepare data according to those settings
    *        3) draw to the printer
    */
  uint pdmWidth = printer->width();
  uint pdmHeight = printer->height();
  int y = 0;
  uint xstart = 0; // beginning point for painting lines
  uint lineCount = 0;
  uint maxWidth = pdmWidth;
  int headerWidth = pdmWidth;
  bool pageStarted = true;
  int remainder = 0; // remaining sublines from a wrapped line (for the top of a new page)

  // Text Settings Page
  bool selectionOnly = (printer->printRange() == QPrinter::Selection);
  bool useGuide = m_printGuide;

  bool printLineNumbers = m_printLineNumbers;
  uint lineNumberWidth( 0 );

  // Header/Footer Page
  QFont headerFont(m_fhFont); // used for header/footer

  bool useHeader = m_useHeader;
  QColor headerBgColor(m_headerBackground);
  QColor headerFgColor(m_headerForeground);
  uint headerHeight( 0 ); // further init only if needed
  QStringList headerTagList; // do
  bool headerDrawBg = false; // do

  bool useFooter = m_useFooter;
  QColor footerBgColor(m_footerBackground);
  QColor footerFgColor(m_footerForeground);
  uint footerHeight( 0 ); // further init only if needed
  QStringList footerTagList; // do
  bool footerDrawBg = false; // do

  // Layout Page
  renderer.config()->setSchema(m_colorScheme);
  bool useBackground = m_useBackground;
  bool useBox = m_useBox;
  int boxWidth(m_boxWidth);
  QColor boxColor(m_boxColor);
  int innerMargin = useBox ? m_boxMargin : 6;

  // Post initialization
  int maxHeight = (useBox ? pdmHeight-innerMargin : pdmHeight);
  uint currentPage( 1 );
  uint lastline = m_doc->lastLine(); // necessary to print selection only
  uint firstline( 0 );
  const int fontHeight = renderer.fontHeight();
  KTextEditor::Range selectionRange;

  /*
  *        Now on for preparations...
  *        during preparations, variable names starting with a "_" means
  *        those variables are local to the enclosing block.
  */
  {
    if ( selectionOnly )
    {
      // set a line range from the first selected line to the last
      selectionRange = m_doc->activeView()->selectionRange();
      firstline = selectionRange.start().line();
      lastline = selectionRange.end().line();
      lineCount = firstline;
    }

    if ( printLineNumbers )
    {
      // figure out the horiizontal space required
      QString s( QString("%1 ").arg( m_doc->lines() ) );
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

      tags["d"] =  QLocale().toString(dt, QLocale::ShortFormat);
      tags["D"] =  QLocale().toString(dt, QLocale::LongFormat);
      tags["h"] =  QLocale().toString(dt.time(), QLocale::ShortFormat);
      tags["y"] =  QLocale().toString(dt.date(), QLocale::ShortFormat);
      tags["Y"] =  QLocale().toString(dt.date(), QLocale::LongFormat);
      tags["f"] =  m_doc->url().fileName();
      tags["U"] =  m_doc->url().toString();
      if ( selectionOnly )
      {
        QString s( i18n("(Selection of) ") );
        tags["f"].prepend( s );
        tags["U"].prepend( s );
      }

      QRegExp reTags( "%([dDfUhuyY])" ); // TODO tjeck for "%%<TAG>"

      if (useHeader)
      {
        headerDrawBg = m_useHeaderBackground;
        headerHeight = QFontMetrics( headerFont ).height();
        if ( useBox || headerDrawBg )
          headerHeight += innerMargin * 2;
        else
          headerHeight += 1 + QFontMetrics( headerFont ).leading();

        headerTagList = m_headerFormat;
        QMutableStringListIterator it(headerTagList);
        while ( it.hasNext() ) {
          QString tag = it.next();
          int pos = reTags.indexIn( tag );
          QString rep;
          while ( pos > -1 )
          {
            rep = tags[reTags.cap( 1 )];
            tag.replace( (uint)pos, 2, rep );
            pos += rep.length();
            pos = reTags.indexIn( tag, pos );
          }
          it.setValue( tag );
        }

        if (!headerBgColor.isValid())
          headerBgColor = Qt::lightGray;
        if (!headerFgColor.isValid())
          headerFgColor = Qt::black;
      }

      if (useFooter)
      {
        footerDrawBg = m_useFooterBackground;
        footerHeight = QFontMetrics( headerFont ).height();
        if ( useBox || footerDrawBg )
          footerHeight += 2*innerMargin;
        else
          footerHeight += 1; // line only

        footerTagList = m_footerFormat;
        QMutableStringListIterator it(footerTagList);
        while ( it.hasNext() ) {
          QString tag = it.next();
          int pos = reTags.indexIn( tag );
          QString rep;
          while ( pos > -1 )
          {
            rep = tags[reTags.cap( 1 )];
            tag.replace( (uint)pos, 2, rep );
            pos += rep.length();
            pos = reTags.indexIn( tag, pos );
          }
          it.setValue( tag );
        }

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

    // now that we know the vertical amount of space needed,
    // it is possible to calculate the total number of pages
    // if needed, that is if any header/footer tag contains "%P".
    if ( !headerTagList.filter("%P").isEmpty() || !footerTagList.filter("%P").isEmpty() )
    {
      qCDebug(LOG_PART)<<"'%P' found! calculating number of pages...";
      int pageHeight = maxHeight;
      if ( useHeader )
        pageHeight -= ( headerHeight + innerMargin );
      if ( useFooter )
        pageHeight -= innerMargin;
      const int linesPerPage = pageHeight / fontHeight;
//         qCDebug(LOG_PART) << "Lines per page:" << linesPerPage;

      // calculate total layouted lines in the document
      int totalLines = 0;
      // TODO: right now ignores selection printing
      for (unsigned int i = firstline; i <= lastline; ++i) {
        KateLineLayoutPtr rangeptr(new KateLineLayout(renderer));
        rangeptr->setLine(i);
        renderer.layoutLine(rangeptr, (int)maxWidth, false);
        totalLines += rangeptr->viewLineCount();
      }
      int totalPages = (totalLines / linesPerPage)
                    + ((totalLines % linesPerPage) > 0 ? 1 : 0);
//         qCDebug(LOG_PART) << "_______ pages:" << (totalLines / linesPerPage);
//         qCDebug(LOG_PART) << "________ rest:" << (totalLines % linesPerPage);

      // TODO: add space for guide if required
//         if ( useGuide )
//           _lt += (guideHeight + (fontHeight /2)) / fontHeight;

      // substitute both tag lists
      QString re("%P");
      QStringList::Iterator it;
      for ( it=headerTagList.begin(); it!=headerTagList.end(); ++it )
        (*it).replace( re, QString( "%1" ).arg( totalPages ) );
      for ( it=footerTagList.begin(); it!=footerTagList.end(); ++it )
        (*it).replace( re, QString( "%1" ).arg( totalPages ) );
    }
  } // end prepare block

    /*
      On to draw something :-)
    */
  while (  lineCount <= lastline  )
  {
    if ( y + fontHeight > maxHeight )
    {
      qCDebug(LOG_PART)<<"Starting new page,"<<lineCount<<"lines up to now.";
      printer->newPage();
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
        paint.setPen(footerFgColor);
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

      if ( useGuide && currentPage == 1 )
      {  // FIXME - this may span more pages...
        // draw a box unless we have boxes, in which case we end with a box line
        int _ystart = y;
        QString _hlName = m_doc->highlight()->name();

        QList<KateExtendedAttribute::Ptr> _attributes; // list of highlight attributes for the legend
        m_doc->highlight()->getKateExtendedAttributeList(m_colorScheme, _attributes);

        KateAttributeList _defaultAttributes;
        KateHlManager::self()->getDefaults ( renderer.config()->schema(), _defaultAttributes );

        QColor _defaultPen = _defaultAttributes.at(0)->foreground().color();
        paint.setPen(_defaultPen);

        int _marg = 0;
        if ( useBox )
          _marg += (2*boxWidth) + (2*innerMargin);
        else
        {
          if ( useBackground )
            _marg += 2*innerMargin;
          _marg += 1;
          y += 1 + innerMargin;
        }

        // draw a title string
        QFont _titleFont = renderer.config()->font();
        _titleFont.setBold(true);
        paint.setFont( _titleFont );
        QRect _r;
        paint.drawText( QRect(_marg, y, pdmWidth-(2*_marg), maxHeight - y),
          Qt::AlignTop|Qt::AlignHCenter,
          i18n("Typographical Conventions for %1", _hlName ), &_r );
        int _w = pdmWidth - (_marg*2) - (innerMargin*2);
        int _x = _marg + innerMargin;
        y += _r.height() + innerMargin;
        paint.drawLine( _x, y, _x + _w, y );
        y += 1 + innerMargin;

        int _widest( 0 );
        foreach (const KateExtendedAttribute::Ptr &attribute, _attributes)
          _widest = qMax(QFontMetrics(attribute->font()).width(attribute->name().section(':',1,1)), _widest);

        int _guideCols = _w/( _widest + innerMargin );

        // draw attrib names using their styles
        int _cw = _w/_guideCols;
        int _i(0);

        _titleFont.setUnderline(true);
        QString _currentHlName;
        foreach (const KateExtendedAttribute::Ptr &attribute, _attributes)
        {
          QString _hl = attribute->name().section(':',0,0);
          QString _name = attribute->name().section(':',1,1);
          if ( _hl != _hlName && _hl != _currentHlName ) {
            _currentHlName = _hl;
            if ( _i%_guideCols )
              y += fontHeight;
            y += innerMargin;
            paint.setFont(_titleFont);
            paint.setPen(_defaultPen);
            paint.drawText( _x, y, _w, fontHeight, Qt::AlignTop, _hl + ' ' + i18n("text") );
            y += fontHeight;
            _i = 0;
          }

          KTextEditor::Attribute _attr =  *_defaultAttributes[attribute->defaultStyleIndex()];
          _attr += *attribute;
          paint.setPen( _attr.foreground().color() );
          paint.setFont( _attr.font() );

          if (_attr.hasProperty(QTextFormat::BackgroundBrush) ) {
            QRect _rect = QFontMetrics(_attr.font()).boundingRect(_name);
            _rect.moveTo(_x + ((_i%_guideCols)*_cw), y);
              paint.fillRect(_rect, _attr.background() );
          }

          paint.drawText(( _x + ((_i%_guideCols)*_cw)), y, _cw, fontHeight, Qt::AlignTop, _name );

          _i++;
          if ( _i && ! ( _i%_guideCols ) )
            y += fontHeight;
        }

        if ( _i%_guideCols )
          y += fontHeight;// last row not full

        // draw a box around the legend
        paint.setPen ( _defaultPen );
        if ( useBox )
          paint.fillRect( 0, y+innerMargin, headerWidth, boxWidth, boxColor );
        else
        {
          _marg -=1;
          paint.drawRect( _marg, _ystart, pdmWidth-(2*_marg), y-_ystart+innerMargin );
        }

        y += ( useBox ? boxWidth : 1 ) + (innerMargin*2);
      } // useGuide

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
    KateLineLayoutPtr rangeptr(new KateLineLayout(renderer));
    rangeptr->setLine(lineCount);
    renderer.layoutLine(rangeptr, (int)maxWidth, false);

    // selectionOnly: clip non-selection parts and adjust painter position if needed
    int _xadjust = 0;
    if (selectionOnly) {
      if (m_doc->activeView()->blockSelection()) {
        int _x = renderer.cursorToX(rangeptr->viewLine(0), selectionRange.start());
        int _x1 = renderer.cursorToX(rangeptr->viewLine(rangeptr->viewLineCount()-1), selectionRange.end());
          _xadjust = _x;
          paint.translate(-_xadjust, 0);
        paint.setClipRegion(QRegion( _x, 0, _x1 - _x, rangeptr->viewLineCount()*fontHeight));
      }

      else if (lineCount == firstline || lineCount == lastline) {
        QRegion region(0, 0, maxWidth, rangeptr->viewLineCount()*fontHeight);

        if ( lineCount == firstline) {
          region = region.subtracted(QRegion(0, 0, renderer.cursorToX(rangeptr->viewLine(0), selectionRange.start()), fontHeight));
        }

        if (lineCount == lastline) {
          int _x = renderer.cursorToX(rangeptr->viewLine(rangeptr->viewLineCount()-1), selectionRange.end());
          region = region.subtracted(QRegion(_x, 0, maxWidth-_x, fontHeight));
        }

        paint.setClipRegion(region);
      }
    }

    // If the line is too long (too many 'viewlines') to fit the remaining vertical space,
    // clip and adjust the painter position as necessary
    int _lines = rangeptr->viewLineCount(); // number of "sublines" to paint.

    int proceedLines = _lines;
    if (remainder) {
      proceedLines = qMin((maxHeight - y) / fontHeight, remainder);

      paint.translate(0, -(_lines-remainder)*fontHeight+1);
      paint.setClipRect(0, (_lines-remainder)*fontHeight+1, maxWidth, proceedLines*fontHeight); //### drop the crosspatch in printerfriendly mode???
      remainder -= proceedLines;
    }
    else if (y + fontHeight * _lines > maxHeight) {
      remainder = _lines - ((maxHeight-y)/fontHeight);
      paint.setClipRect(0, 0, maxWidth, (_lines-remainder)*fontHeight+1); //### drop the crosspatch in printerfriendly mode???
    }

    renderer.paintTextLine(paint, rangeptr, 0, (int)maxWidth);

    paint.setClipping(false);
    paint.translate(_xadjust, (fontHeight * (_lines-remainder)));

    y += fontHeight * proceedLines;

    if ( ! remainder )
    lineCount++;
  } // done lineCount <= lastline

  paint.end();
}

//END PrintPainter

#include "printpainter.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
