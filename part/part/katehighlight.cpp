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
#include <qstringlist.h>
#include <qtextstream.h>

#include <kconfig.h>
#include <kglobal.h>
#include <kinstance.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kregexp.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kapplication.h>

#include "katetextline.h"
#include "katedocument.h"
#include "katesyntaxdocument.h"
#include "katefactory.h"

#include "katehighlight.h"
#include "katehighlight.moc"

//END


//BEGIN  Prviate HL classes

class HlCharDetect : public HlItem
{
  public:
    HlCharDetect(int attribute, int context,signed char regionId,signed char regionId2, QChar);
    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    QChar sChar;
};

class Hl2CharDetect : public HlItem
{
  public:
    Hl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2,  QChar ch1, QChar ch2);
    Hl2CharDetect(int attribute, int context,signed char regionId,signed char regionId2,  const QChar *ch);

    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    QChar sChar1;
    QChar sChar2;
};

class HlStringDetect : public HlItem
{
  public:
    HlStringDetect(int attribute, int context, signed char regionId,signed char regionId2, const QString &, bool inSensitive=false);

    virtual ~HlStringDetect();
    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    const QString str;
    bool _inSensitive;
};

class HlRangeDetect : public HlItem
{
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

class HlInt : public HlItem
{
  public:
    HlInt(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlFloat : public HlItem
{
  public:
    HlFloat(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlCOct : public HlItem
{
  public:
    HlCOct(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlCHex : public HlItem
{
  public:
    HlCHex(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlCFloat : public HlFloat
{
  public:
    HlCFloat(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
    int checkIntHgl(const QString& text, int offset, int len);
    virtual bool alwaysStartEnable() const;
};

class HlLineContinue : public HlItem
{
  public:
    HlLineContinue(int attribute, int context, signed char regionId,signed char regionId2);

    virtual bool endEnable(QChar c) {return c == '\0';}
    virtual int checkHgl(const QString& text, int offset, int len);
    virtual bool lineContinue(){return true;}
};

class HlCStringChar : public HlItem
{
  public:
    HlCStringChar(int attribute, int context, signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class HlCChar : public HlItem
{
  public:
    HlCChar(int attribute, int context,signed char regionId,signed char regionId2);

    virtual int checkHgl(const QString& text, int offset, int len);
};

class HlAnyChar : public HlItem
{
  public:
    HlAnyChar(int attribute, int context, signed char regionId,signed char regionId2, const QString& charList);

    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    const QString _charList;
};

class HlRegExpr : public HlItem
{
  public:
    HlRegExpr(int attribute, int context,signed char regionId,signed char regionId2 ,QString expr, bool insensitive, bool minimal);
    ~HlRegExpr(){delete Expr;};

    virtual int checkHgl(const QString& text, int offset, int len);

  private:
    QRegExp *Expr;
    bool handlesLinestart;
};

//END

//BEGIN STATICS
HlManager *HlManager::s_pSelf = 0;
KConfig *HlManager::s_pConfig =0;

// Make this configurable?
QStringList HlManager::commonSuffixes = QStringList::split(";", ".orig;.new;~;.bak;.BAK");

enum Item_styles { dsNormal,dsKeyword,dsDataType,dsDecVal,dsBaseN,dsFloat,dsChar,dsString,dsComment,dsOthers};

static const bool trueBool = true;
static const QString stdDeliminator = QString (" \t.():!+,-<=>%&*/;?[]^{|}~\\");
//END


//BEGIN NON MEMBER FUNCTIONS
static int getDefStyleNum(QString name)
{
  if (name=="dsNormal") return dsNormal;
  else if (name=="dsKeyword") return dsKeyword;
  else if (name=="dsDataType") return dsDataType;
  else if (name=="dsDecVal") return dsDecVal;
  else if (name=="dsBaseN") return dsBaseN;
  else if (name=="dsFloat") return dsFloat;
  else if (name=="dsChar") return dsChar;
  else if (name=="dsString") return dsString;
  else if (name=="dsComment") return dsComment;
  else if (name=="dsOthers")  return dsOthers;

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
  if (subItems!=0)
  {
    subItems->setAutoDelete(true);
    subItems->clear();
    delete subItems;
  }
}

bool HlItem::startEnable(const QChar& c)
{
  // ONLY called when alwaysStartEnable() overridden
  // IN FACT not called at all, copied into doHighlight()...
  Q_ASSERT(false);
  return stdDeliminator.find(c) != -1;
}
//END

//BEGIN HLCharDetect
HlCharDetect::HlCharDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar c)
  : HlItem(attribute,context,regionId,regionId2), sChar(c)
{
}

int HlCharDetect::checkHgl(const QString& text, int offset, int len)
{
  if (len && text[offset] == sChar)
    return offset + 1;

  return 0;
}
//END

//BEGIN Hl2CharDetect
Hl2CharDetect::Hl2CharDetect(int attribute, int context, signed char regionId,signed char regionId2, QChar ch1, QChar ch2)
  : HlItem(attribute,context,regionId,regionId2)
{
  sChar1 = ch1;
  sChar2 = ch2;
}

int Hl2CharDetect::checkHgl(const QString& text, int offset, int len)
{
  if (len < 2)
    return offset;

  if (text[offset++] == sChar1 && text[offset++] == sChar2)
    return offset;

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

  if ( dict.find(text.mid(offset, offset2 - offset)) ) return offset2;

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
        if ( (offset = it->checkHgl(text, offset2, len)) )
          return offset;
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
  : name(name), defStyleNum(defStyleNum) {
}

HlData::HlData(const QString &wildcards, const QString &mimetypes, const QString &identifier, int priority)
  : wildcards(wildcards), mimetypes(mimetypes), identifier(identifier), priority(priority)
{
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
  errorsAndWarnings = "";
  building=false;
  noHl = false;
  folding=false;
  internalIDList.setAutoDelete(true);

  if (def == 0)
  {
    noHl = true;
    iName = I18N_NOOP("Normal");
    iSection = "";
    m_priority = 0;
  }
  else
  {
    iName = def->name;
    iSection = def->section;
    iWildcards = def->extension;
    iMimetypes = def->mimetype;
    identifier = def->identifier;
    iVersion=def->version;
    m_priority=def->priority.toInt();
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
    // There does an old context stack exist -> find the context at the line start
    ctxNum=ctx[oCtx.size()-1]; //context ID of the last character in the previous line

    //kdDebug(13010) << "\t\tctxNum = " << ctxNum << " contextList[ctxNum] = " << contextList[ctxNum] << endl; // ellis

    //if (lineContinue)   kdDebug(13010)<<QString("The old context should be %1").arg((int)ctxNum)<<endl;

    if (!(context = contextNum(ctxNum)))
      context = contextNum(0);

    //kdDebug(13010)<<"test1-2-1-text2"<<endl;

    prevLine=oCtx.size()-1; //position of the last context ID of th previous line within the stack

    //kdDebug(13010)<<"test1-2-1-text3"<<endl;
    generateContextStack(&ctxNum, context->ctx, &ctx, &prevLine, lineContinue); //get stack ID to use

    //kdDebug(13010)<<"test1-2-1-text4"<<endl;

    if (!(context = contextNum(ctxNum)))
      context = contextNum(0);

    //if (lineContinue)   kdDebug(13010)<<QString("The new context is %1").arg((int)ctxNum)<<endl;
  }

  // text, for programming convenience :)
  QChar lastChar = ' ';
  const QString& text = textLine->string();
  uint len = textLine->length();

  int offset1 = 0;
  uint z = 0;
  HlItem *item = 0;
  bool found = false;

  while (z < len)
  {
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
          standardStartEnable = stdDeliminator.find(lastChar) != -1;
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
        int offset2 = item->checkHgl(text, offset1, len-z);

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
    if (!found)
    {
      if ( context->fallthrough )
      {
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
      else
        textLine->setAttribs(context->attr,offset1,offset1 + 1);
    }

    offset1++;
    z++;
  }

  if (item==0)
    textLine->setHlLineContinue(false);
  else
  {
    textLine->setHlLineContinue(item->lineContinue());

    if (item->lineContinue())
      kdDebug(13010)<<"Setting line continue flag"<<endl;
  }

  textLine->setContext(ctx.data(), ctx.size());
}

KConfig *Highlight::getKConfig()
{
  KConfig *config = HlManager::getKConfig();

  config->setGroup(iName + QString(" Highlight"));

  return config;
}

QString Highlight::getWildcards()
{
  return getKConfig()->readEntry("Wildcards", iWildcards);
}

QString Highlight::getMimetypes()
{
  return getKConfig()->readEntry("Mimetypes", iMimetypes);
}

int Highlight::priority()
{
  return getKConfig()->readNumEntry("Priority", m_priority);
}

HlData *Highlight::getData()
{
  KConfig *config = getKConfig();

  HlData *hlData = new HlData(
    config->readEntry("Wildcards", iWildcards),
    config->readEntry("Mimetypes", iMimetypes),
    config->readEntry("Identifier", identifier),
    config->readNumEntry("Priority", m_priority));

  getItemDataList(hlData->itemDataList, config);

  return hlData;
}

void Highlight::setData(HlData *hlData)
{
  KConfig *config = getKConfig();

  config->writeEntry("Wildcards",hlData->wildcards);
  config->writeEntry("Mimetypes",hlData->mimetypes);
  config->writeEntry("Priority",hlData->priority);

  setItemDataList(hlData->itemDataList,config);

  config->sync ();
}

void Highlight::getItemDataList(ItemDataList &list)
{
  getItemDataList(list, getKConfig());
}

void Highlight::getItemDataList(ItemDataList &list, KConfig *config)
{
  list.clear();
  createItemData(list);

  for (ItemData *p = list.first(); p != 0L; p = list.next())
  {
    QStringList s = config->readListEntry(p->name);

//    kdDebug()<<p->name<<s.count()<<endl;
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

void Highlight::setItemDataList(ItemDataList &list, KConfig *config)
{
  QStringList settings;

  for (ItemData *p = list.first(); p != 0L; p = list.next())
  {
	settings.clear();
	settings<<QString::number(p->defStyleNum,10);
	settings<<(p->itemSet(KateAttribute::TextColor)?QString::number(p->textColor().rgb(),16):"");
	settings<<(p->itemSet(KateAttribute::SelectedTextColor)?QString::number(p->selectedTextColor().rgb(),16):"");
	settings<<(p->itemSet(KateAttribute::Bold)?(p->bold()?"1":0):"");
	settings<<(p->itemSet(KateAttribute::Italic)?(p->italic()?"1":0):"");
	settings<<(p->itemSet(KateAttribute::StrikeOut)?(p->strikeOut()?"1":0):"");
	settings<<(p->itemSet(KateAttribute::Underline)?(p->underline()?"1":0):"");
	settings<<(p->itemSet(KateAttribute::BGColor)?QString::number(p->bgColor().rgb(),16):"");
	settings<<(p->itemSet(KateAttribute::SelectedBGColor)?QString::number(p->selectedBGColor().rgb(),16):"");
	settings<<"---";
//    s.sprintf("%d,%X,%X,%d,%d,%d,%d",
//      p->isSomethingSet(),p->textColor().rgb(),p->selectedTextColor().rgb(),p->bold(),p->italic(),p->strikeOut(),p->underline());

//    config->writeEntry(p->name,s);
      config->writeEntry(p->name,settings);
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
  if (refCount == 0)
    init();

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

  if (refCount == 0)
    done();
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
  //Tell the syntax document class which file we want to parse and which data group
  HlManager::self()->syntax->setIdentifier(buildIdentifier);
  syntaxContextData *data = HlManager::self()->syntax->getGroupInfo("highlighting","itemData");

  //begin with the real parsing
  while (HlManager::self()->syntax->nextGroup(data))
  {
    // read all attributes
    QString color = HlManager::self()->syntax->groupData(data,QString("color"));
    QString selColor = HlManager::self()->syntax->groupData(data,QString("selColor"));
    QString bold = HlManager::self()->syntax->groupData(data,QString("bold"));
    QString italic = HlManager::self()->syntax->groupData(data,QString("italic"));
    QString underline = HlManager::self()->syntax->groupData(data,QString("underline"));
    QString strikeOut = HlManager::self()->syntax->groupData(data,QString("strikeOut"));
    QString bgColor = HlManager::self()->syntax->groupData(data,QString("backgroundColor"));
    QString selBgColor = HlManager::self()->syntax->groupData(data,QString("selBackgroundColor"));

      ItemData* newData = new ItemData(
              buildPrefix+HlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace(),
              getDefStyleNum(HlManager::self()->syntax->groupData(data,QString("defStyleNum"))));


      /* here the custom style overrides are specified, if needed */
      if (!color.isEmpty()) newData->setTextColor(QColor(color));
      if (!selColor.isEmpty()) newData->setSelectedTextColor(QColor(selColor));
      if (!bold.isEmpty()) newData->setBold(bold=="true" || bold=="1");
      if (!italic.isEmpty()) newData->setItalic(italic=="true" || italic=="1");
      // new attributes for the new rendering view
      if (!underline.isEmpty()) newData->setUnderline(underline=="true" || underline=="1");
      if (!strikeOut.isEmpty()) newData->setStrikeOut(strikeOut=="true" || strikeOut=="1"); 
      if (!bgColor.isEmpty()) newData->setBGColor(QColor(bgColor));
      if (!selBgColor.isEmpty()) newData->setSelectedBGColor(QColor(selBgColor));

      internalIDList.append(newData);
  }

  //clean up
  if (data)
    HlManager::self()->syntax->freeGroupInfo(data);
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
  static const QString sq("\"'");
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
  HlManager::self()->syntax->setIdentifier(buildIdentifier);
  syntaxContextData *data=HlManager::self()->syntax->getGroupInfo("general","comment");

  if (data)
  {
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
  else
  {
    cslStart = "";
    cmlStart = "";
    cmlEnd = "";
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
  syntaxContextData *data = HlManager::self()->syntax->getConfig("general","keywords");

  if (data)
  {
    kdDebug(13010)<<"Found global keyword config"<<endl;

    if (HlManager::self()->syntax->groupItemData(data,QString("casesensitive"))!="0")
      casesensitive=true;
    else
      casesensitive=false;

    //get the weak deliminators
    weakDeliminator=(HlManager::self()->syntax->groupItemData(data,QString("weakDeliminator")));

    kdDebug(13010)<<"weak delimiters are: "<<weakDeliminator<<endl;

    // remove any weakDelimitars (if any) from the default list and store this list.
    for (uint s=0; s < weakDeliminator.length(); s++)
    {
      int f = deliminator.find (weakDeliminator[s]);

      if (f > -1)
        deliminator.remove (f, 1);
    }

    QString addDelim = (HlManager::self()->syntax->groupItemData(data,QString("additionalDeliminator")));

    if (!addDelim.isEmpty())
      deliminator=deliminator+addDelim;

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
  kdDebug(13010)<<"creatingContextNameList:BEGIN"<<endl;

  if (ctx0 == 0)
      ContextNameList->clear();

  HlManager::self()->syntax->setIdentifier(buildIdentifier);

  syntaxContextData *data=HlManager::self()->syntax->getGroupInfo("highlighting","context");

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

  uint i=0;
  while (i < modeList.count())
  {
    Highlight *hl = new Highlight(modeList.at(i));

    uint insert = 0;
    for (; insert <= hlList.count(); insert++)
    {
      if (insert == hlList.count())
        break;

      if ( QString(hlList.at(insert)->section() + hlList.at(insert)->name()).lower()
            > QString(hl->section() + hl->name()).lower() )
        break;
    }

    hlList.insert (insert, hl);
    hlDict.insert (hl->name(), hl);

    i++;
  }

  // Normal HL
  Highlight *hl = new Highlight(0);
  hlList.prepend (hl);
  hlDict.insert (hl->name(), hl);
}

HlManager::~HlManager()
{
  if(syntax)
  {
    delete syntax;
    syntax = 0;
  }
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

Highlight *HlManager::getHl(int n)
{
  if (n < 0 || n >= (int) hlList.count())
    n = 0;

  return hlList.at(n);
}

int HlManager::defaultHl()
{
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("General Options");

  return nameFind(config->readEntry("Highlight"));
}

int HlManager::nameFind(const QString &name)
{
  int z (hlList.count() - 1);
  for (; z > 0; z--)
    if (hlList.at(z)->name() == name)
      return z;

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
  static QRegExp sep("\\s*;\\s*");

  QPtrList<Highlight> highlights;

  for (Highlight *highlight = hlList.first(); highlight != 0L; highlight = hlList.next())
  {
    // anders: this is more likely to catch the right one ;)
    QStringList l = QStringList::split( sep, highlight->getWildcards() );

    for( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(*it, true, true);
      if ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        highlights.append (highlight);
    }
  }

  if ( !highlights.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    for (Highlight *highlight = highlights.first(); highlight != 0L; highlight = highlights.next())
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

int HlManager::mimeFind(const QByteArray &contents, const QString &)
{
  static QRegExp sep("\\s*;\\s*");

  int accuracy;
  KMimeType::Ptr mt = KMimeType::findByContent( contents, &accuracy );

  QPtrList<Highlight> highlights;

  for (Highlight *highlight = hlList.first(); highlight != 0L; highlight = hlList.next())
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

    for (Highlight *highlight = highlights.first(); highlight != 0L; highlight = highlights.next())
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

void HlManager::makeAttribs(KateDocument *doc, Highlight *highlight)
{
  KateAttributeList defaultStyleList;
  defaultStyleList.setAutoDelete(true);
  getDefaults(defaultStyleList);

  ItemDataList itemDataList;
  highlight->getItemDataList(itemDataList);

  uint nAttribs = itemDataList.count();
  doc->attribs()->resize (nAttribs);

  for (uint z = 0; z < nAttribs; z++)
  {
    KateAttribute n;
    ItemData *itemData = itemDataList.at(z);

      KateAttribute *defaultStyle = defaultStyleList.at(itemData->defStyleNum);
      n+= *defaultStyle;

    if (itemData->isSomethingSet())
    {
      // custom style
      n += *itemData;
    }

    doc->attribs()->at(z) = n;
  }
}

int HlManager::defaultStyles()
{
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

void HlManager::getDefaults(KateAttributeList &list)
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

  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Default Item Styles");

  for (int z = 0; z < defaultStyles(); z++)
  {
    KateAttribute *i = list.at(z);
    QStringList s = config->readListEntry(defaultStyleName(z));

//    kdDebug()<<defaultStyleName(z)<<s.count()<<endl;
    if (s.count()>0)
    {

      while(s.count()<8) s<<"";

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

void HlManager::setDefaults(KateAttributeList &list)
{
  KConfig *config =  KateFactory::instance()->config();
  config->setGroup("Default Item Styles");

   QStringList settings;

  for (int z = 0; z < defaultStyles(); z++)
  {
    KateAttribute *i = list.at(z);

        settings.clear();
        settings<<(i->itemSet(KateAttribute::TextColor)?QString::number(i->textColor().rgb(),16):"");
        settings<<(i->itemSet(KateAttribute::SelectedTextColor)?QString::number(i->selectedTextColor().rgb(),16):"");
        settings<<(i->itemSet(KateAttribute::Bold)?(i->bold()?"1":0):"");
        settings<<(i->itemSet(KateAttribute::Italic)?(i->italic()?"1":0):"");
        settings<<(i->itemSet(KateAttribute::StrikeOut)?(i->strikeOut()?"1":0):"");
        settings<<(i->itemSet(KateAttribute::Underline)?(i->underline()?"1":0):"");
        settings<<(i->itemSet(KateAttribute::BGColor)?QString::number(i->bgColor().rgb(),16):"");
        settings<<(i->itemSet(KateAttribute::SelectedBGColor)?QString::number(i->selectedBGColor().rgb(),16):"");
	settings<<"---";
        config->writeEntry(defaultStyleName(z),settings);
//    config->writeEntry(defaultStyleName(z),s);
  }

  emit changed();
}

int HlManager::highlights()
{
  return (int) hlList.count();
}

QString HlManager::hlName(int n)
{
  return hlList.at(n)->name();
}

QString HlManager::hlSection(int n)
{
  return hlList.at(n)->section();
}

void HlManager::getHlDataList(HlDataList &list)
{
  for (uint z = 0; z < hlList.count(); z++)
    list.append(hlList.at(z)->getData());
}

void HlManager::setHlDataList(HlDataList &list)
{
  for (uint z = 0; z < hlList.count(); z++)
    hlList.at(z)->setData(list.at(z));

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
