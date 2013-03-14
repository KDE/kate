/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateglobal.h"
#include "kateglobal.moc"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katecmds.h"
#include "katemodemanager.h"
#include "kateschema.h"
#include "kateschemaconfig.h"
#include "kateconfig.h"
#include "katescriptmanager.h"
#include "katecmd.h"
#include "katebuffer.h"
#include "katepartpluginmanager.h"
#include "kateviglobal.h"
#include "katewordcompletion.h"
#include "spellcheck/spellcheck.h"
#include "snippet/katesnippetglobal.h"

#include <klocale.h>
#include <kservicetypetrader.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kpagedialog.h>
#include <kpagewidgetmodel.h>
#include <kiconloader.h>

#include <QtCore/QPointer>
#include <QtGui/QBoxLayout>
#include <QApplication>

KateGlobal *KateGlobal::s_self = 0;

int KateGlobal::s_ref = 0;

QString KateGlobal::katePartVersion()
{
  return QString("3.7");
}

KateGlobal::KateGlobal ()
 : KTextEditor::Editor (0)
 , m_aboutData ("katepart", 0, ki18n("Kate Part"), katePartVersion().toLatin1(),
             ki18n( "Embeddable editor component" ), KAboutData::License_LGPL_V2,
             ki18n( "(c) 2000-2009 The Kate Authors" ), KLocalizedString(), "http://www.kate-editor.org")
 , m_componentData (&m_aboutData)
 , m_snippetGlobal (0) // lazy constructed
 , m_sessionConfig (KGlobal::config())
{
  // set s_self
  s_self = this;

  // load the kate part translation catalog
  KGlobal::locale()->insertCatalog("katepart4");

  //
  // fill about data
  //
  m_aboutData.setProgramIconName("preferences-plugin");
  m_aboutData.addAuthor (ki18n("Christoph Cullmann"), ki18n("Maintainer"), "cullmann@kde.org", "http://www.cullmann.io");
  m_aboutData.addAuthor (ki18n("Dominik Haumann"), ki18n("Core Developer"), "dhaumann@kde.org");
  m_aboutData.addAuthor (ki18n("Milian Wolff"), ki18n("Core Developer"), "mail@milianw.de", "http://milianw.de");
  m_aboutData.addAuthor (ki18n("Joseph Wenninger"), ki18n("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  m_aboutData.addAuthor (ki18n("Erlend Hamberg"), ki18n("Vi Input Mode"), "ehamberg@gmail.com", "http://hamberg.no/erlend");
  m_aboutData.addAuthor (ki18n("Bernhard Beschow"), ki18n("Developer"), "bbeschow@cs.tu-berlin.de", "https://user.cs.tu-berlin.de/~bbeschow");
  m_aboutData.addAuthor (ki18n("Anders Lund"), ki18n("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  m_aboutData.addAuthor (ki18n("Michel Ludwig"), ki18n("On-the-fly spell checking"), "michel.ludwig@kdemail.net");
  m_aboutData.addAuthor (ki18n("Pascal Létourneau"), ki18n("Large scale bug fixing"), "pascal.letourneau@gmail.com");
  m_aboutData.addAuthor (ki18n("Hamish Rodda"), ki18n("Core Developer"), "rodda@kde.org");
  m_aboutData.addAuthor (ki18n("Waldo Bastian"), ki18n( "The cool buffersystem" ), "bastian@kde.org" );
  m_aboutData.addAuthor (ki18n("Charles Samuels"), ki18n("The Editing Commands"), "charles@kde.org");
  m_aboutData.addAuthor (ki18n("Matt Newell"), ki18n("Testing, ..."), "newellm@proaxis.com");
  m_aboutData.addAuthor (ki18n("Michael Bartl"), ki18n("Former Core Developer"), "michael.bartl1@chello.at");
  m_aboutData.addAuthor (ki18n("Michael McCallum"), ki18n("Core Developer"), "gholam@xtra.co.nz");
  m_aboutData.addAuthor (ki18n("Michael Koch"), ki18n("KWrite port to KParts"), "koch@kde.org");
  m_aboutData.addAuthor (ki18n("Christian Gebauer"), KLocalizedString(), "gebauer@kde.org" );
  m_aboutData.addAuthor (ki18n("Simon Hausmann"), KLocalizedString(), "hausmann@kde.org" );
  m_aboutData.addAuthor (ki18n("Glen Parker"), ki18n("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  m_aboutData.addAuthor (ki18n("Scott Manson"), ki18n("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  m_aboutData.addAuthor (ki18n("John Firebaugh"), ki18n("Patches and more"), "jfirebaugh@kde.org");
  m_aboutData.addAuthor (ki18n("Andreas Kling"), ki18n("Developer"), "kling@impul.se");
  m_aboutData.addAuthor (ki18n("Mirko Stocker"), ki18n("Various bugfixes"), "me@misto.ch", "http://misto.ch/");
  m_aboutData.addAuthor (ki18n("Matthew Woehlke"), ki18n("Selection, KColorScheme integration"), "mw_triad@users.sourceforge.net");
  m_aboutData.addAuthor (ki18n("Sebastian Pipping"), ki18n("Search bar back- and front-end"), "webmaster@hartwork.org", "http://www.hartwork.org/");
  m_aboutData.addAuthor (ki18n("Jochen Wilhelmy"), ki18n( "Original KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  m_aboutData.addAuthor (ki18n("Gerald Senarclens de Grancy"), ki18n("QA and Scripting"), "oss@senarclens.eu", "http://find-santa.eu/");

  m_aboutData.addCredit (ki18n("Matteo Merli"), ki18n("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  m_aboutData.addCredit (ki18n("Rocky Scaletta"), ki18n("Highlighting for VHDL"), "rocky@purdue.edu");
  m_aboutData.addCredit (ki18n("Yury Lebedev"), ki18n("Highlighting for SQL"),"");
  m_aboutData.addCredit (ki18n("Chris Ross"), ki18n("Highlighting for Ferite"),"");
  m_aboutData.addCredit (ki18n("Nick Roux"), ki18n("Highlighting for ILERPG"),"");
  m_aboutData.addCredit (ki18n("Carsten Niehaus"), ki18n("Highlighting for LaTeX"),"");
  m_aboutData.addCredit (ki18n("Per Wigren"), ki18n("Highlighting for Makefiles, Python"),"");
  m_aboutData.addCredit (ki18n("Jan Fritz"), ki18n("Highlighting for Python"),"");
  m_aboutData.addCredit (ki18n("Daniel Naber"));
  m_aboutData.addCredit (ki18n("Roland Pabel"), ki18n("Highlighting for Scheme"),"");
  m_aboutData.addCredit (ki18n("Cristi Dumitrescu"), ki18n("PHP Keyword/Datatype list"),"");
  m_aboutData.addCredit (ki18n("Carsten Pfeiffer"), ki18n("Very nice help"), "");
  m_aboutData.addCredit (ki18n("Bruno Massa"), ki18n("Highlighting for Lua"), "brmassa@gmail.com");

  m_aboutData.addCredit (ki18n("All people who have contributed and I have forgotten to mention"));

  m_aboutData.setTranslator(ki18nc("NAME OF TRANSLATORS","Your names"), ki18nc("EMAIL OF TRANSLATORS","Your emails"));

  //
  // dir watch
  //
  m_dirWatch = new KDirWatch ();

  //
  // command manager
  //
  m_cmdManager = new KateCmd ();

  //
  // hl manager
  //
  m_hlManager = new KateHlManager ();

  //
  // mode man
  //
  m_modeManager = new KateModeManager ();

  //
  // schema man
  //
  m_schemaManager = new KateSchemaManager ();

  //
  // vi input mode global
  //
  m_viInputModeGlobal = new KateViGlobal ();

  //
  // spell check manager
  //
  m_spellCheckManager = new KateSpellCheckManager ();

  // config objects
  m_globalConfig = new KateGlobalConfig ();
  m_documentConfig = new KateDocumentConfig ();
  m_viewConfig = new KateViewConfig ();
  m_rendererConfig = new KateRendererConfig ();

  // create script manager (search scripts)
  m_scriptManager = KateScriptManager::self();

  //
  // plugin manager
  //
  m_pluginManager = new KatePartPluginManager ();

  //
  // init the cmds
  //
  m_cmds.push_back( KateCommands::CoreCommands::self() );
  m_cmds.push_back( KateCommands::ViCommands::self() );
  m_cmds.push_back( KateCommands::AppCommands::self() );
  m_cmds.push_back( KateCommands::SedReplace::self() );
  m_cmds.push_back( KateCommands::Character::self() );
  m_cmds.push_back( KateCommands::Date::self() );

  for ( QList<KTextEditor::Command *>::iterator it = m_cmds.begin(); it != m_cmds.end(); ++it )
    m_cmdManager->registerCommand (*it);

  // global word completion model
  m_wordCompletionModel = new KateWordCompletionModel (this);

  //
  // finally setup connections
  //
  connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()), this, SLOT(updateColorPalette()));

  //required for setting sessionConfig property
  qRegisterMetaType<KSharedConfig::Ptr>("KSharedConfig::Ptr");
}

KateGlobal::~KateGlobal()
{
  delete m_snippetGlobal;
  delete m_pluginManager;

  delete m_globalConfig;
  delete m_documentConfig;
  delete m_viewConfig;
  delete m_rendererConfig;

  delete m_modeManager;
  delete m_schemaManager;

  delete m_viInputModeGlobal;

  delete m_dirWatch;

  // you too
  qDeleteAll (m_cmds);

  // cu managers
  delete m_scriptManager;
  delete m_hlManager;
  delete m_cmdManager;

  delete m_spellCheckManager;

  // cu model
  delete m_wordCompletionModel;

  s_self = 0;
}

KTextEditor::Document *KateGlobal::createDocument ( QObject *parent )
{
  KateDocument *doc = new KateDocument (false, false, false, 0, parent);

  emit documentCreated (this, doc);

  return doc;
}

const QList<KTextEditor::Document*> &KateGlobal::documents ()
{
  return m_docs;
}

KateSnippetGlobal *KateGlobal::snippetGlobal()
{
  if (!m_snippetGlobal)
    m_snippetGlobal = new KateSnippetGlobal (this);  
  return m_snippetGlobal;
}

//BEGIN KTextEditor::Editor config stuff
void KateGlobal::readConfig(KConfig *config)
{
  if( !config )
    config = KGlobal::config().data();

  KateGlobalConfig::global()->readConfig (KConfigGroup(config, "Kate Part Defaults"));

  KateDocumentConfig::global()->readConfig (KConfigGroup(config, "Kate Document Defaults"));

  KateViewConfig::global()->readConfig (KConfigGroup(config, "Kate View Defaults"));

  KateRendererConfig::global()->readConfig (KConfigGroup(config, "Kate Renderer Defaults"));

  m_viInputModeGlobal->readConfig( KConfigGroup( config, "Kate Vi Input Mode Settings" ) );
}

void KateGlobal::writeConfig(KConfig *config)
{
  if( !config )
    config = KGlobal::config().data();

  KConfigGroup cgGlobal(config, "Kate Part Defaults");
  KateGlobalConfig::global()->writeConfig (cgGlobal);

  KConfigGroup cg(config, "Kate Document Defaults");
  KateDocumentConfig::global()->writeConfig (cg);

  KConfigGroup cgDefault(config, "Kate View Defaults");
  KateViewConfig::global()->writeConfig (cgDefault);

  KConfigGroup cgRenderer(config, "Kate Renderer Defaults");
  KateRendererConfig::global()->writeConfig (cgRenderer);

  KConfigGroup cgViInputMode(config, "Kate Vi Input Mode Settings");
  m_viInputModeGlobal->writeConfig (cgViInputMode);

  config->sync();
}
//END KTextEditor::Editor config stuff

bool KateGlobal::configDialogSupported () const
{
  return true;
}

void KateGlobal::configDialog(QWidget *parent)
{
  QPointer<KPageDialog> kd = new KPageDialog(parent);
  kd->setCaption( i18n("Configure") );
  kd->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply | KDialog::Help );
  kd->setFaceType( KPageDialog::List );
  kd->setHelp( QString(), KGlobal::mainComponent().componentName() );

  QList<KTextEditor::ConfigPage*> editorPages;

  for (int i = 0; i < configPages (); ++i)
  {
    const QString name = configPageName (i);

    QFrame *page = new QFrame();

    KPageWidgetItem *item = kd->addPage( page, name );
    item->setHeader( configPageFullName (i) );
    item->setIcon( configPageIcon(i) );

    QVBoxLayout *topLayout = new QVBoxLayout( page );
    topLayout->setMargin( 0 );

    KTextEditor::ConfigPage *cp = configPage(i, page);
    connect(kd, SIGNAL(applyClicked  ( )), cp, SLOT(apply()));
    topLayout->addWidget( cp);
    editorPages.append (cp);
  }

  if (kd->exec() && kd)
  {
    KateGlobalConfig::global()->configStart ();
    KateDocumentConfig::global()->configStart ();
    KateViewConfig::global()->configStart ();
    KateRendererConfig::global()->configStart ();

    for (int i=0; i < editorPages.count(); ++i)
    {
      editorPages.at(i)->apply();
    }

    KateGlobalConfig::global()->configEnd ();
    KateDocumentConfig::global()->configEnd ();
    KateViewConfig::global()->configEnd ();
    KateRendererConfig::global()->configEnd ();
  }

  delete kd;
}

int KateGlobal::configPages () const
{
  return 5;
}

KTextEditor::ConfigPage *KateGlobal::configPage (int number, QWidget *parent)
{
  switch( number )
  {
    case 0:
      return new KateViewDefaultsConfig (parent);

    case 1:
      return new KateSchemaConfigPage (parent);

    case 2:
      return new KateEditConfigTab (parent);

    case 3:
      return new KateSaveConfigTab (parent);

    case 4:
      return new KatePartPluginConfigPage (parent);

    default:
      return 0;
  }

  return 0;
}

QString KateGlobal::configPageName (int number) const
{
  switch( number )
  {
    case 0:
      return i18n ("Appearance");

    case 1:
      return i18n ("Fonts & Colors");

    case 2:
      return i18n ("Editing");

    case 3:
      return i18n("Open/Save");

    case 4:
      return i18n ("Extensions");

    default:
      return QString ("");
  }

  return QString ("");
}

QString KateGlobal::configPageFullName (int number) const
{
  switch( number )
  {
    case 0:
      return i18n("Appearance");

    case 1:
      return i18n ("Font & Color Schemas");

    case 2:
      return i18n ("Editing Options");

    case 3:
      return i18n("File Opening & Saving");

    case 4:
      return i18n ("Extensions Manager");

    default:
      return QString ("");
  }

  return QString ("");
}

KIcon KateGlobal::configPageIcon (int number) const
{
  switch( number )
  {
    case 0:
      return KIcon("preferences-desktop-theme");

    case 1:
      return KIcon("preferences-desktop-color");

    case 2:
      return KIcon("accessories-text-editor");

    case 3:
      return KIcon("document-save");

    case 4:
      return KIcon("preferences-plugin");

    default:
      return KIcon("document-properties");
  }

  return KIcon("document-properties");
}

KateGlobal *KateGlobal::self ()
{
  if (!s_self) {
    new KateGlobal ();
  }

  return s_self;
}

void KateGlobal::registerDocument ( KateDocument *doc )
{
  KateGlobal::incRef ();
  m_documents.append( doc );
  m_docs.append (doc);
}

void KateGlobal::deregisterDocument ( KateDocument *doc )
{
  m_docs.removeAll (doc);
  m_documents.removeAll( doc );
  KateGlobal::decRef ();
}

void KateGlobal::registerView ( KateView *view )
{
  KateGlobal::incRef ();
  m_views.append( view );
}

void KateGlobal::deregisterView ( KateView *view )
{
  m_views.removeAll( view );
  KateGlobal::decRef ();
}

//BEGIN command interface
bool KateGlobal::registerCommand (KTextEditor::Command *cmd)
{return m_cmdManager->registerCommand(cmd);}

bool KateGlobal::unregisterCommand (KTextEditor::Command *cmd)
{return m_cmdManager->unregisterCommand(cmd);}

KTextEditor::Command *KateGlobal::queryCommand (const QString &cmd) const
{return m_cmdManager->queryCommand(cmd);}

QList<KTextEditor::Command*> KateGlobal::commands() const
{return m_cmdManager->commands();}

QStringList KateGlobal::commandList() const
{return m_cmdManager->commandList();}
//END command interface


//BEGIN container interface
QObject * KateGlobal::container()
{return m_container.data();}

void KateGlobal::setContainer( QObject * container )
{m_container=container;}
//END container interface

QWidget *KateGlobal::snippetWidget ()
{
  return snippetGlobal()->snippetWidget ();
}

KTextEditor::TemplateScript* KateGlobal::registerTemplateScript (QObject* owner, const QString& script)
{
  return scriptManager()->registerTemplateScript(owner, script);
}

void KateGlobal::unregisterTemplateScript(KTextEditor::TemplateScript* templateScript)
{
  scriptManager()->unregisterTemplateScript(templateScript);
}

void KateGlobal::updateColorPalette()
{
  // reload the global schema (triggers reload for every view as well)
  m_rendererConfig->reloadSchema();

  // force full update of all view caches and colors
  m_rendererConfig->updateConfig();
}

void KateGlobal::copyToClipboard (const QString &text)
{
  /**
   * empty => nop
   */
  if (text.isEmpty())
    return;
  
  /**
   * move to clipboard
   */
  QApplication::clipboard()->setText (text, QClipboard::Clipboard);
  
  /**
   * remember in history
   * cut after 10 entries
   */
  m_clipboardHistory.prepend (text);
  if (m_clipboardHistory.size () > 10)
    m_clipboardHistory.removeLast ();
  
  /**
   * notify about change
   */
  emit clipboardHistoryChanged ();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
