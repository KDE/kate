/* This file is part of the KDE libraries
   Copyright (C) 2001-2002 Christoph Cullmann <cullmann@kde.org> 
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   
   Based on:
     TextLine : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
   
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

// $Id$

#include "katetextline.h"
#include <kdebug.h>
#include <qregexp.h>

TextLine::TextLine()
  : m_flags(TextLine::flagVisible)
{
}

TextLine::~TextLine()
{
}

void TextLine::replace(uint pos, uint delLen, const QChar *insText, uint insLen, uchar *insAttribs)
{
  uint textLen, newLen, z;
  int i, z2;
  uchar newAttr;

  textLen = m_text.size ();

  //find new length
  if (delLen <= textLen)
    newLen = textLen - delLen;
  else
    newLen = 0;

  if (newLen < pos) newLen = pos;
  newLen += insLen;
  newAttr = (pos < textLen) ? m_attributes[pos] : 0;

  if (newLen > textLen)
  {
    m_text.resize (newLen);
    m_attributes.resize (newLen);
  }

  //fill up with spaces and attribute
  for (z = textLen; z < pos; z++) {
    m_text[z] = QChar(' ');
    m_attributes[z] = 0;
  }

  i = (insLen - delLen);

  if (i != 0)
  {
    if (i <= 0)
    {
      //text to replace longer than new text
      for (z = pos + delLen; z < textLen; z++) {
        if ((z+i) >= newLen) break;
        m_text[z + i] = m_text[z];
        m_attributes[z + i] = m_attributes[z];
      }


    } else {
      //text to replace shorter than new text
      for (z2 = textLen-1; z2 >= (int) (pos + delLen); z2--) {
        if (z2 < 0) break;
        m_text[z2 + i] = m_text[z2];
        m_attributes[z2 + i] = m_attributes[z2];
      }
    }
  }

  if (insAttribs == 0L) {
    for (z = 0; z < insLen; z++) {
      m_text[pos + z] = insText[z];
      m_attributes[pos + z] = newAttr;
    }
  } else {
    for (z = 0; z < insLen; z++) {
      m_text[pos + z] = insText[z];
      m_attributes[pos + z] = insAttribs[z];
    }
  }

  if (newLen < textLen)
  {
    m_text.truncate (newLen);
    m_attributes.truncate (newLen);
  }
}

void TextLine::append(const QChar *s, uint l)
{
  replace(m_text.size(), 0, s, l);
}

void TextLine::truncate(uint newLen)
{
  if (newLen < m_text.size())
  {
    m_text.truncate (newLen);
    m_attributes.truncate (newLen);
  }
}

void TextLine::wrap(TextLine::Ptr nextLine, uint pos)
{
  int l = m_text.size() - pos;

  if (l > 0)
  {
    nextLine->replace(0, 0, &m_text[pos], l, &m_attributes[pos]);
    truncate(pos);
  }
}

void TextLine::unWrap(uint pos, TextLine::Ptr nextLine, uint len)
{
  replace(pos, 0, nextLine->m_text.data(), len, nextLine->m_attributes.data());
  nextLine->replace(0, len, 0L, 0);
}

int TextLine::nextNonSpaceChar(uint pos) const
{
    for(int i = pos; i < (int)m_text.size(); i++) {
        if(!m_text[i].isSpace())
            return i;
    }
    return -1;
}

int TextLine::previousNonSpaceChar(uint pos) const
{
    if(pos >= m_text.size()) {
        kdDebug(1714) << "pos >= m_text.size()" << endl;
        pos = m_text.size() - 1;
    }

    for(int i = pos; i >= 0; i--) {
        if(!m_text[i].isSpace())
            return i;
    }
    return -1;
}
 
int TextLine::firstChar() const
{
  return nextNonSpaceChar(0); 
} 
 
int TextLine::lastChar() const
{
    return previousNonSpaceChar(m_text.size() - 1);
}

QString TextLine::string (uint startCol, uint length) const
{
  if ( startCol >= m_text.size() )
    return QString ();
    
  if ( (startCol + length) > m_text.size())
    length = m_text.size() - startCol;
    
  return QString (m_text.data() + startCol, length);
}

void TextLine::removeSpaces()
{
  truncate(lastChar() + 1);
}

QChar *TextLine::firstNonSpace() const
{
  int first = firstChar();
  return (first > -1) ? &m_text[first] : m_text.data();
}

bool TextLine::stringAtPos(uint pos, const QString& match) const
{
  return (QConstString (this->m_text.data(), this->m_text.size()).string().mid(pos, match.length()) == match);
}

bool TextLine::startingWith(const QString& match) const
{
  return (QConstString (this->m_text.data(), this->m_text.size()).string().left(match.length()) == match);
}

bool TextLine::endingWith(const QString& match) const
{
  return (QConstString (this->m_text.data(), this->m_text.size()).string().right(match.length()) == match);
}

int TextLine::cursorX(uint pos, uint tabChars) const
{
  int l, x, z;

  l = (pos < m_text.size()) ? pos : m_text.size();
  x = 0;
  for (z = 0; z < l; z++) {
    if (m_text[z] == QChar('\t')) x += tabChars - (x % tabChars); else x++;
  }
  x += pos - l;
  return x;
}

void TextLine::setAttribs(uchar attribute, uint start, uint end) {
  uint z;

  if (end > m_text.size()) end = m_text.size();
  for (z = start; z < end; z++) m_attributes[z] = attribute;
}

bool TextLine::searchText (unsigned int startCol, const QString &text, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive, bool backwards)
{
  int index;

  if (backwards)
    index = QConstString (this->m_text.data(), this->m_text.size()).string().findRev (text, startCol, casesensitive);
  else
    index = QConstString (this->m_text.data(), this->m_text.size()).string().find (text, startCol, casesensitive);

   if (index > -1)
	{
	  (*foundAtCol) = index;
		(*matchLen)=text.length();
		return true;
  }

  return false;
} 
 
bool TextLine::searchText (unsigned int startCol, const QRegExp &regexp, unsigned int *foundAtCol, unsigned int *matchLen, bool backwards) 
{ 
  int index; 
 
  if (backwards)
    index = regexp.searchRev (QConstString (this->m_text.data(), this->m_text.size()).string(), startCol);
  else
    index = regexp.search (QConstString (this->m_text.data(), this->m_text.size()).string(), startCol);
 
   if (index > -1) 
	{ 
	  (*foundAtCol) = index; 
		(*matchLen)=regexp.matchedLength(); 
		return true; 
  } 
 
  return false; 
}                                 

uint TextLine::dumpSize () const
{  
  return (3*sizeof(uint)) + (m_text.size()*(sizeof(QChar) + 1)) + 1 + (m_ctx.size() * sizeof(signed char)) + (m_foldingList.size() * sizeof(signed char));
}
 
char *TextLine::dump (char *buf) const
{
  uint l = m_text.size();
  uint lctx = m_ctx.size();
  uint lfold = m_foldingList.size();

  memcpy(buf, &l, sizeof(uint));
  buf += sizeof(uint);

  memcpy(buf, &lctx, sizeof(uint));
  buf += sizeof(uint);

  memcpy(buf, &lfold, sizeof(uint));
  buf += sizeof(uint);

  memcpy(buf, (char *) m_text.data(), sizeof(QChar)*l);
  buf += sizeof(QChar)*l;

  memcpy(buf, (char *) m_attributes.data(), l);
  buf += l;
  
  memcpy(buf, (char *) &m_flags, 1);
  buf += 1;

  memcpy(buf, (signed char *)m_ctx.data(), lctx);
  buf += sizeof (signed char) * lctx;

  memcpy(buf, (signed char *)m_foldingList.data(), lfold);
  buf += sizeof (signed char) * lfold;       
      
  return buf;
}      

char *TextLine::restore (char *buf)
{
  uint l = 0;
  uint lctx = 0;
  uint lfold = 0;

  // text + context length read
  memcpy((char *) &l, buf, sizeof(uint));
  buf += sizeof(uint);
  memcpy((char *) &lctx, buf, sizeof(uint));
  buf += sizeof(uint);
  memcpy((char *) &lfold, buf, sizeof(uint));
  buf += sizeof(uint);

  // text + attributes
  m_text.duplicate ((QChar *) buf, l);
  m_attributes.duplicate ((uchar *) buf + (sizeof(QChar)*l), l);
  buf += sizeof(QChar)*l;
  buf += l;

  memcpy((char *) &m_flags, buf, 1);
  buf += 1;

  m_ctx.duplicate ((signed char *) buf, lctx);
  buf += lctx;

  m_foldingList.duplicate ((signed char *) buf, lfold);
  buf += lfold;           
  
  return buf;
}







