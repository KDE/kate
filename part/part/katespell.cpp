/* This file is part of the KDE libraries
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
#include <kstdaction.h>
#include <kspell.h>
#include <kdebug.h>
#include <kmessagebox.h>

KateSpell::KateSpell( KateView* view )
  : QObject( view )
  , m_view (view)
  , m_kspell (0)
{
}

KateSpell::~KateSpell()
{
  // kspell stuff
  if( m_kspell )
  {
    m_kspell->setAutoDelete(true);
    m_kspell->cleanUp(); // need a way to wait for this to complete
    delete m_kspell;
  }
}

void KateSpell::createActions( KActionCollection* ac )
{
   KStdAction::spelling( this, SLOT(spellcheck()), ac );
   KAction *a = new KAction( i18n("Spelling (from cursor)..."), "spellcheck", 0, this, SLOT(spellcheckFromCursor()), ac, "tools_spelling_from_cursor" );
   a->setWhatsThis(i18n("Check the document's spelling from the cursor and forward"));

   m_spellcheckSelection = new KAction( i18n("Spellcheck Selection..."), "spellcheck", 0, this, SLOT(spellcheckSelection()), ac, "tools_spelling_selection" );
   m_spellcheckSelection->setWhatsThis(i18n("Check spelling of the selected text"));
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
    int lln = m_view->doc()->lastLine();
    m_spellEnd.setLine( lln );
    m_spellEnd.setColumn( m_view->doc()->lineLength( lln ) );
  }

  m_spellPosCursor = from;
  m_spellLastPos = 0;

  QString mt = m_view->doc()->mimeType()/*->name()*/;

  KSpell::SpellerType type = KSpell::Text;
  if ( mt == "text/x-tex" || mt == "text/x-latex" )
    type = KSpell::TeX;
  else if ( mt == "text/html" || mt == "text/xml" )
    type = KSpell::HTML;

  m_kspell = new KSpell( 0, i18n("Spellcheck"),
                         this, SLOT(ready(KSpell *)), 0, true, false, type );

  connect( m_kspell, SIGNAL(death()),
           this, SLOT(spellCleanDone()) );

  connect( m_kspell, SIGNAL(misspelling(const QString&, const QStringList&, unsigned int)),
           this, SLOT(misspelling(const QString&, const QStringList&, unsigned int)) );
  connect( m_kspell, SIGNAL(corrected(const QString&, const QString&, unsigned int)),
           this, SLOT(corrected(const QString&, const QString&, unsigned int)) );
  connect( m_kspell, SIGNAL(done(const QString&)),
           this, SLOT(spellResult(const QString&)) );
}

void KateSpell::ready(KSpell *)
{
  m_kspell->setProgressResolution( 1 );

  m_kspell->check( m_view->doc()->text( KTextEditor::Range(m_spellStart, m_spellEnd) ) );

  kDebug (13020) << "SPELLING READY STATUS: " << m_kspell->status () << endl;
}

KTextEditor::Cursor KateSpell::locatePosition( int pos )
{
  uint remains;

  while ( m_spellLastPos < pos )
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

void KateSpell::misspelling( const QString& origword, const QStringList&, unsigned int pos )
{
  KTextEditor::Cursor cursor = locatePosition( pos );

  m_view->setCursorPositionInternal (cursor, 1);
  m_view->setSelection( cursor, origword.length() );
}

void KateSpell::corrected( const QString& originalword, const QString& newword, unsigned int pos )
{
  KTextEditor::Cursor cursor = locatePosition( pos );

  m_view->doc()->replaceText( KTextEditor::Range(cursor, originalword.length()), newword );
}

void KateSpell::spellResult( const QString& )
{
  m_view->clearSelection();
  m_kspell->cleanUp();
}

void KateSpell::spellCleanDone()
{
  KSpell::spellStatus status = m_kspell->status();

  if( status == KSpell::Error ) {
    KMessageBox::sorry( 0,
      i18n("The spelling program could not be started. "
           "Please make sure you have set the correct spelling program "
           "and that it is properly configured and in your PATH."));
  } else if( status == KSpell::Crashed ) {
    KMessageBox::sorry( 0,
      i18n("The spelling program seems to have crashed."));
  }

  delete m_kspell;
  m_kspell = 0;

  kDebug (13020) << "SPELLING END" << endl;
}
//END


// kate: space-indent on; indent-width 2; replace-tabs on;
