/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>

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

#include "kateautoindent.h"
#include "kateconfig.h"
#include "kateview.h"

#include <klocale.h>

KateAutoIndent *KateAutoIndent::createIndenter (KateDocument *doc, uint mode)
{
  if (mode == KateDocumentConfig::imCStyle)
    return new KateCSmartIndent (doc);

  return new KateAutoIndent (doc);
}

QStringList KateAutoIndent::listModes ()
{
  QStringList l;

  l << modeDescription(KateDocumentConfig::imNormal);
  l << modeDescription(KateDocumentConfig::imCStyle);

  return l;
}

QString KateAutoIndent::modeName (uint mode)
{
  if (mode == KateDocumentConfig::imCStyle)
    return QString ("cstyle");

  return QString ("normal");
}

QString KateAutoIndent::modeDescription (uint mode)
{
  if (mode == KateDocumentConfig::imCStyle)
    return i18n ("C Style");

  return i18n ("Normal");
}

uint KateAutoIndent::modeNumber (const QString &name)
{
  if (modeName(KateDocumentConfig::imCStyle) == name)
    return KateDocumentConfig::imCStyle;

  return KateDocumentConfig::imNormal;
}

KateAutoIndent::KateAutoIndent (KateDocument *_doc)
 : doc(_doc)
{
}
KateAutoIndent::~KateAutoIndent ()
{
}

void KateAutoIndent::updateConfig ()
{
  KateDocumentConfig *config = doc->config();

  useSpaces   = config->configFlags() & KateDocument::cfSpaceIndent;
  tabWidth    = config->tabWidth();
  indentWidth = (useSpaces) ? config->indentationWidth() : tabWidth;
}

bool KateAutoIndent::isBalanced (KateDocCursor &begin, const KateDocCursor &end, QChar open, QChar close) const
{
  uint parenOpen = 0;
  int curLine = begin.line();
  uchar attrib = 0;
  bool parenFound = false;

  TextLine::Ptr textLine = doc->kateTextLine(curLine);

  // Iterate one-by-one finding opening and closing chars
  // We assume that the opening and ending chars appear in same context
  // meaning we can check their attribute to skip comments and strings etc.
  while (begin.validPosition() && begin < end)
  {
    if (curLine != begin.line())
    {
      curLine = begin.line();
      textLine = doc->kateTextLine(curLine);
    }
    QChar c = begin.currentChar();
    if (c == open)
    {
      if (!parenFound) // assume first open encountered is the good one
        attrib = textLine->attribute(begin.col());

      if (textLine->attribute(begin.col()) != attrib)
      {
        begin.moveForward(1);
        continue;
      }

      parenOpen ++;
      parenFound = true;
    }
    else if (c == close && textLine->attribute(begin.col()) == attrib)
    {
      parenOpen --;
    }
    else if (!parenFound && !c.isSpace())
    {
      return false;
    }

    if (parenFound && parenOpen <= 0)
    {
      return true;
    }

    begin.moveForward(1);
  }

  return false;
}

bool KateAutoIndent::skipBlanks (KateDocCursor &cur, KateDocCursor &max, bool newline) const
{
  int beginLine = cur.line();
  if (newline)
    cur.moveForward(1);

  if (cur == max)
    return false;
  while (cur < max)
  {
    if (!newline && cur.line() != beginLine)
      break;

    QChar c = cur.currentChar();
    if (c == '/')
    {
      cur.setCol(cur.col() + 1);
      if (cur.currentChar() == '/')
        cur.gotoNextLine();
    }
    else if ((!c.isNull() && !c.isSpace()) || !cur.validPosition())
    {
      break;
    }

    // Make sure col is 0 if we spill into next line  i.e. count the '\n' as a character
    int tline = cur.line();
    cur.moveForward(1);
    if (cur.line() != tline)
      cur.setCol(0);
  }

  if (cur > max)
    cur = max;
  return cur.validPosition();
}

uint KateAutoIndent::measureIndent (KateDocCursor &cur) const
{
  if (useSpaces)
    return cur.col();

  return doc->kateTextLine(cur.line())->cursorX(cur.col(), tabWidth);
}

QString KateAutoIndent::tabString(uint pos) const
{
  QString s;
  if (!useSpaces)
  {
    while (pos >= tabWidth)
    {
      s += '\t';
      pos -= tabWidth;
    }
  }
  while (pos > 0)
  {
    s += ' ';
    pos--;
  }
  return s;
}

