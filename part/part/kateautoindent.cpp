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
#include "katehighlight.h"
#include "kateview.h"

#include <klocale.h>
#include <kdebug.h>

// BEGIN KateAutoIndent

KateAutoIndent *KateAutoIndent::createIndenter (KateDocument *doc, uint mode)
{
  if (mode == KateDocumentConfig::imCStyle)
    return new KateCSmartIndent (doc);
  else if (mode == KateDocumentConfig::imPythonStyle)
    return new KatePythonIndent (doc);
  else if (mode == KateDocumentConfig::imXmlStyle)
    return new KateXmlIndent (doc);

  return new KateAutoIndent (doc);
}

QStringList KateAutoIndent::listModes ()
{
  QStringList l;

  l << modeDescription(KateDocumentConfig::imNormal);
  l << modeDescription(KateDocumentConfig::imCStyle);
  l << modeDescription(KateDocumentConfig::imPythonStyle);
  l << modeDescription(KateDocumentConfig::imXmlStyle);

  return l;
}

QString KateAutoIndent::modeName (uint mode)
{
  if (mode == KateDocumentConfig::imCStyle)
    return QString ("cstyle");
  else if (mode == KateDocumentConfig::imPythonStyle)
    return QString ("python");
  else if (mode == KateDocumentConfig::imXmlStyle)
    return QString ("xml");

  return QString ("normal");
}

QString KateAutoIndent::modeDescription (uint mode)
{
  if (mode == KateDocumentConfig::imCStyle)
    return i18n ("C Style");
  else if (mode == KateDocumentConfig::imPythonStyle)
    return i18n ("Python Style");
  else if (mode == KateDocumentConfig::imXmlStyle)
    return i18n ("XML Style");

  return i18n ("Normal");
}

uint KateAutoIndent::modeNumber (const QString &name)
{
  if (modeName(KateDocumentConfig::imCStyle) == name)
    return KateDocumentConfig::imCStyle;
  else if (modeName(KateDocumentConfig::imPythonStyle) == name)
    return KateDocumentConfig::imPythonStyle;
  else if (modeName(KateDocumentConfig::imXmlStyle) == name)
    return KateDocumentConfig::imXmlStyle;

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

  useSpaces   = config->configFlags() & KateDocument::cfSpaceIndent || config->configFlags() & KateDocumentConfig::cfReplaceTabsDyn;
  keepProfile = config->configFlags() & KateDocument::cfKeepIndentProfile;
  tabWidth    = config->tabWidth();
  indentWidth = (config->configFlags() & KateDocument::cfSpaceIndent) ? config->indentationWidth() : tabWidth;

  commentAttrib = 255;
  doxyCommentAttrib = 255;
  regionAttrib = 255;
  symbolAttrib = 255;
  alertAttrib = 255;
  tagAttrib = 255;
  wordAttrib = 255;

  KateHlItemDataList items;
  doc->highlight()->getKateHlItemDataListCopy (0, items);

  for (uint i=0; i<items.count(); i++)
  {
    QString name = items.at(i)->name;
    if (name.find("Comment") != -1 && commentAttrib == 255)
    {
      commentAttrib = i;
    }
    else if (name.find("Region Marker") != -1 && regionAttrib == 255)
    {
      regionAttrib = i;
    }
    else if (name.find("Symbol") != -1 && symbolAttrib == 255)
    {
      symbolAttrib = i;
    }
    else if (name.find("Alert") != -1)
    {
      alertAttrib = i;
    }
    else if (name.find("Comment") != -1 && commentAttrib != 255 && doxyCommentAttrib == 255)
    {
      doxyCommentAttrib = i;
    }
    else if (name.find("Tags") != -1 && tagAttrib == 255)
    {
      tagAttrib = i;
    }
    else if (name.find("Word") != -1 && wordAttrib == 255)
    {
      wordAttrib = i;
    }
  }
}

