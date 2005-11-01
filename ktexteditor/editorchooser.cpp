/* This file is part of the KDE libraries
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include <editorchooser.h>
#include <editorchooser.moc>

#include <qcombobox.h>
#include <qstringlist.h>
#include <qlabel.h>
#include <qlayout.h>

#include <ktrader.h>
#include <kconfig.h>
#include <kservice.h>
#include <klocale.h>
#include <kglobal.h>

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
			d->chooser->editorCombo->addItem(i18n("System Default (currently: %1)").arg((*it)->name()));
			break;
		}
  	}

  	for (KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it)
  	{
    		d->chooser->editorCombo->addItem((*it)->name());
		d->elements.append((*it)->desktopEntryName());
  	}
    	d->chooser->editorCombo->setCurrentIndex(0);

	connect(d->chooser->editorCombo,SIGNAL(activated(int)),this,SIGNAL(changed()));
}

EditorChooser:: ~EditorChooser(){
  delete d;
}

void EditorChooser::readAppSetting(const QString& postfix){
        KConfigGroup cg(KGlobal::config(), postfix.isEmpty() ? "KTEXTEDITOR:" : "KTEXTEDITOR:"+postfix);
	QString editor=cg.readPathEntry("editor");
	if (editor.isEmpty()) d->chooser->editorCombo->setCurrentIndex(0);
	else
	{
		int idx=d->elements.indexOf(editor);
		idx=idx+1;
		d->chooser->editorCombo->setCurrentIndex(idx);
	}
}

void EditorChooser::writeAppSetting(const QString& postfix){
	KConfigGroup cg(KGlobal::config(), postfix.isEmpty() ? "KTEXTEDITOR:" : "KTEXTEDITOR:"+postfix);
	cg.writeEntry("DEVELOPER_INFO","NEVER TRY TO USE VALUES FROM THAT GROUP, THEY ARE SUBJECT TO CHANGES");
	cg.writePathEntry("editor", (d->chooser->editorCombo->currentIndex()==0) ?
		QString() : QString(d->elements.at(d->chooser->editorCombo->currentIndex()-1)));
}

KTextEditor::Editor *EditorChooser::editor(const QString& postfix,bool fallBackToKatePart){

	KTextEditor::Editor *tmpEd=0;

	KConfigGroup cg(KGlobal::config(), postfix.isEmpty() ? "KTEXTEDITOR:" : "KTEXTEDITOR:"+postfix);
        QString editor=cg.readPathEntry("editor");
	if (editor.isEmpty())
	{
		KConfig config("default_components");
  		config.setGroup("KTextEditor");
	  	editor = config.readPathEntry("embeddedEditor", "katepart");
	}

	KService::Ptr serv=KService::serviceByDesktopName(editor);
	if (serv)
	{
		tmpEd=KTextEditor::editor(serv->library().toLatin1());
		if (tmpEd) return tmpEd;
	}
	if (fallBackToKatePart)
		return KTextEditor::editor("katepart");

	return 0;
}

