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
      flagHlContinue = 1,
      flagAutoWrapped = 2,
      flagFoldingColumnsOutdated = 4,
      flagNoIndentationBasedFolding = 8,
      flagNoIndentationBasedFoldingAtStart = 16
    };

  public:
    /**
     * Constructor
     * Creates an empty text line with given attribute and syntax highlight
     * context
     */
    KateTextLine ();
    
    KateTextLine (const QChar *data, int length);

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
     * Returns \e true, if the folding colums are outdated, otherwise returns \e false.
     */
     inline bool foldingColumnsOutdated() { return m_flags & KateTextLine::flagFoldingColumnsOutdated; }


    /**
     * Returns the line's length.
     */
    inline int length() const { return m_text.length(); }

    /**
     * Returns \e true, if the line's hl-continue flag is set, otherwise returns
     * \e false. The hl-continue flag is set in the hl-definition files.
     */
    inline bool hlLineContinue () const { return m_flags & KateTextLine::flagHlContinue; }

    /**
     * Returns \e true, if the line was automagically wrapped, otherwise returns
     * \e false.
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
     * @return The position of the first non-whitespace character preceding pos,
     *   or -1 if none is found.
     */
    int previousNonSpaceChar(int pos) const;

    /**
     * Returns the character at the given \e column. If \e column is out of
     * range, the return value is QChar().
     */
    inline QChar at (int column) const
    {
      if (column >= 0 && column < m_text.length())
        return m_text[column];

      return QChar();
    }

    /**
     * Same as at().
     */
    inline QChar operator[](int column) const
    {
      if (column >= 0 && column < m_text.length())
        return m_text[column];

      return QChar();
    }

    /**
     * Returns the complete text line (as a QString reference).
     */
    inline const QString& string() const { return m_text; }

    /**
     * Returns the substring with \e length beginning at the given \e column.
     */
    inline QString string(int column, int length) const
    { return m_text.mid(column, length); }

    /**
     * Returns the indentation depth with each tab expanded into \e tabWidth characters.
     */
    int indentDepth (int tabWidth) const;

    /**
     * Returns the \e column with each tab expanded into \e tabWidth characters.
     */
    int toVirtualColumn (int column, int tabWidth) const;

    /**
     * Returns the "real" column where each tab only counts one character.
     * The conversion calculates with \e tabWidth characters for each tab.
     */
    int fromVirtualColumn (int column, int tabWidth) const;

    /**
     * Returns the text length with each tab expanded into \e tabWidth characters.
     */
    int virtualLength (int tabWidth) const;

    /**
     * Returns \e true, if \e match equals to the text at position \e column,
     * otherwise returns \e false.
     */
    bool matchesAt(int column, const QString& match) const;

    /**
     * Returns \e true, if the line starts with \e match, otherwise returns \e false.
     */
    inline bool startsWith(const QString& match) const { return m_text.startsWith (match); }

    /**
     * Returns \e true, if the line ends with \e match, otherwise returns \e false.
     */
    inline bool endsWith(const QString& match) const { return m_text.endsWith (match); }

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
