// This file is part of the KDE libraries
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2009 Dominik Haumann <dhaumann kde org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "katescriptdocument.h"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katehighlight.h"
#include "katescript.h"

#include <ktexteditor/highlightinterface.h>
#include <ktexteditor/movingcursor.h>

#include <QtScript/QScriptEngine>

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

int KateScriptDocument::defStyleNum(int line, int column)
{
  QList<KTextEditor::Attribute::Ptr> attributes = m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  KTextEditor::Attribute::Ptr a = attributes[document()->plainKateTextLine(line)->attribute(column)];
  return a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
}

int KateScriptDocument::defStyleNum(const KTextEditor::Cursor& cursor)
{
  return defStyleNum(cursor.line(), cursor.column());
}


bool KateScriptDocument::isCode(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return _isCode(defaultStyle);
}

bool KateScriptDocument::isCode(const KTextEditor::Cursor& cursor)
{
  return isCode(cursor.line(), cursor.column());
}

bool KateScriptDocument::isComment(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KTextEditor::HighlightInterface::dsComment;
}

bool KateScriptDocument::isComment(const KTextEditor::Cursor& cursor)
{
  return isComment(cursor.line(), cursor.column());
}

bool KateScriptDocument::isString(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KTextEditor::HighlightInterface::dsString;
}

bool KateScriptDocument::isString(const KTextEditor::Cursor& cursor)
{
  return isString(cursor.line(), cursor.column());
}

bool KateScriptDocument::isRegionMarker(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KTextEditor::HighlightInterface::dsRegionMarker;
}

bool KateScriptDocument::isRegionMarker(const KTextEditor::Cursor& cursor)
{
  return isRegionMarker(cursor.line(), cursor.column());
}

bool KateScriptDocument::isChar(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KTextEditor::HighlightInterface::dsChar;
}

bool KateScriptDocument::isChar(const KTextEditor::Cursor& cursor)
{
  return isChar(cursor.line(), cursor.column());
}

bool KateScriptDocument::isOthers(int line, int column)
{
  const int defaultStyle = defStyleNum(line, column);
  return defaultStyle == KTextEditor::HighlightInterface::dsOthers;
}

bool KateScriptDocument::isOthers(const KTextEditor::Cursor& cursor)
{
  return isOthers(cursor.line(), cursor.column());
}

int KateScriptDocument::firstVirtualColumn(int line)
{
  const int tabWidth = m_document->config()->tabWidth();
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  const int firstPos = textLine ? textLine->firstChar() : -1;
  if(!textLine || firstPos == -1)
    return -1;
  return textLine->indentDepth(tabWidth);
}

int KateScriptDocument::lastVirtualColumn(int line)
{
  const int tabWidth = m_document->config()->tabWidth();
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  const int lastPos = textLine ? textLine->lastChar() : -1;
  if(!textLine || lastPos == -1)
    return -1;
  return textLine->toVirtualColumn(lastPos, tabWidth);
}

int KateScriptDocument::toVirtualColumn(int line, int column)
{
  const int tabWidth = m_document->config()->tabWidth();
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  if (!textLine || column < 0 || column > textLine->length()) return -1;
  return textLine->toVirtualColumn(column, tabWidth);
}

int KateScriptDocument::toVirtualColumn(const KTextEditor::Cursor& cursor)
{
  return toVirtualColumn(cursor.line(), cursor.column());
}

KTextEditor::Cursor KateScriptDocument::toVirtualCursor(const KTextEditor::Cursor& cursor)
{
  return KTextEditor::Cursor(cursor.line(),
                             toVirtualColumn(cursor.line(), cursor.column()));
}

int KateScriptDocument::fromVirtualColumn(int line, int virtualColumn)
{
  const int tabWidth = m_document->config()->tabWidth();
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  if(!textLine || virtualColumn < 0 || virtualColumn > textLine->virtualLength(tabWidth))
    return -1;
  return textLine->fromVirtualColumn(virtualColumn, tabWidth);
}

int KateScriptDocument::fromVirtualColumn(const KTextEditor::Cursor& virtualCursor)
{
  return fromVirtualColumn(virtualCursor.line(), virtualCursor.column());
}

KTextEditor::Cursor KateScriptDocument::fromVirtualCursor(const KTextEditor::Cursor& virtualCursor)
{
  return KTextEditor::Cursor(virtualCursor.line(),
                             fromVirtualColumn(virtualCursor.line(), virtualCursor.column()));
}

