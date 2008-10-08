
#include "plugin_katehelloworld.h"
#include "plugin_katehelloworld.moc"

#include <kate/application.h>
#include <ktexteditor/view.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

K_PLUGIN_FACTORY(KatePluginHelloWorldFactory, registerPlugin<KatePluginHelloWorld>();)
K_EXPORT_PLUGIN(KatePluginHelloWorldFactory(KAboutData("katehelloworld","katehelloworld",ki18n("Hello World"), "0.1", ki18n("Example kate plugin"))) )

KatePluginHelloWorld::KatePluginHelloWorld( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin( (Kate::Application*)parent, "kate-hello-world-plugin" )
{
}

KatePluginHelloWorld::~KatePluginHelloWorld()
{
}

Kate::PluginView *KatePluginHelloWorld::createView( Kate::MainWindow *mainWindow )
{
  return new KatePluginHelloWorldView( mainWindow );
}


KatePluginHelloWorldView::KatePluginHelloWorldView( Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin )
{
  setComponentData( KatePluginHelloWorldFactory::componentData() );
  setXMLFile( "plugins/katehelloworld/ui.rc" );

  KAction *a = actionCollection()->addAction( "edit_insert_helloworld" );
  a->setText( i18n("Insert Hello World") );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( slotInsertHello() ) );

  mainWindow()->guiFactory()->addClient( this );
}

KatePluginHelloWorldView::~KatePluginHelloWorldView()
{
  mainWindow()->guiFactory()->removeClient( this );
}

void KatePluginHelloWorldView::slotInsertHello()
{
  if (!mainWindow()) {
    return;
  }

  KTextEditor::View *kv = mainWindow()->activeView();

  if (kv) {
    kv->insertText( "Hello World" );
  }
}

void KatePluginHelloWorldView::readSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, load them here.
  // If you have application wide settings, you have to read your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void KatePluginHelloWorldView::writeSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, save them here.
  // If you have application wide settings, you have to create your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

// kate: space-indent on; indent-width 2; replace-tabs on;

