/* This file is part of the KDE libraries
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

// $Id$

//BEGIN INCLUDES
//#include <string.h>
#include <qstringlist.h>

#include <qtextstream.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kinstance.h>
//#include <kmimemagic.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kregexp.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kapplication.h>

#include "katehighlight.h"
#include "katehighlight.moc"

#include "katetextline.h"
#include "katedocument.h"
#include "katesyntaxdocument.h"

#include "katefactory.h"
//END

/**
  Prviate HL classes
*/

class HlCharDetect : public HlItem {
  public:
    HlCharDetect(int attribute, int context,signed char regionId,signed char regionId2, QChar);
    virtual int checkHgl(const QString& text, int offset, int len);
  private:
    QChar sChar;
};

class Hl2CharDetect : public HlItem {
  public:
    Hl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2,  QChar ch1, QChar ch2);
   	Hl2CharDetect(int attribute, int context,signed char regionId,signed char regionId2,  const QChar *ch);

    virtual int checkHgl(const QString& text, int offset, int len);
  private:
    QChar sChar1;
    QChar sChar2;
};

class HlStringDetect : public HlItem {
  public:
    HlStringDetect(int attribute, int context, signed char regionId,signed char regionId2, const QString &, bool inSensitive=false);
    virtual ~HlStringDetect();
    virtual int checkHgl(const QString& text, int offset, int len);
  private:
    const QString str;
    bool _inSensitive;
};

class HlRangeDetect : public HlItem {
  public:
    HlRangeDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2);
    virtual int checkHgl(const QString& text, int offset, int len);
  private:
    QChar sChar1;
    QChar sChar2;
};

class HlKeyword : public HlItem
{
  public:
    HlKeyword(int attribute, int context,signed char regionId,signed char regionId2, bool casesensitive, const QString& delims);
    virtual ~HlKeyword();

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
};

