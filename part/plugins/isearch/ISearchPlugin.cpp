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

#include <qtoolbutton.h>
#include <qregexp.h>
#include <qstyle.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kaction.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>

#include "ISearchPlugin.h"
#include "ISearchPlugin.moc"

K_EXPORT_COMPONENT_FACTORY( ktexteditor_isearch, KGenericFactory<ISearchPlugin>( "ktexteditor_isearch" ) );

namespace
{

// Copied from kdebase/konqueror/konq_misc.cc with modifications.
// Yay code duplication. Hacking widgets to support a specific
// style is bad bad bad. Why oh why doesn't the toolbar draw the
// darn gradient once, and then have buttons paint themselves with
// a mask? (John)
// Use a toolbutton instead of a label so it is styled correctly. (gallium)
class KToolBarLabel : public QToolButton
{
public:
	KToolBarLabel( const QString & text, QWidget* parent, const char* name = 0 )
		: QToolButton( parent, name )
	{ setText( text ); }

protected:
	QSize sizeHint() const { return QSize( fontMetrics().width( text() ), 
	                                       fontMetrics().height() ); }
	void drawButton( QPainter * p )
	{
		// Draw the background
		style().drawComplexControl( QStyle::CC_ToolButton, p, this, rect(), colorGroup(),
		    QStyle::Style_Enabled, QStyle::SC_ToolButton );
		// Draw the label
		style().drawControl( QStyle::CE_ToolButtonLabel, p, this, rect(), colorGroup(),
		     QStyle::Style_Enabled );
}
	void enterEvent( QEvent* ) {};
	void leaveEvent( QEvent* ) {};
};

}
                                            
ISearchPluginView::ISearchPluginView( KTextEditor::View *view )
	: QObject ( view ), KXMLGUIClient (view)
 	, m_view( 0L )
	, m_doc( 0L )
	, m_searchIF( 0L )
	, m_cursorIF( 0L )
	, m_selectIF( 0L )
//	, m_toolBarAction( 0L )
	, m_searchForwardAction( 0L )
	, m_searchBackwardAction( 0L )
	, m_label( 0L )
	, m_combo( 0L )
	, m_searchBackward( false )
	, m_caseSensitive( false )
	, m_fromBeginning( false )
	, m_regExp( false )
	, m_autoWrap( false )
	, m_wrapped( false )
	, m_startLine( 0 )
	, m_startCol( 0 )
	, m_searchLine( 0 )
	, m_searchCol( 0 )
	, m_foundLine( 0 )
	, m_foundCol( 0 )
	, m_matchLen( 0 )
	, m_toolBarWasHidden( false )
{      
  view->insertChildClient (this);

	setInstance( KGenericFactory<ISearchPlugin>::instance() );
	
	m_searchForwardAction = new KAction(
		i18n("Search Incrementally"), CTRL+ALT+Key_F,
		this, SLOT(slotSearchForwardAction()),
		actionCollection(), "edit_isearch" );
	m_searchBackwardAction = new KAction(
		i18n("Search Incrementally Backwards"), CTRL+ALT+SHIFT+Key_F,
		this, SLOT(slotSearchBackwardAction()),
		actionCollection(), "edit_isearch_reverse" );
	
	m_label = new KToolBarLabel( i18n("I-Search:"), 0L );
	KWidgetAction* labelAction = new KWidgetAction(
		m_label,
		i18n("I-Search:"), 0, 0, 0,
		actionCollection(), "isearch_label" );
	labelAction->setShortcutConfigurable( false );
	
	m_combo = new KHistoryCombo();
	m_combo->setDuplicatesEnabled( false );
	m_combo->setMaximumWidth( 300 );
	m_combo->lineEdit()->installEventFilter( this );
	connect( m_combo, SIGNAL(textChanged(const QString&)),
	         this, SLOT(slotTextChanged(const QString&)) );
	connect( m_combo, SIGNAL(returnPressed(const QString&)),
	         this, SLOT(slotReturnPressed(const QString&)) );
	KWidgetAction* comboAction = new KWidgetAction(
		m_combo,
		i18n("Search"), 0, 0, 0,
		actionCollection(), "isearch_combo" );
	comboAction->setAutoSized( true );
	comboAction->setShortcutConfigurable( false );
	
	KActionMenu* optionMenu = new KActionMenu(
		i18n("Search Options"), "configure",
		actionCollection(), "isearch_options" );
	optionMenu->setDelayed( false );
	
	KToggleAction* action = new KToggleAction(
		i18n("Case Sensitive"), KShortcut(),
		actionCollection(), "isearch_case_sensitive" );
	action->setShortcutConfigurable( false );
	connect( action, SIGNAL(toggled(bool)),
	         this, SLOT(setCaseSensitive(bool)) );
	action->setChecked( m_caseSensitive );
	optionMenu->insert( action );
	
	action = new KToggleAction(
		i18n("From Beginning"), KShortcut(),
		actionCollection(), "isearch_from_beginning" );
	action->setShortcutConfigurable( false );
	connect( action, SIGNAL(toggled(bool)),
	         this, SLOT(setFromBeginning(bool)) );
	action->setChecked( m_fromBeginning );
	optionMenu->insert( action );
	
	action = new KToggleAction(
		i18n("Regular Expression"), KShortcut(),
		actionCollection(), "isearch_reg_exp" );
	action->setShortcutConfigurable( false );
	connect( action, SIGNAL(toggled(bool)),
	         this, SLOT(setRegExp(bool)) );
	action->setChecked( m_regExp );
	optionMenu->insert( action );
	
// 	optionMenu->insert( new KActionSeparator() );
// 	
// 	action = new KToggleAction(
// 		i18n("Auto-Wrap Search"), KShortcut(),
// 		actionCollection(), "isearch_auto_wrap" );
// 	connect( action, SIGNAL(toggled(bool)),
// 	         this, SLOT(setAutoWrap(bool)) );
// 	action->setChecked( m_autoWrap );
// 	optionMenu->insert( action );
	
	setXMLFile( "ktexteditor_isearchui.rc" );
}

