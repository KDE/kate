/* This file is part of the KDE libraries
   Copyright (C) 2001,2005 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_CODECOMPLETIONINTERFACE_H
#define KDELIBS_KTEXTEDITOR_CODECOMPLETIONINTERFACE_H

#include <qstring.h>
#include <qstringlist.h>
#include <kdelibs_export.h>
#include <QVariant>
#include <QIcon>
#include <QSharedDataPointer>
#include <ktexteditor/cursor.h>
#include <kdebug.h>
namespace KTextEditor
{

class View;
class CompletionProvider;

/**
 * An item for the completion popup. <code>text</code> is the completed string,
 * <code>prefix</code> appears in front of it, <code>suffix</code> appears after it.
 * <code>type</code> does not appear in the completion list.
 * <code>prefix</code>, <code>suffix</code>, and <code>type</code> are not part of the
 * inserted text if a completion takes place. <code>comment</code> appears in a tooltip right of
 * the completion list for the currently selected item. <code>userdata</code> can be
 * free formed data, which the user of this interface can use in
 * CodeCompletionInterface::filterInsertString().
 *
 *
 */
//possible better d pointer + functions
class KTEXTEDITOR_EXPORT CompletionItem
{
  public:
    CompletionItem();
    CompletionItem(const QString& _text, const QIcon &_icon=QIcon(), CompletionProvider* _provider=0, const QString &_prefix=QString(), const QString& _postfix=QString(), const QString& _comment=QString(), const QVariant &_userdata=QVariant(), const QString &_type=QString());
    ~CompletionItem();
    const QIcon&    icon() const;
    const QString&  text() const;
    const QString&  markupText() const;
    const QString&  prefix() const;
    const QString&  postfix() const;
    const QString&  comment() const;
    const QVariant& userdata() const;
    CompletionProvider *provider() const;//must not be set to a provider, instead of 0, if the provider doesn't support the doComplete method or doesn't want to handle the item itself


    CompletionItem operator=(const CompletionItem &c);
    CompletionItem (const CompletionItem &c);
    bool operator==( const CompletionItem &c ) const;
  private:
    class Private;
    QSharedDataPointer<Private> d;
};



/**
 * Information about matching code completion data.
 *
 * @author Joseph Wenninger \<jowenn@kde.org\>
 */
class KTEXTEDITOR_EXPORT CompletionData
{
  public:
    /**
     * Constructor.
     * You should never use this constructor yourself.
     */
    CompletionData():m_id(0) {}
    /**
     * Constructor.
     *
     * Create a new completion data instance with @p items and cursorposition
     * @p matchStart.
     * @param items the items containing the completion information
     * @param matchStart start position
     * @param casesensitive FIXME ?
     */
    CompletionData(QList<CompletionItem> items,
                   const KTextEditor::Cursor& matchStart,
                   bool casesensitive)
      : m_items(items),
        m_matchStart(matchStart),
        m_casesensitive(casesensitive),
        m_id(((++s_id)==0)?(++s_id):s_id)
    {}

    /**
     * Get all the completion data's items.
     * @return all items
     */
    inline const QList<CompletionItem>& items()const {return m_items;}

    /**
     * Get the start position.
     * @return the start position
     */
    inline const KTextEditor::Cursor& matchStart() const {return m_matchStart;}

    /**
     * Check, whether comparisons should be case sensitive or case
     * insensitive.
     * @return @e true, if the comparison is case sensitive, otherwise
     *         @e false
     */
    inline bool casesensitive() const {return m_casesensitive;}

    /**
     * Check item @p d for equality.
     * @param d comparison completion data item
     * @return @e true, d.id() == this->id(), otherwise @e false
     */
    inline bool operator==( const CompletionData &d ) const { kdDebug()<<"Checking equality"<<endl;return m_id==d.m_id;}

    /**
     * Static accessor for an empty completion data list.
     * Return CompletionData::Null() whenever you do not have any completion
     * data.
     * @return an empty completion data instance
     * @see KTextEditor::Provider::completionData()
     */
    inline static const CompletionData Null() {return CompletionData();}

    /**
     * Check the completion data's validity.
     * @return @e true, if the data is valid, otherwise @e false
     */
    inline bool isValid()const {return m_id!=0;}

