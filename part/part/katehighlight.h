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

#ifndef __KATE_HIGHLIGHT_H__
#define __KATE_HIGHLIGHT_H__

#include "katetextline.h"
#include "kateattribute.h"

#include "../interfaces/document.h"

#include <kconfig.h>

#include <qptrlist.h>
#include <qvaluelist.h>
#include <qregexp.h>
#include <qdict.h>
#include <qintdict.h>
#include <qmap.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qguardedptr.h>

class KateHlContext;
class KateHlItem;
class KateHlItemData;
class KateHlData;
class KateEmbeddedHlInfo;
class KateHlIncludeRule;
class KateSyntaxDocument;
class KateTextLine;
class KateSyntaxModeListItem;
class KateSyntaxContextData;

class QPopupMenu;

// some typedefs
typedef QPtrList<KateAttribute> KateAttributeList;
typedef QValueList<KateHlIncludeRule*> KateHlIncludeRules;
typedef QPtrList<KateHlItemData> KateHlItemDataList;
typedef QPtrList<KateHlData> KateHlDataList;
typedef QMap<QString,KateEmbeddedHlInfo> KateEmbeddedHlInfos;
typedef QMap<int*,QString> KateHlUnresolvedCtxRefs;

//Item Properties: name, Item Style, Item Font
class KateHlItemData : public KateAttribute
{
  public:
    KateHlItemData(const QString  name, int defStyleNum);

    enum ItemStyles {
      dsNormal,
      dsKeyword,
      dsDataType,
      dsDecVal,
      dsBaseN,
      dsFloat,
      dsChar,
      dsString,
      dsComment,
      dsOthers,
      dsAlert,
      dsFunction,
      dsRegionMarker };

  public:
    const QString name;
    int defStyleNum;
};

class KateHlData
{
  public:
    KateHlData(const QString &wildcards, const QString &mimetypes,const QString &identifier, int priority);

  public:
    QString wildcards;
    QString mimetypes;
    QString identifier;
    int priority;
};

class KateHighlighting
{
  public:
    KateHighlighting(const KateSyntaxModeListItem *def);
    ~KateHighlighting();

  public:
    void doHighlight ( KateTextLine *prevLine,
                       KateTextLine *textLine,
                       QMemArray<signed char> *foldingList,
                       bool *ctxChanged );

    void loadWildcards();
    QValueList<QRegExp>& getRegexpExtensions();
    QStringList& getPlainExtensions();

    QString getMimetypes();

    // this pointer needs to be deleted !!!!!!!!!!
    KateHlData *getData();
    void setData(KateHlData *);

    void setKateHlItemDataList(uint schema, KateHlItemDataList &);

    // both methodes return hard copies of the internal lists
    // the lists are cleared first + autodelete is set !
    // keep track that you delete them, or mem will be lost
    void getKateHlItemDataListCopy (uint schema, KateHlItemDataList &);

    inline QString name() const {return iName;}
    inline QString section() const {return iSection;}
    inline QString version() const {return iVersion;}
    QString author () const { return iAuthor; }
    QString license () const { return iLicense; }
    int priority();
    inline QString getIdentifier() const {return identifier;}
    void use();
    void release();
    bool isInWord(QChar c);

    inline QString getCommentStart() const {return cmlStart;};
    inline QString getCommentEnd()  const {return cmlEnd;};
    inline QString getCommentSingleLineStart() const { return cslStart;};

    void clearAttributeArrays ();

    QMemArray<KateAttribute> *attributes (uint schema);

    inline bool noHighlighting () const { return noHl; };

  private:
    // make this private, nobody should play with the internal data pointers
    void getKateHlItemDataList(uint schema, KateHlItemDataList &);

    void init();
    void done();
    void makeContextList ();
    void handleKateHlIncludeRules ();
    void handleKateHlIncludeRulesRecursive(KateHlIncludeRules::iterator it, KateHlIncludeRules *list);
    int addToContextList(const QString &ident, int ctx0);
    void addToKateHlItemDataList();
    void createKateHlItemData (KateHlItemDataList &list);
    void readGlobalKeywordConfig();
    void readCommentConfig();
    void readFoldingConfig ();

