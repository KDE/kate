/* This file is part of the KDE libraries
   Copyright (C) 2003 Clarence Dang <dang@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "katesearch.h"
#include "katesearch.moc"

#include "kateview.h"
#include "katedocument.h"
#include "kateviewdialog.h"
#include "katesupercursor.h"
#include "katearbitraryhighlight.h"
#include "kateconfig.h"

#include <klocale.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kdebug.h>
#include <kfinddialog.h>
#include <kreplacedialog.h>

QStringList KateSearch::s_searchList  = QStringList();
QStringList KateSearch::s_replaceList = QStringList();
static const bool arbitraryHLExample = false;

KateSearch::KateSearch( KateView* view )
  : QObject( view, "kate search" )
  , m_view( view )
  , m_doc( view->doc() )
  , replacePrompt( new ReplacePrompt( view ) )
{
  m_arbitraryHLList = new KateSuperRangeList();
  if (arbitraryHLExample) m_doc->arbitraryHL()->addHighlightToView(m_arbitraryHLList, m_view);

  connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
// TODO: Configuration
//  s_searchFlags = config->readNumEntry("SearchFlags", SConfig::sfPrompt);
//   config->writeEntry("SearchFlags",s_searchFlags);
}

KateSearch::~KateSearch()
{
}

void KateSearch::createActions( KActionCollection* ac )
{
  KStdAction::find( this, SLOT(find()), ac )->setWhatsThis(
    i18n("Look up the first occurrence of a piece of text or regular expression."));
  KStdAction::findNext( this, SLOT(slotFindNext()), ac )->setWhatsThis(
    i18n("Look up the next occurrence of the search phrase."));
  KStdAction::findPrev( this, SLOT(slotFindPrev()), ac, "edit_find_prev" )->setWhatsThis(
    i18n("Look up the previous occurrence of the search phrase."));
  KStdAction::replace( this, SLOT(replace()), ac )->setWhatsThis(
    i18n("Look up a piece of text or regular expression and replace the result with some given text."));
}

void KateSearch::addToList( QStringList& list, const QString& s )
{
  if( list.count() > 0 ) {
    QStringList::Iterator it = list.find( s );
    if( *it != 0L )
      list.remove( it );
    if( list.count() >= 16 )
      list.remove( list.fromLast() );
  }
  list.prepend( s );
}

void KateSearch::find()
{
  KFindDialog *findDialog = new KFindDialog (  m_view, "", KateViewConfig::global()->searchFlags(),
                                               s_searchList, m_doc->hasSelection() );

  findDialog->setPattern (getSearchText());

  if( findDialog->exec() == QDialog::Accepted ) {
    s_searchList =  findDialog->findHistory () ;
    KateViewConfig::global()->setSearchFlags(findDialog->options ());

    SearchFlags searchFlags;

    searchFlags.caseSensitive = KateViewConfig::global()->searchFlags() & KFindDialog::CaseSensitive;
    searchFlags.wholeWords = KateViewConfig::global()->searchFlags() & KFindDialog::WholeWordsOnly;
    searchFlags.fromBeginning = !(KateViewConfig::global()->searchFlags() & KFindDialog::FromCursor)
                               && !(KateViewConfig::global()->searchFlags() & KFindDialog::SelectedText);
    searchFlags.backward = KateViewConfig::global()->searchFlags() & KFindDialog::FindBackwards;
    searchFlags.selected = KateViewConfig::global()->searchFlags() & KFindDialog::SelectedText;
    searchFlags.prompt = false;
    searchFlags.replace = false;
    searchFlags.finished = false;
    searchFlags.regExp = KateViewConfig::global()->searchFlags() & KFindDialog::RegularExpression;
    if ( searchFlags.selected )
    {
      s.selBegin = KateTextCursor( doc()->selStartLine(), doc()->selStartCol() );
      s.selEnd   = KateTextCursor( doc()->selEndLine(),   doc()->selEndCol()   );
      s.cursor   = s.flags.backward ? s.selEnd : s.selBegin;
    } else {
      s.cursor = getCursor();
    }

    search( searchFlags );
  }

  delete findDialog;
  m_view->update ();
}

void KateSearch::replace()
{
  if (!doc()->isReadWrite()) return;

  KReplaceDialog *replaceDialog = new KReplaceDialog (  m_view, "", KateViewConfig::global()->searchFlags(),
                                               s_searchList, s_replaceList, m_doc->hasSelection() );

  replaceDialog->setPattern (getSearchText());

  if( replaceDialog->exec() == QDialog::Accepted ) {
    m_replacement = replaceDialog->replacement();
    s_searchList = replaceDialog->findHistory () ;
    s_replaceList = replaceDialog->replacementHistory () ;
    KateViewConfig::global()->setSearchFlags(replaceDialog->options ());

    SearchFlags searchFlags;
    searchFlags.caseSensitive = KateViewConfig::global()->searchFlags() & KFindDialog::CaseSensitive;
    searchFlags.wholeWords = KateViewConfig::global()->searchFlags() & KFindDialog::WholeWordsOnly;
    searchFlags.fromBeginning = !(KateViewConfig::global()->searchFlags() & KFindDialog::FromCursor)
                               && !(KateViewConfig::global()->searchFlags() & KFindDialog::SelectedText);
    searchFlags.backward = KateViewConfig::global()->searchFlags() & KFindDialog::FindBackwards;
    searchFlags.selected = KateViewConfig::global()->searchFlags() & KFindDialog::SelectedText;
    searchFlags.prompt = KateViewConfig::global()->searchFlags() & KReplaceDialog::PromptOnReplace;
    searchFlags.replace = true;
    searchFlags.finished = false;
    searchFlags.regExp = KateViewConfig::global()->searchFlags() & KFindDialog::RegularExpression;
    if ( searchFlags.selected )
    {
      s.selBegin = KateTextCursor( doc()->selStartLine(), doc()->selStartCol() );
      s.selEnd   = KateTextCursor( doc()->selEndLine(),   doc()->selEndCol()   );
      s.cursor   = s.flags.backward ? s.selEnd : s.selBegin;
    } else {
      s.cursor = getCursor();
    }

    search( searchFlags );
  }

  delete replaceDialog;
  m_view->update ();
}

void KateSearch::findAgain( bool back )
{
  SearchFlags searchFlags;
  searchFlags.caseSensitive = KateViewConfig::global()->searchFlags() & KFindDialog::CaseSensitive;
  searchFlags.wholeWords = KateViewConfig::global()->searchFlags() & KFindDialog::WholeWordsOnly;
  searchFlags.fromBeginning = !(KateViewConfig::global()->searchFlags() & KFindDialog::FromCursor)
                               && !(KateViewConfig::global()->searchFlags() & KFindDialog::SelectedText);
  searchFlags.backward = KateViewConfig::global()->searchFlags() & KFindDialog::FindBackwards;
  searchFlags.selected = KateViewConfig::global()->searchFlags() & KFindDialog::SelectedText;
  searchFlags.prompt = KateViewConfig::global()->searchFlags() & KReplaceDialog::PromptOnReplace;
  searchFlags.replace = false;
  searchFlags.finished = false;
  searchFlags.regExp = KateViewConfig::global()->searchFlags() & KFindDialog::RegularExpression;

  searchFlags.backward = searchFlags.backward != back;
  searchFlags.fromBeginning = false;
  searchFlags.prompt = true;
  s.cursor = getCursor();

  search( searchFlags );
}

void KateSearch::search( SearchFlags flags )
{
  s.flags = flags;

  if( s.flags.fromBeginning ) {
    if( !s.flags.backward ) {
      s.cursor.setPos(0, 0);
    } else {
      s.cursor.setLine(doc()->numLines() - 1);
      s.cursor.setCol(doc()->lineLength( s.cursor.line() ));
    }
  }

  if((!s.flags.backward &&
       s.cursor.col() == 0 &&
       s.cursor.line() == 0 ) ||
     ( s.flags.backward &&
       s.cursor.col() == doc()->lineLength( s.cursor.line() ) &&
       s.cursor.line() == (((int)doc()->numLines()) - 1) ) ) {
    s.flags.finished = true;
  }

  if( s.flags.replace ) {
    replaces = 0;
    if( s.flags.prompt )
      promptReplace();
    else
      replaceAll();
  } else {
    findAgain();
  }
}

void KateSearch::wrapSearch()
{
  if( s.flags.selected )
  {
      s.cursor = s.flags.backward ? s.selEnd : s.selBegin;
  }
  else
  {
    if( !s.flags.backward ) {
      s.cursor.setPos(0, 0);
    } else {
      s.cursor.setLine(doc()->numLines() - 1);
      s.cursor.setCol(doc()->lineLength( s.cursor.line() ));
    }
  }
  replaces = 0;
  s.flags.finished = true;
}

void KateSearch::findAgain()
{
  QString searchFor = s_searchList.first();

  if( searchFor.isEmpty() ) {
    find();
    return;
  }

  if ( doSearch( searchFor ) ) {
    exposeFound( s.cursor, s.matchedLength );
  } else if( !s.flags.finished ) {
    if( askContinue() ) {
      wrapSearch();
      findAgain();
    } else {
      if (arbitraryHLExample) m_arbitraryHLList->clear();
    }
  } else {
    if (arbitraryHLExample) m_arbitraryHLList->clear();
    KMessageBox::sorry( view(),
        i18n("Search string '%1' not found!")
             .arg( KStringHandler::csqueeze( searchFor ) ),
        i18n("Find"));
  }
}

void KateSearch::replaceAll()
{
  QString searchFor = s_searchList.first();

  doc()->editStart ();

  while( doSearch( searchFor ) )
    replaceOne();

  doc()->editEnd ();

  if( !s.flags.finished ) {
    if( askContinue() ) {
      wrapSearch();
      replaceAll();
    }
  } else {
    KMessageBox::information( view(),
        i18n("%n replacement made.","%n replacements made.",replaces),
        i18n("Replace") );
  }
}

void KateSearch::promptReplace()
{
  QString searchFor = s_searchList.first();
  if ( doSearch( searchFor ) ) {
    exposeFound( s.cursor, s.matchedLength );
    replacePrompt->show();
  } else if( !s.flags.finished && askContinue() ) {
    wrapSearch();
    promptReplace();
  } else {
    if (arbitraryHLExample) m_arbitraryHLList->clear();
    replacePrompt->hide();
    KMessageBox::information( view(),
        i18n("%n replacement made.","%n replacements made.",replaces),
        i18n("Replace") );
  }
}

void KateSearch::replaceOne()
{
  QString replaceWith = m_replacement;
  if ( s.flags.regExp ) {
    // replace each "(?!\)\d+" with the corresponding capture
    QRegExp br("\\\\(\\d+)");
    int pos = br.search( replaceWith );
    int ncaps = m_re.numCaptures();
    while ( pos >= 0 ) {
      QString sc;
      if ( !pos ||  replaceWith.at( pos-1) != '\\' ) {
        int ccap = br.cap(1).toInt();
        if (ccap <= ncaps ) {
          sc = m_re.cap( ccap );
          replaceWith.replace( pos, br.matchedLength(), sc );
        }
        else {
          // TODO add a sanity check at some point prior to this
          kdDebug()<<"KateSearch::replaceOne(): you don't have "<<ccap<<" backreferences in regexp '"<<m_re.pattern()<<"'"<<endl;
        }
      }
      pos = br.search( replaceWith, pos+QMAX(br.matchedLength(), (int)sc.length()) );
    }
  }

  doc()->editStart();
  doc()->removeText( s.cursor.line(), s.cursor.col(),
      s.cursor.line(), s.cursor.col() + s.matchedLength );
  doc()->insertText( s.cursor.line(), s.cursor.col(), replaceWith );
  doc()->editEnd(),

  replaces++;

  if( s.flags.selected && s.cursor.line() == s.selEnd.line() )
  {
    s.selEnd.setCol(s.selEnd.col() + replaceWith.length() - s.matchedLength );
  }

  if( !s.flags.backward ) {
    s.cursor.setCol(s.cursor.col() + replaceWith.length());
  } else if( s.cursor.col() > 0 ) {
    s.cursor.setCol(s.cursor.col() - 1);
  } else {
    s.cursor.setLine(s.cursor.line() - 1);
    if( s.cursor.line() >= 0 ) {
      s.cursor.setCol(doc()->lineLength( s.cursor.line() ));
    }
  }
}

void KateSearch::skipOne()
{
  if( !s.flags.backward ) {
    s.cursor.setCol(s.cursor.col() + s.matchedLength);
  } else if( s.cursor.col() > 0 ) {
    s.cursor.setCol(s.cursor.col() - 1);
  } else {
    s.cursor.setLine(s.cursor.line() - 1);
    if( s.cursor.line() >= 0 ) {
      s.cursor.setCol(doc()->lineLength(s.cursor.line()));
    }
  }
}

void KateSearch::replaceSlot() {
  switch( (Dialog_results)replacePrompt->result() ) {
  case srCancel: replacePrompt->hide();                break;
  case srAll:    replacePrompt->hide(); replaceAll();  break;
  case srYes:    replaceOne(); promptReplace();        break;
  case srLast:   replacePrompt->hide(), replaceOne();  break;
  case srNo:     skipOne();    promptReplace();        break;
  }
}

bool KateSearch::askContinue()
{
  QString made =
     i18n( "%n replacement made.",
           "%n replacements made.",
           replaces );

  QString reached = !s.flags.backward ?
     i18n( "End of document reached." ) :
     i18n( "Beginning of document reached." );

  if (KateViewConfig::global()->searchFlags() & KFindDialog::SelectedText)
  {
    reached = !s.flags.backward ?
     i18n( "End of selection reached." ) :
     i18n( "Beginning of selection reached." );
  }

  QString question = !s.flags.backward ?
     i18n( "Continue from the beginning?" ) :
     i18n( "Continue from the end?" );

  QString text = s.flags.replace ?
     made + "\n" + reached + "\n" + question :
     reached + "\n" + question;

  return KMessageBox::Yes == KMessageBox::questionYesNo(
     view(), text, s.flags.replace ? i18n("&Replace") : i18n("&Find"),
     i18n("&Continue"), i18n("&Stop") );
}

QString KateSearch::getSearchText()
{
  // SelectionOnly: use selection
  // WordOnly: use word under cursor
  // SelectionWord: use selection if available, else use word under cursor
  // WordSelection: use word if available, else use selection
  QString str;

  int getFrom = doc()->getSearchTextFrom();
  switch (getFrom)
  {
  case KateDocument::SelectionOnly: // (Windows)
    //kdDebug() << "getSearchText(): SelectionOnly" << endl;
    if( doc()->hasSelection() )
      str = doc()->selection();
    break;

  case KateDocument::SelectionWord: // (classic Kate behaviour)
    //kdDebug() << "getSearchText(): SelectionWord" << endl;
    if( doc()->hasSelection() )
      str = doc()->selection();
    else
      str = view()->currentWord();
    break;

  case KateDocument::WordOnly: // (weird?)
    //kdDebug() << "getSearchText(): WordOnly" << endl;
    str = view()->currentWord();
    break;

  case KateDocument::WordSelection: // (persistent selection lover)
    //kdDebug() << "getSearchText(): WordSelection" << endl;
    str = view()->currentWord();
    if (str.isEmpty() && doc()->hasSelection() )
      str = doc()->selection();
    break;

  default: // (nowhere)
    //kdDebug() << "getSearchText(): Nowhere" << endl;
    break;
  }

  str.replace( QRegExp("^\\n"), "" );
  str.replace( QRegExp("\\n.*"), "" );

  return str;
}

KateTextCursor KateSearch::getCursor()
{
  return KateTextCursor(view()->cursorLine(), view()->cursorColumnReal());
}

bool KateSearch::doSearch( const QString& text )
{
/*
  rodda: Still Working on this... :)

  bool result = false;

  if (m_searchResults.count()) {
    m_resultIndex++;
    if (m_resultIndex < (int)m_searchResults.count()) {
      s = m_searchResults[m_resultIndex];
      result = true;
    }

  } else {
    int temp = 0;
    do {*/