class HlInt : public HlItem {
  public:
    HlInt(int attribute, int context, signed char regionId,signed char regionId2);
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlFloat : public HlItem {
  public:
    HlFloat(int attribute, int context, signed char regionId,signed char regionId2);
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlCOct : public HlItem {
  public:
    HlCOct(int attribute, int context, signed char regionId,signed char regionId2);
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlCHex : public HlItem {
  public:
    HlCHex(int attribute, int context, signed char regionId,signed char regionId2);
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlCFloat : public HlFloat {
  public:
    HlCFloat(int attribute, int context, signed char regionId,signed char regionId2);
    virtual int checkHgl(const QString& text, int offset, int len);
    int checkIntHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlLineContinue : public HlItem {
  public:
    HlLineContinue(int attribute, int context, signed char regionId,signed char regionId2);
    virtual bool endEnable(QChar c) {return c == '\0';}
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool lineContinue(){return true;}
};

class HlCStringChar : public HlItem {
  public:
    HlCStringChar(int attribute, int context, signed char regionId,signed char regionId2);
    virtual int checkHgl(const QString& text, int offset, int len);
};

class HlCChar : public HlItem {
  public:
    HlCChar(int attribute, int context,signed char regionId,signed char regionId2);
    virtual int checkHgl(const QString& text, int offset, int len);
};

class HlAnyChar : public HlItem {
  public:
    HlAnyChar(int attribute, int context, signed char regionId,signed char regionId2, const QString& charList);
    virtual int checkHgl(const QString& text, int offset, int len);
    const QString _charList;
};

class HlRegExpr : public HlItem {
  public:
  HlRegExpr(int attribute, int context,signed char regionId,signed char regionId2 ,QString expr, bool insensitive, bool minimal);
  virtual int checkHgl(const QString& text, int offset, int len);
  ~HlRegExpr(){delete Expr;};

  QRegExp *Expr;

  bool handlesLinestart;
};

//--------

//BEGIN STATICS
HlManager *HlManager::s_pSelf = 0;
KConfig *HlManager::s_pConfig =0;

// Make this configurable?
QStringList HlManager::commonSuffixes = QStringList::split(";", ".orig;.new;~;.bak;.BAK");

enum Item_styles { dsNormal,dsKeyword,dsDataType,dsDecVal,dsBaseN,dsFloat,dsChar,dsString,dsComment,dsOthers};

static bool trueBool = true;
static QString stdDeliminator = QString ("!%&()*+,-./:;<=>?[]^{|}~ \t\\");
//END

//BEGIN NON MEMBER FUNCTIONS
static int getDefStyleNum(QString name)
{
  if (name=="dsNormal") return dsNormal;
  if (name=="dsKeyword") return dsKeyword;
  if (name=="dsDataType") return dsDataType;
  if (name=="dsDecVal") return dsDecVal;
  if (name=="dsBaseN") return dsBaseN;
  if (name=="dsFloat") return dsFloat;
  if (name=="dsChar") return dsChar;
  if (name=="dsString") return dsString;
  if (name=="dsComment") return dsComment;
  if (name=="dsOthers")  return dsOthers;

  return dsNormal;
}
//END

//BEGIN HlItem
HlItem::HlItem(int attribute, int context,signed char regionId,signed char regionId2)
  : attr(attribute), ctx(context),region(regionId),region2(regionId2)  {subItems=0;
}

HlItem::~HlItem()
{
  //kdDebug(13010)<<"In hlItem::~HlItem()"<<endl;
  if (subItems!=0) {subItems->setAutoDelete(true); subItems->clear(); delete subItems;}
}

bool HlItem::startEnable(const QChar& c)
{
  // ONLY called when alwaysStartEnable() overridden
  // IN FACT not called at all, copied into doHighlight()...
  Q_ASSERT(false);
  return stdDeliminator.find(c) != 1;
}
//END

//BEGIN HLCharDetect
HlCharDetect::HlCharDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar c)
  : HlItem(attribute,context,regionId,regionId2), sChar(c) {
}

int HlCharDetect::checkHgl(const QString& text, int offset, int len) {
  if (len && text[offset] == sChar) return offset + 1;
  return 0;
}
//END

//BEGIN Hl2CharDetect
Hl2CharDetect::Hl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2)
  : HlItem(attribute,context,regionId,regionId2) {
  sChar1 = ch1;
  sChar2 = ch2;
}

int Hl2CharDetect::checkHgl(const QString& text, int offset, int len)
{
  if (len < 2) return offset;
  if (text[offset++] == sChar1 && text[offset++] == sChar2) return offset;
  return 0;
}
//END

//BEGIN HlStringDetect
HlStringDetect::HlStringDetect(int attribute, int context, signed char regionId,signed char regionId2,const QString &s, bool inSensitive)
  : HlItem(attribute, context,regionId,regionId2), str(inSensitive ? s.upper():s), _inSensitive(inSensitive) {
}

HlStringDetect::~HlStringDetect() {
}

int HlStringDetect::checkHgl(const QString& text, int offset, int len)
{
  if (len < (int)str.length())
    return 0;

  if (QConstString(text.unicode() + offset, str.length()).string().find(str, 0, !_inSensitive) == 0)
    return offset + str.length();

  return 0;
}

//END

//BEGIN HLRangeDetect
HlRangeDetect::HlRangeDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2)
  : HlItem(attribute,context,regionId,regionId2) {
  sChar1 = ch1;
  sChar2 = ch2;
}

int HlRangeDetect::checkHgl(const QString& text, int offset, int len)
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

//BEGIN HlKeyword
HlKeyword::HlKeyword (int attribute, int context, signed char regionId,signed char regionId2, bool casesensitive, const QString& delims)
  : HlItem(attribute,context,regionId,regionId2)
  , dict (113, casesensitive)
  , _caseSensitive(casesensitive)
  , deliminators(delims)
{
}

HlKeyword::~HlKeyword() {
}

bool HlKeyword::alwaysStartEnable() const
{
  return false;
}

bool HlKeyword::hasCustomStartEnable() const
{
  return true;
}

bool HlKeyword::startEnable(const QChar& c)
{
  return deliminators.find(c) != -1;
}

// If we use a dictionary for lookup we don't really need
// an item as such we are using the key to lookup
void HlKeyword::addWord(const QString &word)
{
  dict.insert(word,&trueBool);
}

void HlKeyword::addList(const QStringList& list)
{
  for(uint i=0;i<list.count();i++) dict.insert(list[i], &trueBool);
}

int HlKeyword::checkHgl(const QString& text, int offset, int len)
{
  if (len == 0 || dict.isEmpty()) return 0;

  int offset2 = offset;

  while (len > 0 && deliminators.find(text[offset2]) == -1 )
  {
    offset2++;
    len--;
  }

  if (offset2 == offset) return 0;

  QString lookup = text.mid(offset, offset2 - offset);

  if ( dict.find(lookup) ) return offset2;

  return 0;
}
//END

//BEGIN HlInt
HlInt::HlInt(int attribute, int context, signed char regionId,signed char regionId2)
  : HlItem(attribute,context,regionId,regionId2)
{
}

bool HlInt::alwaysStartEnable() const
{
  return false;
}

int HlInt::checkHgl(const QString& text, int offset, int len)
{
  int offset2 = offset;

  while ((len > 0) && text[offset2].isDigit())
  {
    offset2++;
    len--;
  }

  if (offset2 > offset)
  {
    if (subItems)
    {
      for (HlItem *it = subItems->first(); it; it = subItems->next())
      {
        offset = it->checkHgl(text, offset2, len);
        if (offset) return offset;
      }
    }

    return offset2;
  }

  return 0;
}
//END

//BEGIN HlFloat
HlFloat::HlFloat(int attribute, int context, signed char regionId,signed char regionId2)
  : HlItem(attribute,context, regionId,regionId2) {
}

bool HlFloat::alwaysStartEnable() const
{
  return false;
}

int HlFloat::checkHgl(const QString& text, int offset, int len)
{
  bool b, p;

  b = false;
  p = false;

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
      if (subItems)
      {
        for (HlItem *it = subItems->first(); it; it = subItems->next())
        {
          int offset2 = it->checkHgl(text, offset, len);

          if (offset2)
            return offset2;
        }
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
    if (subItems)
    {
      for (HlItem *it = subItems->first(); it; it = subItems->next())
      {
        int offset2 = it->checkHgl(text, offset, len);

        if (offset2)
          return offset2;
      }
    }

    return offset;
  }

  return 0;
}
//END

//BEGIN HlCOct
HlCOct::HlCOct(int attribute, int context, signed char regionId,signed char regionId2)
  : HlItem(attribute,context,regionId,regionId2) {
}

bool HlCOct::alwaysStartEnable() const
{
  return false;
}

int HlCOct::checkHgl(const QString& text, int offset, int len)
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

//BEGIN HlCHex
HlCHex::HlCHex(int attribute, int context,signed char regionId,signed char regionId2)
  : HlItem(attribute,context,regionId,regionId2) {
}

bool HlCHex::alwaysStartEnable() const
{
  return false;
}

int HlCHex::checkHgl(const QString& text, int offset, int len)
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

HlCFloat::HlCFloat(int attribute, int context, signed char regionId,signed char regionId2)
  : HlFloat(attribute,context,regionId,regionId2) {
}

bool HlCFloat::alwaysStartEnable() const
{
  return false;
}

int HlCFloat::checkIntHgl(const QString& text, int offset, int len)
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

int HlCFloat::checkHgl(const QString& text, int offset, int len)
{
  int offset2 = HlFloat::checkHgl(text, offset, len);

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

HlAnyChar::HlAnyChar(int attribute, int context, signed char regionId,signed char regionId2, const QString& charList)
  : HlItem(attribute, context,regionId,regionId2)
  , _charList(charList)
{
}

int HlAnyChar::checkHgl(const QString& text, int offset, int len)
{
  if ((len > 0) && _charList.find(text[offset]) != -1)
    return ++offset;

  return 0;
}

HlRegExpr::HlRegExpr( int attribute, int context, signed char regionId,signed char regionId2, QString regexp, bool insensitive, bool minimal )
  : HlItem(attribute, context, regionId,regionId2)
{
    handlesLinestart=regexp.startsWith("^");
    if(!handlesLinestart) regexp.prepend("^");
    Expr=new QRegExp(regexp, !insensitive);
    Expr->setMinimal(minimal);
}

int HlRegExpr::checkHgl(const QString& text, int offset, int /*len*/)
{
  if (offset && handlesLinestart)
    return 0;

  int offset2 = Expr->search( text, offset, QRegExp::CaretAtOffset );

  if (offset2 == -1) return 0;

  return (offset + Expr->matchedLength());
};

HlLineContinue::HlLineContinue(int attribute, int context, signed char regionId,signed char regionId2)
  : HlItem(attribute,context,regionId,regionId2) {
}

int HlLineContinue::checkHgl(const QString& text, int offset, int len)
{
  if ((len == 1) && (text[offset] == '\\'))
    return ++offset;

  return 0;
}

HlCStringChar::HlCStringChar(int attribute, int context,signed char regionId,signed char regionId2)
  : HlItem(attribute,context,regionId,regionId2) {
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

int HlCStringChar::checkHgl(const QString& text, int offset, int len)
{
  return checkEscapedChar(text, offset, len);
}

HlCChar::HlCChar(int attribute, int context,signed char regionId,signed char regionId2)
  : HlItem(attribute,context,regionId,regionId2) {
}

int HlCChar::checkHgl(const QString& text, int offset, int len)
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

ItemData::ItemData(const QString  name, int defStyleNum)
  : name(name), defStyleNum(defStyleNum), defStyle(true) {
}

HlData::HlData(const QString &wildcards, const QString &mimetypes, const QString &identifier)
  : wildcards(wildcards), mimetypes(mimetypes), identifier(identifier) {

//JW  itemDataList.setAutoDelete(true);
}

HlContext::HlContext (int attribute, int lineEndContext, int _lineBeginContext, bool _fallthrough, int _fallthroughContext)
{
  attr = attribute;
  ctx = lineEndContext;
  lineBeginContext = _lineBeginContext;
  fallthrough = _fallthrough;
  ftctx = _fallthroughContext;
}

Hl2CharDetect::Hl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2, const QChar *s)
  : HlItem(attribute,context,regionId,regionId2) {
  sChar1 = s[0];
  sChar2 = s[1];
}

Highlight::Highlight(const syntaxModeListItem *def) : refCount(0)
{
  errorsAndWarnings="";
  building=false;
  noHl = false;
  folding=false;
  internalIDList.setAutoDelete(true);

  if (def == 0)
  {
    noHl = true;
    iName = I18N_NOOP("Normal");
    iSection = "";
  }
  else
  {
    iName = def->name;
    iSection = def->section;
    iWildcards = def->extension;
    iMimetypes = def->mimetype;
    identifier = def->identifier;
    iVersion=def->version;
  }
  deliminator = stdDeliminator;
}

Highlight::~Highlight()
{
    contextList.setAutoDelete( true );
}

void Highlight::generateContextStack(int *ctxNum, int ctx, QMemArray<uint>* ctxs, int *prevLine, bool lineContinue)
{
  //kdDebug(13010)<<QString("Entering generateContextStack with %1").arg(ctx)<<endl;

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

    ctxs->resize (ctxs->size()+1);
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
          ctxs->truncate (ctxs->size()-1);
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
          generateContextStack(ctxNum, contextNum((*ctxs)[ctxs->size()-1])->ctx,ctxs, prevLine);
          return;
        }
      }
    }
    else
    {
      if (ctx == -1)
        (*ctxNum)=( (ctxs->isEmpty() ) ? 0 : (*ctxs)[ctxs->size()-1]);
    }
  }
}

