#include <editorchooser.h>
#include <qcombobox.h>
#include <ktrader.h>
#include <kconfig.h>
#include <qstringlist.h>
#include <kservice.h>

using namespace KTextEditor;

namespace KTextEditor {
EditorChooser::EditorChooser(QWidget *parent,const char *name) :
	EditorChooser_UI(parent,name){
	KTrader::OfferList offers = KTrader::self()->query("text/plain", "'KTextEditor/Document' in ServiceTypes");
	KConfig *config=new KConfig("default_texteditor_component");
  	config->setGroup("Editor");
  	QString editor = config->readEntry("EmbeddedEditor", "");

	KTrader::OfferList::Iterator it;
	int index=-1, current=0;
  	for (it = offers.begin(); it != offers.end(); ++it)
  	{
    		editorCombo->insertItem((*it)->name());
    		if ( (*it)->name() == editor )
      			index = current;
    		++current;
  	}

	if (index >=0)
    	editorCombo->setCurrentItem(index);
	delete config;
}

EditorChooser:: ~EditorChooser(){
;
}

void EditorChooser::writeSysDefault()
{
}

//	readSetting(KConfig *cfg);
//	writeSetting(KConfig *cfg);

}