    /**
     * Get the id.
     * @return the id
     */
    inline int id() const {return m_id;};

  private:
    QList<CompletionItem> m_items;
    Cursor m_matchStart;
    bool m_casesensitive;
    long m_id;
    static long s_id;
};


/**
 * Argument hint class containing information about argument hints.
 *
 * @author Joseph Wenninger \<jowenn@kde.org\>
 */
class KTEXTEDITOR_EXPORT ArgHintData
{
  public:
    /**
     * Constructor.
     * You should never use this constructor yourself.
     */
    ArgHintData():m_id(0) {}

    /**
     * Constructor.
     *
     */
    ArgHintData(const QString& wrapping,
                const QString& delimiter,
                const QStringList& items)
      : m_wrapping(wrapping),
        m_delimiter(delimiter),
        m_items(items),
        m_id(((++s_id)==0)?(++s_id):s_id)
    {}

    /**
     *
     */
    inline const QString& wrapping() const {return m_wrapping;}
    /**
     *
     */
    inline const QString& delimiter() const {return m_delimiter;}
    /**
     *
     */
    inline const QStringList& items() const {return m_items;}
    /**
     *
     */
    inline bool operator==( const ArgHintData &d ) const { return m_id==d.m_id;}
    /**
     *
     */
    inline static const ArgHintData Null() {return ArgHintData();}

  private:
    QString m_wrapping;
    QString m_delimiter;
    QStringList m_items;
    long m_id;
    static long s_id;
};

    enum CompletionType
    {
      /*standard types*/
      CompletionType0 =0x00000000,
      CompletionType1 =0x00000001,
      CompletionType2 =0x00000002,
      CompletionType3 =0x00000004,
      CompletionType4 =0x00000008,
      CompletionType5 =0x00000010,
      /*generic single provider invocation helpers (free for usage)*/
      CompletionType6 =0x00000020,
      CompletionType6a =0x00000060,
      CompletionType6b =0x000000a0,
      CompletionType6c =0x00000120,
      CompletionType6d =0x00000220,
      CompletionType6e =0x00000420,
      CompletionType6f =0x00000820,
      CompletionType6g =0x00001020,
      CompletionType6h =0x00002020,
      CompletionType6i =0x00004020,
      CompletionType6j =0x00008020,
      CompletionType6k =0x00010020,
      CompletionType6l =0x00020020,
      CompletionType6m =0x00040020,
      CompletionType6n =0x00080020,
      CompletionType6o =0x00100020,
      CompletionType6p =0x00200020,
      CompletionType6q =0x00400020,
      CompletionType6r =0x00800020,
      CompletionType6s =0x01000020,
      CompletionType6t =0x02000020,
      CompletionType6u =0x04000020,
      CompletionType6v =0x08000020,
      CompletionType6w =0x10000020,
      CompletionType6x =0x20000020,
      CompletionType6y =0x40000020,
      CompletionType6z =0x80000020,
      /*reserved types */
      CompletionType7 =0x00000040,
      CompletionType8 =0x00000080,
      CompletionType9=0x00000100,
      CompletionType10=0x00000200,
      CompletionType11=0x00000400,
      CompletionType12=0x00000800,
      CompletionType13=0x00001000,
      CompletionType14=0x00002000,
      CompletionType15=0x00004000,
      CompletionType16=0x00008000,
      CompletionType17=0x00010000,
      CompletionType18=0x00020000,
      CompletionType19=0x00040000,
      CompletionType20=0x00080000,
      CompletionType21=0x00100000,
      CompletionType22=0x00200000,
      CompletionType23=0x00400000,
      CompletionType24=0x00800000,
      CompletionType25=0x01000000,
      CompletionType26=0x02000000,
      CompletionType27=0x04000000,
      CompletionType28=0x08000000,
      CompletionType29=0x10000000,
      CompletionType30=0x20000000,
      CompletionType31=0x40000000,
      CompletionType32=0x80000000,

      CompletionNone=CompletionType0,
      CompletionAsYouType=CompletionType1,
      CompletionAsYouTypeBackspace=CompletionType2,
      CompletionReinvokeAsYouType=CompletionType3, //there does not have to be a CompleteAsYouType before an invokation of that kind
      CompletionContextIndependent=CompletionType4,
      CompletionContextDependent=CompletionType5,
      CompletionGenericSingleProvider=CompletionType6 //recommend for the single provider invokeCompletion call, should never be used with the multiple provider enabled invokation call.
    };



