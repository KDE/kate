
#include "plugin_katehelloworld.h"
#include "plugin_katehelloworld.moc"

#include <kaction.h>
#include <klocale.h>
#include <kgenericfactory.h>

K_EXPORT_COMPONENT_FACTORY( katehelloworldplugin, KGenericFactory<KatePluginHelloWorld>( "katehelloworld" ) )
                       
class PluginView : public KXMLGUIClient
{             
  friend class KatePluginHelloWorld;

  public:
    Kate::MainWindow *win;
};

KatePluginHelloWorld::KatePluginHelloWorld( QObject* parent, const QStringList& )
    : Kate::Plugin ( (Kate::Application*)parent, "kate-hello-world-plugin" )
{
}

KatePluginHelloWorld::~KatePluginHelloWorld()
{
}

void KatePluginHelloWorld::addView(Kate::MainWindow *win)
{
    PluginView *view = new PluginView ();
             
    KAction* a = new KAction ( i18n("Insert Hello World"), view->actionCollection(), "edit_insert_helloworld" );
    connect( a, SIGNAL( triggered(bool) ), this, SLOT( slotInsertHello() ) );
    
    view->setComponentData (KComponentData("kate"));
    view->setXMLFile("plugins/katehelloworld/ui.rc");
    win->guiFactory()->addClient (view);
    view->win = win; 
    
   m_views.append (view);
}   

void KatePluginHelloWorld::removeView(Kate::MainWindow *win)
{
  for (int z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginView *view = m_views.at(z);
      m_views.removeAll (view);
      win->guiFactory()->removeClient (view);
      delete view;
    }  
}

void KatePluginHelloWorld::storeGeneralConfig(KConfig* config,const QString& groupPrefix)
{
  // If you have application wide settings, save them here
}

void KatePluginHelloWorld::loadGeneralConfig(KConfig* config,const QString& groupPrefix)
{
  // If you have application wide settings, load them here
}

void KatePluginHelloWorld::storeViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix)
{
  // If you have session-dependant settings, save them here
}

void KatePluginHelloWorld::loadViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix)
{
  // If you have session-dependant settings, lode them here
}

void KatePluginHelloWorld::slotInsertHello()
{
  if (!application()->activeMainWindow())
    return;

  KTextEditor::View *kv = application()->activeMainWindow()->activeView();

  if (kv)
    kv->insertText ("Hello World");
}

// kate: space-indent on; indent-width 2; replace-tabs on;

