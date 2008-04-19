/// This file is part of the KDE libraries
/// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License version 2 as published by the Free Software Foundation.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
///
/// You should have received a copy of the GNU Library General Public License
/// along with this library; see the file COPYING.LIB.  If not, write to
/// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
/// Boston, MA 02110-1301, USA.

#include "katescriptdocument.h"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katehighlight.h"
#include "katescript.h"

KateScriptDocument::KateScriptDocument(QObject *parent)
  : QObject(parent), m_document(0)
{
}

void KateScriptDocument::setDocument(KateDocument *document)
{
  m_document = document;
}

KateDocument *KateScriptDocument::document()
{
  return m_document;
}

bool KateScriptDocument::isCode(uint line, uint column)
{
  KateDocCursor cursor(line, column, m_document);
  QList<KTextEditor::Attribute::Ptr> attributes = m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  KTextEditor::Attribute::Ptr a = attributes[cursor.currentAttrib()];
  const int defaultStyle = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
  return _isCode(defaultStyle);
}


int KateScriptDocument::firstVirtualColumn(uint line)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  const int firstPos = textLine ? textLine->firstChar() : -1;
  if(!textLine || firstPos == -1)
    return -1;
  return textLine->indentDepth(tabWidth);
}

int KateScriptDocument::lastVirtualColumn(uint line)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  const int lastPos = textLine ? textLine->lastChar() : -1;
  if(!textLine || lastPos == -1)
    return -1;
  return textLine->toVirtualColumn(lastPos, tabWidth);
}

int KateScriptDocument::toVirtualColumn(uint line, uint column)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  if (!textLine || column < 0 || column > textLine->length()) return -1;
  return textLine->toVirtualColumn(column, tabWidth);
}

int KateScriptDocument::fromVirtualColumn(uint line, uint virtualColumn)
{
  const int tabWidth = m_document->config()->tabWidth();
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(line);
  if(!textLine || virtualColumn < 0 || virtualColumn > textLine->virtualLength(tabWidth))
    return -1;
  return textLine->fromVirtualColumn(virtualColumn, tabWidth);
}

KateScriptCursor *KateScriptDocument::anchor(uint line, uint column, QChar character)
{
  KateDocCursor cursor(line, column, m_document);
  QList<KTextEditor::Attribute::Ptr> attributes =
      m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  int count = 1;
  QChar lc = character;
  QChar rc;
  if (lc == '(') rc = ')';
  else if (lc == '{') rc = '}';
  else if (lc == '[') rc = ']';
  else return 0;

  // Move backwards char by char and find the opening character
  while (cursor.moveBackward(1)) {
    QChar ch = cursor.currentChar();
    if (ch == lc) {
      KTextEditor::Attribute::Ptr a = attributes[cursor.currentAttrib()];
      const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
      if (_isCode(ds)) {
        --count;
      }
    }
    else if (ch == rc) {
      KTextEditor::Attribute::Ptr a = attributes[cursor.currentAttrib()];
      const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
      if (_isCode(ds)) {
        ++count;
      }
    }

    if (count == 0) {
      return new KateScriptCursor(line, column);
    }
  }
  return 0;
}


//BEGIN Automatically generated

QString KateScriptDocument::fileName()
{
  return m_document->documentName();
}

QString KateScriptDocument::url()
{
  return m_document->url().prettyUrl();
}

QString KateScriptDocument::mimeType()
{
  return m_document->mimeType();
}

QString KateScriptDocument::encoding()
{
  return m_document->encoding();
}

bool KateScriptDocument::isModified()
{
  return m_document->isModified();
}

QString KateScriptDocument::text()
{
  return m_document->text();
}

QString KateScriptDocument::textRange(uint i, uint j, uint k, uint l)
{
  return m_document->text(KTextEditor::Range(i, j, k, l));
}

QString KateScriptDocument::line(uint i)
{
  return m_document->line (i);
}

QString KateScriptDocument::wordAt(uint i, uint j)
{
  return m_document->getWord(KTextEditor::Cursor(i, j));
}

QString KateScriptDocument::charAt(uint i, uint j)
{
  const QChar c = m_document->character (KTextEditor::Cursor(i, j));
  return c.isNull() ? "" : QString(c);
}

QString KateScriptDocument::firstChar(uint i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return "";
  // check for isNull(), as the returned character then would be "\0"
  const QChar c = textLine->at(textLine->firstChar());
  return c.isNull() ? "" : QString(c);
}

QString KateScriptDocument::lastChar(uint i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return "";
  // check for isNull(), as the returned character then would be "\0"
  const QChar c = textLine->at(textLine->lastChar());
  return c.isNull() ? "" : QString(c);
}