/*******************************************************************************************
        Highlight - doHighlight
        Increase the usage count and trigger initialization if needed

                        * input: signed char *oCtx	Pointer to the "stack" of the previous line
				 uint *oCtxLen		Size of the stack
				 TextLine *textline	Current textline to work on
                        *************
                        * output: (TextLine *textline)
                        *************
                        * return value: signed char*	new context stack at the end of the line
*******************************************************************************************/

void Highlight::doHighlight(QMemArray<uint> oCtx, TextLine *textLine,bool lineContinue,
				QMemArray<signed char>* foldingList)
{
  if (!textLine)
    return;

  if (noHl)
  {
    textLine->setAttribs(0,0,textLine->length());
    return;
  }

//  kdDebug(13010)<<QString("The context stack length is: %1").arg(oCtx.size())<<endl;

  HlContext *context;
  HlItem *item=0;

  // if (lineContinue) kdDebug(13010)<<"Entering with lineContinue flag set"<<endl;

  int ctxNum;
  int prevLine;

  QMemArray<uint> ctx;
  ctx.duplicate (oCtx);

  if ( oCtx.isEmpty() )
  {
    // If the stack is empty, we assume to be in Context 0 (Normal)
    ctxNum=0;
    context=contextNum(ctxNum);
    prevLine=-1;
  }
  else
  {
    //  kdDebug(13010)<<"test1-2-1"<<endl;

     /*QString tmpStr="";
     for (int jwtest=0;jwtest<oCtx.size();jwtest++)
	tmpStr=tmpStr+QString("%1 ,").arg(ctx[jwtest]);
    kdDebug(13010)<< "Old Context stack:" << tmpStr<<endl; */

    // There does an old context stack exist -> find the context at the line start
    ctxNum=ctx[oCtx.size()-1]; //context ID of the last character in the previous line

    //kdDebug(13010)<<"test1-2-1-text1"<<endl;

    //kdDebug(13010) << "\t\tctxNum = " << ctxNum << " contextList[ctxNum] = " << contextList[ctxNum] << endl; // ellis

    //if (lineContinue)   kdDebug(13010)<<QString("The old context should be %1").arg((int)ctxNum)<<endl;

    if (contextNum(ctxNum))
      context=contextNum(ctxNum); //context structure
    else
      context = contextNum(0);

    //kdDebug(13010)<<"test1-2-1-text2"<<endl;

    prevLine=oCtx.size()-1;	//position of the last context ID of th previous line within the stack

    //kdDebug(13010)<<"test1-2-1-text3"<<endl;
    generateContextStack(&ctxNum, context->ctx, &ctx, &prevLine,lineContinue);	//get stack ID to use

    //kdDebug(13010)<<"test1-2-1-text4"<<endl;

    context=contextNum(ctxNum);	//current context to use
    //kdDebug(13010)<<"test1-2-2"<<endl;

    //if (lineContinue)   kdDebug(13010)<<QString("The new context is %1").arg((int)ctxNum)<<endl;
  }

  QChar lastChar = ' ';

  // text, for programming convenience :)
  const QString& text = textLine->string();

  // non space char - index of that char
//  const QChar *s1 = textLine->firstNonSpace();
  int offset1 = 0, offset2 = 0;
  uint z=0;
//  uint z = textLine->firstChar();

  // length of textline
  uint len = textLine->length();

  bool found = false;

  //kdDebug() << k_funcinfo << len << " z " << z << endl;

  while (z < len)
  {
    //kdDebug() << "offset1" << offset1 << " offset2 " << offset2 << endl;

    found = false;

    bool standardStartEnableDetermined = false;
    bool standardStartEnable = false;

    for (item = context->items.first(); item != 0L; item = context->items.next())
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
          standardStartEnable = stdDeliminator.find(lastChar) != 1;
          standardStartEnableDetermined = true;
        }

        thisStartEnabled = standardStartEnable;
      }
      else if (item->startEnable(lastChar))
      {
        thisStartEnabled = true;
      }

      if (thisStartEnabled)
      {
        offset2 = item->checkHgl(text, offset1, len-z);

        if (offset2 > offset1)
        {
          textLine->setAttribs(item->attr,offset1,offset2);
          //kdDebug(13010)<<QString("item->ctx: %1").arg(item->ctx)<<endl;

          if (item->region)
          {
//              kdDebug(13010)<<QString("Region mark detected: %1").arg(item->region)<<endl;

            if ( !foldingList->isEmpty() && ((item->region < 0) && (*foldingList)[foldingList->size()-1] == -item->region ) )
            {
              foldingList->resize (foldingList->size()-1);
            }
            else
            {
              foldingList->resize (foldingList->size()+1);
              (*foldingList)[foldingList->size()-1] = item->region;
            }

          }

          if (item->region2)
          {
//              kdDebug(13010)<<QString("Region mark 2 detected: %1").arg(item->region2)<<endl;

            if ( !foldingList->isEmpty() && ((item->region2 < 0) && (*foldingList)[foldingList->size()-1] == -item->region2 ) )
            {
              foldingList->resize (foldingList->size()-1);
            }
            else
            {
              foldingList->resize (foldingList->size()+1);
              (*foldingList)[foldingList->size()-1] = item->region2;
            }

          }

          generateContextStack(&ctxNum, item->ctx, &ctx, &prevLine);  //regenerate context stack

      //kdDebug(13010)<<QString("generateContextStack has been left in item loop, size: %1").arg(ctx.size())<<endl;
    //    kdDebug(13010)<<QString("current ctxNum==%1").arg(ctxNum)<<endl;

          context=contextNum(ctxNum);

          z = z + offset2 - offset1 - 1;
          offset1 = offset2 - 1;
          found = true;
          break;
        }
      }
    }

    lastChar = text[offset1];

    // nothing found: set attribute of one char
    // anders: unless this context does not want that!
    if (!found) {
      if ( context->fallthrough ) {
        // set context to context->ftctx.
        generateContextStack(&ctxNum, context->ftctx, &ctx, &prevLine);  //regenerate context stack
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
      else {
        textLine->setAttribs(context->attr,offset1,offset1 + 1);
      }
    }

    offset1++;
    z++;
  }

  if (item==0)
    textLine->setHlLineContinue(false);
  else {
    textLine->setHlLineContinue(item->lineContinue());
    if (item->lineContinue()) kdDebug(13010)<<"Setting line continue flag"<<endl;
  }

//  if (oCtxLen>0)
//  kdDebug(13010)<<QString("Last line end context entry: %1").arg((int)ctx[oCtxLen-1])<<endl;
//  else kdDebug(13010)<<QString("Context stack len:0")<<endl;

  textLine->setContext(ctx.data(), ctx.size());

  //kdDebug(13010)<<QString("Context stack size on exit :%1").arg(ctx.size())<<endl;
}

KConfig *Highlight::getKConfig() {
  KConfig *config;

  config = HlManager::getKConfig();//KateFactory::instance()->config();
  config->setGroup(iName + QString(" Highlight"));
  return config;
}

QString Highlight::getWildcards() {
  //if wildcards not yet in config, then use iWildCards as default
  return getKConfig()->readEntry("Wildcards", iWildcards);
}

QString Highlight::getMimetypes() {
  return getKConfig()->readEntry("Mimetypes", iMimetypes);
}

HlData *Highlight::getData() {
  KConfig *config;
  HlData *hlData;

  config = getKConfig();

//  iWildcards = config->readEntry("Wildcards");
//  iMimetypes = config->readEntry("Mimetypes");
//  hlData = new HlData(iWildcards,iMimetypes);
  hlData = new HlData(
    config->readEntry("Wildcards", iWildcards),
    config->readEntry("Mimetypes", iMimetypes),
    config->readEntry("Identifier", identifier));
  getItemDataList(hlData->itemDataList, config);
  return hlData;
}

