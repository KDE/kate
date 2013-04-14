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

#ifndef KDEVICONINSERTERPLUGIN_H
#define KDEVICONINSERTERPLUGIN_H

#include <ktexteditor/plugin.h>

#include <kpluginfactory.h>

#include <QVariantList>

#include <kaction.h>
#include <kxmlguiclient.h>

using namespace KTextEditor;

class IconInserterPlugin;

class IconInserterPluginView: public QObject, public KXMLGUIClient
{
	Q_OBJECT
	public:
		IconInserterPluginView(IconInserterPlugin *plugin, KTextEditor::View *view);
		virtual ~IconInserterPluginView();
	private slots:
		void insertIcon();
	private:
		QPointer<KTextEditor::View> m_view;
};

class IconInserterPlugin: public Plugin
{
	Q_OBJECT
	public:
		IconInserterPlugin (QObject *parent, const QVariantList & = QVariantList());
		~IconInserterPlugin();
		void addView (View *view);
		void removeView(View *view);
		virtual void readConfig (KConfig*) {}
		virtual void writeConfig (KConfig*) {}
	private:
	QMap<KTextEditor::View*,IconInserterPluginView*> m_views;
};

K_PLUGIN_FACTORY_DECLARATION(IconInserterPluginFactory)

#endif
