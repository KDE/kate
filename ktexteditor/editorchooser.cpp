#include <editorchooser.h>
#include <qcombobox.h>
#include <ktrader.h>
#include <kconfig.h>
#include <qstringlist.h>
#include <kservice.h>
#include <klocale.h>
#include <qlabel.h>
#include <kapplication.h>
using namespace KTextEditor;

namespace KTextEditor {
EditorChooser::EditorChooser(QWidget *parent,const char *name) :
	EditorChooser_UI(parent,name){

	KTrader::OfferList offers = KTrader::self()->query("text/plain", "'KTextEditor/Document' in ServiceTypes");
	KConfig *config=new KConfig("default_components");
  	config->setGroup("KTextEditor");
  	QString editor = config->readEntry("embeddedEditor", "");
	if (editor.isEmpty()) editor="katepart";

	for (KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it)
	{
    		if ((*it)->desktopEntryName().contains(editor))
		{
			editorCombo->insertItem(i18n("System default (%1)").arg((*it)->name()));
			break;
		}
  	}
	
  	for (KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it)
  	{
    		editorCombo->insertItem((*it)->name());
		elements.append((*it)->desktopEntryName());
  	}
    	editorCombo->setCurrentItem(0);
}

EditorChooser:: ~EditorChooser(){
;
}

void EditorChooser::readAppSetting(const QString& postfix){
	KConfig *cfg=kapp->config();
	QString previousGroup=cfg->group();
	cfg->setGroup("KTEXTEDITOR:"+postfix);
	QString editor=cfg->readEntry("editor","");
	if (editor.isEmpty()) editorCombo->setCurrentItem(0);
	else
	{
		int idx=elements.findIndex(editor);
		idx=idx+1;
		editorCombo->setCurrentItem(idx);
	}
	cfg->setGroup(previousGroup);
}

void EditorChooser::writeAppSetting(const QString& postfix){
	KConfig *cfg=kapp->config();
	QString previousGroup=cfg->group();
	cfg->setGroup("KTEXTEDITOR:"+postfix);
	cfg->writeEntry("DEVELOPER_INFO","NEVER TRY TO USE VALUES FROM THAT GROUP, THEY ARE SUBJECT TO CHANGES");
	cfg->writeEntry("editor",editorCombo->currentItem()==0?"":(*elements.at(editorCombo->currentItem()-1)));
	cfg->sync();
	cfg->setGroup(previousGroup);

}

KTextEditor::Document *EditorChooser::createDocument(QObject *parent,const char* name, const QString& postfix,bool fallBackToKatePart){

	KTextEditor::Document *tmpDoc=0;

	KConfig *cfg=kapp->config();
        QString previousGroup=cfg->group();
        cfg->setGroup("KTEXTEDITOR:"+postfix);
        QString editor=cfg->readEntry("editor","");
	cfg->setGroup(previousGroup);
	if (editor.isEmpty())
	{
		KConfig *config=new KConfig("default_components");
  		config->setGroup("KTextEditor");
	  	editor = config->readEntry("embeddedEditor", "katepart");
		delete config;
	}
	
	KService::Ptr serv=KService::serviceByDesktopName(editor);
	if (serv)
	{
		tmpDoc=KTextEditor::createDocument(serv->library().latin1(),parent,name);
		if (tmpDoc) return tmpDoc;
	}
	if (fallBackToKatePart)
		return KTextEditor::createDocument("katepart",parent,name);

	return 0;
}

KTextEditor::Editor *EditorChooser::createEditor(const QString& postfix,bool fallBackToKatePart){
;
}

}
