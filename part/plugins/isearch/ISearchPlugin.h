 /* This file is part of the KDE libraries
    Copyright (C) 2002 by John Firebaugh <jfirebaugh@kde.org>

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

#ifndef _ISearchPlugin_H_
#define _ISearchPlugin_H_

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/range.h>

#include <kxmlguiclient.h>
#include <qobject.h>
#include <qpointer.h>

class QLabel;

class ISearchPlugin : public KTextEditor::Plugin
{
	Q_OBJECT

public:
	ISearchPlugin( QObject *parent = 0, const char* name = 0, const QStringList &args = QStringList() );
	virtual ~ISearchPlugin();

	void addView (KTextEditor::View *view);
	void removeView (KTextEditor::View *view);

private:
	QList<class ISearchPluginView*> m_views;
};

class ISearchPluginView : public QObject, public KXMLGUIClient
{
	Q_OBJECT

public:
	ISearchPluginView( KTextEditor::View *view );
	virtual ~ISearchPluginView();

	virtual bool eventFilter( QObject*, QEvent* );

	void setView( KTextEditor::View* view );

public Q_SLOTS:
	void setCaseSensitive( bool );
	void setFromBeginning( bool );
	void setRegExp( bool );
	void setAutoWrap( bool );

private Q_SLOTS:
	void slotSearchForwardAction();
	void slotSearchBackwardAction();
	void slotSearchAction( bool reverse );
	void slotTextChanged( const QString& text );
	void slotReturnPressed( const QString& text );
	void slotAddContextMenuItems( QMenu *menu);

private:
	void readConfig();
	void writeConfig();

	void updateLabelText( bool failing = false, bool reverse = false,
	                      bool wrapped = false, bool overwrapped = false );
	void startSearch();
	void endSearch();
	void quitToView( const QString &text );

	void nextMatch( bool reverse );
	KTextEditor::Range iSearch( const KTextEditor::Cursor& start,
	              const QString& text, bool reverse, bool autoWrap );

	KTextEditor::View*     m_view;
	KTextEditor::Document* m_doc;
	KTextEditor::SearchInterface* m_searchIF;
	KAction*               m_searchForwardAction;
	KAction*               m_searchBackwardAction;
	KWidgetAction*         m_comboAction;
	QPointer<QLabel>    m_label;
	QPointer<KHistoryCombo> m_combo;
	QString        m_lastString;
	bool           m_searchBackward;
	bool           m_caseSensitive;
	bool           m_fromBeginning;
	bool           m_regExp;
	bool           m_autoWrap;
	bool           m_wrapped;
	KTextEditor::Cursor           m_start;
	KTextEditor::Cursor           m_search;
	KTextEditor::Range            m_match;
	bool           m_toolBarWasHidden;
	enum { NoSearch, TextSearch, MatchSearch } state;
};

#endif // _ISearchPlugin_H_
