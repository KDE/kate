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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

//BEGIN INCLUDES
#include <string.h>
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
    HlCharDetect(int attribute, int context,signed char regionId, QChar);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
  private:
    QChar sChar;
};

class Hl2CharDetect : public HlItem {
  public:
    Hl2CharDetect(int attribute, int context, signed char regionId,  QChar ch1, QChar ch2);
   	Hl2CharDetect(int attribute, int context,signed char regionId,  const QChar *ch);

    virtual const QChar *checkHgl(const QChar *, int len, bool);
  private:
    QChar sChar1;
    QChar sChar2;
};

class HlStringDetect : public HlItem {
  public:
    HlStringDetect(int attribute, int context, signed char regionId, const QString &, bool inSensitive=false);
    virtual ~HlStringDetect();
    virtual const QChar *checkHgl(const QChar *, int len, bool);
  private:
    const QString str;
    bool _inSensitive;
};

class HlRangeDetect : public HlItem {
  public:
    HlRangeDetect(int attribute, int context, signed char regionId, QChar ch1, QChar ch2);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
  private:
    QChar sChar1;
    QChar sChar2;
};

class HlKeyword : public HlItem
{
  public:
    HlKeyword(int attribute, int context,signed char regionId, bool casesensitive, const QChar *deliminator, uint deliLen);
    virtual ~HlKeyword();

    virtual void addWord(const QString &);
    virtual void addList(const QStringList &);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool startEnable(QChar c);

  private:
    QDict<bool> dict;
    bool _caseSensitive;
    const QChar *deliminatorChars;
    uint deliminatorLen;
};

