/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   Based on:
     KateTextLine : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "katetextline.h"
#include "katerenderer.h"

#include <qregexp.h>
#include <kglobal.h>
#include <kdebug.h>
#include <qstylesheet.h>

KateTextLine::KateTextLine ()
  : m_flags(0)
{
}

KateTextLine::~KateTextLine()
{
}

void KateTextLine::insertText (uint pos, uint insLen, const QChar *insText, uchar *insAttribs)
{
  // nothing to do
  if (insLen == 0)
    return;

  // calc new textLen, store old
  uint oldTextLen = m_text.length();
  m_text.insert (pos, insText, insLen);
  uint textLen = m_text.length();

  // resize the array
  m_attributes.resize (textLen);

  // HA, insert behind text end, fill with spaces
  if (pos >= oldTextLen)
  {
    for (uint z = oldTextLen; z < pos; z++)
      m_attributes[z] = 0;
  }
  // HA, insert in text, move the old text behind pos
  else if (oldTextLen > 0)
  {
    for (int z = oldTextLen -1; z >= (int) pos; z--)
      m_attributes[z+insLen] = m_attributes[z];
  }

  // BUH, actually insert the new text
  for (uint z = 0; z < insLen; z++)
  {
    if (insAttribs == 0)
      m_attributes[z+pos] = 0;
    else
      m_attributes[z+pos] = insAttribs[z];
  }
}

void KateTextLine::removeText (uint pos, uint delLen)
{
  // nothing to do
  if (delLen == 0)
    return;

  uint textLen = m_text.length();

  if (textLen == 0)
    return; // uh, again nothing real to do ;)

  if (pos >= textLen)
    return;

  if ((pos + delLen) > textLen)
    delLen = textLen - pos;

  // BU, MOVE THE OLD TEXT AROUND
  for (uint z = pos; z < textLen - delLen; z++)
    m_attributes[z] = m_attributes[z+delLen];

  m_text.remove (pos, delLen);
  m_attributes.resize (m_text.length ());
}

void KateTextLine::truncate(uint newLen)
{
  if (newLen < m_text.length())
  {
    m_text.truncate (newLen);
    m_attributes.truncate (newLen);
  }
}

int KateTextLine::nextNonSpaceChar(uint pos) const
{
  for(int i = pos; i < (int)m_text.length(); i++)
  {
    if(!m_text[i].isSpace())
      return i;
  }

  return -1;
}

int KateTextLine::previousNonSpaceChar(uint pos) const
{
  if (pos >= m_text.length())
    pos = m_text.length() - 1;

  for(int i = pos; i >= 0; i--)
  {
    if(!m_text[i].isSpace())
      return i;
  }

  return -1;
}

int KateTextLine::firstChar() const
{
  return nextNonSpaceChar(0);
}

int KateTextLine::lastChar() const
{
  return previousNonSpaceChar(m_text.length() - 1);
}

const QChar *KateTextLine::firstNonSpace() const
{
  int first = firstChar();
  return (first > -1) ? ((QChar*)m_text.unicode())+first : m_text.unicode();
}

uint KateTextLine::indentDepth (uint tabwidth) const
{
  uint d = 0;

  for(uint i = 0; i < m_text.length(); i++)
  {
    if(m_text[i].isSpace())
    {
      if (m_text[i] == QChar('\t'))
        d += tabwidth - (d % tabwidth);
      else
        d++;
    }
    else
      return d;
  }

  return d;
}

bool KateTextLine::stringAtPos(uint pos, const QString& match) const
{
  if ((pos+match.length()) > m_text.length())
    return false;

  for (uint i=0; i < match.length(); i++)
    if (m_text[i+pos] != match[i])
      return false;

  return true;
}

bool KateTextLine::startingWith(const QString& match) const
{
  if (match.length() > m_text.length())
    return false;

  for (uint i=0; i < match.length(); i++)
    if (m_text[i] != match[i])
      return false;

  return true;
}

bool KateTextLine::endingWith(const QString& match) const
{
  if (match.length() > m_text.length())
    return false;

  uint start = m_text.length() - match.length();
  for (uint i=0; i < match.length(); i++)
    if (m_text[start+i] != match[i])
      return false;

  return true;
}

