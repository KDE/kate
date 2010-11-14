/****************************************************************************
 *   This file is part of the KTextEditor-Icon-Inserter-Plugin              *
 *   Copyright 2009-2010 Jonathan Schmidt-Dominé <devel@the-user.org>       *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU Library General Public License version   *
 *   3, or (at your option) any later version, as published by the Free     *
 *   Software Foundation.                                                   *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful, but    *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *   Library General Public License for more details.                       *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with the kdelibs library; see the file COPYING.LIB. If   *
 *   not, write to the Free Software Foundation, Inc., 51 Franklin Street,  *
 *   Fifth Floor, Boston, MA 02110-1301, USA. or see                        *
 *   <http://www.gnu.org/licenses/>.                                        *
 ****************************************************************************/

#include "iconinserterplugin.h"

#include <kicondialog.h>

#include <kpluginfactory.h>
#include <kaboutdata.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <QMenu>
#include <kactioncollection.h>



K_PLUGIN_FACTORY_DEFINITION(IconInserterPluginFactory,
        registerPlugin<IconInserterPlugin>("ktexteditor_iconinserter");
        )
K_EXPORT_PLUGIN(IconInserterPluginFactory(KAboutData ("ktexteditor_iconinserter","ktexteditor_iconinserter", ki18n ("Select an Icon to use it inside the Code"), "0.1", ki18n ("Insert Code for KIcon-Creation"), KAboutData::License_LGPL_V3)))


IconInserterPluginView::IconInserterPluginView(IconInserterPlugin *plugin, KTextEditor::View *view): QObject(plugin),KXMLGUIClient(view),m_view(view)
{
	setComponentData( IconInserterPluginFactory::componentData() );
	setXMLFile("ktexteditor_iconinserterui.rc");
	QAction *a=actionCollection()->addAction("iconinserter_inserticon",this,SLOT(insertIcon()));	
	a->setIcon(KIcon("kcoloredit"));
	a->setText(i18n ("Insert KIcon-Code"));
	a->setToolTip (i18n ("Insert Code for KIcon-Creation"));
	a->setWhatsThis (i18n ("<b>IconInserter</b><p> Select an icon and use it as a KIcon in your source code."));
}

IconInserterPluginView::~IconInserterPluginView()
{
}


void IconInserterPluginView::insertIcon()
{
	if (m_view.isNull()) return;
	QString iconName = KIconDialog::getIcon ( KIconLoader::Desktop,
							KIconLoader::Application,
							false,
							0,
							false,
							0,
							i18n( "Select the Icon you want to use in your code as KIcon." )
						);
	if(iconName == "")
		return;
	
	View *view=m_view.data();
	Document *doc=view->document();
	
	
	QString suffix = doc->url().url();
	suffix = suffix.right(suffix.size() - suffix.lastIndexOf('.') - 1);
	QString code;
	if(suffix == "cpp" || suffix == "h" || suffix == "py")
		code = "KIcon (\"" + iconName + "\")";
	else if(suffix == "rb")
		code = "KDE::Icon.new (:\"" + iconName + "\")";
	else if(suffix == "js" || suffix == "qts" || suffix == "cs")
		code = "new KIcon (\"" + iconName + "\")";
	else if(suffix == "java")
		code = "new org.kde.kdeui.KIcon (\"" + iconName + "\")";
	else if(suffix == "fal" || suffix == "ftd")
		code = "KIcon ('" + iconName + "')";
	else if(suffix == "php")
		code = "new KIcon ('" + iconName + "')";
	else if(suffix == "pl")
		code = "KDE::Icon (\"" + iconName + "\")";
	else if(suffix == "pas")
		code = "KIcon_create ('" + iconName + "')";
	else if(suffix == "scm")
		code = "(make KIcon '" + iconName + ")";
	else if(suffix == "hs")
		code = "kIcon \"" + iconName + "\"";
	else if(suffix == "ads" || suffix == "adb")
		code = "KDEui.Icons.Constructos.Create (\"" + iconName + "\")";
	else
		code = iconName;
	doc->insertText(view->cursorPosition(), code);
}




IconInserterPlugin::IconInserterPlugin (QObject *parent, const QVariantList &)
	: Plugin (parent)
{
}

IconInserterPlugin::~IconInserterPlugin()
{
}


void IconInserterPlugin::addView (KTextEditor::View *view)
{
	m_views.insert(view,new IconInserterPluginView(this,view));
}

void IconInserterPlugin::removeView(KTextEditor::View *view)
{
	kDebug();
	delete m_views.take(view);
}

#include "iconinserterplugin.moc"