bool KateAutoIndent::isBalanced (KateDocCursor &begin, const KateDocCursor &end, QChar open, QChar close, uint &pos) const
{
  int parenOpen = 0;
  bool atLeastOne = false;
  bool getNext = false;

  pos = doc->plainKateTextLine(begin.line())->firstChar();

  // Iterate one-by-one finding opening and closing chars
  // Assume that open and close are 'Symbol' characters
  while (begin < end)
  {
    QChar c = begin.currentChar();
    if (begin.currentAttrib() == symbolAttrib)
    {
      if (c == open)
      {
        if (!atLeastOne)
        {
          atLeastOne = true;
          getNext = true;
          pos = measureIndent(begin) + 1;
        }
        parenOpen++;
      }
      else if (c == close)
      {
        parenOpen--;
      }
    }
    else if (getNext && !c.isSpace())
    {
      getNext = false;
      pos = measureIndent(begin);
    }

    if (atLeastOne && parenOpen <= 0)
      return true;

    begin.moveForward(1);
  }

  return (atLeastOne) ? false : true;
}

bool KateAutoIndent::skipBlanks (KateDocCursor &cur, KateDocCursor &max, bool newline) const
{
  int curLine = cur.line();
  if (newline)
    cur.moveForward(1);

  if (cur >= max)
    return false;

  do
  {
    uchar attrib = cur.currentAttrib();
    if (attrib != commentAttrib && attrib != doxyCommentAttrib && attrib != regionAttrib && attrib != alertAttrib && attrib != tagAttrib && attrib != wordAttrib)
    {
      QChar c = cur.currentChar();
      if (!c.isNull() && !c.isSpace())
        break;
    }

    // Make sure col is 0 if we spill into next line  i.e. count the '\n' as a character
    if (!cur.moveForward(1))
      break;
    if (curLine != cur.line())
    {
      if (!newline)
        break;
      curLine = cur.line();
      cur.setCol(0);
    }
  } while (cur < max);

  if (cur > max)
    cur = max;
  return true;
}

uint KateAutoIndent::measureIndent (KateDocCursor &cur) const
{
  if (useSpaces && !keepProfile)
    return cur.col();

  return doc->plainKateTextLine(cur.line())->cursorX(cur.col(), tabWidth);
}

QString KateAutoIndent::tabString(uint pos) const
{
  QString s;
  pos = QMIN (pos, 80); // sanity check for large values of pos

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
    pos = doc->plainKateTextLine(--line)->firstChar();

  if (pos > 0)
  {
    uint indent = doc->plainKateTextLine(line)->cursorX(pos, tabWidth);
    QString filler = tabString (indent);
    doc->insertText (begin.line(), 0, filler);
    begin.setCol(filler.length());
  }
  else
    begin.setCol(0);
}

//END

// BEGIN KateCSmartIndent

KateCSmartIndent::KateCSmartIndent (KateDocument *doc)
 :  KateAutoIndent (doc),
    allowSemi (false),
    processingBlock (false)
{

}

KateCSmartIndent::~KateCSmartIndent ()
{

}

