/* This file is part of the KDE libraries
   Copyright (C) 2003, 2004 Anders Lund <anders@alweb.dk>
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

//BEGIN INCLUDES
#include "katehighlight.h"
#include "katehighlight.moc"

#include "katetextline.h"
#include "katedocument.h"
#include "katesyntaxdocument.h"
#include "katerenderer.h"
#include "katefactory.h"
#include "kateschema.h"
#include "kateconfig.h"

#include <kconfig.h>
#include <kglobal.h>
#include <kinstance.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kregexp.h>
#include <kpopupmenu.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kstaticdeleter.h>
#include <kapplication.h>

#include <qstringlist.h>
#include <qtextstream.h>
//END

// BEGIN defines
// same as in kmimemagic, no need to feed more data
#define KATE_HL_HOWMANY 1024

// min. x seconds between two dynamic contexts reset
static const int KATE_DYNAMIC_CONTEXTS_RESET_DELAY = 30 * 1000;

// x is a QString. if x is "true" or "1" this expression returns "true"
#define IS_TRUE(x) x.lower() == QString("true") || x.toInt() == 1
// END defines

//BEGIN  Prviate HL classes

inline bool kateInsideString (const QString &str, QChar ch)
{
  for (uint i=0; i < str.length(); i++)
    if (*(str.unicode()+i) == ch)
      return true;

  return false;
}

class KateHlItem
{
  public:
    KateHlItem(int attribute, int context,signed char regionId, signed char regionId2);
    virtual ~KateHlItem();

  public:
    virtual bool alwaysStartEnable() const { return true; };
    virtual bool hasCustomStartEnable() const { return false; };
    virtual bool startEnable(const QChar&);

    // Changed from using QChar*, because it makes the regular expression check very
    // inefficient (forces it to copy the string, very bad for long strings)
    // Now, the function returns the offset detected, or 0 if no match is found.
    // bool linestart isn't needed, this is equivalent to offset == 0.
    virtual int checkHgl(const QString& text, int offset, int len) = 0;

    virtual bool lineContinue(){return false;}

    virtual QStringList *capturedTexts() {return 0;}
    virtual KateHlItem *clone(const QStringList *) {return this;}

    static void dynamicSubstitute(QString& str, const QStringList *args);

    QMemArray<KateHlItem*> subItems;
    int attr;
    int ctx;
    signed char region;
    signed char region2;

    bool lookAhead;

    bool dynamic;
    bool dynamicChild;
    bool firstNonSpace;
    bool justConsume;
    int column;
};

class KateHlContext
{
  public:
    KateHlContext(int attribute, int lineEndContext,int _lineBeginContext,
                  bool _fallthrough, int _fallthroughContext, bool _dynamic);
    virtual ~KateHlContext();
    KateHlContext *clone(const QStringList *args);

    QValueVector<KateHlItem*> items;
    int attr;
    int ctx;
    int lineBeginContext;
    /** @internal anders: possible escape if no rules matches.
       false unless 'fallthrough="1|true"' (insensitive)
       if true, go to ftcxt w/o eating of string.
       ftctx is "fallthroughContext" in xml files, valid values are int or #pop[..]
       see in KateHighlighting::doHighlight */
    bool fallthrough;
    int ftctx; // where to go after no rules matched

    bool dynamic;
    bool dynamicChild;
};

class KateEmbeddedHlInfo
{
  public:
    KateEmbeddedHlInfo() {loaded=false;context0=-1;}
    KateEmbeddedHlInfo(bool l, int ctx0) {loaded=l;context0=ctx0;}

  public:
    bool loaded;
    int context0;
};

class KateHlIncludeRule
{
  public:
    KateHlIncludeRule(int ctx_=0, uint pos_=0, const QString &incCtxN_="", bool incAttrib=false)
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
    int incCtx;
    QString incCtxN;
    bool includeAttrib;
};

class KateHlCharDetect : public KateHlItem
{
  public:
    KateHlCharDetect(int attribute, int context,signed char regionId,signed char regionId2, QChar);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual KateHlItem *clone(const QStringList *args);

  private:
    QChar sChar;
};

class KateHl2CharDetect : public KateHlItem
{
  public:
    KateHl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2,  QChar ch1, QChar ch2);
    KateHl2CharDetect(int attribute, int context,signed char regionId,signed char regionId2,  const QChar *ch);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual KateHlItem *clone(const QStringList *args);

  private:
    QChar sChar1;
    QChar sChar2;
};

class KateHlStringDetect : public KateHlItem
{
  public:
    KateHlStringDetect(int attribute, int context, signed char regionId,signed char regionId2, const QString &, bool inSensitive=false);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual KateHlItem *clone(const QStringList *args);

  private:
    const QString str;
    bool _inSensitive;
};

class KateHlRangeDetect : public KateHlItem
{
  public:
    KateHlRangeDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2);

    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    QChar sChar1;
    QChar sChar2;
};

class KateHlKeyword : public KateHlItem
{
  public:
    KateHlKeyword(int attribute, int context,signed char regionId,signed char regionId2, bool casesensitive, const QString& delims);

    virtual void addWord(const QString &);
    virtual void addList(const QStringList &);
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool startEnable(const QChar& c);
    virtual bool alwaysStartEnable() const;
    virtual bool hasCustomStartEnable() const;

  private:
    QDict<bool> dict;
    bool _caseSensitive;
    const QString& deliminators;
    int minLen;
    int maxLen;
};