void Highlight::setData(HlData *hlData) {
  KConfig *config;

  config = getKConfig();

//  iWildcards = hlData->wildcards;
//  iMimetypes = hlData->mimetypes;

  config->writeEntry("Wildcards",hlData->wildcards);
  config->writeEntry("Mimetypes",hlData->mimetypes);

  setItemDataList(hlData->itemDataList,config);

  config->sync ();
}

void Highlight::getItemDataList(ItemDataList &list) {
  getItemDataList(list, getKConfig());
}

void Highlight::getItemDataList(ItemDataList &list, KConfig *config) {
  ItemData *p;
  QString s;
  QRgb col, selCol;

  list.clear();
//JW  list.setAutoDelete(true);
  createItemData(list);

  for (p = list.first(); p != 0L; p = list.next()) {
    s = config->readEntry(p->name);
    if (!s.isEmpty()) {
      int bold, italic;
      sscanf(s.latin1(),"%d,%X,%X,%d,%d", &p->defStyle,&col,&selCol,&bold,&italic);
      QColor color = p->textColor();
      color.setRgb(col);
      p->setTextColor(color);

      QColor selColor = p->selectedTextColor();
      selColor.setRgb(selCol);
      p->setSelectedTextColor(selColor);

      p->setBold(bold);
      p->setItalic(italic);
    }
  }
}

/*******************************************************************************************
        Highlight - setItemDataList
        saves the ItemData / attribute / style definitions to the apps configfile.
        Especially needed for user overridden values.

                        * input: ItemDataList &list             :reference to the list, whose
                        *                                        items should be saved
                        *        KConfig *config                :Pointer KDE configuration
                        *                                        class, which should be used
                        *                                        as storage
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::setItemDataList(ItemDataList &list, KConfig *config) {
  ItemData *p;
  QString s;

  for (p = list.first(); p != 0L; p = list.next()) {
    s.sprintf("%d,%X,%X,%d,%d",
      p->defStyle,p->textColor().rgb(),p->selectedTextColor().rgb(),p->bold(),p->italic());
    config->writeEntry(p->name,s);
  }
}

/*******************************************************************************************
        Highlight - use
        Increase the usage count and trigger initialization if needed

                        * input: none
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::use()
{
  if (refCount == 0) init();
  refCount++;
}

/*******************************************************************************************
        Highlight - release
        Decrease the usage count and trigger a cleanup if needed

                        * input: none
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::release()
{
  refCount--;
  if (refCount == 0) done();
}

/*******************************************************************************************
        Highlight - init
        If it's the first time a particular highlighting is used create the needed contextlist

                        * input: none
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::init()
{
  if (noHl)
    return;

  contextList.clear ();
  makeContextList();
}


/*******************************************************************************************
        Highlight - done
        If the there is no document using the highlighting style free the complete context
        structure.

                        * input: none
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::done()
{
  if (noHl)
    return;

  contextList.clear ();
}

HlContext *Highlight::contextNum (uint n)
{
  return contextList[n];
}

/*******************************************************************************************
        Highlight - createItemData
        This function reads the itemData entries from the config file, which specifies the
        default attribute styles for matched items/contexts.

                        * input: none
                        *************
                        * output: ItemDataList &list            :A reference to the internal
                                                                list containing the parsed
                                                                default config
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::createItemData(ItemDataList &list)
{
  // If no highlighting is selected we need only one default.
  if (noHl)
  {
     list.append(new ItemData(I18N_NOOP("Normal Text"), dsNormal));
     return;
  }

  // If the internal list isn't already available read the config file
  if (internalIDList.isEmpty())
    makeContextList();

  list=internalIDList;
}


void Highlight::addToItemDataList()
  {
    syntaxContextData *data;
    QString color;
    QString selColor;
    QString bold;
    QString italic;

    //Tell the syntax document class which file we want to parse and which data group
    HlManager::self()->syntax->setIdentifier(buildIdentifier);
    data=HlManager::self()->syntax->getGroupInfo("highlighting","itemData");
    //begin with the real parsing
    while (HlManager::self()->syntax->nextGroup(data))
      {
        // read all attributes
        color=HlManager::self()->syntax->groupData(data,QString("color"));
        selColor=HlManager::self()->syntax->groupData(data,QString("selColor"));
        bold=HlManager::self()->syntax->groupData(data,QString("bold"));
        italic=HlManager::self()->syntax->groupData(data,QString("italic"));
        //check if the user overrides something
        if ( (!color.isEmpty()) && (!selColor.isEmpty()) && (!bold.isEmpty()) && (!italic.isEmpty()))
                {
                        //create a user defined style
                        ItemData* newData = new ItemData(
                                buildPrefix+HlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace(),
                                getDefStyleNum(HlManager::self()->syntax->groupData(data,QString("defStyleNum"))));

                        newData->setTextColor(QColor(color));
                        newData->setSelectedTextColor(QColor(selColor));
                        newData->setBold(bold=="true" || bold=="1");
                        newData->setItalic(italic=="true" || italic=="1");

                        internalIDList.append(newData);
                }
        else
                {
                        //assign a default style
                        internalIDList.append(new ItemData(
                                buildPrefix+HlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace(),
                                getDefStyleNum(HlManager::self()->syntax->groupData(data,QString("defStyleNum")))));

                }
      }
    //clean up
    if (data) HlManager::self()->syntax->freeGroupInfo(data);
  }

/*******************************************************************************************
        Highlight - lookupAttrName
        This function is  a helper for makeContextList and createHlItem. It looks the given
        attribute name in the itemData list up and returns it's index

                        * input: QString &name                  :the attribute name to lookup
                        *        ItemDataList &iDl               :the list containing all
                        *                                         available attributes
                        *************
                        * output: none
                        *************
                        * return value: int                     :The index of the attribute
                        *                                        or 0
*******************************************************************************************/

int  Highlight::lookupAttrName(const QString& name, ItemDataList &iDl)
{
  for (uint i = 0; i < iDl.count(); i++)
    if (iDl.at(i)->name == buildPrefix+name)
      return i;

  kdDebug(13010)<<"Couldn't resolve itemDataName"<<endl;
  return 0;
}

/*******************************************************************************************
        Highlight - createHlItem
        This function is  a helper for makeContextList. It parses the xml file for
        information, how single or multi line comments are marked

                        * input: syntaxContextData *data : Data about the item read from
                        *                                  the xml file
                        *        ItemDataList &iDl :       List of all available itemData
                        *                                   entries. Needed for attribute
                        *                                   name->index translation
			*	 QStringList *RegionList	: list of code folding region names
			*	 QStringList ContextList	: list of context names
                        *************
                        * output: none
                        *************
                        * return value: HlItem * :          Pointer to the newly created item
                        *                                   object
*******************************************************************************************/

