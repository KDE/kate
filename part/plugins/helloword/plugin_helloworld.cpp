
#include <qmultilineedit.h>
#include "plugin_helloworld.h"
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

HelloWorld::HelloWorld( QObject* parent, const char* name )
    : KTextEditor::ViewPlugin( parent, name )
{
    (void) new KAction( "&Select current line (plugin)", 0, this, SLOT(slotSpellCheck()),
                        actionCollection(), "spellcheck" );
			
  kdDebug()<<"TEST IT NOW #############################################################################################"<<endl;
   kdDebug()<<"TEST IT NOW #############################################################################################"<<endl;
    kdDebug()<<"TEST IT NOW #############################################################################################"<<endl;
     kdDebug()<<"TEST IT NOW #############################################################################################"<<endl;
      kdDebug()<<"TEST IT NOW #############################################################################################"<<endl;
}

HelloWorld::~HelloWorld()
{
}

void HelloWorld::slotSpellCheck()
{
}

KPluginFactory::KPluginFactory( QObject* parent, const char* name )
  : KLibFactory( parent, name )
{
  s_instance = new KInstance("KPluginFactory");
}

KPluginFactory::~KPluginFactory()
{
  delete s_instance;
}

QObject* KPluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
    return new HelloWorld( parent, name );
}

extern "C"
{
  void* init_ktexteditorviewpluginhelloworld()
  {
    return new KPluginFactory;
  }

}

KInstance* KPluginFactory::s_instance = 0L;

#include <plugin_helloworld.moc>
