/* This file is part of the KDE libraries
   Copyright (C) 2001-2002 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
      
   Based on:
     TextLine : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
     
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

#ifndef _KATE_TEXTLINE_H_
#define _KATE_TEXTLINE_H_

#include <qmemarray.h>
#include <ksharedptr.h>
#include <qvaluevector.h>
 
/** 
  The TextLine represents a line of text. A text line that contains the  
  text, an attribute for each character, an attribute for the free space  
  behind the last character and a context number for the syntax highlight.  
  The attribute stores the index to a table that contains fonts and colors  
  and also if a character is selected.  
*/ 
 
class TextLine : public KShared  
{  
  public:
    typedef KSharedPtr<TextLine> Ptr;
    typedef QValueVector<Ptr> List;

  public:  
    /**  
      Creates an empty text line with given attribute and syntax highlight  
      context  
    */  
    TextLine(); 
    ~TextLine();

  /**
    Methodes to get data
  */
  public:
    /**
      Returns the length
    */
    inline uint length() const { return m_text.size(); }

    /**
      Returns the visibility flag
    */
    inline bool isVisible() const { return m_flags & TextLine::flagVisible; }  
     
    inline bool isFoldingListValid() const { return m_flags & TextLine::flagFoldingListValid; }

    /**
      Returns the position of the first character which is not a white space
    */
    int firstChar() const;
    
    /**
      Returns the position of the last character which is not a white space
    */
    int lastChar() const;
    
    /**
      Returns the position of the first non-space char after a given position
    */
    int nextNonSpaceChar(uint pos) const;
    
    /**
      Returns the position of the last non-space char before a given position
    */
    int previousNonSpaceChar(uint pos) const;
                                              
    /**
      Gets the char at the given position
    */
    inline QChar getChar (uint pos) const
    {
      if (pos < m_text.size()) return m_text[pos];
      return QChar(' ');
    }
    
    /**
      Gets the text. WARNING: it is not null terminated
    */
    inline QChar *text() const { return m_text.data(); };    
    
    inline uchar *attributes () const { return m_attributes.data(); }  
    
    /**
      Gets a QString
    */
    inline QString string() const { return QString (m_text.data(), m_text.size()); };
    
    /**
      Gets a QString
    */
    QString string (uint startCol, uint length) const;

    /*
      Gets a null terminated pointer to first non space char
    */
    QChar *firstNonSpace() const;
    
    /**
      Returns the x position of the cursor at the given position, which
      depends on the number of tab characters
    */
    int cursorX(uint pos, uint tabChars) const;
    
    /**
      Can we find the given string at the given position
    */
    bool stringAtPos(uint pos, const QString& match) const;
    
    /**
      Is the line starting with the given string
    */
    bool startingWith(const QString& match) const;
    
    /**
      Is the line ending with the given string
    */
    bool endingWith(const QString& match) const;    
    
    /**
      Gets the syntax highlight context number
    */
    inline signed char *ctx () const { return m_ctx.data(); };
    
    /**
      Gets the syntax highlight context number
    */
    inline uint ctxLength() const { return m_ctx.size(); };

    bool searchText (unsigned int startCol, const QString &text, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive = true, bool backwards = false);
    bool searchText (unsigned int startCol, const QRegExp &regexp, unsigned int *foundAtCol, unsigned int *matchLen, bool backwards = false); 
    
     /**
      Gets the attribute at the given position
    */
    inline uchar attribute (uint pos) const
    {
      if (pos < m_text.size()) return m_attributes[pos];
      return 0;
    }
        
    inline bool hlLineContinue () const
    {
      return m_flags & TextLine::flagHlContinue;
    }
                                                                                                       
    /**
      Raw access on the memarray's, for example the katebuffer class
    */
    inline const QMemArray<QChar> &textArray () const { return m_text; };
    inline const QMemArray<unsigned char> &attributesArray () const { return m_attributes; };
    inline const QMemArray<signed char> &ctxArray () const { return m_ctx; };
    inline const QMemArray<signed char> &foldingListArray () const { return m_foldingList; };      
         
  /**
    Methodes to manipulate data
  */
  public:
     /**
      Universal text manipulation method. It can be used to insert, delete
      or replace text.
    */
    void replace(uint pos, uint delLen, const QChar *insText, uint insLen, uchar *insAttribs = 0L);
    
    /**
      Appends a string of length l to the textline
    */
    void append(const QChar *s, uint l);
    
    /**
      Wraps the text from the given position to the end to the next line
    */
    void wrap(TextLine::Ptr nextLine, uint pos);
    
    /**
      Wraps the text of given length from the beginning of the next line to
      this line at the given position
    */
    void unWrap(uint pos, TextLine::Ptr nextLine, uint len);
    
    /**
      Truncates the textline to the new length
    */
    void truncate(uint newLen);
    
    /**
      Removes trailing spaces
    */
    void removeSpaces();     
    
    /**
      Sets the visibility flag
    */
    inline void setVisible(bool val)
    {
      if (val) m_flags = m_flags | TextLine::flagVisible;
      else m_flags = m_flags & ~ TextLine::flagVisible;
    }
    
    /**
      Sets the attributes from start to end -1
    */
    void setAttribs(uchar attribute, uint start, uint end);
        
    /**
      Sets the syntax highlight context number
    */
    inline void setContext(signed char *newctx, uint len)
    {
      m_ctx.duplicate (newctx, len);
    }
    
    inline void setHlLineContinue (bool cont)
    {
      if (cont) m_flags = m_flags | TextLine::flagHlContinue;
      else m_flags = m_flags & ~ TextLine::flagHlContinue;
    }
        
    inline void setFoldingList (QMemArray<signed char> &val)
    {
      m_foldingList=val;
      m_foldingList.detach();
      m_flags = m_flags | TextLine::flagFoldingListValid;
    }     
  
  /**
    Methodes for dump/restore of the line in the buffer
  */  
  public:                      
    /**
      Dumpsize in bytes
    */
    uint dumpSize () const;
    
    /**
      Dumps the line to *buf and counts buff dumpSize bytes up 
      as return value
    */
    char *dump (char *buf) const;
                                        
    /**
      Restores the line from *buf and counts buff dumpSize bytes up 
      as return value
    */
    char *restore (char *buf);
                  
  /**
   REALLY PRIVATE ;) please no new friend classes
   */
  private:
    /**
      The text & attributes
    */
    QMemArray<QChar> m_text;
    QMemArray<unsigned char> m_attributes;
    
    /**
     Data for context + folding 
     */
    QMemArray<signed char> m_ctx; 
    QMemArray<signed char> m_foldingList;
                                     
    enum Flags
    {
      flagHlContinue,
      flagVisible,
      flagFoldingListValid
    };
    
    /**
     Some bools packed
    */
    uchar m_flags;
};

#endif