int KateTextLine::cursorX(uint pos, uint tabChars) const
{
  uint x = 0;

  for ( uint z = 0; z < kMin (pos, m_text.length()); z++)
  {
    if (m_text[z] == QChar('\t'))
      x += tabChars - (x % tabChars);
    else
      x++;
  }

  return x;
}


uint KateTextLine::lengthWithTabs (uint tabChars) const
{
  uint x = 0;

  for ( uint z = 0; z < m_text.length(); z++)
  {
    if (m_text[z] == QChar('\t'))
      x += tabChars - (x % tabChars);
    else
      x++;
  }

  return x;
}

bool KateTextLine::searchText (uint startCol, const QString &text, uint *foundAtCol, uint *matchLen, bool casesensitive, bool backwards)
{
  int index;

  if (backwards)
  {
    int col = startCol;
    uint l = text.length();
    // allow finding the string ending at eol
    if ( col == m_text.length() ) startCol++;

    do {
      index = m_text.findRev( text, col, casesensitive );
      col--;
    } while ( col >= 0 && l + index >= startCol );
  }
  else
    index = m_text.find (text, startCol, casesensitive);

  if (index > -1)
  {
    if (foundAtCol)
      (*foundAtCol) = index;
    if (matchLen)
      (*matchLen)=text.length();
    return true;
  }

  return false;
}

bool KateTextLine::searchText (uint startCol, const QRegExp &regexp, uint *foundAtCol, uint *matchLen, bool backwards)
{
  int index;

  if (backwards)
  {
    int col = startCol;

    // allow finding the string ending at eol
    if ( col == m_text.length() ) startCol++;
    do {
      index = regexp.searchRev (m_text, col);
      col--;
    } while ( col >= 0 && regexp.matchedLength() + index >= (int)startCol );
  }
  else
    index = regexp.search (m_text, startCol);

  if (index > -1)
  {
    if (foundAtCol)
      (*foundAtCol) = index;

    if (matchLen)
      (*matchLen)=regexp.matchedLength();
    return true;
  }

  return false;
}

char *KateTextLine::dump (char *buf, bool withHighlighting) const
{
  uint l = m_text.length();
  char f = m_flags;

  if (!withHighlighting)
    f = f | KateTextLine::flagNoOtherData;

  memcpy(buf, (char *) &f, 1);
  buf += 1;

  memcpy(buf, &l, sizeof(uint));
  buf += sizeof(uint);

  memcpy(buf, (char *) m_text.unicode(), sizeof(QChar)*l);
  buf += sizeof(QChar) * l;

  if (!withHighlighting)
    return buf;

  memcpy(buf, (char *)m_attributes.data(), sizeof(uchar) * l);
  buf += sizeof (uchar) * l;

  uint lctx = m_ctx.size();
  uint lfold = m_foldingList.size();
  uint lind = m_indentationDepth.size();

  memcpy(buf, &lctx, sizeof(uint));
  buf += sizeof(uint);

  memcpy(buf, &lfold, sizeof(uint));
  buf += sizeof(uint);

  memcpy(buf, &lind, sizeof(uint));
  buf += sizeof(uint);

  memcpy(buf, (char *)m_ctx.data(), sizeof(short) * lctx);
  buf += sizeof (short) * lctx;

  memcpy(buf, (char *)m_foldingList.data(), sizeof(uint)*lfold);
  buf += sizeof (uint) * lfold;

  memcpy(buf, (char *)m_indentationDepth.data(), sizeof(unsigned short) * lind);
  buf += sizeof (unsigned short) * lind;

  return buf;
}

char *KateTextLine::restore (char *buf)
{
  uint l = 0;
  char f = 0;

  memcpy((char *) &f, buf, 1);
  buf += 1;

  // text + context length read
  memcpy((char *) &l, buf, sizeof(uint));
  buf += sizeof(uint);

  // text + attributes
  m_text.setUnicode ((QChar *) buf, l);
  buf += sizeof(QChar) * l;

  // we just restore a KateTextLine from a buffer first time
  if (f & KateTextLine::flagNoOtherData)
  {
    m_flags = 0;

    if (f & KateTextLine::flagAutoWrapped)
      m_flags = m_flags | KateTextLine::flagAutoWrapped;

    // fill with clean empty attribs !
    m_attributes.fill (0, l);

    return buf;
  }
  else
    m_flags = f;

  m_attributes.duplicate ((uchar *) buf, l);
  buf += sizeof(uchar) * l;

  uint lctx = 0;
  uint lfold = 0;
  uint lind = 0;

  memcpy((char *) &lctx, buf, sizeof(uint));
  buf += sizeof(uint);

  memcpy((char *) &lfold, buf, sizeof(uint));
  buf += sizeof(uint);

  memcpy((char *) &lind, buf, sizeof(uint));
  buf += sizeof(uint);

  m_ctx.duplicate ((short *) buf, lctx);
  buf += sizeof(short) * lctx;

  m_foldingList.duplicate ((uint *) buf, lfold);
  buf += sizeof(uint)*lfold;

  m_indentationDepth.duplicate ((unsigned short *) buf, lind);
  buf += sizeof(unsigned short) * lind;

  return buf;
}


