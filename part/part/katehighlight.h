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

#ifndef _KATE_HIGHLIGHT_H_
#define _KATE_HIGHLIGHT_H_

#include "katetextline.h"

#include <qptrlist.h>
#include <qvaluelist.h>
#include <qdialog.h>
#include <kcolorbutton.h>
#include <qregexp.h>
#include <kdebug.h>
#include <qdict.h>
#include <qintdict.h>


class SyntaxDocument;
struct syntaxModeListItem;
struct syntaxContextData;

class QCheckBox;
class QComboBox;
class QLineEdit;

class TextLine;
class Attribute;

class QStringList;

class HlItem {
  public:
    HlItem(int attribute, int context,signed char regionId);
    virtual ~HlItem();
    virtual bool startEnable(QChar);
    virtual const QChar *checkHgl(const QChar *, int len, bool) = 0;
    virtual bool lineContinue(){return false;}

    QPtrList<HlItem> *subItems;
    int attr;
    int ctx;
    signed char region;
};

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


//Item Style: color, selected color, bold, italic
class ItemStyle {
  public:
    ItemStyle();
//    ItemStyle(const ItemStyle &);
    ItemStyle(const QColor &, const QColor &, bool bold, bool italic);
    ItemStyle(ItemStyle *its){col=its->col;selCol=its->selCol; bold=its->bold; italic=its->italic;}
//    void setData(const ItemStyle &);
    QColor col;
    QColor selCol;
    int bold;   //boolean value
    int italic; //boolean value
};

typedef QPtrList<ItemStyle> ItemStyleList;

//Item Properties: name, Item Style, Item Font
class ItemData : public ItemStyle {
  public:
    ItemData(const QString  name, int defStyleNum);
    ItemData(const QString  name, int defStyleNum,
      const QColor&, const QColor&, bool bold, bool italic);
    ItemData(ItemData
*itd):ItemStyle((ItemStyle*)itd),name(itd->name),defStyleNum(itd->defStyleNum),defStyle(itd->defStyle){;}
    const QString name;
    int defStyleNum;
    int defStyle; //boolean value
};

typedef QPtrList<ItemData> ItemDataList;

class HlData {
  public:
    HlData(const QString &wildcards, const QString &mimetypes,const QString &identifier);
    ItemDataList itemDataList;
    QString wildcards;
    QString mimetypes;
    QString identifier;
};

typedef QPtrList<HlData> HlDataList;

class HlManager;
class KConfig;

//context
class HlContext {
  public:
    HlContext (int attribute, int lineEndContext,int _lineBeginContext,
               bool _fallthrough, int _fallthroughContext);

    QPtrList<HlItem> items;
    int attr;
    int ctx;
    int lineBeginContext;
    /** @internal anders: possible escape if no rules matches.
       false unless 'fallthrough="1|true"' (insensitive)
       if true, go to ftcxt w/o eating of string.
       ftctx is "fallthroughContext" in xml files, valid values are int or #pop[..]
       see in Highlight::doHighlight */
    bool fallthrough;
    int ftctx; // where to go after no rules matched
};

class Highlight
{
  friend class HlManager;

  public:
    Highlight(const syntaxModeListItem *def);
    ~Highlight();

    void doHighlight(QMemArray<signed char> oCtx, TextLine *,bool lineContinue,QMemArray<signed char> *foldingList);

    KConfig *getKConfig();
    QString getWildcards();
    QString getMimetypes();
    HlData *getData();
    void setData(HlData *);
    void getItemDataList(ItemDataList &);
    void getItemDataList(ItemDataList &, KConfig *);
    void setItemDataList(ItemDataList &, KConfig *);
    QString name() {return iName;}
    QString section() {return iSection;}
    QString version() {return iVersion;}
    void use();
    void release();
    bool isInWord(QChar c);

    QString getCommentStart() {return cmlStart;};
    QString getCommentEnd()  {return cmlEnd;};
    QString getCommentSingleLineStart() { return cslStart;};

  private:
    void init();
    void done();
    void makeContextList ();
    void createItemData (ItemDataList &list);
    void readGlobalKeywordConfig();
    void readCommentConfig();

    // manipulates the ctxs array directly ;)
    void generateContextStack(int *ctxNum, int ctx, QMemArray<signed char> *ctxs, int *posPrevLine,bool lineContinue=false);

    HlItem *createHlItem(struct syntaxContextData *data, ItemDataList &iDl, QStringList *RegionList, QStringList *ContextList);
    int lookupAttrName(const QString& name, ItemDataList &iDl);

    void createContextNameList(QStringList *ContextNameList);
    int getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext);

    ItemDataList internalIDList;

    QIntDict<HlContext> contextList;
    HlContext *contextNum (uint n);

    bool noHl;
    bool folding;
    bool casesensitive;
    QString weakDeliminator;
    QString deliminator;
    const QChar *deliminatorChars;
    uint deliminatorLen;
    QString cmlStart;
    QString cmlEnd;
    QString cslStart;
    QString iName;
    QString iSection;
    QString iWildcards;
    QString iMimetypes;
    QString identifier;
    QString iVersion;
    int refCount;

    public:
    	bool allowsFolding(){return folding;}
};

class HlManager : public QObject {
    Q_OBJECT
  public:
    HlManager();
    ~HlManager();

    static HlManager *self();
    static KConfig *getKConfig();
    Highlight *getHl(int n);
    int defaultHl();
    int nameFind(const QString &name);

    int wildcardFind(const QString &fileName);
    int mimeFind(const QByteArray &contents, const QString &fname);
    int findHl(Highlight *h) {return hlList.find(h);}

    void makeAttribs(class KateDocument *, Highlight *);

    int defaultStyles();
    QString defaultStyleName(int n);
    void getDefaults(ItemStyleList &);
    void setDefaults(ItemStyleList &);

    int highlights();
    QString hlName(int n);
    QString hlSection(int n);
    void getHlDataList(HlDataList &);
    void setHlDataList(HlDataList &);

    SyntaxDocument *syntax;

  signals:
    void changed();

  private:
    QPtrList<Highlight> hlList;
    static HlManager *s_pSelf;
    static KConfig *s_pConfig;
};




#endif //_HIGHLIGHT_H_
