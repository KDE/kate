
#include "plugin_katehelloworld.h"
#include "plugin_katehelloworld.moc"

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kgenericfactory.h>

K_EXPORT_COMPONENT_FACTORY( katehelloworldplugin, KGenericFactory<KatePluginHelloWorld>("katehelloworld") )

KatePluginHelloWorld::KatePluginHelloWorld( QObject* parent, const QStringList& )
    : Kate::Plugin( (Kate::Application*)parent, "kate-hello-world-plugin" )
{
}

KatePluginHelloWorld::~KatePluginHelloWorld()
{
}

Kate::PluginView *KatePluginHelloWorld::createView( Kate::MainWindow *mainWindow )
{
  KatePluginHelloWorldView *view = new KatePluginHelloWorldView( mainWindow );

  KAction* a = view->actionCollection()->addAction( "edit_insert_helloworld" );
  a->setText( i18n("Insert Hello World") );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( slotInsertHello() ) );

  mainWindow->guiFactory()->addClient( view );

  return view;
}

void KatePluginHelloWorld::slotInsertHello()
{
  if (!application()->activeMainWindow()) {
    return;
  }

  KTextEditor::View *kv = application()->activeMainWindow()->activeView();

  if (kv) {
    kv->insertText( "Hello World" );
  }
}


KatePluginHelloWorldView::KatePluginHelloWorldView( Kate::MainWindow *mainWindow )
    : Kate::PluginView( mainWindow )
{
  setComponentData( KComponentData("kate") );
  setXMLFile( "plugins/katehelloworld/ui.rc" );
}

KatePluginHelloWorldView::~KatePluginHelloWorldView()
{
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

