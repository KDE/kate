/* This file is part of the KDE libraries
   Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2002-2004 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_BUFFER_H__
#define __KATE_BUFFER_H__

#include "katetextbuffer.h"
#include "katecodefolding.h"

#include "katepartprivate_export.h"

#include <QtCore/QObject>

class KateLineInfo;
class KateDocument;
class KateHighlighting;

/**
 * The KateBuffer class maintains a collections of lines.
 *
 * @author Waldo Bastian <bastian@kde.org>
 * @author Christoph Cullmann <cullmann@kde.org>
 */
class KATEPART_TESTS_EXPORT KateBuffer : public Kate::TextBuffer
{
  Q_OBJECT

  public:
    /**
     * Create an empty buffer.
     * @param doc parent document
     */
    explicit KateBuffer (KateDocument *doc);

    /**
     * Goodbye buffer
     */
    ~KateBuffer ();

  public:
    /**
     * start some editing action
     */
    void editStart ();

    /**
     * finish some editing action
     */
    void editEnd ();

    /**
     * were there changes in the current running
     * editing session?
     * @return changes done?
     */
    inline bool editChanged () const { return editingChangedBuffer (); }

    /**
     * dirty lines start
     * @return start line
     */
    inline int editTagStart () const { return editingMinimalLineChanged (); }

    /**
     * dirty lines end
     * @return end line
     */
    inline int editTagEnd () const { return editingMaximalLineChanged (); }

    /**
     * line inserted/removed?
     * @return line inserted/removed?
     */
    inline bool editTagFrom () const { return editingChangedNumberOfLines() != 0; }

  public:
    /**
     * Clear the buffer.
     */
    void clear();

    /**
     * Open a file, use the given filename
     * @param m_file filename to open
     * @return success
     */
    bool openFile (const QString &m_file);

    /**
     * Did encoding errors occured on load?
     * @return encoding errors occured on load?
     */
    bool brokenEncoding () const { return m_brokenEncoding; }

    /**
     * Can the current codec handle all chars
     * @return chars can be encoded
     */
    bool canEncode ();

    /**
     * Save the buffer to a file, use the given filename + codec + end of line chars (internal use of qtextstream)
     * @param m_file filename to save to
     * @return success
     */
    bool saveFile (const QString &m_file);

  public:
    /**
     * Return line @p lineno.
     * Highlighting of returned line might be out-dated, which may be sufficient
     * for pure text manipulation functions, like search/replace.
     * If you require highlighting to be up to date, call @ref ensureHighlighted
     * prior to this method.
     */
    inline Kate::TextLine plainLine (int lineno)
    {
        if (lineno < 0 || lineno >= lines())
          return Kate::TextLine ();

        return line (lineno);
    }

    /**
     * Update highlighting of given line @p line, if needed.
     */
    void ensureHighlighted(int line);

    /**
     * Return the total number of lines in the buffer.
     */
    inline int count() const { return lines(); }

    /**
     * Wrap line at given cursor position.
     * @param position line/column as cursor where to wrap
     */
    void wrapLine (const KTextEditor::Cursor &position);

    /**
     * Unwrap given line.
     * @param line line to unwrap
     */
    void unwrapLine (int line);

  private:
     inline void addIndentBasedFoldingInformation(QVector<int> &foldingList,int linelength,bool addindent,int deindent);
     inline void updatePreviousNotEmptyLine(int current_line,bool addindent,int deindent);

  public:
    inline int countVisible () { return lines() - m_regionTree.getHiddenLinesCount(lines()); }

    inline int lineNumber (int visibleLine) { return m_regionTree.getRealLine (visibleLine); }

    inline int lineVisibleNumber (int line) { return m_regionTree.getVirtualLine (line); }

    inline void lineInfo (KateLineInfo *info, int line) { m_regionTree.getLineInfo(info,line); }

    inline int tabWidth () const { return m_tabWidth; }

  public:
    void setTabWidth (int w);

    /**
     * Use @p highlight for highlighting
     *
     * @p highlight may be 0 in which case highlighting
     * will be disabled.
     */
    void setHighlight (int hlMode);

    KateHighlighting *highlight () { return m_highlight; }

    /**
     * Invalidate highlighting of whole buffer.
     */
    void invalidateHighlighting();

    KateCodeFoldingTree *foldingTree () { return &m_regionTree; }

    void codeFoldingColumnUpdate(int lineNr);

  private:
    /**
     * Highlight information needs to be updated.
     *
     * @param from first line in range
     * @param to last line in range
     * @param invalidat should the rehighlighted lines be tagged ?
     *
     * @returns true when the highlighting in the next block needs to be updated,
     * false otherwise.
     */
    bool doHighlight (int from, int to, bool invalidate);
    bool isEmptyLine(Kate::TextLine textline);

  Q_SIGNALS:
    /**
     * Emittend if codefolding returned with a changed list
     */
    void codeFoldingUpdated();

    /**
     * Emitted when the highlighting of a certain range has
     * changed.
     */
    void tagLines(int start, int end);
    void respellCheckBlock(int start, int end);
  private:
    /**
     * document we belong to
     */
    KateDocument *const m_doc;

    /**
     * file loaded with encoding problems?
     */
    bool m_brokenEncoding;

  /**
   * highlighting & folding relevant stuff
   */
  private:
    /**
     * current highlighting mode or 0
     */
    KateHighlighting *m_highlight;

    /**
     * folding tree
     */
    KateCodeFoldingTree m_regionTree;

    // for the scrapty indent sensitive langs
    int m_tabWidth;

    int m_lineHighlightedMax;
    int m_lineHighlighted;

    /**
     * number of dynamic contexts causing a full invalidation
     */
    int m_maxDynamicContexts;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
