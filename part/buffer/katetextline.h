/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_TEXTLINE_H
#define KATE_TEXTLINE_H

#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

#include "katepartprivate_export.h"

namespace Kate {

/**
 * Class representing a single text line.
 * For efficience reasons, not only pure text is stored here, but also additional data.
 * Will be only accessed over shared pointers.
 */
class KATEPART_TESTS_EXPORT TextLineData {
  /**
   * TexBlock is a friend class, only one allowed to touch the text content.
   */
  friend class TextBlock;

  public:
    /**
     * Context stack
     */
    typedef QVector<short> ContextStack;
    
    /**
     * Attribute storage
     */
    class Attribute {
      public:
        /**
         * Attribute constructor
         * @param _offset offset
         * @param _length length
         * @param _attributeValue attribute value
         * @param _foldingValue folding value
         */
        Attribute (int _offset = 0, int _length = 0, short _attributeValue = 0, short _foldingValue = 0)
          : offset (_offset)
          , length (_length)
          , attributeValue (_attributeValue)
          , foldingValue (_foldingValue)
        {
        }
        
        /**
         * offset
         */  
        int offset;
        
        /**
         * length
         */
        int length;
        
        /**
         * attribute value (to encode type of this range)
         */
        short attributeValue;
        
        /**
         * folding value (begin/end type)
         */
        short foldingValue;
    };
    
    /**
     * Flags of TextLineData
     */
    enum Flags
    {
      flagHlContinue = 1,
      flagAutoWrapped = 2,
      flagFoldingStartAttribute = 4,
      flagFoldingStartIndentation = 8,
      flagLineModified = 16,
      flagLineSavedOnDisk = 32
    };

    /**
     * Construct an empty text line.
     */
    TextLineData ();

    /**
     * Construct an text line with given text.
     * @param text text to use for this line
     */
    TextLineData (const QString &text);

    /**
     * Destruct the text line
     */
    ~TextLineData ();

    /**
     * Accessor to the text contained in this line.
     * @return text of this line as constant reference
     */
    const QString &text () const { return m_text; }

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
    int nextNonSpaceChar(int pos) const;

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
     * @param column column you want char for
     * @return char at given column or QChar()
     */
    inline QChar at (int column) const
    {
      if (column >= 0 && column < m_text.length())
        return m_text[column];

      return QChar();
    }

    /**
     * Same as at().
     * @param column column you want char for
     * @return char at given column or QChar()
     */
    inline QChar operator[](int column) const
    {
      if (column >= 0 && column < m_text.length())
        return m_text[column];

      return QChar();
    }

    inline void markAsModified(bool modified)
    {
      if (modified) {
        m_flags |= flagLineModified;
        m_flags &= (~flagLineSavedOnDisk);
      } else {
        m_flags &= (~flagLineModified);
      }
    }

    inline bool markedAsModified() const
    {
      return m_flags & flagLineModified;
    }

    inline void markAsSavedOnDisk(bool savedOnDisk)
    {
      if (savedOnDisk) {
        m_flags |= flagLineSavedOnDisk;
        m_flags &= (~flagLineModified);
      } else {
        m_flags &= (~flagLineSavedOnDisk);
      }
    }

    inline bool markedAsSavedOnDisk() const
    {
      return m_flags & flagLineSavedOnDisk;
    }

    /**
     * Is on this line a folding start?
     * @return folding start line or not?
     */
    bool markedAsFoldingStart() const
    {
      return m_flags & (flagFoldingStartAttribute | flagFoldingStartIndentation);
    }

    /**
     * Clear folding start status.
     */
    void clearMarkedAsFoldingStart ()
    {
      m_flags &= ~(flagFoldingStartAttribute | flagFoldingStartIndentation);
    }

    /**
     * Is on this line a folding start per attribute?
     * @return folding start line per attribute? or not?
     */
    bool markedAsFoldingStartAttribute() const
    {
      return m_flags & flagFoldingStartAttribute;
    }

    /**
     * Is on this line a folding start per indentation?
     * @return folding start line per indentation? or not?
     */
    bool markedAsFoldingStartIndentation() const
    {
      return m_flags & flagFoldingStartIndentation;
    }

