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

#include <qlabel.h>
#include <qregexp.h>
#include <qstyle.h>
#include <qmenu.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kaction.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <QFocusEvent>

#include "ISearchPlugin.h"
#include "ISearchPlugin.moc"

K_EXPORT_COMPONENT_FACTORY( ktexteditor_isearch, KGenericFactory<ISearchPlugin>( "ktexteditor_isearch" ) )

ISearchPluginView::ISearchPluginView( KTextEditor::View *view )
	: QObject ( view ), KXMLGUIClient (view)
	, m_view( 0L )
	, m_doc( 0L )
	, m_searchIF( 0L )
//	, m_toolBarAction( 0L )
	, m_searchForwardAction( 0L )
	, m_searchBackwardAction( 0L )
	, m_label( 0L )
	, m_combo( 0L )
	, m_lastString( "" )
	, m_searchBackward( false )
	, m_caseSensitive( false )
	, m_fromBeginning( false )
	, m_regExp( false )
	, m_autoWrap( false )
	, m_wrapped( false )
	, m_toolBarWasHidden( false )
{
	view->insertChildClient (this);

	setInstance( KGenericFactory<ISearchPlugin>::instance() );

	m_searchForwardAction = new KAction(
		i18n("Search Incrementally"), Qt::CTRL+Qt::ALT+Qt::Key_F,
		this, SLOT(slotSearchForwardAction()),
		actionCollection(), "edit_isearch" );
	m_searchBackwardAction = new KAction(
		i18n("Search Incrementally Backwards"), Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_F,
		this, SLOT(slotSearchBackwardAction()),
		actionCollection(), "edit_isearch_reverse" );

	m_label = new QLabel( i18n("I-Search:"), 0L, "kde toolbar widget" );
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
	connect( m_combo, SIGNAL(aboutToShowContextMenu(QMenu*)),
		 this, SLOT(slotAddContextMenuItems(QMenu*)) );
	m_comboAction = new KWidgetAction(
		m_combo,
		i18n("Search"), 0, 0, 0,
		actionCollection(), "isearch_combo" );
	m_comboAction->setAutoSized( true );
	m_comboAction->setShortcutConfigurable( false );

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
	m_combo->lineEdit()->removeEventFilter( this );
	delete m_combo;
	delete m_label;
}

void ISearchPluginView::setView( KTextEditor::View* view )
{
	m_view = view;
	m_doc  = m_view->document();
	m_searchIF = qobject_cast<KTextEditor::SearchInterface *>( m_doc );
	if( !m_doc ) {
		m_view = 0L;
		m_doc = 0L;
		m_searchIF = 0L;
	}

	readConfig();
}

void ISearchPluginView::readConfig()
{
    // KConfig* config = instance()->config();
}

void ISearchPluginView::writeConfig()
{
    // KConfig* config = instance()->config();
}

void ISearchPluginView::setCaseSensitive( bool caseSensitive )
{
	m_caseSensitive = caseSensitive;
}

void ISearchPluginView::setFromBeginning( bool fromBeginning )
{
	m_fromBeginning = fromBeginning;

	if( m_fromBeginning ) {
		m_search.setPosition(0,0);
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
		if( focusEvent->reason() == Qt::ActiveWindowFocusReason ||
		    focusEvent->reason() == Qt::PopupFocusReason )
			return false;
		startSearch();
	}

	if( e->type() == QEvent::FocusOut ) {
		QFocusEvent* focusEvent = (QFocusEvent*)e;
		if( focusEvent->reason() == Qt::ActiveWindowFocusReason ||
		    focusEvent->reason() == Qt::PopupFocusReason )
			return false;
		endSearch();
	}

	if( e->type() == QEvent::KeyPress ) {
		QKeyEvent *keyEvent = (QKeyEvent*)e;
		if( keyEvent->key() == Qt::Key_Escape )
			quitToView( QString() );
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
		text = i18n("Incremental Search", "I-Search:");
	// 1000
	} else if ( failing && !reverse && !wrapped && !overwrapped ) {
		text = i18n("Incremental Search found no match", "Failing I-Search:");
	// 0100
	} else if ( !failing && reverse && !wrapped && !overwrapped ) {
		text = i18n("Incremental Search in the reverse direction", "I-Search Backward:");
	// 1100
	} else if ( failing && reverse && !wrapped && !overwrapped ) {
		text = i18n("Failing I-Search Backward:");
	// 0010
	} else if ( !failing && !reverse && wrapped  && !overwrapped ) {
		text = i18n("Incremental Search has passed the end of the document", "Wrapped I-Search:");
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
		text = i18n("Incremental Search has passed both the end of the document "
		            "and the original starting position", "Overwrapped I-Search:");
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
		text = i18n("Error: unknown i-search state!");
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
		if( m_comboAction->container(0) && m_comboAction->container(0)->isHidden() ) {
			m_toolBarWasHidden = true;
			m_comboAction->container(0)->setHidden( false );
		} else {
			m_toolBarWasHidden = false;
		}
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
			m_search = m_match.end();
		} else {
			m_search = m_match.start();
		}
		state = MatchSearch;
	}

	KTextEditor::Range found = iSearch( m_search, text, reverse, m_autoWrap );
	if( found.isValid() ) {
		m_search = m_match.end();
	} else {
		m_wrapped = true;
		m_search.setPosition(0,0);
	}
}

