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

#include "commandinterface.h"
#include "markinterface.h"
#include "modificationinterface.h"
#include "searchinterface.h"
#include "sessionconfiginterface.h"
#include "templateinterface.h"
#include "texthintinterface.h"
#include "variableinterface.h"
#include "containerinterface.h"

#include "documentadaptor_p.h"
#include "documentadaptor_p.moc"


//#include <kaction.h>
#include <kparts/factory.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kdebug.h>

using namespace KTextEditor;

Factory::Factory( QObject *parent )
 : KParts::Factory( parent )
 , d(0)
{
}

Factory::~Factory()
{
}

class KTextEditor::EditorPrivate {
  public:
    EditorPrivate()
      : simpleMode (false) { }
    bool simpleMode;
};

Editor::Editor( QObject *parent )
 : QObject ( parent )
 , d(new KTextEditor::EditorPrivate())
{
}

Editor::~Editor()
{
  delete d;
}

void Editor::setSimpleMode (bool on)
{
  d->simpleMode = on;
}

bool Editor::simpleMode () const
{
  return d->simpleMode;
}


class KTextEditor::DocumentPrivate {
  public:
    DocumentPrivate()
      : openingError(false), suppressOpeningErrorDialogs(false) { }
    bool openingError;
    bool suppressOpeningErrorDialogs;
    QString openingErrorMessage;
};

Document::Document( QObject *parent)
 : KParts::ReadWritePart(parent)
 , d(new DocumentPrivate())
{
        qRegisterMetaType<KTextEditor::Document*>("KTextEditor::Document*");
	new DocumentAdaptor(this);
}

Document::~Document()
{
  delete d;
}

void Document::setSuppressOpeningErrorDialogs(bool suppress) {
  d->suppressOpeningErrorDialogs=suppress;
}

bool Document::suppressOpeningErrorDialogs() const {
  return d->suppressOpeningErrorDialogs;
}

bool Document::openingError() const {
  return d->openingError;
}

QString Document::openingErrorMessage() const {
  return d->openingErrorMessage;
}

void Document::setOpeningError(bool errors) {
  d->openingError=errors;
}

void Document::setOpeningErrorMessage(const QString& message) {
  d->openingErrorMessage=message;
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

bool View::isActiveView() const
{
  return this == document()->activeView();
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
    kDebug()<<"KTextEditor::View::setSelection(pos,len,true) not implemented yet";
  }
  return setSelection(Range(position,end));
}

bool View::insertText (const QString &text )
{
  KTextEditor::Document *doc=document();
  if (!doc) return false;
  return doc->insertText(cursorPosition(),text);
}

Plugin *KTextEditor::createPlugin ( KService::Ptr service, QObject *parent )
{
  return KService::createInstance<KTextEditor::Plugin>(service, parent);
}

struct KTextEditorFactorySet : public QSet<KPluginFactory*>
{
  KTextEditorFactorySet();
  ~KTextEditorFactorySet();
};
K_GLOBAL_STATIC(KTextEditorFactorySet, s_factories)
KTextEditorFactorySet::KTextEditorFactorySet() {
  // K_GLOBAL_STATIC is cleaned up *after* Q(Core)Application is gone
  // but we have to cleanup before -> use qAddPostRoutine
  qAddPostRoutine(s_factories.destroy);
}
KTextEditorFactorySet::~KTextEditorFactorySet() {
  qRemovePostRoutine(s_factories.destroy); // post routine is installed!
  qDeleteAll(*this);
}

Editor *KTextEditor::editor(const char *libname)
{
  KPluginFactory *fact=KPluginLoader(libname).factory();

  KTextEditor::Factory *ef=qobject_cast<KTextEditor::Factory*>(fact);

  if (!ef) {
    delete fact;
    return 0;
  }

  s_factories->insert(fact);

  return ef->editor();
}

bool Document::isEmpty( ) const
{
  return documentEnd() == Cursor::start();
}

ConfigPage::ConfigPage ( QWidget *parent )
  : QWidget (parent)
  , d(0)
{}

ConfigPage::~ConfigPage ()
{}

View::View ( QWidget *parent )
  : QWidget(parent), KXMLGUIClient()
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

ContainerInterface::ContainerInterface ()
{}

ContainerInterface::~ContainerInterface ()
{}

MdiContainer::MdiContainer ()
{}

MdiContainer::~MdiContainer ()
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