HlItem *Highlight::createHlItem(syntaxContextData *data, ItemDataList &iDl,QStringList *RegionList, QStringList *ContextNameList)
{
  // No highlighting -> exit
  if (noHl)
    return 0;

  // get the (tagname) itemd type
  QString dataname=HlManager::self()->syntax->groupItemData(data,QString(""));

  // BEGIN - Translation of the attribute parameter
  QString tmpAttr=HlManager::self()->syntax->groupItemData(data,QString("attribute")).simplifyWhiteSpace();
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
  QString tmpcontext=HlManager::self()->syntax->groupItemData(data,QString("context"));


  QString unresolvedContext;
  context=getIdFromString(ContextNameList, tmpcontext,unresolvedContext);

  // Get the char parameter (eg DetectChar)
  char chr;
  if (! HlManager::self()->syntax->groupItemData(data,QString("char")).isEmpty())
    chr= (HlManager::self()->syntax->groupItemData(data,QString("char")).latin1())[0];
  else
    chr=0;

  // Get the String parameter (eg. StringDetect)
  QString stringdata=HlManager::self()->syntax->groupItemData(data,QString("String"));

  // Get a second char parameter (char1) (eg Detect2Chars)
  char chr1;
  if (! HlManager::self()->syntax->groupItemData(data,QString("char1")).isEmpty())
    chr1= (HlManager::self()->syntax->groupItemData(data,QString("char1")).latin1())[0];
  else
    chr1=0;

  // Will be removed eventuall. Atm used for StringDetect
  bool insensitive=( HlManager::self()->syntax->groupItemData(data,QString("insensitive")).lower() == QString("true") );
  // anders: very resonable for regexp too!

  // for regexp only
  bool minimal = ( HlManager::self()->syntax->groupItemData(data,QString("minimal")).lower() == QString("true") );


  // code folding region handling:
  QString beginRegionStr=HlManager::self()->syntax->groupItemData(data,QString("beginRegion"));
  QString endRegionStr=HlManager::self()->syntax->groupItemData(data,QString("endRegion"));

  signed char regionId=0;
  signed char regionId2=0;

  signed char *regionIdToSet=&regionId;
  if (!endRegionStr.isEmpty())
  {
    regionId=RegionList->findIndex(endRegionStr);
    if (regionId==-1) // if the region name doesn't already exist, add it to the list
    {
      (*RegionList)<<endRegionStr;
      regionId=RegionList->findIndex(endRegionStr);
    }
    regionId=-regionId;
    regionIdToSet=&regionId2;
  }

  if (!beginRegionStr.isEmpty())
  {
    *regionIdToSet=RegionList->findIndex(beginRegionStr);
    if (*regionIdToSet==-1) // if the region name doesn't already exist, add it to the list
    {
      (*RegionList)<<beginRegionStr;
      *regionIdToSet=RegionList->findIndex(beginRegionStr);
    }
//    *regionIdToSet=-(*regionIdToSet);
  }

                //Create the item corresponding to it's type and set it's parameters
  HlItem *tmpItem;

  if (dataname=="keyword")
  {
    HlKeyword *keyword=new HlKeyword(attr,context,regionId,regionId2,casesensitive,
      deliminator);

    //Get the entries for the keyword lookup list
    keyword->addList(HlManager::self()->syntax->finddata("highlighting",stringdata));
    tmpItem=keyword;
  } else
    if (dataname=="Float") tmpItem= (new HlFloat(attr,context,regionId,regionId2)); else
    if (dataname=="Int") tmpItem=(new HlInt(attr,context,regionId,regionId2)); else
    if (dataname=="DetectChar") tmpItem=(new HlCharDetect(attr,context,regionId,regionId2,chr)); else
    if (dataname=="Detect2Chars") tmpItem=(new Hl2CharDetect(attr,context,regionId,regionId2,chr,chr1)); else
    if (dataname=="RangeDetect") tmpItem=(new HlRangeDetect(attr,context,regionId,regionId2, chr, chr1)); else
    if (dataname=="LineContinue") tmpItem=(new HlLineContinue(attr,context,regionId,regionId2)); else
    if (dataname=="StringDetect") tmpItem=(new HlStringDetect(attr,context,regionId,regionId2,stringdata,insensitive)); else
    if (dataname=="AnyChar") tmpItem=(new HlAnyChar(attr,context,regionId,regionId2,stringdata)); else
    if (dataname=="RegExpr") tmpItem=(new HlRegExpr(attr,context,regionId,regionId2,stringdata, insensitive, minimal)); else
    if (dataname=="HlCChar") tmpItem= ( new HlCChar(attr,context,regionId,regionId2));else
    if (dataname=="HlCHex") tmpItem= (new HlCHex(attr,context,regionId,regionId2));else
    if (dataname=="HlCOct") tmpItem= (new HlCOct(attr,context,regionId,regionId2)); else
    if (dataname=="HlCFloat") tmpItem= (new HlCFloat(attr,context,regionId,regionId2)); else
    if (dataname=="HlCStringChar") tmpItem= (new HlCStringChar(attr,context,regionId,regionId2)); else

  {
    // oops, unknown type. Perhaps a spelling error in the xml file
    return 0;
  }

  if (!unresolvedContext.isEmpty())
  {
    unresolvedContextReferences.insert(&(tmpItem->ctx),unresolvedContext);
  }
  return tmpItem;
}


/*******************************************************************************************
        Highlight - isInWord

                        * input: Qchar c       Character to investigate
                        *************
                        * output: none
                        *************
                        * return value: returns true, if c is no deliminator
*******************************************************************************************/

bool Highlight::isInWord(QChar c)
{
  const QString sq("\"'");
  //const QChar *q = sq.unicode();
  //return !ustrchr(deliminatorChars, deliminatorLen, c) && !ustrchr( q, 2, c);
  return deliminator.find(c) == -1 && sq.find(c) == -1;
}

/*******************************************************************************************
        Highlight - readCommentConfig
        This function is  a helper for makeContextList. It parses the xml file for
        information, how single or multi line comments are marked

                        * input: none
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::readCommentConfig()
{

  cslStart = "";
  HlManager::self()->syntax->setIdentifier(buildIdentifier);

  syntaxContextData *data=HlManager::self()->syntax->getGroupInfo("general","comment");
  if (data)
    {
//      kdDebug(13010)<<"COMMENT DATA FOUND"<<endl;
    while  (HlManager::self()->syntax->nextGroup(data))
      {

        if (HlManager::self()->syntax->groupData(data,"name")=="singleLine")
		cslStart=HlManager::self()->syntax->groupData(data,"start");
	if (HlManager::self()->syntax->groupData(data,"name")=="multiLine")
           {
		cmlStart=HlManager::self()->syntax->groupData(data,"start");
		cmlEnd=HlManager::self()->syntax->groupData(data,"end");
           }
      }
    HlManager::self()->syntax->freeGroupInfo(data);
    }

}

/*******************************************************************************************
        Highlight - readGlobalKeyWordConfig
        This function is  a helper for makeContextList. It parses the xml file for
        information, if keywords should be treated case(in)sensitive and creates the keyword
        delimiter list. Which is the default list, without any given weak deliminiators

                        * input: none
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/


void Highlight::readGlobalKeywordConfig()
{
  // Tell the syntax document class which file we want to parse
  kdDebug(13010)<<"readGlobalKeywordConfig:BEGIN"<<endl;
  HlManager::self()->syntax->setIdentifier(buildIdentifier);

  // Get the keywords config entry
  syntaxContextData * data=HlManager::self()->syntax->getConfig("general","keywords");
  if (data)
    {
	kdDebug(13010)<<"Found global keyword config"<<endl;

        if (HlManager::self()->syntax->groupItemData(data,QString("casesensitive"))!="0")
		casesensitive=true; else {casesensitive=false; kdDebug(13010)<<"Turning on case insensitiveness"<<endl;}
     //get the weak deliminators
     weakDeliminator=(HlManager::self()->syntax->groupItemData(data,QString("weakDeliminator")));

     kdDebug(13010)<<"weak delimiters are: "<<weakDeliminator<<endl;
     // remove any weakDelimitars (if any) from the default list and store this list.
     int f;
     for (uint s=0; s < weakDeliminator.length(); s++)
     {
        f = 0;
        f = deliminator.find (weakDeliminator[s]);

        if (f > -1)
          deliminator.remove (f, 1);
     }

     QString addDelim=(HlManager::self()->syntax->groupItemData(data,QString("additionalDeliminator")));
     if (!addDelim.isEmpty()) deliminator=deliminator+addDelim;
//      deliminatorChars = deliminator.unicode();
//      deliminatorLen = deliminator.length();


	HlManager::self()->syntax->freeGroupInfo(data);
    }
  else
    {
       //Default values
       casesensitive=true;
       weakDeliminator=QString("");
    }


  kdDebug(13010)<<"readGlobalKeywordConfig:END"<<endl;

  kdDebug(13010)<<"delimiterCharacters are: "<<deliminator<<endl;
}

void  Highlight::createContextNameList(QStringList *ContextNameList,int ctx0)
{
  syntaxContextData *data;

  kdDebug(13010)<<"creatingContextNameList:BEGIN"<<endl;

  if (ctx0 == 0)
      ContextNameList->clear();

  HlManager::self()->syntax->setIdentifier(buildIdentifier);

  data=HlManager::self()->syntax->getGroupInfo("highlighting","context");

  int id=ctx0;

  if (data)
  {
     while (HlManager::self()->syntax->nextGroup(data))
     {
          QString tmpAttr=HlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace();
	  if (tmpAttr.isEmpty())
	  {
		 tmpAttr=QString("!KATE_INTERNAL_DUMMY! %1").arg(id);
		 errorsAndWarnings +=i18n("<B>%1</B>: Deprecated syntax. Context %2 has no symbolic name<BR>").arg(buildIdentifier).arg(id-ctx0);
	  }
          else tmpAttr=buildPrefix+tmpAttr;
	  (*ContextNameList)<<tmpAttr;
          id++;
     }
     HlManager::self()->syntax->freeGroupInfo(data);
  }
  kdDebug(13010)<<"creatingContextNameList:END"<<endl;

}

int Highlight::getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext, /*NO CONST*/ QString &unres)
{
  unres="";
  int context;
  if ((tmpLineEndContext=="#stay") || (tmpLineEndContext.simplifyWhiteSpace().isEmpty())) context=-1;
      else if (tmpLineEndContext.startsWith("#pop"))
      {
           context=-1;
           for(;tmpLineEndContext.startsWith("#pop");context--)
           {
               tmpLineEndContext.remove(0,4);
               kdDebug(13010)<<"#pop found"<<endl;
           }
      }
      else
	if ( tmpLineEndContext.startsWith("##"))
	{
		QString tmp=tmpLineEndContext.right(tmpLineEndContext.length()-2);
		if (!embeddedHls.contains(tmp))	embeddedHls.insert(tmp,EmbeddedHlInfo());
		unres=tmp;
		context=0;
	}
	else
	{
		context=ContextNameList->findIndex(buildPrefix+tmpLineEndContext);
		if (context==-1)
		{
			context=tmpLineEndContext.toInt();
			errorsAndWarnings+=i18n("<B>%1</B>:Deprecated syntax. Context %2 not addressed by a symbolic name").arg(buildIdentifier).arg(tmpLineEndContext);
		}
//#warning restructure this the name list storage.
//		context=context+buildContext0Offset;
	}
  return context;
}

