/* This file is part of the KDE libraries
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   Based on:
     KateTextLine : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef __KATE_TEXTLINE_H__
#define __KATE_TEXTLINE_H__

#include <ksharedptr.h>

#include <QString>
#include <QVector>

class KateRenderer;
class QTextStream;

/**
 * The KateTextLine represents a line of text. A text line that contains the
 * text, an attribute for each character, an attribute for the free space
 * behind the last character and a context number for the syntax highlight.
 * The attribute stores the index to a table that contains fonts and colors
 * and also if a character is selected.
 */
class KateTextLine : public KShared
{
  public:
    /**
     * Define a Shared-Pointer type
     */
    typedef KSharedPtr<KateTextLine> Ptr;

  public:
    /**
     * Used Flags
     */
    enum Flags
    {
      flagNoOtherData = 1, // ONLY INTERNAL USE, NEVER EVER SET THAT !!!!
      flagHlContinue = 2,
      flagAutoWrapped = 4,
      flagFoldingColumnsOutdated = 8,
      flagNoIndentationBasedFolding = 16,
      flagNoIndentationBasedFoldingAtStart = 32
    };

  public:
    /**
     * Constructor
     * Creates an empty text line with given attribute and syntax highlight
     * context
     */
    KateTextLine ();

    /**
     * Destructor
     */
    ~KateTextLine ();

  /**
   * Methods to get data
   */
  public:
    /**
    * Set the flag that only positions have changed, not folding region begins/ends themselve
    */
    inline void setFoldingColumnsOutdated(bool set) { if (set) m_flags |= KateTextLine::flagFoldingColumnsOutdated; else m_flags&=
                                                      (~KateTextLine::flagFoldingColumnsOutdated);}

    /**
     * folding columns outdated ?
     * @return folding columns outdated?
     */
     inline bool foldingColumnsOutdated() { return m_flags & KateTextLine::flagFoldingColumnsOutdated; }


    /**
     * Returns the length
     * @return length of text in line
     */
    inline int length() const { return m_text.length(); }

    /**
     * has the line the hl continue flag set
     * @return hl continue set?
     */
    inline bool hlLineContinue () const { return m_flags & KateTextLine::flagHlContinue; }

    /**
     * was this line automagically wrapped
     * @return line auto-wrapped
     */
    inline bool isAutoWrapped () const { return m_flags & KateTextLine::flagAutoWrapped; }

    /**
     * Returns the position of the first non-whitespace character
     * @return position of first non-whitespace char or -1 if there is none
     */
    int firstChar() const;

    /**
     * Returns the position of the last non-whitespace character
     * @return position of last non-whitespace char or -1 if there is none
     */
    int lastChar() const;

    /**
     * Find the position of the next char that is not a space.
     * @param pos Column of the character which is examined first.
     * @return True if the specified or a following character is not a space
     *          Otherwise false.
     */
    int nextNonSpaceChar(uint pos) const;

    /**
     * Find the position of the previous char that is not a space.
     * @param pos Column of the character which is examined first.
     * @return The position of the first none-whitespace character preceeding pos,
     *   or -1 if none is found.
     */
    int previousNonSpaceChar(int pos) const;

    /**
     * Gets the char at the given position
     * @param pos position
     * @return character at the given position or QChar::null if position is
     *   beyond the length of the string
     */
    inline QChar getChar (int pos) const
    {
      if (pos >= 0 && pos < m_text.length())
        return m_text[pos];

      return QChar();
    }

    /**
     * Gets a QString
     * @return text of line as QString reference
     */
    inline const QString& string() const { return m_text; }

    /**
     * Gets a substring.
     * @param column start column of substring
     * @param length length of substring
     * @return wanted substring
     */
    inline QString string(int column, int length) const
    { return m_text.mid(column, length); }

    /**
     * indentation depth of this line
     * @param tabwidth width of the tabulators
     * @return indentation width
     */
    int indentDepth (int tabwidth) const;

    /**
     * Calculate the position if we expand the tabs in the string
     * @param pos position in chars
     * @param tabChars tabulator width in chars
     * @return position with tabulators calculated
     */
    int positionWithTabs (int pos, int tabChars) const;

    /**
     * Returns the text length with tabs calced in
     * @param tabChars tabulator width in chars
     * @return text length
     */
    int lengthWithTabs (int tabChars) const;

    /**
     * Can we find the given string at the given position
     * @param pos startpostion of given string
     * @param match string to match at given pos
     * @return did the string match?
     */
    bool stringAtPos(int pos, const QString& match) const;

    /**
     * Is the line starting with the given string
     * @param match string to test
     * @return does line start with given string?
     */
    inline bool startingWith(const QString& match) const { return m_text.startsWith (match); }

    /**
     * Is the line ending with the given string
     * @param match string to test
     * @return does the line end with given string?
     */
    inline bool endingWith(const QString& match) const { return m_text.endsWith (match); }

    /**
     * search given string
     * @param startCol column to start search
     * @param text string to search for
     * @param foundAtCol column where text was found
     * @param matchLen length of matching
     * @param casesensitive should search be case-sensitive
     * @param backwards search backwards?
     * @return string found?
     */
    bool searchText (uint startCol, const QString &text,
                     uint *foundAtCol, uint *matchLen,
                     bool casesensitive = true,
                     bool backwards = false);

    /**
     * search given regexp
     * @param startCol column to start search
     * @param regexp regex to search for
     * @param foundAtCol column where text was found
     * @param matchLen length of matching
     * @param backwards search backwards?
     * @return regexp found?
     */
    bool searchText (uint startCol, const QRegExp &regexp,
                     uint *foundAtCol, uint *matchLen,
                     bool backwards = false);

