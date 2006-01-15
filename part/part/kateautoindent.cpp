/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>
   Copyright (C) 2004 >Anders Lund <anders@alweb.dk> (KateVarIndent class)
   Copyright (C) 2005 Dominik Haumann <dhdev@gmx.de> (basic support for config page)

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

#include "kateautoindent.h"
#include "kateautoindent.moc"

#include "kateconfig.h"
#include "katehighlight.h"
#include "kateglobal.h"
#include "katejscript.h"
#include "kateview.h"
#include "kateextendedattribute.h"

#include <klocale.h>
#include <kdebug.h>
#include <kmenu.h>

//BEGIN KateAutoIndent

KateAutoIndent *KateAutoIndent::createIndenter (KateDocument *doc, uint mode)
{
  if (mode == KateDocumentConfig::imNormal)
    return new KateNormalIndent (doc);
  else if (mode == KateDocumentConfig::imCStyle)
    return new KateCSmartIndent (doc);
  else if (mode == KateDocumentConfig::imPythonStyle)
    return new KatePythonIndent (doc);
  else if (mode == KateDocumentConfig::imXmlStyle)
    return new KateXmlIndent (doc);
  else if (mode == KateDocumentConfig::imCSAndS)
    return new KateCSAndSIndent (doc);
  else if ( mode == KateDocumentConfig::imVarIndent )
    return new KateVarIndent ( doc );
//  else if ( mode == KateDocumentConfig::imScriptIndent)
//    return new KateScriptIndent ( doc );

  return new KateAutoIndent (doc);
}

QStringList KateAutoIndent::listModes ()
{
  QStringList l;

  l << modeDescription(KateDocumentConfig::imNone);
  l << modeDescription(KateDocumentConfig::imNormal);
  l << modeDescription(KateDocumentConfig::imCStyle);
  l << modeDescription(KateDocumentConfig::imPythonStyle);
  l << modeDescription(KateDocumentConfig::imXmlStyle);
  l << modeDescription(KateDocumentConfig::imCSAndS);
  l << modeDescription(KateDocumentConfig::imVarIndent);
//  l << modeDescription(KateDocumentConfig::imScriptIndent);

  return l;
}

QString KateAutoIndent::modeName (uint mode)
{
  if (mode == KateDocumentConfig::imNormal)
    return QString ("normal");
  else if (mode == KateDocumentConfig::imCStyle)
    return QString ("cstyle");
  else if (mode == KateDocumentConfig::imPythonStyle)
    return QString ("python");
  else if (mode == KateDocumentConfig::imXmlStyle)
    return QString ("xml");
  else if (mode == KateDocumentConfig::imCSAndS)
    return QString ("csands");
  else if ( mode  == KateDocumentConfig::imVarIndent )
    return QString( "varindent" );
//  else if ( mode  == KateDocumentConfig::imScriptIndent )
//    return QString( "scriptindent" );

  return QString ("none");
}

QString KateAutoIndent::modeDescription (uint mode)
{
  if (mode == KateDocumentConfig::imNormal)
    return i18n ("Normal");
  else if (mode == KateDocumentConfig::imCStyle)
    return i18n ("C Style");
  else if (mode == KateDocumentConfig::imPythonStyle)
    return i18n ("Python Style");
  else if (mode == KateDocumentConfig::imXmlStyle)
    return i18n ("XML Style");
  else if (mode == KateDocumentConfig::imCSAndS)
    return i18n ("S&S C Style");
  else if ( mode == KateDocumentConfig::imVarIndent )
    return i18n("Variable Based Indenter");
//  else if ( mode == KateDocumentConfig::imScriptIndent )
//    return i18n("JavaScript Indenter");

  return i18n ("None");
}

uint KateAutoIndent::modeNumber (const QString &name)
{
  if (modeName(KateDocumentConfig::imNormal) == name)
    return KateDocumentConfig::imNormal;
  else if (modeName(KateDocumentConfig::imCStyle) == name)
    return KateDocumentConfig::imCStyle;
  else if (modeName(KateDocumentConfig::imPythonStyle) == name)
    return KateDocumentConfig::imPythonStyle;
  else if (modeName(KateDocumentConfig::imXmlStyle) == name)
    return KateDocumentConfig::imXmlStyle;
  else if (modeName(KateDocumentConfig::imCSAndS) == name)
    return KateDocumentConfig::imCSAndS;
  else if ( modeName( KateDocumentConfig::imVarIndent ) == name )
    return KateDocumentConfig::imVarIndent;
//  else if ( modeName( KateDocumentConfig::imScriptIndent ) == name )
//    return KateDocumentConfig::imScriptIndent;

  return KateDocumentConfig::imNone;
}

bool KateAutoIndent::hasConfigPage (uint mode)
{
//  if ( mode == KateDocumentConfig::imScriptIndent )
//    return true;

  return false;
}

IndenterConfigPage* KateAutoIndent::configPage(QWidget *parent, uint mode)
{
//  if ( mode == KateDocumentConfig::imScriptIndent )
//    return new ScriptIndentConfigPage(parent, "script_indent_config_page");

  return 0;
}

KateAutoIndent::KateAutoIndent (KateDocument *_doc)
: doc(_doc)
{
}
KateAutoIndent::~KateAutoIndent ()
{
}

//END KateAutoIndent