/*******************************************************************************************
        Highlight - makeContextList
        That's the most important initialization function for each highlighting. It's called
        each time a document gets a highlighting style assigned. parses the xml file and
        creates a corresponding internal structure

                        * input: none
                        *************
                        * output: none
                        *************
                        * return value: none
*******************************************************************************************/

void Highlight::makeContextList()
{
  if (noHl)	// if this a highlighting for "normal texts" only, tere is no need for a context list creation
    return;

  embeddedHls.clear();
  unresolvedContextReferences.clear();
  RegionList.clear();
  ContextNameList.clear();

  // prepare list creation. To reuse as much code as possible handle this highlighting the same way as embedded onces
  embeddedHls.insert(iName,EmbeddedHlInfo());

  bool something_changed;
  int startctx=0;	// the context "0" id is 0 for this hl, all embedded context "0"s have offsets
  building=true;	// inform everybody that we are building the highlighting contexts and itemlists
  do
  {
	kdDebug(13010)<<"**************** Outter loop in make ContextList"<<endl;
	kdDebug(13010)<<"**************** Hl List count:"<<embeddedHls.count()<<endl;
	something_changed=false; //assume all "embedded" hls have already been loaded
	for (EmbeddedHlInfos::const_iterator it=embeddedHls.begin(); it!=embeddedHls.end();++it)
	{
		if (!it.data().loaded)	// we found one, we still have to load
		{
			kdDebug(13010)<<"**************** Inner loop in make ContextList"<<endl;
			QString identifierToUse;
			kdDebug(13010)<<"Trying to open highlighting definition file: "<< it.key()<<endl;
			if (iName==it.key()) identifierToUse=identifier;	// the own identifier is known
			else
				identifierToUse=HlManager::self()->identifierForName(it.key()); // all others have to be looked up

			kdDebug(13010)<<"Location is:"<< identifierToUse<<endl;

			buildPrefix=it.key()+':';	// attribute names get prefixed by the names of the highlighting definitions they belong to

			if (identifierToUse.isEmpty() ) kdDebug()<<"OHOH, unknown highlighting description referenced"<<endl;

			kdDebug()<<"setting ("<<it.key()<<") to loaded"<<endl;
			it=embeddedHls.insert(it.key(),EmbeddedHlInfo(true,startctx)); //mark hl as loaded
			buildContext0Offset=startctx;	//set class member for context 0 offset, so we don't need to pass it around
			startctx=addToContextList(identifierToUse,startctx);	//parse one hl definition file
			if (noHl) return;	// an error occured
			something_changed=true; // something has been loaded

		}
	}
  } while (something_changed);	// as long as there has been another file parsed repeat everything, there could be newly added embedded hls.


  /* at this point all needed highlighing (sub)definitions are loaded. It's time to resolve cross file
     references (if there are some
  */
  kdDebug(13010)<<"Unresolved contexts, which need attention: "<<unresolvedContextReferences.count()<<endl;
//optimize this a littlebit
  for (UnresolvedContextReferences::iterator unresIt=unresolvedContextReferences.begin();
		unresIt!=unresolvedContextReferences.end();++unresIt)
	{
		//try to find the context0 id for a given unresolvedReference
		EmbeddedHlInfos::const_iterator hlIt=embeddedHls.find(unresIt.data());
		if (hlIt!=embeddedHls.end())
			*(unresIt.key())=hlIt.data().context0;
	}

	/*eventually handle IncludeRules items, if they exist.
		This has to be done after the cross file references, because it is allowed
		to include the context0 from a different definition, than the one the rule belongs to */
	handleIncludeRules();

	embeddedHls.clear(); //save some memory.
	unresolvedContextReferences.clear(); //save some memory
	RegionList.clear();	// I think you get the idea ;)
	ContextNameList.clear();


// if there have been errors show them
	if (!errorsAndWarnings.isEmpty())
	KMessageBox::detailedSorry(0L,i18n("There were warning(s) and/or error(s) while parsing the syntax highlighting configuration."), errorsAndWarnings, i18n("Kate Syntax Highlight Parser"));

// we have finished
  building=false;
}

void Highlight::handleIncludeRules()
{

  // if there are noe include rules to take care of, just return
  kdDebug(13010)<<"IncludeRules, which need attention: " <<includeRules.count()<<endl;
  if (includeRules.isEmpty()) return;

  buildPrefix="";
  QString dummy;

  /*by now the context0 references are resolved, now more or less only inner file references are resolved.
	If we decide that arbitrary inclusion is needed, this doesn't need to be changed, only the addToContextList
	method
   */

  //resolove context names
  for (IncludeRules::iterator it=includeRules.begin();it!=includeRules.end();)
  {

	if ((*it)->incCtx==-1) // context unresolved ?
	{ //yes

		if ((*it)->incCtxN.isEmpty())
		{
			// no context name given, and no valid context id set, so this item is going to be removed
			IncludeRules::iterator it1=it;
			++it1;
			delete (*it);
			includeRules.remove(it);
			it=it1;
		}
		else
		{
			// resolve name to id
			(*it)->incCtx=getIdFromString(&ContextNameList,(*it)->incCtxN,dummy);
			kdDebug()<<"Resolved "<<(*it)->incCtxN<< " to "<<(*it)->incCtx<<" for include rule"<<endl;
			// It would be good to look here somehow, if the result is valid
		}
	} else ++it; //nothing to do, already resolved (by the cross defintion reference resolver
  }

  // now that all IncludeRule items should be valid and completely resolved, do the real inclusion of the rules.
  // recursiveness is needed, because context 0 could include context 1, which itself includes context 2 and so on.
  //	In that case we have to handle context 2 first, then 1, 0
//TODO: catch circular references: eg 0->1->2->3->1
  while (!includeRules.isEmpty())
  	handleIncludeRulesRecursive(includeRules.begin(),&includeRules);


}

