/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KATE_TEXTLINE_H__
#define __KATE_TEXTLINE_H__

#include <ksharedptr.h>

#include <qmemarray.h>
#include <qstring.h>

class KateRenderer;

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
      flagNoOtherData = 0x1, // ONLY INTERNAL USE, NEVER EVER SET THAT !!!!
      flagHlContinue = 0x2,
      flagAutoWrapped = 0x4,
      flagFoldingColumnsOutdated=0x8
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
    inline uint length() const { return m_text.length(); }

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
    int previousNonSpaceChar(uint pos) const;

    /**
     * Gets the char at the given position
     * @param pos position
     * @return character at the given position or QChar::null if position is
     *   beyond the length of the string
     */
    inline QChar getChar (uint pos) const { return m_text[pos]; }

    /**
     * Gets the text as a unicode representation
     * @return text of this line as QChar array
     */
    inline const QChar *text() const { return m_text.unicode(); }

    /**
     * Highlighting array
     * The size of this is string().length()
     *
     * This contains the index for the attributes (so you can only
     * have a maximum of 2^8 different highlighting styles in a document)
     * 
     * To turn this into actual attributes (bold, green, etc), 
     * you need to feed these values into KRenderer::attributes
     *
     * e.g:  m_renderer->attributes(attributes[3]);
     *
     * @return hl-attributes array
     */
    inline uchar *attributes () const { return m_attributes.data(); }

    /**
     * Gets a QString
     * @return text of line as QString reference
     */
    inline const QString& string() const { return m_text; }

    /**
     * Gets a substring.
     * @param startCol start column of substring
     * @param length length of substring
     * @return wanted substring
     */
    inline QString string(uint startCol, uint length) const
    { return m_text.mid(startCol, length); }

    /**
     * Gets a substring in valid-xml html.
     * Example:  "<b>const</b> b = <i>34</i>"
     * It won't contain <p> or <body> or <html> or anything like that.
     *
     * @param startCol start column of substring
     * @param length length of substring
     * @param renderer The katerenderer.  This will have the schema
     *                 information that describes how to render the
     *                 attributes.
     * @return wanted valid-xml html substring
     */
    QString stringAsHtml(uint startCol, uint length, KateRenderer *renderer) const;

    /**
     * Gets the string a valid-xml html.
     * Overloaded version of stringAsHtml
     * 
     * @param renderer The katerenderer.  This will have the schema
     *                 information that describes how to render the
     *                 attributes.
     * @return wanted valid-xml html substring
     */
    QString stringAsHtml(KateRenderer *renderer) const
    { return stringAsHtml(0,m_text.length(),renderer);}
    
    /**
     * Gets a null terminated pointer to first non space char
     * @return array of QChars starting at first non-whitespace char
     */
    const QChar *firstNonSpace() const;

    /**
     * indentation depth of this line
     * @param tabwidth width of the tabulators
     * @return indentation width
     */
    uint indentDepth (uint tabwidth) const;

    /**
     * Returns the x position of the cursor at the given position, which
     * depends on the number of tab characters
     * @param pos position in chars
     * @param tabChars tabulator width in chars
     * @return position with tabulators calculated
     */
    int cursorX(uint pos, uint tabChars) const;

    /**
     * Returns the text length with tabs calced in
     * @param tabChars tabulator width in chars
     * @return text length
     */
    uint lengthWithTabs (uint tabChars) const;

    /**
     * Can we find the given string at the given position
     * @param pos startpostion of given string
     * @param match string to match at given pos
     * @return did the string match?
     */
    bool stringAtPos(uint pos, const QString& match) const;

    /**
     * Is the line starting with the given string
     * @param match string to test
     * @return does line start with given string?
     */
    bool startingWith(const QString& match) const;

    /**
     * Is the line ending with the given string
     * @param match string to test
     * @return does the line end with given string?
     */
    bool endingWith(const QString& match) const;

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
     * @see attributes
     */
    inline uchar attribute (uint pos) const
    {
      if (pos < m_attributes.size()) return m_attributes[pos];
      return 0;
    }

    /**
     * context stack
     * @return context stack
     */
    inline const QMemArray<short> &ctxArray () const { return m_ctx; };

    /**
     * folding list
     * @return folding array
     */
    inline const QMemArray<uint> &foldingListArray () const { return m_foldingList; };

    /**
     * indentation stack
     * @return indentation array
     */
    inline const QMemArray<unsigned short> &indentationDepthArray () const { return m_indentationDepth; };

    /**
     * insert text into line
     * @param pos insert position
     * @param insLen insert length
     * @param insText text to insert
     * @param insAttribs attributes for the insert text
     */
    void insertText (uint pos, uint insLen, const QChar *insText, uchar *insAttribs = 0);

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
    void truncate(uint newLen);

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
    inline void setContext (QMemArray<short> &val) { m_ctx.assign (val); }

    /**
     * update folding list
     * @param val new folding list
     */
    inline void setFoldingList (QMemArray<uint> &val) { m_foldingList.assign (val); m_foldingList.detach(); }

    /**
     * update indentation stack
     * @param val new indentation stack
     */
    inline void setIndentationDepth (QMemArray<unsigned short> &val) { m_indentationDepth.assign (val); }

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
      return ( 1
               + sizeof(uint)
               + (m_text.length() * sizeof(QChar))
               + ( withHighlighting ?
                     ( (3 * sizeof(uint))
                       + (m_text.length() * sizeof(uchar))
                       + (m_ctx.size() * sizeof(short))
                       + (m_foldingList.size() * sizeof(uint))
                       + (m_indentationDepth.size() * sizeof(unsigned short))
                     ) : 0
                 )
             );
    }

    /**
     * Dumps the line to *buf and counts buff dumpSize bytes up
     * as return value
     * @param buf buffer to dump to
     * @param withHighlight dump hl data, too?
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
   * REALLY PRIVATE ;) please no new friend classes
   */
  private:
    /**
     * text of line as unicode
     */
    QString m_text;

    /**
     * array of highlighting attributes
     * This is exactly the same size as m_text.length()
     * Each letter in m_text has a uchar attribute
     */
    QMemArray<uchar> m_attributes;

    /**
     * context stack
     */
    QMemArray<short> m_ctx;

    /**
     * list of folding starts/ends
     */
    QMemArray<uint> m_foldingList;

    /**
     * indentation stack
     */
    QMemArray<unsigned short> m_indentationDepth;

    /**
     * flags
     */
    uchar m_flags;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
