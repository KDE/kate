/* This file is part of the KDE libraries
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

#include <klocale.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kdebug.h>
#include <kfinddialog.h>
#include <kreplacedialog.h>

#include "kateview.h"
#include "katedocument.h"
#include "kateviewdialog.h"

QStringList KateSearch::s_searchList  = QStringList();
QStringList KateSearch::s_replaceList = QStringList();
SearchFlags KateSearch::s_searchFlags = SearchFlags ();

KateSearch::KateSearch( KateView* view )
  : QObject( view, "kate search" )
  , m_view( view )
  , m_doc( view->doc() )
  , options (0)
  , replacePrompt( new ReplacePrompt( view ) )
{
  connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
  s_searchFlags.prompt = true;
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
  KFindDialog *findDialog = new KFindDialog (  m_view, "", options,
                                               s_searchList, m_doc->hasSelection() ); 

    findDialog->setPattern (getSearchText());

  if( findDialog->exec() == QDialog::Accepted ) {
    s_searchList =  findDialog->findHistory () ;
          options = findDialog->options ();

    
  s_searchFlags.caseSensitive = options & KFindDialog::CaseSensitive;
  s_searchFlags.wholeWords = options & KFindDialog::WholeWordsOnly;
    s_searchFlags.fromBeginning = !(options & KFindDialog::FromCursor)
                               && !(options & KFindDialog::SelectedText);
  s_searchFlags.backward = options & KFindDialog::FindBackwards;
  s_searchFlags.selected = options & KFindDialog::SelectedText;
  s_searchFlags.prompt = false;
  s_searchFlags.replace = false;
  s_searchFlags.finished = false;
  s_searchFlags.regExp = options & KFindDialog::RegularExpression;
    if ( s_searchFlags.selected )
    {
      s.selBegin = KateTextCursor( doc()->selStartLine(), doc()->selStartCol() );
      s.selEnd   = KateTextCursor( doc()->selEndLine(),   doc()->selEndCol()   );
      s.cursor   = s.flags.backward ? s.selEnd : s.selBegin;
    } else {
      s.cursor = getCursor();
    }
    
    search( s_searchFlags );
  }
  delete findDialog;
}

void KateSearch::replace()
{
  if (!doc()->isReadWrite()) return;

  KReplaceDialog *replaceDialog = new KReplaceDialog (  m_view, "", options,
                                               s_searchList, s_replaceList, m_doc->hasSelection() ); 

  replaceDialog->setPattern (getSearchText());

  if( replaceDialog->exec() == QDialog::Accepted ) {
    m_replacement = replaceDialog->replacement();
    s_searchList =  replaceDialog->findHistory () ;
    s_replaceList =  replaceDialog->replacementHistory () ;
          options = replaceDialog->options ();

  s_searchFlags.caseSensitive = options & KFindDialog::CaseSensitive;
  s_searchFlags.wholeWords = options & KFindDialog::WholeWordsOnly;
    s_searchFlags.fromBeginning = !(options & KFindDialog::FromCursor)
                               && !(options & KFindDialog::SelectedText);
  s_searchFlags.backward = options & KFindDialog::FindBackwards;
  s_searchFlags.selected = options & KFindDialog::SelectedText;
  s_searchFlags.prompt = options & KReplaceDialog::PromptOnReplace;
  s_searchFlags.replace = true;
  s_searchFlags.finished = false;
  s_searchFlags.regExp = options & KFindDialog::RegularExpression;
    if ( s_searchFlags.selected )
    {
      s.selBegin = KateTextCursor( doc()->selStartLine(), doc()->selStartCol() );
      s.selEnd   = KateTextCursor( doc()->selEndLine(),   doc()->selEndCol()   );
      s.cursor   = s.flags.backward ? s.selEnd : s.selBegin;
    } else {
      s.cursor = getCursor();
    }
    
    search( s_searchFlags );
  }
  delete replaceDialog;
}

void KateSearch::findAgain( bool back )
{
  SearchFlags flags = s_searchFlags;
  flags.backward = s_searchFlags.backward != back;
  flags.fromBeginning = false;
  flags.prompt = true;
  s.cursor = getCursor();
  
  search( flags );
}

void KateSearch::search( SearchFlags flags )
{
  s.flags = flags;
  
  if( s.flags.fromBeginning ) {
    if( !s.flags.backward ) {
      s.cursor.col = 0;
      s.cursor.line = 0;
    } else {
      s.cursor.line = doc()->numLines() - 1;
      s.cursor.col = doc()->lineLength( s.cursor.line );
    }
  } else if( s.flags.backward ) {
    // If we are continuing a backward search, make sure
    // we do not get stuck at an existing match.
    QString txt = view()->currentTextLine();
    QString searchFor = s_searchList.first();
    uint length = searchFor.length();
    int pos = s.cursor.col - length;
    kdDebug(13000) << pos << ", " << length << ": " << txt.mid( pos, length ) << endl;
    if( searchFor.find( txt.mid( pos, length ), 0, s.flags.caseSensitive ) == 0 ) {
      if( pos > 0 ) {
        s.cursor.col = pos - 1;
      } else if ( pos == 0 && s.cursor.line > 0 ) {
        s.cursor.line--;
        s.cursor.col = doc()->lineLength( s.cursor.line );
      } else if ( pos == 0 && s.cursor.line == 0 ) {
        // TODO: FIXME
      }
    }
  }
  if((!s.flags.backward && 
       s.cursor.col == 0 && 
       s.cursor.line == 0 ) ||
     ( s.flags.backward && 
       s.cursor.col == doc()->lineLength( s.cursor.line ) && 
       s.cursor.line == (((int)doc()->numLines()) - 1) ) ) {
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
    s.cursor.col = 0;
    s.cursor.line = 0;
  } else {
    s.cursor.line = doc()->numLines() - 1;
    s.cursor.col = doc()->lineLength( s.cursor.line );
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
    }
  } else {
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
        i18n("%n replacement made","%n replacements made",replaces),
        i18n("Replace") );
  }
}

void KateSearch::promptReplace()
{
  QString searchFor = s_searchList.first();
  if ( doSearch( searchFor ) ) {
    exposeFound( s.cursor, s.matchedLength );
    replacePrompt->show();
  } else if( !s.flags.finished ) {
    if( askContinue() ) {
      wrapSearch();
      promptReplace();
    }
  } else {
    replacePrompt->hide();
    KMessageBox::information( view(),
        i18n("%n replacement made","%n replacements made",replaces),
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
      pos = br.search( replaceWith, pos+QMAX(br.matchedLength(), sc.length()) );
    }
  }
  
  doc()->removeText( s.cursor.line, s.cursor.col,
      s.cursor.line, s.cursor.col + s.matchedLength );
  doc()->insertText( s.cursor.line, s.cursor.col, replaceWith );
  
  replaces++;
  
  if( s.flags.selected && s.cursor.line == s.selEnd.line )
  {
    s.selEnd.col += ( replaceWith.length() - s.matchedLength );
  }
  
  if( !s.flags.backward ) {
    s.cursor.col += replaceWith.length();
  } else if( s.cursor.col > 0 ) {
    s.cursor.col--;
  } else {
    s.cursor.line--;
    if( s.cursor.line >= 0 ) {
      s.cursor.col = doc()->lineLength( s.cursor.line );
    }
  }
}

void KateSearch::skipOne()
{
  if( !s.flags.backward ) {
    s.cursor.col += s.matchedLength;
  } else if( s.cursor.col > 0 ) {
    s.cursor.col--;
  } else {
    s.cursor.line--;
    if( s.cursor.line >= 0 ) {
      s.cursor.col = doc()->lineLength(s.cursor.line);
    }
  }
}

void KateSearch::replaceSlot() {
  switch( (Dialog_results)replacePrompt->result() ) {
  case srCancel: replacePrompt->hide();                break;
  case srAll:    replacePrompt->hide(); replaceAll();  break;
  case srYes:    replaceOne(); promptReplace();        break;
  case srNo:     skipOne();    promptReplace();        break;
  }
}

bool KateSearch::askContinue()
{
  QString made =
     i18n( "%n replacement made",
           "%n replacements made",
           replaces );
  QString reached = !s.flags.backward ?
     i18n( "End of document reached." ) :
     i18n( "Beginning of document reached." );
  QString question = !s.flags.backward ?
     i18n( "Continue from the beginning?" ) :
     i18n( "Continue from the end?" );
  QString text = s.flags.replace ?
     made + "\n" + reached + "\n" + question :
     reached + "\n" + question;
  return KMessageBox::Yes == KMessageBox::questionYesNo(
     view(), text, s.flags.replace ? i18n("Replace") : i18n("Find"),
     i18n("Continue"), i18n("Stop") );
}

QString KateSearch::getSearchText()
{
  // If the user has marked some text we use that otherwise
  // use the word under the cursor.
  QString str;
  
  if( doc()->hasSelection() )
    str = doc()->selection();
  else
    str = view()->currentWord();
  
  str.replace( QRegExp("^\\n"), "" );
  str.replace( QRegExp("\\n.*"), "" );
  
  return str;
}

KateTextCursor KateSearch::getCursor()
{
  KateTextCursor c;
  c.line = view()->cursorLine();
  c.col = view()->cursorColumnReal();
  return c;
}

bool KateSearch::doSearch( const QString& text )
{
  uint line = s.cursor.line;
  uint col = s.cursor.col;
  bool backward = s.flags.backward;
  bool caseSensitive = s.flags.caseSensitive;
  bool regExp = s.flags.regExp;
  bool wholeWords = s.flags.wholeWords;
  uint foundLine, foundCol, matchLen;
  bool found = false;
//  kdDebug(13000) << "Searching at " << line << ", " << col << endl;
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
  if( !found ) return false;
//  kdDebug(13000) << "Found at " << foundLine << ", " << foundCol << endl;
  s.cursor.line = foundLine;
  s.cursor.col = foundCol;
  s.matchedLength = matchLen;
  return true;
}

void KateSearch::exposeFound( KateTextCursor &cursor, int slen )
{
  view()->setCursorPositionReal( cursor.line, cursor.col + slen );
  doc()->setSelection( cursor.line, cursor.col, cursor.line, cursor.col + slen );
}

// vim: noet ts=2
