/* This file is part of the KDE libraries
   Copyright (C) 2008 Mirko Stocker <me@misto.ch>
   Copyright (C) 2004-2005 Anders Lund <anders@alweb.dk>
   Copyright (C) 2003 Clarence Dang <dang@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "katespell.h"
#include "katespell.moc"

#include "kateview.h"
#include "katedocument.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <kicon.h>
#include <kstandardaction.h>
#include <sonnet/dialog.h>
#include <sonnet/backgroundchecker.h>

#include <kdebug.h>
#include <kmessagebox.h>

KateSpell::KateSpell( KateView* view )
  : QObject( view )
  , m_view (view)
  , m_sonnetDialog(0)
{
}

KateSpell::~KateSpell()
{
  if( m_sonnetDialog )
  {
    delete m_sonnetDialog;
  }
}

void KateSpell::createActions( KActionCollection* ac )
{
    ac->addAction( KStandardAction::Spelling, this, SLOT(spellcheck()) );

    KAction *a = new KAction( i18n("Spelling (from cursor)..."), this);
    ac->addAction("tools_spelling_from_cursor", a );
    a->setIcon( KIcon( "tools-check-spelling" ) );
    a->setWhatsThis(i18n("Check the document's spelling from the cursor and forward"));
    connect( a, SIGNAL( triggered() ), this, SLOT(spellcheckFromCursor()) );

    m_spellcheckSelection = new KAction( i18n("Spellcheck Selection..."), this );
    ac->addAction("tools_spelling_selection", m_spellcheckSelection);
    m_spellcheckSelection->setIcon( KIcon( "tools-check-spelling" ) );
    m_spellcheckSelection->setWhatsThis(i18n("Check spelling of the selected text"));
    connect( m_spellcheckSelection, SIGNAL( triggered() ), this, SLOT(spellcheckSelection()) );
}

void KateSpell::updateActions ()
{
  m_spellcheckSelection->setEnabled (m_view->selection ());
}

void KateSpell::spellcheckFromCursor()
{
  spellcheck( m_view->cursorPosition() );
}

void KateSpell::spellcheckSelection()
{
  spellcheck( m_view->selectionRange().start(), m_view->selectionRange().end() );
}

void KateSpell::spellcheck()
{
  spellcheck( KTextEditor::Cursor( 0, 0 ) );
}

void KateSpell::spellcheck( const KTextEditor::Cursor &from, const KTextEditor::Cursor &to )
{
  m_spellStart = from;
  m_spellEnd = to;

  if ( to.line() == 0 && to.column() == 0 )
  {
    m_spellEnd = m_view->doc()->documentEnd();
  }

  m_spellPosCursor = from;
  m_spellLastPos = 0;

  if ( !m_sonnetDialog )
  {
    m_sonnetDialog = new Sonnet::Dialog(new Sonnet::BackgroundChecker(this), m_view);

    connect(m_sonnetDialog,SIGNAL(done(const QString&)),this,SLOT(spellResult()));

    connect(m_sonnetDialog,SIGNAL(replace(const QString&,int,const QString&)),
        this,SLOT(corrected(const QString&,int,const QString&)));

    connect(m_sonnetDialog,SIGNAL(misspelling(const QString&,int)),
        this,SLOT(misspelling(const QString&,int)));
  }

  m_sonnetDialog->setBuffer(m_view->doc()->text( KTextEditor::Range(m_spellStart, m_spellEnd) ));
  m_sonnetDialog->show();
}

KTextEditor::Cursor KateSpell::locatePosition( int pos )
{
  uint remains;

  while ( m_spellLastPos < (uint)pos )
  {
    remains = pos - m_spellLastPos;
    uint l = m_view->doc()->lineLength( m_spellPosCursor.line() ) - m_spellPosCursor.column();
    if ( l > remains )
    {
      m_spellPosCursor.setColumn( m_spellPosCursor.column() + remains );
      m_spellLastPos = pos;
    }
    else
    {
      m_spellPosCursor.setLine( m_spellPosCursor.line() + 1 );
      m_spellPosCursor.setColumn(0);
      m_spellLastPos += l + 1;
    }
  }

  return m_spellPosCursor;
}

void KateSpell::misspelling( const QString& origword, int pos )
{
  KTextEditor::Cursor cursor = locatePosition( pos );

  m_view->setCursorPositionInternal (cursor, 1);
  m_view->setSelection( KTextEditor::Range(cursor, origword.length()) );
}

void KateSpell::corrected( const QString& originalword, int pos, const QString& newword)
{
  KTextEditor::Cursor cursor = locatePosition( pos );

  m_view->doc()->replaceText( KTextEditor::Range(cursor, originalword.length()), newword );
}

void KateSpell::spellResult()
{
  m_view->clearSelection();
}
//END


// kate: space-indent on; indent-width 2; replace-tabs on;