void Highlight::handleIncludeRulesRecursive(IncludeRules::iterator it, IncludeRules *list)
{
	if (it==list->end()) return;  //invalid iterator, shouldn't happen, but better have a rule prepared ;)
	IncludeRules::iterator it1=it;
	int ctx=(*it1)->ctx;

	/*find the last entry for the given context in the IncludeRules list
 	  this is need if one context includes more than one. This saves us from updating all insert positions:
	  eg: context 0:
		pos 3 - include context 2
		pos 5 - include context 3
	  During the building of the includeRules list the items are inserted in ascending order, now we need it
	  descending to make our life easier.
	*/
	while ((it!=list->end()) && ((*it)->ctx==ctx))
	{
		it1=it;
		++it;
//		kdDebug()<<"loop1"<<endl;
	}
	// iterate over each include rule for the context the function has been called for.
	while ((it1!=list->end()) && ((*it1)->ctx==ctx))
	{
//		kdDebug()<<"loop2"<<endl;


		int ctx1=(*it1)->incCtx;

		//let's see, if the the included context includes other contexts
		for (IncludeRules::iterator it2=list->begin();it2!=list->end();++it2)
		{
//			kdDebug()<<"loop3"<<endl;

			if ((*it2)->ctx==ctx1)
			{
				//yes it does, so first handle that include rules, since we want to
				// include those subincludes too
				handleIncludeRulesRecursive(it2,list);
				break;
			}
		}

		// if the context we want to include had sub includes, they are already inserted there.
		HlContext *dest=contextList[ctx];
		HlContext *src=contextList[ctx1];
		uint p=(*it1)->pos; //insert the included context's rules starting at position p
		for ( HlItem *c = src->items.first(); c; c=src->items.next(), p++ )
                        dest->items.insert(p,c);

		it=it1; //backup the iterator
		--it1; //move to the next entry, which has to be take care of
		delete (*it); //free the already handled data structure
		list->remove(it); // remove it from the list
	}
}