    /**
     * Gets the attribute at the given position
     * use KRenderer::attributes  to get the KTextAttribute for this.
     *
     * @param pos position of attribute requested
     * @return value of attribute
     */
    inline uchar attribute (int pos) const
    {
      for (int i=0; i < m_attributesList.size(); i+=3)
      {
        if (pos >= m_attributesList[i] && pos <= m_attributesList[i]+m_attributesList[i+1])
          return m_attributesList[i+2];

        if (pos < m_attributesList[i])
          break;
      }

      return 0;
    }

    /**
     * context stack
     * @return context stack
     */
    inline const QVector<short> &ctxArray () const { return m_ctx; };

    /**
     * @return true if any context at the line end has the noIndentBasedFolding flag set
     */
    inline const bool noIndentBasedFolding() const { return m_flags & KateTextLine::flagNoIndentationBasedFolding; };
    inline const bool noIndentBasedFoldingAtStart() const { return m_flags & KateTextLine::flagNoIndentationBasedFoldingAtStart; };
    /**
     * folding list
     * @return folding array
     */
    inline const QVector<int> &foldingListArray () const { return m_foldingList; };

    /**
     * indentation stack
     * @return indentation array
     */
    inline const QVector<unsigned short> &indentationDepthArray () const { return m_indentationDepth; };

    /**
     * insert text into line
     * @param pos insert position
     * @param insLen insert length
     * @param insText text to insert
     */
    void insertText (int pos, const QString& insText);

    /**
     * remove text at given position
     * @param pos start position of remove
     * @param delLen length to remove
     */
    void removeText (uint pos, uint delLen);

    /**
     * Truncates the textline to the new length
     * @param newLen new length of line
     */
    void truncate(int newLen);

    /**
     * set hl continue flag
     * @param cont continue flag?
     */
    inline void setHlLineContinue (bool cont)
    {
      if (cont) m_flags = m_flags | KateTextLine::flagHlContinue;
      else m_flags = m_flags & ~ KateTextLine::flagHlContinue;
    }

    /**
     * auto-wrapped
     * @param wrapped line was wrapped?
     */
    inline void setAutoWrapped (bool wrapped)
    {
      if (wrapped) m_flags = m_flags | KateTextLine::flagAutoWrapped;
      else m_flags = m_flags & ~ KateTextLine::flagAutoWrapped;
    }

    /**
     * Sets the syntax highlight context number
     * @param val new context array
     */
    inline void setContext (QVector<short> &val) { m_ctx = val; }

    /**
     * sets if for the next line indent based folding should be disabled
     */
    inline void setNoIndentBasedFolding(bool val)
    {
      if (val) m_flags = m_flags | KateTextLine::flagNoIndentationBasedFolding;
      else m_flags = m_flags & ~ KateTextLine::flagNoIndentationBasedFolding;
    }

    inline void setNoIndentBasedFoldingAtStart(bool val)
    {
      if (val) m_flags = m_flags | KateTextLine::flagNoIndentationBasedFoldingAtStart;
      else m_flags = m_flags & ~ KateTextLine::flagNoIndentationBasedFoldingAtStart;
    }

    /**
     * update folding list
     * @param val new folding list
     */
    inline void setFoldingList (QVector<int> &val) { m_foldingList = val; }

    /**
     * update indentation stack
     * @param val new indentation stack
     */
    inline void setIndentationDepth (QVector<unsigned short> &val) { m_indentationDepth = val; }

  /**
   * Methodes for dump/restore of the line in the buffer
   */
  public:
    /**
     * Dumpsize in bytes
     * @param withHighlighting should we dump the hl, too?
     * @return size of line for dumping
     */
    inline uint dumpSize (bool withHighlighting) const
    {
      return ( sizeof(uchar) // the cool flags
               + sizeof(uint) // textlen
               + (m_text.length() * sizeof(QChar)) // the TEXT, important, I guess
               + ( withHighlighting ?
                     ( (4 * sizeof(uint)) // 4 lenghts of the following arrays
                       + (m_attributesList.size() * sizeof(int)) // attributes
                       + (m_ctx.size() * sizeof(short)) // context stack
                       + (m_foldingList.size() * sizeof(int)) // folding list
                       + (m_indentationDepth.size() * sizeof(unsigned short)) // indentation depth
                     ) : 0
                 )
             );
    }

    /**
     * Dumps the line to *buf and counts buff dumpSize bytes up
     * as return value
     * @param buf buffer to dump to
     * @param withHighlighting dump hl data, too?
     * @return buffer index after dumping
     */
    char *dump (char *buf, bool withHighlighting) const;

    /**
     * Restores the line from *buf and counts buff dumpSize bytes up
     * as return value
     * @param buf buffer to restore from
     * @return buffer index after restoring
     */
    char *restore (char *buf);

  /**
   * methodes to manipulate the attribute list
   */
  public:
    void addAttribute (int start, int length, int attribute);

    void clearAttributes () { m_attributesList.clear (); }

    const QVector<int> &attributesList () const { return m_attributesList; }

  /**
   * REALLY PRIVATE ;) please no new friend classes
   */
  private:
    /**
     * text of line as unicode
     */
    QString m_text;

    /**
     * new kind to store the attribs, int array
     * one int start, next one len, next one attrib
     */
    QVector<int> m_attributesList;

    /**
     * context stack
     */
    QVector<short> m_ctx;

    /**
     * list of folding starts/ends
     */
    QVector<int> m_foldingList;

    /**
     * indentation stack
     */
    QVector<unsigned short> m_indentationDepth;

    /**
     * flags
     */
    uchar m_flags;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