class KateHlInt : public KateHlItem
{
  public:
    KateHlInt(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class KateHlFloat : public KateHlItem
{
  public:
    KateHlFloat(int attribute, int context, signed char regionId,signed char regionId2);
    virtual ~KateHlFloat () {}

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class KateHlCFloat : public KateHlFloat
{
  public:
    KateHlCFloat(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    int checkIntHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class KateHlCOct : public KateHlItem
{
  public:
    KateHlCOct(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class KateHlCHex : public KateHlItem
{
  public:
    KateHlCHex(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class KateHlLineContinue : public KateHlItem
{
  public:
    KateHlLineContinue(int attribute, int context, signed char regionId,signed char regionId2);

    virtual bool endEnable(QChar c) {return c == '\0';}
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool lineContinue(){return true;}
};

class KateHlCStringChar : public KateHlItem
{
  public:
    KateHlCStringChar(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlCChar : public KateHlItem
{
  public:
    KateHlCChar(int attribute, int context,signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class KateHlAnyChar : public KateHlItem
{
  public:
    KateHlAnyChar(int attribute, int context, signed char regionId,signed char regionId2, const QString& charList);

    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    const QString _charList;
};

class KateHlRegExpr : public KateHlItem
{
  public:
    KateHlRegExpr(int attribute, int context,signed char regionId,signed char regionId2 ,QString expr, bool insensitive, bool minimal);
    ~KateHlRegExpr() { delete Expr; };

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

class KateHlConsumeSpaces : public KateHlItem
{
  public:
    KateHlConsumeSpaces () : KateHlItem (0,0,0,0) { justConsume = true; }

    virtual int checkHgl(const QString& text, int offset, int len)
    {
      while (text[offset].isSpace()) offset++;
      return offset;
    }
};

class KateHlConsumeIdentifier : public KateHlItem
{
  public:
    KateHlConsumeIdentifier () : KateHlItem (0,0,0,0) { justConsume = true; }

    virtual int checkHgl(const QString& text, int offset, int len)
    {
      while (text[offset].isLetterOrNumber() || (text[offset] == QChar ('_'))) offset++;
      return offset;
    }
};

//END

//BEGIN STATICS
KateHlManager *KateHlManager::s_self = 0;

static const bool trueBool = true;
static const QString stdDeliminator = QString (" \t.():!+,-<=>%&*/;?[]^{|}~\\");
//END

//BEGIN NON MEMBER FUNCTIONS
static KateHlItemData::ItemStyles getDefStyleNum(QString name)
{
  if (name=="dsNormal") return KateHlItemData::dsNormal;
  else if (name=="dsKeyword") return KateHlItemData::dsKeyword;
  else if (name=="dsDataType") return KateHlItemData::dsDataType;
  else if (name=="dsDecVal") return KateHlItemData::dsDecVal;
  else if (name=="dsBaseN") return KateHlItemData::dsBaseN;
  else if (name=="dsFloat") return KateHlItemData::dsFloat;
  else if (name=="dsChar") return KateHlItemData::dsChar;
  else if (name=="dsString") return KateHlItemData::dsString;
  else if (name=="dsComment") return KateHlItemData::dsComment;
  else if (name=="dsOthers")  return KateHlItemData::dsOthers;
  else if (name=="dsAlert") return KateHlItemData::dsAlert;
  else if (name=="dsFunction") return KateHlItemData::dsFunction;
  else if (name=="dsRegionMarker") return KateHlItemData::dsRegionMarker;
  else if (name=="dsError") return KateHlItemData::dsError;

  return KateHlItemData::dsNormal;
}
//END

//BEGIN KateHlItem
KateHlItem::KateHlItem(int attribute, int context,signed char regionId,signed char regionId2)
  : attr(attribute),
    ctx(context),
    region(regionId),
    region2(regionId2),
    lookAhead(false),
    dynamic(false),
    dynamicChild(false),
    firstNonSpace(false),
    justConsume(false),
    column (-1)
{
}

KateHlItem::~KateHlItem()
{
  //kdDebug(13010)<<"In hlItem::~KateHlItem()"<<endl;
  for (uint i=0; i < subItems.size(); i++)
    delete subItems[i];
}

bool KateHlItem::startEnable(const QChar& c)
{
  // ONLY called when alwaysStartEnable() overridden
  // IN FACT not called at all, copied into doHighlight()...
  Q_ASSERT(false);
  return kateInsideString (stdDeliminator, c);
}

void KateHlItem::dynamicSubstitute(QString &str, const QStringList *args)
{
  for (uint i = 0; i < str.length() - 1; ++i)
  {
    if (str[i] == '%')
    {
      char c = str[i + 1].latin1();
      if (c == '%')
        str.replace(i, 1, "");
      else if (c >= '0' && c <= '9')
      {
        if ((uint)(c - '0') < args->size())
        {
          str.replace(i, 2, (*args)[c - '0']);
          i += ((*args)[c - '0']).length() - 1;
        }
        else
        {
          str.replace(i, 2, "");
          --i;
        }
      }
    }
  }
}
//END

//BEGIN KateHlCharDetect
KateHlCharDetect::KateHlCharDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar c)
  : KateHlItem(attribute,context,regionId,regionId2)
  , sChar(c)
{
}

int KateHlCharDetect::checkHgl(const QString& text, int offset, int len)
{
  if (len && text[offset] == sChar)
    return offset + 1;

  return 0;
}

KateHlItem *KateHlCharDetect::clone(const QStringList *args)
{
  char c = sChar.latin1();

  if (c < '0' || c > '9' || (unsigned)(c - '0') >= args->size())
    return this;

  KateHlCharDetect *ret = new KateHlCharDetect(attr, ctx, region, region2, (*args)[c - '0'][0]);
  ret->dynamicChild = true;
  return ret;
}
//END

//BEGIN KateHl2CharDetect
KateHl2CharDetect::KateHl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2)
  : KateHlItem(attribute,context,regionId,regionId2)
  , sChar1 (ch1)
  , sChar2 (ch2)
{
}

int KateHl2CharDetect::checkHgl(const QString& text, int offset, int len)
{
  if (len < 2)
    return offset;

  if (text[offset++] == sChar1 && text[offset++] == sChar2)
    return offset;

  return 0;
}

KateHlItem *KateHl2CharDetect::clone(const QStringList *args)
{
  char c1 = sChar1.latin1();
  char c2 = sChar2.latin1();

  if (c1 < '0' || c1 > '9' || (unsigned)(c1 - '0') >= args->size())
    return this;

  if (c2 < '0' || c2 > '9' || (unsigned)(c2 - '0') >= args->size())
    return this;

  KateHl2CharDetect *ret = new KateHl2CharDetect(attr, ctx, region, region2, (*args)[c1 - '0'][0], (*args)[c2 - '0'][0]);
  ret->dynamicChild = true;
  return ret;
}
//END

//BEGIN KateHlStringDetect
KateHlStringDetect::KateHlStringDetect(int attribute, int context, signed char regionId,signed char regionId2,const QString &s, bool inSensitive)
  : KateHlItem(attribute, context,regionId,regionId2)
  , str(inSensitive ? s.upper() : s)
  , _inSensitive(inSensitive)
{
}

int KateHlStringDetect::checkHgl(const QString& text, int offset, int len)
{
  if (len < (int)str.length())
    return 0;

  if (QConstString(text.unicode() + offset, str.length()).string().find(str, 0, !_inSensitive) == 0)
    return offset + str.length();

  return 0;
}

KateHlItem *KateHlStringDetect::clone(const QStringList *args)
{
  QString newstr = str;

  dynamicSubstitute(newstr, args);

  if (newstr == str)
    return this;

  KateHlStringDetect *ret = new KateHlStringDetect(attr, ctx, region, region2, newstr, _inSensitive);
  ret->dynamicChild = true;
  return ret;
}
//END

//BEGIN KateHlRangeDetect
KateHlRangeDetect::KateHlRangeDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2)
  : KateHlItem(attribute,context,regionId,regionId2)
  , sChar1 (ch1)
  , sChar2 (ch2)
{
}

int KateHlRangeDetect::checkHgl(const QString& text, int offset, int len)
{
  if ((len > 0) && (text[offset] == sChar1))
  {
    do
    {
      offset++;
      len--;
      if (len < 1) return 0;
    }
    while (text[offset] != sChar2);

    return offset + 1;
  }
  return 0;
}
//END

//BEGIN KateHlKeyword
KateHlKeyword::KateHlKeyword (int attribute, int context, signed char regionId,signed char regionId2, bool casesensitive, const QString& delims)
  : KateHlItem(attribute,context,regionId,regionId2)
  , dict (113, casesensitive)
  , _caseSensitive(casesensitive)
  , deliminators(delims)
  , minLen (0xFFFFFF)
  , maxLen (-1)
{
}

bool KateHlKeyword::alwaysStartEnable() const
{
  return false;
}

bool KateHlKeyword::hasCustomStartEnable() const
{
  return true;
}

bool KateHlKeyword::startEnable(const QChar& c)
{
  return kateInsideString (deliminators, c);
}

// If we use a dictionary for lookup we don't really need
// an item as such we are using the key to lookup
void KateHlKeyword::addWord(const QString &word)
{
  dict.insert(word,&trueBool);
}

void KateHlKeyword::addList(const QStringList& list)
{
  for(uint i=0;i<list.count();i++)
  {
    dict.insert(list[i], &trueBool);

    int len = list[i].length();

    if (minLen > len)
      minLen = len;

    if (maxLen < len)
      maxLen = len;
  }
}

int KateHlKeyword::checkHgl(const QString& text, int offset, int len)
{
  if (len == 0 || dict.isEmpty()) return 0;

  int offset2 = offset;
  int wordLen = 0;

  while ((len > wordLen) && !kateInsideString (deliminators, text[offset2]))
  {
    offset2++;
    wordLen++;

    if (wordLen > maxLen) return 0;
  }

  if (offset2 == offset) return 0;

  if (wordLen < minLen) return 0;

  if ( dict.find(QConstString(text.unicode() + offset, wordLen).string()) ) return offset2;

  return 0;
}
//END

//BEGIN KateHlInt
KateHlInt::KateHlInt(int attribute, int context, signed char regionId,signed char regionId2)
  : KateHlItem(attribute,context,regionId,regionId2)
{
}

bool KateHlInt::alwaysStartEnable() const
{
  return false;
}

int KateHlInt::checkHgl(const QString& text, int offset, int len)
{
  int offset2 = offset;

  while ((len > 0) && text[offset2].isDigit())
  {
    offset2++;
    len--;
  }

  if (offset2 > offset)
  {
    for (uint i=0; i < subItems.size(); i++)
    {
      if ( (offset = subItems[i]->checkHgl(text, offset2, len)) )
        return offset;
    }

    return offset2;
  }

  return 0;
}
//END

//BEGIN KateHlFloat
KateHlFloat::KateHlFloat(int attribute, int context, signed char regionId,signed char regionId2)
  : KateHlItem(attribute,context, regionId,regionId2)
{
}

bool KateHlFloat::alwaysStartEnable() const
{
  return false;
}

int KateHlFloat::checkHgl(const QString& text, int offset, int len)
{
  bool b = false;
  bool p = false;

  while ((len > 0) && text[offset].isDigit())
  {
    offset++;
    len--;
    b = true;
  }

  if ((len > 0) && (p = (text[offset] == '.')))
  {
    offset++;
    len--;

    while ((len > 0) && text[offset].isDigit())
    {
      offset++;
      len--;
      b = true;
    }
  }

  if (!b)
    return 0;

  if ((len > 0) && ((text[offset] & 0xdf) == 'E'))
  {
    offset++;
    len--;
  }
  else
  {
    if (!p)
      return 0;
    else
    {
      for (uint i=0; i < subItems.size(); i++)
      {
        int offset2 = subItems[i]->checkHgl(text, offset, len);

        if (offset2)
          return offset2;
      }

      return offset;
    }
  }

  if ((len > 0) && (text[offset] == '-' || text[offset] =='+'))
  {
    offset++;
    len--;
  }

  b = false;

  while ((len > 0) && text[offset].isDigit())
  {
    offset++;
    len--;
    b = true;
  }

  if (b)
  {
    for (uint i=0; i < subItems.size(); i++)
    {
      int offset2 = subItems[i]->checkHgl(text, offset, len);

      if (offset2)
        return offset2;
    }

    return offset;
  }

  return 0;
}
//END

//BEGIN KateHlCOct
KateHlCOct::KateHlCOct(int attribute, int context, signed char regionId,signed char regionId2)
  : KateHlItem(attribute,context,regionId,regionId2)
{
}

bool KateHlCOct::alwaysStartEnable() const
{
  return false;
}

int KateHlCOct::checkHgl(const QString& text, int offset, int len)
{
  if ((len > 0) && text[offset] == '0')
  {
    offset++;
    len--;

    int offset2 = offset;

    while ((len > 0) && (text[offset2] >= '0' && text[offset2] <= '7'))
    {
      offset2++;
      len--;
    }

    if (offset2 > offset)
    {
      if ((len > 0) && ((text[offset2] & 0xdf) == 'L' || (text[offset] & 0xdf) == 'U' ))
        offset2++;

      return offset2;
    }
  }

  return 0;
}
//END

//BEGIN KateHlCHex
KateHlCHex::KateHlCHex(int attribute, int context,signed char regionId,signed char regionId2)
  : KateHlItem(attribute,context,regionId,regionId2)
{
}

bool KateHlCHex::alwaysStartEnable() const
{
  return false;
}

int KateHlCHex::checkHgl(const QString& text, int offset, int len)
{
  if ((len > 1) && (text[offset++] == '0') && ((text[offset++] & 0xdf) == 'X' ))
  {
    len -= 2;

    int offset2 = offset;

    while ((len > 0) && (text[offset2].isDigit() || ((text[offset2] & 0xdf) >= 'A' && (text[offset2] & 0xdf) <= 'F')))
    {
      offset2++;
      len--;
    }

    if (offset2 > offset)
    {
      if ((len > 0) && ((text[offset2] & 0xdf) == 'L' || (text[offset2] & 0xdf) == 'U' ))
        offset2++;

      return offset2;
    }
  }

  return 0;
}
//END

//BEGIN KateHlCFloat
KateHlCFloat::KateHlCFloat(int attribute, int context, signed char regionId,signed char regionId2)
  : KateHlFloat(attribute,context,regionId,regionId2)
{
}

bool KateHlCFloat::alwaysStartEnable() const
{
  return false;
}

int KateHlCFloat::checkIntHgl(const QString& text, int offset, int len)
{
  int offset2 = offset;

  while ((len > 0) && text[offset].isDigit()) {
    offset2++;
    len--;
  }

  if (offset2 > offset)
     return offset2;

  return 0;
}

int KateHlCFloat::checkHgl(const QString& text, int offset, int len)
{
  int offset2 = KateHlFloat::checkHgl(text, offset, len);

  if (offset2)
  {
    if ((text[offset2] & 0xdf) == 'F' )
      offset2++;

    return offset2;
  }
  else
  {
    offset2 = checkIntHgl(text, offset, len);

    if (offset2 && ((text[offset2] & 0xdf) == 'F' ))
      return ++offset2;
    else
      return 0;
  }
}
//END

//BEGIN KateHlAnyChar
KateHlAnyChar::KateHlAnyChar(int attribute, int context, signed char regionId,signed char regionId2, const QString& charList)
  : KateHlItem(attribute, context,regionId,regionId2)
  , _charList(charList)
{
}

int KateHlAnyChar::checkHgl(const QString& text, int offset, int len)
{
  if ((len > 0) && kateInsideString (_charList, text[offset]))
    return ++offset;

  return 0;
}
//END

//BEGIN KateHlRegExpr
KateHlRegExpr::KateHlRegExpr( int attribute, int context, signed char regionId,signed char regionId2, QString regexp, bool insensitive, bool minimal)
  : KateHlItem(attribute, context, regionId,regionId2)
  , handlesLinestart (regexp.startsWith("^"))
  , _regexp(regexp)
  , _insensitive(insensitive)
  , _minimal(minimal)
{
  if (!handlesLinestart)
    regexp.prepend("^");

  Expr = new QRegExp(regexp, !_insensitive);
  Expr->setMinimal(_minimal);
}

int KateHlRegExpr::checkHgl(const QString& text, int offset, int /*len*/)
{
  if (offset && handlesLinestart)
    return 0;

  int offset2 = Expr->search( text, offset, QRegExp::CaretAtOffset );

  if (offset2 == -1) return 0;

  return (offset + Expr->matchedLength());
}

QStringList *KateHlRegExpr::capturedTexts()
{
  return new QStringList(Expr->capturedTexts());
}

KateHlItem *KateHlRegExpr::clone(const QStringList *args)
{
  QString regexp = _regexp;
  QStringList escArgs = *args;

  for (QStringList::Iterator it = escArgs.begin(); it != escArgs.end(); ++it)
  {
    (*it).replace(QRegExp("(\\W)"), "\\\\1");
  }

  dynamicSubstitute(regexp, &escArgs);

  if (regexp == _regexp)
    return this;

  // kdDebug (13010) << "clone regexp: " << regexp << endl;

  KateHlRegExpr *ret = new KateHlRegExpr(attr, ctx, region, region2, regexp, _insensitive, _minimal);
  ret->dynamicChild = true;
  return ret;
}
//END

//BEGIN KateHlLineContinue
KateHlLineContinue::KateHlLineContinue(int attribute, int context, signed char regionId,signed char regionId2)
  : KateHlItem(attribute,context,regionId,regionId2) {
}

int KateHlLineContinue::checkHgl(const QString& text, int offset, int len)
{
  if ((len == 1) && (text[offset] == '\\'))
    return ++offset;

  return 0;
}
//END

//BEGIN KateHlCStringChar
KateHlCStringChar::KateHlCStringChar(int attribute, int context,signed char regionId,signed char regionId2)
  : KateHlItem(attribute,context,regionId,regionId2) {
}

// checks for C escaped chars \n and escaped hex/octal chars
static int checkEscapedChar(const QString& text, int offset, int& len)
{
  int i;
  if (text[offset] == '\\' && len > 1)
  {
    offset++;
    len--;

    switch(text[offset])
    {
      case  'a': // checks for control chars
      case  'b': // we want to fall through
      case  'e':
      case  'f':

      case  'n':
      case  'r':
      case  't':
      case  'v':
      case '\'':
      case '\"':
      case '?' : // added ? ANSI C classifies this as an escaped char
      case '\\':
        offset++;
        len--;
        break;

      case 'x': // if it's like \xff
        offset++; // eat the x
        len--;
        // these for loops can probably be
        // replaced with something else but
        // for right now they work
        // check for hexdigits
        for (i = 0; (len > 0) && (i < 2) && (text[offset] >= '0' && text[offset] <= '9' || (text[offset] & 0xdf) >= 'A' && (text[offset] & 0xdf) <= 'F'); i++)
        {
          offset++;
          len--;
        }

        if (i == 0)
          return 0; // takes care of case '\x'

        break;

      case '0': case '1': case '2': case '3' :
      case '4': case '5': case '6': case '7' :
        for (i = 0; (len > 0) && (i < 3) && (text[offset] >='0'&& text[offset] <='7'); i++)
        {
          offset++;
          len--;
        }
        break;

      default:
        return 0;
    }

    return offset;
  }

  return 0;
}

int KateHlCStringChar::checkHgl(const QString& text, int offset, int len)
{
  return checkEscapedChar(text, offset, len);
}
//END

//BEGIN KateHlCChar
KateHlCChar::KateHlCChar(int attribute, int context,signed char regionId,signed char regionId2)
  : KateHlItem(attribute,context,regionId,regionId2) {
}

int KateHlCChar::checkHgl(const QString& text, int offset, int len)
{
  if ((len > 1) && (text[offset] == '\'') && (text[offset+1] != '\''))
  {
    int oldl;
    oldl = len;

    len--;

    int offset2 = checkEscapedChar(text, offset + 1, len);

    if (!offset2)
    {
      if (oldl > 2)
      {
        offset2 = offset + 2;
        len = oldl - 2;
      }
      else
      {
        return 0;
      }
    }

    if ((len > 0) && (text[offset2] == '\''))
      return ++offset2;
  }

  return 0;
}
//END

//BEGIN KateHl2CharDetect
KateHl2CharDetect::KateHl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2, const QChar *s)
  : KateHlItem(attribute,context,regionId,regionId2) {
  sChar1 = s[0];
  sChar2 = s[1];
  }
//END KateHl2CharDetect

KateHlItemData::KateHlItemData(const QString  name, int defStyleNum)
  : name(name), defStyleNum(defStyleNum) {
}

KateHlData::KateHlData(const QString &wildcards, const QString &mimetypes, const QString &identifier, int priority)
  : wildcards(wildcards), mimetypes(mimetypes), identifier(identifier), priority(priority)
{
}

//BEGIN KateHlContext
KateHlContext::KateHlContext (int attribute, int lineEndContext, int _lineBeginContext, bool _fallthrough, int _fallthroughContext, bool _dynamic)
{
  attr = attribute;
  ctx = lineEndContext;
  lineBeginContext = _lineBeginContext;
  fallthrough = _fallthrough;
  ftctx = _fallthroughContext;
  dynamic = _dynamic;
  dynamicChild = false;
}

KateHlContext *KateHlContext::clone(const QStringList *args)
{
  KateHlContext *ret = new KateHlContext(attr, ctx, lineBeginContext, fallthrough, ftctx, false);

  for (uint n=0; n < items.size(); ++n)
  {
    KateHlItem *item = items[n];
    KateHlItem *i = (item->dynamic ? item->clone(args) : item);
    ret->items.append(i);
  }

  ret->dynamicChild = true;

  return ret;
}

KateHlContext::~KateHlContext()
{
  if (dynamicChild)
  {
    for (uint n=0; n < items.size(); ++n)
    {
      if (items[n]->dynamicChild)
        delete items[n];
    }
  }
}
//END

//BEGIN KateHighlighting
KateHighlighting::KateHighlighting(const KateSyntaxModeListItem *def) : refCount(0)
{
  m_attributeArrays.setAutoDelete (true);

  errorsAndWarnings = "";
  building=false;
  noHl = false;
  m_foldingIndentationSensitive = false;
  folding=false;
  internalIDList.setAutoDelete(true);

  if (def == 0)
  {
    noHl = true;
    iName = "None"; // not translated internal name (for config and more)
    iNameTranslated = i18n("None"); // user visible name
    iSection = "";
    m_priority = 0;
    iHidden = false;
  }
  else
  {
    iName = def->name;
    iNameTranslated = def->nameTranslated;
    iSection = def->section;
    iHidden = def->hidden;
    iWildcards = def->extension;
    iMimetypes = def->mimetype;
    identifier = def->identifier;
    iVersion=def->version;
    iAuthor=def->author;
    iLicense=def->license;
    m_priority=def->priority.toInt();
  }

  deliminator = stdDeliminator;
}

KateHighlighting::~KateHighlighting()
{
  contextList.setAutoDelete( true );
}

void KateHighlighting::generateContextStack(int *ctxNum, int ctx, QMemArray<short>* ctxs, int *prevLine, bool lineContinue)
{
  //kdDebug(13010)<<QString("Entering generateContextStack with %1").arg(ctx)<<endl;
  while (true)
  {
    if (lineContinue)
    {
      if ( !ctxs->isEmpty() )
      {
        (*ctxNum)=(*ctxs)[ctxs->size()-1];
        (*prevLine)--;
      }
      else
      {
        //kdDebug(13010)<<QString("generateContextStack: line continue: len ==0");
        (*ctxNum)=0;
      }

      return;
    }

    if (ctx >= 0)
    {
      (*ctxNum) = ctx;

      ctxs->resize (ctxs->size()+1, QGArray::SpeedOptim);
      (*ctxs)[ctxs->size()-1]=(*ctxNum);
    }
    else
    {
      if (ctx < -1)
      {
        while (ctx < -1)
        {
          if ( ctxs->isEmpty() )
            (*ctxNum)=0;
          else
          {
            ctxs->resize (ctxs->size()-1, QGArray::SpeedOptim);
            //kdDebug(13010)<<QString("generate context stack: truncated stack to :%1").arg(ctxs->size())<<endl;
            (*ctxNum) = ( (ctxs->isEmpty() ) ? 0 : (*ctxs)[ctxs->size()-1]);
          }

          ctx++;
        }

        ctx = 0;

        if ((*prevLine) >= (int)(ctxs->size()-1))
        {
          *prevLine=ctxs->size()-1;

          if ( ctxs->isEmpty() )
            return;

          if (contextNum((*ctxs)[ctxs->size()-1]) && (contextNum((*ctxs)[ctxs->size()-1])->ctx != -1))
          {
            //kdDebug(13010)<<"PrevLine > size()-1 and ctx!=-1)"<<endl;
            ctx = contextNum((*ctxs)[ctxs->size()-1])->ctx;
            lineContinue = false;

            continue;
          }
        }
      }
      else
      {
        if (ctx == -1)
          (*ctxNum)=( (ctxs->isEmpty() ) ? 0 : (*ctxs)[ctxs->size()-1]);
      }
    }

    return;
  }
}

/**
 * Creates a new dynamic context or reuse an old one if it has already been created.
 */
int KateHighlighting::makeDynamicContext(KateHlContext *model, const QStringList *args)
{
  QPair<KateHlContext *, QString> key(model, args->front());
  short value;

  if (dynamicCtxs.contains(key))
    value = dynamicCtxs[key];
  else
  {
    ++startctx;
    KateHlContext *newctx = model->clone(args);
    contextList.insert(startctx, newctx);
    value = startctx;
    dynamicCtxs[key] = value;
    KateHlManager::self()->incDynamicCtxs();
  }

  // kdDebug(13010) << "Dynamic context: using context #" << value << " (for model " << model << " with args " << *args << ")" << endl;

  return value;
}

/**
 * Drop all dynamic contexts. Shall be called with extreme care, and shall be immediatly
 * followed by a full HL invalidation.
 */
void KateHighlighting::dropDynamicContexts()
{
  QMap< QPair<KateHlContext *, QString>, short>::Iterator it;
  for (it = dynamicCtxs.begin(); it != dynamicCtxs.end(); ++it)
  {
    if (contextList[it.data()] != 0 && contextList[it.data()]->dynamicChild)
    {
      KateHlContext *todelete = contextList.take(it.data());
      delete todelete;
    }
  }

  dynamicCtxs.clear();
  startctx = base_startctx;
}

/**
 * Parse the text and fill in the context array and folding list array
 *
 * @param prevLine The previous line, the context array is picked up from that if present.
 * @param textLine The text line to parse
 * @param foldinglist will be filled
 * @param ctxChanged will be set to reflect if the context changed
 */
void KateHighlighting::doHighlight ( KateTextLine *prevLine,
                                     KateTextLine *textLine,
                                     QMemArray<uint>* foldingList,
                                     bool *ctxChanged )
{
  if (!textLine)
    return;

  if (noHl)
  {
    textLine->setAttribs(0,0,textLine->length());
    return;
  }

//  kdDebug(13010)<<QString("The context stack length is: %1").arg(oCtx.size())<<endl;
  // if (lineContinue) kdDebug(13010)<<"Entering with lineContinue flag set"<<endl;

  // duplicate the ctx stack, only once !
  QMemArray<short> ctx;
  ctx.duplicate (prevLine->ctxArray());

  // line continue flag !
  bool lineContinue = prevLine->hlLineContinue();

  int ctxNum = 0;
  int previousLine = -1;
  KateHlContext *context;

  if ( prevLine->ctxArray().isEmpty() )
  {
    // If the stack is empty, we assume to be in Context 0 (Normal)
    context=contextNum(ctxNum);
  }
  else
  {
    // There does an old context stack exist -> find the context at the line start
    ctxNum=ctx[prevLine->ctxArray().size()-1]; //context ID of the last character in the previous line

    //kdDebug(13010) << "\t\tctxNum = " << ctxNum << " contextList[ctxNum] = " << contextList[ctxNum] << endl; // ellis

    //if (lineContinue)   kdDebug(13010)<<QString("The old context should be %1").arg((int)ctxNum)<<endl;

    if (!(context = contextNum(ctxNum)))
      context = contextNum(0);

    //kdDebug(13010)<<"test1-2-1-text2"<<endl;

    previousLine=prevLine->ctxArray().size()-1; //position of the last context ID of th previous line within the stack

    //kdDebug(13010)<<"test1-2-1-text3"<<endl;
    generateContextStack(&ctxNum, context->ctx, &ctx, &previousLine, lineContinue); //get stack ID to use

    //kdDebug(13010)<<"test1-2-1-text4"<<endl;

    if (!(context = contextNum(ctxNum)))
      context = contextNum(0);

    //if (lineContinue)   kdDebug(13010)<<QString("The new context is %1").arg((int)ctxNum)<<endl;
  }

  // text, for programming convenience :)
  QChar lastChar = ' ';
  const QString& text = textLine->string();
  int len = textLine->length();

  // calc at which char the first char occurs, set it to lenght of line if never
  int firstChar = textLine->firstChar();
  int startNonSpace = (firstChar == -1) ? len : firstChar;

  int offset1 = 0;
  int z = 0;
  KateHlItem *item = 0;

  while (z < len)
  {
    bool found = false;
    bool standardStartEnableDetermined = false;
    bool standardStartEnable = false;

    uint index = 0;
    for (item = context->items.empty() ? 0 : context->items[0]; item; item = (++index < context->items.size()) ? context->items[index] : 0 )
    {
      // does we only match if we are firstNonSpace?
      if (item->firstNonSpace && (z > startNonSpace))
        continue;

      // have we a column specified? if yes, only match at this column
      if ((item->column != -1) && (item->column != z))
        continue;

      // do we only consume stuff?
      if (item->justConsume)
      {
        int offset2 = item->checkHgl(text, offset1, len-z);

        if (offset2 <= offset1)
          continue;

        // set attribute of this context
        textLine->setAttribs(context->attr,offset1,offset2);

        z = z + offset2 - offset1 - 1;
        offset1 = offset2 - 1;

        found = true;
        break;
      }
      else
      {
        bool thisStartEnabled = false;

        if (item->alwaysStartEnable())
        {
          thisStartEnabled = true;
        }
        else if (!item->hasCustomStartEnable())
        {
          if (!standardStartEnableDetermined)
          {
            standardStartEnable = kateInsideString (stdDeliminator, lastChar);
            standardStartEnableDetermined = true;
          }

          thisStartEnabled = standardStartEnable;
        }
        else if (item->startEnable(lastChar))
        {
          thisStartEnabled = true;
        }

        if (!thisStartEnabled)
          continue;

        int offset2 = item->checkHgl(text, offset1, len-z);

        if (offset2 <= offset1)
          continue;

        if(!item->lookAhead)
          textLine->setAttribs(item->attr,offset1,offset2);

          //kdDebug(13010)<<QString("item->ctx: %1").arg(item->ctx)<<endl;

        if (item->region2)
        {
          // kdDebug(13010)<<QString("Region mark 2 detected: %1").arg(item->region2)<<endl;
          if ( !foldingList->isEmpty() && ((item->region2 < 0) && (*foldingList)[foldingList->size()-2] == -item->region2 ) )
          {
            foldingList->resize (foldingList->size()-2, QGArray::SpeedOptim);
          }
          else
          {
            foldingList->resize (foldingList->size()+2, QGArray::SpeedOptim);
            (*foldingList)[foldingList->size()-2] = (uint)item->region2;
            if (item->region2<0) //check not really needed yet
              (*foldingList)[foldingList->size()-1] = offset2;
            else
            (*foldingList)[foldingList->size()-1] = offset1;
          }

        }

        if (item->region)
        {
          // kdDebug(13010)<<QString("Region mark detected: %1").arg(item->region)<<endl;

        /* if ( !foldingList->isEmpty() && ((item->region < 0) && (*foldingList)[foldingList->size()-1] == -item->region ) )
          {
            foldingList->resize (foldingList->size()-1, QGArray::SpeedOptim);
          }
          else*/
          {
            foldingList->resize (foldingList->size()+2, QGArray::SpeedOptim);
            (*foldingList)[foldingList->size()-2] = item->region;
            if (item->region<0) //check not really needed yet
              (*foldingList)[foldingList->size()-1] = offset2;
            else
              (*foldingList)[foldingList->size()-1] = offset1;
          }

        }

        generateContextStack(&ctxNum, item->ctx, &ctx, &previousLine);  //regenerate context stack

    //kdDebug(13010)<<QString("generateContextStack has been left in item loop, size: %1").arg(ctx.size())<<endl;
  //    kdDebug(13010)<<QString("current ctxNum==%1").arg(ctxNum)<<endl;

        context=contextNum(ctxNum);

        // dynamic context: substitute the model with an 'instance'
        if (context->dynamic)
        {
          QStringList *lst = item->capturedTexts();
          if (lst != 0)
          {
            // Replace the top of the stack and the current context
            int newctx = makeDynamicContext(context, lst);
            if (ctx.size() > 0)
              ctx[ctx.size() - 1] = newctx;
            ctxNum = newctx;
            context = contextNum(ctxNum);
          }
          delete lst;
        }

        // dominik: look ahead w/o changing offset?
        if (!item->lookAhead)
        {
          z = z + offset2 - offset1 - 1;
          offset1 = offset2 - 1;
        }

        found = true;
        break;
      }
    }

    if (!found)
    {

      // nothing found: set attribute of one char
      // anders: unless this context does not want that!
      if ( context->fallthrough )
      {
      // set context to context->ftctx.
        generateContextStack(&ctxNum, context->ftctx, &ctx, &previousLine);  //regenerate context stack
        context=contextNum(ctxNum);
      //kdDebug(13010)<<"context num after fallthrough at col "<<z<<": "<<ctxNum<<endl;
      // the next is nessecary, as otherwise keyword (or anything using the std delimitor check)
      // immediately after fallthrough fails. Is it bad?
      // jowenn, can you come up with a nicer way to do this?
        if (z)
          lastChar = text[offset1 - 1];
        else
          lastChar = '\\';
        continue;
      }
      else
        textLine->setAttribs(context->attr,offset1,offset1 + 1);
    }

    // dominik: do not change offset if we look ahead
    if (!(item && item->lookAhead))
    {
      lastChar = text[offset1];
      offset1++;
      z++;
    }
  }

  // has the context stack changed ?
  if (ctx == textLine->ctxArray())
  {
    if (ctxChanged)
      (*ctxChanged) = false;
  }
  else
  {
    if (ctxChanged)
      (*ctxChanged) = true;

    // assign ctx stack !
    textLine->setContext(ctx);
  }

  // write hl continue flag
  textLine->setHlLineContinue (item && item->lineContinue());
}

void KateHighlighting::loadWildcards()
{
  KConfig *config = KateHlManager::self()->getKConfig();
  config->setGroup("Highlighting " + iName);

  QString extensionString = config->readEntry("Wildcards", iWildcards);

  if (extensionSource != extensionString) {
    regexpExtensions.clear();
    plainExtensions.clear();

    extensionSource = extensionString;

    static QRegExp sep("\\s*;\\s*");

    QStringList l = QStringList::split( sep, extensionSource );

    static QRegExp boringExpression("\\*\\.[\\d\\w]+");

    for( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
      if (boringExpression.exactMatch(*it))
        plainExtensions.append((*it).mid(1));
      else
        regexpExtensions.append(QRegExp((*it), true, true));
  }
}

QValueList<QRegExp>& KateHighlighting::getRegexpExtensions()
{
  return regexpExtensions;
}

QStringList& KateHighlighting::getPlainExtensions()
{
  return plainExtensions;
}

QString KateHighlighting::getMimetypes()
{
  KConfig *config = KateHlManager::self()->getKConfig();
  config->setGroup("Highlighting " + iName);

  return config->readEntry("Mimetypes", iMimetypes);
}

int KateHighlighting::priority()
{
  KConfig *config = KateHlManager::self()->getKConfig();
  config->setGroup("Highlighting " + iName);

  return config->readNumEntry("Priority", m_priority);
}

KateHlData *KateHighlighting::getData()
{
  KConfig *config = KateHlManager::self()->getKConfig();
  config->setGroup("Highlighting " + iName);

  KateHlData *hlData = new KateHlData(
  config->readEntry("Wildcards", iWildcards),
  config->readEntry("Mimetypes", iMimetypes),
  config->readEntry("Identifier", identifier),
  config->readNumEntry("Priority", m_priority));

 return hlData;
}

void KateHighlighting::setData(KateHlData *hlData)
{
  KConfig *config = KateHlManager::self()->getKConfig();
  config->setGroup("Highlighting " + iName);

  config->writeEntry("Wildcards",hlData->wildcards);
  config->writeEntry("Mimetypes",hlData->mimetypes);
  config->writeEntry("Priority",hlData->priority);
}

void KateHighlighting::getKateHlItemDataList (uint schema, KateHlItemDataList &list)
{
  KConfig *config = KateHlManager::self()->getKConfig();
  config->setGroup("Highlighting " + iName + " - Schema " + KateFactory::self()->schemaManager()->name(schema));

  list.clear();
  createKateHlItemData(list);

  for (KateHlItemData *p = list.first(); p != 0L; p = list.next())
  {
    QStringList s = config->readListEntry(p->name);

//    kdDebug(13010)<<p->name<<s.count()<<endl;
    if (s.count()>0)
    {

      while(s.count()<9) s<<"";
      p->clear();

      QString tmp=s[0]; if (!tmp.isEmpty()) p->defStyleNum=tmp.toInt();

      QRgb col;

      tmp=s[1]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); p->setTextColor(col); }

      tmp=s[2]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); p->setSelectedTextColor(col); }

      tmp=s[3]; if (!tmp.isEmpty()) p->setBold(tmp!="0");

      tmp=s[4]; if (!tmp.isEmpty()) p->setItalic(tmp!="0");

      tmp=s[5]; if (!tmp.isEmpty()) p->setStrikeOut(tmp!="0");

      tmp=s[6]; if (!tmp.isEmpty()) p->setUnderline(tmp!="0");

      tmp=s[7]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); p->setBGColor(col); }

      tmp=s[8]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); p->setSelectedBGColor(col); }

    }
  }
}

/**
 * Saves the KateHlData attribute definitions to the config file.
 *
 * @param schema The id of the schema group to save
 * @param list KateHlItemDataList containing the data to be used
 */
void KateHighlighting::setKateHlItemDataList(uint schema, KateHlItemDataList &list)
{
  KConfig *config = KateHlManager::self()->getKConfig();
  config->setGroup("Highlighting " + iName + " - Schema " + KateFactory::self()->schemaManager()->name(schema));

  QStringList settings;

  for (KateHlItemData *p = list.first(); p != 0L; p = list.next())
  {
    settings.clear();
    settings<<QString::number(p->defStyleNum,10);
    settings<<(p->itemSet(KateAttribute::TextColor)?QString::number(p->textColor().rgb(),16):"");
    settings<<(p->itemSet(KateAttribute::SelectedTextColor)?QString::number(p->selectedTextColor().rgb(),16):"");
    settings<<(p->itemSet(KateAttribute::Weight)?(p->bold()?"1":"0"):"");
    settings<<(p->itemSet(KateAttribute::Italic)?(p->italic()?"1":"0"):"");
    settings<<(p->itemSet(KateAttribute::StrikeOut)?(p->strikeOut()?"1":"0"):"");
    settings<<(p->itemSet(KateAttribute::Underline)?(p->underline()?"1":"0"):"");
    settings<<(p->itemSet(KateAttribute::BGColor)?QString::number(p->bgColor().rgb(),16):"");
    settings<<(p->itemSet(KateAttribute::SelectedBGColor)?QString::number(p->selectedBGColor().rgb(),16):"");
    settings<<"---";
    config->writeEntry(p->name,settings);
  }
}

/**
 * Increase the usage count, and trigger initialization if needed.
 */
void KateHighlighting::use()
{
  if (refCount == 0)
    init();

  refCount++;
}

/**
 * Decrease the usage count, and trigger cleanup if needed.
 */
void KateHighlighting::release()
{
  refCount--;

  if (refCount == 0)
    done();
}

/**
 * Initialize a context for the first time.
 */

void KateHighlighting::init()
{
  if (noHl)
    return;

  contextList.clear ();
  makeContextList();
}


/**
 * If the there is no document using the highlighting style free the complete
 * context structure.
 */
void KateHighlighting::done()
{
  if (noHl)
    return;

  contextList.clear ();
  internalIDList.clear();
}

/**
 * KateHighlighting - createKateHlItemData
 * This function reads the itemData entries from the config file, which specifies the
 * default attribute styles for matched items/contexts.
 *
 * @param list A reference to the internal list containing the parsed default config
 */
void KateHighlighting::createKateHlItemData(KateHlItemDataList &list)
{
  // If no highlighting is selected we need only one default.
  if (noHl)
  {
    list.append(new KateHlItemData(i18n("Normal Text"), KateHlItemData::dsNormal));
    return;
  }

  // If the internal list isn't already available read the config file
  if (internalIDList.isEmpty())
    makeContextList();

  list=internalIDList;
}

/**
 * Adds the styles of the currently parsed highlight to the itemdata list
 */
void KateHighlighting::addToKateHlItemDataList()
{
  //Tell the syntax document class which file we want to parse and which data group
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getGroupInfo("highlighting","itemData");

  //begin with the real parsing
  while (KateHlManager::self()->syntax->nextGroup(data))
  {
    // read all attributes
    QString color = KateHlManager::self()->syntax->groupData(data,QString("color"));
    QString selColor = KateHlManager::self()->syntax->groupData(data,QString("selColor"));
    QString bold = KateHlManager::self()->syntax->groupData(data,QString("bold"));
    QString italic = KateHlManager::self()->syntax->groupData(data,QString("italic"));
    QString underline = KateHlManager::self()->syntax->groupData(data,QString("underline"));
    QString strikeOut = KateHlManager::self()->syntax->groupData(data,QString("strikeOut"));
    QString bgColor = KateHlManager::self()->syntax->groupData(data,QString("backgroundColor"));
    QString selBgColor = KateHlManager::self()->syntax->groupData(data,QString("selBackgroundColor"));

    KateHlItemData* newData = new KateHlItemData(
            buildPrefix+KateHlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace(),
            getDefStyleNum(KateHlManager::self()->syntax->groupData(data,QString("defStyleNum"))));

    /* here the custom style overrides are specified, if needed */
    if (!color.isEmpty()) newData->setTextColor(QColor(color));
    if (!selColor.isEmpty()) newData->setSelectedTextColor(QColor(selColor));
    if (!bold.isEmpty()) newData->setBold( IS_TRUE(bold) );
    if (!italic.isEmpty()) newData->setItalic( IS_TRUE(italic) );
    // new attributes for the new rendering view
    if (!underline.isEmpty()) newData->setUnderline( IS_TRUE(underline) );
    if (!strikeOut.isEmpty()) newData->setStrikeOut( IS_TRUE(strikeOut) );
    if (!bgColor.isEmpty()) newData->setBGColor(QColor(bgColor));
    if (!selBgColor.isEmpty()) newData->setSelectedBGColor(QColor(selBgColor));

    internalIDList.append(newData);
  }

  //clean up
  if (data)
    KateHlManager::self()->syntax->freeGroupInfo(data);
}

/**
 * KateHighlighting - lookupAttrName
 * This function is  a helper for makeContextList and createKateHlItem. It looks the given
 * attribute name in the itemData list up and returns it's index
 *
 * @param name the attribute name to lookup
 * @param iDl the list containing all available attributes
 *
 * @return The index of the attribute, or 0 if the attribute isn't found
 */
int  KateHighlighting::lookupAttrName(const QString& name, KateHlItemDataList &iDl)
{
  for (uint i = 0; i < iDl.count(); i++)
    if (iDl.at(i)->name == buildPrefix+name)
      return i;

  kdDebug(13010)<<"Couldn't resolve itemDataName:"<<name<<endl;
  return 0;
}

/**
 * KateHighlighting - createKateHlItem
 * This function is  a helper for makeContextList. It parses the xml file for
 * information.
 *
 * @param data Data about the item read from the xml file
 * @param iDl List of all available itemData entries.
 *            Needed for attribute name->index translation
 * @param RegionList list of code folding region names
 * @param ContextNameList list of context names
 *
 * @return A pointer to the newly created item object
 */
KateHlItem *KateHighlighting::createKateHlItem(KateSyntaxContextData *data, KateHlItemDataList &iDl,QStringList *RegionList, QStringList *ContextNameList)
{
  // No highlighting -> exit
  if (noHl)
    return 0;

  // get the (tagname) itemd type
  QString dataname=KateHlManager::self()->syntax->groupItemData(data,QString(""));

  // BEGIN - Translation of the attribute parameter
  QString tmpAttr=KateHlManager::self()->syntax->groupItemData(data,QString("attribute")).simplifyWhiteSpace();
  int attr;
  if (QString("%1").arg(tmpAttr.toInt())==tmpAttr)
  {
    errorsAndWarnings+=i18n("<B>%1</B>: Deprecated syntax. Attribute (%2) not addressed by symbolic name<BR>").
    arg(buildIdentifier).arg(tmpAttr);
    attr=tmpAttr.toInt();
  }
  else
    attr=lookupAttrName(tmpAttr,iDl);
  // END - Translation of the attribute parameter

  // Info about context switch
  int context;
  QString tmpcontext=KateHlManager::self()->syntax->groupItemData(data,QString("context"));

  QString unresolvedContext;
  context=getIdFromString(ContextNameList, tmpcontext,unresolvedContext);

  // Get the char parameter (eg DetectChar)
  char chr;
  if (! KateHlManager::self()->syntax->groupItemData(data,QString("char")).isEmpty())
    chr= (KateHlManager::self()->syntax->groupItemData(data,QString("char")).latin1())[0];
  else
    chr=0;

  // Get the String parameter (eg. StringDetect)
  QString stringdata=KateHlManager::self()->syntax->groupItemData(data,QString("String"));

  // Get a second char parameter (char1) (eg Detect2Chars)
  char chr1;
  if (! KateHlManager::self()->syntax->groupItemData(data,QString("char1")).isEmpty())
    chr1= (KateHlManager::self()->syntax->groupItemData(data,QString("char1")).latin1())[0];
  else
    chr1=0;

  // Will be removed eventuall. Atm used for StringDetect and RegExp
  bool insensitive = IS_TRUE( KateHlManager::self()->syntax->groupItemData(data,QString("insensitive")) );

  // for regexp only
  bool minimal = IS_TRUE( KateHlManager::self()->syntax->groupItemData(data,QString("minimal")) );

  // dominik: look ahead and do not change offset. so we can change contexts w/o changing offset1.
  bool lookAhead = IS_TRUE( KateHlManager::self()->syntax->groupItemData(data,QString("lookAhead")) );

  bool dynamic= IS_TRUE(KateHlManager::self()->syntax->groupItemData(data,QString("dynamic")) );

  bool firstNonSpace = IS_TRUE(KateHlManager::self()->syntax->groupItemData(data,QString("firstNonSpace")) );

  int column = -1;
  QString colStr = KateHlManager::self()->syntax->groupItemData(data,QString("column"));
  if (!colStr.isEmpty())
    column = colStr.toInt();

  // code folding region handling:
  QString beginRegionStr=KateHlManager::self()->syntax->groupItemData(data,QString("beginRegion"));
  QString endRegionStr=KateHlManager::self()->syntax->groupItemData(data,QString("endRegion"));

  signed char regionId=0;
  signed char regionId2=0;

  if (!beginRegionStr.isEmpty())
  {
    regionId = RegionList->findIndex(beginRegionStr);

    if (regionId==-1) // if the region name doesn't already exist, add it to the list
    {
      (*RegionList)<<beginRegionStr;
      regionId = RegionList->findIndex(beginRegionStr);
    }

    regionId++;

    kdDebug () << "########### BEG REG: "  << beginRegionStr << " NUM: " << regionId << endl;
  }

  if (!endRegionStr.isEmpty())
  {
    regionId2 = RegionList->findIndex(endRegionStr);

    if (regionId2==-1) // if the region name doesn't already exist, add it to the list
    {
      (*RegionList)<<endRegionStr;
      regionId2 = RegionList->findIndex(endRegionStr);
    }

    regionId2 = -regionId2 - 1;

    kdDebug () << "########### END REG: "  << endRegionStr << " NUM: " << regionId2 << endl;
  }

  //Create the item corresponding to it's type and set it's parameters
  KateHlItem *tmpItem;

  if (dataname=="keyword")
  {
    KateHlKeyword *keyword=new KateHlKeyword(attr,context,regionId,regionId2,casesensitive,
      deliminator);

    //Get the entries for the keyword lookup list
    keyword->addList(KateHlManager::self()->syntax->finddata("highlighting",stringdata));
    tmpItem=keyword;
  }
  else if (dataname=="Float") tmpItem= (new KateHlFloat(attr,context,regionId,regionId2));
  else if (dataname=="Int") tmpItem=(new KateHlInt(attr,context,regionId,regionId2));
  else if (dataname=="DetectChar") tmpItem=(new KateHlCharDetect(attr,context,regionId,regionId2,chr));
  else if (dataname=="Detect2Chars") tmpItem=(new KateHl2CharDetect(attr,context,regionId,regionId2,chr,chr1));
  else if (dataname=="RangeDetect") tmpItem=(new KateHlRangeDetect(attr,context,regionId,regionId2, chr, chr1));
  else if (dataname=="LineContinue") tmpItem=(new KateHlLineContinue(attr,context,regionId,regionId2));
  else if (dataname=="StringDetect") tmpItem=(new KateHlStringDetect(attr,context,regionId,regionId2,stringdata,insensitive));
  else if (dataname=="AnyChar") tmpItem=(new KateHlAnyChar(attr,context,regionId,regionId2,stringdata));
  else if (dataname=="RegExpr") tmpItem=(new KateHlRegExpr(attr,context,regionId,regionId2,stringdata, insensitive, minimal));
  else if (dataname=="HlCChar") tmpItem= ( new KateHlCChar(attr,context,regionId,regionId2));
  else if (dataname=="HlCHex") tmpItem= (new KateHlCHex(attr,context,regionId,regionId2));
  else if (dataname=="HlCOct") tmpItem= (new KateHlCOct(attr,context,regionId,regionId2));
  else if (dataname=="HlCFloat") tmpItem= (new KateHlCFloat(attr,context,regionId,regionId2));
  else if (dataname=="HlCStringChar") tmpItem= (new KateHlCStringChar(attr,context,regionId,regionId2));
  else if (dataname=="ConsumeSpaces") tmpItem= (new KateHlConsumeSpaces());
  else if (dataname=="ConsumeIdentifier") tmpItem= (new KateHlConsumeIdentifier());
  else
  {
    // oops, unknown type. Perhaps a spelling error in the xml file
    return 0;
  }

  // set lookAhead & dynamic properties
  tmpItem->lookAhead = lookAhead;
  tmpItem->dynamic = dynamic;
  tmpItem->firstNonSpace = firstNonSpace;
  tmpItem->column = column;

  if (!unresolvedContext.isEmpty())
  {
    unresolvedContextReferences.insert(&(tmpItem->ctx),unresolvedContext);
  }
  return tmpItem;
}

int KateHighlighting::hlKeyForAttrib( int attrib ) const
{
  int k = 0;
  IntList::const_iterator it = m_hlIndex.constEnd();
  while ( it != m_hlIndex.constBegin() )
  {
    --it;
    k = (*it);
    if ( attrib >= k )
      break;
  }
  return k;
}

bool KateHighlighting::isInWord( QChar c, int attrib ) const
{
  static const QString& sq = KGlobal::staticQString(" \"'");
  return getCommentString(4, attrib).find(c) < 0 && sq.find(c) < 0;
}

bool KateHighlighting::canBreakAt( QChar c, int attrib ) const
{
  static const QString& sq = KGlobal::staticQString("\"'");
  return (getCommentString(5, attrib).find(c) != -1) && (sq.find(c) == -1);
}

signed char KateHighlighting::commentRegion(int attr) const {
  int k = hlKeyForAttrib( attr );
  QString commentRegion=m_additionalData[k][MultiLineRegion];
  return (commentRegion.isEmpty()?0:(commentRegion.toShort()));
}

bool KateHighlighting::canComment( int startAttrib, int endAttrib ) const
{
  int k = hlKeyForAttrib( startAttrib );
  return ( k == hlKeyForAttrib( endAttrib ) &&
      ( ( !m_additionalData[k][Start].isEmpty() && !m_additionalData[k][End].isEmpty() ) ||
       ! m_additionalData[k][SingleLine].isEmpty() ) );
}

QString KateHighlighting::getCommentString( int which, int attrib ) const
{
  if ( noHl )
    return which == 4 ? stdDeliminator : "";

  int k = hlKeyForAttrib( attrib );
  const QStringList& lst = m_additionalData[k];
  return lst.isEmpty() ? QString::null : lst[which];
}

QString KateHighlighting::getCommentStart( int attrib ) const
{
  return getCommentString( Start, attrib );
}

QString KateHighlighting::getCommentEnd( int attrib ) const
{
  return getCommentString( End, attrib );
}

QString KateHighlighting::getCommentSingleLineStart( int attrib ) const
{
  return getCommentString( SingleLine, attrib );
}

/**
 * Helper for makeContextList. It parses the xml file for
 * information, how single or multi line comments are marked
 *
 * @return a stringlist containing the comment marker strings in the order
 * multilineCommentStart, multilineCommentEnd, singlelineCommentMarker
 */
QStringList KateHighlighting::readCommentConfig()
{
  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data=KateHlManager::self()->syntax->getGroupInfo("general","comment");

  QString cmlStart, cmlEnd, cmlRegion, cslStart  ;

  if (data)
  {
    while  (KateHlManager::self()->syntax->nextGroup(data))
    {
      if (KateHlManager::self()->syntax->groupData(data,"name")=="singleLine")
        cslStart=KateHlManager::self()->syntax->groupData(data,"start");

      if (KateHlManager::self()->syntax->groupData(data,"name")=="multiLine")
      {
        cmlStart=KateHlManager::self()->syntax->groupData(data,"start");
        cmlEnd=KateHlManager::self()->syntax->groupData(data,"end");
        cmlRegion=KateHlManager::self()->syntax->groupData(data,"region");
      }
    }

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }
  else
  {
    cslStart = "";
    cmlStart = "";
    cmlEnd = "";
    cmlRegion = "";
  }
  QStringList res;
  res << cmlStart << cmlEnd <<cmlRegion<< cslStart;
  return res;
}

/**
 * Helper for makeContextList. It parses the xml file for information,
 * if keywords should be treated case(in)sensitive and creates the keyword
 * delimiter list. Which is the default list, without any given weak deliminiators
 *
 * @return the computed delimiter string.
 */
QString KateHighlighting::readGlobalKeywordConfig()
{
  // Tell the syntax document class which file we want to parse
  kdDebug(13010)<<"readGlobalKeywordConfig:BEGIN"<<endl;

  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getConfig("general","keywords");

  if (data)
  {
    kdDebug(13010)<<"Found global keyword config"<<endl;

    if (KateHlManager::self()->syntax->groupItemData(data,QString("casesensitive"))!="0")
      casesensitive=true;
    else
      casesensitive=false;

    //get the weak deliminators
    weakDeliminator=(KateHlManager::self()->syntax->groupItemData(data,QString("weakDeliminator")));

    kdDebug(13010)<<"weak delimiters are: "<<weakDeliminator<<endl;

    // remove any weakDelimitars (if any) from the default list and store this list.
    for (uint s=0; s < weakDeliminator.length(); s++)
    {
      int f = deliminator.find (weakDeliminator[s]);

      if (f > -1)
        deliminator.remove (f, 1);
    }

    QString addDelim = (KateHlManager::self()->syntax->groupItemData(data,QString("additionalDeliminator")));

    if (!addDelim.isEmpty())
      deliminator=deliminator+addDelim;

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }
  else
  {
    //Default values
    casesensitive=true;
    weakDeliminator=QString("");
  }

  kdDebug(13010)<<"readGlobalKeywordConfig:END"<<endl;

  kdDebug(13010)<<"delimiterCharacters are: "<<deliminator<<endl;

  return deliminator; // FIXME un-globalize
}

/**
 * Helper for makeContextList. It parses the xml file for any wordwrap deliminators, characters
 * at which line can be broken. In case no keyword tag is found in the xml file,
 * the wordwrap deliminators list defaults to the standard denominators. In case a keyword tag
 * is defined, but no wordWrapDeliminator attribute is specified, the deliminator list as computed
 * in readGlobalKeywordConfig is used.
 *
 * @return the computed delimiter string.
 */
QString KateHighlighting::readWordWrapConfig()
{
  // Tell the syntax document class which file we want to parse
  kdDebug(13010)<<"readWordWrapConfig:BEGIN"<<endl;

  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getConfig("general","keywords");

  QString wordWrapDeliminator = stdDeliminator;
  if (data)
  {
    kdDebug(13010)<<"Found global keyword config"<<endl;

    wordWrapDeliminator = (KateHlManager::self()->syntax->groupItemData(data,QString("wordWrapDeliminator")));
    //when no wordWrapDeliminator is defined use the deliminator list
    if ( wordWrapDeliminator.length() == 0 ) wordWrapDeliminator = deliminator;

    kdDebug(13010) << "word wrap deliminators are " << wordWrapDeliminator << endl;

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }

  kdDebug(13010)<<"readWordWrapConfig:END"<<endl;

  return wordWrapDeliminator; // FIXME un-globalize
}

void KateHighlighting::readFoldingConfig()
{
  // Tell the syntax document class which file we want to parse
  kdDebug(13010)<<"readfoldignConfig:BEGIN"<<endl;

  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);
  KateSyntaxContextData *data = KateHlManager::self()->syntax->getConfig("general","folding");

  if (data)
  {
    kdDebug(13010)<<"Found global keyword config"<<endl;

    if (KateHlManager::self()->syntax->groupItemData(data,QString("indentationsensitive"))!="1")
      m_foldingIndentationSensitive=false;
    else
      m_foldingIndentationSensitive=true;

    KateHlManager::self()->syntax->freeGroupInfo(data);
  }
  else
  {
    //Default values
    m_foldingIndentationSensitive = false;
  }

  kdDebug(13010)<<"readfoldingConfig:END"<<endl;

  kdDebug(13010)<<"############################ use indent for fold are: "<<m_foldingIndentationSensitive<<endl;
}

void  KateHighlighting::createContextNameList(QStringList *ContextNameList,int ctx0)
{
  kdDebug(13010)<<"creatingContextNameList:BEGIN"<<endl;

  if (ctx0 == 0)
      ContextNameList->clear();

  KateHlManager::self()->syntax->setIdentifier(buildIdentifier);

  KateSyntaxContextData *data=KateHlManager::self()->syntax->getGroupInfo("highlighting","context");

  int id=ctx0;

  if (data)
  {
     while (KateHlManager::self()->syntax->nextGroup(data))
     {
          QString tmpAttr=KateHlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace();
    if (tmpAttr.isEmpty())
    {
     tmpAttr=QString("!KATE_INTERNAL_DUMMY! %1").arg(id);
     errorsAndWarnings +=i18n("<B>%1</B>: Deprecated syntax. Context %2 has no symbolic name<BR>").arg(buildIdentifier).arg(id-ctx0);
    }
          else tmpAttr=buildPrefix+tmpAttr;
    (*ContextNameList)<<tmpAttr;
          id++;
     }
     KateHlManager::self()->syntax->freeGroupInfo(data);
  }
  kdDebug(13010)<<"creatingContextNameList:END"<<endl;

}

int KateHighlighting::getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext, /*NO CONST*/ QString &unres)
{
  unres="";
  int context;
  if ((tmpLineEndContext=="#stay") || (tmpLineEndContext.simplifyWhiteSpace().isEmpty()))
    context=-1;

  else if (tmpLineEndContext.startsWith("#pop"))
  {
    context=-1;
    for(;tmpLineEndContext.startsWith("#pop");context--)
    {
      tmpLineEndContext.remove(0,4);
      kdDebug(13010)<<"#pop found"<<endl;
    }
  }

  else if ( tmpLineEndContext.startsWith("##"))
  {
    QString tmp=tmpLineEndContext.right(tmpLineEndContext.length()-2);
    if (!embeddedHls.contains(tmp))  embeddedHls.insert(tmp,KateEmbeddedHlInfo());
    unres=tmp;
    context=0;
  }

  else
  {
    context=ContextNameList->findIndex(buildPrefix+tmpLineEndContext);
    if (context==-1)
    {
      context=tmpLineEndContext.toInt();
      errorsAndWarnings+=i18n(
        "<B>%1</B>:Deprecated syntax. Context %2 not addressed by a symbolic name"
        ).arg(buildIdentifier).arg(tmpLineEndContext);
    }
//#warning restructure this the name list storage.
//    context=context+buildContext0Offset;
  }
  return context;
}

/**
 * The most important initialization function for each highlighting. It's called
 * each time a document gets a highlighting style assigned. parses the xml file
 * and creates a corresponding internal structure
 */
void KateHighlighting::makeContextList()
{
  if (noHl)  // if this a highlighting for "normal texts" only, tere is no need for a context list creation
    return;

  embeddedHls.clear();
  unresolvedContextReferences.clear();
  RegionList.clear();
  ContextNameList.clear();

  // prepare list creation. To reuse as much code as possible handle this
  // highlighting the same way as embedded onces
  embeddedHls.insert(iName,KateEmbeddedHlInfo());

  bool something_changed;
  // the context "0" id is 0 for this hl, all embedded context "0"s have offsets
  startctx=base_startctx=0;
  // inform everybody that we are building the highlighting contexts and itemlists
  building=true;

  do
  {
    kdDebug(13010)<<"**************** Outter loop in make ContextList"<<endl;
    kdDebug(13010)<<"**************** Hl List count:"<<embeddedHls.count()<<endl;
    something_changed=false; //assume all "embedded" hls have already been loaded
    for (KateEmbeddedHlInfos::const_iterator it=embeddedHls.begin(); it!=embeddedHls.end();++it)
    {
      if (!it.data().loaded)  // we found one, we still have to load
      {
        kdDebug(13010)<<"**************** Inner loop in make ContextList"<<endl;
        QString identifierToUse;
        kdDebug(13010)<<"Trying to open highlighting definition file: "<< it.key()<<endl;
        if (iName==it.key())
          identifierToUse=identifier;  // the own identifier is known
        else
          identifierToUse=KateHlManager::self()->identifierForName(it.key()); // all others have to be looked up

        kdDebug(13010)<<"Location is:"<< identifierToUse<<endl;

        buildPrefix=it.key()+':';  // attribute names get prefixed by the names of the highlighting definitions they belong to

        if (identifierToUse.isEmpty() ) kdDebug(13010)<<"OHOH, unknown highlighting description referenced"<<endl;

        kdDebug(13010)<<"setting ("<<it.key()<<") to loaded"<<endl;

        //mark hl as loaded
        it=embeddedHls.insert(it.key(),KateEmbeddedHlInfo(true,startctx));
        //set class member for context 0 offset, so we don't need to pass it around
        buildContext0Offset=startctx;
        //parse one hl definition file
        startctx=addToContextList(identifierToUse,startctx);

        if (noHl) return;  // an error occurred

        base_startctx = startctx;
        something_changed=true; // something has been loaded
      }
    }
  } while (something_changed);  // as long as there has been another file parsed
                  // repeat everything, there could be newly added embedded hls.


  // at this point all needed highlighing (sub)definitions are loaded. It's time
  // to resolve cross file  references (if there are any )
  kdDebug(13010)<<"Unresolved contexts, which need attention: "<<unresolvedContextReferences.count()<<endl;

  //optimize this a littlebit
  for (KateHlUnresolvedCtxRefs::iterator unresIt=unresolvedContextReferences.begin();
    unresIt!=unresolvedContextReferences.end();++unresIt)
  {
    //try to find the context0 id for a given unresolvedReference
    KateEmbeddedHlInfos::const_iterator hlIt=embeddedHls.find(unresIt.data());
    if (hlIt!=embeddedHls.end())
      *(unresIt.key())=hlIt.data().context0;
  }

  // eventually handle KateHlIncludeRules items, if they exist.
  // This has to be done after the cross file references, because it is allowed
  // to include the context0 from a different definition, than the one the rule
  // belongs to
  handleKateHlIncludeRules();

  embeddedHls.clear(); //save some memory.
  unresolvedContextReferences.clear(); //save some memory
  RegionList.clear();  // I think you get the idea ;)
  ContextNameList.clear();


  // if there have been errors show them
  if (!errorsAndWarnings.isEmpty())
  KMessageBox::detailedSorry(0L,i18n("There were warning(s) and/or error(s) while parsing the syntax highlighting configuration."), errorsAndWarnings, i18n("Kate Syntax Highlighting Parser"));

  // we have finished
  building=false;
}

void KateHighlighting::handleKateHlIncludeRules()
{
  // if there are noe include rules to take care of, just return
  kdDebug(13010)<<"KateHlIncludeRules, which need attention: " <<includeRules.count()<<endl;
  if (includeRules.isEmpty()) return;

  buildPrefix="";
  QString dummy;

  // By now the context0 references are resolved, now more or less only inner
  // file references are resolved. If we decide that arbitrary inclusion is
  // needed, this doesn't need to be changed, only the addToContextList
  // method.

  //resolove context names
  for (KateHlIncludeRules::iterator it=includeRules.begin();it!=includeRules.end();)
  {
    if ((*it)->incCtx==-1) // context unresolved ?
    {

      if ((*it)->incCtxN.isEmpty())
      {
        // no context name given, and no valid context id set, so this item is going to be removed
        KateHlIncludeRules::iterator it1=it;
        ++it1;
        delete (*it);
        includeRules.remove(it);
        it=it1;
      }
      else
      {
        // resolve name to id
        (*it)->incCtx=getIdFromString(&ContextNameList,(*it)->incCtxN,dummy);
        kdDebug(13010)<<"Resolved "<<(*it)->incCtxN<< " to "<<(*it)->incCtx<<" for include rule"<<endl;
        // It would be good to look here somehow, if the result is valid
      }
    }
    else ++it; //nothing to do, already resolved (by the cross defintion reference resolver
  }

  // now that all KateHlIncludeRule items should be valid and completely resolved, do the real inclusion of the rules.
  // recursiveness is needed, because context 0 could include context 1, which itself includes context 2 and so on.
  //  In that case we have to handle context 2 first, then 1, 0
  //TODO: catch circular references: eg 0->1->2->3->1
  while (!includeRules.isEmpty())
    handleKateHlIncludeRulesRecursive(includeRules.begin(),&includeRules);
}

void KateHighlighting::handleKateHlIncludeRulesRecursive(KateHlIncludeRules::iterator it, KateHlIncludeRules *list)
{
  if (it==list->end()) return;  //invalid iterator, shouldn't happen, but better have a rule prepared ;)
  KateHlIncludeRules::iterator it1=it;
  int ctx=(*it1)->ctx;

  // find the last entry for the given context in the KateHlIncludeRules list
  // this is need if one context includes more than one. This saves us from
  // updating all insert positions:
  // eg: context 0:
  // pos 3 - include context 2
  // pos 5 - include context 3
  // During the building of the includeRules list the items are inserted in
  // ascending order, now we need it descending to make our life easier.
  while ((it!=list->end()) && ((*it)->ctx==ctx))
  {
    it1=it;
    ++it;
  }

  // iterate over each include rule for the context the function has been called for.
  while ((it1!=list->end()) && ((*it1)->ctx==ctx))
  {
    int ctx1=(*it1)->incCtx;

    //let's see, if the the included context includes other contexts
    for (KateHlIncludeRules::iterator it2=list->begin();it2!=list->end();++it2)
    {
      if ((*it2)->ctx==ctx1)
      {
        //yes it does, so first handle that include rules, since we want to
        // include those subincludes too
        handleKateHlIncludeRulesRecursive(it2,list);
        break;
      }
    }

    // if the context we want to include had sub includes, they are already inserted there.
    KateHlContext *dest=contextList[ctx];
    KateHlContext *src=contextList[ctx1];
//     kdDebug(3010)<<"linking included rules from "<<ctx<<" to "<<ctx1<<endl;

    // If so desired, change the dest attribute to the one of the src.
    // Required to make commenting work, if text matched by the included context
    // is a different highlight than the host context.
    if ( (*it1)->includeAttrib )
      dest->attr = src->attr;

    // insert the included context's rules starting at position p
    int p=(*it1)->pos;

    // remember some stuff
    int oldLen = dest->items.size();
    uint itemsToInsert = src->items.size();

    // resize target
    dest->items.resize (oldLen + itemsToInsert);

    // move old elements
    for (int i=oldLen-1; i >= p; --i)
      dest->items[i+itemsToInsert] = dest->items[i];

    // insert new stuff
    for (uint i=0; i < itemsToInsert; ++i  )
      dest->items[p+i] = src->items[i];

    it=it1; //backup the iterator
    --it1;  //move to the next entry, which has to be take care of
    delete (*it); //free the already handled data structure
    list->remove(it); // remove it from the list
  }
}

/**
 * Add one highlight to the contextlist.
 *
 * @return the number of contexts after this is added.
 */
int KateHighlighting::addToContextList(const QString &ident, int ctx0)
{
  buildIdentifier=ident;
  KateSyntaxContextData *data, *datasub;
  KateHlItem *c;

  QString dummy;

  // Let the syntax document class know, which file we'd like to parse
  if (!KateHlManager::self()->syntax->setIdentifier(ident))
  {
    noHl=true;
    KMessageBox::information(0L,i18n("Since there has been an error parsing the highlighting description, this highlighting will be disabled"));
    return 0;
  }


  RegionList<<"!KateInternal_TopLevel!";

  // Now save the comment and delimitor data. We associate it with the
  // length of internalDataList, so when we look it up for an attrib,
  // all the attribs added in a moment will be in the correct range
  QStringList additionaldata = readCommentConfig();
  additionaldata << readGlobalKeywordConfig();
  additionaldata << readWordWrapConfig();

  readFoldingConfig ();

  uint additionalDataIndex=internalIDList.count();
  m_additionalData.insert( additionalDataIndex, additionaldata );
  m_hlIndex.append( additionalDataIndex );

  QString ctxName;

  // This list is needed for the translation of the attribute parameter,
  // if the itemData name is given instead of the index
  addToKateHlItemDataList();
  KateHlItemDataList iDl = internalIDList;

  createContextNameList(&ContextNameList,ctx0);


  kdDebug(13010)<<"Parsing Context structure"<<endl;
  //start the real work
  data=KateHlManager::self()->syntax->getGroupInfo("highlighting","context");
  uint i=buildContext0Offset;
  if (data)
  {
    while (KateHlManager::self()->syntax->nextGroup(data))
    {
      kdDebug(13010)<<"Found a context in file, building structure now"<<endl;
      // BEGIN - Translation of the attribute parameter
      QString tmpAttr=KateHlManager::self()->syntax->groupData(data,QString("attribute")).simplifyWhiteSpace();
      int attr;
      if (QString("%1").arg(tmpAttr.toInt())==tmpAttr)
        attr=tmpAttr.toInt();
      else
        attr=lookupAttrName(tmpAttr,iDl);
      // END - Translation of the attribute parameter

      ctxName=buildPrefix+KateHlManager::self()->syntax->groupData(data,QString("lineEndContext")).simplifyWhiteSpace();

      QString tmpLineEndContext=KateHlManager::self()->syntax->groupData(data,QString("lineEndContext")).simplifyWhiteSpace();
      int context;

      context=getIdFromString(&ContextNameList, tmpLineEndContext,dummy);

      // BEGIN get fallthrough props
      bool ft = false;
      int ftc = 0; // fallthrough context
      if ( i > 0 )  // fallthrough is not smart in context 0
      {
        QString tmpFt = KateHlManager::self()->syntax->groupData(data, QString("fallthrough") );
        if ( IS_TRUE(tmpFt) )
          ft = true;
        if ( ft )
        {
          QString tmpFtc = KateHlManager::self()->syntax->groupData( data, QString("fallthroughContext") );

          ftc=getIdFromString(&ContextNameList, tmpFtc,dummy);
          if (ftc == -1) ftc =0;

          kdDebug(13010)<<"Setting fall through context (context "<<i<<"): "<<ftc<<endl;
        }
      }
      // END falltrhough props

      bool dynamic = false;
      QString tmpDynamic = KateHlManager::self()->syntax->groupData(data, QString("dynamic") );
      if ( tmpDynamic.lower() == "true" ||  tmpDynamic.toInt() == 1 )
        dynamic = true;

      contextList.insert (i, new KateHlContext (
        attr,
        context,
        (KateHlManager::self()->syntax->groupData(data,QString("lineBeginContext"))).isEmpty()?-1:
        (KateHlManager::self()->syntax->groupData(data,QString("lineBeginContext"))).toInt(),
        ft, ftc, dynamic));

      //Let's create all items for the context
      while (KateHlManager::self()->syntax->nextItem(data))
      {
//    kdDebug(13010)<< "In make Contextlist: Item:"<<endl;

      // KateHlIncludeRules : add a pointer to each item in that context
        // TODO add a attrib includeAttrib
      QString tag = KateHlManager::self()->syntax->groupItemData(data,QString(""));
      if ( tag == "IncludeRules" ) //if the new item is an Include rule, we have to take special care
      {
        QString incCtx = KateHlManager::self()->syntax->groupItemData( data, QString("context"));
        QString incAttrib = KateHlManager::self()->syntax->groupItemData( data, QString("includeAttrib"));
        bool includeAttrib = ( incAttrib.lower() == "true" || incAttrib.toInt() == 1 );
        // only context refernces of type NAME and ##Name are allowed
        if (incCtx.startsWith("##") || (!incCtx.startsWith("#")))
        {
          //#stay, #pop is not interesting here
          if (!incCtx.startsWith("#"))
          {
            // a local reference -> just initialize the include rule structure
            incCtx=buildPrefix+incCtx.simplifyWhiteSpace();
            includeRules.append(new KateHlIncludeRule(i,contextList[i]->items.count(),incCtx, includeAttrib));
          }
          else
          {
            //a cross highlighting reference
            kdDebug(13010)<<"Cross highlight reference <IncludeRules>"<<endl;
            KateHlIncludeRule *ir=new KateHlIncludeRule(i,contextList[i]->items.count(),"",includeAttrib);

            //use the same way to determine cross hl file references as other items do
            if (!embeddedHls.contains(incCtx.right(incCtx.length()-2)))
              embeddedHls.insert(incCtx.right(incCtx.length()-2),KateEmbeddedHlInfo());

            unresolvedContextReferences.insert(&(ir->incCtx),
                incCtx.right(incCtx.length()-2));

            includeRules.append(ir);
          }
        }

        continue;
      }
      // TODO -- can we remove the block below??
#if 0
                QString tag = KateHlManager::self()->syntax->groupKateHlItemData(data,QString(""));
                if ( tag == "IncludeRules" ) {
                  // attrib context: the index (jowenn, i think using names here would be a cool feat, goes for mentioning the context in any item. a map or dict?)
                  int ctxId = getIdFromString(&ContextNameList,
      KateHlManager::self()->syntax->groupKateHlItemData( data, QString("context")),dummy); // the index is *required*
                  if ( ctxId > -1) { // we can even reuse rules of 0 if we want to:)
                    kdDebug(13010)<<"makeContextList["<<i<<"]: including all items of context "<<ctxId<<endl;
                    if ( ctxId < (int) i ) { // must be defined
                      for ( c = contextList[ctxId]->items.first(); c; c = contextList[ctxId]->items.next() )
                        contextList[i]->items.append(c);
                    }
                    else
                      kdDebug(13010)<<"Context "<<ctxId<<"not defined. You can not include the rules of an undefined context"<<endl;
                  }
                  continue; // while nextItem
                }
#endif
      c=createKateHlItem(data,iDl,&RegionList,&ContextNameList);
      if (c)
      {
        contextList[i]->items.append(c);

        // Not supported completely atm and only one level. Subitems.(all have to be matched to at once)
        datasub=KateHlManager::self()->syntax->getSubItems(data);
        bool tmpbool;
        if (tmpbool=KateHlManager::self()->syntax->nextItem(datasub))
        {
          for (;tmpbool;tmpbool=KateHlManager::self()->syntax->nextItem(datasub))
          {
            c->subItems.resize (c->subItems.size()+1);
            c->subItems[c->subItems.size()-1] = createKateHlItem(datasub,iDl,&RegionList,&ContextNameList);
          }                             }
          KateHlManager::self()->syntax->freeGroupInfo(datasub);
                              // end of sublevel
        }
      }
      i++;
    }
  }

  KateHlManager::self()->syntax->freeGroupInfo(data);

  if (RegionList.count()!=1)
    folding=true;

  folding = folding || m_foldingIndentationSensitive;

  //BEGIN Resolve multiline region if possible
  QStringList& commentData=m_additionalData[additionalDataIndex];
  if (!commentData[MultiLineRegion].isEmpty()) {
    long commentregionid=RegionList.findIndex(commentData[MultiLineRegion]);
    if (-1==commentregionid) {
      errorsAndWarnings+=i18n("<B>%1</B>: Specified multiline comment region (%2) could not be resolved<BR>").arg(buildIdentifier).arg(commentData[MultiLineRegion]);
      commentData[MultiLineRegion]=QString();
      kdDebug()<<"ERROR comment region attribute could not be resolved"<<endl;

    } else {
        commentData[MultiLineRegion]=QString::number(commentregionid+1);
        kdDebug()<<"comment region resolved to:"<<m_additionalData[additionalDataIndex][MultiLineRegion]<<endl;
    }
  }
  //END Resolve multiline region if possible
  return i;
}

void KateHighlighting::clearAttributeArrays ()
{
  for ( QIntDictIterator< QMemArray<KateAttribute> > it( m_attributeArrays ); it.current(); ++it )
  {
    // k, schema correct, let create the data
    KateAttributeList defaultStyleList;
    defaultStyleList.setAutoDelete(true);
    KateHlManager::self()->getDefaults(it.currentKey(), defaultStyleList);

    KateHlItemDataList itemDataList;
    getKateHlItemDataList(it.currentKey(), itemDataList);

    uint nAttribs = itemDataList.count();
    QMemArray<KateAttribute> *array = it.current();
    array->resize (nAttribs);

    for (uint z = 0; z < nAttribs; z++)
    {
      KateHlItemData *itemData = itemDataList.at(z);
      KateAttribute n = *defaultStyleList.at(itemData->defStyleNum);

      if (itemData && itemData->isSomethingSet())
        n += *itemData;

      array->at(z) = n;
    }
  }
}

QMemArray<KateAttribute> *KateHighlighting::attributes (uint schema)
{
  QMemArray<KateAttribute> *array;

  // found it, allready floating around
  if ((array = m_attributeArrays[schema]))
    return array;

  // ohh, not found, check if valid schema number
  if (!KateFactory::self()->schemaManager()->validSchema(schema))
  {
    // uhh, not valid :/, stick with normal default schema, it's always there !
    return attributes (0);
  }

  // k, schema correct, let create the data
  KateAttributeList defaultStyleList;
  defaultStyleList.setAutoDelete(true);
  KateHlManager::self()->getDefaults(schema, defaultStyleList);

  KateHlItemDataList itemDataList;
  getKateHlItemDataList(schema, itemDataList);

  uint nAttribs = itemDataList.count();
  array = new QMemArray<KateAttribute> (nAttribs);

  for (uint z = 0; z < nAttribs; z++)
  {
    KateHlItemData *itemData = itemDataList.at(z);
    KateAttribute n = *defaultStyleList.at(itemData->defStyleNum);

    if (itemData && itemData->isSomethingSet())
      n += *itemData;

    array->at(z) = n;
  }

  m_attributeArrays.insert(schema, array);

  return array;
}

void KateHighlighting::getKateHlItemDataListCopy (uint schema, KateHlItemDataList &outlist)
{
  KateHlItemDataList itemDataList;
  getKateHlItemDataList(schema, itemDataList);

  outlist.clear ();
  outlist.setAutoDelete (true);
  for (uint z=0; z < itemDataList.count(); z++)
    outlist.append (new KateHlItemData (*itemDataList.at(z)));
}

//END

//BEGIN KateHlManager
KateHlManager::KateHlManager()
  : QObject()
  , m_config ("katesyntaxhighlightingrc", false, false)
  , commonSuffixes (QStringList::split(";", ".orig;.new;~;.bak;.BAK"))
  , syntax (new KateSyntaxDocument())
  , dynamicCtxsCount(0)
  , forceNoDCReset(false)
{
  hlList.setAutoDelete(true);
  hlDict.setAutoDelete(false);

  KateSyntaxModeList modeList = syntax->modeList();
  for (uint i=0; i < modeList.count(); i++)
  {
    KateHighlighting *hl = new KateHighlighting(modeList[i]);

    uint insert = 0;
    for (; insert <= hlList.count(); insert++)
    {
      if (insert == hlList.count())
        break;

      if ( QString(hlList.at(insert)->section() + hlList.at(insert)->nameTranslated()).lower()
            > QString(hl->section() + hl->nameTranslated()).lower() )
        break;
    }

    hlList.insert (insert, hl);
    hlDict.insert (hl->name(), hl);
  }

  // Normal HL
  KateHighlighting *hl = new KateHighlighting(0);
  hlList.prepend (hl);
  hlDict.insert (hl->name(), hl);

  lastCtxsReset.start();
}

KateHlManager::~KateHlManager()
{
  delete syntax;
}

static KStaticDeleter<KateHlManager> sdHlMan;

KateHlManager *KateHlManager::self()
{
  if ( !s_self )
    sdHlMan.setObject(s_self, new KateHlManager ());

  return s_self;
}

KateHighlighting *KateHlManager::getHl(int n)
{
  if (n < 0 || n >= (int) hlList.count())
    n = 0;

  return hlList.at(n);
}

int KateHlManager::nameFind(const QString &name)
{
  int z (hlList.count() - 1);
  for (; z > 0; z--)
    if (hlList.at(z)->name() == name)
      return z;

  return z;
}

int KateHlManager::detectHighlighting (KateDocument *doc)
{
  int hl = wildcardFind( doc->url().filename() );
  if ( hl < 0 )
    hl = mimeFind ( doc );

  return hl;
}

int KateHlManager::wildcardFind(const QString &fileName)
{
  int result = -1;
  if ((result = realWildcardFind(fileName)) != -1)
    return result;

  int length = fileName.length();
  QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
  if (fileName.endsWith(backupSuffix)) {
    if ((result = realWildcardFind(fileName.left(length - backupSuffix.length()))) != -1)
      return result;
  }

  for (QStringList::Iterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
    if (*it != backupSuffix && fileName.endsWith(*it)) {
      if ((result = realWildcardFind(fileName.left(length - (*it).length()))) != -1)
        return result;
    }
  }

  return -1;
}

int KateHlManager::realWildcardFind(const QString &fileName)
{
  static QRegExp sep("\\s*;\\s*");

  QPtrList<KateHighlighting> highlights;

  for (KateHighlighting *highlight = hlList.first(); highlight != 0L; highlight = hlList.next()) {
    highlight->loadWildcards();

    for (QStringList::Iterator it = highlight->getPlainExtensions().begin(); it != highlight->getPlainExtensions().end(); ++it)
      if (fileName.endsWith((*it)))
        highlights.append(highlight);

    for (int i = 0; i < (int)highlight->getRegexpExtensions().count(); i++) {
      QRegExp re = highlight->getRegexpExtensions()[i];
      if (re.exactMatch(fileName))
        highlights.append(highlight);
    }
  }

  if ( !highlights.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    for (KateHighlighting *highlight = highlights.first(); highlight != 0L; highlight = highlights.next())
    {
      if (highlight->priority() > pri)
      {
        pri = highlight->priority();
        hl = hlList.findRef (highlight);
      }
    }
    return hl;
  }

  return -1;
}

int KateHlManager::mimeFind( KateDocument *doc )
{
  static QRegExp sep("\\s*;\\s*");

  KMimeType::Ptr mt = doc->mimeTypeForContent();

  QPtrList<KateHighlighting> highlights;

  for (KateHighlighting *highlight = hlList.first(); highlight != 0L; highlight = hlList.next())
  {
    QStringList l = QStringList::split( sep, highlight->getMimetypes() );

    for( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
    {
      if ( *it == mt->name() ) // faster than a regexp i guess?
        highlights.append (highlight);
    }
  }

  if ( !highlights.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    for (KateHighlighting *highlight = highlights.first(); highlight != 0L; highlight = highlights.next())
    {
      if (highlight->priority() > pri)
      {
        pri = highlight->priority();
        hl = hlList.findRef (highlight);
      }
    }

    return hl;
  }

  return -1;
}

uint KateHlManager::defaultStyles()
{
  return 14;
}

QString KateHlManager::defaultStyleName(int n, bool translateNames)
{
  static QStringList names;
  static QStringList translatedNames;

  if (names.isEmpty())
  {
    names << "Normal";
    names << "Keyword";
    names << "Data Type";
    names << "Decimal/Value";
    names << "Base-N Integer";
    names << "Floating Point";
    names << "Character";
    names << "String";
    names << "Comment";
    names << "Others";
    names << "Alert";
    names << "Function";
    // this next one is for denoting the beginning/end of a user defined folding region
    names << "Region Marker";
    // this one is for marking invalid input
    names << "Error";

    translatedNames << i18n("Normal");
    translatedNames << i18n("Keyword");
    translatedNames << i18n("Data Type");
    translatedNames << i18n("Decimal/Value");
    translatedNames << i18n("Base-N Integer");
    translatedNames << i18n("Floating Point");
    translatedNames << i18n("Character");
    translatedNames << i18n("String");
    translatedNames << i18n("Comment");
    translatedNames << i18n("Others");
    translatedNames << i18n("Alert");
    translatedNames << i18n("Function");
    // this next one is for denoting the beginning/end of a user defined folding region
    translatedNames << i18n("Region Marker");
    // this one is for marking invalid input
    translatedNames << i18n("Error");
  }

  return translateNames ? translatedNames[n] : names[n];
}

void KateHlManager::getDefaults(uint schema, KateAttributeList &list)
{
  list.setAutoDelete(true);

  KateAttribute* normal = new KateAttribute();
  normal->setTextColor(Qt::black);
  normal->setSelectedTextColor(Qt::white);
  list.append(normal);

  KateAttribute* keyword = new KateAttribute();
  keyword->setTextColor(Qt::black);
  keyword->setSelectedTextColor(Qt::white);
  keyword->setBold(true);
  list.append(keyword);

  KateAttribute* dataType = new KateAttribute();
  dataType->setTextColor(Qt::darkRed);
  dataType->setSelectedTextColor(Qt::white);
  list.append(dataType);

  KateAttribute* decimal = new KateAttribute();
  decimal->setTextColor(Qt::blue);
  decimal->setSelectedTextColor(Qt::cyan);
  list.append(decimal);

  KateAttribute* basen = new KateAttribute();
  basen->setTextColor(Qt::darkCyan);
  basen->setSelectedTextColor(Qt::cyan);
  list.append(basen);

  KateAttribute* floatAttribute = new KateAttribute();
  floatAttribute->setTextColor(Qt::darkMagenta);
  floatAttribute->setSelectedTextColor(Qt::cyan);
  list.append(floatAttribute);

  KateAttribute* charAttribute = new KateAttribute();
  charAttribute->setTextColor(Qt::magenta);
  charAttribute->setSelectedTextColor(Qt::magenta);
  list.append(charAttribute);

  KateAttribute* string = new KateAttribute();
  string->setTextColor(QColor::QColor("#D00"));
  string->setSelectedTextColor(Qt::red);
  list.append(string);

  KateAttribute* comment = new KateAttribute();
  comment->setTextColor(Qt::darkGray);
  comment->setSelectedTextColor(Qt::gray);
  comment->setItalic(true);
  list.append(comment);

  KateAttribute* others = new KateAttribute();
  others->setTextColor(Qt::darkGreen);
  others->setSelectedTextColor(Qt::green);
  list.append(others);

  KateAttribute* alert = new KateAttribute();
  alert->setTextColor(Qt::white);
  alert->setSelectedTextColor( QColor::QColor("#FCC") );
  alert->setBold(true);
  alert->setBGColor( QColor::QColor("#FCC") );
  list.append(alert);

  KateAttribute* functionAttribute = new KateAttribute();
  functionAttribute->setTextColor(Qt::darkBlue);
  functionAttribute->setSelectedTextColor(Qt::white);
  list.append(functionAttribute);

  KateAttribute* regionmarker = new KateAttribute();
  regionmarker->setTextColor(Qt::white);
  regionmarker->setBGColor(Qt::gray);
  regionmarker->setSelectedTextColor(Qt::gray);
  list.append(regionmarker);

  KateAttribute* error = new KateAttribute();
  error->setTextColor(Qt::red);
  error->setUnderline(true);
  error->setSelectedTextColor(Qt::red);
  list.append(error);

  KConfig *config = KateHlManager::self()->self()->getKConfig();
  config->setGroup("Default Item Styles - Schema " + KateFactory::self()->schemaManager()->name(schema));

  for (uint z = 0; z < defaultStyles(); z++)
  {
    KateAttribute *i = list.at(z);
    QStringList s = config->readListEntry(defaultStyleName(z));
    if (!s.isEmpty())
    {
      while( s.count()<8)
        s << "";

      QString tmp;
      QRgb col;

      tmp=s[0]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); i->setTextColor(col); }

      tmp=s[1]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); i->setSelectedTextColor(col); }