class HlPHex : public HlItem {
  public:
    HlPHex(int attribute,int context, signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};
class HlInt : public HlItem {
  public:
    HlInt(int attribute, int context, signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool startEnable(QChar c);

};

class HlFloat : public HlItem {
  public:
    HlFloat(int attribute, int context, signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool startEnable(QChar c);
};

class HlCOct : public HlItem {
  public:
    HlCOct(int attribute, int context, signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlCHex : public HlItem {
  public:
    HlCHex(int attribute, int context, signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlCFloat : public HlFloat {
  public:
    HlCFloat(int attribute, int context, signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    const QChar *checkIntHgl(const QChar *, int, bool);
};

class HlLineContinue : public HlItem {
  public:
    HlLineContinue(int attribute, int context, signed char regionId);
    virtual bool endEnable(QChar c) {return c == '\0';}
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool lineContinue(){return true;}
};

class HlCStringChar : public HlItem {
  public:
    HlCStringChar(int attribute, int context, signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlCChar : public HlItem {
  public:
    HlCChar(int attribute, int context,signed char regionId);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlAnyChar : public HlItem {
  public:
    HlAnyChar(int attribute, int context, signed char regionId, const QChar* charList, uint len);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    const QChar* _charList;
    uint _charListLen;
};

class HlRegExpr : public HlItem {
  public:
  HlRegExpr(int attribute, int context,signed char regionId ,QString expr, bool insensitive, bool minimal);
  virtual const QChar *checkHgl(const QChar *, int len, bool);
  ~HlRegExpr(){delete Expr;};

  QRegExp *Expr;

  bool handlesLinestart;
};

//--------

//BEGIN STATICS
HlManager *HlManager::s_pSelf = 0;
KConfig *HlManager::s_pConfig =0;

enum Item_styles { dsNormal,dsKeyword,dsDataType,dsDecVal,dsBaseN,dsFloat,dsChar,dsString,dsComment,dsOthers};

static bool trueBool = true;
static QString stdDeliminator = QString ("!%&()*+,-./:;<=>?[]^{|}~ \t\\");

static const QChar *stdDeliminatorChars = stdDeliminator.unicode();
static int stdDeliminatorLen=stdDeliminator.length();
//END

//BEGIN NON MEMBER FUNCTIONS
int getDefStyleNum(QString name)
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

bool ustrchr(const QChar *s, uint len, QChar c)
{
  for (uint z=0; z < len; z++)
  {
    if (*s == c) return true;
    s++;
  }

  return false;
}

//END


//BEGIN HlItem
HlItem::HlItem(int attribute, int context,signed char regionId)
  : attr(attribute), ctx(context),region(regionId)  {subItems=0;
}

HlItem::~HlItem()
{
  //kdDebug(13010)<<"In hlItem::~HlItem()"<<endl;
  if (subItems!=0) {subItems->setAutoDelete(true); subItems->clear(); delete subItems;}
}

bool HlItem::startEnable(QChar)
{
  return true;
}
//END

//BEGIN HLCharDetect
HlCharDetect::HlCharDetect(int attribute, int context, signed char regionId, QChar c)
  : HlItem(attribute,context,regionId), sChar(c) {
}

const QChar *HlCharDetect::checkHgl(const QChar *str, int len, bool) {
  if ((len > 0) && (*str == sChar)) return str + 1;
  return 0L;
}
//END

//BEGIN Hl2CharDetect
Hl2CharDetect::Hl2CharDetect(int attribute, int context, signed char regionId, QChar ch1, QChar ch2)
  : HlItem(attribute,context,regionId) {
  sChar1 = ch1;
  sChar2 = ch2;
}

const QChar *Hl2CharDetect::checkHgl(const QChar *str, int len, bool) {
  if (len <2) return 0L;
  if (str[0] == sChar1 && str[1] == sChar2) return str + 2;
  return 0L;
}
//END

//BEGIN HlStringDetect
HlStringDetect::HlStringDetect(int attribute, int context, signed char regionId,const QString &s, bool inSensitive)
  : HlItem(attribute, context,regionId), str(inSensitive ? s.upper():s), _inSensitive(inSensitive) {
}

HlStringDetect::~HlStringDetect() {
}

const QChar *HlStringDetect::checkHgl(const QChar *s, int len , bool) {
  if (len<(int)str.length()) return 0L;
  if (!_inSensitive) {if (memcmp(s, str.unicode(), str.length()*sizeof(QChar)) == 0) return s + str.length();}
     else
       {
	 QString tmp=QString(s,str.length()).upper();
	 if (tmp==str) return s+str.length();
       }
  return 0L;
}
//END


//BEGIN HLRangeDetect
HlRangeDetect::HlRangeDetect(int attribute, int context, signed char regionId, QChar ch1, QChar ch2)
  : HlItem(attribute,context,regionId) {
  sChar1 = ch1;
  sChar2 = ch2;
}

const QChar *HlRangeDetect::checkHgl(const QChar *s, int len, bool) {
  if ((len > 0) && (*s == sChar1))
  {
    do
    {
      s++;
      len--;
      if (len < 1) return 0L;
    }
    while (*s != sChar2);

    return s + 1;
  }
  return 0L;
}
//END

//BEGIN HlKeyword
HlKeyword::HlKeyword (int attribute, int context, signed char regionId, bool casesensitive, const QChar *deliminator, uint deliLen)
  : HlItem(attribute,context,regionId), dict (113, casesensitive)
{
  deliminatorChars = deliminator;
  deliminatorLen = deliLen;
  _caseSensitive=casesensitive;
}

HlKeyword::~HlKeyword() {
}

bool HlKeyword::startEnable(QChar c)
{
  return ustrchr(deliminatorChars, deliminatorLen, c);
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

const QChar *HlKeyword::checkHgl(const QChar *s, int len, bool )
{
  if (len == 0) return 0L;

  const QChar *s2 = s;

  while ( (len > 0) && (!ustrchr(deliminatorChars, deliminatorLen, *s2)) )
  {
    s2++;
    len--;
  }

  if (s2 == s) return 0L;

  QString lookup = QString(s,s2-s);

  if ( dict.find(lookup) ) return s2;
  return 0L;
}
//END


//BEGIN HlInt
HlInt::HlInt(int attribute, int context, signed char regionId)
  : HlItem(attribute,context,regionId) {
}


bool HlInt::startEnable(QChar c)
{
//  return ustrchr(deliminatorChars, deliminatorLen, c);
    return ustrchr(stdDeliminatorChars, stdDeliminatorLen, c);
}

const QChar *HlInt::checkHgl(const QChar *str, int len, bool)
{
  const QChar *s,*s1;

  s = str;
  while ((len > 0) && s->isDigit())
  {
    s++;
    len--;
  }

  if (s > str)
  {
    if (subItems)
    {
      for (HlItem *it=subItems->first();it;it=subItems->next())
      {
        s1=it->checkHgl(s, len, false);
        if (s1) return s1;
      }
    }

    return s;
  }

  return 0L;
}
//END


//BEGIN HlFloat
HlFloat::HlFloat(int attribute, int context, signed char regionId)
  : HlItem(attribute,context, regionId) {
}

bool HlFloat::startEnable(QChar c)
{
//  return ustrchr(deliminatorChars, deliminatorLen, c);
    return ustrchr(stdDeliminatorChars, stdDeliminatorLen, c);
}

const QChar *HlFloat::checkHgl(const QChar *s, int len, bool)
{
  bool b, p;
  const QChar *s1;

  b = false;
  p = false;

  while ((len > 0) && s->isDigit())
  {
    s++;
    len--;
    b = true;
  }

  if ((len > 0) && (p = (*s == '.')))
  {
    s++;
    len--;

    while ((len > 0) && s->isDigit())
    {
      s++;
      len--;
      b = true;
    }
  }

  if (!b)
    return 0L;

  if ((len > 0) && ((*s&0xdf) == 'E'))
  {
    s++;
    len--;
  }
  else
  {
    if (!p)
      return 0L;
    else
    {
      if (subItems)
      {
        for (HlItem *it=subItems->first();it;it=subItems->next())
        {
          s1=it->checkHgl(s, len, false);

          if (s1)
            return s1;
        }
      }

      return s;
    }
  }

  if ((len > 0) && ((*s == '-')||(*s =='+')))
  {
    s++;
    len--;
  }

  b = false;

  while ((len > 0) && s->isDigit())
  {
    s++;
    len--;
    b = true;
  }

  if (b)
  {
    if (subItems)
    {
      for (HlItem *it=subItems->first();it;it=subItems->next())
      {
        s1=it->checkHgl(s, len, false);

        if (s1)
          return s1;
      }
    }

    return s;
  }

  return 0L;
}
//END

//BEGIN HlCOct
HlCOct::HlCOct(int attribute, int context, signed char regionId)
  : HlItem(attribute,context,regionId) {
}

const QChar *HlCOct::checkHgl(const QChar *str, int len, bool) {
  const QChar *s;

  if ((len > 0) && (*str == '0'))
  {
    str++;
    len--;

    s = str;

    while ((len > 0) && (*s >= '0' && *s <= '7'))
    {
      s++;
      len--;
    }

    if (s > str)
    {
      if ((len >0) && ((*s&0xdf) == 'L' || (*s&0xdf) == 'U' )) s++;

      return s;
    }
  }

  return 0L;
}
//END

//BEGIN HlCHex
HlCHex::HlCHex(int attribute, int context,signed char regionId)
  : HlItem(attribute,context,regionId) {
}

const QChar *HlCHex::checkHgl(const QChar *str, int len, bool)
{
  const QChar *s=str;

  if ((len > 1) && (str[0] == '0') && ((str[1]&0xdf) == 'X' ))
  {
    str += 2;
    len -= 2;

    s = str;

    while ((len > 0) && (s->isDigit() || ((*s&0xdf) >= 'A' && (*s&0xdf) <= 'F')))
    {
      s++;
      len--;
    }

    if (s > str)
    {
      if ((len > 0) && ((*s&0xdf) == 'L' || (*s&0xdf) == 'U' )) s++;

      return s;
    }
  }
  return 0L;
}
//END


HlCFloat::HlCFloat(int attribute, int context, signed char regionId)
  : HlFloat(attribute,context,regionId) {
}



const QChar *HlCFloat::checkIntHgl(const QChar *str, int, bool)
{
  const QChar *s;

  s = str;
  while (s->isDigit()) s++;
  if (s > str)
   {
     return s;
  }
  return 0L;
}

const QChar *HlCFloat::checkHgl(const QChar *s, int len, bool lineStart) {
  const QChar *tmp;
  tmp=s;

  s = HlFloat::checkHgl(s, len, lineStart);
  if (s)
  {
     if (s && ((*s&0xdf) == 'F' )) s++;
     return s;
  }
  else
  {
     tmp=checkIntHgl(tmp,len,lineStart);
     if (tmp && ((*tmp&0xdf) == 'F' ))
     {
	 tmp++;
     	return tmp;
     }
     else
        return 0;
  }
}

HlAnyChar::HlAnyChar(int attribute, int context, signed char regionId, const QChar* charList, uint len)
  : HlItem(attribute, context,regionId) {
  _charList=charList;
  _charListLen=len;
}

const QChar *HlAnyChar::checkHgl(const QChar *s, int len, bool)
{
  if ((len > 0) && ustrchr(_charList, _charListLen, *s)) return s + 1;
  return 0L;
}

HlRegExpr::HlRegExpr( int attribute, int context, signed char regionId, QString regexp, bool insensitive, bool minimal )
  : HlItem(attribute, context, regionId) {

    handlesLinestart=regexp.startsWith("^");
    if(!handlesLinestart) regexp.prepend("^");
    Expr=new QRegExp(regexp, !insensitive);
    Expr->setMinimal(minimal);
}

const QChar *HlRegExpr::checkHgl(const QChar *s, int len, bool lineStart)
{
  if ((!lineStart) && handlesLinestart) return 0;

  QString line(s,len);
  int pos = Expr->search( line, 0 );
  if (pos==-1) return 0L;
    else
	 return (s+Expr->matchedLength());
};


HlLineContinue::HlLineContinue(int attribute, int context, signed char regionId)
  : HlItem(attribute,context,regionId) {
}

const QChar *HlLineContinue::checkHgl(const QChar *s, int len, bool) {

  if ((len == 1) && (s[0] == '\\'))
	{
           return s + 1;
	}
  return 0L;
}


HlCStringChar::HlCStringChar(int attribute, int context,signed char regionId)
  : HlItem(attribute,context,regionId) {
}

// checks for C escaped chars \n and escaped hex/octal chars
const QChar *checkEscapedChar(const QChar *s, int *len) {
  int i;
  if (s[0] == '\\' && ((*len) > 1) )
  {
        s++;
        (*len) = (*len) - 1;
        switch(*s)
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
                case '\\': s++;
                               (*len) = (*len) - 1;
                           break;
                case 'x': // if it's like \xff
                        s++; // eat the x
                         (*len) = (*len) - 1;
                        // these for loops can probably be
                        // replaced with something else but
                        // for right now they work
                        // check for hexdigits
                        for(i=0; ((*len) > 0) && (i<2) && (*s >= '0' && *s <= '9' || (*s&0xdf) >= 'A' && (*s&0xdf) <= 'F'); i++) {s++;  (*len) = (*len) - 1;}
                        if(i==0) return 0L; // takes care of case '\x'
                        break;

                case '0': case '1': case '2': case '3' :
                case '4': case '5': case '6': case '7' :
                        for(i=0; ((*len) > 0) && (i < 3) &&(*s >='0'&& *s<='7');i++) {s++;  (*len) = (*len) - 1; }
                        break;
                        default: return 0L;
        }
  return s;
  }
  return 0L;
}

const QChar *HlCStringChar::checkHgl(const QChar *str, int len, bool) {
  return checkEscapedChar(str, &len);
}


HlCChar::HlCChar(int attribute, int context,signed char regionId)
  : HlItem(attribute,context,regionId) {
}

const QChar *HlCChar::checkHgl(const QChar *str, int len, bool) {
  const QChar *s;

  if ((len > 1) && (str[0] == '\'') && (str[1] != '\''))
  {
    int oldl;
    oldl = len;

    len--;

    s = checkEscapedChar(&str[1], &len);

    if (!s)
    {
      if (oldl > 2)
      {
        s = &str[2];
        len = oldl - 2;
      }
      else return 0L;
    }

    if ((len > 0) && (*s == '\'')) return s + 1;
  }

  return 0L;
}


//--------
ItemStyle::ItemStyle() : selCol(Qt::white), bold(false), italic(false) {
}

ItemStyle::ItemStyle(const QColor &col, const QColor &selCol,
  bool bold, bool italic)
  : col(col), selCol(selCol), bold(bold), italic(italic) {
}

ItemData::ItemData(const QString  name, int defStyleNum)
  : name(name), defStyleNum(defStyleNum), defStyle(true) {
}

ItemData::ItemData(const QString name, int defStyleNum,
  const QColor &col, const QColor &selCol, bool bold, bool italic)
  : ItemStyle(col,selCol,bold,italic), name(name), defStyleNum(defStyleNum),
  defStyle(false) {
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

Hl2CharDetect::Hl2CharDetect(int attribute, int context, signed char regionId, const QChar *s)
  : HlItem(attribute,context,regionId) {
  sChar1 = s[0];
  sChar2 = s[1];
}

Highlight::Highlight(const syntaxModeListItem *def) : refCount(0)
{
  noHl = false;
  folding=false;
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
  deliminatorChars = deliminator.unicode();
  deliminatorLen = deliminator.length();
}

Highlight::~Highlight()
{
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
  const QChar *s2;
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

  // first char
  const QChar *str = textLine->text();

  // non space char - index of that char
//  const QChar *s1 = textLine->firstNonSpace();
  const QChar *s1 = textLine->text();
  uint z=0;
//  uint z = textLine->firstChar();

  // length of textline
  uint len = textLine->length();

  bool found = false;
  while (z < len)
  {
    found = false;

    for (item = context->items.first(); item != 0L; item = context->items.next())
    {
      if (item->startEnable(lastChar))
      {
        s2 = item->checkHgl(s1, len-z, z==0);
        if (s2 > s1)
        {
          textLine->setAttribs(item->attr,s1 - str,s2 - str);
          //kdDebug(13010)<<QString("item->ctx: %1").arg(item->ctx)<<endl;
		if (item->region)
		{
//			kdDebug(13010)<<QString("Region mark detected: %1").arg(item->region)<<endl;
      foldingList->resize (foldingList->size()+1);

      for (uint z4=foldingList->size()-1; z4 >= 1; z4--)
      {
        (*foldingList)[z4] = (*foldingList)[z4-1];
      }

			(*foldingList)[0] = item->region;

		}

	      generateContextStack(&ctxNum, item->ctx, &ctx, &prevLine);  //regenerate context stack
		//kdDebug(13010)<<QString("generateContextStack has been left in item loop, size: %1").arg(ctx.size())<<endl;
	//    kdDebug(13010)<<QString("current ctxNum==%1").arg(ctxNum)<<endl;
	    context=contextNum(ctxNum);

            z = z + s2 - s1 - 1;
            s1 = s2 - 1;
            found = true;
            break;
        }
      }
    }

    lastChar = *s1;

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
        if (z) {
          const QChar *cheat;
          cheat = s1;
          cheat--;
          lastChar = *cheat;
        }
        else
          lastChar = '\\';
        continue;
      }
      else {
        textLine->setAttribs(context->attr,s1 - str,s1 - str + 1);
      }
    }
        s1++;
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
      sscanf(s.latin1(),"%d,%X,%X,%d,%d", &p->defStyle,&col,&selCol,&p->bold,&p->italic);
      p->col.setRgb(col);
      p->selCol.setRgb(selCol);
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
      p->defStyle,p->col.rgb(),p->selCol.rgb(),p->bold,p->italic);
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

  QString color;
  QString selColor;
  QString bold;
  QString italic;

  // If the internal list isn't already available read the config file
  if (internalIDList.count()==0)
  {
    //if all references to the list are destried the contents will also be deleted
    internalIDList.setAutoDelete(true);
    syntaxContextData *data;

    //Tell the syntax document class which file we want to parse and which data group
    HlManager::self()->syntax->setIdentifier(identifier);
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
                        internalIDList.append(new ItemData(
                                HlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace(),
                                getDefStyleNum(HlManager::self()->syntax->groupData(data,QString("defStyleNum"))),
                                QColor(color),QColor(selColor),(bold=="true") || (bold=="1"), (italic=="true") || (italic=="1")
                                ));
                }
        else
                {
                        //assign a default style
                        internalIDList.append(new ItemData(
                                HlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace(),
                                getDefStyleNum(HlManager::self()->syntax->groupData(data,QString("defStyleNum")))));

                }
      }
    //clean up
    if (data) HlManager::self()->syntax->freeGroupInfo(data);
  }

  //set the ouput reference
  list=internalIDList;
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
	for (uint i=0;i<iDl.count();i++)
		{
			if (iDl.at(i)->name==name) return i;
		}
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
                  attr=tmpAttr.toInt();
                else
                  attr=lookupAttrName(tmpAttr,iDl);
                // END - Translation of the attribute parameter

                // Info about context switch
		int context;
		QString tmpcontext=HlManager::self()->syntax->groupItemData(data,QString("context"));

	  	context=getIdFromString(ContextNameList, tmpcontext);

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
                bool insensitive=(HlManager::self()->syntax->groupItemData(data,QString("insensitive"))==QString("TRUE"));
                // anders: very resonable for regexp too!

                // for regexp only
                bool minimal = ( HlManager::self()->syntax->groupItemData(data,QString("minimal")).lower() == "true" );


		// code folding region handling:
		QString beginRegionStr=HlManager::self()->syntax->groupItemData(data,QString("beginRegion"));
		QString endRegionStr=HlManager::self()->syntax->groupItemData(data,QString("endRegion"));

		signed char regionId=0;
		if (!beginRegionStr.isEmpty())
		{
			regionId=RegionList->findIndex(beginRegionStr);
			if (regionId==-1) // if the region name doesn't already exist, add it to the list
				{
					(*RegionList)<<beginRegionStr;
					regionId=RegionList->findIndex(beginRegionStr);
				}
		}
		else
		{
			if (!endRegionStr.isEmpty())
			regionId=RegionList->findIndex(endRegionStr);
			if (regionId==-1) // if the region name doesn't already exist, add it to the list
				{
					(*RegionList)<<endRegionStr;
					regionId=RegionList->findIndex(endRegionStr);
				}
			regionId=-regionId;
		}

		
                //Create the item corresponding to it's type and set it's parameters
                if (dataname=="keyword")
                {
                  HlKeyword *keyword=new HlKeyword(attr,context,regionId,casesensitive,
                     deliminatorChars, deliminatorLen);

                   //Get the entries for the keyword lookup list
                  keyword->addList(HlManager::self()->syntax->finddata("highlighting",stringdata));
                  return keyword;
                } else
                if (dataname=="Float") return (new HlFloat(attr,context,regionId)); else
                if (dataname=="Int") return(new HlInt(attr,context,regionId)); else
                if (dataname=="DetectChar") return(new HlCharDetect(attr,context,regionId,chr)); else
                if (dataname=="Detect2Chars") return(new Hl2CharDetect(attr,context,regionId,chr,chr1)); else
                if (dataname=="RangeDetect") return(new HlRangeDetect(attr,context,regionId, chr, chr1)); else
                if (dataname=="LineContinue") return(new HlLineContinue(attr,context,regionId)); else
                if (dataname=="StringDetect") return(new HlStringDetect(attr,context,regionId,stringdata,insensitive)); else
                if (dataname=="AnyChar") return(new HlAnyChar(attr,context,regionId,stringdata.unicode(), stringdata.length())); else
                if (dataname=="RegExpr") return(new HlRegExpr(attr,context,regionId,stringdata, insensitive, minimal)); else
                if(dataname=="HlCChar") return ( new HlCChar(attr,context,regionId));else
                if(dataname=="HlCHex") return (new HlCHex(attr,context,regionId));else
                if(dataname=="HlCOct") return (new HlCOct(attr,context,regionId)); else
                if(dataname=="HlCStringChar") return (new HlCStringChar(attr,context,regionId)); else

                  {
                    // oops, unknown type. Perhaps a spelling error in the xml file
                    return 0;
                  }


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
  const QChar *q = sq.unicode();
  return !ustrchr(deliminatorChars, deliminatorLen, c) && !ustrchr( q, 2, c);
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
  HlManager::self()->syntax->setIdentifier(identifier);

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
  HlManager::self()->syntax->setIdentifier(identifier);

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
     deliminatorChars = deliminator.unicode();
     deliminatorLen = deliminator.length();

     
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






void  Highlight::createContextNameList(QStringList *ContextNameList)
{
  syntaxContextData *data;

  kdDebug(13010)<<"creatingContextNameList:BEGIN"<<endl;

  ContextNameList->clear();

  HlManager::self()->syntax->setIdentifier(identifier);

  data=HlManager::self()->syntax->getGroupInfo("highlighting","context");

  int id=0;

  if (data)
  {
     while (HlManager::self()->syntax->nextGroup(data))    
     {
          QString tmpAttr=HlManager::self()->syntax->groupData(data,QString("name")).simplifyWhiteSpace();
	  if (tmpAttr.isEmpty()) tmpAttr=QString("!KATE_INTERNAL_DUMMY! %1").arg(id);
	  (*ContextNameList)<<tmpAttr;
          id++;
     }
     HlManager::self()->syntax->freeGroupInfo(data);
  }
  kdDebug(13010)<<"creatingContextNameList:END"<<endl;

}

int Highlight::getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext)
{
  int context;
  if (tmpLineEndContext=="#stay") context=-1;
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
      {
           context=ContextNameList->findIndex(tmpLineEndContext);
           if (context==-1) context=tmpLineEndContext.toInt();
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
  if (noHl)
    return;

  syntaxContextData *data, *datasub;
  HlItem *c;

  QStringList RegionList;
  QStringList ContextNameList;


  // Let the syntax document class know, which file we'd like to parse
  if (!HlManager::self()->syntax->setIdentifier(identifier))
  {
	noHl=true;
	KMessageBox::information(0L,i18n("Since there has been an error parsing the highlighting description, this highlighting will be disabled"));
	return;
  }

  RegionList<<"!KateInternal_TopLevel!";
  readCommentConfig();
  readGlobalKeywordConfig();


  // This list is needed for the translation of the attribute parameter, if the itemData name is given instead of the index
  ItemDataList iDl;
  createItemData(iDl);

  createContextNameList(&ContextNameList);

  kdDebug(13010)<<"Parsing Context structure"<<endl;
  //start the real work
  data=HlManager::self()->syntax->getGroupInfo("highlighting","context");
  uint i=0;
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

	  QString tmpLineEndContext=HlManager::self()->syntax->groupData(data,QString("lineEndContext")).simplifyWhiteSpace();
	  int context;

	  context=getIdFromString(&ContextNameList, tmpLineEndContext);

          // BEGIN get fallthrough props
          bool ft = false;
          int ftc = 0; // fallthrough context
          if ( i > 0 ) { // fallthrough is not smart in context 0
            QString tmpFt = HlManager::self()->syntax->groupData(data, QString("fallthrough") );
            if ( tmpFt.lower() == "true" ||  tmpFt.toInt() == 1 )
              ft = true;
            if ( ft ) {
              QString tmpFtc = HlManager::self()->syntax->groupData( data, QString("fallthroughContext") );

  	      ftc=getIdFromString(&ContextNameList, tmpFtc);
	      if (ftc == -1) ftc =0;

#if 0			// This shouldn't be needed anymore (jowenn)

              if ( ! tmpFtc.isEmpty() ) {
                //kdDebug(13010)<<"fallthgoughContext = "<<tmpFtc<<endl;
                if ( tmpFtc.startsWith("#pop") ) {
                  ftc = -1;
                  for ( ; tmpFtc.startsWith("#pop") ; ftc-- )
                    tmpFtc.remove( 0, 4 );
                }
                else
                  ftc = tmpFtc.toInt();
                if ( ftc == -1 ) // better make damned sure not, staying in a context that does not match any rule == infinite loop:(
                  ftc = 0;
              }
#endif
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
                if ( tag == "IncludeRules" ) {
                  // attrib context: the index (jowenn, i think using names here would be a cool feat, goes for mentioning the context in any item. a map or dict?)
                  int ctxId = HlManager::self()->syntax->groupItemData( data, QString("context") ).toInt(); // the index is *required*
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

}

HlManager::HlManager() : QObject(0)
{
  syntax = new SyntaxDocument();
  SyntaxModeList modeList = syntax->modeList();

  hlList.setAutoDelete(true);
  hlList.append(new Highlight(0));

  uint i=0;
  while (i < modeList.count())
  {
    hlList.append(new Highlight(modeList.at(i)));
    i++;
  }
}

HlManager::~HlManager() {
  if(syntax) delete syntax;
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

int HlManager::wildcardFind(const QString &fileName) {
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
  ItemStyleList defaultStyleList;
  ItemStyle *defaultStyle;
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
	  Attribute n;

    itemData = itemDataList.at(z);
    if (itemData->defStyle)
		{
      // default style
      defaultStyle = defaultStyleList.at(itemData->defStyleNum);
      n.col = defaultStyle->col;
      n.selCol = defaultStyle->selCol;
      n.bold = defaultStyle->bold;
      n.italic = defaultStyle->italic;
    }
		else
		{
      // custom style
      n.col = itemData->col;
      n.selCol = itemData->selCol;
      n.bold = itemData->bold;
      n.italic = itemData->italic;
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

void HlManager::getDefaults(ItemStyleList &list) {
  KConfig *config;
  int z;
  ItemStyle *i;
  QString s;
  QRgb col, selCol;

  list.setAutoDelete(true);
  //ItemStyle(color, selected color, bold, italic)
  list.append(new ItemStyle(black,white,false,false));     //normal
  list.append(new ItemStyle(black,white,true,false));      //keyword
  list.append(new ItemStyle(darkRed,white,false,false));   //datatype
  list.append(new ItemStyle(blue,cyan,false,false));       //decimal/value
  list.append(new ItemStyle(darkCyan,cyan,false,false));   //base n
  list.append(new ItemStyle(darkMagenta,cyan,false,false));//float
  list.append(new ItemStyle(magenta,magenta,false,false)); //char
  list.append(new ItemStyle(red,red,false,false));         //string
  list.append(new ItemStyle(darkGray,gray,false,true));    //comment
  list.append(new ItemStyle(darkGreen,green,false,false)); //others

  config = KateFactory::instance()->config();
  config->setGroup("Default Item Styles");
  for (z = 0; z < defaultStyles(); z++) {
    i = list.at(z);
    s = config->readEntry(defaultStyleName(z));
    if (!s.isEmpty()) {
      sscanf(s.latin1(),"%X,%X,%d,%d",&col,&selCol,&i->bold,&i->italic);
      i->col.setRgb(col);
      i->selCol.setRgb(selCol);
    }
  }
}

void HlManager::setDefaults(ItemStyleList &list) {
  KConfig *config;
  int z;
  ItemStyle *i;
  char s[64];

  config =  KateFactory::instance()->config();
  config->setGroup("Default Item Styles");
  for (z = 0; z < defaultStyles(); z++) {
    i = list.at(z);
    sprintf(s,"%X,%X,%d,%d",i->col.rgb(),i->selCol.rgb(),i->bold, i->italic);
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