//BEGIN KateViewIndentAction
KateViewIndentationAction::KateViewIndentationAction(KateDocument *_doc, const QString& text, KActionCollection* parent, const char* name)
       : KActionMenu (text, parent, name), doc(_doc)
{
  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewIndentationAction::slotAboutToShow()
{
  QStringList modes = KateAutoIndent::listModes ();

  popupMenu()->clear ();
  for (int z=0; z<modes.size(); ++z)
    popupMenu()->insertItem ( '&' + KateAutoIndent::modeDescription(z), this, SLOT(setMode(int)), 0,  z);

  popupMenu()->setItemChecked (doc->config()->indentationMode(), true);
}

void KateViewIndentationAction::setMode (int mode)
{
  doc->config()->setIndentationMode((uint)mode);
}
//END KateViewIndentationAction

//BEGIN KateNormalIndent

KateNormalIndent::KateNormalIndent (KateDocument *_doc)
 : KateAutoIndent (_doc)
{
}
KateNormalIndent::~KateNormalIndent ()
{
}

void KateNormalIndent::updateConfig ()
{
  KateDocumentConfig *config = doc->config();

  useSpaces   = config->configFlags() & KateDocumentConfig::cfSpaceIndent || config->configFlags() & KateDocumentConfig::cfReplaceTabsDyn;
  mixedIndent = useSpaces && config->configFlags() & KateDocumentConfig::cfMixedIndent;
  keepProfile = config->configFlags() & KateDocumentConfig::cfKeepIndentProfile;
  tabWidth    = config->tabWidth();
  indentWidth = useSpaces? config->indentationWidth() : tabWidth;

  commentAttrib = 255;
  doxyCommentAttrib = 255;
  regionAttrib = 255;
  symbolAttrib = 255;
  alertAttrib = 255;
  tagAttrib = 255;
  wordAttrib = 255;
  keywordAttrib = 255;
  normalAttrib = 255;
  extensionAttrib = 255;

  KateExtendedAttributeList items;
  doc->highlight()->getKateExtendedAttributeListCopy (0, items);

  for (uint i=0; i<items.count(); i++)
  {
    QString name = items.at(i)->name();
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
    else if (name.find("Keyword") != -1 && keywordAttrib == 255)
    {
      keywordAttrib = i;
    }
    else if (name.find("Normal") != -1 && normalAttrib == 255)
    {
      normalAttrib = i;
    }
    else if (name.find("Extensions") != -1 && extensionAttrib == 255)
    {
      extensionAttrib = i;
    }
  }
}

bool KateNormalIndent::isBalanced (KateDocCursor &begin, const KateDocCursor &end, QChar open, QChar close, uint &pos) const
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

bool KateNormalIndent::skipBlanks (KateDocCursor &cur, KateDocCursor &max, bool newline) const
{
  int curLine = cur.line();
  if (newline)
    cur.moveForward(1);

  if (cur >= max)
    return false;

  do
  {
    uchar attrib = cur.currentAttrib();
    const QString hlFile = doc->highlight()->hlKeyForAttrib( attrib );

    if (attrib != commentAttrib && attrib != regionAttrib && attrib != alertAttrib && !hlFile.endsWith("doxygen.xml"))
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
      cur.setColumn(0);
    }
  } while (cur < max);

  if (cur > max)
    cur = max;
  return true;
}

uint KateNormalIndent::measureIndent (KateDocCursor &cur) const
{
  if (useSpaces && !mixedIndent)
    return cur.column();

  return doc->plainKateTextLine(cur.line())->positionWithTabs(cur.column(), tabWidth);
}

QString KateNormalIndent::tabString(uint pos) const
{
  QString s;
  pos = QMIN (pos, (uint)80); // sanity check for large values of pos

  if (!useSpaces || mixedIndent)
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

void KateNormalIndent::processNewline (KateDocCursor &begin, bool /*needContinue*/)
{
  int line = begin.line() - 1;
  int pos = begin.column();

  while ((line > 0) && (pos < 0)) // search a not empty text line
    pos = doc->plainKateTextLine(--line)->firstChar();

  begin.setColumn(0);

  if (pos > 0)
  {
    QString filler = doc->text(KTextEditor::Range(line, 0, line, pos));
    doc->insertText(begin, filler);
    begin.setColumn(filler.length());
  }
}

//END

//BEGIN KateCSmartIndent

KateCSmartIndent::KateCSmartIndent (KateDocument *doc)
:  KateNormalIndent (doc),
    allowSemi (false),
    processingBlock (false)
{
  kdDebug(13030)<<"CREATING KATECSMART INTDETER"<<endl;
}

KateCSmartIndent::~KateCSmartIndent ()
{

}

void KateCSmartIndent::processLine (KateDocCursor &line)
{
  kdDebug(13030)<<"PROCESSING LINE "<<line.line()<<endl;
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
      KateNormalIndent::processNewline(line, true);
      return;
    }
  }

  // Slightly faster if we don't indent what we don't have to
  if (indent != measureIndent(line) || first == '}' || first == '{' || first == '#')
  {
    doc->removeText(KTextEditor::Range(line.line(), 0, line.line(), firstChar));
    QString filler = tabString(indent);
    if (indent > 0) doc->insertText(KTextEditor::Cursor(line.line(), 0), filler);
    if (!processingBlock) line.setColumn(filler.length());
  }
}

void KateCSmartIndent::processSection (const KateDocCursor &begin, const KateDocCursor &end)
{
  kdDebug(13030)<<"PROCESS SECTION"<<endl;
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
  kdDebug(13030) << "+++ total: " << t.elapsed() << endl;
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
      if (!textLine->stringAtPos(textLine->lastChar()-1, "*/"))
        insideDoxygen = true;
      while (textLine->attribute(first) != doxyCommentAttrib && first <= textLine->lastChar())
        first++;
      if (textLine->stringAtPos(first, "//"))
        return false;
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

      begin.setColumn(0);
      doc->replaceText(KTextEditor::Range(begin, first), filler);
      begin.setColumn(filler.length());

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
      begin.setColumn(0);
      doc->insertText(begin, filler);
      begin.setColumn(filler.length());

      // Handles cases where user hits enter at the beginning or middle of text
      if (inMiddle)
      {
        processLine(begin);
        begin.setColumn(textLine->firstChar());
      }
    }
    else
    {
      KateNormalIndent::processNewline (begin, needContinue);
    }

    if (begin.column() < 0)
      begin.setColumn(0);
  }
}

