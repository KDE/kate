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

#include "factory.h"

#include "editor.h"

#include "document.h"

#include "view.h"

#include "plugin.h"

#include "recoveryinterface.h"
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

#include "modeinterface.h"

#include <kpluginfactory.h>
#include <kpluginloader.h>

using namespace KTextEditor;

Factory::Factory (const char *componentName, QObject *parent)
 : KPluginFactory ( componentName, parent )
 , d(0)
{
}

Factory::~Factory()
{
}

class KTextEditor::EditorPrivate {
  public:
    EditorPrivate()
    {
    }
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

bool View::insertText (const QString &text )
{
  KTextEditor::Document *doc=document();
  if (!doc) return false;
  return doc->insertText(cursorPosition(),text,blockSelection());
}

#ifndef KDE_NO_DEPRECATED
Plugin *KTextEditor::createPlugin ( KService::Ptr service, QObject *parent )
{
  QString error;
  Plugin* plugin = service->createInstance<KTextEditor::Plugin>(parent, QVariantList(), &error);
  if (!plugin)
    qWarning() << error; // TODO: would be nice to have a qCWarning here, but it seems like overkill
  return plugin;
}
#endif

struct KTextEditorFactorySet : public QSet<KPluginFactory*>
{
  KTextEditorFactorySet();
  ~KTextEditorFactorySet();
};

Q_GLOBAL_STATIC(KTextEditorFactorySet, s_factories)

static void cleanupFactories()
{
  qDeleteAll(*s_factories);
  s_factories->clear();
}

KTextEditorFactorySet::KTextEditorFactorySet() {
  // K_GLOBAL_STATIC is cleaned up *after* Q(Core)Application is gone
  // but we have to cleanup before -> use qAddPostRoutine
  qAddPostRoutine(cleanupFactories);
}
KTextEditorFactorySet::~KTextEditorFactorySet() {
  qRemovePostRoutine(cleanupFactories); // post routine is installed!
  qDeleteAll(*this);
}

Editor *KTextEditor::editor()
{
  KPluginFactory *fact=KPluginLoader("katepart").factory();

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



ModeInterface::ModeInterface() {
}

ModeInterface::~ModeInterface() {
}

RecoveryInterface::RecoveryInterface()
  : d(0) {
}

RecoveryInterface::~RecoveryInterface() {
}


// kate: space-indent on; indent-width 2; replace-tabs on;

