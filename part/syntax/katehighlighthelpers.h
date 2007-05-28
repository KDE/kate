/* This file is part of the KDE libraries
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_HIGHLIGHTHELPERS_H__
#define __KATE_HIGHLIGHTHELPERS_H__

#include "katehighlight.h"

class KateHlItem
{
  public:
    KateHlItem(int attribute, KateHlContextModification context,signed char regionId, signed char regionId2);
    virtual ~KateHlItem();

  public:
    // caller must keep in mind: LEN > 0 is a must !!!!!!!!!!!!!!!!!!!!!1
    // Now, the function returns the offset detected, or 0 if no match is found.
    // bool linestart isn't needed, this is equivalent to offset == 0.
    virtual int checkHgl(const QString& text, int offset, int len) = 0;

    virtual bool lineContinue(){return false;}

    virtual QStringList *capturedTexts() {return 0;}
    virtual KateHlItem *clone(const QStringList *) {return this;}

    static void dynamicSubstitute(QString& str, const QStringList *args);

    QVector<KateHlItem*> subItems;
    int attr;
    KateHlContextModification ctx;
    signed char region;
    signed char region2;

    bool lookAhead;

    bool dynamic;
    bool dynamicChild;
    bool firstNonSpace;
    bool onlyConsume;
    int column;

    // start enable flags, nicer than the virtual methodes
    // saves function calls
    bool alwaysStartEnable;
    bool customStartEnable;
};

class KateHlContext
{
  public:
    KateHlContext(const QString &_hlId, int attribute, KateHlContextModification _lineEndContext,KateHlContextModification _lineBeginContext,
                  bool _fallthrough, KateHlContextModification _fallthroughContext, bool _dynamic,bool _noIndentationBasedFolding);
    virtual ~KateHlContext();
    KateHlContext *clone(const QStringList *args);

    QVector<KateHlItem*> items;
    QString hlId; ///< A unique highlight identifier. Used to look up correct properties.
    int attr;
    KateHlContextModification lineEndContext;
    KateHlContextModification lineBeginContext;
    /** @internal anders: possible escape if no rules matches.
       false unless 'fallthrough="1|true"' (insensitive)
       if true, go to ftcxt w/o eating of string.
       ftctx is "fallthroughContext" in xml files, valid values are int or #pop[..]
       see in KateHighlighting::doHighlight */
    bool fallthrough;
    KateHlContextModification ftctx; // where to go after no rules matched

    bool dynamic;
    bool dynamicChild;
    bool noIndentationBasedFolding;
};

class KateHlIncludeRule
{
  public:
    explicit KateHlIncludeRule(int ctx_=0, uint pos_=0, const QString &incCtxN_="", bool incAttrib=false)
      : ctx(ctx_)
      , pos( pos_)
      , incCtxN( incCtxN_ )
      , includeAttrib( incAttrib )
    {
      incCtx=-1;
    }
    //KateHlIncludeRule(int ctx_, uint  pos_, bool incAttrib) {ctx=ctx_;pos=pos_;incCtx=-1;incCtxN="";includeAttrib=incAttrib}

  public:
    int ctx;
    uint pos;
    KateHlContextModification incCtx;
    QString incCtxN;
    bool includeAttrib;
};

class KateHlCharDetect : public KateHlItem
{
  public:
    KateHlCharDetect(int attribute, KateHlContextModification context,signed char regionId,signed char regionId2, QChar);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual KateHlItem *clone(const QStringList *args);

  private:
    QChar sChar;
};

class KateHl2CharDetect : public KateHlItem
{
  public:
    KateHl2CharDetect(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2,  QChar ch1, QChar ch2);
    KateHl2CharDetect(int attribute, KateHlContextModification context,signed char regionId,signed char regionId2,  const QChar *ch);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual KateHlItem *clone(const QStringList *args);

  private:
    QChar sChar1;
    QChar sChar2;
};

class KateHlStringDetect : public KateHlItem
{
  public:
    KateHlStringDetect(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2, const QString &, bool inSensitive=false);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual KateHlItem *clone(const QStringList *args);

  private:
    const QString str;
    const int strLen;
    const bool _inSensitive;
};

class KateHlRangeDetect : public KateHlItem
{
  public:
    KateHlRangeDetect(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2);

    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    QChar sChar1;
    QChar sChar2;
};

class KateHlKeyword : public KateHlItem
{
  public:
    KateHlKeyword(int attribute, KateHlContextModification context,signed char regionId,signed char regionId2, bool insensitive, const QString& delims);
    virtual ~KateHlKeyword ();

    void addList(const QStringList &);
    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    QVector< QSet<QString>* > dict;
    bool _insensitive;
    const QString& deliminators;
    int minLen;
    int maxLen;
};

class KateHlInt : public KateHlItem
{
  public:
    KateHlInt(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlFloat : public KateHlItem
{
  public:
    KateHlFloat(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2);
    virtual ~KateHlFloat () {}

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlCFloat : public KateHlFloat
{
  public:
    KateHlCFloat(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    int checkIntHgl(const QString& text, int offset, int len);
};

class KateHlCOct : public KateHlItem
{
  public:
    KateHlCOct(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlCHex : public KateHlItem
{
  public:
    KateHlCHex(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlLineContinue : public KateHlItem
{
  public:
    KateHlLineContinue(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2);

    virtual bool endEnable(QChar c) {return c == '\0';}
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool lineContinue(){return true;}
};

class KateHlCStringChar : public KateHlItem
{
  public:
    KateHlCStringChar(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlCChar : public KateHlItem
{
  public:
    KateHlCChar(int attribute, KateHlContextModification context,signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlAnyChar : public KateHlItem
{
  public:
    KateHlAnyChar(int attribute, KateHlContextModification context, signed char regionId,signed char regionId2, const QString& charList);

    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    const QString _charList;
};

class KateHlRegExpr : public KateHlItem
{
  public:
    KateHlRegExpr(int attribute, KateHlContextModification context,signed char regionId,signed char regionId2 ,const QString &expr, bool insensitive, bool minimal);
    ~KateHlRegExpr() { delete Expr; }

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual QStringList *capturedTexts();
    virtual KateHlItem *clone(const QStringList *args);

  private:
    QRegExp *Expr;
    bool handlesLinestart;
    QString _regexp;
    bool _insensitive;
    bool _minimal;
};

class KateHlDetectSpaces : public KateHlItem
{
  public:
    KateHlDetectSpaces (int attribute, KateHlContextModification context,signed char regionId,signed char regionId2)
      : KateHlItem(attribute,context,regionId,regionId2) {}

    virtual int checkHgl(const QString& text, int offset, int len)
    {
      int len2 = offset + len;
      while ((offset < len2) && text[offset].isSpace()) offset++;
      return offset;
    }
};

class KateHlDetectIdentifier : public KateHlItem
{
  public:
    KateHlDetectIdentifier (int attribute, KateHlContextModification context,signed char regionId,signed char regionId2)
      : KateHlItem(attribute,context,regionId,regionId2) { alwaysStartEnable = false; }

    virtual int checkHgl(const QString& text, int offset, int len)
    {
      // first char should be a letter or underscore
      if ( text[offset].isLetter() || text[offset] == QChar ('_') )
      {
        // memorize length
        int len2 = offset+len;

        // one char seen
        offset++;

        // now loop for all other thingies
        while (
               (offset < len2)
               && (text[offset].isLetterOrNumber() || (text[offset] == QChar ('_')))
              )
          offset++;

        return offset;
      }

      return 0;
    }
};

//END

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
