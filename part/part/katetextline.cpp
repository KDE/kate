/***************************************************************************
                          katetextline.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
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
 ***************************************************************************/

#include "katetextline.h"
#include <kdebug.h>

TextLine::TextLine()
  : text(0L), attributes(0L), textLen(0), ctx(0L), ctxLen(0), hlContinue(false)
{
  attr = 0;
}

TextLine::~TextLine()
{
  if (text != 0L)
    free (text);

  if (attributes != 0L)
    free (attributes);

  if (ctx != 0L)
    free (ctx);
} 
 
void TextLine::replace(uint pos, uint delLen, const QChar *insText, uint insLen, uchar *insAttribs) 
{ 
  uint newLen, z;
  int i, z2; 
  uchar newAttr; 
 
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
    if (text == 0L) 
      text = (QChar *) malloc (sizeof (QChar)*newLen); 
    else 
      text = (QChar *) realloc(text, sizeof (QChar)*newLen); 
 
    if (attributes == 0L) 
      attributes = (uchar *) malloc (sizeof (uchar)*newLen); 
    else 
      attributes = (uchar *) realloc(attributes, sizeof (uchar)*newLen); 
  } 
 
  //fill up with spaces and attribute 
  for (z = textLen; z < pos; z++) { 
    text[z] = QChar(' '); 
    attributes[z] = attr; 
  } 
 
  i = (insLen - delLen); 
 
  // realloc has gone - no idea why it was here 
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
    text = (QChar *) realloc(text, sizeof (QChar)*newLen); 
    attributes = (uchar *) realloc(attributes, sizeof (uchar)*newLen); 
  } 
 
  textLen = newLen; 
} 
 
void TextLine::append(const QChar *s, uint l) 
{ 
  replace(textLen, 0, s, l); 
} 
 
void TextLine::truncate(uint newLen) 
{ 
  if (newLen < textLen) 
  {
    textLen = newLen;
    text = (QChar *) realloc(text, sizeof (QChar)*newLen);
    attributes = (uchar *) realloc(attributes, sizeof (uchar)*newLen);
  }
}

void TextLine::wrap(TextLine::Ptr nextLine, uint pos)
{
  int l = textLen - pos;

  if (l > 0)
  {
    nextLine->replace(0, 0, &text[pos], l, &attributes[pos]);
    attr = attributes[pos];
    truncate(pos);
  }
}

void TextLine::unWrap(uint pos, TextLine::Ptr nextLine, uint len)
{
  replace(pos, 0, nextLine->text, len, nextLine->attributes); 
  attr = nextLine->getAttr(len); 
  nextLine->replace(0, len, 0L, 0); 
} 
 
int TextLine::firstChar() const
{
  uint z = 0; 
 
  while (z < textLen && text[z].isSpace()) z++; 
 
  if (z < textLen) 
    return z; 
  else 
    return -1; 
} 
 
int TextLine::lastChar() const
{
  uint z = textLen;

  while (z > 0 && text[z - 1].isSpace()) z--;
  return z;
}

void TextLine::removeSpaces()
{
  while (textLen > 0 && text[textLen - 1].isSpace()) truncate (textLen-1);
}

QChar TextLine::getChar(uint pos) const
{
  if (pos < textLen)
	  return text[pos];

  return QChar(' ');
}

const QChar *TextLine::firstNonSpace()
{
  int first=firstChar();
  return (first > -1) ? &text[first] : text;
}

bool TextLine::startingWith(const QString& match) const
{
  if (match.length() > textLen)
	  return false;

	for (uint z=0; z<match.length(); z++)
	  if (match[z] != text[z])
		  return false;

  return true;
}

bool TextLine::endingWith(const QString& match) const
{
  if (match.length() > textLen)
    return false;

  for (int z=textLen; z>(int)(textLen-match.length()); z--)
    if (match[z] != text[z])
      return false;

  return true;
}

int TextLine::cursorX(uint pos, uint tabChars) const
{
  int l, x, z;

  l = (pos < textLen) ? pos : textLen;
  x = 0;
  for (z = 0; z < l; z++) {
    if (text[z] == QChar('\t')) x += tabChars - (x % tabChars); else x++;
  }
  x += pos - l;
  return x;
}

void TextLine::setAttribs(uchar attribute, uint start, uint end) {
  uint z;

  if (end > textLen) end = textLen;
  for (z = start; z < end; z++) attributes[z] = attribute;
}

void TextLine::setAttr(uchar attribute) {
  attr = attribute;
}

uchar TextLine::getAttr(uint pos) const {
  if (pos < textLen) return attributes[pos];
  return attr;
}

uchar TextLine::getAttr() const {
  return attr;
}

void TextLine::setContext(signed char *newctx, uint len)
{
  ctxLen = len;

	if (ctx == 0L)
    ctx = (signed char*)malloc (len);
  else
    ctx = (signed char*)realloc (ctx, len);

  for (uint z=0; z < len; z++) ctx[z] = newctx[z];
}

bool TextLine::searchText (unsigned int startCol, const QString &text, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive, bool backwards)
{
  int index;

  if (backwards)
    index = QString (this->text, textLen).findRev (text, startCol, casesensitive);
  else
    index = QString (this->text, textLen).find (text, startCol, casesensitive);
 
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
    index = regexp.searchRev (QString (this->text, textLen), startCol); 
  else 
    index = regexp.search (QString (this->text, textLen), startCol); 
 
   if (index > -1) 
	{ 
	  (*foundAtCol) = index; 
		(*matchLen)=regexp.matchedLength(); 
		return true; 
  } 
 
  return false; 
} 
 
