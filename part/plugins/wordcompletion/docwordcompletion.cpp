/*
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    ---
    file: docwordcompletion.cpp

    KTextEditor plugin to autocompletion with document words.
    Copyright Anders Lund <anders.lund@lund.tdcadsl.dk>, 2003

    The following completion methods are supported:
    * Completion with bigger matching words in
      either direction (backward/forward).
    * NOT YET Pop up a list of all bigger matching words in document

*/
//BEGIN includes
#include "docwordcompletion.h"

#include <ktexteditor/document.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/editinterface.h>

#include <kgenericfactory.h>
#include <klocale.h>
#include <kaction.h>
#include <knotifyclient.h>
#include <kparts/part.h>

#include <qregexp.h>
#include <qstring.h>
#include <qdict.h>

#include <kdebug.h>
//END

//BEGIN DocWordCompletionPlugin
K_EXPORT_COMPONENT_FACTORY( ktexteditor_docwordcompletion, KGenericFactory<DocWordCompletionPlugin>( "ktexteditor_docwordcompletion" ) );
DocWordCompletionPlugin::DocWordCompletionPlugin( QObject *parent,
                            const char* name,
                            const QStringList& /*args*/ )
	: KTextEditor::Plugin ( (KTextEditor::Document*) parent, name )
{
}

void DocWordCompletionPlugin::addView(KTextEditor::View *view)
{
  DocWordCompletionPluginView *nview = new DocWordCompletionPluginView (view, "Document word completion");
  m_views.append (nview);
}

void DocWordCompletionPlugin::removeView(KTextEditor::View *view)
{
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->parentClient() == view)
    {
       DocWordCompletionPluginView *nview = m_views.at(z);
       m_views.remove (nview);
       delete nview;
    }
}

//END

//BEGIN DocWordCompletionPluginView
struct DocWordCompletionPluginViewPrivate
{
  uint line, col;       // start position of last match (where to search from)
  uint cline, ccol;     // cursor position
  uint lilen;           // length of last insertion
  QString last;         // last word we were trying to match
  QString lastIns;      // latest applied completion
  QRegExp re;           // hrm
  KToggleAction *autopopup; // for accessing state
};

DocWordCompletionPluginView::DocWordCompletionPluginView( KTextEditor::View *view, const char *name )
  : QObject( view, name ),
    KXMLGUIClient( view ),
    m_view( view ),
    d( new DocWordCompletionPluginViewPrivate )
{
  view->insertChildClient( this );
  setInstance( KGenericFactory<DocWordCompletionPlugin>::instance() );

  (void) new KAction( i18n("Reuse word behind"), CTRL+Key_H, this,
    SLOT(completeBackwards()), actionCollection(), "doccomplete_bw" );
  (void) new KAction( i18n("Reuse word ahead"), CTRL+Key_J, this,
    SLOT(completeForwards()), actionCollection(), "doccomplete_fw" );
  (void) new KAction( i18n("Pop up completion list"), CTRL+Key_Y, this,
    SLOT(popupCompletionList()), actionCollection(), "doccomplete_pu" );
  d->autopopup = new KToggleAction( i18n("Automatic completion popup"), 0, this,
    SLOT(toggleAutoPopup()), actionCollection(), "enable_autopopup" );

  //d->autopopup->activate(); // default to ON for now

  setXMLFile("docwordcompletionui.rc");
}

void DocWordCompletionPluginView::completeBackwards()
{
  complete( false );
}

void DocWordCompletionPluginView::completeForwards()
{
  complete();
}

// Pop up the editors completion list if applicable
void DocWordCompletionPluginView::popupCompletionList( QString w )
{
  if ( w.isEmpty() )
    w = word();
  if ( w.isEmpty() )
    return;

  KTextEditor::CodeCompletionInterface *cci = codeCompletionInterface( m_view );
  cci->showCompletionBox( allMatches( w ), w.length() );
}

void DocWordCompletionPluginView::toggleAutoPopup()
{
  if ( d->autopopup->isChecked() )
    connect( m_view->document(), SIGNAL(textChanged()), this, SLOT(autoPopupCompletionList()) );
  else
    disconnect( m_view->document(), SIGNAL(textChanged()), this, SLOT(autoPopupCompletionList()) );
}

// for autopopup FIXME - don't pop up if reuse word is inserting
void DocWordCompletionPluginView::autoPopupCompletionList()
{
  QString w = word();
  if ( w.length() == 3 )
  {
      popupCompletionList( w );
  }
}