void KateCSmartIndent::processLine (KateDocCursor &line)
{
  KateTextLine::Ptr textLine = doc->plainKateTextLine(line.line());

  int firstChar = textLine->firstChar();
  // Empty line is worthless ... but only when doing more than 1 line
  if (firstChar == -1 && processingBlock)
    return;

  uint indent = 0;

  // TODO Here we do not check for beginning and ending comments ...
  QChar first = textLine->getChar(firstChar);
  QChar last = textLine->getChar(textLine->lastChar());

  if (first == '}')
  {
    indent = findOpeningBrace(line);
  }
  else if (first == ')')
  {
    indent = findOpeningParen(line);
  }
  else if (first == '{')
  {
    // If this is the first brace, we keep the indent at 0
    KateDocCursor temp(line.line(), firstChar, doc);
    if (!firstOpeningBrace(temp))
      indent = calcIndent(temp, false);
  }
  else if (first == ':')
  {
    // Initialization lists (handle c++ and c#)
    int pos = findOpeningBrace(line);
    if (pos == 0)
      indent = indentWidth;
    else
      indent = pos + (indentWidth * 2);
  }
  else if (last == ':')
  {
    if (textLine->stringAtPos (firstChar, "case") ||
        textLine->stringAtPos (firstChar, "default") ||
        textLine->stringAtPos (firstChar, "public") ||
        textLine->stringAtPos (firstChar, "private") ||
        textLine->stringAtPos (firstChar, "protected") ||
        textLine->stringAtPos (firstChar, "signals") ||
        textLine->stringAtPos (firstChar, "slots"))
    {
      indent = findOpeningBrace(line) + indentWidth;
    }
  }
  else if (first == '*')
  {
    if (last == '/')
    {
      int lineEnd = textLine->lastChar();
      if (lineEnd > 0 && textLine->getChar(lineEnd - 1) == '*')
      {
        indent = findOpeningComment(line);
        if (textLine->attribute(firstChar) == doxyCommentAttrib)
          indent++;
      }
      else
        return;
    }
    else
    {
      KateDocCursor temp = line;
      if (textLine->attribute(firstChar) == doxyCommentAttrib)
        indent = calcIndent(temp, false) + 1;
      else
        indent = calcIndent(temp, true);
    }
  }
  else if (first == '#')
  {
    // c# regions
    if (textLine->stringAtPos (firstChar, "#region") ||
        textLine->stringAtPos (firstChar, "#endregion"))
    {
      KateDocCursor temp = line;
      indent = calcIndent(temp, true);
    }
  }
  else
  {
    // Everything else ...
    if (first == '/' && last != '/')
      return;

    KateDocCursor temp = line;
    indent = calcIndent(temp, true);
    if (indent == 0)
    {
      KateAutoIndent::processNewline(line, true);
      return;
    }
  }

  // Slightly faster if we don't indent what we don't have to
  if (indent != measureIndent(line) || first == '}' || first == '{' || first == '#')
  {
    doc->removeText(line.line(), 0, line.line(), firstChar);
    QString filler = tabString(indent);
    if (indent > 0) doc->insertText(line.line(), 0, filler);
    if (!processingBlock) line.setCol(filler.length());
  }
}

void KateCSmartIndent::processSection (KateDocCursor &begin, KateDocCursor &end)
{
  KateDocCursor cur = begin;
  QTime t;
  t.start();

  processingBlock = (end.line() - cur.line() > 0) ? true : false;

  while (cur.line() <= end.line())
  {
    processLine (cur);
    if (!cur.gotoNextLine())
      break;
  }

  processingBlock = false;
  kdDebug(13000) << "+++ total: " << t.elapsed() << endl;
}

bool KateCSmartIndent::handleDoxygen (KateDocCursor &begin)
{
  // Factor out the rather involved Doxygen stuff here ...
  int line = begin.line();
  int first = -1;
  while ((line > 0) && (first < 0))
    first = doc->plainKateTextLine(--line)->firstChar();

  if (first >= 0)
  {
    KateTextLine::Ptr textLine = doc->plainKateTextLine(line);
    bool insideDoxygen = false;
    if (textLine->attribute(first) == doxyCommentAttrib || textLine->attribute(textLine->lastChar()) == doxyCommentAttrib)
    {
      if (!textLine->endingWith("*/"))
        insideDoxygen = true;
    }

    // Align the *'s and then go ahead and insert one too ...
    if (insideDoxygen)
    {
      textLine = doc->plainKateTextLine(begin.line());
      first = textLine->firstChar();
      int indent = findOpeningComment(begin);
      QString filler = tabString (indent);

      bool doxygenAutoInsert = doc->config()->configFlags() & KateDocumentConfig::cfDoxygenAutoTyping;
      if ( doxygenAutoInsert &&
           (!textLine->stringAtPos(first, "*/") && !textLine->stringAtPos(first, "*")))
      {
        filler = filler + " * ";
      }

      doc->removeText (begin.line(), 0, begin.line(), first);
      doc->insertText (begin.line(), 0, filler);
      begin.setCol(filler.length());

      return true;
    }
  }

  return false;
}

