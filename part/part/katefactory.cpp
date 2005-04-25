/* This file is part of the KDE libraries
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>

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

#include "katefactory.h"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katecmds.h"
#include "katefiletype.h"
#include "kateschema.h"
#include "katesearch.h"
#include "kateconfig.h"
#ifndef Q_WS_WIN //todo
#include "katejscript.h"
#endif

#include "../interfaces/katecmd.h"

#include <kvmallocator.h>
#include <klocale.h>
#include <kdirwatch.h>
#include <kstaticdeleter.h>

#include <qapplication.h>

/**
 * dummy wrapper factory to be sure nobody external deletes our katefactory
 */
class KateFactoryPublic : public KParts::Factory
{
  public:
    /**
     * reimplemented create object method
     * @param parentWidget parent widget
     * @param widgetName widget name
     * @param parent QObject parent
     * @param name object name
     * @param args additional arguments
     * @return constructed part object
     */
    KParts::Part *createPartObject ( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *classname, const QStringList &args )
    {
      return KateFactory::self()->createPartObject (parentWidget, widgetName, parent, name, classname, args);
    }
};

K_EXPORT_COMPONENT_FACTORY( libkatepart, KateFactoryPublic )

KateFactory *KateFactory::s_self = 0;

KateFactory::KateFactory ()
 : m_aboutData ("katepart", I18N_NOOP("Kate Part"), KATEPART_VERSION,
             I18N_NOOP( "Embeddable editor component" ), KAboutData::License_LGPL_V2,
             I18N_NOOP( "(c) 2000-2004 The Kate Authors" ), 0, "http://kate.kde.org")
 , m_instance (&m_aboutData)
 , m_plugins (KTrader::self()->query("KTextEditor/Plugin"))
 , m_jscript (0)
{
  // set s_self
  s_self = this;

  //
  // fill about data
  //
  m_aboutData.addAuthor ("Christoph Cullmann", I18N_NOOP("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
  m_aboutData.addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  m_aboutData.addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  m_aboutData.addAuthor ("Hamish Rodda",I18N_NOOP("Core Developer"), "rodda@kde.org");
  m_aboutData.addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
  m_aboutData.addAuthor ("Charles Samuels", I18N_NOOP("The Editing Commands"), "charles@kde.org");
  m_aboutData.addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  m_aboutData.addAuthor ("Michael Bartl", I18N_NOOP("Former Core Developer"), "michael.bartl1@chello.at");
  m_aboutData.addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  m_aboutData.addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  m_aboutData.addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  m_aboutData.addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
  m_aboutData.addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
  m_aboutData.addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  m_aboutData.addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  m_aboutData.addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");
  m_aboutData.addAuthor ("Dominik Haumann", I18N_NOOP("Developer & Highlight wizard"), "dhdev@gmx.de");

  m_aboutData.addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  m_aboutData.addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  m_aboutData.addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  m_aboutData.addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  m_aboutData.addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  m_aboutData.addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  m_aboutData.addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  m_aboutData.addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  m_aboutData.addCredit ("Daniel Naber","","");
  m_aboutData.addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  m_aboutData.addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  m_aboutData.addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  m_aboutData.addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

  m_aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  //
  // dir watch
  //
  m_dirWatch = new KDirWatch ();

  //
  // filetype man
  //
  m_fileTypeManager = new KateFileTypeManager ();

  //
  // schema man
  //
  m_schemaManager = new KateSchemaManager ();

  // config objects
  m_documentConfig = new KateDocumentConfig ();
  m_viewConfig = new KateViewConfig ();
  m_rendererConfig = new KateRendererConfig ();

  // vm allocator
  m_vm = new KVMAllocator ();

#ifndef Q_WS_WIN //todo
  // create script man (search scripts) + register commands
  m_jscriptManager = new KateJScriptManager ();
  KateCmd::self()->registerCommand (m_jscriptManager);
#else
  m_jscriptManager = 0;
#endif

  //
  // init the cmds
  //
  m_cmds.push_back (new KateCommands::CoreCommands());
  m_cmds.push_back (new KateCommands::SedReplace ());
  m_cmds.push_back (new KateCommands::Character ());
  m_cmds.push_back (new KateCommands::Date ());
  m_cmds.push_back (new SearchCommand());

  for ( QValueList<Kate::Command *>::iterator it = m_cmds.begin(); it != m_cmds.end(); ++it )
    KateCmd::self()->registerCommand (*it);
}

KateFactory::~KateFactory()
{
  /* ?hack? If  MainApplication-Interface::quit is called by dcop the factory gets destroyed before all documents are destroyed eg in kwrite.
  This could happen in other apps too. Since the documents try to unregister a new factory is created (in the ::self call) and registered with a
  KStaticDeleter which causes a crash. That's why I ensure here that all documents are destroyed before the factory goes down (JOWENN)*/
  while (KateDocument *doc=m_documents.first()) {
    s_self=this; /* this is needed because the KStaticDeleter sets the global reference to 0, before it deletes the object it handles.
    To prevent a crash again restore the factory pointer temporarily. (jowenn)*/
    delete doc;
    s_self=0;
  }
  /*another solution would be to set a flag in the documents, and inhibit calling of the deregistering methods, but I don't see a problem
  if all created objects are deleted before their factory. If somebody sees a problem, let me know*/

  delete m_documentConfig;
  delete m_viewConfig;
  delete m_rendererConfig;

  delete m_fileTypeManager;
  delete m_schemaManager;

  delete m_dirWatch;

  delete m_vm;

  for ( QValueList<Kate::Command *>::iterator it = m_cmds.begin(); it != m_cmds.end(); ++it )
    delete *it;

  // cu manager
  delete m_jscriptManager;

  // cu jscript
  delete m_jscript;
}

static KStaticDeleter<KateFactory> sdFactory;

KateFactory *KateFactory::self ()
{
  if (!s_self) {
    sdFactory.setObject(s_self, new KateFactory ());
  }
  return s_self;
}

KParts::Part *KateFactory::createPartObject ( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *_classname, const QStringList & )
{
  QCString classname( _classname );
  bool bWantSingleView = ( classname != "KTextEditor::Document" && classname != "Kate::Document" );
  bool bWantBrowserView = ( classname == "Browser/View" );
  bool bWantReadOnly = (bWantBrowserView || ( classname == "KParts::ReadOnlyPart" ));

  KParts::ReadWritePart *part = new KateDocument (bWantSingleView, bWantBrowserView, bWantReadOnly, parentWidget, widgetName, parent, name);
  part->setReadWrite( !bWantReadOnly );

  return part;
}

void KateFactory::registerDocument ( KateDocument *doc )
{
  m_documents.append( doc );
}

void KateFactory::deregisterDocument ( KateDocument *doc )
{
  m_documents.removeRef( doc );
}

void KateFactory::registerView ( KateView *view )
{
  m_views.append( view );
}

void KateFactory::deregisterView ( KateView *view )
{
  m_views.removeRef( view );
}

void KateFactory::registerRenderer ( KateRenderer  *renderer )
{
  m_renderers.append( renderer );
}

void KateFactory::deregisterRenderer ( KateRenderer  *renderer )
{
  m_renderers.removeRef( renderer );
}

KateJScript *KateFactory::jscript ()
{
#ifndef Q_WS_WIN //todo
  if (m_jscript)
    return m_jscript;

  return m_jscript = new KateJScript ();
#else
  return 0;
#endif
}

// kate: space-indent on; indent-width 2; replace-tabs on;
