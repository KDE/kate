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

class CompletionProvider;

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




class KTEXTEDITOR_EXPORT CompletionData {
  public:
     CompletionData():m_id(0) {} //You should never use that yourself
     CompletionData(QList<CompletionItem> items,const KTextEditor::Cursor& matchStart,bool casesensitive):
       m_items(items),m_matchStart(matchStart),m_casesensitive(casesensitive),m_id(((++s_id)==0)?(++s_id):s_id){ }
     inline const QList<CompletionItem>& items()const {return m_items;}
     inline const KTextEditor::Cursor& matchStart() const {return m_matchStart;}
     inline bool casesensitive() const {return m_casesensitive;}
     inline bool operator==( const CompletionData &d ) const { kdDebug()<<"Checking equality"<<endl;return m_id==d.m_id;}
     inline static const CompletionData Null() {return CompletionData();}
     inline bool isValid()const {return m_id!=0;}
     inline int id() const {return m_id;};
  private:
     QList<CompletionItem> m_items;
     Cursor m_matchStart;
     bool m_casesensitive;
     long m_id;
     static long s_id;
  };


class KTEXTEDITOR_EXPORT ArgHintData {
  public:
    ArgHintData():m_id(0) {} //You should never use that yourself
    ArgHintData(const QString& wrapping, const QString& delimiter, const QStringList& items) :
        m_wrapping(wrapping),m_delimiter(delimiter),m_items(items),m_id(((++s_id)==0)?(++s_id):s_id) {}
    inline const QString& wrapping() const {return m_wrapping;}
    inline const QString& delimiter() const {return m_delimiter;}
    inline const QStringList& items() const {return m_items;}
    inline bool operator==( const ArgHintData &d ) const { return m_id==d.m_id;}
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



/**The provider should be asked by the editor after each typed character(block) if it wants
to show a completion of type CompletionAsYouType.
The provider should cache the completiondata as long as the begin of the word to be completed doesn't change and the previous word part which has been used to determine the completion list is a substring of the current word part. Similiar behaviour if a word part is removed would be desirable, but is at least for kate not needed, since it doesn't support the CompleteAsYouTypeBackspace. If a completion type >CompleteReinvokeAsYouType is active, no further requests are sent out from the editor, till the completion has been aborted or executed
**/
class KTEXTEDITOR_EXPORT CompletionProvider
{
  public:
    virtual ~CompletionProvider(){;}
  public:


    /* insertion position can only be assumed valid, if the completion type is CompleteAsYouType and the inserted text is not empty*/
    virtual const CompletionData completionData(View*,enum CompletionType, const Cursor& /*insertion pos*/, const QString& /*insertedText*/,const Cursor& /*current pos*/, const QString& /*current line*/)=0;
    virtual const ArgHintData argHintData(View *,const Cursor&, const QString&)=0;
    /* this function is called if a completion process has been aborted, the providers should not assume, that they only get this signal, if they provided data for the completion popup*/
    virtual void completionAborted(View*)=0;
    /* this function is called if a completion process has been aborted, the providers should not assume, that they only get this signal, if they provided data for the completion popup*/
    virtual void completionDone(View*)=0;
    /* this method is called if for a specific item the provider has been set, no default handling will be done by the editor component */
    virtual void doComplete(View*,const CompletionData&,const CompletionItem&)=0;

};

class KTEXTEDITOR_EXPORT CodeCompletionInterface
{
  public:
  	virtual ~CodeCompletionInterface() {}

        virtual bool registerCompletionProvider(CompletionProvider*)=0;
        virtual bool unregisterCompletionProvider(CompletionProvider*)=0;
	/*AsYouType and AsYouTypeBackspace have to be ignored
	If this call is made from a providers Aborted/Done function, the execution has to be
	delayed, till all providers have been finished and the last type used in this call will be used. If called from within a doComplete call it should be delayed till after all completionDone calls*/
	virtual void invokeCompletion(enum CompletionType)=0;
        /* just like the above, but only generates generates one  call to completionData for the specified provider. The implementor of this interface should not accept providers in this function, which are not registered*/
        virtual void invokeCompletion(CompletionProvider*,enum CompletionType)=0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionInterface, "org.kde.KTextEditor.CodeCompletionInterface")
Q_DECLARE_INTERFACE(KTextEditor::CompletionProvider, "org.kde.KTextEditor.CompletionProvider")
#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

