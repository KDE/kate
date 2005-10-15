/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <config.h>

#include "cursor.h"

#include "configpage.h"
#include "configpage.moc"

#include "factory.h"
#include "factory.moc"

#include "editor.h"
#include "editor.moc"

#include "document.h"
#include "document.moc"

#include "view.h"
#include "view.moc"

#include "plugin.h"
#include "plugin.moc"

#include "codecompletioninterface.h"
#include "commandinterface.h"
#include "highlightinginterface.h"
#include "markinterface.h"
#include "modificationinterface.h"
#include "searchinterface.h"
#include "sessionconfiginterface.h"
#include "templateinterface.h"
#include "texthintinterface.h"
#include "variableinterface.h"
#include "smartinterface.h"

#include <kaction.h>
#include <kparts/factory.h>
#include <klibloader.h>
#include <kdebug.h>

using namespace KTextEditor;

namespace KTextEditor
{
    class CompletionItem::Private: public QSharedData { //implicitly shared data, I can't move it into a private header file or the implementation, since the QSharedDataPointer causes compile problems, perhaps I should make it a QSharedDataPointer* ....
      public:
        Private():icon(QIcon()),type(QString()),text(QString()),prefix(QString()),postfix(QString()),comment(QString()),userdata(QVariant()),provider(0){}
        Private(const QString& _text, const QIcon &_icon=QIcon(), CompletionProvider* _provider=0, const QString &_prefix=QString(), const QString& _postfix=QString(), const QString& _comment=QString(), const QVariant &_userdata=QVariant(), const QString &_type=QString()):icon(_icon),type(_type),text(_text),prefix(_prefix),postfix(_postfix),comment(_comment),userdata(_userdata),provider(_provider) {}

      QIcon icon;
      QString type;
      QString text;
      QString prefix;
      QString postfix;
      QString comment;
      QVariant userdata;
      CompletionProvider *provider; 

      bool cmp(const Private* c) const {
        return ( c->type == type &&
                 c->text == text &&
                 c->postfix == postfix &&
                 c->prefix == prefix &&
                 c->comment == comment &&
                 c->userdata == userdata &&
                 c->provider==provider &&
                 c->icon.serialNumber()==icon.serialNumber());
      }
    };


}


CompletionItem::CompletionItem() {
  d=new CompletionItem::Private();
}

CompletionItem::CompletionItem(const QString& _text, const QIcon &_icon, CompletionProvider* _provider, const QString &_prefix, const QString& _postfix, const QString& _comment, const QVariant &_userdata, const QString &_type)
{
  d=new CompletionItem::Private(_text,_icon,_provider,_prefix,_postfix,_comment,_userdata,_type);
}

CompletionItem::~CompletionItem() {}

CompletionItem CompletionItem::operator=(const CompletionItem &c) {d=c.d; return *this; }

CompletionItem::CompletionItem (const CompletionItem &c) {d=c.d;}


bool CompletionItem::operator==( const CompletionItem &c ) const {
      return d->cmp(c.d);
}


const QIcon&    CompletionItem::icon() const {return d->icon;}
const QString&  CompletionItem::text() const {return d->text;}
const QString&  CompletionItem::markupText() const {return d->text;}
const QString&  CompletionItem::prefix() const {return d->prefix;}
const QString&  CompletionItem::postfix() const{return d->postfix;}
const QString&  CompletionItem::comment() const {return d->comment;}
const QVariant& CompletionItem::userdata() const {return d->userdata;}
CompletionProvider *CompletionItem::provider() const {return d->provider;}

Factory::Factory( QObject *parent )
 : KParts::Factory( parent )
{
}

Factory::~Factory()
{
}

Editor::Editor( QObject *parent )
 : QObject ( parent )
{
}

Editor::~Editor()
{
}

Document::Document( QObject *parent)
 : KDocument::Document( parent)
{
}

Document::~Document()
{
}

bool Document::cursorInText(const Cursor& cursor)
{
  if ( (cursor.line()<0) || (cursor.line()>=lines())) return false;
  return (cursor.column()>=0) && (cursor.column()<=lineLength(cursor.line())); // = because new line isn't usually contained in line length
}

bool Document::replaceText( const Range & range, const QStringList & text, bool block )
{
  bool success = true;
  startEditing();
  success &= removeText(range, block);
  success &= insertText(range.start(), text, block);
  endEditing();
  return success;
}

bool View::setSelection(const Cursor& position, int length,bool wrap)
{
  KTextEditor::Document *doc=document();
  if (!document()) return false;
  if (length==0) return false;
  if (!doc->cursorInText(position)) return false;
  Cursor end=Cursor(position.line(),position.column());
  if (!wrap) {
    int col=end.column()+length;
    if (col<0) col=0;
    if (col>doc->lineLength(end.line())) col=doc->lineLength(end.line());
    end.setColumn(col);
  } else {
    kdDebug()<<"KTextEditor::View::setSelection(pos,len,true) not implemented yet"<<endl;
  }
  return setSelection(Range(position,end));
}

bool View::insertText (const QString &text )
{
  KTextEditor::Document *doc=document();
  if (!doc) return false;
  return doc->insertText(cursorPosition(),text);
}

Plugin *KTextEditor::createPlugin ( const char *libname, QObject *parent )
{
  return KLibLoader::createInstance<Plugin>( libname, parent );
}

Editor *KTextEditor::editor(const char *libname)
{
  KLibFactory *fact=KLibLoader::self()->factory(libname);

  KTextEditor::Factory *ef=qobject_cast<KTextEditor::Factory*>(fact);

  if (!ef) return 0;

  return ef->editor();
}

KTextEditor::SmartInterface::~ SmartInterface( )
{
}

long ArgHintData::s_id=0;
long CompletionData::s_id=0;

// kate: space-indent on; indent-width 2; replace-tabs on;
