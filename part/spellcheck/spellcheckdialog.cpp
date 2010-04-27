/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2010 by Michel Ludwig <michel.ludwig@kdemail.net>
 *  Copyright (C) 2008 Mirko Stocker <me@misto.ch>
 *  Copyright (C) 2004-2005 Anders Lund <anders@alweb.dk>
 *  Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
 *  Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "spellcheckdialog.h"
#include "spellcheckdialog.moc"

#include "katedocument.h"
#include "kateglobal.h"
#include "kateview.h"
#include "spellcheck/spellcheck.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <kicon.h>
#include <kstandardaction.h>
#include <sonnet/dialog.h>
#include <sonnet/backgroundchecker.h>
#include <sonnet/speller.h>

KateSpellCheckDialog::KateSpellCheckDialog( KateView* view )
  : QObject( view )
  , m_view (view)
  , m_speller (NULL)
  , m_backgroundChecker(NULL)
  , m_sonnetDialog(NULL)
  , m_globalSpellCheckRange(NULL)
  , m_spellCheckCancelledByUser(false)
{
}

KateSpellCheckDialog::~KateSpellCheckDialog()
{
    delete m_globalSpellCheckRange;
    delete m_sonnetDialog;
    delete m_backgroundChecker;
    delete m_speller;
}

void KateSpellCheckDialog::createActions( KActionCollection* ac )
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

void KateSpellCheckDialog::updateActions ()
{
  m_spellcheckSelection->setEnabled (m_view->selection ());
}

void KateSpellCheckDialog::spellcheckFromCursor()
{
  spellcheck( m_view->cursorPosition() );
}

void KateSpellCheckDialog::spellcheckSelection()
{
  spellcheck( m_view->selectionRange().start(), m_view->selectionRange().end() );
}

void KateSpellCheckDialog::spellcheck()
{
  spellcheck( KTextEditor::Cursor( 0, 0 ) );
}

void KateSpellCheckDialog::spellcheck( const KTextEditor::Cursor &from, const KTextEditor::Cursor &to )
{
  KTextEditor::Cursor start = from;
  KTextEditor::Cursor end = to;

  if ( end.line() == 0 && end.column() == 0 )
  {
    end = m_view->doc()->documentEnd();
  }

  if ( !m_speller )
  {
    m_speller = new Sonnet::Speller();
  }
  else
  {
    m_speller->restore(KGlobal::config().data());
  }

  if ( !m_backgroundChecker )
  {
    m_backgroundChecker = new Sonnet::BackgroundChecker(*m_speller);
  }

  if ( !m_sonnetDialog )
  {
    m_sonnetDialog = new Sonnet::Dialog(m_backgroundChecker, m_view);
    m_sonnetDialog->showProgressDialog(200);
    m_sonnetDialog->showSpellCheckCompletionMessage();
    m_sonnetDialog->setSpellCheckContinuedAfterReplacement(false);

    connect(m_sonnetDialog,SIGNAL(done(const QString&)),this,SLOT(installNextSpellCheckRange()));

    connect(m_sonnetDialog,SIGNAL(replace(const QString&,int,const QString&)),
        this,SLOT(corrected(const QString&,int,const QString&)));

    connect(m_sonnetDialog,SIGNAL(misspelling(const QString&,int)),
        this,SLOT(misspelling(const QString&,int)));

    connect(m_sonnetDialog,SIGNAL(cancel()),
        this,SLOT(cancelClicked()));

    connect(m_sonnetDialog,SIGNAL(destroyed(QObject*)),
            this,SLOT(objectDestroyed(QObject*)));
  }

  delete m_globalSpellCheckRange;
  // we expand to handle the situation when the last word in the range is replace by a new one
  m_globalSpellCheckRange = m_view->doc()->newMovingRange (KTextEditor::Range( start, end ),
                                                           KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);
  m_spellCheckCancelledByUser = false;
  performSpellCheck( *m_globalSpellCheckRange );
}

KTextEditor::Cursor KateSpellCheckDialog::locatePosition( int pos )
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

void KateSpellCheckDialog::misspelling( const QString& word, int pos )
{
  KTextEditor::Cursor cursor;
  int length;
  int origPos = m_view->doc()->computePositionWrtOffsets( m_currentDecToEncOffsetList, pos );
  cursor = locatePosition( origPos );
  length = m_view->doc()->computePositionWrtOffsets( m_currentDecToEncOffsetList, pos + word.length() )
            - origPos;

  m_view->setCursorPositionInternal (cursor, 1);
  m_view->setSelection( KTextEditor::Range(cursor, length) );
}

