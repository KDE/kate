 /***************************************************************************
                          ISearchPlugin.cpp  -  description
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

#include <qlabel.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kaction.h>
#include <kcombobox.h>
#include <kdebug.h>   
#include <kinstance.h>      


#include "plugin_isearch.h"
#include "plugin_isearch.moc"

K_EXPORT_COMPONENT_FACTORY( ktexteditorviewpluginisearch, KGenericFactory<ISearchPlugin>( "ktexteditorviewpluginisearch" ) );

ISearchPlugin::ISearchPlugin( QObject* parent, const char* name, const QStringList &args )
	: KTextEditor::ViewPlugin ( parent, name )
	, m_combo( 0L )
	, m_currentText( QString::null )
	, m_startLine( 0 )
	, m_startCol( 0 )
	, m_foundAtLine( 0 )
	, m_foundAtCol( 0 )
	, m_matchLen( 0 )     
	, myView (0)
{                       
new KAction(
		i18n("Search Incrementally"), CTRL+ALT+Key_F,
		this, SLOT(slotISearch()),
		actionCollection(), "edit_isearch" );
	
	new KToggleToolBarAction(
		"isearchToolBar", i18n("Show Search Toolbar"),
		actionCollection(), "settings_show_search_toolbar" );
	
	m_combo = new KHistoryCombo();
	m_combo->setDuplicatesEnabled( false );
	m_combo->setMaximumWidth( 300 );
	connect( m_combo, SIGNAL(textChanged(const QString&)),
	         this, SLOT(slotISearch(const QString&)) );
	connect( m_combo, SIGNAL(activated(const QString&)),
	         this, SLOT(slotActivated(const QString&)) );
	new KWidgetAction( m_combo,
		i18n("Search"), 0, 0, 0,
		actionCollection(), "isearch_combo" );
	                             
	setInstance (new KInstance ("ktexteditorviewpluginisearch" ));
	setXMLFile( "isearchui.rc" );     
	
	kdDebug()<<"TEST IT NOW ISEARCH !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  "<<KXMLGUIClient::xmlFile()<<endl;

}     

void ISearchPlugin::setView (KTextEditor::View *view)
{
   myView = view;
}

ISearchPlugin::~ISearchPlugin()
{
}

void ISearchPlugin::slotISearch()
{
	if( !m_combo->hasFocus() ) {
		m_combo->setFocus();
		m_combo->lineEdit()->selectAll();
	} else {
		iSearch( m_foundAtLine, m_foundAtCol + m_matchLen, m_currentText );
	}
}

void ISearchPlugin::slotISearch( const QString& text )
{
		if( !myView ) return;     
		
	KTextEditor::ViewCursorInterface *cIface = dynamic_cast<KTextEditor::ViewCursorInterface*>(myView);
	
	if( m_currentText.isEmpty() ) {
		cIface->cursorPositionReal( &m_startLine, &m_startCol );
	}
	
	m_currentText = text;
	if( m_currentText.isEmpty() ) {
		cIface->setCursorPositionReal( m_startLine, m_startCol );
		return;
	}
	
	iSearch( 0, 0, m_currentText );
}

void ISearchPlugin::iSearch( uint startLine, uint startCol, const QString& text )
{
	if( !myView ) return;
	KTextEditor::Document* d = myView->document();
	if( !d ) return;
	
	kdDebug() << "Searching for " << text << " at " << startLine << ", " << startCol << endl;
	bool casesensitive = false;
	bool backwards = false;        
	
        KTextEditor::SearchInterface *searchIface = dynamic_cast<KTextEditor::SearchInterface*>(d);    
	KTextEditor::SelectionInterface *selIface = dynamic_cast<KTextEditor::SelectionInterface*>(d); 
	KTextEditor::ViewCursorInterface *cIface = dynamic_cast<KTextEditor::ViewCursorInterface*>(myView);
	
	if( searchIface->searchText( startLine,
	                   startCol,
	                   text,
	                   &m_foundAtLine,
	                   &m_foundAtCol,
	                   &m_matchLen,
	                   casesensitive,
	                   backwards ) ) {
		kdDebug() << "Found '" << text << "' at " << m_foundAtLine << ", " << m_foundAtCol << endl;
		//myView->gotoLineNumber( m_foundAtLine );
		cIface->setCursorPositionReal( m_foundAtLine, m_foundAtCol + m_matchLen );
		selIface->setSelection( m_foundAtLine, m_foundAtCol, m_foundAtLine, m_foundAtCol + m_matchLen );
	}
}

void ISearchPlugin::slotActivated( const QString& text )
{
	m_combo->addToHistory( text );
		
		if( myView ) {
		myView->setFocus();
	}
	
	m_currentText = QString::null;
	m_foundAtLine = m_foundAtCol = m_matchLen = 0;
	m_combo->blockSignals( true );
	m_combo->clear();
	m_combo->blockSignals( false );
}
 