KTextEditor::Cursor KateScriptDocument::rfind(int line, int column, const QString& text, int attribute)
{
  QScopedPointer<KTextEditor::MovingCursor> cursor(document()->newMovingCursor(KTextEditor::Cursor(line, column)));
  const int start = cursor->line();
  QList<KTextEditor::Attribute::Ptr> attributes =
      m_document->highlight()->attributes(((KateView*)m_document->activeView())->renderer()->config()->schema());

  do {
    Kate::TextLine textLine = m_document->plainKateTextLine(cursor->line());
    if (!textLine)
      break;

    if (cursor->line() != start) {
      cursor->setColumn(textLine->length());
    } else if (column >= textLine->length()) {
      cursor->setColumn(qMax(textLine->length(), 0));
    }

    int foundAt;
    while ((foundAt = textLine->string().left(cursor->column()).lastIndexOf(text, -1, Qt::CaseSensitive)) >= 0) {
        bool hasStyle = true;
        if (attribute != -1) {
          KTextEditor::Attribute::Ptr a = attributes[textLine->attribute(foundAt)];
          const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
          hasStyle = (ds == attribute);
        }

        if (hasStyle) {
          return KTextEditor::Cursor(cursor->line(), foundAt);
        } else {
          cursor->setColumn(foundAt);
        }
    }
  } while (cursor->gotoPreviousLine());

  return KTextEditor::Cursor::invalid();
}

KTextEditor::Cursor KateScriptDocument::rfind(const KTextEditor::Cursor& cursor, const QString& text, int attribute)
{
  return rfind(cursor.line(), cursor.column(), text, attribute);
}

KTextEditor::Cursor KateScriptDocument::anchor(int line, int column, QChar character)
{
  qDebug() << line << column << character;
  QList<KTextEditor::Attribute::Ptr> attributes =
      m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  int count = 1;
  QChar lc;
  QChar rc;
  if (character == '(' || character == ')') {
    lc = '(';
    rc = ')';
  } else if (character == '{' || character == '}') {
    lc = '{';
    rc = '}';
  } else if (character == '[' || character == ']') {
    lc = '[';
    rc = ']';
  } else {
    kDebug(13060) << "invalid anchor character:" << character << " allowed are: (){}[]";
    return KTextEditor::Cursor::invalid();
  }

  QScopedPointer<KTextEditor::MovingCursor> cursor(document()->newMovingCursor(KTextEditor::Cursor(line, column)));

  // Move backwards char by char and find the opening character
  while (cursor->move(-1)) {
    QChar ch = document()->character(cursor->toCursor());
    if (ch == lc) {
      KTextEditor::Attribute::Ptr a = attributes[document()->plainKateTextLine(cursor->line())->attribute(cursor->column())];
      const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
      if (_isCode(ds)) {
        --count;
      }
    }
    else if (ch == rc) {
      KTextEditor::Attribute::Ptr a = attributes[document()->plainKateTextLine(cursor->line())->attribute(cursor->column())];
      const int ds = a->property(KateExtendedAttribute::AttributeDefaultStyleIndex).toInt();
      if (_isCode(ds)) {
        ++count;
      }
    }

    if (count == 0) {
      return cursor->toCursor();
    }
  }
  return KTextEditor::Cursor::invalid ();
}

KTextEditor::Cursor KateScriptDocument::anchor(const KTextEditor::Cursor& cursor, QChar character)
{
  return anchor(cursor.line(), cursor.column(), character);
}

bool KateScriptDocument::startsWith (int line, const QString &pattern, bool skipWhiteSpaces)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);

  if (!textLine)
    return false;

  if (skipWhiteSpaces)
    return textLine->matchesAt(textLine->firstChar(), pattern);

  return textLine->startsWith (pattern);
}

bool KateScriptDocument::endsWith (int line, const QString &pattern, bool skipWhiteSpaces)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);

  if (!textLine)
    return false;

  if (skipWhiteSpaces)
    return textLine->matchesAt(textLine->lastChar() - pattern.length() + 1, pattern);

  return textLine->endsWith (pattern);
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

QString KateScriptDocument::highlightingMode()
{
  return m_document->highlightingMode();
}

QStringList KateScriptDocument::embeddedHighlightingModes()
{
  return m_document->embeddedHighlightingModes();
}

QString KateScriptDocument::highlightingModeAt(const KTextEditor::Cursor& pos)
{
  return m_document->highlightingModeAt(pos);
}

bool KateScriptDocument::isModified()
{
  return m_document->isModified();
}

QString KateScriptDocument::text()
{
  return m_document->text();
}

QString KateScriptDocument::text(int fromLine, int fromColumn, int toLine, int toColumn)
{
  return text(KTextEditor::Range(fromLine, fromColumn, toLine, toColumn));
}

