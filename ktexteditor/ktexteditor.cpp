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

#include "documentadaptor_p.h"
#include "documentadaptor_p.moc"
#include "highlightinginterfaceadaptor_p.h"
#include "highlightinginterfaceadaptor_p.moc"


//#include <kaction.h>
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
 , d(0)
{
}

Factory::~Factory()
{
}

Editor::Editor( QObject *parent )
 : QObject ( parent )
 , d(0)
{
}

Editor::~Editor()
{
}

Document::Document( QObject *parent)
 : KDocument::Document( parent)
 , d(0)
{
        qRegisterMetaType<KTextEditor::Document*>("KTextEditor::Document*");
	new DocumentAdaptor(this);
}

Document::~Document()
{
}

bool Document::cursorInText(const Cursor& cursor)
{
  if ( (cursor.line()<0) || (cursor.line()>=lines())) return false;
  return (cursor.column()>=0) && (cursor.column()<=lineLength(cursor.line())); // = because new line isn't usually contained in line length
}

bool KTextEditor::Document::replaceText( const Range & range, const QString & text, bool block )
{
  bool success = true;
  startEditing();
  success &= removeText(range, block);
  success &= insertText(range.start(), text, block);
  endEditing();
  return success;
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

QList< View * > Document::textViews( ) const
{
  QList< View * > v;
  foreach (KDocument::View* view, views())
    v << static_cast<View*>(view);
  return v;
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
    kDebug()<<"KTextEditor::View::setSelection(pos,len,true) not implemented yet"<<endl;
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

bool Document::isEmpty( ) const
{
  return documentEnd() == Cursor::start();
}

long ArgHintData::s_id=0;
long CompletionData::s_id=0;

ConfigPage::ConfigPage ( QWidget *parent )
  : QWidget (parent)
  , d(0)
{}

ConfigPage::~ConfigPage ()
{}

View::View ( QWidget *parent )
  : KDocument::View( parent )
  , d(0)
{}

View::~View ()
{}

Plugin::Plugin ( QObject *parent )
  : QObject (parent)
  , d(0)
{}

Plugin::~Plugin ()
{}

HighlightingInterface::HighlightingInterface (QObject *myself)
  : d(0)
{ new HighlightingInterfaceAdaptor(myself,this);}

HighlightingInterface::~HighlightingInterface ()
{}

MarkInterface::MarkInterface ()
  : d(0)
{}

MarkInterface::~MarkInterface ()
{}

ModificationInterface::ModificationInterface ()
  : d(0)
{}

ModificationInterface::~ModificationInterface ()
{}

SearchInterface::SearchInterface()
  : d(0)
{}

SearchInterface::~SearchInterface()
{}

SessionConfigInterface::SessionConfigInterface()
  : d(0)
{}

SessionConfigInterface::~SessionConfigInterface()
{}

TemplateInterface::TemplateInterface()
  : d(0)
{}

TemplateInterface::~TemplateInterface()
{}

TextHintInterface::TextHintInterface()
  : d(0)
{}

TextHintInterface::~TextHintInterface()
{}

VariableInterface::VariableInterface()
  : d(0)
{}

VariableInterface::~VariableInterface()
{}

DocumentAdaptor::DocumentAdaptor(Document *document):
  QDBusAbstractAdaptor(document),m_document(document) {
}

DocumentAdaptor::~DocumentAdaptor() {}

bool DocumentAdaptor::clear() {
  return m_document->clear();
}

bool DocumentAdaptor::reload() {
  return m_document->documentReload();
}

bool DocumentAdaptor::save() {
  return m_document->documentSave();
}

bool DocumentAdaptor::saveAs() {
  return m_document->documentSaveAs();
}

bool DocumentAdaptor::setTextLines(const QStringList &text) {
  return m_document->setText(text);
}

bool DocumentAdaptor::isEmpty() const {
  return m_document->isEmpty();
}

bool DocumentAdaptor::setEncoding(const QString &encoding) {
  return m_document->setEncoding(encoding);
}

const QString &DocumentAdaptor::encoding() const {
  return m_document->encoding();
}

bool DocumentAdaptor::setText(const QString &text) {
  return m_document->setText(text);
}

QString DocumentAdaptor::text() const {
  return m_document->text();
}

int DocumentAdaptor::lines() const {
  return m_document->lines();
}

int DocumentAdaptor::totalCharacters() const {
  return m_document->totalCharacters();
}

int DocumentAdaptor::lineLength(int line) const {
  return m_document->lineLength(line);
}
                        
QPoint DocumentAdaptor::endOfLine(int line) const {
  Cursor c=m_document->endOfLine(line);
  return QPoint(c.column(),c.line());
}

bool DocumentAdaptor::insertText(const QPoint& cursor,const QString& text, bool block) {
  return m_document->insertText(Cursor(cursor.y(),cursor.x()),text,block);
}

bool DocumentAdaptor::insertTextLines(const QPoint& cursor,const QStringList& text, bool block) {
  return m_document->insertText(Cursor(cursor.y(),cursor.x()),text,block);
}

bool DocumentAdaptor::cursorInText(const QPoint& cursor) {
  return m_document->cursorInText(Cursor(cursor.y(),cursor.x()));
}

bool DocumentAdaptor::insertLine(int line, const QString& text) {
  return m_document->insertLine(line,text);
}

bool DocumentAdaptor::insertLines(int line, const QStringList& text) {
  return m_document->insertLines(line,text);
}

bool DocumentAdaptor::removeLine(int line) {
  return m_document->removeLine(line);
}

// kate: space-indent on; indent-width 2; replace-tabs on;

