/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "kateglobal.h"

#include <qmemarray.h>
#include <qstring.h>
#include <qvaluevector.h>
#include <ksharedptr.h>
#include <qregexp.h>
 
/** 
  The TextLine represents a line of text. A text line that contains the  
  text, an attribute for each character, an attribute for the free space  
  behind the last character and a context number for the syntax highlight.  
  The attribute stores the index to a table that contains fonts and colors  
  and also if a character is selected.  
*/ 
 
class TextLine : public KShared  
{  
  friend class KateBuffer;  
  friend class KateBufBlock;
  
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
      Returns the length  
    */
    uint length() const { return text.size(); }

    /**
      Returns the visibility flag
     */
     bool isVisible() {return m_visible;}

     /**
      Sets the visibility flag
      */
     void setVisible(bool val){m_visible=val;}

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
      Returns the position of the first character which is not a white space
    */
    int firstChar() const;
    /**
      Returns the position of the last character which is not a white space
    */
    int lastChar() const;
    /**
      Removes trailing spaces
    */
    void removeSpaces();
    /**
      Gets the char at the given position
    */
    QChar getChar(uint pos) const;
    /**
      Gets the text. WARNING: it is not null terminated
    */
    QChar *getText() const { return text.data(); };
    /**
      Gets a C-like null terminated string
    */
    QString getString() { return QString (text.data(), text.size()); };

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
      Is the line starting with the given string
    */
    bool startingWith(const QString& match) const;
    /**
      Is the line ending with the given string
    */
    bool endingWith(const QString& match) const;
    /**
      Sets the attributes from start to end -1
    */
    void setAttribs(uchar attribute, uint start, uint end);
    /**
      Sets the attribute for the free space behind the last character
    */
    void setAttr(uchar attribute);
    /**
      Gets the attribute at the given position
    */
    uchar getAttr(uint pos) const;
    /**
      Gets the attribute for the free space behind the last character
    */
    uchar getAttr() const;
    /**
      Sets the syntax highlight context number
    */
    void setContext(signed char *newctx, uint len);
    /**
      Gets the syntax highlight context number
    */
    signed char *getContext() const { return ctx.data(); };
		/**
      Gets the syntax highlight context number
    */
    uint getContextLength() const { return ctx.size(); };

    bool searchText (unsigned int startCol, const QString &text, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive = true, bool backwards = false);
    bool searchText (unsigned int startCol, const QRegExp &regexp, unsigned int *foundAtCol, unsigned int *matchLen, bool backwards = false);

    uchar *getAttribs() { return attributes.data(); }

    void setHlLineContinue (bool cont) { hlContinue = cont; }
    bool getHlLineContinue () { return hlContinue; }

    QMemArray<QChar> textArray () { return text; };
    QMemArray<unsigned char> attributesArray () { return attributes; };
    QMemArray<signed char> ctxArray () { return ctx; };
    QValueList<signed char> foldingList;
    bool foldingListValid;
    void setFoldingList(QValueList<signed char> val){foldingList=val; foldingListValid=true;}
  private:
    /**
      The text & attributes
    */
    QMemArray<QChar> text;
    QMemArray<unsigned char> attributes;
    QMemArray<signed char> ctx;

    /**
      The attribute of the free space behind the end
    */
    uchar attr;

    /**
      The line continue flag
    */
    bool hlContinue;

    bool m_visible;
};

#endif