void KateCSmartIndent::processNewline (KateDocCursor &begin, bool needContinue)
{
  if (!handleDoxygen (begin))
  {
    KateTextLine::Ptr textLine = doc->plainKateTextLine(begin.line());
    bool inMiddle = textLine->firstChar() > -1;

    int indent = calcIndent (begin, needContinue);

    if (indent > 0 || inMiddle)
    {
      QString filler = tabString (indent);
      doc->insertText (begin.line(), 0, filler);
      begin.setCol(filler.length());

      // Handles cases where user hits enter at the beginning or middle of text
      if (inMiddle)
      {
        processLine(begin);
        begin.setCol(textLine->firstChar());
      }
    }
    else
    {
      KateAutoIndent::processNewline (begin, needContinue);
      begin.setCol(begin.col() - 1);
    }

    if (begin.col() < 0)
      begin.setCol(0);
  }
}

void KateCSmartIndent::processChar(QChar c)
{
  static const QString triggers("}{)/:;#n");
  if (triggers.find(c) == -1)
    return;

  KateView *view = doc->activeView();
  KateDocCursor begin(view->cursorLine(), 0, doc);

  if (c == 'n')
  {
    KateTextLine::Ptr textLine = doc->plainKateTextLine(begin.line());
    if (textLine->getChar(textLine->firstChar()) != '#')
      return;
  }

  processLine(begin);
}

