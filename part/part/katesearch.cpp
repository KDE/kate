/* This file is part of the KDE libraries
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katesearch.h"
#include "katesearch.moc"

#include "kateview.h"
#include "katedocument.h"
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

#include <qlayout.h>
#include <qlabel.h>

//BEGIN KateSearch
QStringList KateSearch::s_searchList  = QStringList();
QStringList KateSearch::s_replaceList = QStringList();
static const bool arbitraryHLExample = false;

KateSearch::KateSearch( KateView* view )
  : QObject( view, "kate search" )
  , m_view( view )
  , m_doc( view->doc() )
  , replacePrompt( new KateReplacePrompt( view ) )
{
  m_arbitraryHLList = new KateSuperRangeList();
  if (arbitraryHLExample) m_doc->arbitraryHL()->addHighlightToView(m_arbitraryHLList, m_view);

  connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
}

KateSearch::~KateSearch()
{
  delete m_arbitraryHLList;
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
  // if multiline selection around, search in it
  long searchf = KateViewConfig::global()->searchFlags();
  if (m_doc->hasSelection() && m_doc->selStartLine() != m_doc->selEndLine())
    searchf |= KFindDialog::SelectedText;

  KFindDialog *findDialog = new KFindDialog (  m_view, "", searchf,
                                               s_searchList, m_doc->hasSelection() );

  findDialog->setPattern (getSearchText());


  if( findDialog->exec() == QDialog::Accepted ) {
    s_searchList =  findDialog->findHistory () ;
    find( s_searchList.first(), findDialog->options() );
  }

  delete findDialog;
  m_view->repaintText ();
}

void KateSearch::find( const QString &pattern, long flags )
{
  KateViewConfig::global()->setSearchFlags( flags );
  addToList( s_searchList, pattern );

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

  s.wrappedEnd = s.cursor;
  s.wrapped = false;

  search( searchFlags );
}

void KateSearch::replace()
{
  if (!doc()->isReadWrite()) return;

  // if multiline selection around, search in it
  long searchf = KateViewConfig::global()->searchFlags();
  if (m_doc->hasSelection() && m_doc->selStartLine() != m_doc->selEndLine())
    searchf |= KFindDialog::SelectedText;

  KReplaceDialog *replaceDialog = new KReplaceDialog (  m_view, "", searchf,
                                               s_searchList, s_replaceList, m_doc->hasSelection() );

  replaceDialog->setPattern (getSearchText());

  if( replaceDialog->exec() == QDialog::Accepted ) {
    long opts = replaceDialog->options();
    m_replacement = replaceDialog->replacement();
    s_searchList = replaceDialog->findHistory () ;
    s_replaceList = replaceDialog->replacementHistory () ;

    replace( s_searchList.first(), s_replaceList.first(), opts );
  }

  delete replaceDialog;
  m_view->update ();
}

void KateSearch::replace( const QString& pattern, const QString &replacement, long flags )
{
  if (!doc()->isReadWrite()) return;

  addToList( s_searchList, pattern );
  addToList( s_replaceList, replacement );
  m_replacement = replacement;
  KateViewConfig::global()->setSearchFlags( flags );

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
  searchFlags.useBackRefs = KateViewConfig::global()->searchFlags() & KReplaceDialog::BackReference;
  if ( searchFlags.selected )
  {
    s.selBegin = KateTextCursor( doc()->selStartLine(), doc()->selStartCol() );
    s.selEnd   = KateTextCursor( doc()->selEndLine(),   doc()->selEndCol()   );
    s.cursor   = s.flags.backward ? s.selEnd : s.selBegin;
  } else {
    s.cursor = getCursor();
  }

  s.wrappedEnd = s.cursor;
  s.wrapped = false;

  search( searchFlags );
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

  // oh, we wrapped around one time allready now !
  // only check that on replace
  s.wrapped = s.flags.replace;

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
    replacePrompt->setFocus ();
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
  if ( s.flags.regExp && s.flags.useBackRefs ) {
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

  // if we inserted newlines, we better adjust.
  uint newlines = replaceWith.contains('\n');
  if ( newlines )
  {
    if ( ! s.flags.backward )
    {
      s.cursor.setLine( s.cursor.line() + newlines );
      s.cursor.setCol( replaceWith.length() - replaceWith.findRev('\n') );
    }
    // selection?
    if ( s.flags.selected )
      s.selEnd.setLine( s.selEnd.line() + newlines );
  }


  // adjust selection endcursor if needed
  if( s.flags.selected && s.cursor.line() == s.selEnd.line() )
  {
    s.selEnd.setCol(s.selEnd.col() + replaceWith.length() - s.matchedLength );
  }

  // adjust wrap cursor if needed
  if( s.cursor.line() == s.wrappedEnd.line() && s.cursor.col() <= s.wrappedEnd.col())
  {
    s.wrappedEnd.setCol(s.wrappedEnd.col() + replaceWith.length() - s.matchedLength );
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
     view(), text, s.flags.replace ? i18n("Replace") : i18n("Find"),
     KStdGuiItem::cont(), i18n("&Stop") );
}

QString KateSearch::getSearchText()
{
  // SelectionOnly: use selection
  // WordOnly: use word under cursor
  // SelectionWord: use selection if available, else use word under cursor
  // WordSelection: use word if available, else use selection
  QString str;

  int getFrom = view()->config()->textToSearchMode();
  switch (getFrom)
  {
  case KateViewConfig::SelectionOnly: // (Windows)
    //kdDebug() << "getSearchText(): SelectionOnly" << endl;
    if( doc()->hasSelection() )
      str = doc()->selection();
    break;

  case KateViewConfig::SelectionWord: // (classic Kate behavior)
    //kdDebug() << "getSearchText(): SelectionWord" << endl;
    if( doc()->hasSelection() )
      str = doc()->selection();
    else
      str = view()->currentWord();
    break;

  case KateViewConfig::WordOnly: // (weird?)
    //kdDebug() << "getSearchText(): WordOnly" << endl;
    str = view()->currentWord();
    break;

  case KateViewConfig::WordSelection: // (persistent selection lover)
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
//   kdDebug()<<"KateSearch::doSearch: "<<line<<", "<<col<<", "<<backward<<endl;

  do {
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
      else if (m_doc->blockSelectionMode())
      {
        if ((int)foundCol < s.selEnd.col() && (int)foundCol >= s.selBegin.col())
          break;
      }
    }

    line = foundLine;
    col = foundCol+1;
  }
  while (m_doc->blockSelectionMode() && found);

  if( !found ) return false;

  // save the search result
  s.cursor.setPos(foundLine, foundCol);
  s.matchedLength = matchLen;

  // we allready wrapped around one time
  if (s.wrapped)
  {
    if (s.flags.backward)
    {
      if ( (s.cursor.line() < s.wrappedEnd.line())
           || ( (s.cursor.line() == s.wrappedEnd.line()) && ((s.cursor.col()+matchLen) <= uint(s.wrappedEnd.col())) ) )
        return false;
    }
    else
    {
      if ( (s.cursor.line() > s.wrappedEnd.line())
           || ( (s.cursor.line() == s.wrappedEnd.line()) && (s.cursor.col() > s.wrappedEnd.col()) ) )
        return false;
    }
  }

//   kdDebug() << "Found at " << s.cursor.line() << ", " << s.cursor.col() << endl;


  //m_searchResults.append(s);

  if (arbitraryHLExample)  {
    KateArbitraryHighlightRange* hl = new KateArbitraryHighlightRange(new KateSuperCursor(m_doc, true, s.cursor), new KateSuperCursor(m_doc, true, s.cursor.line(), s.cursor.col() + s.matchedLength), this);
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
  view()->setCursorPositionInternal ( cursor.line(), cursor.col() + slen, 1 );
  doc()->setSelection( cursor.line(), cursor.col(), cursor.line(), cursor.col() + slen );
}
//END KateSearch

//BEGIN KateReplacePrompt
// this dialog is not modal
KateReplacePrompt::KateReplacePrompt ( QWidget *parent )
  : KDialogBase ( parent, 0L, false, i18n( "Replace Confirmation" ),
                  User3 | User2 | User1 | Close | Ok , Ok, true,
                  i18n("Replace &All"), i18n("Re&place && Close"), i18n("&Replace") )
{
  setButtonOK( i18n("&Find Next") );
  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  QLabel *label = new QLabel(i18n("Found an occurrence of your search term. What do you want to do?"),page);
  topLayout->addWidget(label );
}

void KateReplacePrompt::slotOk ()
{ // Search Next
  done(KateSearch::srNo);
}

void KateReplacePrompt::slotClose ()
{ // Close
  done(KateSearch::srCancel);
}

void KateReplacePrompt::slotUser1 ()
{ // Replace All
  done(KateSearch::srAll);
}

void KateReplacePrompt::slotUser2 ()
{ // Replace & Close
  done(KateSearch::srLast);
}

void KateReplacePrompt::slotUser3 ()
{ // Replace
  done(KateSearch::srYes);
}

void KateReplacePrompt::done (int result)
{
  setResult(result);

  emit clicked();
}
//END KateReplacePrompt

//BEGIN SearchCommand
bool SearchCommand::exec(class Kate::View *view, const QString &cmd, QString &msg)
{
  QString flags, pattern, replacement;
  if ( cmd.startsWith( "find" ) )
  {

    QRegExp re("find(?::([bcersw]*))?\\s+(.+)");
    if ( re.search( cmd ) < 0 )
    {
      msg = i18n("Usage: find[:[bcersw]] PATTERN");
      return false;
    }
    flags = re.cap( 1 );
    pattern = re.cap( 2 );
  }

  else if ( cmd.startsWith( "replace" ) )
  {
    // Try if the pattern and replacement is quoted, using a quote character [^\w\s\\]
    QRegExp re("replace(?::([bceprsw]*))?\\s+([^\\w\\s\\\\])((?:[^\\\\\\\\2]|\\\\.)*)\\2\\s+\\2((?:[^\\\\\\\\2]|\\\\.)*)\\2\\s*$");
    // Or one quoted argument
    QRegExp re1("replace(?::([bceprsw]*))?\\s+([^\\w\\s\\\\])((?:[^\\\\\\\\2]|\\\\.)*)\\2\\s*$");
    // Else, it's just one or two (space separated) words
    QRegExp re2("replace(?::([bceprsw]*))?\\s+(\\S+)\\s*(.*)");
#define unbackslash(s) p=0;\
while ( (p = pattern.find( '\\' + delim, p )) > -1 )\
{\
  if ( !p || pattern[p-1] != '\\' )\
    pattern.remove( p, 1 );\
  p++;\
}

    if ( re.search( cmd ) >= 0 )
    {
      flags = re.cap(1);
      pattern = re.cap( 3 );
      replacement = re.cap( 4 );

      int p(0);
      // unbackslash backslashed delimiter strings
      // in pattern ..
      QString delim = re.cap( 2 );
      unbackslash(pattern);
      // .. and in replacement
      unbackslash(replacement);
    }
    else if ( re1.search( cmd ) >= 0 )
    {
      flags = re1.cap(1);
      pattern = re1.cap( 3 );

      int p(0);
      QString delim = re1.cap( 2 );
      unbackslash(pattern);
    }
    else if ( re2.search( cmd ) >= 0 )
    {
      flags = re2.cap( 1 );
      pattern = re2.cap( 2 );
      replacement = re2.cap( 3 );
    }
    else
    {
      msg = i18n("Usage: replace[:[bceprsw]] PATTERN [REPLACEMENT]");
      return false;
    }
    kdDebug()<<"replace '"<<pattern<<"' with '"<<replacement<<"'"<<endl;
#undef unbackslash(s)
  }

  long f = 0;
  if ( flags.contains( 'b' ) ) f |= KFindDialog::FindBackwards;
  if ( flags.contains( 'c' ) ) f |= KFindDialog::FromCursor;
  if ( flags.contains( 'e' ) ) f |= KFindDialog::SelectedText;
  if ( flags.contains( 'r' ) ) f |= KFindDialog::RegularExpression;
  if ( flags.contains( 'p' ) ) f |= KReplaceDialog::PromptOnReplace;
  if ( flags.contains( 's' ) ) f |= KFindDialog::CaseSensitive;
  if ( flags.contains( 'w' ) ) f |= KFindDialog::WholeWordsOnly;

  if ( cmd.startsWith( "find" ) )
  {
    ((KateView*)view)->find( pattern, f );
    return true;
  }
  else if ( cmd.startsWith( "replace" ) )
  {
    f |= KReplaceDialog::BackReference; // mandatory here?
    ((KateView*)view)->replace( pattern, replacement, f );
    return true;
  }

  return false;
}

bool SearchCommand::help(class Kate::View *, const QString &cmd, QString &msg)
{
  if ( cmd == "find" )
    msg = i18n("<p>Usage: <code>find[:bcersw] PATTERN</code>");
  else msg = i18n("<p>Usage: <code>replace[:bceprsw] PATTERN REPLACEMENT</code>");

  msg += i18n(
      "<h4><caption>Options</h4><p>"
      "<b>b</b> - Search backward"
      "<br><b>c</b> - Search from cursor"
      "<br><b>E</b> - Search in selected text only"
      "<br><b>r</b> - Pattern is a regular expression"
      "<br><b>s</b> - Case sensitive search"
      "<br><b>w</b> - Search whole words only"
             );

  if ( cmd == "replace" )
    msg += i18n(
        "<br><b>p</b> - Prompt for replace</p>"
        "<p>If REPLACEMENT is not present, an empty string is used.</p>"
        "<p>If you want to have whitespace in your PATTERN, you need to "
        "quote both PATTERN and REPLACEMENT with either single or double "
        "quotes. To have the quote characters in the strings, prepend them "
        "with a backslash.</p>");

  msg += "</p></ul>";
  return true;
}

QStringList SearchCommand::cmds()
{
  QStringList l;
  l << "find" << "replace";
  return l;
}
//END SearchCommand

// kate: space-indent on; indent-width 2; replace-tabs on;