ISearchPluginView::~ISearchPluginView()
{       
	writeConfig();   
  delete m_combo;
  delete m_label; 
}

void ISearchPluginView::setView( KTextEditor::View* view )
{
	m_view = view;
	m_doc  = m_view->document();
	m_searchIF = KTextEditor::searchInterface ( m_doc );
	m_cursorIF = KTextEditor::viewCursorInterface ( m_view );
	m_selectIF = KTextEditor::selectionInterface ( m_doc );
	if( !m_doc || !m_cursorIF || !m_selectIF ) {
		m_view = 0L;
		m_doc = 0L;
		m_searchIF = 0L;
		m_cursorIF = 0L;
		m_selectIF = 0L;
	}
	
	readConfig();
}

void ISearchPluginView::readConfig()
{
	KConfig* config = instance()->config();
}

void ISearchPluginView::writeConfig()
{
	KConfig* config = instance()->config();
}

void ISearchPluginView::setCaseSensitive( bool caseSensitive )
{
	m_caseSensitive = caseSensitive;
}

void ISearchPluginView::setFromBeginning( bool fromBeginning )
{
	m_fromBeginning = fromBeginning;
	
	if( m_fromBeginning ) {
		m_searchLine = m_searchCol = 0;
	} 
}

void ISearchPluginView::setRegExp( bool regExp )
{
	m_regExp = regExp;
}

void ISearchPluginView::setAutoWrap( bool autoWrap )
{
	m_autoWrap = autoWrap;
}

bool ISearchPluginView::eventFilter( QObject* o, QEvent* e )
{
	if( o != m_combo->lineEdit() )
		return false;
	
	if( e->type() == QEvent::FocusIn ) {
		QFocusEvent* focusEvent = (QFocusEvent*)e;
		if( focusEvent->reason() == QFocusEvent::ActiveWindow ||
		    focusEvent->reason() == QFocusEvent::Popup )
			return false;
		startSearch();
	}
	
	if( e->type() == QEvent::FocusOut ) {
		QFocusEvent* focusEvent = (QFocusEvent*)e;
		if( focusEvent->reason() == QFocusEvent::ActiveWindow ||
		    focusEvent->reason() == QFocusEvent::Popup )
			return false;
		endSearch();
	}
	
	return false;
}