/**
 * Code completion provider.
 *
 * <b>Introduction</b>\n
 *
 * A CompletionProvider is supposed to provide the data for code completion
 * and argument hints.
 *
 * Every provider is queried for completion data @e everytime a character
 * was inserted, you will know this in completionData() as the type then is
 * set to @p CompletionAsYouType. If the type for example is
 * @p CompletionGenericSingleProvider you can be sure that you are the only
 * provider, i.e. a provider knows that the invokation was forced.
 *
 * There are several other completion types, look into the
 * KTextEditor::CompletionType list for further details.
 *
 * <b>Implementation Notes</b>\n
 *
 * The provider should cache the completion data it initially created as long
 * as possible, i.e. as long as the word that was used to create the data did
 * not change.
 *
 * Similiar behaviour if a word part is removed would be desirable, but is at
 * least for kate not needed, since it doesn't support the
 * CompleteAsYouTypeBackspace. If a completion type greater than
 * CompleteReinvokeAsYouType is active, no further requests are sent out from
 * the editor, till the completion has been aborted or executed.
 *
 * @see KTextEditor::CodeCompletionInterface, KTextEditor::CompletionData,
 *      KTextEditor::CompletionType
 * @author Joseph Wenninger \<jowenn@kde.org\>
 */
class KTEXTEDITOR_EXPORT CompletionProvider
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~CompletionProvider(){;}

  public:
    /**
     * Return the completion data for the @p view.
     *
     * @p insertionPosition can only be assumed valid, if the completion type
     * is @e CompleteAsYouType and the inserted text is not empty.
     *
     * Return CompletionData::Null() whenever you do not have any completion
     * data, i.e. for empty lists.
     *
     * @param view the view that wants to show a completion box
     * @param completionData the completion type
     * @param insertionPosition the start position of the inserted text
     * @param insertedText the inserted text
     * @param currentPos the current cursor position
     * @param currentLine the whole text line
     *
     * @return a list of completion items
     */
    virtual const CompletionData completionData(View* view,
                                                enum CompletionType completionType,
                                                const Cursor& insertionPosition,
                                                const QString& insertedText,
                                                const Cursor& currentPos,
                                                const QString& currentLine)=0;

    /**
     * @todo
     */
    virtual const ArgHintData argHintData(View *view,
                                          const Cursor& cursor,
                                          const QString& text)=0;

    /**
     * This function is called whenever a completion process has been aborted.
     *
     * A provider that inserted data to the completion box for a completion
     * process should @e not assume that it is the only one who gets this
     * call, as in one completion box can be entries of several different
     * providers.
     *
     * @param view the view of the completion box
     */
    virtual void completionAborted(View *view)=0;

    /**
     * This function is called whenever a completion entry was chosen.
     * If a provider has cached data this is the place to clean it up.
     * @param view the view of the completion box
     */
    virtual void completionDone(View *view)=0;

    /**
     * Do the code completion by using the data given in @p item.
     *
     * This method is called only for the provider that provided the item, no
     * default handling will be done by the editor component, i.e. you have to
     * insert the text yourself.
     *
     * @param view the view of the completion box
     * @param data the completion data
     * @param item the chosen item, use it to do the completion
     */
    virtual void doComplete(View *view,
                            const CompletionData &data,
                            const CompletionItem &item)=0;
};

