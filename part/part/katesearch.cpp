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

#include "katesearch.h"
#include "katesearch.moc"

#include "kateview.h"
#include "katedocument.h"
#include "kateconfig.h"

#include <klocale.h>
#include <kstdaction.h>
#include <kpushbutton.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kdebug.h>
#include <kfinddialog.h>
#include <kreplacedialog.h>
#include <kfind.h>
#include <qlayout.h>
#include <qlabel.h>

//BEGIN KateSearch
QStringList KateSearch::s_searchList  = QStringList();
QStringList KateSearch::s_replaceList = QStringList();
QString KateSearch::s_pattern = QString();

KateSearch::KateSearch( KateView* view )
  : QObject( view )
  , m_view( view )
  , m_doc( view->doc() )
  , replacePrompt( new KateReplacePrompt( view ) )
{
  setObjectName("kate search");
  connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
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
  list.removeAll (s);

  if( list.count() >= 16 )
    list.removeLast();

  list.prepend( s );
}

void KateSearch::find()
{
  // if multiline selection around, search in it
  long searchf = KateViewConfig::global()->searchFlags();
  if (m_view->selection() && !m_view->selectionRange().onSingleLine())
    searchf |= KFind::SelectedText;

  KFindDialog *findDialog = new KFindDialog (  m_view, "", searchf,
                                               s_searchList, m_view->selection() );

  findDialog->setPattern (getSearchText());


  if( findDialog->exec() == QDialog::Accepted ) {
    s_searchList =  findDialog->findHistory () ;
    // Do *not* remove the QString() wrapping, it fixes a nasty crash
    find( QString(s_searchList.first()), findDialog->options(), true, true );
  }

  delete findDialog;
  m_view->repaintText ();
}

void KateSearch::find( const QString &pattern, long flags, bool add, bool shownotfound )
{
  KateViewConfig::global()->setSearchFlags( flags );
  if( add )
    addToList( s_searchList, pattern );

   s_pattern = pattern;

  SearchFlags searchFlags;

  searchFlags.caseSensitive = KateViewConfig::global()->searchFlags() & KFind::CaseSensitive;
  searchFlags.wholeWords = KateViewConfig::global()->searchFlags() & KFind::WholeWordsOnly;
  searchFlags.fromBeginning = !(KateViewConfig::global()->searchFlags() & KFind::FromCursor)
      && !(KateViewConfig::global()->searchFlags() & KFind::SelectedText);
  searchFlags.backward = KateViewConfig::global()->searchFlags() & KFind::FindBackwards;
  searchFlags.selected = KateViewConfig::global()->searchFlags() & KFind::SelectedText;
  searchFlags.prompt = false;
  searchFlags.replace = false;
  searchFlags.finished = false;
  searchFlags.regExp = KateViewConfig::global()->searchFlags() & KFind::RegularExpression;
  searchFlags.useBackRefs = KateViewConfig::global()->searchFlags() & KReplaceDialog::BackReference;

  if ( searchFlags.selected )
  {
    s.selection = m_view->selectionRange();
    s.cursor   = s.flags.backward ? s.selection.end() : s.selection.start();
  } else {
    s.cursor = getCursor();
  }

  s.wrappedEnd = s.cursor;
  s.wrapped = false;
  s.showNotFound = shownotfound;

  search( searchFlags );
}

void KateSearch::replace()
{
  if (!doc()->isReadWrite()) return;

  // if multiline selection around, search in it
  long searchf = KateViewConfig::global()->searchFlags();
  if (m_view->selection() && !m_view->selectionRange().onSingleLine())
    searchf |= KFind::SelectedText;

  KReplaceDialog *replaceDialog = new KReplaceDialog (  m_view, "", searchf,
                                               s_searchList, s_replaceList, m_view->selection() );

  replaceDialog->setPattern (getSearchText());

  if( replaceDialog->exec() == QDialog::Accepted ) {
    long opts = replaceDialog->options();
    m_replacement = replaceDialog->replacement();
    s_searchList = replaceDialog->findHistory () ;
    s_replaceList = replaceDialog->replacementHistory () ;

    // Do *not* remove the QString() wrapping, it fixes a nasty crash
    replace( QString(s_searchList.first()), m_replacement, opts );
  }

  delete replaceDialog;
  m_view->update ();
}