QString KateScriptDocument::text(const KTextEditor::Cursor& from, const KTextEditor::Cursor& to)
{
  return text(KTextEditor::Range(from, to));
}

QString KateScriptDocument::text(const KTextEditor::Range& range)
{
  return m_document->text(range);
}

QString KateScriptDocument::line(int line)
{
  return m_document->line(line);
}

QString KateScriptDocument::wordAt(int line, int column)
{
  return m_document->getWord(KTextEditor::Cursor(line, column));
}

QString KateScriptDocument::wordAt(const KTextEditor::Cursor& cursor)
{
  return m_document->getWord(cursor);
}

QString KateScriptDocument::charAt(int line, int column)
{
  return charAt(KTextEditor::Cursor(line, column));
}

QString KateScriptDocument::charAt(const KTextEditor::Cursor& cursor)
{
  const QChar c = m_document->character(cursor);
  return c.isNull() ? "" : QString(c);
}

QString KateScriptDocument::firstChar(int line)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  if(!textLine) return "";
  // check for isNull(), as the returned character then would be "\0"
  const QChar c = textLine->at(textLine->firstChar());
  return c.isNull() ? "" : QString(c);
}

QString KateScriptDocument::lastChar(int line)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  if(!textLine) return "";
  // check for isNull(), as the returned character then would be "\0"
  const QChar c = textLine->at(textLine->lastChar());
  return c.isNull() ? "" : QString(c);
}

bool KateScriptDocument::isSpace(int line, int column)
{
  return isSpace(KTextEditor::Cursor(line, column));
}

bool KateScriptDocument::isSpace(const KTextEditor::Cursor& cursor)
{
  return m_document->character(cursor).isSpace();
}

bool KateScriptDocument::matchesAt(int line, int column, const QString &s)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  return textLine ? textLine->matchesAt(column, s) : false;
}

bool KateScriptDocument::matchesAt(const KTextEditor::Cursor& cursor, const QString &s)
{
  return matchesAt(cursor.line(), cursor.column(), s);
}

bool KateScriptDocument::setText(const QString &s)
{
  return m_document->setText(s);
}

bool KateScriptDocument::clear()
{
  return m_document->clear();
}

bool KateScriptDocument::truncate(int line, int column)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  if (!textLine || textLine->text().size() < column)
    return false;

  KTextEditor::Cursor from (line, column), to (line, textLine->text().size() - column);
  return removeText(KTextEditor::Range(from, to));
}

bool KateScriptDocument::truncate(const KTextEditor::Cursor& cursor)
{
  return truncate(cursor.line(), cursor.column());
}

bool KateScriptDocument::insertText(int line, int column, const QString &s)
{
  return insertText(KTextEditor::Cursor(line, column), s);
}

bool KateScriptDocument::insertText(const KTextEditor::Cursor& cursor, const QString &s)
{
  return m_document->insertText(cursor, s);
}

bool KateScriptDocument::removeText(int fromLine, int fromColumn, int toLine, int toColumn)
{
  return removeText(KTextEditor::Range(fromLine, fromColumn, toLine, toColumn));
}

bool KateScriptDocument::removeText(const KTextEditor::Cursor& from, const KTextEditor::Cursor& to)
{
  return removeText(KTextEditor::Range(from, to));
}

bool KateScriptDocument::removeText(const KTextEditor::Range& range)
{
  return m_document->removeText(range);
}

bool KateScriptDocument::insertLine(int line, const QString &s)
{
  return m_document->insertLine (line, s);
}

bool KateScriptDocument::removeLine(int line)
{
  return m_document->removeLine (line);
}

void KateScriptDocument::joinLines(int startLine, int endLine)
{
  m_document->joinLines (startLine, endLine);
}

int KateScriptDocument::lines()
{
  return m_document->lines();
}

int KateScriptDocument::length()
{
  return m_document->totalCharacters();
}

int KateScriptDocument::lineLength(int line)
{
  return m_document->lineLength(line);
}

void KateScriptDocument::editBegin()
{
  m_document->editBegin();
}

void KateScriptDocument::editEnd()
{
  m_document->editEnd ();
}

int KateScriptDocument::firstColumn(int line)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  return textLine ? textLine->firstChar() : -1;
}

int KateScriptDocument::lastColumn(int line)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  return textLine ? textLine->lastChar() : -1;
}

int KateScriptDocument::prevNonSpaceColumn(int line, int column)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  if(!textLine) return -1;
  return textLine->previousNonSpaceChar(column);
}

int KateScriptDocument::prevNonSpaceColumn(const KTextEditor::Cursor& cursor)
{
  return prevNonSpaceColumn(cursor.line(), cursor.column());
}