/**
 * Code completion extension interface for the View.
 *
 * <b>Introduction</b>\n
 *
 * The idea of code completion basically is to provide methods to
 *  - complete a partially written string by popping up a small listbox at the
 *    current cursor position showing all matches including an additional
 *    comment
 *  - show argument hints for functions.
 *
 * <b>Code Completion Architecture</b>\n
 *
 * It is possible that several clients want to access the code completion
 * interface, e.g. the Word completion Plugin and Quanta+. So it is important
 * that several clients do not clash and work smooth simultaneously.
 *
 * The solution is to install a so-called code completion @e provider. A
 * client has to register a new CompletionProvider by using
 * registerCompletionProvider(). To invoke the provider use invokeCompletion()
 * with the appropriate arguments.
 *
 * The provider itself then can control the completion and argument hint data.
 * This is visualized in the following hierarchy:
 * @image html ktexteditorcodecompletion "Code Completion Hierarchy"
 *
 * <b>Accessing the CodeCompletionInterface</b>\n
 *
 * The CodeCompletionInterface is supposed to be an extension interface for a
 * View, i.e. the View inherits the interface @e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * @code
 *   // view is of type KTextEditor::View*
 *   KTextEditor::CodeCompletionInterface *iface =
 *       qobject_cast<KTextEditor::CodeCompletionInterface*>( view );
 *
 *   if( iface ) {
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * @endcode
 *
 * <b>Example Code</b>\n
 *
 * Throughout the example we assume that we work on the @e view and that
 * @e this is a class derived from CompletionProvider.
 *
 * Step 1: register the provider
 * @code
 *   KTextEditor::CodeCompletionInterface *iface =
 *       qobject_cast<KTextEditor::CodeCompletionInterface *>( view );
 *   if( iface ) iface->registerCompletionProvider( provider );
 * @endcode
 *
 * Step 2: reimplement Provider::completionData()
 * @code
 *   // get current cursor position, then current word
 *   const KTextEditor::Cursor& cursor = view.cursorPosition();
 *   // assumint you have something like a wordAt() function
 *   QString text = wordAt( view->document(), cursor );
 *
 *   // create list of matches, assuming allMatches() exist
 *   QList<KTextEditor::CompletionItem> matches = allMatches( text );
 *   return matches;
 *
 *   // or if you know there is no data just return the following
 *   return KTextEditor::CompletionData::Null();
 * @endcode
 *
 * Step 3: show the completion box
 * @code
 *   KTextEditor::CodeCompletionInterface *iface =
 *       qobject_cast<KTextEditor::CodeCompletionInterface *>( view );
 *   if( iface )
 *       iface->invokeCompletion( provider,
 *           KTextEditor::CompletionGenericSingleProvider );
 * @endcode
 *
 * @see KTextEditor::View, KTextEditor::CompletionProvider,
 *      KTextEditor::CompletionData, KTextEditor::CompletionItem,
 *      KTextEditor::ArgHintData
 * @author Joseph Wenninger \<jowenn@kde.org\>
 */
class KTEXTEDITOR_EXPORT CodeCompletionInterface
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~CodeCompletionInterface() {}

    /**
     * Register a new code completion @p provider.
     * @param provider new completion provider
     * @return @e true on success, otherwise @e false
     * @see unregisterCompletionProvider()
     */
    virtual bool registerCompletionProvider(CompletionProvider *provider)=0;

    /**
     * Unregister the code completion provider @p provider.
     * @param provider the provider that should be unregistered
     * @return @e true on success, otherwise @e false
     * @see registerCompletionProvider()
     */
    virtual bool unregisterCompletionProvider(CompletionProvider *provider)=0;

    /**
     * AsYouType and AsYouTypeBackspace have to be ignored
     * If this call is made from a providers Aborted/Done function, the
     * execution has to be delayed, till all providers have been finished and
     * the last type used in this call will be used. If called from within a
     * doComplete call it should be delayed till after all completionDone
     * calls
     * @param completionType the completion type
     * @see registerCompletionProvider()
     */
    virtual void invokeCompletion(enum CompletionType completionType)=0;

    /**
     * Invoke the code completion with the given @p provider and completion
     * type @p completionType.
     *
     * CompletionProvider::completionData() will only be called for the given
     * @p provider.
     *
     * @note An implementation of this interface should not accept providers
     *       which are not registered.
     * @param provider the provider object
     * @param completionType the completion type
     * @see registerCompletionProvider()
     */
    virtual void invokeCompletion(CompletionProvider* provider,
                                  enum CompletionType completionType)=0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionInterface, "org.kde.KTextEditor.CodeCompletionInterface")
Q_DECLARE_INTERFACE(KTextEditor::CompletionProvider, "org.kde.KTextEditor.CompletionProvider")
#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

