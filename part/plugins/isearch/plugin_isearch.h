 /***************************************************************************
                          ISearchPlugin.h  -  description
                             -------------------
    begin                : Mon Apr 1 2002
    copyright            : (C) 2002 by John Firebaugh
    email                : jfirebaugh@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _ISearchPlugin_H_
#define _ISearchPlugin_H_
                       
#include <ktexteditor/document.h>     
#include <ktexteditor/view.h>   
#include <ktexteditor/plugin.h>   
#include <ktexteditor/searchinterface.h>   
#include <ktexteditor/viewcursorinterface.h>   
#include <ktexteditor/selectioninterface.h>    
 

class KHistoryCombo;

class ISearchPlugin : public KTextEditor::ViewPlugin
{
	Q_OBJECT
	
public:
	ISearchPlugin( QObject* parent = 0, const char* name = 0, const QStringList &args = QStringList() );
	virtual ~ISearchPlugin();     
	
	void setView (KTextEditor::View *view);
		
private slots:
	void iSearch( uint startLine, uint startCol, const QString& text );
	void slotISearch();
	void slotISearch( const QString& text );
	void slotActivated( const QString& text );

private:
	KHistoryCombo* m_combo;        
	QString        m_currentText;
	uint           m_startLine, m_startCol;
	uint           m_foundAtLine, m_foundAtCol, m_matchLen;  
	KTextEditor::View *myView;
};

#endif // _ISearchPlugin_H_