int KateScriptDocument::nextNonSpaceColumn(int line, int column)
{
  Kate::TextLine textLine = m_document->plainKateTextLine(line);
  if(!textLine) return -1;
  return textLine->nextNonSpaceChar(column);
}

int KateScriptDocument::nextNonSpaceColumn(const KTextEditor::Cursor& cursor)
{
  return nextNonSpaceColumn(cursor.line(), cursor.column());
}

int KateScriptDocument::prevNonEmptyLine(int line)
{
  const int startLine = line;
  for (int currentLine = startLine; currentLine >= 0; --currentLine) {
    Kate::TextLine textLine = m_document->plainKateTextLine(currentLine);
    if(!textLine)
      return -1;
    if(textLine->firstChar() != -1)
      return currentLine;
  }
  return -1;
}

int KateScriptDocument::nextNonEmptyLine(int line)
{
  const int startLine = line;
  for (int currentLine = startLine; currentLine < m_document->lines(); ++currentLine) {
    Kate::TextLine textLine = m_document->plainKateTextLine(currentLine);
    if(!textLine)
      return -1;
    if(textLine->firstChar() != -1)
      return currentLine;
  }
  return -1;
}

bool KateScriptDocument::isInWord(const QString &character, int attribute)
{
  return m_document->highlight()->isInWord(character.at(0), attribute);
}

bool KateScriptDocument::canBreakAt(const QString &character, int attribute)
{
  return m_document->highlight()->canBreakAt(character.at(0), attribute);
}

bool KateScriptDocument::canComment(int startAttribute, int endAttribute)
{
  return m_document->highlight()->canComment(startAttribute, endAttribute);
}

QString KateScriptDocument::commentMarker(int attribute)
{
  return m_document->highlight()->getCommentSingleLineStart(attribute);
}

QString KateScriptDocument::commentStart(int attribute)
{
  return m_document->highlight()->getCommentStart(attribute);
}

QString KateScriptDocument::commentEnd(int attribute)
{
  return m_document->highlight()->getCommentEnd(attribute);
}

KTextEditor::Range KateScriptDocument::documentRange()
{
  return m_document->documentRange();
}

KTextEditor::Cursor KateScriptDocument::documentEnd()
{
  return m_document->documentEnd();
}

int KateScriptDocument::attribute(int line, int column)
{
  Kate::TextLine textLine = m_document->kateTextLine(line);
  if(!textLine) return 0;
  return textLine->attribute(column);
}

int KateScriptDocument::attribute(const KTextEditor::Cursor& cursor)
{
  return attribute(cursor.line(), cursor.column());
}

bool KateScriptDocument::isAttribute(int line, int column, int attr)
{
  return attr == attribute(line, column);
}

bool KateScriptDocument::isAttribute(const KTextEditor::Cursor& cursor, int attr)
{
  return isAttribute(cursor.line(), cursor.column(), attr);
}

QString KateScriptDocument::attributeName(int line, int column)
{
  QList<KTextEditor::Attribute::Ptr> attributes = m_document->highlight()->attributes(((KateView*) m_document->activeView())->renderer()->config()->schema());
  KTextEditor::Attribute::Ptr a = attributes[document()->plainKateTextLine(line)->attribute(column)];
  return a->property(KateExtendedAttribute::AttributeName).toString();
}

QString KateScriptDocument::attributeName(const KTextEditor::Cursor& cursor)
{
  return attributeName(cursor.line(), cursor.column());
}

bool KateScriptDocument::isAttributeName(int line, int column, const QString &name)
{
  return name == attributeName(line, column);
}

bool KateScriptDocument::isAttributeName(const KTextEditor::Cursor& cursor, const QString &name)
{
  return isAttributeName(cursor.line(), cursor.column(), name);
}

QString KateScriptDocument::variable(const QString &s)
{
  return m_document->variable(s);
}

//END

bool KateScriptDocument::_isCode(int defaultStyle)
{
  return (defaultStyle != KTextEditor::HighlightInterface::dsComment
       && defaultStyle != KTextEditor::HighlightInterface::dsString
       && defaultStyle != KTextEditor::HighlightInterface::dsRegionMarker
       && defaultStyle != KTextEditor::HighlightInterface::dsChar
       && defaultStyle != KTextEditor::HighlightInterface::dsOthers);
}

void KateScriptDocument::indent(KTextEditor::Range range, int change)
{
  m_document->indent(range, change);
}

void KateScriptDocument::align(const KTextEditor::Range& range)
{
  if (m_document->activeKateView()) {
    m_document->align(m_document->activeKateView(), range);
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;