bool KateScriptDocument::isSpace(uint i, uint j)
{
  return m_document->character (KTextEditor::Cursor(i, j)).isSpace();
}

bool KateScriptDocument::matchesAt(uint i, uint j, const QString &s)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  return textLine ? textLine->matchesAt(j, s) : false;
}

bool KateScriptDocument::setText(const QString &s)
{
  return m_document->setText(s);
}

bool KateScriptDocument::clear()
{
  return m_document->clear();
}

bool KateScriptDocument::truncate(uint i, uint j)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(textLine) textLine->truncate(j);
  return static_cast<bool>(textLine);
}

bool KateScriptDocument::insertText(uint i, uint j, const QString &s)
{
  return m_document->insertText (KTextEditor::Cursor(i, j), s);
}

bool KateScriptDocument::removeText(uint i, uint j, uint k, uint l)
{
  return m_document->removeText(KTextEditor::Range(i, j, k, l));
}

bool KateScriptDocument::insertLine(uint i, const QString &s)
{
  return m_document->insertLine (i, s);
}

bool KateScriptDocument::removeLine(uint i)
{
  return m_document->removeLine (i);
}

void KateScriptDocument::joinLines(uint i, uint j)
{
  m_document->joinLines (i, j);
  return;
}

int KateScriptDocument::lines()
{
  return m_document->lines();
}

int KateScriptDocument::length()
{
  return m_document->totalCharacters();
}

int KateScriptDocument::lineLength(uint i)
{
  return m_document->lineLength(i);
}

void KateScriptDocument::editBegin()
{
  m_document->editBegin();
  return;
}

void KateScriptDocument::editEnd()
{
  m_document->editEnd ();
  return;
}

int KateScriptDocument::firstColumn(uint i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  return textLine ? textLine->firstChar() : -1;
}

int KateScriptDocument::lastColumn(uint i)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  return textLine ? textLine->lastChar() : -1;
}

int KateScriptDocument::prevNonSpaceColumn(uint i, uint j)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return -1;
  return textLine->previousNonSpaceChar(j);
}

int KateScriptDocument::nextNonSpaceColumn(uint i, uint j)
{
  KateTextLine::Ptr textLine = m_document->plainKateTextLine(i);
  if(!textLine) return -1;
  return textLine->nextNonSpaceChar(j);
}

int KateScriptDocument::prevNonEmptyLine(uint i)
{
  const int startLine = i;
  for (int currentLine = startLine; currentLine >= 0; --currentLine) {
    KateTextLine::Ptr textLine = m_document->plainKateTextLine(currentLine);
    if(!textLine)
      return -1;
    if(textLine->firstChar() != -1)
      return currentLine;
  }
  return -1;
}

int KateScriptDocument::nextNonEmptyLine(uint i)
{
  const int startLine = i;
  for (int currentLine = startLine; currentLine < m_document->lines(); ++currentLine) {
    KateTextLine::Ptr textLine = m_document->plainKateTextLine(currentLine);
    if(!textLine)
      return -1;
    if(textLine->firstChar() != -1)
      return currentLine;
  }
  return -1;
}

bool KateScriptDocument::isInWord(const QString &s, uint i)
{
  return m_document->highlight()->isInWord(s.at(0), i);
}

bool KateScriptDocument::canBreakAt(const QString &s, uint i)
{
  return m_document->highlight()->canBreakAt(s.at(0), i);
}

bool KateScriptDocument::canComment(uint i, uint j)
{
  return m_document->highlight()->canComment(i, j);
}

QString KateScriptDocument::commentMarker(uint i)
{
  return m_document->highlight()->getCommentSingleLineStart(i);
}

QString KateScriptDocument::commentStart(uint i)
{
  return m_document->highlight()->getCommentStart(i);
}

QString KateScriptDocument::commentEnd(uint i)
{
  return m_document->highlight()->getCommentEnd(i);
}

int KateScriptDocument::attribute(uint i, uint j)
{
  KateTextLine::Ptr textLine = m_document->kateTextLine(i);
  if(!textLine) return 0;
  return textLine->attribute(j);
}

QString KateScriptDocument::variable(const QString &s)
{
  return m_document->variable(s);
}

//END

bool KateScriptDocument::_isCode(int defaultStyle)
{
  return (defaultStyle != KateExtendedAttribute::dsComment
       && defaultStyle != KateExtendedAttribute::dsString
       && defaultStyle != KateExtendedAttribute::dsRegionMarker
       && defaultStyle != KateExtendedAttribute::dsChar
       && defaultStyle != KateExtendedAttribute::dsOthers);
}

// kate: space-indent on; indent-width 2; replace-tabs on;