int Highlight::addToContextList(const QString &ident, int ctx0)
{
  buildIdentifier=ident;
  syntaxContextData *data, *datasub;
  HlItem *c;

  QString dummy;

  // Let the syntax document class know, which file we'd like to parse
  if (!HlManager::self()->syntax->setIdentifier(ident))
  {
	noHl=true;
	KMessageBox::information(0L,i18n("Since there has been an error parsing the highlighting description, this highlighting will be disabled"));
	return 0;
  }

  RegionList<<"!KateInternal_TopLevel!";
  readCommentConfig();
  readGlobalKeywordConfig();


  QString ctxName;

  // This list is needed for the translation of the attribute parameter, if the itemData name is given instead of the index
  addToItemDataList();
  ItemDataList iDl = internalIDList;

  createContextNameList(&ContextNameList,ctx0);

  kdDebug(13010)<<"Parsing Context structure"<<endl;
  //start the real work
  data=HlManager::self()->syntax->getGroupInfo("highlighting","context");
  uint i=buildContext0Offset;
  if (data)
    {
      while (HlManager::self()->syntax->nextGroup(data))
        {
	  kdDebug(13010)<<"Found a context in file, building structure now"<<endl;
          // BEGIN - Translation of the attribute parameter
          QString tmpAttr=HlManager::self()->syntax->groupData(data,QString("attribute")).simplifyWhiteSpace();
          int attr;
          if (QString("%1").arg(tmpAttr.toInt())==tmpAttr)
            attr=tmpAttr.toInt();
          else
            attr=lookupAttrName(tmpAttr,iDl);
          // END - Translation of the attribute parameter

	  ctxName=buildPrefix+HlManager::self()->syntax->groupData(data,QString("lineEndContext")).simplifyWhiteSpace();

	  QString tmpLineEndContext=HlManager::self()->syntax->groupData(data,QString("lineEndContext")).simplifyWhiteSpace();
	  int context;

	  context=getIdFromString(&ContextNameList, tmpLineEndContext,dummy);

          // BEGIN get fallthrough props
          bool ft = false;
          int ftc = 0; // fallthrough context
          if ( i > 0 ) { // fallthrough is not smart in context 0
            QString tmpFt = HlManager::self()->syntax->groupData(data, QString("fallthrough") );
            if ( tmpFt.lower() == "true" ||  tmpFt.toInt() == 1 )
              ft = true;
            if ( ft ) {
              QString tmpFtc = HlManager::self()->syntax->groupData( data, QString("fallthroughContext") );

  	      ftc=getIdFromString(&ContextNameList, tmpFtc,dummy);
	      if (ftc == -1) ftc =0;

              kdDebug(13010)<<"Setting fall through context (context "<<i<<"): "<<ftc<<endl;
            }
          }

          // END falltrhough props
          contextList.insert (i, new HlContext (
            attr,
            context,
            (HlManager::self()->syntax->groupData(data,QString("lineBeginContext"))).isEmpty()?-1:
            (HlManager::self()->syntax->groupData(data,QString("lineBeginContext"))).toInt(),
            ft, ftc
                                       ));


            //Let's create all items for the context
            while (HlManager::self()->syntax->nextItem(data))
              {
//		kdDebug(13010)<< "In make Contextlist: Item:"<<endl;

                // IncludeRules : add a pointer to each item in that context

                QString tag = HlManager::self()->syntax->groupItemData(data,QString(""));
                if ( tag == "IncludeRules" ) { //if the new item is an Include rule, we have to take special care
			QString incCtx=HlManager::self()->syntax->groupItemData( data, QString("context"));
			// only context refernces of type NAME and ##Name are allowed
			if (incCtx.startsWith("##") || (!incCtx.startsWith("#"))) { //#stay, #pop is not interesting here
				if (!incCtx.startsWith("#")) { // a local reference -> just initialize the include rule structure
					incCtx=buildPrefix+incCtx.simplifyWhiteSpace();
					includeRules.append(new IncludeRule(i,contextList[i]->items.count(),incCtx));
				}
				else { //a cross highlighting reference
					kdDebug()<<"Cross highlight reference <IncludeRules>"<<endl;
					IncludeRule *ir=new IncludeRule(i,contextList[i]->items.count());
					//use the same way to determine cross hl file references as other items do
					if (!embeddedHls.contains(incCtx.right(incCtx.length()-2)))
						embeddedHls.insert(incCtx.right(incCtx.length()-2),EmbeddedHlInfo());
					unresolvedContextReferences.insert(&(ir->incCtx),
							incCtx.right(incCtx.length()-2));
					includeRules.append(ir);
				}
			}
			continue;
		}
#if 0
                QString tag = HlManager::self()->syntax->groupItemData(data,QString(""));
                if ( tag == "IncludeRules" ) {
                  // attrib context: the index (jowenn, i think using names here would be a cool feat, goes for mentioning the context in any item. a map or dict?)
                  int ctxId = getIdFromString(&ContextNameList,
			HlManager::self()->syntax->groupItemData( data, QString("context")),dummy); // the index is *required*
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
		c=createHlItem(data,iDl,&RegionList,&ContextNameList);
		if (c)
			{
                                contextList[i]->items.append(c);

                                // Not supported completely atm and only one level. Subitems.(all have to be matched to at once)
				datasub=HlManager::self()->syntax->getSubItems(data);
				bool tmpbool;
				if (tmpbool=HlManager::self()->syntax->nextItem(datasub))
					{
					  c->subItems=new QPtrList<HlItem>;
					  for (;tmpbool;tmpbool=HlManager::self()->syntax->nextItem(datasub))
                                            c->subItems->append(createHlItem(datasub,iDl,&RegionList,&ContextNameList));
                                        }
				HlManager::self()->syntax->freeGroupInfo(datasub);
                                // end of sublevel
			}
//		kdDebug(13010)<<"Last line in loop"<<endl;
              }
          i++;
        }
      }

  HlManager::self()->syntax->freeGroupInfo(data);
  if (RegionList.count()!=1) folding=true;
  return i;
}

HlManager::HlManager() : QObject(0)
{
  hlList.setAutoDelete(true);
  hlDict.setAutoDelete(false);

  syntax = new SyntaxDocument();
  SyntaxModeList modeList = syntax->modeList();

  Highlight *hl = new Highlight(0);
  hlList.append (hl);
  hlDict.insert (hl->name(), hl);

  uint i=0;
  while (i < modeList.count())
  {
    hl = new Highlight(modeList.at(i));
    hlList.append (hl);
    hlDict.insert (hl->name(), hl);

    i++;
  }
}

HlManager::~HlManager() {
    if(syntax)
        delete syntax;

}

HlManager *HlManager::self()
{
  if ( !s_pSelf )
    s_pSelf = new HlManager;
  return s_pSelf;
}

KConfig *HlManager::getKConfig()
{
  if (!s_pConfig)
    s_pConfig = new KConfig("katesyntaxhighlightingrc");
  return s_pConfig;
}

Highlight *HlManager::getHl(int n) {
  if (n < 0 || n >= (int) hlList.count()) n = 0;
  return hlList.at(n);
}

int HlManager::defaultHl() {
  KConfig *config;

  config = KateFactory::instance()->config();
  config->setGroup("General Options");
  return nameFind(config->readEntry("Highlight"));
}


int HlManager::nameFind(const QString &name) {
  int z;

  for (z = hlList.count() - 1; z > 0; z--) {
    if (hlList.at(z)->name() == name) break;
  }
  return z;
}

int HlManager::wildcardFind(const QString &fileName)
{
  int result;
  if ((result = realWildcardFind(fileName)) != -1)
    return result;

  int length = fileName.length();
  QString backupSuffix = KateDocument::backupSuffix();
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

int HlManager::realWildcardFind(const QString &fileName)
{
  Highlight *highlight;
  QStringList l;
  QRegExp sep("\\s*;\\s*");
  for (highlight = hlList.first(); highlight != 0L; highlight = hlList.next()) {
    // anders: this is more likely to catch the right one ;)
    QStringList l = QStringList::split( sep, highlight->getWildcards() );
    for( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(*it, false, true);
      if ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        return hlList.at();
    }
  }
  return -1;
}

int HlManager::mimeFind(const QByteArray &contents, const QString &)
{
//  kdDebug(13010)<<"hlManager::mimeFind( [contents], "<<fname<<")"<<endl;
//  kdDebug(13010)<<"file contents: "<<endl<<contents.data()<<endl<<"- - - - - - END CONTENTS - - - - -"<<endl;

  // detect the mime type
  KMimeType::Ptr mt;
  int accuracy; // just for debugging
  mt = KMimeType::findByContent( contents, &accuracy );
  QString mtname = mt->name();
//  kdDebug(13010)<<"KMimeType::findByContent() returned '"<<mtname<<"', accuracy: "<<accuracy<<endl;

  Highlight *highlight;
  for (highlight = hlList.first(); highlight != 0L; highlight = hlList.next())
  {
//    kdDebug(13010)<<"KMimeType::findByContent(): considering hl "<<highlight->name()<<endl;
    QRegExp sep("\\s*;\\s*");
    QStringList l = QStringList::split( sep, highlight->getMimetypes() );
    for( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
    {
//      kdDebug(13010)<<"..trying "<<*it<<endl;
      if ( *it == mtname ) // faster than a regexp i guess?
        return hlList.at();
    }
  }

  return -1;
}

void HlManager::makeAttribs(KateDocument *doc, Highlight *highlight)
{
  KateAttributeList defaultStyleList;
  KateAttribute *defaultStyle;
  ItemDataList itemDataList;
  ItemData *itemData;
  uint nAttribs, z;

  defaultStyleList.setAutoDelete(true);
  getDefaults(defaultStyleList);

  highlight->getItemDataList(itemDataList);
  nAttribs = itemDataList.count();

  doc->attribs()->resize (nAttribs);

  for (z = 0; z < nAttribs; z++)
  {
    KateAttribute n;

    itemData = itemDataList.at(z);
    if (itemData->defStyle)
    {
      // default style
      defaultStyle = defaultStyleList.at(itemData->defStyleNum);
      n += *defaultStyle;
    }
    else
    {
      // custom style
      n += *itemData;
    }

    doc->attribs()->at(z) = n;
  }
}

int HlManager::defaultStyles() {
  return 10;
}

QString HlManager::defaultStyleName(int n)
{
  static QStringList names;

  if (names.isEmpty())
  {
    names << i18n("Normal");
    names << i18n("Keyword");
    names << i18n("Data Type");
    names << i18n("Decimal/Value");
    names << i18n("Base-N Integer");
    names << i18n("Floating Point");
    names << i18n("Character");
    names << i18n("String");
    names << i18n("Comment");
    names << i18n("Others");
  }

  return names[n];
}

void HlManager::getDefaults(KateAttributeList &list) {
  KConfig *config;
  int z;
  KateAttribute *i;
  QString s;
  QRgb col, selCol;

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
  string->setTextColor(Qt::red);
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

  config = KateFactory::instance()->config();
  config->setGroup("Default Item Styles");
  for (z = 0; z < defaultStyles(); z++) {
    i = list.at(z);
    s = config->readEntry(defaultStyleName(z));
    if (!s.isEmpty()) {
      int bold, italic;
      sscanf(s.latin1(),"%X,%X,%d,%d",&col,&selCol,&bold,&italic);
      QColor color = i->textColor();
      color.setRgb(col);
      i->setTextColor(color);

      QColor selColor = i->selectedTextColor();
      selColor.setRgb(selCol);
      i->setSelectedTextColor(selColor);

      i->setItalic(italic);
      i->setBold(bold);
    }
  }
}

void HlManager::setDefaults(KateAttributeList &list) {
  KConfig *config;
  int z;
  KateAttribute *i;
  char s[64];

  config =  KateFactory::instance()->config();
  config->setGroup("Default Item Styles");
  for (z = 0; z < defaultStyles(); z++) {
    i = list.at(z);
    sprintf(s,"%X,%X,%d,%d",i->textColor().rgb(),i->selectedTextColor().rgb(),i->bold(), i->italic());
    config->writeEntry(defaultStyleName(z),s);
  }

  emit changed();
}


int HlManager::highlights() {
  return (int) hlList.count();
}

QString HlManager::hlName(int n) {
  return hlList.at(n)->name();
}

QString HlManager::hlSection(int n) {
  return hlList.at(n)->section();
}

void HlManager::getHlDataList(HlDataList &list) {
  int z;

  for (z = 0; z < (int) hlList.count(); z++) {
    list.append(hlList.at(z)->getData());
  }
}

void HlManager::setHlDataList(HlDataList &list) {
  int z;

  for (z = 0; z < (int) hlList.count(); z++) {
    hlList.at(z)->setData(list.at(z));
  }
  //notify documents about changes in highlight configuration
  emit changed();
}

QString HlManager::identifierForName(const QString& name)
{
  Highlight *hl = 0;

  if ((hl = hlDict[name]))
    return hl->getIdentifier ();

  return QString();
}