uint KateCSmartIndent::calcIndent(KateDocCursor &begin, bool needContinue)
{
  KateTextLine::Ptr textLine;
  KateDocCursor cur = begin;

  uint anchorIndent = 0;
  int anchorPos = 0;
  int parenCount = 0;  // Possibly in a multiline for stmt.  Used to skip ';' ...
  bool found = false;
  bool isSpecial = false;

  //kdDebug() << "calcIndent begin line:" << begin.line() << " col:" << begin.col() << endl;

  // Find Indent Anchor Point
  while (cur.gotoPreviousLine())
  {
    isSpecial = found = false;
    textLine = doc->plainKateTextLine(cur.line());

    // Skip comments and handle cases like if (...) { stmt;
    int pos = textLine->lastChar();
    int openCount = 0;
    int otherAnchor = -1;
    do
    {
      if (textLine->attribute(pos) == symbolAttrib)
      {
        QChar tc = textLine->getChar (pos);
        if ((tc == ';' || tc == ':' || tc == ',') && otherAnchor == -1 && parenCount <= 0)
          otherAnchor = pos;
        else if (tc == ')')
          parenCount++;
        else if (tc == '(')
          parenCount--;
        else if (tc == '}')
          openCount--;
        else if (tc == '{')
        {
          openCount++;
          if (openCount == 1)
            break;
        }
      }
    } while (--pos >= textLine->firstChar());

    if (openCount != 0 || otherAnchor != -1)
    {
      found = true;
      QChar c;
      if (openCount > 0)
        c = '{';
      else if (openCount < 0)
        c = '}';
      else if (otherAnchor >= 0)
        c = textLine->getChar (otherAnchor);

      int specialIndent = 0;
      if (c == ':' && needContinue)
      {
        QChar ch;
        specialIndent = textLine->firstChar();
        if (textLine->stringAtPos(specialIndent, "case"))
          ch = textLine->getChar(specialIndent + 4);
        else if (textLine->stringAtPos(specialIndent, "default"))
          ch = textLine->getChar(specialIndent + 7);
        else if (textLine->stringAtPos(specialIndent, "public"))
          ch = textLine->getChar(specialIndent + 6);
        else if (textLine->stringAtPos(specialIndent, "private"))
          ch = textLine->getChar(specialIndent + 7);
        else if (textLine->stringAtPos(specialIndent, "protected"))
          ch = textLine->getChar(specialIndent + 9);
        else if (textLine->stringAtPos(specialIndent, "signals"))
          ch = textLine->getChar(specialIndent + 7);
        else if (textLine->stringAtPos(specialIndent, "slots"))
          ch = textLine->getChar(specialIndent + 5);

        if (ch.isNull() || (!ch.isSpace() && ch != '(' && ch != ':'))
          continue;

        KateDocCursor lineBegin = cur;
        lineBegin.setCol(specialIndent);
        specialIndent = measureIndent(lineBegin);
        isSpecial = true;
      }

      // Move forward past blank lines
      KateDocCursor skip = cur;
      skip.setCol(textLine->lastChar());
      bool result = skipBlanks(skip, begin, true);

      anchorPos = skip.col();
      anchorIndent = measureIndent(skip);

      //kdDebug() << "calcIndent anchorPos:" << anchorPos << " anchorIndent:" << anchorIndent << " at line:" << skip.line() << endl;

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
        anchorIndent = measureIndent (cur);
        break;
      }
    }
  }

  if (!found)
    return 0;

  uint continueIndent = (needContinue) ? calcContinue (cur, begin) : 0;
  //kdDebug() << "calcIndent continueIndent:" << continueIndent << endl;

  // Move forward from anchor and determine last known reference character
  // Braces take precedance over others ...
  textLine = doc->plainKateTextLine(cur.line());
  QChar lastChar = textLine->getChar (anchorPos);
  int lastLine = cur.line();
  if (lastChar == '#' || lastChar == '[')
  {
    // Never continue if # or [ is encountered at this point here
    // A fail-safe really... most likely an #include, #region, or a c# attribute
    continueIndent = 0;
  }

  int openCount = 0;
  while (cur.validPosition() && cur < begin)
  {
    if (!skipBlanks(cur, begin, true))
      return 0;

    QChar tc = cur.currentChar();
    //kdDebug() << "  cur.line:" << cur.line() << " cur.col:" << cur.col() << " currentChar '" << tc << "' " << textLine->attribute(cur.col()) << endl;
    if (cur == begin || tc.isNull())
      break;

    if (!tc.isSpace() && cur < begin)
    {
      uchar attrib = cur.currentAttrib();
      if (tc == '{' && attrib == symbolAttrib)
        openCount++;
      else if (tc == '}' && attrib == symbolAttrib)
        openCount--;

      lastChar = tc;
      lastLine = cur.line();
    }
  }
  if (openCount > 0) // Open braces override
    lastChar = '{';

  uint indent = 0;
  //kdDebug() << "calcIndent lastChar '" << lastChar << "'" << endl;

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
  else if (lastChar == ',')
  {
    textLine = doc->plainKateTextLine(lastLine);
    KateDocCursor start(lastLine, textLine->firstChar(), doc);
    KateDocCursor finish(lastLine, textLine->lastChar(), doc);
    uint pos = 0;

    if (isBalanced(start, finish, QChar('('), QChar(')'), pos))
      indent = anchorIndent;
    else
    {
      // TODO: Config option. If we're below 48, go ahead and line them up
      indent = ((pos < 48) ? pos : anchorIndent + (indentWidth * 2));
    }
  }
  else if (!lastChar.isNull())
  {
    if (anchorIndent != 0)
      indent = anchorIndent + continueIndent;
    else
      indent = continueIndent;
  }

  return indent;
}

