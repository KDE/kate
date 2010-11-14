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

#ifndef KATE_SCRIPT_DOCUMENT_H
#define KATE_SCRIPT_DOCUMENT_H

#include <QObject>
#include <QStringList>
#include <QtScript/QScriptable>

#include "katepartprivate_export.h"

#include <QtScript/QScriptValue>

#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>

class KateDocument;

/**
 * Thinish wrapping around KateDocument, exposing the methods we want exposed
 * and adding some helper methods.
 *
 * We inherit from QScriptable to have more thight access to the scripting
 * engine.
 *
 * setDocument _must_ be called before using any other method. This is not checked
 * for the sake of speed.
 */
class KATEPART_TESTS_EXPORT KateScriptDocument : public QObject, protected QScriptable
{
  Q_OBJECT
  // Note: we have no Q_PROPERTIES due to consistency: everything is a function.

  public:
    KateScriptDocument(QObject *parent=0);
    void setDocument(KateDocument *document);
    KateDocument *document();

    //BEGIN
    Q_INVOKABLE QString fileName();
    Q_INVOKABLE QString url();
    Q_INVOKABLE QString mimeType();
    Q_INVOKABLE QString encoding();
    Q_INVOKABLE QString highlightingMode();
    Q_INVOKABLE QStringList embeddedHighlightingModes();
    Q_INVOKABLE QString highlightingModeAt(const KTextEditor::Cursor& pos);
    Q_INVOKABLE bool isModified();
    Q_INVOKABLE QString text();
    Q_INVOKABLE QString text(int fromLine, int fromColumn, int toLine, int toColumn);
    Q_INVOKABLE QString text(const KTextEditor::Cursor& from, const KTextEditor::Cursor& to);
    Q_INVOKABLE QString text(const KTextEditor::Range& range);
    Q_INVOKABLE QString line(int line);
    Q_INVOKABLE QString wordAt(int line, int column);
    Q_INVOKABLE QString wordAt(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE QString charAt(int line, int column);
    Q_INVOKABLE QString charAt(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE QString firstChar(int line);
    Q_INVOKABLE QString lastChar(int line);
    Q_INVOKABLE bool isSpace(int line, int column);
    Q_INVOKABLE bool isSpace(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool matchesAt(int line, int column, const QString &s);
    Q_INVOKABLE bool matchesAt(const KTextEditor::Cursor& cursor, const QString &s);
    Q_INVOKABLE bool setText(const QString &s);
    Q_INVOKABLE bool clear();
    Q_INVOKABLE bool truncate(int line, int column);
    Q_INVOKABLE bool truncate(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool insertText(int line, int column, const QString &s);
    Q_INVOKABLE bool insertText(const KTextEditor::Cursor& cursor, const QString &s);
    Q_INVOKABLE bool removeText(int fromLine, int fromColumn, int toLine, int toColumn);
    Q_INVOKABLE bool removeText(const KTextEditor::Cursor& from, const KTextEditor::Cursor& to);
    Q_INVOKABLE bool removeText(const KTextEditor::Range& range);
    Q_INVOKABLE bool insertLine(int line, const QString &s);
    Q_INVOKABLE bool removeLine(int line);
    Q_INVOKABLE void joinLines(int startLine, int endLine);
    Q_INVOKABLE int lines();
    Q_INVOKABLE int length();
    Q_INVOKABLE int lineLength(int line);
    Q_INVOKABLE void editBegin();
    Q_INVOKABLE void editEnd();
    Q_INVOKABLE int firstColumn(int line);
    Q_INVOKABLE int lastColumn(int line);
    Q_INVOKABLE int prevNonSpaceColumn(int line, int column);
    Q_INVOKABLE int prevNonSpaceColumn(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE int nextNonSpaceColumn(int line, int column);
    Q_INVOKABLE int nextNonSpaceColumn(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE int prevNonEmptyLine(int line);
    Q_INVOKABLE int nextNonEmptyLine(int line);
    Q_INVOKABLE bool isInWord(const QString &character, int attribute);
    Q_INVOKABLE bool canBreakAt(const QString &character, int attribute);
    Q_INVOKABLE bool canComment(int startAttribute, int endAttribute);
    Q_INVOKABLE QString commentMarker(int attribute);
    Q_INVOKABLE QString commentStart(int attribute);
    Q_INVOKABLE QString commentEnd(int attribute);

    Q_INVOKABLE KTextEditor::Range documentRange();
    Q_INVOKABLE KTextEditor::Cursor documentEnd();

    /**
     * Get the syntax highlighting attribute at a given position in the document.
     */
    Q_INVOKABLE int attribute(int line, int column);
    Q_INVOKABLE int attribute(const KTextEditor::Cursor& cursor);

    /**
     * Return true if the highlight attribute equals @p attr.
     */
    Q_INVOKABLE bool isAttribute(int line, int column, int attr);
    Q_INVOKABLE bool isAttribute(const KTextEditor::Cursor& cursor, int attr);

    /**
     * Get the name of the syntax highlighting attribute at the given position.
     */
    Q_INVOKABLE QString attributeName(int line, int column);
    Q_INVOKABLE QString attributeName(const KTextEditor::Cursor& cursor);

    /**
     * Return true is the name of the syntax attribute equals @p name.
     */
    Q_INVOKABLE bool isAttributeName(int line, int column, const QString &name);
    Q_INVOKABLE bool isAttributeName(const KTextEditor::Cursor& cursor, const QString &name);

    Q_INVOKABLE QString variable(const QString &s);
    //END

    Q_INVOKABLE int firstVirtualColumn(int line);
    Q_INVOKABLE int lastVirtualColumn(int line);
    Q_INVOKABLE int toVirtualColumn(int line, int column);
    Q_INVOKABLE int toVirtualColumn(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE KTextEditor::Cursor toVirtualCursor(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE int fromVirtualColumn(int line, int virtualColumn);
    Q_INVOKABLE int fromVirtualColumn(const KTextEditor::Cursor& virtualCursor);
    Q_INVOKABLE KTextEditor::Cursor fromVirtualCursor(const KTextEditor::Cursor& virtualCursor);

    Q_INVOKABLE KTextEditor::Cursor anchor(int line, int column, QChar character);
    Q_INVOKABLE KTextEditor::Cursor anchor(const KTextEditor::Cursor& cursor, QChar character);
    Q_INVOKABLE KTextEditor::Cursor rfind(int line, int column, const QString& text, int attribute = -1);
    Q_INVOKABLE KTextEditor::Cursor rfind(const KTextEditor::Cursor& cursor, const QString& text, int attribute = -1);

    Q_INVOKABLE int defStyleNum(int line, int column);
    Q_INVOKABLE int defStyleNum(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool isCode(int line, int column);
    Q_INVOKABLE bool isCode(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool isComment(int line, int column);
    Q_INVOKABLE bool isComment(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool isString(int line, int column);
    Q_INVOKABLE bool isString(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool isRegionMarker(int line, int column);
    Q_INVOKABLE bool isRegionMarker(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool isChar(int line, int column);
    Q_INVOKABLE bool isChar(const KTextEditor::Cursor& cursor);
    Q_INVOKABLE bool isOthers(int line, int column);
    Q_INVOKABLE bool isOthers(const KTextEditor::Cursor& cursor);


    Q_INVOKABLE bool startsWith (int line, const QString &pattern, bool skipWhiteSpaces);
    Q_INVOKABLE bool endsWith (int line, const QString &pattern, bool skipWhiteSpaces);

    Q_INVOKABLE void indent(KTextEditor::Range range, int change);
    Q_INVOKABLE void align(const KTextEditor::Range& range);

  private:
    bool _isCode(int defaultStyle);

    KateDocument *m_document;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