void KateCSmartIndent::processChar(QChar c)
{
  static const QString triggers("}{)/:;#n");
  if (triggers.find(c) < 0)
    return;

  KateView *view = doc->activeKateView();
  KateDocCursor begin(view->cursorPosition().line(), 0, doc);

  KateTextLine::Ptr textLine = doc->plainKateTextLine(begin.line());
  if (c == 'n')
  {
    if (textLine->getChar(textLine->firstChar()) != '#')
      return;
  }

  if ( textLine->attribute( begin.column() ) == doxyCommentAttrib )
  {
    // dominik: if line is "* /", change it to "*/"
    if ( c == '/' )
    {
      int first = textLine->firstChar();
      // if the first char exists and is a '*', and the next non-space-char
      // is already the just typed '/', concatenate it to "*/".
      if ( first != -1
           && textLine->getChar( first ) == '*'
           && textLine->nextNonSpaceChar( first+1 ) == (int)view->cursorColumn()-1 )
        doc->removeText( KTextEditor::Range(view->cursorPosition().line(), first+1, view->cursorPosition().line(), view->cursorColumn()-1));
    }

    // anders: don't change the indent of doxygen lines here.
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

  //kdDebug(13030) << "calcIndent begin line:" << begin.line() << " col:" << begin.column() << endl;

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
        lineBegin.setColumn(specialIndent);
        specialIndent = measureIndent(lineBegin);
        isSpecial = true;
      }

      // Move forward past blank lines
      KateDocCursor skip = cur;
      skip.setColumn(textLine->lastChar());
      bool result = skipBlanks(skip, begin, true);

      anchorPos = skip.column();
      anchorIndent = measureIndent(skip);

      //kdDebug(13030) << "calcIndent anchorPos:" << anchorPos << " anchorIndent:" << anchorIndent << " at line:" << skip.line() << endl;

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
        cur.setColumn(anchorPos = textLine->firstChar());
        anchorIndent = measureIndent (cur);
        break;
      }
    }
  }

  if (!found)
    return 0;

  uint continueIndent = (needContinue) ? calcContinue (cur, begin) : 0;
  //kdDebug(13030) << "calcIndent continueIndent:" << continueIndent << endl;

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
    //kdDebug(13030) << "  cur.line:" << cur.line() << " cur.col:" << cur.column() << " currentChar '" << tc << "' " << textLine->attribute(cur.column()) << endl;
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
  //kdDebug(13030) << "calcIndent lastChar '" << lastChar << "'" << endl;

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
  if (textLine->attribute(cur.column()) == symbolAttrib)
  {
    cur.moveForward(1);
    skipBlanks(cur, end, false);
  }

  if (textLine->getChar(cur.column()) == '}')
  {
    skipBlanks(cur, end, true);
    if (cur.line() != start.line())
      textLine = doc->plainKateTextLine(cur.line());

    if (textLine->stringAtPos(cur.column(), "else"))
      cur.setColumn(cur.column() + 4);
    else
      return indentWidth * 2;

    needsBalanced = false;
  }
  else if (textLine->stringAtPos(cur.column(), "else"))
  {
    cur.setColumn(cur.column() + 4);
    needsBalanced = false;
    if (textLine->stringAtPos(textLine->nextNonSpaceChar(cur.column()), "if"))
    {
      cur.setColumn(textLine->nextNonSpaceChar(cur.column()) + 2);
      needsBalanced = true;
    }
  }
  else if (textLine->stringAtPos(cur.column(), "if"))
  {
    cur.setColumn(cur.column() + 2);
  }
  else if (textLine->stringAtPos(cur.column(), "do"))
  {
    cur.setColumn(cur.column() + 2);
    needsBalanced = false;
  }
  else if (textLine->stringAtPos(cur.column(), "for"))
  {
    cur.setColumn(cur.column() + 3);
    isFor = true;
  }
  else if (textLine->stringAtPos(cur.column(), "while"))
  {
    cur.setColumn(cur.column() + 5);
  }
  else if (textLine->stringAtPos(cur.column(), "switch"))
  {
    cur.setColumn(cur.column() + 6);
  }
  else if (textLine->stringAtPos(cur.column(), "using"))
  {
    cur.setColumn(cur.column() + 5);
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
      else if (ch == '}' && cur.column() == 0)
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

//END

//BEGIN KatePythonIndent

QRegExp KatePythonIndent::endWithColon = QRegExp( "^[^#]*:\\s*(#.*)?$" );
QRegExp KatePythonIndent::stopStmt = QRegExp( "^\\s*(break|continue|raise|return|pass)\\b.*" );
QRegExp KatePythonIndent::blockBegin = QRegExp( "^\\s*(def|if|elif|else|for|while|try)\\b.*" );

KatePythonIndent::KatePythonIndent (KateDocument *doc)
: KateNormalIndent (doc)
{
}
KatePythonIndent::~KatePythonIndent ()
{
}

void KatePythonIndent::processNewline (KateDocCursor &begin, bool /*newline*/)
{
  int prevLine = begin.line() - 1;
  int prevPos = begin.column();

  while ((prevLine > 0) && (prevPos < 0)) // search a not empty text line
    prevPos = doc->plainKateTextLine(--prevLine)->firstChar();

  int prevBlock = prevLine;
  int prevBlockPos = prevPos;
  int extraIndent = calcExtra (prevBlock, prevBlockPos, begin);

  int indent = doc->plainKateTextLine(prevBlock)->positionWithTabs(prevBlockPos, tabWidth);
  if (extraIndent == 0)
  {
    if (!stopStmt.exactMatch(doc->plainKateTextLine(prevLine)->string()))
    {
      if (endWithColon.exactMatch(doc->plainKateTextLine(prevLine)->string()))
        indent += indentWidth;
      else
        indent = doc->plainKateTextLine(prevLine)->positionWithTabs(prevPos, tabWidth);
    }
  }
  else
    indent += extraIndent;

  begin.setColumn(0);
  if (indent > 0)
  {
    QString filler = tabString (indent);
    doc->insertText(begin, filler);
    begin.setColumn(filler.length());
  }
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

//END

//BEGIN KateXmlIndent

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
: KateNormalIndent (doc)
{
}

KateXmlIndent::~KateXmlIndent ()
{
}

void KateXmlIndent::processNewline (KateDocCursor &begin, bool /*newline*/)
{
  begin.setColumn(processLine(begin.line()));
}

void KateXmlIndent::processChar (QChar c)
{
  if(c != '/') return;

  // only alter lines that start with a close element
  KateView *view = doc->activeKateView();
  QString text = doc->plainKateTextLine(view->cursorPosition().line())->string();
  if(text.find(startsWithCloseTag) == -1) return;

  // process it
  processLine(view->cursorPosition().line());
}

void KateXmlIndent::processLine (KateDocCursor &line)
{
  processLine (line.line());
}

void KateXmlIndent::processSection (const KateDocCursor &start, const KateDocCursor &end)
{
  KateDocCursor cur (start);
  int endLine = end.line();

  do {
    processLine(cur.line());
    if(!cur.gotoNextLine()) break;
  } while(cur.line() < endLine);
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
  prevIndent = prevLine->positionWithTabs(prevLine->firstChar(), tabWidth);
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

    attrCol = prevLine->positionWithTabs(attrCol, tabWidth);
  }
}

uint KateXmlIndent::processLine (uint line)
{
  KateTextLine::Ptr kateLine = doc->plainKateTextLine(line);
  if(!kateLine) return 0; // sanity check

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
  KTextEditor::Cursor start;
  doc->removeText(KTextEditor::Range(start, kateLine->firstChar()));
  QString filler = tabString(indent);
  doc->insertText(start, filler);

  return filler.length();
}

//END

//BEGIN KateCSAndSIndent

KateCSAndSIndent::KateCSAndSIndent (KateDocument *doc)
:  KateNormalIndent (doc)
{
}

void KateCSAndSIndent::updateIndentString()
{
  if( useSpaces )
    indentString.fill( ' ', indentWidth );
  else
    indentString = '\t';
}

KateCSAndSIndent::~KateCSAndSIndent ()
{
}

void KateCSAndSIndent::processLine (KateDocCursor &line)
{
  KateTextLine::Ptr textLine = doc->plainKateTextLine(line.line());

  if (!textLine)
    return;

  updateIndentString();

  const int oldCol = line.column();
  QString whitespace = calcIndent(line);
  // strip off existing whitespace
  int oldIndent = textLine->firstChar();

  line.setColumn( 0 );

  if ( oldIndent < 0 )
    oldIndent = doc->lineLength( line.line() );

  if( oldIndent > 0 )
    doc->removeText(KTextEditor::Range(line, oldIndent));

  // add correct amount
  doc->insertText(line, whitespace);

  // try to preserve the cursor position in the line
  if ( int(oldCol + whitespace.length()) >= oldIndent )
    line.setColumn( oldCol + whitespace.length() - oldIndent );
}

void KateCSAndSIndent::processSection (const KateDocCursor &begin, const KateDocCursor &end)
{
  QTime t; t.start();
  for( KateDocCursor cur = begin; cur.line() <= end.line(); )
  {
    processLine (cur);
    if (!cur.gotoNextLine())
      break;
  }
  kdDebug(13030) << "+++ total: " << t.elapsed() << endl;
}

/**
 * Returns the first @p chars characters of @p line, converted to whitespace.
 * If @p convert is set to false, characters at and after the first non-whitespace
 * character are removed, not converted.
 */
static QString initialWhitespace(const KateTextLine::Ptr &line, int chars, bool convert = true)
{
  QString text = line->string(0, chars);
  if( (int)text.length() < chars )
  {
    QString filler; filler.fill(' ',chars - text.length());
    text += filler;
  }
  for( int n = 0; n < text.length(); ++n )
  {
    if( text[n] != '\t' && text[n] != ' ' )
    {
      if( !convert )
        return text.left( n );
      text[n] = ' ';
    }
  }
  return text;
}

QString KateCSAndSIndent::findOpeningCommentIndentation(const KateDocCursor &start)
{
  KateDocCursor cur = start;

  // Find the line with the opening /* and return the indentation of it
  do
  {
    KateTextLine::Ptr textLine = doc->plainKateTextLine(cur.line());

    int pos = textLine->string().lastIndexOf("/*");
    // FIXME: /* inside /* is possible. This screws up in that case...
    if (pos >= 0)
      return initialWhitespace(textLine, pos);
  } while (cur.gotoPreviousLine());

  // should never happen.
  kdWarning( 13030 ) << " in a comment, but can't find the start of it" << endl;
  return QString();
}

bool KateCSAndSIndent::handleDoxygen (KateDocCursor &begin)
{
  // Look backwards for a nonempty line
  int line = begin.line();
  int first = -1;
  while ((line > 0) && (first < 0))
    first = doc->plainKateTextLine(--line)->firstChar();

  // no earlier nonempty line
  if (first < 0)
    return false;

  KateTextLine::Ptr textLine = doc->plainKateTextLine(line);

  // if the line doesn't end with a doxygen comment (that's not closed)
  // and doesn't start with a doxygen comment (that's not closed), we don't care.
  // note that we do need to check the start of the line, or lines ending with, say, @brief aren't
  // recognised.
  if ( !(textLine->attribute(textLine->lastChar()) == doxyCommentAttrib && !textLine->endingWith("*/")) &&
       !(textLine->attribute(textLine->firstChar()) == doxyCommentAttrib && !textLine->string().contains("*/")) )
    return false;

  // our line is inside a doxygen comment. align the *'s and then maybe insert one too ...
  textLine = doc->plainKateTextLine(begin.line());
  first = textLine->firstChar();
  QString indent = findOpeningCommentIndentation(begin);

  bool doxygenAutoInsert = doc->config()->configFlags() & KateDocumentConfig::cfDoxygenAutoTyping;

  // starts with *: indent one space more to line up *s
  if ( textLine->stringAtPos(first, "*") )
    indent = indent + " ";
  // does not start with *: insert one if user wants that
  else if ( doxygenAutoInsert )
    indent = indent + " * ";
  // user doesn't want * inserted automatically: put in spaces?
  //else
  //  indent = indent + "   ";

  begin.setColumn(0);
  doc->replaceText (KTextEditor::Range(begin, first), indent);
  begin.setColumn(indent.length());

  return true;
}

/**
 * @brief User pressed enter. Line has been split; begin is on the new line.
 * @param begin Three unrelated variables: the new line number, where the first
 *              non-whitespace char was on the previous line, and the document.
 * @param needContinue Something to do with indenting the current line; always true.
 */
void KateCSAndSIndent::processNewline (KateDocCursor &begin, bool /*needContinue*/)
{
  // in a comment, add a * doxygen-style.
  if( handleDoxygen(begin) )
    return;

  // TODO: if the user presses enter in the middle of a label, maybe the first half of the
  //  label should be indented?

  // where the cursor actually is...
  int cursorPos = doc->plainKateTextLine( begin.line() )->firstChar();
  if ( cursorPos < 0 )
    cursorPos = doc->lineLength( begin.line() );
  begin.setColumn( cursorPos );

  processLine( begin );
}

/**
 * Does the line @p line start with a label?
 * @note May also return @c true if the line starts in a continuation.
 */
bool KateCSAndSIndent::startsWithLabel( int line )
{
  KateTextLine::Ptr indentLine = doc->plainKateTextLine( line );
  const int indentFirst = indentLine->firstChar();

  int attrib = indentLine->attribute(indentFirst);
  if (attrib != 0 && attrib != keywordAttrib && attrib != normalAttrib && attrib != extensionAttrib)
    return false;

  const QString lineContents = indentLine->string();
  static const QString symbols = QLatin1String(";:[]{}");
  const int last = indentLine->lastChar();
  for ( int n = indentFirst + 1; n <= last; ++n )
  {
    QChar c = lineContents[n];
    // FIXME: symbols inside comments are not skipped
    if ( !symbols.contains(c) )
      continue;

    // if we find a symbol other than a :, this is not a label.
    if ( c != ':' )
      return false;

    // : but not ::, this is a label.
    if ( lineContents[n+1] != ':' )
      return true;

    // xy::[^:] is a scope-resolution operator. can occur in case X::Y: for instance.
    // skip both :s and keep going.
    if ( lineContents[n+2] != ':' )
    {
      ++n;
      continue;
    }

    // xy::: outside a continuation is a label followed by a scope-resolution operator.
    // more than 3 :s is illegal, so we don't care that's not indented.
    return true;
  }
  return false;
}

template<class T> T min(T a, T b) { return (a < b) ? a : b; }

int KateCSAndSIndent::lastNonCommentChar( const KateDocCursor &line )
{
  KateTextLine::Ptr textLine = doc->plainKateTextLine( line.line() );
  QString str = textLine->string();

  // find a possible start-of-comment
  int p = -2; // so the first find starts at position 0
  do p = str.find( "//", p + 2 );
  while ( p >= 0 && textLine->attribute(p) != commentAttrib && textLine->attribute(p) != doxyCommentAttrib );

  // no // found? use whole string
  if ( p < 0 )
    p = str.length();

  // ignore trailing blanks. p starts one-past-the-end.
  while( p > 0 && str[p-1].isSpace() ) --p;
  return p - 1;
}

bool KateCSAndSIndent::inForStatement( int line )
{
  // does this line end in a for ( ...
  // with no closing ) ?
  int parens = 0, semicolons = 0;
  for ( ; line >= 0; --line )
  {
    KateTextLine::Ptr textLine = doc->plainKateTextLine(line);
    const int first = textLine->firstChar();
    const int last = textLine->lastChar();

    // look backwards for a symbol: (){};
    // match ()s, {...; and }...; => not in a for
    // ; ; ; => not in a for
    // ( ; and ( ; ; => a for
    for ( int curr = last; curr >= first; --curr )
    {
      if ( textLine->attribute(curr) != symbolAttrib )
        continue;

      switch( textLine->getChar(curr).toAscii() )
      {
      case ';':
        if( ++semicolons > 2 )
          return false;
        break;
      case '{': case '}':
        return false;
      case ')':
        ++parens;
        break;
      case '(':
        if( --parens < 0 )
          return true;
        break;
      }
    }
  }
  // no useful symbols before the ;?
  // not in a for then
  return false;
}


// is the start of the line containing 'begin' in a statement?
bool KateCSAndSIndent::inStatement( const KateDocCursor &begin )
{
  // if the current line starts with an open brace, it's not a continuation.
  // this happens after a function definition (which is treated as a continuation).
  KateTextLine::Ptr textLine = doc->plainKateTextLine(begin.line());
  const int first = textLine->firstChar();
  // note that if we're being called from processChar the attribute has not yet been calculated
  // should be reasonably safe to assume that unattributed {s are symbols; if the { is in a comment
  // we don't want to touch it anyway.
  const int attrib = textLine->attribute(first);
  if( first >= 0 && (attrib == 0 || attrib == symbolAttrib) && textLine->getChar(first) == '{' )
    return false;

  int line;
  for ( line = begin.line() - 1; line >= 0; --line )
  {
    textLine = doc->plainKateTextLine(line);
    const int first = textLine->firstChar();
    if ( first == -1 )
      continue;

    // starts with #: in a comment, don't care
    // outside a comment: preprocessor, don't care
    if ( textLine->getChar( first ) == '#' )
      continue;
    KateDocCursor currLine = begin;
    currLine.setLine( line );
    const int last = lastNonCommentChar( currLine );
    if ( last < first )
      continue;

    // HACK: if we see a comment, assume boldly that this isn't a continuation.
    //       detecting comments (using attributes) is HARD, since they may have
    //       embedded alerts, or doxygen stuff, or just about anything. this is
    //       wrong, and needs fixing. note that only multi-line comments and
    //       single-line comments continued with \ are affected.
    const int attrib = textLine->attribute(last);
    if ( attrib == commentAttrib || attrib == doxyCommentAttrib )
      return false;

    char c = textLine->getChar(last).toAscii();

    // brace => not a continuation.
    if ( attrib == symbolAttrib && c == '{' || c == '}' )
      return false;

    // ; => not a continuation, unless in a for (;;)
    if ( attrib == symbolAttrib && c == ';' )
      return inForStatement( line );

    // found something interesting. maybe it's a label?
    if ( attrib == symbolAttrib && c == ':' )
    {
      // the : above isn't necessarily the : in the label, eg in
      // case 'x': a = b ? c :
      // this will say no continuation incorrectly. but continued statements
      // starting on a line with a label at the start is Bad Style (tm).
      if( startsWithLabel( line ) )
      {
        // either starts with a label or a continuation. if the current line
        // starts in a continuation, we're still in one. if not, this was
        // a label, so we're not in one now. so continue to the next line
        // upwards.
        continue;
      }
    }

    // any other character => in a continuation
    return true;
  }
  // no non-comment text found before here - not a continuation.
  return false;
}

QString KateCSAndSIndent::continuationIndent( const KateDocCursor &begin )
{
  if( !inStatement( begin ) )
    return QString();
  return indentString;
}

/**
 * Figure out how indented the line containing @p begin should be.
 */
QString KateCSAndSIndent::calcIndent (const KateDocCursor &begin)
{
  KateTextLine::Ptr currLine = doc->plainKateTextLine(begin.line());
  int currLineFirst = currLine->firstChar();

  // if the line starts inside a comment, no change of indentation.
  // FIXME: this unnecessarily copies the current indentation over itself.
  // FIXME: on newline, this should copy from the previous line.
  if ( currLineFirst >= 0 &&
       (currLine->attribute(currLineFirst) == commentAttrib ||
        currLine->attribute(currLineFirst) == doxyCommentAttrib) )
    return currLine->string( 0, currLineFirst );

  // if the line starts with # (but isn't a c# region thingy), no indentation at all.
  if( currLineFirst >= 0 && currLine->getChar(currLineFirst) == '#' )
  {
    if( !currLine->stringAtPos( currLineFirst+1, QLatin1String("region") ) &&
        !currLine->stringAtPos( currLineFirst+1, QLatin1String("endregion") ) )
      return QString();
  }

  /* Strategy:
   * Look for an open bracket or brace, or a keyword opening a new scope, whichever comes latest.
   * Found a brace: indent one tab in.
   * Found a bracket: indent to the first non-white after it.
   * Found a keyword: indent one tab in. for try, catch and switch, if newline is set, also add
   *                  an open brace, a newline, and indent two tabs in.
   */
  KateDocCursor cur = begin;
  int pos, openBraceCount = 0, openParenCount = 0;
  bool lookingForScopeKeywords = true;
  const char * const scopeKeywords[] = { "for", "do", "while", "if", "else" };
  const char * const blockScopeKeywords[] = { "try", "catch", "switch" };

  while (cur.gotoPreviousLine())
  {
    KateTextLine::Ptr textLine = doc->plainKateTextLine(cur.line());
    const int lastChar = textLine->lastChar();
    const int firstChar = textLine->firstChar();

    // look through line backwards for interesting characters
    for( pos = lastChar; pos >= firstChar; --pos )
    {
      if (textLine->attribute(pos) == symbolAttrib)
      {
        char tc = textLine->getChar (pos).toAscii();
        switch( tc )
        {
          case '(': case '[':
            if( ++openParenCount > 0 )
              return calcIndentInBracket( begin, cur, pos );
            break;
          case ')': case ']': openParenCount--; break;
          case '{':
            if( ++openBraceCount > 0 )
              return calcIndentInBrace( begin, cur, pos );
            break;
          case '}': openBraceCount--; lookingForScopeKeywords = false; break;
          case ';':
            if( openParenCount == 0 )
              lookingForScopeKeywords = false;
            break;
        }
      }

      // if we've not had a close brace or a semicolon yet, and we're at the same parenthesis level
      // as the cursor, and we're at the start of a scope keyword, indent from it.
      if ( lookingForScopeKeywords && openParenCount == 0 &&
           textLine->attribute(pos) == keywordAttrib &&
           (pos == 0 || textLine->attribute(pos-1) != keywordAttrib ) )
      {
        #define ARRLEN( array ) ( sizeof(array)/sizeof(array[0]) )
        for( uint n = 0; n < ARRLEN(scopeKeywords); ++n )
          if( textLine->stringAtPos(pos, QLatin1String(scopeKeywords[n]) ) )
            return calcIndentAfterKeyword( begin, cur, pos, false );
        for( uint n = 0; n < ARRLEN(blockScopeKeywords); ++n )
          if( textLine->stringAtPos(pos, QLatin1String(blockScopeKeywords[n]) ) )
            return calcIndentAfterKeyword( begin, cur, pos, true );
        #undef ARRLEN
      }
    }
  }

  // no active { in file.
  return QString();
}

QString KateCSAndSIndent::calcIndentInBracket(const KateDocCursor &indentCursor, const KateDocCursor &bracketCursor, int bracketPos)
{
  KateTextLine::Ptr indentLine = doc->plainKateTextLine(indentCursor.line());
  KateTextLine::Ptr bracketLine = doc->plainKateTextLine(bracketCursor.line());

  // FIXME: hard-coded max indent to bracket width - use a kate variable
  // FIXME: expand tabs first...
  if ( bracketPos > 48 )
  {
    // how far to indent? we could look back for a brace or keyword, 2 from that.
    // as it is, we just indent one more than the line with the ( on it.
    // the potential problem with this is when
    //   you have code ( which does          <-- continuation + start of func call
    //     something like this );            <-- extra indentation for func call
    // then again (
    //   it works better than (
    //     the other method for (
    //       cases like this )));
    // consequently, i think this method wins.
    return indentString + initialWhitespace( bracketLine, bracketLine->firstChar() );
  }

  const int indentLineFirst = indentLine->firstChar();

  int indentTo;
  const int attrib = indentLine->attribute(indentLineFirst);
  if( indentLineFirst >= 0 && (attrib == 0 || attrib == symbolAttrib) &&
      ( indentLine->getChar(indentLineFirst) == ')' || indentLine->getChar(indentLineFirst) == ']' ) )
  {
    // If the line starts with a close bracket, line it up
    indentTo = bracketPos;
  }
  else
  {
    // Otherwise, line up with the text after the open bracket
    indentTo = bracketLine->nextNonSpaceChar( bracketPos + 1 );
    if( indentTo == -1 )
      indentTo = bracketPos + 2;
  }
  return initialWhitespace( bracketLine, indentTo );
}

QString KateCSAndSIndent::calcIndentAfterKeyword(const KateDocCursor &indentCursor, const KateDocCursor &keywordCursor, int keywordPos, bool blockKeyword)
{
  KateTextLine::Ptr keywordLine = doc->plainKateTextLine(keywordCursor.line());
  KateTextLine::Ptr indentLine = doc->plainKateTextLine(indentCursor.line());

  QString whitespaceToKeyword = initialWhitespace( keywordLine, keywordPos, false );
  if( blockKeyword )
    ; // FIXME: we could add the open brace and subsequent newline here since they're definitely needed.

  // If the line starts with an open brace, don't indent...
  int first = indentLine->firstChar();
  // if we're being called from processChar attribute won't be set
  const int attrib = indentLine->attribute(first);
  if( first >= 0 && (attrib == 0 || attrib == symbolAttrib) && indentLine->getChar(first) == '{' )
    return whitespaceToKeyword;

  // don't check for a continuation. rules are simple here:
  // if we're in a non-compound statement after a scope keyword, we indent all lines
  // once. so:
  // if ( some stuff
  //      goes here )
  //   apples, and         <-- continuation here is ignored. but this is Bad Style (tm) anyway.
  //   oranges too;
  return indentString + whitespaceToKeyword;
}

QString KateCSAndSIndent::calcIndentInBrace(const KateDocCursor &indentCursor, const KateDocCursor &braceCursor, int bracePos)
{
  KateTextLine::Ptr braceLine = doc->plainKateTextLine(braceCursor.line());
  const int braceFirst = braceLine->firstChar();

  QString whitespaceToOpenBrace = initialWhitespace( braceLine, bracePos, false );

  // if the open brace is the start of a namespace, don't indent...
  // FIXME: this is an extremely poor heuristic. it looks on the line with
  //        the { and the line before to see if they start with a keyword
  //        beginning 'namespace'. that's 99% of usage, I'd guess.
  {
    if( braceFirst >= 0 && braceLine->attribute(braceFirst) == keywordAttrib &&
        braceLine->stringAtPos( braceFirst, QLatin1String( "namespace" ) ) )
      return continuationIndent(indentCursor) + whitespaceToOpenBrace;

    if( braceCursor.line() > 0 )
    {
      KateTextLine::Ptr prevLine = doc->plainKateTextLine(braceCursor.line() - 1);
      int firstPrev = prevLine->firstChar();
      if( firstPrev >= 0 && prevLine->attribute(firstPrev) == keywordAttrib &&
          prevLine->stringAtPos( firstPrev, QLatin1String( "namespace" ) ) )
        return continuationIndent(indentCursor) + whitespaceToOpenBrace;
    }
  }

  KateTextLine::Ptr indentLine = doc->plainKateTextLine(indentCursor.line());
  const int indentFirst = indentLine->firstChar();

  // if the line starts with a close brace, don't indent...
  if( indentFirst >= 0 && indentLine->getChar(indentFirst) == '}' )
    return whitespaceToOpenBrace;

  // if : is the first character (and not followed by another :), this is the start
  // of an initialization list, or a continuation of a ?:. either way, indent twice.
  if ( indentFirst >= 0 && indentLine->attribute(indentFirst) == symbolAttrib &&
       indentLine->getChar(indentFirst) == ':' && indentLine->getChar(indentFirst+1) != ':' )
  {
    return indentString + indentString + whitespaceToOpenBrace;
  }

  const bool continuation = inStatement(indentCursor);
  // if the current line starts with a label, don't indent...
  if( !continuation && startsWithLabel( indentCursor.line() ) )
    return whitespaceToOpenBrace;

  // the normal case: indent once for the brace, again if it's a continuation
  QString continuationIndent = continuation ? indentString : QString();
  return indentString + continuationIndent + whitespaceToOpenBrace;
}

void KateCSAndSIndent::processChar(QChar c)
{
  // 'n' trigger is for c# regions.
  static const QString triggers("}{)]/:;#n");
  if (triggers.find(c) == -1)
    return;

  // for historic reasons, processChar doesn't get a cursor
  // to work on. so fabricate one.
  KateView *view = doc->activeKateView();
  KateDocCursor begin(view->cursorPosition().line(), 0, doc);

  KateTextLine::Ptr textLine = doc->plainKateTextLine(begin.line());
  if ( c == 'n' )
  {
    int first = textLine->firstChar();
    if( first < 0 || textLine->getChar(first) != '#' )
      return;
  }

  if ( textLine->attribute( begin.column() ) == doxyCommentAttrib )
  {
    // dominik: if line is "* /", change it to "*/"
    if ( c == '/' )
    {
      int first = textLine->firstChar();
      // if the first char exists and is a '*', and the next non-space-char
      // is already the just typed '/', concatenate it to "*/".
      if ( first != -1
           && textLine->getChar( first ) == '*'
           && textLine->nextNonSpaceChar( first+1 ) == (int)view->cursorColumn()-1 )
        doc->removeText( KTextEditor::Range(view->cursorPosition().line(), first+1, view->cursorPosition().line(), view->cursorColumn()-1));
    }

    // anders: don't change the indent of doxygen lines here.
    return;
  }

  processLine(begin);
}

//END

//BEGIN KateVarIndent
class KateVarIndentPrivate {
  public:
    QRegExp reIndentAfter, reIndent, reUnindent;
    QString triggers;
    uint couples;
    uchar coupleAttrib;
};

KateVarIndent::KateVarIndent( KateDocument *doc )
: QObject( 0, "variable indenter"), KateNormalIndent( doc ),d(new KateVarIndentPrivate)
{
  d->reIndentAfter = QRegExp( doc->variable( "var-indent-indent-after" ) );
  d->reIndent = QRegExp( doc->variable( "var-indent-indent" ) );
  d->reUnindent = QRegExp( doc->variable( "var-indent-unindent" ) );
  d->triggers = doc->variable( "var-indent-triggerchars" );
  d->coupleAttrib = 0;

  slotVariableChanged( doc, "var-indent-couple-attribute", doc->variable( "var-indent-couple-attribute" ) );
  slotVariableChanged( doc, "var-indent-handle-couples", doc->variable( "var-indent-handle-couples" ) );

  // update if a setting is changed
  connect( doc, SIGNAL(variableChanged( KTextEditor::Document*, const QString&, const QString&) ),
           this, SLOT(slotVariableChanged( KTextEditor::Document*, const QString&, const QString& )) );
}

KateVarIndent::~KateVarIndent()
{
  delete d;
}

void KateVarIndent::processNewline ( KateDocCursor &begin, bool /*needContinue*/ )
{
  // process the line left, as well as the one entered
  KateDocCursor left( begin.line()-1, 0, doc );
  processLine( left );
  processLine( begin );
}

void KateVarIndent::processChar ( QChar c )
{
  // process line if the c is in our list, and we are not in comment text
  if ( d->triggers.contains( c ) )
  {
    KateTextLine::Ptr ln = doc->plainKateTextLine( doc->activeView()->cursorPosition().line() );
    if ( ln->attribute( doc->activeView()->cursorPosition().column()-1 ) == commentAttrib )
      return;

    KTextEditor::View *view = doc->activeView();
    KateDocCursor begin( view->cursorPosition().line(), 0, doc );
    kdDebug(13030)<<"variable indenter: process char '"<<c<<", line "<<begin.line()<<endl;
    processLine( begin );
  }
}

void KateVarIndent::processLine ( KateDocCursor &line )
{
  updateConfig(); // ### is it really nessecary *each time* ??

  QString indent; // store the indent string here

  // find the first line with content that is not starting with comment text,
  // and take the position from that
  int ln = line.line();
  int pos = -1;
  KateTextLine::Ptr ktl = doc->plainKateTextLine( ln );
  if ( ! ktl ) return; // no line!?

  // skip blank lines, except for the cursor line
  KTextEditor::View *v = doc->activeView();
  if ( (ktl->firstChar() < 0) && (!v || (int)v->cursorPosition().line() != ln ) )
    return;

  int fc;
  if ( ln > 0 )
  do
  {

    ktl = doc->plainKateTextLine( --ln );
    fc = ktl->firstChar();
    if ( ktl->attribute( fc ) != commentAttrib )
      pos = fc;
  }
  while ( (ln > 0) && (pos < 0) ); // search a not empty text line

  if ( pos < 0 )
    pos = 0;
  else
    pos = ktl->positionWithTabs( pos, tabWidth );

  int adjustment = 0;

  // try 'couples' for an opening on the above line first. since we only adjust by 1 unit,
  // we only need 1 match.
  if ( d->couples & Parens && coupleBalance( ln, '(', ')' ) > 0 )
    adjustment++;
  else if ( d->couples & Braces && coupleBalance( ln, '{', '}' ) > 0 )
    adjustment++;
  else if ( d->couples & Brackets && coupleBalance( ln, '[', ']' ) > 0 )
    adjustment++;

  // Try 'couples' for a closing on this line first. since we only adjust by 1 unit,
  // we only need 1 match. For unindenting, we look for a closing character
  // *at the beginning of the line*
  // NOTE Assume that a closing brace with the configured attribute on the start
  // of the line is closing.
  // When acting on processChar, the character isn't highlighted. So I could
  // either not check, assuming that the first char *is* meant to close, or do a
  // match test if the attrib is 0. How ever, doing that is
  // a potentially huge job, if the match is several hundred lines away.
  // Currently, the check is done.
  {
    KateTextLine::Ptr tl = doc->plainKateTextLine( line.line() );
    int i = tl->firstChar();
    if ( i > -1 )
    {
      QChar ch = tl->getChar( i );
      uchar at = tl->attribute( i );
      kdDebug(13030)<<"attrib is "<<at<<endl;
      if ( d->couples & Parens && ch == ')'
           && ( at == d->coupleAttrib
                || (! at && hasRelevantOpening( KateDocCursor( line.line(), i, doc ) ))
              )
         )
        adjustment--;
      else if ( d->couples & Braces && ch == '}'
                && ( at == d->coupleAttrib
                     || (! at && hasRelevantOpening( KateDocCursor( line.line(), i, doc ) ))
                   )
              )
        adjustment--;
      else if ( d->couples & Brackets && ch == ']'
                && ( at == d->coupleAttrib
                     || (! at && hasRelevantOpening( KateDocCursor( line.line(), i, doc ) ))
                   )
              )
        adjustment--;
    }
  }
#define ISCOMMENTATTR(attr) (attr==commentAttrib||attr==doxyCommentAttrib)
#define ISCOMMENT (ISCOMMENTATTR(ktl->attribute(ktl->firstChar()))||ISCOMMENTATTR(ktl->attribute(matchpos)))
  // check if we should indent, unless the line starts with comment text,
  // or the match is in comment text
  kdDebug(13030)<<"variable indenter: starting indent: "<<pos<<endl;
  // check if the above line indicates that we shuld add indentation
  int matchpos = 0;
  if ( ktl && ! d->reIndentAfter.isEmpty()
       && (matchpos = d->reIndentAfter.search( doc->line( ln ) )) > -1
       && ! ISCOMMENT )
    adjustment++;

  // else, check if this line should indent unless ...
  ktl = doc->plainKateTextLine( line.line() );
  if ( ! d->reIndent.isEmpty()
         && (matchpos = d->reIndent.search( doc->line( line.line() ) )) > -1
         && ! ISCOMMENT )
    adjustment++;

  // else, check if the current line indicates if we should remove indentation unless ...
  if ( ! d->reUnindent.isEmpty()
       && (matchpos = d->reUnindent.search( doc->line( line.line() ) )) > -1
       && ! ISCOMMENT )
    adjustment--;

  kdDebug(13030)<<"variable indenter: adjusting by "<<adjustment<<" units"<<endl;

  if ( adjustment > 0 )
    pos += indentWidth;
  else if ( adjustment < 0 )
    pos -= indentWidth;

  fc = doc->plainKateTextLine( line.line() )->firstChar();

  // dont change if there is no change.
  // ### should I actually compare the strings?
  // FIXME for some odd reason, the document gets marked as changed
  //       even if we don't change it !?
  if ( fc == pos )
    return;

  line.setColumn(0);

  if ( fc > 0 )
    doc->removeText(KTextEditor::Range(line, fc) );

  if ( pos > 0 )
    indent = tabString( pos );

  if ( pos > 0 )
    doc->insertText (line, indent);

  // try to restore cursor ?
  line.setColumn( pos );
}

void KateVarIndent::processSection (const KateDocCursor &begin, const KateDocCursor &end)
{
  KateDocCursor cur = begin;
  while (cur.line() <= end.line())
  {
    processLine (cur);
    if (!cur.gotoNextLine())
      break;
  }
}

void KateVarIndent::slotVariableChanged( KTextEditor::Document*, const QString &var, const QString &val )
{
  if ( ! var.startsWith("var-indent") )
    return;

  if ( var == "var-indent-indent-after" )
    d->reIndentAfter.setPattern( val );
  else if ( var == "var-indent-indent" )
    d->reIndent.setPattern( val );
  else if ( var == "var-indent-unindent" )
    d->reUnindent.setPattern( val );
  else if ( var == "var-indent-triggerchars" )
    d->triggers = val;
  else if ( var == "var-indent-handle-couples" )
  {
    d->couples = 0;
    QStringList l = QStringList::split( " ", val );
    if ( l.contains("parens") ) d->couples |= Parens;
    if ( l.contains("braces") ) d->couples |= Braces;
    if ( l.contains("brackets") ) d->couples |= Brackets;
  }
  else if ( var == "var-indent-couple-attribute" )
  {
    //read a named attribute of the config.
    KateExtendedAttributeList items;
    doc->highlight()->getKateExtendedAttributeListCopy (0, items);

    for (int i=0; i<items.count(); i++)
    {
      if ( items.at(i)->name().section( ':', 1 ) == val )
      {
        d->coupleAttrib = i;
        break;
      }
    }
  }
}

int KateVarIndent::coupleBalance ( int line, const QChar &open, const QChar &close ) const
{
  int r = 0;

  KateTextLine::Ptr ln = doc->plainKateTextLine( line );
  if ( ! ln || ! ln->length() ) return 0;

  for ( int z=0; z < ln->length(); ++z )
  {
    QChar c = ln->getChar( z );
    if ( ln->attribute(z) == d->coupleAttrib )
    {
      kdDebug(13030)<<z<<", "<<c<<endl;
      if (c == open)
        r++;
      else if (c == close)
        r--;
    }
  }
  return r;
}

bool KateVarIndent::hasRelevantOpening( const KateDocCursor &end ) const
{
  KateDocCursor cur = end;
  int count = 1;

  QChar close = cur.currentChar();
  QChar opener;
  if ( close == QChar::fromAscii('}') ) opener = QChar::fromAscii('{');
  else if ( close == QChar::fromAscii(')') ) opener = QChar::fromAscii('(');
  else if (close == QChar::fromAscii(']') ) opener = QChar::fromAscii('[');
  else return false;

  //Move backwards 1 by 1 and find the opening partner
  while (cur.moveBackward(1))
  {
    if (cur.currentAttrib() == d->coupleAttrib)
    {
      QChar ch = cur.currentChar();
      if (ch == opener)
        count--;
      else if (ch == close)
        count++;

      if (count == 0)
        return true;
    }
  }

  return false;
}


//END KateVarIndent

//BEGIN KateScriptIndent
KateScriptIndent::KateScriptIndent( KateDocument *doc )
  : KateNormalIndent( doc )
{
    m_script=KateGlobal::self()->indentScript ("script-indent-c1-test");
}

KateScriptIndent::~KateScriptIndent()
{
}

void KateScriptIndent::processNewline( KateDocCursor &begin, bool needContinue )
{
  kdDebug(13030) << "processNewline" << endl;
  KateView *view = doc->activeKateView();

  if (view)
  {
    QString errorMsg;

    QTime t;
    t.start();
    kdDebug(13030)<<"calling m_script.processChar"<<endl;
    if( !m_script.processNewline( view, begin, needContinue , errorMsg ) )
    {
      kdDebug(13030) << "Error in script-indent: " << errorMsg << endl;
    }
    kdDebug(13030) << "ScriptIndent::TIME in ms: " << t.elapsed() << endl;
  }
}

void KateScriptIndent::processChar( QChar c)
{
  kdDebug(13030) << "processChar" << endl;
  KateView *view = doc->activeKateView();

  if (view)
  {
    QString errorMsg;

    QTime t;
    t.start();
    kdDebug(13030)<<"calling m_script.processChar"<<endl;
    if( !m_script.processChar( view, c , errorMsg ) )
    {
      kdDebug(13030) << "Error in script-indent: " << errorMsg << endl;
    }
    kdDebug(13030) << "ScriptIndent::TIME in ms: " << t.elapsed() << endl;
  }
}

void KateScriptIndent::processLine (KateDocCursor &line)
{
  kdDebug(13030) << "processLine" << endl;
  KateView *view = doc->activeKateView();

  if (view)
  {
    QString errorMsg;

    QTime t;
    t.start();
    kdDebug(13030)<<"calling m_script.processLine"<<endl;
    if( !m_script.processLine( view, line , errorMsg ) )
    {
      kdDebug(13030) << "Error in script-indent: " << errorMsg << endl;
    }
    kdDebug(13030) << "ScriptIndent::TIME in ms: " << t.elapsed() << endl;
  }
}
//END KateScriptIndent

//BEGIN ScriptIndentConfigPage, THIS IS ONLY A TEST! :)
#include <qlabel.h>
ScriptIndentConfigPage::ScriptIndentConfigPage ( QWidget *parent)
    : IndenterConfigPage(parent)
{
  QLabel* hello = new QLabel("Hello world! Dummy for testing purpose.", this);
  hello->show();
}

ScriptIndentConfigPage::~ScriptIndentConfigPage ()
{
}

void ScriptIndentConfigPage::apply ()
{
  kdDebug(13030) << "ScriptIndentConfigPagE::apply() was called, save config options now!" << endl;
}
//END ScriptIndentConfigPage

// kate: space-indent on; indent-width 2; replace-tabs on;