uint KateCSmartIndent::calcContinue(KateDocCursor &start, KateDocCursor &end)
{
  KateDocCursor cur = start;

  bool needsBalanced = true;
  bool isFor = false;
  allowSemi = false;

  KateTextLine::Ptr textLine = doc->plainKateTextLine(cur.line());

  // Handle cases such as  } while (s ... by skipping the leading symbol
  if (textLine->attribute(cur.col()) == symbolAttrib)
  {
    cur.moveForward(1);
    skipBlanks(cur, end, false);
  }

  if (textLine->getChar(cur.col()) == '}')
  {
    skipBlanks(cur, end, true);
    if (cur.line() != start.line())
      textLine = doc->plainKateTextLine(cur.line());

    if (textLine->stringAtPos(cur.col(), "else"))
      cur.setCol(cur.col() + 4);
    else
      return indentWidth * 2;

    needsBalanced = false;
  }
  else if (textLine->stringAtPos(cur.col(), "else"))
  {
    cur.setCol(cur.col() + 4);
    needsBalanced = false;
    if (textLine->stringAtPos(textLine->nextNonSpaceChar(cur.col()), "if"))
    {
      cur.setCol(textLine->nextNonSpaceChar(cur.col()) + 2);
      needsBalanced = true;
    }
  }
  else if (textLine->stringAtPos(cur.col(), "if"))
  {
    cur.setCol(cur.col() + 2);
  }
  else if (textLine->stringAtPos(cur.col(), "do"))
  {
    cur.setCol(cur.col() + 2);
    needsBalanced = false;
  }
  else if (textLine->stringAtPos(cur.col(), "for"))
  {
    cur.setCol(cur.col() + 3);
    isFor = true;
  }
  else if (textLine->stringAtPos(cur.col(), "while"))
  {
    cur.setCol(cur.col() + 5);
  }
  else if (textLine->stringAtPos(cur.col(), "switch"))
  {
    cur.setCol(cur.col() + 6);
  }
  else if (textLine->stringAtPos(cur.col(), "using"))
  {
    cur.setCol(cur.col() + 5);
  }
  else
  {
    return indentWidth * 2;
  }

  uint openPos = 0;
  if (needsBalanced && !isBalanced (cur, end, QChar('('), QChar(')'), openPos))
  {
    allowSemi = isFor;
    if (openPos > 0)
      return (openPos - textLine->firstChar());
    else
      return indentWidth * 2;
  }

  // Check if this statement ends a line now
  skipBlanks(cur, end, false);
  if (cur == end)
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

uint KateCSmartIndent::findOpeningBrace(KateDocCursor &start)
{
  KateDocCursor cur = start;
  int count = 1;

  // Move backwards 1 by 1 and find the opening brace
  // Return the indent of that line
  while (cur.moveBackward(1))
  {
    if (cur.currentAttrib() == symbolAttrib)
    {
      QChar ch = cur.currentChar();
      if (ch == '{')
        count--;
      else if (ch == '}')
        count++;

      if (count == 0)
      {
        KateDocCursor temp(cur.line(), doc->plainKateTextLine(cur.line())->firstChar(), doc);
        return measureIndent(temp);
      }
    }
  }

  return 0;
}

bool KateCSmartIndent::firstOpeningBrace(KateDocCursor &start)
{
  KateDocCursor cur = start;

  // Are we the first opening brace at this level?
  while(cur.moveBackward(1))
  {
    if (cur.currentAttrib() == symbolAttrib)
    {
      QChar ch = cur.currentChar();
      if (ch == '{')
        return false;
      else if (ch == '}' && cur.col() == 0)
        break;
    }
  }

  return true;
}

uint KateCSmartIndent::findOpeningParen(KateDocCursor &start)
{
  KateDocCursor cur = start;
  int count = 1;

  // Move backwards 1 by 1 and find the opening (
  // Return the indent of that line
  while (cur.moveBackward(1))
  {
    if (cur.currentAttrib() == symbolAttrib)
    {
      QChar ch = cur.currentChar();
      if (ch == '(')
        count--;
      else if (ch == ')')
        count++;

      if (count == 0)
        return measureIndent(cur);
    }
  }

  return 0;
}

uint KateCSmartIndent::findOpeningComment(KateDocCursor &start)
{
  KateDocCursor cur = start;

  // Find the line with the opening /* and return the proper indent
  do
  {
    KateTextLine::Ptr textLine = doc->plainKateTextLine(cur.line());

    int pos = textLine->string().find("/*", false);
    if (pos >= 0)
    {
      KateDocCursor temp(cur.line(), pos, doc);
      return measureIndent(temp);
    }

  } while (cur.gotoPreviousLine());

  return 0;
}

// END

// BEGIN KatePythonIndent

QRegExp KatePythonIndent::endWithColon = QRegExp( "^[^#]*:\\s*(#.*)?$" );
QRegExp KatePythonIndent::stopStmt = QRegExp( "^\\s*(break|continue|raise|return|pass)\\b.*" );
QRegExp KatePythonIndent::blockBegin = QRegExp( "^\\s*(def|if|elif|else|for|while|try)\\b.*" );

KatePythonIndent::KatePythonIndent (KateDocument *doc)
  : KateAutoIndent (doc)
{
}
KatePythonIndent::~KatePythonIndent ()
{
}

void KatePythonIndent::processNewline (KateDocCursor &begin, bool /*newline*/)
{
  int prevLine = begin.line() - 1;
  int prevPos = begin.col();

  while ((prevLine > 0) && (prevPos < 0)) // search a not empty text line
    prevPos = doc->plainKateTextLine(--prevLine)->firstChar();

  int prevBlock = prevLine;
  int prevBlockPos = prevPos;
  int extraIndent = calcExtra (prevBlock, prevBlockPos, begin);

  int indent = doc->plainKateTextLine(prevBlock)->cursorX(prevBlockPos, tabWidth);
  if (extraIndent == 0)
  {
    if (!stopStmt.exactMatch(doc->plainKateTextLine(prevLine)->string()))
    {
      if (endWithColon.exactMatch(doc->plainKateTextLine(prevLine)->string()))
        indent += indentWidth;
      else
        indent = doc->plainKateTextLine(prevLine)->cursorX(prevPos, tabWidth);
    }
  }
  else
    indent += extraIndent;

  if (indent > 0)
  {
    QString filler = tabString (indent);
    doc->insertText (begin.line(), 0, filler);
    begin.setCol(filler.length());
  }
  else
    begin.setCol(0);
}

int KatePythonIndent::calcExtra (int &prevBlock, int &pos, KateDocCursor &end)
{
  int nestLevel = 0;
  bool levelFound = false;
  while ((prevBlock > 0))
  {
    if (blockBegin.exactMatch(doc->plainKateTextLine(prevBlock)->string()))
    {
      if ((!levelFound && nestLevel == 0) || (levelFound && nestLevel - 1 <= 0))
      {
        pos = doc->plainKateTextLine(prevBlock)->firstChar();
        break;
      }

      nestLevel --;
    }
    else if (stopStmt.exactMatch(doc->plainKateTextLine(prevBlock)->string()))
    {
      nestLevel ++;
      levelFound = true;
    }

    --prevBlock;
  }

  KateDocCursor cur (prevBlock, pos, doc);
  QChar c;
  int extraIndent = 0;
  while (cur.line() < end.line())
  {
    c = cur.currentChar();

    if (c == '(')
      extraIndent += indentWidth;
    else if (c == ')')
      extraIndent -= indentWidth;
    else if (c == ':')
      break;

    if (c.isNull() || c == '#')
      cur.gotoNextLine();
    else
      cur.moveForward(1);
  }

  return extraIndent;
}

// END

// BEGIN KateXmlIndent

/* Explanation

The XML indenter simply inherits the indentation of the previous line,
with the first line starting at 0 (of course!). For each element that
is opened on the previous line, the indentation is increased by one
level; for each element that is closed, it is decreased by one.

We also have a special case of opening an element on one line and then
entering attributes on the following lines, in which case we would like
to see the following layout:
<elem attr="..."
      blah="..." />

<x><a href="..."
      title="..." />
</x>

This is accomplished by checking for lines that contain an unclosed open
tag.

*/

const QRegExp KateXmlIndent::startsWithCloseTag("^[ \t]*</");
const QRegExp KateXmlIndent::unclosedDoctype("<!DOCTYPE[^>]*$");

KateXmlIndent::KateXmlIndent (KateDocument *doc)
  : KateAutoIndent (doc)
{
}

KateXmlIndent::~KateXmlIndent ()
{
}

void KateXmlIndent::processNewline (KateDocCursor &begin, bool /*newline*/)
{
  begin.setCol(processLine(begin.line()));
}

void KateXmlIndent::processChar (QChar c)
{
  if(c != '/') return;

  // only alter lines that start with a close element
  KateView *view = doc->activeView();
  QString text = doc->plainKateTextLine(view->cursorLine())->string();
  if(text.find(startsWithCloseTag) == -1) return;

  // process it
  processLine(view->cursorLine());
}

void KateXmlIndent::processLine (KateDocCursor &line)
{
  processLine (line.line());
}

void KateXmlIndent::processSection (KateDocCursor &begin, KateDocCursor &end)
{
  uint endLine = end.line();
  for(uint line = begin.line(); line <= endLine; ++line) processLine(line);
}

void KateXmlIndent::getLineInfo (uint line, uint &prevIndent, int &numTags,
  uint &attrCol, bool &unclosedTag)
{
  prevIndent = 0;
  int firstChar;
  KateTextLine::Ptr prevLine = 0;

  // get the indentation of the first non-empty line
  while(true) {
    prevLine = doc->plainKateTextLine(line);
    if( (firstChar = prevLine->firstChar()) < 0) {
      if(!line--) return;
      continue;
    }
    break;
  }
  prevIndent = prevLine->cursorX(prevLine->firstChar(), tabWidth);
  QString text = prevLine->string();

  // special case:
  // <a>
  // </a>              <!-- indentation *already* decreased -->
  // requires that we discount the </a> from the number of closed tags
  if(text.find(startsWithCloseTag) != -1) ++numTags;

  // count the number of open and close tags
  int lastCh = 0;
  uint pos, len = text.length();
  bool seenOpen = false;
  for(pos = 0; pos < len; ++pos) {
    int ch = text.at(pos).unicode();
    switch(ch) {
      case '<':
        seenOpen = true;
        unclosedTag = true;
        attrCol = pos;
        ++numTags;
        break;

      // don't indent because of DOCTYPE, comment, CDATA, etc.
      case '!':
        if(lastCh == '<') --numTags;
        break;

      // don't indent because of xml decl or PI
      case '?':
        if(lastCh == '<') --numTags;
        break;

      case '>':
        if(!seenOpen) {
          // we are on a line like the second one here:
          // <element attr="val"
          //          other="val">
          // so we need to set prevIndent to the indent of the first line
          //
          // however, we need to special case "<!DOCTYPE" because
          // it's not an open tag

          prevIndent = 0;

          for(uint backLine = line; backLine; ) {
            // find first line with an open tag
            KateTextLine::Ptr x = doc->plainKateTextLine(--backLine);
            if(x->string().find('<') == -1) continue;

            // recalculate the indent
            if(x->string().find(unclosedDoctype) != -1) --numTags;
            getLineInfo(backLine, prevIndent, numTags, attrCol, unclosedTag);
            break;
          }
        }
        if(lastCh == '/') --numTags;
        unclosedTag = false;
        break;

      case '/':
        if(lastCh == '<') numTags -= 2; // correct for '<', above
        break;
    }
    lastCh = ch;
  }

  if(unclosedTag) {
    // find the start of the next attribute, so we can align with it
    do {
      lastCh = text.at(++attrCol).unicode();
    }while(lastCh && lastCh != ' ' && lastCh != '\t');

    while(lastCh == ' ' || lastCh == '\t') {
      lastCh = text.at(++attrCol).unicode();
    }

    attrCol = prevLine->cursorX(attrCol, tabWidth);
  }
}

uint KateXmlIndent::processLine (uint line)
{
  KateTextLine::Ptr kateLine = doc->plainKateTextLine(line);

  // get details from previous line
  uint prevIndent = 0, attrCol = 0;
  int numTags = 0;
  bool unclosedTag = false; // for aligning attributes

  if(line) {
    getLineInfo(line - 1, prevIndent, numTags, attrCol, unclosedTag);
  }

  // compute new indent
  int indent = 0;
  if(unclosedTag) indent = attrCol;
  else  indent = prevIndent + numTags * indentWidth;
  if(indent < 0) indent = 0;

  // unindent lines that start with a close tag
  if(kateLine->string().find(startsWithCloseTag) != -1) {
    indent -= indentWidth;
  }
  if(indent < 0) indent = 0;

  // apply new indent
  doc->removeText(line, 0, line, kateLine->firstChar());
  QString filler = tabString(indent);
  doc->insertText(line, 0, filler);

  return filler.length();
}

// END

// kate: space-indent on; indent-width 2; replace-tabs on;