void ISearchPluginView::startSearch()
{
	if( !m_view ) return;

	m_searchForwardAction->setText( i18n("Next Incremental Search Match") );
	m_searchBackwardAction->setText( i18n("Previous Incremental Search Match") );

	m_wrapped = false;

	if( m_fromBeginning )
		m_start.setPosition(0,0);
	else
		m_start = m_view->cursorPosition();

	m_search = m_start;

	updateLabelText( false, m_searchBackward );

	m_combo->blockSignals( true );

	QString text = m_view->selectionText();
	if( text.isEmpty() )
		text = m_lastString;
	m_combo->setCurrentText( text );

	m_combo->blockSignals( false );
	m_combo->lineEdit()->selectAll();

//	kDebug() << "Starting search at " << m_startLine << ", " << m_startCol << endl;
}

void ISearchPluginView::endSearch()
{
	m_searchForwardAction->setText( i18n("Search Incrementally") );
	m_searchBackwardAction->setText( i18n("Search Incrementally Backwards") );

	updateLabelText();

	if( m_toolBarWasHidden && m_comboAction->containerCount() > 0 ) {
		m_comboAction->container(0)->setHidden( true );
	}
}

void ISearchPluginView::quitToView( const QString &text )
{
	if( !text.isNull() && !text.isEmpty() ) {
		m_combo->addToHistory( text );
		m_combo->insertItem( text );
		m_lastString = text;
	}

	m_combo->blockSignals( true );
	m_combo->clear();
	m_combo->blockSignals( false );

	if( m_view ) {
		m_view->setFocus(); // Will call endSearch()
	}
}

void ISearchPluginView::slotTextChanged( const QString& text )
{
	state = TextSearch;

	if( text.isEmpty() )
		return;

	iSearch( m_search, text, m_searchBackward, m_autoWrap );
}

void ISearchPluginView::slotReturnPressed( const QString& text )
{
	quitToView( text );
}

void ISearchPluginView::slotAddContextMenuItems( QMenu *menu )
{
	if( menu ) {
		menu->insertSeparator();
		menu->insertItem( i18n("Case Sensitive"), this,
				  SLOT(setCaseSensitive(bool)));
		menu->insertItem( i18n("From Beginning"), this,
				  SLOT(setFromBeginning(bool)));
		menu->insertItem( i18n("Regular Expression"), this,
				  SLOT(setRegExp(bool)));
		//menu->insertItem( i18n("Auto-Wrap Search"), this,
		//		  SLOT(setAutoWrap(bool)));
	}
}

KTextEditor::Range ISearchPluginView::iSearch(
	const KTextEditor::Cursor& start,
	const QString& text, bool reverse,
	bool autoWrap )
{
	if( !m_view ) return KTextEditor::Range::invalid();

//	kDebug() << "Searching for " << text << " at " << startLine << ", " << startCol << endl;
	KTextEditor::Range match;

	if( !m_regExp ) {
		match = m_searchIF->searchText( start,
			           text,
			           m_caseSensitive,
			           reverse );
	} else {
		match = m_searchIF->searchText( start,
			           QRegExp( text ),
			           reverse );
	}

	if( match.isValid() ) {
//		kDebug() << "Found '" << text << "' at " << m_matchLine << ", " << m_matchCol << endl;
//		v->gotoLineNumber( m_matchLine );
		m_view->setCursorPosition( match.end() );
		m_view->setSelection( match );
	} else if ( autoWrap ) {
		m_wrapped = true;
		match = iSearch( KTextEditor::Cursor(), text, reverse, false );
	}
	// FIXME
	bool overwrapped = m_wrapped && match.start() >= m_start;
//	kDebug() << "Overwrap = " << overwrapped << ". Start was " << m_startLine << ", " << m_startCol << endl;
	updateLabelText( !match.isValid(), reverse, m_wrapped, overwrapped );
	return match;
}

ISearchPlugin::ISearchPlugin( QObject *parent, const char* name, const QStringList& )
	: KTextEditor::Plugin ( parent )
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
	for (int z=0; z < m_views.count(); z++)
        {
		if (m_views.at(z)->parentClient() == view)
		{
			ISearchPluginView *nview = m_views.at(z);
			m_views.remove (nview);
			delete nview;
		}
	}
}