#if 0
  static int oldLine = -1;
  static int oldCol = -1;
#endif

  uint line = s.cursor.line();
  uint col = s.cursor.col();// + (result ? s.matchedLength : 0);
  bool backward = s.flags.backward;
  bool caseSensitive = s.flags.caseSensitive;
  bool regExp = s.flags.regExp;
  bool wholeWords = s.flags.wholeWords;
  uint foundLine, foundCol, matchLen;
  bool found = false;
  //kdDebug() << "Searching at " << line << ", " << col << endl;
  if( regExp ) {
    m_re = QRegExp( text, caseSensitive );
    found = doc()->searchText( line, col, m_re,
                              &foundLine, &foundCol,
                              &matchLen, backward );
  } else if ( wholeWords ) {
    QRegExp re( "\\b" + text + "\\b", caseSensitive );
    found = doc()->searchText( line, col, re,
                              &foundLine, &foundCol,
                              &matchLen, backward );
  } else {
    found = doc()->searchText( line, col, text,
                              &foundLine, &foundCol,
                              &matchLen, caseSensitive, backward );
  }
  if ( found && s.flags.selected )
  {
    if ( !s.flags.backward && KateTextCursor( foundLine, foundCol ) >= s.selEnd
      ||  s.flags.backward && KateTextCursor( foundLine, foundCol ) < s.selBegin )
      found = false;
  }
  if( !found ) return false; //break;

  //result = true;

  s.cursor.setPos(foundLine, foundCol);

  //kdDebug() << "Found at " << s.cursor.line() << ", " << s.cursor.col() << endl;
  s.matchedLength = matchLen;

  //m_searchResults.append(s);

  if (arbitraryHLExample)  {
    ArbitraryHighlightRange* hl = new ArbitraryHighlightRange(new KateSuperCursor(m_doc, true, s.cursor), new KateSuperCursor(m_doc, true, s.cursor.line(), s.cursor.col() + s.matchedLength), this);
    hl->setBold();
    hl->setTextColor(Qt::white);
    hl->setBGColor(Qt::black);
    // destroy the highlight upon change
    connect(hl, SIGNAL(contentsChanged()), hl, SIGNAL(eliminated()));
    m_arbitraryHLList->append(hl);
  }

  return true;

    /* rodda: more of my search highlighting work

    } while (++temp < 100);

    if (result) {
      s = m_searchResults.first();
      m_resultIndex = 0;
    }
  }

  return result;*/
}

void KateSearch::exposeFound( KateTextCursor &cursor, int slen )
{
  view()->setCursorPositionReal( cursor.line(), cursor.col() + slen );
  doc()->setSelection( cursor.line(), cursor.col(), cursor.line(), cursor.col() + slen );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