      tmp=s[2]; if (!tmp.isEmpty()) i->setBold(tmp!="0");

      tmp=s[3]; if (!tmp.isEmpty()) i->setItalic(tmp!="0");

      tmp=s[4]; if (!tmp.isEmpty()) i->setStrikeOut(tmp!="0");

      tmp=s[5]; if (!tmp.isEmpty()) i->setUnderline(tmp!="0");

      tmp=s[6]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); i->setBGColor(col); }

      tmp=s[7]; if (!tmp.isEmpty()) {
         col=tmp.toUInt(0,16); i->setSelectedBGColor(col); }
    }
  }
}

void KateHlManager::setDefaults(uint schema, KateAttributeList &list)
{
  KConfig *config =  KateHlManager::self()->self()->getKConfig();
  config->setGroup("Default Item Styles - Schema " + KateFactory::self()->schemaManager()->name(schema));

  for (uint z = 0; z < defaultStyles(); z++)
  {
    QStringList settings;
    KateAttribute *i = list.at(z);

    settings<<(i->itemSet(KateAttribute::TextColor)?QString::number(i->textColor().rgb(),16):"");
    settings<<(i->itemSet(KateAttribute::SelectedTextColor)?QString::number(i->selectedTextColor().rgb(),16):"");
    settings<<(i->itemSet(KateAttribute::Weight)?(i->bold()?"1":"0"):"");
    settings<<(i->itemSet(KateAttribute::Italic)?(i->italic()?"1":"0"):"");
    settings<<(i->itemSet(KateAttribute::StrikeOut)?(i->strikeOut()?"1":"0"):"");
    settings<<(i->itemSet(KateAttribute::Underline)?(i->underline()?"1":"0"):"");
    settings<<(i->itemSet(KateAttribute::BGColor)?QString::number(i->bgColor().rgb(),16):"");
    settings<<(i->itemSet(KateAttribute::SelectedBGColor)?QString::number(i->selectedBGColor().rgb(),16):"");
    settings<<"---";

    config->writeEntry(defaultStyleName(z),settings);
  }

  emit changed();
}

