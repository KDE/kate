/***************************************************************************
                          katehighlight.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
		                       (C) 2001 by Joseph Wenninger
    email                : cullmann@kde.org
		                       jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATE_HIGHLIGHT_H_
#define _KATE_HIGHLIGHT_H_

#include "kateglobal.h"
#include "katetextline.h"

#include <qptrlist.h>
#include <qdialog.h>
#include <kcolorbtn.h>
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

class HlItem {
  public:
    HlItem(int attribute, int context);
    virtual ~HlItem();
    virtual bool startEnable(QChar);
    virtual const QChar *checkHgl(const QChar *, int len, bool) = 0;
    virtual bool lineContinue(){return false;}

    QPtrList<HlItem> *subItems;
    int attr;
    int ctx;
};

class HlCharDetect : public HlItem {
  public:
    HlCharDetect(int attribute, int context, QChar);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
  protected:
    QChar sChar;
};

class Hl2CharDetect : public HlItem {
  public:
    Hl2CharDetect(int attribute, int context,  QChar ch1, QChar ch2);
   	Hl2CharDetect(int attribute, int context, const QChar *ch);

    virtual const QChar *checkHgl(const QChar *, int len, bool);
  protected:
    QChar sChar1;
    QChar sChar2;
};

class HlStringDetect : public HlItem {
  public:
    HlStringDetect(int attribute, int context, const QString &, bool inSensitive=false);
    virtual ~HlStringDetect();
    virtual const QChar *checkHgl(const QChar *, int len, bool);
  protected:
    const QString str;
    bool _inSensitive;
};

class HlRangeDetect : public HlItem {
  public:
    HlRangeDetect(int attribute, int context, QChar ch1, QChar ch2);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
  protected:
    QChar sChar1;
    QChar sChar2;
};

class HlKeyword : public HlItem
{
  public:
    HlKeyword(int attribute, int context,bool casesensitive, const QChar *deliminator, uint deliLen);
    virtual ~HlKeyword();

    virtual void addWord(const QString &);
    virtual void addList(const QStringList &);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool startEnable(QChar c);

  protected:
    QDict<bool> dict;
    bool _caseSensitive;
    const QChar *deliminatorChars;
    uint deliminatorLen;
};

class HlPHex : public HlItem {
  public:
    HlPHex(int attribute,int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};
class HlInt : public HlItem {
  public:
    HlInt(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool startEnable(QChar c);

};

class HlFloat : public HlItem {
  public:
    HlFloat(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool startEnable(QChar c);
};

class HlCInt : public HlInt {
  public:
    HlCInt(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlCOct : public HlItem {
  public:
    HlCOct(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlCHex : public HlItem {
  public:
    HlCHex(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlCFloat : public HlFloat {
  public:
    HlCFloat(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    const QChar *checkIntHgl(const QChar *, int, bool);
};

class HlLineContinue : public HlItem {
  public:
    HlLineContinue(int attribute, int context);
    virtual bool endEnable(QChar c) {return c == '\0';}
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    virtual bool lineContinue(){return true;}
};

class HlCStringChar : public HlItem {
  public:
    HlCStringChar(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlCChar : public HlItem {
  public:
    HlCChar(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
};

class HlAnyChar : public HlItem {
  public:
    HlAnyChar(int attribute, int context, const QChar* charList, uint len);
    virtual const QChar *checkHgl(const QChar *, int len, bool);
    const QChar* _charList;
    uint _charListLen;
};

class HlRegExpr : public HlItem {
  public:
  HlRegExpr(int attribute, int context,QString expr);
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
    HlContext(int attribute, int lineEndContext,int _lineBeginContext);

    QPtrList<HlItem> items;
    int attr;
    int ctx;
    int lineBeginContext;
};

class Highlight
{
  friend class HlManager;

  public:
    Highlight(const syntaxModeListItem *def);
    ~Highlight();

    void doHighlight(signed char *oCtx, uint oCtxLen, TextLine *,bool lineContinue);

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
    void use();
    void release();
    bool isInWord(QChar c);

    QString getCommentStart() {return cmlStart;};
    QString getCommentEnd()  {return cmlEnd;};
    QString getCommentSingleLineStart() { return cslStart;};

  protected:
    void init();
    void done();
    void makeContextList ();
    void createItemData (ItemDataList &list);
    void readGlobalKeywordConfig();
    void readCommentConfig();

    signed char *generateContextStack(int *ctxNum, int ctx,signed char *ctxs, uint *ctxsLen, int *posPrevLine,bool lineContinue=false);

    HlItem *createHlItem(struct syntaxContextData *data, ItemDataList &iDl);
    int lookupAttrName(const QString& name, ItemDataList &iDl);
    ItemDataList internalIDList;


    static const int nContexts = 32;
    HlContext *contextList[nContexts];

    bool noHl;
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
    int refCount;
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

  protected:
    QPtrList<Highlight> hlList;
    static HlManager *s_pSelf;
    static KConfig *s_pConfig;
};




#endif //_HIGHLIGHT_H_
