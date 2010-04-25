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

#include <QtDBus/QtDBus>

#include "cursor.h"

#include "configpage.h"
#include "configpage.moc"

#include "factory.h"
#include "factory.moc"

#include "editor.h"
#include "editor.moc"

#include "document.h"

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

#include "annotationinterface.h"
#include "annotationinterface.moc"

#include "loadsavefiltercheckplugin.h"
#include "loadsavefiltercheckplugin.moc"

#include "modeinterface.h"

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
    QString defaultEncoding;
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

const QString &Editor::defaultEncoding () const
{
  return d->defaultEncoding;
}

void Editor::setDefaultEncoding (const QString &defaultEncoding)
{
  d->defaultEncoding = defaultEncoding;
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
  return doc->insertText(cursorPosition(),text,blockSelection());
}

Plugin *KTextEditor::createPlugin ( KService::Ptr service, QObject *parent )
{
  return service->createInstance<KTextEditor::Plugin>(parent);
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

ViewBarContainer::ViewBarContainer()
{}

ViewBarContainer::~ViewBarContainer()
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

ParameterizedSessionConfigInterface::ParameterizedSessionConfigInterface()
{}

ParameterizedSessionConfigInterface::~ParameterizedSessionConfigInterface()
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

class KTextEditor::LoadSaveFilterCheckPluginPrivate {
  public:
    LoadSaveFilterCheckPluginPrivate(){}
    ~LoadSaveFilterCheckPluginPrivate(){}
};

LoadSaveFilterCheckPlugin::LoadSaveFilterCheckPlugin(QObject *parent):
    QObject(parent),
    d(new LoadSaveFilterCheckPluginPrivate()) { }

LoadSaveFilterCheckPlugin::~LoadSaveFilterCheckPlugin() { delete d; }

CoordinatesToCursorInterface::~CoordinatesToCursorInterface() {
}



ModeInterface::ModeInterface() {
}

ModeInterface::~ModeInterface() {
}


// kate: space-indent on; indent-width 2; replace-tabs on;

