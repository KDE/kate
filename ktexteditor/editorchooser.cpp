#include <editorchooser.h>
#include <editorchooser.moc>

#include <qcombobox.h>
#include <ktrader.h>
#include <kconfig.h>
#include <qstringlist.h>
#include <kservice.h>
#include <klocale.h>
#include <qlabel.h>
#include <kapplication.h>
#include <qlayout.h>

#include "editorchooser_ui.h"

using namespace KTextEditor;

namespace KTextEditor
{
  class PrivateEditorChooser
  {
  public:
    PrivateEditorChooser()
    {
    }
    ~PrivateEditorChooser(){}
  // Data Members
  EditorChooser_UI *chooser;
  QStringList ElementNames;
  QStringList elements;
  };

};

EditorChooser::EditorChooser(QWidget *parent,const char *name) :
	QWidget (parent,name)
  {
  d = new PrivateEditorChooser ();

  // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );


  d->chooser = new EditorChooser_UI (this, name);

  grid->addWidget( d->chooser, 0, 0);


	KTrader::OfferList offers = KTrader::self()->query("text/plain", "'KTextEditor/Document' in ServiceTypes");
	KConfig *config=new KConfig("default_components");
  	config->setGroup("KTextEditor");
  	QString editor = config->readEntry("embeddedEditor", "");

        if (editor.isEmpty()) editor="katepart";

	for (KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it)
	{
    		if ((*it)->desktopEntryName().contains(editor))
		{
			d->chooser->editorCombo->insertItem(i18n("System default (%1)").arg((*it)->name()));
			break;
		}
  	}

  	for (KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it)
  	{
    		d->chooser->editorCombo->insertItem((*it)->name());
		d->elements.append((*it)->desktopEntryName());
  	}
    	d->chooser->editorCombo->setCurrentItem(0);
}

EditorChooser:: ~EditorChooser(){
  delete d;
}

void EditorChooser::readAppSetting(const QString& postfix){
	KConfig *cfg=kapp->config();
	QString previousGroup=cfg->group();
	cfg->setGroup("KTEXTEDITOR:"+postfix);
	QString editor=cfg->readEntry("editor","");
	if (editor.isEmpty()) d->chooser->editorCombo->setCurrentItem(0);
	else
	{
		int idx=d->elements.findIndex(editor);
		idx=idx+1;
		d->chooser->editorCombo->setCurrentItem(idx);
	}
	cfg->setGroup(previousGroup);
}

void EditorChooser::writeAppSetting(const QString& postfix){
	KConfig *cfg=kapp->config();
	QString previousGroup=cfg->group();
	cfg->setGroup("KTEXTEDITOR:"+postfix);
	cfg->writeEntry("DEVELOPER_INFO","NEVER TRY TO USE VALUES FROM THAT GROUP, THEY ARE SUBJECT TO CHANGES");
	cfg->writeEntry("editor",d->chooser->editorCombo->currentItem()==0?"":(*d->elements.at(d->chooser->editorCombo->currentItem()-1)));
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
		return KTextEditor::createDocument("libkatepart",parent,name);

	return 0;
}

KTextEditor::Editor *EditorChooser::createEditor(QWidget *parentWidget,QObject *parent,const char* widgetName,
	const char* name,const QString& postfix,bool fallBackToKatePart){

        KTextEditor::Editor *tmpEd=0;

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
                tmpEd=KTextEditor::createEditor(serv->library().latin1(),parentWidget,widgetName,parent,name);
                if (tmpEd) return tmpEd;
        }
        if (fallBackToKatePart)
                return KTextEditor::createEditor("libkatepart",parentWidget,widgetName,parent,name);

        return 0;
}