void KateTextLine::stringAsHtml(uint startCol, uint length, KateRenderer *renderer, QTextStream *outputStream) const
{
  if(length == 0) return;
  // some variables :
  bool previousCharacterWasBold = false;
  bool previousCharacterWasItalic = false;
  // when entering a new color, we'll close all the <b> & <i> tags,
  // for HTML compliancy. that means right after that font tag, we'll
  // need to reinitialize the <b> and <i> tags.
  bool needToReinitializeTags = false;
  QColor previousCharacterColor(0,0,0); // default color of HTML characters is black
  QColor blackColor(0,0,0);
//  (*outputStream) << "<span style='color: #000000'>";


  // for each character of the line : (curPos is the position in the line)
  for (uint curPos=startCol;curPos<(length+startCol);curPos++)
    {
      KateAttribute* charAttributes = 0;

      charAttributes = renderer->attribute(attribute(curPos));


      //ASSERT(charAttributes != NULL);
      // let's give the color for that character :
      if ( (charAttributes->textColor() != previousCharacterColor))
      {  // the new character has a different color :
        // if we were in a bold or italic section, close it
        if (previousCharacterWasBold)
          (*outputStream) << "</b>";
        if (previousCharacterWasItalic)
          (*outputStream) << "</i>";

        // close the previous font tag :
	if(previousCharacterColor != blackColor)
          (*outputStream) << "</span>";
        // let's read that color :
        int red, green, blue;
        // getting the red, green, blue values of the color :
        charAttributes->textColor().rgb(&red, &green, &blue);
	if(!(red == 0 && green == 0 && blue == 0)) {
          (*outputStream) << "<span style='color: #"
              << ( (red < 0x10)?"0":"")  // need to put 0f, NOT f for instance. don't touch 1f.
              << QString::number(red, 16) // html wants the hex value here (hence the 16)
              << ( (green < 0x10)?"0":"")
              << QString::number(green, 16)
              << ( (blue < 0x10)?"0":"")
              << QString::number(blue, 16)
              << "'>";
	}
        // we need to reinitialize the bold/italic status, since we closed all the tags
        needToReinitializeTags = true;
      }
      // bold status :
      if ( (needToReinitializeTags && charAttributes->bold()) ||
          (!previousCharacterWasBold && charAttributes->bold()) )
        // we enter a bold section
        (*outputStream) << "<b>";
      if ( !needToReinitializeTags && (previousCharacterWasBold && !charAttributes->bold()) )
        // we leave a bold section
        (*outputStream) << "</b>";

      // italic status :
      if ( (needToReinitializeTags && charAttributes->italic()) ||
           (!previousCharacterWasItalic && charAttributes->italic()) )
        // we enter an italic section
        (*outputStream) << "<i>";
      if ( !needToReinitializeTags && (previousCharacterWasItalic && !charAttributes->italic()) )
        // we leave an italic section
        (*outputStream) << "</i>";

      // write the actual character :
      (*outputStream) << QStyleSheet::escape(QString(getChar(curPos)));

      // save status for the next character :
      previousCharacterWasItalic = charAttributes->italic();
      previousCharacterWasBold = charAttributes->bold();
      previousCharacterColor = charAttributes->textColor();
      needToReinitializeTags = false;
    }
  // Be good citizens and close our tags
  if (previousCharacterWasBold)
    (*outputStream) << "</b>";
  if (previousCharacterWasItalic)
    (*outputStream) << "</i>";

  if(previousCharacterColor != blackColor)
    (*outputStream) << "</span>";
}

// kate: space-indent on; indent-width 2; replace-tabs on;