void KateAutoIndent::processNewline (KateDocCursor &begin, bool /*needContinue*/)
{
  int line = begin.line() - 1;
  int pos = begin.col();

  while ((line > 0) && (pos < 0)) // search a not empty text line
    pos = doc->kateTextLine(--line)->firstChar();

  if (pos > 0)
  {
    uint indent = doc->kateTextLine(line)->cursorX(pos, tabWidth);
    QString filler = tabString (indent);
    doc->insertText (begin.line(), 0, filler);
    begin.setCol(filler.length());
  }
  else
    begin.setCol(0);
}


// ----------------------------------------------------------------------------
KateCSmartIndent::KateCSmartIndent (KateDocument *doc)
 :  KateAutoIndent (doc),
    allowSemi (false)
{

}

KateCSmartIndent::~KateCSmartIndent ()
{

}

void KateCSmartIndent::processNewline (KateDocCursor &begin, bool needContinue)
{
  begin.setCol(0);
  uint indent = calcIndent (begin, needContinue);

  QString filler = tabString (indent);
  doc->insertText (begin.line(), 0, filler);
  begin.setCol(filler.length());
}

void KateCSmartIndent::processChar(QChar c)
{
  if (c != '}' && c != '{' && c != '#' && c != ':')
    return;
  KateView *view = doc->activeView();
  KateDocCursor begin(view->cursorLine(), view->cursorColumnReal() - 1, doc);

  // Make sure this is the only character on the line if it isn't a ':'
  TextLine::Ptr textLine = doc->kateTextLine(begin.line());
  if (c != ':')
  {
    if (textLine->firstChar() != begin.col())
      return;
  }

  // For # directives just remove the entire beginning of the line
  if (c == '#')
  {
    doc->removeText(begin.line(), 0, begin.line(), begin.col());
  }
  else if (c == ':')  // For a ':' line everything up :)
  {
    int lineStart = textLine->firstChar();
    if (textLine->stringAtPos (lineStart, "public") ||
        textLine->stringAtPos (lineStart, "private") ||
        textLine->stringAtPos (lineStart, "protected") ||
        textLine->stringAtPos (lineStart, "case"))
    {
      int line = begin.line();
      int pos = 0;
      while (line > 0) // search backwards until we find an ending ':' and go from there
      {
        textLine = doc->kateTextLine(--line);
        pos = textLine->lastChar();
        if (pos >= 0 && textLine->getChar(pos) == '{')
          return;
        if (pos >= 0 && textLine->getChar(pos) == ':')
          break;
      }

      KateDocCursor temp(line, textLine->firstChar(), doc);
      doc->removeText(begin.line(), 0, begin.line(), lineStart);
      doc->insertText(begin.line(), 0, tabString( measureIndent(temp) ));
    }
  }
  else  // Must be left with a brace. Put it where it belongs
  {
    int indent = calcIndent(begin, false);
    if (c == '}')
    {
      if (indent - (int)indentWidth >= 0)
        indent -= indentWidth;
    }

    if (indent > (int)measureIndent(begin))
      indent = measureIndent(begin);

    doc->removeText(begin.line(), 0, begin.line(), begin.col());
    doc->insertText(begin.line(), 0, tabString(indent));
  }
}