// Sigh... i18n hell.
void ISearchPluginView::updateLabelText(
	bool failing /* = false */, bool reverse /* = false */,
	bool wrapped /* = false */, bool overwrapped /* = false */ )
{
	QString text;
	// Reverse binary:
	// 0000
	if( !failing && !reverse && !wrapped && !overwrapped ) {
		text = i18n("I-Search:");
	// 1000
	} else if ( failing && !reverse && !wrapped && !overwrapped ) {
		text = i18n("Failing I-Search:");
	// 0100
	} else if ( !failing && reverse && !wrapped && !overwrapped ) {
		text = i18n("I-Search Backward:");
	// 1100
	} else if ( failing && reverse && !wrapped && !overwrapped ) {
		text = i18n("Failing I-Search Backward:");
	// 0010
	} else if ( !failing && !reverse && wrapped  && !overwrapped ) {
		text = i18n("Wrapped I-Search:");
	// 1010
	} else if ( failing && !reverse && wrapped && !overwrapped ) {
		text = i18n("Failing Wrapped I-Search:");
	// 0110
	} else if ( !failing && reverse && wrapped && !overwrapped ) {
		text = i18n("Wrapped I-Search Backward:");
	// 1110
	} else if ( failing && reverse && wrapped && !overwrapped ) {
		text = i18n("Failing Wrapped I-Search Backward:");
	// 0011
	} else if ( !failing && !reverse && overwrapped ) {
		text = i18n("Overwrapped I-Search:");
	// 1011
	} else if ( failing && !reverse && overwrapped ) {
		text = i18n("Failing Overwrapped I-Search:");
	// 0111
	} else if ( !failing && reverse && overwrapped ) {
		text = i18n("Overwrapped I-Search Backwards:");	
	// 1111
	} else if ( failing && reverse && overwrapped ) {
		text = i18n("Failing Overwrapped I-Search Backward:");
	} else {
		text = "Error: unknown i-search state!";
	}
	m_label->setText( text );
}

void ISearchPluginView::slotSearchForwardAction()
{
	slotSearchAction( false );
}

void ISearchPluginView::slotSearchBackwardAction()
{
	slotSearchAction( true );
}

void ISearchPluginView::slotSearchAction( bool reverse )
{
	if( !m_combo->hasFocus() ) {
//		if( !m_toolBarAction->isChecked() ) {
//			m_toolBarWasHidden = true;
//			m_toolBarAction->setChecked( true );
//		} else {
//	m_toolBarWasHidden = false;
//		}
		m_combo->setFocus(); // Will call startSearch()
	} else {
		nextMatch( reverse );
	}
}

void ISearchPluginView::nextMatch( bool reverse )
{
	QString text = m_combo->currentText();
	if( text.isEmpty() )
		return;
	if( state != MatchSearch ) {
		// Last search was performed by typing, start from that match.
		if( !reverse ) {
			m_searchLine = m_foundLine;
			m_searchCol = m_foundCol + m_matchLen;
		} else {
			m_searchLine = m_foundLine;
			m_searchCol = m_foundCol;
		}
		state = MatchSearch;
	}
	bool found = false;
	if( !reverse ) {
		found = iSearch( m_searchLine, m_searchCol, text, reverse, m_autoWrap );
	} else {
		found = iSearch( m_searchLine, m_searchCol, text, reverse, m_autoWrap );
	}
	if( found ) {
		m_searchLine = m_foundLine;
		m_searchCol = m_foundCol + m_matchLen;
	} else {
		m_wrapped = true;
		m_searchLine = m_searchCol = 0;
	}
}