    // manipulates the ctxs array directly ;)
    void generateContextStack(int *ctxNum, int ctx, QMemArray<short> *ctxs, int *posPrevLine,bool lineContinue=false);

    KateHlItem *createKateHlItem(struct KateSyntaxContextData *data, KateHlItemDataList &iDl, QStringList *RegionList, QStringList *ContextList);
    int lookupAttrName(const QString& name, KateHlItemDataList &iDl);

    void createContextNameList(QStringList *ContextNameList, int ctx0);
    int getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext,/*NO CONST*/ QString &unres);

    KateHlItemDataList internalIDList;

    QIntDict<KateHlContext> contextList;
    inline KateHlContext *contextNum (uint n) { return contextList[n]; }

    // make them pointers perhaps
    KateEmbeddedHlInfos embeddedHls;
    KateHlUnresolvedCtxRefs unresolvedContextReferences;
    QStringList RegionList;
    QStringList ContextNameList;

    bool noHl;
    bool folding;
    bool casesensitive;
    QString weakDeliminator;
    QString deliminator;

    QString cmlStart;
    QString cmlEnd;
    QString cslStart;
    QString iName;
    QString iSection;
    QString iWildcards;
    QString iMimetypes;
    QString identifier;
    QString iVersion;
    QString iAuthor;
    QString iLicense;
    int m_priority;
    int refCount;

    QString errorsAndWarnings;
    QString buildIdentifier;
    QString buildPrefix;
    bool building;
    uint itemData0;
    uint buildContext0Offset;
    KateHlIncludeRules includeRules;
    QValueList<int> contextsIncludingSomething;
    bool m_foldingIndentationSensitive;

    QIntDict< QMemArray<KateAttribute> > m_attributeArrays;

    QString extensionSource;
    QValueList<QRegExp> regexpExtensions;
    QStringList plainExtensions;

  public:
    inline bool foldingIndentationSensitive () { return m_foldingIndentationSensitive; }
    inline bool allowsFolding(){return folding;}
};

class KateHlManager : public QObject
{
  Q_OBJECT

  private:
    KateHlManager();

  public:
    ~KateHlManager();

    static KateHlManager *self();

    inline KConfig *getKConfig() { return &m_config; };

    KateHighlighting *getHl(int n);
    int nameFind(const QString &name);

    int detectHighlighting (class KateDocument *doc);

    int findHl(KateHighlighting *h) {return hlList.find(h);}
    QString identifierForName(const QString&);

    // methodes to get the default style count + names
    static uint defaultStyles();
    static QString defaultStyleName(int n);

    void getDefaults(uint schema, KateAttributeList &);
    void setDefaults(uint schema, KateAttributeList &);

    int highlights();
    QString hlName(int n);
    QString hlSection(int n);

  signals:
    void changed();

  private:
    int wildcardFind(const QString &fileName);
    int mimeFind(KateDocument *);
    int realWildcardFind(const QString &fileName);

  private:
    friend class KateHighlighting;

    QPtrList<KateHighlighting> hlList;
    QDict<KateHighlighting> hlDict;

    static KateHlManager *s_self;

    KConfig m_config;
    QStringList commonSuffixes;

    KateSyntaxDocument *syntax;
};

class KateViewHighlightAction: public Kate::ActionMenu
{
  Q_OBJECT

  public:
    KateViewHighlightAction(const QString& text, QObject* parent = 0, const char* name = 0)
       : Kate::ActionMenu(text, parent, name) { init(); };

    ~KateViewHighlightAction(){;};

    void updateMenu (Kate::Document *doc);

  private:
    void init();

    QGuardedPtr<Kate::Document> m_doc;
    QStringList subMenusName;
    QStringList names;
    QPtrList<QPopupMenu> subMenus;

  public  slots:
    void slotAboutToShow();

  private slots:
    void setHl (int mode);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
