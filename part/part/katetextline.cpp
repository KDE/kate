/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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

// $Id$

#include "katetextline.h"
#include <kdebug.h>

TextLine::TextLine()
  : attr (0), hlContinue(false),m_visible(true)
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

  textLen = text.size ();

  //find new length
  if (delLen <= textLen)
    newLen = textLen - delLen;
  else
    newLen = 0;

  if (newLen < pos) newLen = pos;
  newLen += insLen;
  newAttr = (pos < textLen) ? attributes[pos] : attr;

  if (newLen > textLen)
  {
    text.resize (newLen);
    attributes.resize (newLen);
  }

  //fill up with spaces and attribute
  for (z = textLen; z < pos; z++) {
    text[z] = QChar(' ');
    attributes[z] = attr;
  }

  i = (insLen - delLen);

  if (i != 0)
  {
    if (i <= 0)
    {
      //text to replace longer than new text
      for (z = pos + delLen; z < textLen; z++) {
        if ((z+i) >= newLen) break;
        text[z + i] = text[z];
        attributes[z + i] = attributes[z];
      }


    } else {
      //text to replace shorter than new text
      for (z2 = textLen-1; z2 >= (int) (pos + delLen); z2--) {
        if (z2 < 0) break;
        text[z2 + i] = text[z2];
        attributes[z2 + i] = attributes[z2];
      }
    }
  }

  if (insAttribs == 0L) {
    for (z = 0; z < insLen; z++) {
      text[pos + z] = insText[z];
      attributes[pos + z] = newAttr;
    }
  } else {
    for (z = 0; z < insLen; z++) {
      text[pos + z] = insText[z];
      attributes[pos + z] = insAttribs[z];
    }
  }

  if (newLen < textLen)
  {
    text.truncate (newLen);
    attributes.truncate (newLen);
  }
}

void TextLine::append(const QChar *s, uint l)
{
  replace(text.size(), 0, s, l);
}

void TextLine::truncate(uint newLen)
{
  if (newLen < text.size())
  {
    text.truncate (newLen);
    attributes.truncate (newLen);
  }
}

void TextLine::wrap(TextLine::Ptr nextLine, uint pos)
{
  int l = text.size() - pos;

  if (l > 0)
  {
    nextLine->replace(0, 0, &text[pos], l, &attributes[pos]);
    attr = attributes[pos];
    truncate(pos);
  }
}

void TextLine::unWrap(uint pos, TextLine::Ptr nextLine, uint len)
{
  replace(pos, 0, nextLine->text.data(), len, nextLine->attributes.data());
  attr = nextLine->getAttr(len);
  nextLine->replace(0, len, 0L, 0);
}

int TextLine::firstChar() const
{
  uint z = 0;

  while (z < text.size() && text[z].isSpace()) z++;

  if (z < text.size())
    return z;
  else
    return -1;
}

int TextLine::lastChar() const
{
  uint z = text.size();

  while (z > 0 && text[z - 1].isSpace()) z--;
  return z;
}

void TextLine::removeSpaces()
{
  while (text.size() > 0 && text[text.size() - 1].isSpace()) truncate (text.size()-1);
}

QChar TextLine::getChar(uint pos) const
{
  if (pos < text.size())
	  return text[pos];

  return QChar(' ');
}

QChar *TextLine::firstNonSpace() const
{
  int first=firstChar();
  return (first > -1) ? &text[first] : text.data();
}

bool TextLine::startingWith(const QString& match) const
{
  if (match.length() > text.size())
	  return false;

	for (uint z=0; z<match.length(); z++)
	  if (match[z] != text[z])
		  return false;

  return true;
}

bool TextLine::endingWith(const QString& match) const
{
  if (match.length() > text.size())
    return false;

  for (int z=text.size(); z>(int)(text.size()-match.length()); z--)
    if (match[z] != text[z])
      return false;

  return true;
}

int TextLine::cursorX(uint pos, uint tabChars) const
{
  int l, x, z;

  l = (pos < text.size()) ? pos : text.size();
  x = 0;
  for (z = 0; z < l; z++) {
    if (text[z] == QChar('\t')) x += tabChars - (x % tabChars); else x++;
  }
  x += pos - l;
  return x;
}

void TextLine::setAttribs(uchar attribute, uint start, uint end) {
  uint z;

  if (end > text.size()) end = text.size();
  for (z = start; z < end; z++) attributes[z] = attribute;
}

void TextLine::setAttr(uchar attribute) {
  attr = attribute;
}

uchar TextLine::getAttr(uint pos) const {
  if (pos < text.size()) return attributes[pos];
  return attr;
}

uchar TextLine::getAttr() const {
  return attr;
}

void TextLine::setContext(signed char *newctx, uint len)
{
  ctx.duplicate (newctx, len);
}

bool TextLine::searchText (unsigned int startCol, const QString &text, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive, bool backwards)
{
  int index;

  if (backwards)
    index = QConstString (this->text.data(), this->text.size()).string().findRev (text, startCol, casesensitive);
  else
    index = QConstString (this->text.data(), this->text.size()).string().find (text, startCol, casesensitive);

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
    index = regexp.searchRev (QConstString (this->text.data(), this->text.size()).string(), startCol);
  else
    index = regexp.search (QConstString (this->text.data(), this->text.size()).string(), startCol);
 
   if (index > -1) 
	{ 
	  (*foundAtCol) = index; 
		(*matchLen)=regexp.matchedLength(); 
		return true; 
  } 
 
  return false; 
} 
 