void ISearchPluginView::startSearch()
{
	if( !m_view ) return;
	
	m_searchForwardAction->setText( i18n("Next Incremental Search Match") );
	m_searchBackwardAction->setText( i18n("Previous Incremental Search Match") );
	
	m_wrapped = false;
	
	if( m_fromBeginning ) {
		m_startLine = m_startCol = 0;
	} else {
		m_cursorIF->cursorPositionReal( &m_startLine, &m_startCol );
	}
	m_searchLine = m_startLine;
	m_searchCol = m_startCol;
	
	updateLabelText( false, m_searchBackward );
	
	m_combo->blockSignals( true );
	m_combo->setCurrentText( m_selectIF->selection() );
	m_combo->blockSignals( false );
	m_combo->lineEdit()->selectAll();
	
//	kdDebug() << "Starting search at " << m_startLine << ", " << m_startCol << endl;
}

void ISearchPluginView::endSearch()
{
	m_searchForwardAction->setText( i18n("Search Incrementally") );
	m_searchBackwardAction->setText( i18n("Search Incrementally Backwards") );
	
	updateLabelText();
	
	if( m_toolBarWasHidden ) {
//		m_toolBarAction->setChecked( false );
	}
}

void ISearchPluginView::slotTextChanged( const QString& text )
{
	state = TextSearch;
	
	if( text.isEmpty() )
		return;
	
	iSearch( m_searchLine, m_searchCol, text, m_searchBackward, m_autoWrap );
}

void ISearchPluginView::slotReturnPressed( const QString& text )
{
	if( !text.isEmpty() ) {
		m_combo->addToHistory( text );
		m_combo->insertItem( text );
	}
	
	m_combo->blockSignals( true );
	m_combo->clear();
	m_combo->blockSignals( false );
	
	if( m_view ) {
		m_view->setFocus(); // Will call endSearch()
	}
}

bool ISearchPluginView::iSearch(
	uint startLine, uint startCol,
	const QString& text, bool reverse,
	bool autoWrap )
{
	if( !m_view ) return false;
	
//	kdDebug() << "Searching for " << text << " at " << startLine << ", " << startCol << endl;
	bool found = false;
	if( !m_regExp ) {
		found = m_searchIF->searchText( startLine,
			           startCol,
			           text,
			           &m_foundLine,
			           &m_foundCol,
			           &m_matchLen,
			           m_caseSensitive,
			           reverse );
	} else {
		found = m_searchIF->searchText( startLine,
			           startCol,
			           QRegExp( text ),
			           &m_foundLine,
			           &m_foundCol,
			           &m_matchLen,
			           reverse );
	}
	if( found ) {
//		kdDebug() << "Found '" << text << "' at " << m_foundLine << ", " << m_foundCol << endl;
//		v->gotoLineNumber( m_foundLine );
		m_cursorIF->setCursorPositionReal( m_foundLine, m_foundCol + m_matchLen );
		m_selectIF->setSelection( m_foundLine, m_foundCol, m_foundLine, m_foundCol + m_matchLen );
	} else if ( autoWrap ) {
		m_wrapped = true;
		found = iSearch( 0, 0, text, reverse, false );
	}
	// FIXME
	bool overwrapped = ( m_wrapped && 
		((m_foundLine > m_startLine ) ||
		 (m_foundLine == m_startLine && m_foundCol >= m_startCol)) );
//	kdDebug() << "Overwrap = " << overwrapped << ". Start was " << m_startLine << ", " << m_startCol << endl;
	updateLabelText( !found, reverse, m_wrapped, overwrapped );
	return found;
}                

ISearchPlugin::ISearchPlugin( QObject *parent, const char* name, const QStringList& )
	: KTextEditor::Plugin ( (KTextEditor::Document*) parent, name )
{
}

ISearchPlugin::~ISearchPlugin()
{
}                    

void ISearchPlugin::addView(KTextEditor::View *view)
{                                          
  ISearchPluginView *nview = new ISearchPluginView (view);
  nview->setView (view); 
  m_views.append (nview);
}   

void ISearchPlugin::removeView(KTextEditor::View *view)
{      
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->parentClient() == view)
    {    
       ISearchPluginView *nview = m_views.at(z);
       m_views.remove (nview);
      delete nview;
    }  
}