void KateSpellCheckDialog::corrected( const QString& word, int pos, const QString& newWord )
{
  int origPos = m_view->doc()->computePositionWrtOffsets( m_currentDecToEncOffsetList, pos );

  int length = m_view->doc()->computePositionWrtOffsets( m_currentDecToEncOffsetList, pos + word.length() )
                - origPos;

  KTextEditor::Cursor replacementStartCursor = locatePosition( origPos );
  KTextEditor::Range replacementRange = KTextEditor::Range( replacementStartCursor, length );
  KateDocument *doc = m_view->doc();
  KateGlobal::self()->spellCheckManager()->replaceCharactersEncodedIfNecessary( newWord, doc, replacementRange );

  m_currentSpellCheckRange.setRange( KTextEditor::Range( replacementStartCursor, m_currentSpellCheckRange.end() ) );
  // we have to be careful here: due to static word wrapping the text might change in addition to simply
  // the misspelled word being replaced, i.e. new line breaks might be inserted as well. As such, the text
  // in the 'Sonnet::Dialog' might be eventually out of sync with the visible text. Therefore, we 'restart'
  // spell checking from the current position.
  performSpellCheck( KTextEditor::Range( replacementStartCursor, m_globalSpellCheckRange->end() ) );
}

void KateSpellCheckDialog::performSpellCheck(const KTextEditor::Range& range)
{
  if(range.isEmpty()) {
    spellCheckDone();
  }
  m_languagesInSpellCheckRange = KateGlobal::self()->spellCheckManager()->spellCheckLanguageRanges(m_view->doc(), range);
  m_currentLanguageRangeIterator = m_languagesInSpellCheckRange.begin();
  m_currentSpellCheckRange = KTextEditor::Range::invalid();
  installNextSpellCheckRange();
  // first check if there is really something to spell check
  if(m_currentSpellCheckRange.isValid()) {
    m_sonnetDialog->show();
  }
}

void KateSpellCheckDialog::installNextSpellCheckRange()
{
  if ( m_spellCheckCancelledByUser
       || m_currentLanguageRangeIterator == m_languagesInSpellCheckRange.end() )
  {
    spellCheckDone();
    return;
  }
  KateSpellCheckManager *spellCheckManager = KateGlobal::self()->spellCheckManager();
  KTextEditor::Cursor nextRangeBegin = (m_currentSpellCheckRange.isValid() ? m_currentSpellCheckRange.end()
                                                                           : KTextEditor::Cursor::invalid());
  m_currentSpellCheckRange = KTextEditor::Range::invalid();
  m_currentDecToEncOffsetList.clear();
  QList<QPair<KTextEditor::Range, QString> > rangeDictionaryPairList;
  while ( m_currentLanguageRangeIterator != m_languagesInSpellCheckRange.end() )
  {
    const KTextEditor::Range& currentLanguageRange = (*m_currentLanguageRangeIterator).first;
    const QString& dictionary = (*m_currentLanguageRangeIterator).second;
    KTextEditor::Range languageSubRange = (nextRangeBegin.isValid() ? KTextEditor::Range(nextRangeBegin, currentLanguageRange.end())
                                                                    : currentLanguageRange);
    rangeDictionaryPairList = spellCheckManager->spellCheckWrtHighlightingRanges(m_view->doc(),
                                                                                 languageSubRange,
                                                                                 dictionary,
                                                                                 false, true);
    Q_ASSERT(rangeDictionaryPairList.size() <= 1);
    if(rangeDictionaryPairList.size() == 0) {
      ++m_currentLanguageRangeIterator;
      if ( m_currentLanguageRangeIterator != m_languagesInSpellCheckRange.end() )
      {
        nextRangeBegin = (*m_currentLanguageRangeIterator).first.start();
      }
    }
    else {
      m_currentSpellCheckRange = rangeDictionaryPairList.first().first;
      const QString& dictionary = rangeDictionaryPairList.first().second;

      m_spellPosCursor = m_currentSpellCheckRange.start();
      m_spellLastPos = 0;

      m_currentDecToEncOffsetList.clear();
      KateDocument::OffsetList encToDecOffsetList;
      QString text = m_view->doc()->decodeCharacters(m_currentSpellCheckRange,
                                                     m_currentDecToEncOffsetList,
                                                     encToDecOffsetList);
      // ensure that no empty string is passed on to Sonnet as this can lead to a crash
      // (bug 228789)
      if(text.isEmpty()) {
        nextRangeBegin = m_currentSpellCheckRange.end();
        continue;
      }

      if(m_speller->language() != dictionary) {
        m_speller->setLanguage(dictionary);
        m_backgroundChecker->setSpeller(*m_speller);
      }

      m_sonnetDialog->setBuffer(text);
      break;
    }
  }
  if ( m_currentLanguageRangeIterator == m_languagesInSpellCheckRange.end() )
  {
    spellCheckDone();
    return;
  }
}

void KateSpellCheckDialog::cancelClicked()
{
  m_spellCheckCancelledByUser = true;
}

void KateSpellCheckDialog::spellCheckDone()
{
  m_currentSpellCheckRange = KTextEditor::Range::invalid();
  m_currentDecToEncOffsetList.clear();
  m_view->clearSelection();
}

void KateSpellCheckDialog::objectDestroyed(QObject *object)
{
  Q_UNUSED(object);
  m_sonnetDialog = NULL;
}

//END


// kate: space-indent on; indent-width 2; replace-tabs on;
