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

#ifndef __KATE_HIGHLIGHT_H__
#define __KATE_HIGHLIGHT_H__

#include "katetextline.h"
#include <ktexteditor/attribute.h>

#include <kconfig.h>
#include <kaction.h>

#include <QVector>
#include <QList>
#include <QHash>
#include <QMap>

#include <q3ptrlist.h>
#include <q3valuelist.h>
#include <q3valuevector.h>
#include <qregexp.h>
#include <q3dict.h>
#include <q3intdict.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qpointer.h>
#include <qdatetime.h>
#include <QLinkedList>
#include <QRegExp>
#include <QAction>

class KateHlContext;
class KateHlItem;
class KateHlItemData;
class KateHlData;
class KateHlIncludeRule;
class KateSyntaxDocument;
class KateTextLine;
class KateSyntaxModeListItem;
class KateSyntaxContextData;

class QMenu;

class KateEmbeddedHlInfo
{
  public:
    KateEmbeddedHlInfo() {loaded=false;context0=-1;}
    KateEmbeddedHlInfo(bool l, int ctx0) {loaded=l;context0=ctx0;}

  public:
    bool loaded;
    int context0;
};

// some typedefs
typedef Q3PtrList<KTextEditor::Attribute> KateAttributeList;
typedef Q3ValueList<KateHlIncludeRule*> KateHlIncludeRules;
typedef QList<KateHlItemData*> KateHlItemDataList;
typedef QMap<QString,KateEmbeddedHlInfo> KateEmbeddedHlInfos;
typedef QMap<int*,QString> KateHlUnresolvedCtxRefs;

//Item Properties: name, Item Style, Item Font
class KateHlItemData : public KTextEditor::Attribute
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

    void clear();

  public:
    const QString name;
    int defStyleNum;
};

class KateHlData
{
  public:
    KateHlData(const QString &wildcards, const QString &mimetypes,const QString &identifier, int priority);
    KateHlData();
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

  private:
    /**
     * this method frees mem ;)
     * used by the destructor and done(), therefor
     * not only delete elements but also reset the array
     * sizes, as we will reuse this object later and refill ;)
     */
    void cleanup ();

  public:
    void doHighlight ( KateTextLine *prevLine,
                       KateTextLine *textLine,
                       QVector<int> *foldingList,
                       bool *ctxChanged );

    void loadWildcards();
    QList<QRegExp>& getRegexpExtensions();
    QStringList& getPlainExtensions();

    QString getMimetypes();

    KateHlData getData();
    void setData(const KateHlData&);

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
     *
     */
    QLinkedList<QRegExp> emptyLines(int attribute=0) const;
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
     * This enum is used for storing the information where a single line comment marker should be inserted
     */
    enum CSLPos { CSLPosColumn0=0,CSLPosAfterWhitespace=1};

    /**
     * @return the single comment marker position for the highlight corresponding
     * to @p attrib.
     */
    CSLPos getCommentSingleLinePosition( int attrib=0 ) const;

    /**
    * @return the attribute for @p context.
    */
    int attribute( int context ) const;

    /**
     * map attribute to its highlighting file.
     * the returned string is used as key for m_additionalData.
     */
    QString hlKeyForAttrib( int attrib ) const;


    void clearAttributeArrays ();

    QVector<KTextEditor::Attribute> *attributes (uint schema);

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
    void readGlobalKeywordConfig();
    void readWordWrapConfig();
    void readCommentConfig();
    void readEmptyLineConfig();
    void readIndentationConfig ();
    void readFoldingConfig ();

    // manipulates the ctxs array directly ;)
    void generateContextStack(int *ctxNum, int ctx, QVector<short> *ctxs, int *posPrevLine);

    KateHlItem *createKateHlItem(KateSyntaxContextData *data, KateHlItemDataList &iDl, QStringList *RegionList, QStringList *ContextList);
    int lookupAttrName(const QString& name, KateHlItemDataList &iDl);

    void createContextNameList(QStringList *ContextNameList, int ctx0);
    int getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext,/*NO CONST*/ QString &unres);

    KateHlItemDataList internalIDList;

    QVector<KateHlContext*> m_contexts;
    inline KateHlContext *contextNum (int n) { if (n >= 0 && n < m_contexts.size()) return m_contexts[n]; return 0; }

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

    QHash< int, QVector<KTextEditor::Attribute> * > m_attributeArrays;


    /**
     * This class holds the additional properties for one highlight
     * definition, such as comment strings, deliminators etc.
     *
     * When a highlight is added, a instance of this class is appended to
     * m_additionalData, and the current position in the attrib and context
     * arrays are stored in the indexes for look up. You can then use
     * hlKeyForAttrib or hlKeyForContext to find the relevant instance of this
     * class from m_additionalData.
     *
     * If you need to add a property to a highlight, add it here.
     */
    class HighlightPropertyBag {
      public:
        QString singleLineCommentMarker;
        QString multiLineCommentStart;
        QString multiLineCommentEnd;
        QString multiLineRegion;
        CSLPos  singleLineCommentPosition;
        QString deliminator;
        QString wordWrapDeliminator;
	QLinkedList<QRegExp> emptyLines;
    };

    /**
     * Highlight properties for each included highlight definition.
     * The key is the identifier
     */
    QHash<QString, HighlightPropertyBag*> m_additionalData;

    /**
     * Fast lookup of hl properties, based on attribute index
     * The key is the starting index in the attribute array for each file.
     * @see hlKeyForAttrib
     */
    QMap<int, QString> m_hlIndex;

    QString extensionSource;
    QList<QRegExp> regexpExtensions;
    QStringList plainExtensions;

  public:
    inline bool foldingIndentationSensitive () { return m_foldingIndentationSensitive; }
    inline bool allowsFolding(){return folding;}
};

class KateHlManager : public QObject
{
  Q_OBJECT

  public:
    KateHlManager();
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
    int countDynamicCtxs() { return dynamicCtxsCount; };
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

    Q3PtrList<KateHighlighting> hlList;
    Q3Dict<KateHighlighting> hlDict;

    KConfig m_config;
    QStringList commonSuffixes;

    KateSyntaxDocument *syntax;

    int dynamicCtxsCount;
    QTime lastCtxsReset;
    bool forceNoDCReset;
};

class KateViewHighlightAction: public KActionMenu
{
  Q_OBJECT

  public:
    KateViewHighlightAction(const QString& text, KActionCollection *parent = 0, const char* name = 0)
       : KActionMenu(text, parent, name) { init(); };

    ~KateViewHighlightAction();

    void updateMenu (KateDocument *doc);

  private:
    void init();

    QPointer<KateDocument> m_doc;
    QStringList subMenusName;
    QStringList names;
    QList<QMenu*> subMenus;
    QList<QAction*> subActions;
  public  slots:
    void slotAboutToShow();

  private slots:
    void setHl ();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
