#include <editorchooser.h>
#include <qcombobox.h>
#include <ktrader.h>
#include <kconfig.h>
#include <qstringlist.h>
#include <kservice.h>
#include <klocale.h>
#include <qlabel.h>
using namespace KTextEditor;

namespace KTextEditor {
EditorChooser::EditorChooser(QWidget *parent,const char *name,const ChooserType type) :
	EditorChooser_UI(parent,name){

	if (type==AppSetting)
	{
		TextLabel1->setText(i18n("Choose the default editing widget this application should use"));
		TextLabel2->setText(i18n(""));
	}
	KTrader::OfferList offers = KTrader::self()->query("text/plain", "'KTextEditor/Document' in ServiceTypes");
	KConfig *config=new KConfig("default_texteditor_component");
  	config->setGroup("Editor");
  	QString editor = config->readEntry("EmbeddedEditor", "");

	KTrader::OfferList::Iterator it;
	int index=-1, current=0;
	int katepartIndex=-1;
  	for (it = offers.begin(); it != offers.end(); ++it)
  	{
    		editorCombo->insertItem((*it)->name());
    		if ( (*it)->name() == editor )
      			index = current;
		if ( (*it)->library().contains("katepart")) katepartIndex=current;
    		++current;
  	}

	if (index >=0)
    	editorCombo->setCurrentItem(index);
	else editorCombo->setCurrentItem(katepartIndex);
	delete config;
}

EditorChooser:: ~EditorChooser(){
;
}

void EditorChooser::writeSysDefault()
{
	KConfig *config=new KConfig("default_texteditor_component");
	config->setGroup("Editor");
	config->writeEntry("EmbeddedEditor",editorCombo->currentText());
	config->sync();
	delete config;
}

void EditorChooser::readAppSetting(KConfig *cfg, const QString& postfix){
	QString previousGroup=cfg->group();
	cfg->setGroup("KTEXTEDITOR:"+postfix);
	QString editor=cfg->readEntry("editor","");
	for (int i=0;i<editorCombo->count();i++)
		if (editorCombo->text(i)==editor)
		{
			editorCombo->setCurrentItem(i);
			break;
		}

	cfg->setGroup(previousGroup);
}

void EditorChooser::writeAppSetting(KConfig *cfg, const QString& postfix){
	QString previousGroup=cfg->group();
	cfg->setGroup("KTEXTEDITOR:"+postfix);
	cfg->writeEntry("DEVELOPER_INFO","NEVER TRY TO USE VALUES FROM THAT GROUP, THEY ARE SUBJECT TO CHANGES");
	cfg->writeEntry("editor",editorCombo->currentText());
	cfg->sync();
	cfg->setGroup(previousGroup);

}

}
