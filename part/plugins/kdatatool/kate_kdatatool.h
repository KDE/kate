/* This file is part of the KDE libraries
   Copyright (C) 2002 Joseph Wenninger <jowenn@jowenn.at> and Daniel Naber <daniel.naber@t-online.de>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id: $

#ifndef _KATE_KDATATOOL_
#define _KATE_KDATATOOL_

#include <ktexteditor/plugin.h>
#include <qstringlist.h>
#include <kxmlguiclient.h>
#include <qguardedptr.h>

class KActionMenu;
class KDataToolInfo;

namespace KTextEditor
{

class View;

class KDataToolPlugin : public KTextEditor::Plugin, public KTextEditor::PluginViewInterface
{
	Q_OBJECT

public:
	KDataToolPlugin( QObject *parent = 0, const char* name = 0, const QStringList &args = QStringList() );
	virtual ~KDataToolPlugin();
	void addView (KTextEditor::View *view);
	void removeView (KTextEditor::View *){;}

//  private:
//	QPtrList<class ISearchPluginView> m_views;
};


class KDataToolPluginView : public QObject, public KXMLGUIClient
{
	Q_OBJECT

public:
	KDataToolPluginView( KTextEditor::View *view );
	virtual ~KDataToolPluginView();
	void setView( KTextEditor::View* ){;}
private:
	View *m_view;
	bool m_singleWord;
	uint m_singleWord_line, m_singleWord_start, m_singleWord_end;
	QString m_wordUnderCursor;
	QPtrList<KAction> m_actionList;
	QGuardedPtr<KActionMenu> m_menu;
protected slots:
	void aboutToShow();
	void slotToolActivated( const KDataToolInfo &datatoolinfo, const QString &string );
};

};

#endif
