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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 */

#ifndef _ISearchPlugin_H_
#define _ISearchPlugin_H_

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/selectioninterface.h>        

#include <kxmlguiclient.h>
#include <qobject.h>

class QLabel;
class KToolBarLabel;

class ISearchPlugin : public KTextEditor::Plugin, public KTextEditor::PluginViewInterface
{             
  Q_OBJECT

  public:
	  ISearchPlugin( QObject *parent = 0, const char* name = 0, const QStringList &args = QStringList() );
	  virtual ~ISearchPlugin();       
    
    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);  
    
  private:
    QPtrList<class ISearchPluginView> m_views;
};

class ISearchPluginView : public QObject, public KXMLGUIClient
{
	Q_OBJECT
	
public:
	ISearchPluginView( KTextEditor::View *view );
	virtual ~ISearchPluginView();
		
	virtual bool eventFilter( QObject*, QEvent* );
  
  void setView( KTextEditor::View* view );   
  
public slots:
	void setCaseSensitive( bool );
	void setFromBeginning( bool );
	void setRegExp( bool );
	void setAutoWrap( bool );

private slots:
	void slotSearchForwardAction();
	void slotSearchBackwardAction();
	void slotSearchAction( bool reverse );
	void slotTextChanged( const QString& text );
	void slotReturnPressed( const QString& text );

private:
	void readConfig();
	void writeConfig();
	
	void updateLabelText( bool failing = false, bool reverse = false,
	                      bool wrapped = false, bool overwrapped = false );
	void startSearch();
	void endSearch();
	void nextMatch( bool reverse );
	bool iSearch( uint startLine, uint startCol,
	              const QString& text, bool reverse, bool autoWrap );

	KTextEditor::View*     m_view;
	KTextEditor::Document* m_doc;
	KTextEditor::SearchInterface* m_searchIF;
	KTextEditor::ViewCursorInterface* m_cursorIF;
	KTextEditor::SelectionInterface* m_selectIF;
       	KAction*               m_searchForwardAction;
	KAction*               m_searchBackwardAction;
	KToolBarLabel* m_label;
	KHistoryCombo* m_combo;
	bool           m_searchBackward;
	bool           m_caseSensitive;
	bool           m_fromBeginning;
	bool           m_regExp;
	bool           m_autoWrap;
	bool           m_wrapped;
	uint           m_startLine, m_startCol;
	uint           m_searchLine, m_searchCol;
	uint           m_foundLine, m_foundCol, m_matchLen;
	bool           m_toolBarWasHidden;
	enum { NoSearch, TextSearch, MatchSearch } state;
};

#endif // _ISearchPlugin_H_