void KateSearch::replace( const QString& pattern, const QString &replacement, long flags )
{
  if (!doc()->isReadWrite()) return;

  addToList( s_searchList, pattern );
   s_pattern = pattern;
  addToList( s_replaceList, replacement );
  m_replacement = replacement;
  KateViewConfig::global()->setSearchFlags( flags );

  SearchFlags searchFlags;
  searchFlags.caseSensitive = KateViewConfig::global()->searchFlags() & KFind::CaseSensitive;
  searchFlags.wholeWords = KateViewConfig::global()->searchFlags() & KFind::WholeWordsOnly;
  searchFlags.fromBeginning = !(KateViewConfig::global()->searchFlags() & KFind::FromCursor)
      && !(KateViewConfig::global()->searchFlags() & KFind::SelectedText);
  searchFlags.backward = KateViewConfig::global()->searchFlags() & KFind::FindBackwards;
  searchFlags.selected = KateViewConfig::global()->searchFlags() & KFind::SelectedText;
  searchFlags.prompt = KateViewConfig::global()->searchFlags() & KReplaceDialog::PromptOnReplace;
  searchFlags.replace = true;
  searchFlags.finished = false;
  searchFlags.regExp = KateViewConfig::global()->searchFlags() & KFind::RegularExpression;
  searchFlags.useBackRefs = KateViewConfig::global()->searchFlags() & KReplaceDialog::BackReference;
  if ( searchFlags.selected )
  {
    s.selection = m_view->selectionRange();
    s.cursor   = s.flags.backward ? s.selection.end() : s.selection.start();
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
  searchFlags.caseSensitive = KateViewConfig::global()->searchFlags() & KFind::CaseSensitive;
  searchFlags.wholeWords = KateViewConfig::global()->searchFlags() & KFind::WholeWordsOnly;
  searchFlags.fromBeginning = !(KateViewConfig::global()->searchFlags() & KFind::FromCursor)
                               && !(KateViewConfig::global()->searchFlags() & KFind::SelectedText);
  searchFlags.backward = KateViewConfig::global()->searchFlags() & KFind::FindBackwards;
  searchFlags.selected = KateViewConfig::global()->searchFlags() & KFind::SelectedText;
  searchFlags.prompt = KateViewConfig::global()->searchFlags() & KReplaceDialog::PromptOnReplace;
  searchFlags.replace = false;
  searchFlags.finished = false;
  searchFlags.regExp = KateViewConfig::global()->searchFlags() & KFind::RegularExpression;
  searchFlags.useBackRefs = KateViewConfig::global()->searchFlags() & KReplaceDialog::BackReference;

  searchFlags.backward = searchFlags.backward != back;
  searchFlags.fromBeginning = false;
  searchFlags.prompt = true; // ### why is the above assignment there?
  s.cursor = getCursor();

  search( searchFlags );
}

void KateSearch::search( SearchFlags flags )
{
  s.flags = flags;

  if( s.flags.fromBeginning ) {
    if( !s.flags.backward ) {
      s.cursor.setPosition(0, 0);
    } else {
      s.cursor.setLine(doc()->lines() - 1);
      s.cursor.setColumn(doc()->lineLength( s.cursor.line() ));
    }
  }

  if((!s.flags.backward &&
       s.cursor.column() == 0 &&
       s.cursor.line() == 0 ) ||
     ( s.flags.backward &&
       s.cursor.column() == doc()->lineLength( s.cursor.line() ) &&
       s.cursor.line() == (((int)doc()->lines()) - 1) ) ) {
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
    KTextEditor::Cursor start (s.selection.start());
    KTextEditor::Cursor end (s.selection.end());
      
    // recalc for block sel, to have start with lowest col, end with highest
    if (m_view->blockSelectionMode())
    {
      start.setColumn (QMIN(s.selection.start().column(), s.selection.end().column()));
      end.setColumn (QMAX(s.selection.start().column(), s.selection.end().column()));
    }
    
    s.cursor = s.flags.backward ? end : start;
  }
  else
  {
    if( !s.flags.backward ) {
      s.cursor.setPosition(0, 0);
    } else {
      s.cursor.setLine(doc()->lines() - 1);
      s.cursor.setColumn(doc()->lineLength( s.cursor.line() ) );
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
  if(  s_pattern.isEmpty() ) {
    find();
    return;
  }

  if ( doSearch(  s_pattern ) ) {
    exposeFound( s.cursor, s.matchedLength );
  } else if( !s.flags.finished ) {
    if( askContinue() ) {
      wrapSearch();
      findAgain();
    }
  } else {
    if ( s.showNotFound )
      KMessageBox::sorry( view(),
        i18n("Search string '%1' not found!")
             .arg( KStringHandler::csqueeze(  s_pattern ) ),
        i18n("Find"));
  }
}

void KateSearch::replaceAll()
{
  doc()->editStart ();

  while( doSearch(  s_pattern ) )
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
  if ( doSearch(  s_pattern ) ) {
    exposeFound( s.cursor, s.matchedLength );
    replacePrompt->show();
    replacePrompt->setFocus ();
  } else if( !s.flags.finished && askContinue() ) {
    wrapSearch();
    promptReplace();
  } else {
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
    int pos = br.indexIn( replaceWith );
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
      pos = br.indexIn( replaceWith, pos + qMax(br.matchedLength(), (int)sc.length()) );
    }
  }

  doc()->editStart();

  doc()->removeText( KTextEditor::Range(s.cursor, s.cursor + KTextEditor::Cursor(0,s.matchedLength)) );
  doc()->insertText( s.cursor, replaceWith );
  doc()->editEnd(),

  replaces++;

  // if we inserted newlines, we better adjust.
  int newlines = replaceWith.count(QChar::fromLatin1('\n'));
  if ( newlines > 0 )
  {
    if ( ! s.flags.backward )
    {
      s.cursor.setLine( s.cursor.line() + newlines );
      s.cursor.setColumn( replaceWith.length() - replaceWith.lastIndexOf('\n') );
    }
    // selection?
    if ( s.flags.selected )
      s.selection.end().setLine( s.selection.end().line() + newlines );
  }


  // adjust selection endcursor if needed
  if( s.flags.selected && s.cursor.line() == s.selection.end().line() )
  {
    s.selection.end().setColumn(s.selection.end().column() + replaceWith.length() - s.matchedLength );
  }

  // adjust wrap cursor if needed
  if( s.cursor.line() == s.wrappedEnd.line() && s.cursor.column() <= s.wrappedEnd.column())
  {
    s.wrappedEnd.setColumn(s.wrappedEnd.column() + replaceWith.length() - s.matchedLength );
  }

  if( !s.flags.backward ) {
    s.cursor.setColumn(s.cursor.column() + replaceWith.length());
  } else if( s.cursor.column() > 0 ) {
    s.cursor.setColumn(s.cursor.column() - 1);
  } else {
    s.cursor.setLine(s.cursor.line() - 1);
    if( s.cursor.line() >= 0 ) {
      s.cursor.setColumn(doc()->lineLength( s.cursor.line() ));
    }
  }
}

void KateSearch::skipOne()
{
  if( !s.flags.backward ) {
    s.cursor.setColumn(s.cursor.column() + s.matchedLength);
  } else if( s.cursor.column() > 0 ) {
    s.cursor.setColumn(s.cursor.column() - 1);
  } else {
    s.cursor.setLine(s.cursor.line() - 1);
    if( s.cursor.line() >= 0 ) {
      s.cursor.setColumn(doc()->lineLength(s.cursor.line()));
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

  if (KateViewConfig::global()->searchFlags() & KFind::SelectedText)
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
    if( m_view->selection() )
      str = m_view->selectionText();
    break;

  case KateViewConfig::SelectionWord: // (classic Kate behavior)
    //kdDebug() << "getSearchText(): SelectionWord" << endl;
    if( m_view->selection() )
      str = m_view->selectionText();
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
    if (str.isEmpty() && m_view->selection() )
      str = m_view->selectionText();
    break;

  default: // (nowhere)
    //kdDebug() << "getSearchText(): Nowhere" << endl;
    break;
  }

  str.replace( QRegExp("^\\n"), "" );
  str.replace( QRegExp("\\n.*"), "" );

  return str;
}

KTextEditor::Cursor KateSearch::getCursor()
{
  return view()->cursorPosition();
}

bool KateSearch::doSearch( const QString& text )
{
  KTextEditor::Cursor startPos = s.cursor;
  bool backward = s.flags.backward;
  bool caseSensitive = s.flags.caseSensitive;
  bool regExp = s.flags.regExp;
  bool wholeWords = s.flags.wholeWords;
  KTextEditor::Range match;
  //kdDebug() << "Searching at " << line << ", " << col << endl;

  do {
      if( regExp ) {
        m_re = QRegExp( text, caseSensitive ? Qt::CaseSensitive: Qt::CaseInsensitive);
        match = doc()->searchText( startPos, m_re, backward );
      } else if ( wholeWords ) {
        QRegExp re( "\\b" + text + "\\b", caseSensitive ? Qt::CaseSensitive: Qt::CaseInsensitive);
        match = doc()->searchText( startPos, re, backward );
      } else {
        match = doc()->searchText( startPos, text, caseSensitive, backward );
      }

    if ( match.isValid() && s.flags.selected )
    {
      KTextEditor::Cursor start (s.selection.start());
      KTextEditor::Cursor end (s.selection.end());
        
      // recalc for block sel, to have start with lowest col, end with highest
      if (m_view->blockSelectionMode())
      {
        start.setColumn (QMIN(s.selection.start().column(), s.selection.end().column()));
        end.setColumn (QMAX(s.selection.start().column(), s.selection.end().column()));
      }
    
      if ( !s.flags.backward && match.start() >= end
        ||  s.flags.backward && match.start() < start )
        match = KTextEditor::Range::invalid();
      else if (m_view->blockSelection())
      {
        if (match.start().column() >= start.column() && match.start().column() < end.column())
          break;
      }
    }

    startPos = match.start() + KTextEditor::Cursor(0,1);
  }
  while (s.flags.selected && m_view->blockSelectionMode() && match.isValid());
  // in the case we want to search in selection + blockselection we need to loop
  
  if( !match.isValid() ) return false;

  // save the search result
  s.cursor.setPosition(match.start());
  s.matchedLength = match.end().column() - match.start().column();

  // we allready wrapped around one time
  if (s.wrapped)
  {
    if (s.flags.backward)
    {
      if ( (s.cursor.line() < s.wrappedEnd.line())
           || ( (s.cursor.line() == s.wrappedEnd.line()) && ((s.cursor.column()+s.matchedLength) <= s.wrappedEnd.column()) ) )
        return false;
    }
    else
    {
      if ( (s.cursor.line() > s.wrappedEnd.line())
           || ( (s.cursor.line() == s.wrappedEnd.line()) && (s.cursor.column() > s.wrappedEnd.column()) ) )
        return false;
    }
  }

//   kdDebug() << "Found at " << s.cursor.line() << ", " << s.cursor.column() << endl;

  return true;
}

void KateSearch::exposeFound( KTextEditor::Cursor &cursor, int slen )
{
  view()->setCursorPositionInternal ( KTextEditor::Cursor(cursor.line(), cursor.column() + slen), 1 );
  view()->setSelection( cursor, slen );
}
//END KateSearch

//BEGIN KateReplacePrompt
// this dialog is not modal
KateReplacePrompt::KateReplacePrompt ( QWidget *parent )
  : KDialogBase ( parent, 0L, false, i18n( "Replace Confirmation" ),
                  User3 | User2 | User1 | Close | Ok , Ok, true,
                  i18n("Replace &All"), i18n("Re&place && Close"), i18n("&Replace") )
{
  setButtonGuiItem( KDialogBase::Ok, i18n("&Find Next") );
  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QBoxLayout *topLayout = new QVBoxLayout( page);
  topLayout->setMargin(0);
  topLayout->setSpacing(spacingHint() );
  QLabel *label = new QLabel(i18n("Found an occurrence of your search term. What do you want to do?"),page);
  topLayout->addWidget(label );
}

void KateReplacePrompt::slotOk ()
{ // Search Next
  done(KateSearch::srNo);
  actionButton(Ok)->setFocus();
}

void KateReplacePrompt::slotClose ()
{ // Close
  done(KateSearch::srCancel);
  actionButton(Close)->setFocus();
}

void KateReplacePrompt::slotUser1 ()
{ // Replace All
  done(KateSearch::srAll);
  actionButton(User1)->setFocus();
}

void KateReplacePrompt::slotUser2 ()
{ // Replace & Close
  done(KateSearch::srLast);
  actionButton(User2)->setFocus();
}

void KateReplacePrompt::slotUser3 ()
{ // Replace
  done(KateSearch::srYes);
  actionButton(User3)->setFocus();
}

void KateReplacePrompt::done (int result)
{
  setResult(result);

  emit clicked();
}
//END KateReplacePrompt

//BEGIN SearchCommand
bool SearchCommand::exec(class KTextEditor::View *view, const QString &cmd, QString &msg)
{
  QString flags, pattern, replacement;
  if ( cmd.startsWith( "find" ) )
  {

    static QRegExp re_find("find(?::([bcersw]*))?\\s+(.+)");
    if ( re_find.indexIn( cmd ) < 0 )
    {
      msg = i18n("Usage: find[:[bcersw]] PATTERN");
      return false;
    }
    flags = re_find.cap( 1 );
    pattern = re_find.cap( 2 );
  }

  else if ( cmd.startsWith( "ifind" ) )
  {
    static QRegExp re_ifind("ifind(?::([bcrs]*))?\\s+(.*)");
    if ( re_ifind.indexIn( cmd ) < 0 )
    {
      msg = i18n("Usage: ifind[:[bcrs]] PATTERN");
      return false;
    }
    ifindClear();
    return true;
  }

  else if ( cmd.startsWith( "replace" ) )
  {
    // Try if the pattern and replacement is quoted, using a quote character ["']
    static QRegExp re_rep("replace(?::([bceprsw]*))?\\s+([\"'])((?:[^\\\\\\\\2]|\\\\.)*)\\2\\s+\\2((?:[^\\\\\\\\2]|\\\\.)*)\\2\\s*$");
    // Or one quoted argument
    QRegExp re_rep1("replace(?::([bceprsw]*))?\\s+([\"'])((?:[^\\\\\\\\2]|\\\\.)*)\\2\\s*$");
    // Else, it's just one or two (space separated) words
    QRegExp re_rep2("replace(?::([bceprsw]*))?\\s+(\\S+)(.*)");
#define unbackslash(s) p=0;\
while ( (p = pattern.indexOf( '\\' + delim, p )) > -1 )\
{\
  if ( !p || pattern[p-1] != '\\' )\
    pattern.remove( p, 1 );\
  p++;\
}

    if ( re_rep.indexIn( cmd ) >= 0 )
    {
      flags = re_rep.cap(1);
      pattern = re_rep.cap( 3 );
      replacement = re_rep.cap( 4 );

      int p(0);
      // unbackslash backslashed delimiter strings
      // in pattern ..
      QString delim = re_rep.cap( 2 );
      unbackslash(pattern);
      // .. and in replacement
      unbackslash(replacement);
    }
    else if ( re_rep1.indexIn( cmd ) >= 0 )
    {
      flags = re_rep1.cap(1);
      pattern = re_rep1.cap( 3 );

      int p(0);
      QString delim = re_rep1.cap( 2 );
      unbackslash(pattern);
    }
    else if ( re_rep2.indexIn( cmd ) >= 0 )
    {
      flags = re_rep2.cap( 1 );
      pattern = re_rep2.cap( 2 );
      replacement = re_rep2.cap( 3 ).trimmed();
    }
    else
    {
      msg = i18n("Usage: replace[:[bceprsw]] PATTERN [REPLACEMENT]");
      return false;
    }
    kdDebug()<<"replace '"<<pattern<<"' with '"<<replacement<<"'"<<endl;
#undef unbackslash
  }

  long f = 0;
  if ( flags.contains( 'b' ) ) f |= KFind::FindBackwards;
  if ( flags.contains( 'c' ) ) f |= KFind::FromCursor;
  if ( flags.contains( 'e' ) ) f |= KFind::SelectedText;
  if ( flags.contains( 'r' ) ) f |= KFind::RegularExpression;
  if ( flags.contains( 'p' ) ) f |= KReplaceDialog::PromptOnReplace;
  if ( flags.contains( 's' ) ) f |= KFind::CaseSensitive;
  if ( flags.contains( 'w' ) ) f |= KFind::WholeWordsOnly;

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

bool SearchCommand::help(class KTextEditor::View *, const QString &cmd, QString &msg)
{
  if ( cmd == "find" )
    msg = i18n("<p>Usage: <code>find[:bcersw] PATTERN</code></p>");

  else if ( cmd == "ifind" )
    msg = i18n("<p>Usage: <code>ifind:[:bcrs] PATTERN</code>"
        "<br>ifind does incremental or 'as-you-type' search</p>");

  else
    msg = i18n("<p>Usage: <code>replace[:bceprsw] PATTERN REPLACEMENT</code></p>");

  msg += i18n(
      "<h4><caption>Options</h4><p>"
      "<b>b</b> - Search backward"
      "<br><b>c</b> - Search from cursor"
      "<br><b>r</b> - Pattern is a regular expression"
      "<br><b>s</b> - Case sensitive search"
             );

  if ( cmd == "find" )
    msg += i18n(
        "<br><b>e</b> - Search in selected text only"
        "<br><b>w</b> - Search whole words only"
               );

  if ( cmd == "replace" )
    msg += i18n(
        "<br><b>p</b> - Prompt for replace</p>"
        "<p>If REPLACEMENT is not present, an empty string is used.</p>"
        "<p>If you want to have whitespace in your PATTERN, you need to "
        "quote both PATTERN and REPLACEMENT with either single or double "
        "quotes. To have the quote characters in the strings, prepend them "
        "with a backslash.");

  msg += "</p>";
  return true;
}

const QStringList &SearchCommand::cmds()
{
  static QStringList l;

  if (l.isEmpty())
    l << "find" << "replace" << "ifind";

  return l;
}

bool SearchCommand::wantsToProcessText( const QString &cmdname )
{
  return  cmdname == "ifind";
}

void SearchCommand::processText( KTextEditor::View *view, const QString &cmd )
{
  static QRegExp re_ifind("ifind(?::([bcrs]*))?\\s(.*)");
  if ( re_ifind.indexIn( cmd ) > -1 )
  {
    QString flags = re_ifind.cap( 1 );
    QString pattern = re_ifind.cap( 2 );


    // if there is no setup, or the text length is 0, set up the properties
    if ( ! m_ifindFlags || pattern.isEmpty() )
      ifindInit( flags );
    // if there is no fromCursor, add it if this is not the first character
    else if ( ! ( m_ifindFlags & KFind::FromCursor ) && ! pattern.isEmpty() )
      m_ifindFlags |= KFind::FromCursor;

    // search..
    if ( ! pattern.isEmpty() )
    {
      KateView *v = (KateView*)view;

      // If it *looks like* we are continuing, place the cursor
      // at the beginning of the selection, so that the search continues.
      // ### check more carefully, like is  the cursor currently at the end
      // of the selection.
      if ( pattern.startsWith( v->selectionText() ) &&
           v->selectionText().length() + 1 == pattern.length() )
        v->setCursorPositionInternal( v->selectionRange().start() );

      v->find( pattern, m_ifindFlags, false );
    }
  }
}

void SearchCommand::ifindInit( const QString &flags )
{
  long f = 0;
  if ( flags.contains( 'b' ) ) f |= KFind::FindBackwards;
  if ( flags.contains( 'c' ) ) f |= KFind::FromCursor;
  if ( flags.contains( 'r' ) ) f |= KFind::RegularExpression;
  if ( flags.contains( 's' ) ) f |= KFind::CaseSensitive;
  m_ifindFlags = f;
}

void SearchCommand::ifindClear()
{
  m_ifindFlags = 0;
}
//END SearchCommand

// kate: space-indent on; indent-width 2; replace-tabs on;
