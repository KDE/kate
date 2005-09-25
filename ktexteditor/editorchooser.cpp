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
#include <q3valuelist.h>

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
  Ui::EditorChooser *chooser;
  QStringList ElementNames;
  QStringList elements;
  };

}

EditorChooser::EditorChooser(QWidget *parent)
    : QWidget(parent)
  {
  d = new PrivateEditorChooser ();

  d->chooser = new Ui::EditorChooser();
  d->chooser->setupUi(this);

	KTrader::OfferList offers = KTrader::self()->query("text/plain", "'KTextEditor/Document' in ServiceTypes");
	KConfig *config=new KConfig("default_components");
  	config->setGroup("KTextEditor");
  	QString editor = config->readPathEntry("embeddedEditor");

        if (editor.isEmpty()) editor="katepart";

	for (KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it)
	{
    		if ((*it)->desktopEntryName().contains(editor))
		{
			d->chooser->editorCombo->insertItem(i18n("System Default (currently: %1)").arg((*it)->name()));
			break;
		}
  	}

  	for (KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it)
  	{
    		d->chooser->editorCombo->insertItem((*it)->name());
		d->elements.append((*it)->desktopEntryName());
  	}
    	d->chooser->editorCombo->setCurrentItem(0);

	connect(d->chooser->editorCombo,SIGNAL(activated(int)),this,SIGNAL(changed()));
}

EditorChooser:: ~EditorChooser(){
  delete d;
}

void EditorChooser::readAppSetting(const QString& postfix){
	KConfig *cfg=kapp->config();
	QString previousGroup=cfg->group();
        if (postfix.isEmpty())
		cfg->setGroup("KTEXTEDITOR:");
	else
		cfg->setGroup("KTEXTEDITOR:"+postfix);
	QString editor=cfg->readPathEntry("editor");
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
	if (postfix.isEmpty())
		cfg->setGroup("KTEXTEDITOR:");
	else
		cfg->setGroup("KTEXTEDITOR:"+postfix);
	cfg->writeEntry("DEVELOPER_INFO","NEVER TRY TO USE VALUES FROM THAT GROUP, THEY ARE SUBJECT TO CHANGES");
	cfg->writePathEntry("editor", (d->chooser->editorCombo->currentItem()==0) ?
		QString() : QString(d->elements.at(d->chooser->editorCombo->currentItem()-1)));
	cfg->sync();
	cfg->setGroup(previousGroup);

}

KTextEditor::Editor *EditorChooser::editor(const QString& postfix,bool fallBackToKatePart){

	KTextEditor::Editor *tmpEd=0;

	KConfig *cfg=kapp->config();
        QString previousGroup=cfg->group();
	if (postfix.isEmpty())
	        cfg->setGroup("KTEXTEDITOR:");
	else
        	cfg->setGroup("KTEXTEDITOR:"+postfix);
        QString editor=cfg->readPathEntry("editor");
	cfg->setGroup(previousGroup);
	if (editor.isEmpty())
	{
		KConfig *config=new KConfig("default_components");
  		config->setGroup("KTextEditor");
	  	editor = config->readPathEntry("embeddedEditor", "katepart");
		delete config;
	}

	KService::Ptr serv=KService::serviceByDesktopName(editor);
	if (serv)
	{
		tmpEd=KTextEditor::editor(serv->library().latin1());
		if (tmpEd) return tmpEd;
	}
	if (fallBackToKatePart)
		return KTextEditor::editor("katepart");

	return 0;
}