int KateHlManager::highlights()
{
  return (int) hlList.count();
}

QString KateHlManager::hlName(int n)
{
  return hlList.at(n)->name();
}

QString KateHlManager::hlNameTranslated(int n)
{
  return hlList.at(n)->nameTranslated();
}

QString KateHlManager::hlSection(int n)
{
  return hlList.at(n)->section();
}

bool KateHlManager::hlHidden(int n)
{
  return hlList.at(n)->hidden();
}

QString KateHlManager::identifierForName(const QString& name)
{
  KateHighlighting *hl = 0;

  if ((hl = hlDict[name]))
    return hl->getIdentifier ();

  return QString();
}

bool KateHlManager::resetDynamicCtxs()
{
  if (forceNoDCReset)
    return false;

  if (lastCtxsReset.elapsed() < KATE_DYNAMIC_CONTEXTS_RESET_DELAY)
    return false;

  KateHighlighting *hl;
  for (hl = hlList.first(); hl; hl = hlList.next())
    hl->dropDynamicContexts();

  dynamicCtxsCount = 0;
  lastCtxsReset.start();

  return true;
}
//END

//BEGIN KateHighlightAction
void KateViewHighlightAction::init()
{
  m_doc = 0;
  subMenus.setAutoDelete( true );

  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewHighlightAction::updateMenu (Kate::Document *doc)
{
  m_doc = doc;
}

void KateViewHighlightAction::slotAboutToShow()
{
  Kate::Document *doc=m_doc;
  int count = KateHlManager::self()->highlights();

  for (int z=0; z<count; z++)
  {
    QString hlName = KateHlManager::self()->hlNameTranslated (z);
    QString hlSection = KateHlManager::self()->hlSection (z);

    if (!KateHlManager::self()->hlHidden(z))
    {
      if ( !hlSection.isEmpty() && (names.contains(hlName) < 1) )
      {
        if (subMenusName.contains(hlSection) < 1)
        {
          subMenusName << hlSection;
          QPopupMenu *menu = new QPopupMenu ();
          subMenus.append(menu);
          popupMenu()->insertItem ( '&' + hlSection, menu);
        }

        int m = subMenusName.findIndex (hlSection);
        names << hlName;
        subMenus.at(m)->insertItem ( '&' + hlName, this, SLOT(setHl(int)), 0,  z);
      }
      else if (names.contains(hlName) < 1)
      {
        names << hlName;
        popupMenu()->insertItem ( '&' + hlName, this, SLOT(setHl(int)), 0,  z);
      }
    }
  }

  if (!doc) return;

  for (uint i=0;i<subMenus.count();i++)
  {
    for (uint i2=0;i2<subMenus.at(i)->count();i2++)
    {
      subMenus.at(i)->setItemChecked(subMenus.at(i)->idAt(i2),false);
    }
  }
  popupMenu()->setItemChecked (0, false);

  int i = subMenusName.findIndex (KateHlManager::self()->hlSection(doc->hlMode()));
  if (i >= 0 && subMenus.at(i))
    subMenus.at(i)->setItemChecked (doc->hlMode(), true);
  else
    popupMenu()->setItemChecked (0, true);
}

void KateViewHighlightAction::setHl (int mode)
{
  Kate::Document *doc=m_doc;

  if (doc)
    doc->setHlMode((uint)mode);
}
//END KateViewHighlightAction

// kate: space-indent on; indent-width 2; replace-tabs on;