// Do one completion, searching in the desired direction,
// if possible
void DocWordCompletionPluginView::complete( bool fw )
{
  // setup
  KTextEditor::EditInterface *ei = KTextEditor::editInterface( m_view->document() );
  // find the word we are typing
  uint cline, ccol;
  viewCursorInterface( m_view )->cursorPosition( &cline, &ccol );
  QString wrd = word();
  if ( wrd.isEmpty() ) return;

  /* IF the current line is equal to the previous line
     AND the position - the length of the last inserted string
          is equal to the old position
     AND the lastinsertedlength last characters of the word is
          equal to the last inserted string
          */
  if ( cline == d-> cline &&
          ccol - d->lilen == d->ccol &&
          wrd.endsWith( d->lastIns ) )
  {
    // this is a repeted activation
    ccol = d->ccol;
    wrd = d->last;
  }
  else
  {
    d->cline = cline;
    d->ccol = ccol;
    d->last = wrd;
    d->lastIns = QString::null;
    d->line = d->cline;
    d->col = d->ccol - wrd.length();
    d->lilen = 0;
  }

  d->re.setPattern( "\\b" + wrd + "(\\w+)" );
  int inc = fw ? 1 : -1;
  int pos ( 0 );
  QString ln = ei->textLine( d->line );

  if ( ! fw )
    ln = ln.mid( 0, d->col );

  while ( true )
  {
    pos = fw ?
      d->re.search( ln, d->col ) :
      d->re.searchRev( ln, d->col );

    if ( pos > -1 ) // we matched a word
    {
      QString m = d->re.cap( 1 );
      if ( m != d->lastIns )
      {
        // we got good a match! replace text and return.
        if ( d->lilen )
          ei->removeText( d->cline, d->ccol, d->cline, d->ccol + d->lilen );
        ei->insertText( d->cline, d->ccol, m );

        d->lastIns = m;
        d->lilen = m.length();
        d->col = pos; // for next try

        if ( fw )
          d->col += m.length();

        return;
      }

      // equal to last one, continue
      else
      {
        d->col = pos; // for next try
        if ( fw )
          d->col += m.length();
        else // FIXME figure out if all of that is really nessecary
        {
          if ( pos == 0 )
          {
            if ( d->line > 0 )
            {
              d->line += inc;
              ln = ei->textLine( d->line );
              d->col = ln.length();
            }
            else
            {
              KNotifyClient::beep();
              return;
            }
          }
          else
            d->col--;
        }
      }
    }

    else  // no match
    {
      if ( ! fw && d->line == 0)
      {
        KNotifyClient::beep();
        return;
      }
      else if ( fw && d->line >= ei->numLines() )
      {
        KNotifyClient::beep();
        return;
      }

      d->line += inc;
      if ( fw )
        d->col++;

      ln = ei->textLine( d->line );
      d->col = fw ? 0 : ln.length();
    }
  } // while true
}

// Return the string to complete (the letters behind the cursor)
QString DocWordCompletionPluginView::word()
{
  uint cline, ccol;
  viewCursorInterface( m_view )->cursorPosition( &cline, &ccol );
  if ( ! ccol ) return QString::null; // no word
  KTextEditor::EditInterface *ei = KTextEditor::editInterface( m_view->document() );
  d->re.setPattern( "\\b(\\w+)$" );
  if ( d->re.searchRev(
        ei->text( cline, 0, cline, ccol )
        ) < 0 )
    return QString::null; // no word
  return d->re.cap( 1 );
}

// Scan throught the entire document for possible completions,
// ignoring any dublets
QValueList<KTextEditor::CompletionEntry> DocWordCompletionPluginView::allMatches( const QString &word )
{
  QValueList<KTextEditor::CompletionEntry> l;
  uint i( 0 );
  int pos( 0 );
  d->re.setPattern( "\\b("+word+"\\w+)" );
  QString s, m;
  KTextEditor::EditInterface *ei = KTextEditor::editInterface( m_view->document() );
  QDict<int> seen; // maybe slow with > 17 matches
  int sawit(1);    // to ref for the dict

  while( i < ei->numLines() )
  {
    s = ei->textLine( i );
    pos = 0;
    while ( pos >= 0 )
    {
      pos = d->re.search( s, pos );
      if ( pos >= 0 )
      {
        m = d->re.cap( 1 );
        if ( ! seen[ m ] ) {
          seen.insert( m, &sawit );
          KTextEditor::CompletionEntry e;
          e.text = m;
          l.append( e );
        }
        pos += d->re.matchedLength();
      }
    }
    i++;
  }
  return l;
}

//END