uint KateCSmartIndent::calcIndent(KateDocCursor &begin, bool needContinue)
{
  TextLine::Ptr textLine;
  KateDocCursor cur = begin;

  uint anchorIndent = 0;
  int anchorPos = 0;
  bool found = false;
  bool isSpecial = false;

  // Find Indent Anchor Point
  while (cur.gotoPreviousLine())
  {
    isSpecial = found = false;

    textLine = doc->kateTextLine(cur.line());
    QChar c = textLine->getChar (textLine->lastChar());
    if (c == ';' || c == '{' || c == '}' || c == ':')
    {
      found = true;

      int specialIndent = 0;
      if (c == ':' && needContinue)
      {
        QChar ch;
        if (textLine->stringAtPos(specialIndent = textLine->firstChar(), "case"))
          ch = textLine->getChar(specialIndent + 4);
        else if (textLine->stringAtPos(specialIndent = textLine->firstChar(), "public"))
          ch = textLine->getChar(specialIndent + 6);
        else if (textLine->stringAtPos(specialIndent = textLine->firstChar(), "private"))
          ch = textLine->getChar(specialIndent + 7);
        else if (textLine->stringAtPos(specialIndent = textLine->firstChar(), "protected"))
          ch = textLine->getChar(specialIndent + 9);

        if (ch.isNull() || (!ch.isSpace() && ch != '(' && ch != ':'))
          continue;

        KateDocCursor temp = cur;
        temp.setCol(specialIndent);
        specialIndent = measureIndent(temp);
        isSpecial = true;
      }

      // Move forward past blank lines
      KateDocCursor skip = cur;
      skip.moveForward(textLine->lastChar());
      bool result = skipBlanks(skip, begin, true);

      anchorPos = skip.col();
      anchorIndent = measureIndent(skip);

      // Accept if it's before requested position or if it was special
      if (result && skip < begin)
      {
        cur = skip;
        break;
      }
      else if (isSpecial)
      {
        anchorIndent = specialIndent;
        break;
      }

      // Are these on a line by themselves? (i.e. both last and first char)
      if ((c == '{' || c == '}') && textLine->getChar(textLine->firstChar()) == c)
      {
        cur.setCol(anchorPos = textLine->firstChar());
        anchorPos = cur.col();
        anchorIndent = measureIndent (cur);
        break;
      }
    }
  }

  if (!found)
    return 0;

  uint continueIndent = (needContinue) ? calcContinue (cur, begin) : 0;

  // Move forward from anchor and determine
  QChar lastChar = textLine->getChar (anchorPos);
  if (cur < begin)
  {
    do
    {
      if (!skipBlanks(cur, begin, true))
        return 0;
      if (cur == begin || cur.currentChar().isNull())
        break;

      if (!cur.currentChar().isSpace() && cur <= begin)
        lastChar = cur.currentChar();
    } while (cur.validPosition() && cur <= begin);
  }

  uint indent = 0;
  if (lastChar == '{' || (lastChar == ':' && isSpecial && needContinue))
  {
    indent = anchorIndent + indentWidth;
  }
  else if (lastChar == '}')
  {
    indent = anchorIndent;
  }
  else if (lastChar == ';')
  {
    indent = anchorIndent + ((allowSemi && needContinue) ? continueIndent : 0);
  }
  else if (!lastChar.isNull() && anchorIndent != 0)
  {
    indent = anchorIndent + continueIndent;
  }

  return indent;
}

uint KateCSmartIndent::calcContinue(KateDocCursor &start, KateDocCursor &end)
{
  KateDocCursor cur = start;

  bool needsBalanced = false;
  bool isFor = false;
  allowSemi = false;

  TextLine::Ptr textLine = doc->kateTextLine(cur.line());
  uint length = textLine->length();

  if (textLine->getChar(cur.col()) == '}')
  {
    skipBlanks(cur, end, true);
    if (cur.line() != start.line())
      textLine = doc->kateTextLine(cur.line());

    if (textLine->stringAtPos(cur.col(), "else"))
      cur.setCol(cur.col() + 4);
    else
      return indentWidth * 2;
  }
  else if (textLine->stringAtPos(cur.col(), "else"))
  {
    cur.setCol(cur.col() + 4);
    if (textLine->stringAtPos(textLine->nextNonSpaceChar(cur.col()), "if"))
    {
      cur.setCol(textLine->nextNonSpaceChar(cur.col()) + 2);
      needsBalanced = true;
    }
  }
  else if (textLine->stringAtPos(cur.col(), "do"))
  {
    cur.setCol(cur.col() + 2);
  }
  else if (textLine->stringAtPos(cur.col(), "for"))
  {
    cur.setCol(cur.col() + 3);
    isFor = needsBalanced = true;
  }
  else if (textLine->stringAtPos(cur.col(), "if"))
  {
    cur.setCol(cur.col() + 2);
    needsBalanced = true;
  }
  else if (textLine->stringAtPos(cur.col(), "while"))
  {
    cur.setCol(cur.col() + 5);
    needsBalanced = true;
  }
  else
    return indentWidth * 2;

  if (needsBalanced && !isBalanced (cur, end, QChar('('), QChar(')')))
  {
    allowSemi = isFor;
    return indentWidth * 2;
  }
  // Check if this statement ends a line now
  skipBlanks(cur, end, false);
  if (cur == end || (cur.col() == (int)length-1))
    return indentWidth;

  if (skipBlanks(cur, end, true))
  {
    if (cur == end)
      return indentWidth;
    else
      return indentWidth + calcContinue(cur, end);
  }

  return 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on; indent-mode 1;