    /**
     * Mark as folding start line of an attribute based folding.
     */
    void markAsFoldingStartAttribute ()
    {
      clearMarkedAsFoldingStart ();
      m_flags |= flagFoldingStartAttribute;
    }

    /**
     * Mark as folding start line of an indentation based folding.
     */
    void markAsFoldingStartIndentation ()
    {
      clearMarkedAsFoldingStart ();
      m_flags |= flagFoldingStartIndentation;
    }

    /**
     * Returns the line's length.
     */
    int length() const { return m_text.length(); }

    /**
     * Returns \e true, if the line's hl-continue flag is set, otherwise returns
     * \e false. The hl-continue flag is set in the hl-definition files.
     * @return hl-continue flag is set
     */
    bool hlLineContinue () const { return m_flags & flagHlContinue; }

    /**
     * Returns \e true, if the line was automagically wrapped, otherwise returns
     * \e false.
     * @return was this line auto-wrapped?
     */
    bool isAutoWrapped () const { return m_flags & flagAutoWrapped; }

    /**
     * Returns the complete text line (as a QString reference).
     * @return text of this line, read-only
     */
    const QString& string() const { return m_text; }

    /**
     * Returns the substring with \e length beginning at the given \e column.
     * @param column start column of text to return
     * @param length length of text to return
     * @return wanted part of text
     */
    QString string (int column, int length) const
    { return m_text.mid(column, length); }

    /**
     * Leading whitespace of this line
     * @return leading whitespace of this line
     */
    QString leadingWhitespace() const;

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
    bool startsWith(const QString& match) const { return m_text.startsWith (match); }

    /**
     * Returns \e true, if the line ends with \e match, otherwise returns \e false.
     */
    bool endsWith(const QString& match) const { return m_text.endsWith (match); }

    /**
     * context stack
     * @return context stack
     */
    const ContextStack &contextStack () const { return m_contextStack; }

    /**
     * Sets the syntax highlight context number
     * @param val new context array
     */
    void setContextStack (const ContextStack &val) { m_contextStack = val; }

    /**
     * Add attribute to this line.
     * @param attribute new attribute to append
     */
    void addAttribute (const Attribute &attribute);

    /**
     * Clear attributes of this line
     */
    void clearAttributes () { m_attributesList.clear (); }

    /**
     * Accessor to attributes
     * @return attributes of this line
     */
    const QVector<Attribute> &attributesList () const { return m_attributesList; }

    /**
     * Gets the attribute at the given position
     * use KRenderer::attributes  to get the KTextAttribute for this.
     *
     * @param pos position of attribute requested
     * @return value of attribute
     */
    short attribute (int pos) const
    {
      for (int i=0; i < m_attributesList.size(); ++i)
      {
        if (pos >= m_attributesList[i].offset && pos < (m_attributesList[i].offset + m_attributesList[i].length))
          return m_attributesList[i].attributeValue;

        if (pos < m_attributesList[i].offset)
          break;
      }

      return 0;
    }

    /**
     * set hl continue flag
     * @param cont continue flag?
     */
    void setHlLineContinue (bool cont)
    {
      if (cont) m_flags = m_flags | flagHlContinue;
      else m_flags = m_flags & ~ flagHlContinue;
    }

    /**
     * set auto-wrapped property
     * @param wrapped line was wrapped?
     */
    void setAutoWrapped (bool wrapped)
    {
      if (wrapped) m_flags = m_flags | flagAutoWrapped;
      else m_flags = m_flags & ~ flagAutoWrapped;
    }

  private:
    /**
     * Accessor to the text contained in this line.
     * This accessor is private, only the friend class text buffer/block is allowed to access the text read/write.
     * @return text of this line
     */
    QString &textReadWrite () { return m_text; }

  private:
    /**
     * text of this line
     */
    QString m_text;

    /**
     * attributes of this line
     */
    QVector<Attribute> m_attributesList;

    /**
     * context stack of this line
     */
    ContextStack m_contextStack;

    /**
     * flags of this line
     */
    unsigned int m_flags;
};

/**
 * The normal world only accesses the text lines with shared pointers.
 */
typedef QSharedPointer<TextLineData> TextLine;

}

#endif
