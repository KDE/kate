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
#include <qvaluevector.h>
#include <qregexp.h>
#include <qdict.h>
#include <qintdict.h>
#include <qmap.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qguardedptr.h>
#include <qdatetime.h>

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
typedef QValueList<int> IntList;

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
      dsRegionMarker,
      dsError };

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
                       QMemArray<uint> *foldingList,
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

    const QString &name() const {return iName;}
    const QString &nameTranslated() const {return iNameTranslated;}
    const QString &section() const {return iSection;}
    bool hidden() const {return iHidden;}
    const QString &version() const {return iVersion;}
    const QString &author () const { return iAuthor; }
    const QString &license () const { return iLicense; }
    int priority();
    const QString &getIdentifier() const {return identifier;}
    void use();
    void release();

    /**
     * @return true if the character @p c is not a deliminator character
     *     for the corresponding highlight.
     */
    bool isInWord( QChar c, int attrib=0 ) const;

    /**
     * @return true if the character @p c is a wordwrap deliminator as specified
     * in the general keyword section of the xml file.
     */
    bool canBreakAt( QChar c, int attrib=0 ) const;

    /**
    * @return true if @p beginAttr and @p endAttr are members of the same
    * highlight, and there are comment markers of either type in that.
    */
    bool canComment( int startAttr, int endAttr ) const;

    /**
    * @return 0 if highlighting which attr is a member of does not
    * define a comment region, otherwise the region is returned
    */
    signed char commentRegion(int attr) const;

    /**
     * Define comment marker type.
     */
    enum commentData { Start, End, MultiLineRegion, SingleLine };

    /**
     * @return the comment marker @p which for the highlight corresponding to
     *         @p attrib.
     */
    QString getCommentString( int which, int attrib ) const;

    /**
     * @return the mulitiline comment start marker for the highlight
     * corresponding to @p attrib.
     */
    QString getCommentStart( int attrib=0 ) const;

    /**
     * @return the muiltiline comment end marker for the highlight corresponding
     * to @p attrib.
     */
    QString getCommentEnd( int attrib=0 ) const;

    /**
     * @return the single comment marker for the highlight corresponding
     * to @p attrib.
     */
    QString getCommentSingleLineStart( int attrib=0 ) const;

    /**
    * @return the attribute for @p context.
    */
    int attribute( int context ) const;

    void clearAttributeArrays ();

    QMemArray<KateAttribute> *attributes (uint schema);

    inline bool noHighlighting () const { return noHl; };

    // be carefull: all documents hl should be invalidated after calling this method!
    void dropDynamicContexts();

    QString indentation () { return m_indentation; }

  private:
    // make this private, nobody should play with the internal data pointers
    void getKateHlItemDataList(uint schema, KateHlItemDataList &);

    void init();
    void done();
    void makeContextList ();
    int makeDynamicContext(KateHlContext *model, const QStringList *args);
    void handleKateHlIncludeRules ();
    void handleKateHlIncludeRulesRecursive(KateHlIncludeRules::iterator it, KateHlIncludeRules *list);
    int addToContextList(const QString &ident, int ctx0);
    void addToKateHlItemDataList();
    void createKateHlItemData (KateHlItemDataList &list);
    QString readGlobalKeywordConfig();
    QString readWordWrapConfig();
    QStringList readCommentConfig();
    void readIndentationConfig ();
    void readFoldingConfig ();

    // manipulates the ctxs array directly ;)
    void generateContextStack(int *ctxNum, int ctx, QMemArray<short> *ctxs, int *posPrevLine);

    KateHlItem *createKateHlItem(KateSyntaxContextData *data, KateHlItemDataList &iDl, QStringList *RegionList, QStringList *ContextList);
    int lookupAttrName(const QString& name, KateHlItemDataList &iDl);

    void createContextNameList(QStringList *ContextNameList, int ctx0);
    int getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext,/*NO CONST*/ QString &unres);

    /**
    * @return the key to use for @p attrib in m_additionalData.
    */
    int hlKeyForAttrib( int attrib ) const;

    KateHlItemDataList internalIDList;

    QValueVector<KateHlContext*> m_contexts;
    inline KateHlContext *contextNum (uint n) { if (n < m_contexts.size()) return m_contexts[n]; return 0; }

    QMap< QPair<KateHlContext *, QString>, short> dynamicCtxs;

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

    QString iName;
    QString iNameTranslated;
    QString iSection;
    bool iHidden;
    QString iWildcards;
    QString iMimetypes;
    QString identifier;
    QString iVersion;
    QString iAuthor;
    QString iLicense;
    QString m_indentation;
    int m_priority;
    int refCount;
    int startctx, base_startctx;

    QString errorsAndWarnings;
    QString buildIdentifier;
    QString buildPrefix;
    bool building;
    uint itemData0;
    uint buildContext0Offset;
    KateHlIncludeRules includeRules;
    bool m_foldingIndentationSensitive;

    QIntDict< QMemArray<KateAttribute> > m_attributeArrays;

    /**
     * This contains a list of comment data, the deliminator string and
     * wordwrap deliminator pr highlight.
     * The key is the highlights entry position in internalIDList.
     * This is used to look up the correct comment and delimitor strings
     * based on the attrtibute.
     */
    QMap<int, QStringList> m_additionalData;

    /**
     * fast lookup of hl properties.
     */
    IntList m_hlIndex;

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
    static QString defaultStyleName(int n, bool translateNames = false);

    void getDefaults(uint schema, KateAttributeList &);
    void setDefaults(uint schema, KateAttributeList &);

    int highlights();
    QString hlName(int n);
    QString hlNameTranslated (int n);
    QString hlSection(int n);
    bool hlHidden(int n);

    void incDynamicCtxs() { ++dynamicCtxsCount; };
    uint countDynamicCtxs() { return dynamicCtxsCount; };
    void setForceNoDCReset(bool b) { forceNoDCReset = b; };

    // be carefull: all documents hl should be invalidated after having successfully called this method!
    bool resetDynamicCtxs();

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

    uint dynamicCtxsCount;
    QTime lastCtxsReset;
    bool forceNoDCReset;
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
