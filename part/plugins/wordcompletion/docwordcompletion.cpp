/*
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
#include <ktexteditor/variableinterface.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kaction.h>
#include <knotifyclient.h>
#include <kparts/part.h>
#include <kiconloader.h>

#include <qregexp.h>
#include <qstring.h>
#include <qdict.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>

// #include <kdebug.h>
//END

//BEGIN DocWordCompletionPlugin
K_EXPORT_COMPONENT_FACTORY( ktexteditor_docwordcompletion, KGenericFactory<DocWordCompletionPlugin>( "ktexteditor_docwordcompletion" ) )
DocWordCompletionPlugin::DocWordCompletionPlugin( QObject *parent,
                            const char* name,
                            const QStringList& /*args*/ )
	: KTextEditor::Plugin ( (KTextEditor::Document*) parent, name )
{
  readConfig();
}

void DocWordCompletionPlugin::readConfig()
{
  KConfig *config = kapp->config();
  config->setGroup( "DocWordCompletion Plugin" );
  m_treshold = config->readNumEntry( "treshold", 3 );
  m_autopopup = config->readBoolEntry( "autopopup", true );
}

void DocWordCompletionPlugin::writeConfig()
{
  KConfig *config = kapp->config();
  config->setGroup("DocWordCompletion Plugin");
  config->writeEntry("autopopup", m_autopopup );
  config->writeEntry("treshold", m_treshold );
}

void DocWordCompletionPlugin::addView(KTextEditor::View *view)
{
  DocWordCompletionPluginView *nview = new DocWordCompletionPluginView (m_treshold, m_autopopup, view, "Document word completion");
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

KTextEditor::ConfigPage* DocWordCompletionPlugin::configPage( uint, QWidget *parent, const char *name )
{
  return new DocWordCompletionConfigPage( this, parent, name );
}

QString DocWordCompletionPlugin::configPageName( uint ) const
{
  return i18n("Word Completion Plugin");
}

QString DocWordCompletionPlugin::configPageFullName( uint ) const
{
  return i18n("Configure the Word Completion Plugin");
}

// FIXME provide sucn a icon
       QPixmap DocWordCompletionPlugin::configPagePixmap( uint, int size ) const
{
  return UserIcon( "kte_wordcompletion", size );
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
  uint treshold;         // the required length of a word before popping up the completion list automatically
};

DocWordCompletionPluginView::DocWordCompletionPluginView( uint treshold, bool autopopup, KTextEditor::View *view, const char *name )
  : QObject( view, name ),
    KXMLGUIClient( view ),
    m_view( view ),
    d( new DocWordCompletionPluginViewPrivate )
{
  d->treshold = treshold;
  view->insertChildClient( this );
  setInstance( KGenericFactory<DocWordCompletionPlugin>::instance() );

  (void) new KAction( i18n("Reuse Word Above"), CTRL+Key_8, this,
    SLOT(completeBackwards()), actionCollection(), "doccomplete_bw" );
  (void) new KAction( i18n("Reuse Word Below"), CTRL+Key_9, this,
    SLOT(completeForwards()), actionCollection(), "doccomplete_fw" );
  (void) new KAction( i18n("Pop Up Completion List"), 0, this,
    SLOT(popupCompletionList()), actionCollection(), "doccomplete_pu" );
  (void) new KAction( i18n("Shell Completion"), 0, this,
    SLOT(shellComplete()), actionCollection(), "doccomplete_sh" );
  d->autopopup = new KToggleAction( i18n("Automatic Completion Popup"), 0, this,
    SLOT(toggleAutoPopup()), actionCollection(), "enable_autopopup" );

  d->autopopup->setChecked( autopopup );
  toggleAutoPopup();

  setXMLFile("docwordcompletionui.rc");

  KTextEditor::VariableInterface *vi = KTextEditor::variableInterface( view->document() );
  if ( vi )
  {
    QString e = vi->variable("wordcompletion-autopopup");
    if ( ! e.isEmpty() )
      d->autopopup->setEnabled( e == "true" );

    connect( view->document(), SIGNAL(variableChanged(const QString &, const QString &)),
             this, SLOT(slotVariableChanged(const QString &, const QString &)) );
  }
}

void DocWordCompletionPluginView::settreshold( uint t )
{
  d->treshold = t;
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
  if ( d->autopopup->isChecked() ) {
    if ( ! connect( m_view->document(), SIGNAL(charactersInteractivelyInserted(int ,int ,const QString&)),
         this, SLOT(autoPopupCompletionList()) ))
    {
      connect( m_view->document(), SIGNAL(textChanged()), this, SLOT(autoPopupCompletionList()) );
    }
  } else {
    disconnect( m_view->document(), SIGNAL(textChanged()), this, SLOT(autoPopupCompletionList()) );
    disconnect( m_view->document(), SIGNAL(charactersInteractivelyInserted(int ,int ,const QString&)),
                this, SLOT(autoPopupCompletionList()) );

  }
}

// for autopopup FIXME - don't pop up if reuse word is inserting
void DocWordCompletionPluginView::autoPopupCompletionList()
{
  if ( ! m_view->hasFocus() ) return;
  QString w = word();
  if ( w.length() >= d->treshold )
  {
      popupCompletionList( w );
  }
}

// Contributed by <brain@hdsnet.hu>
void DocWordCompletionPluginView::shellComplete()
{
    // setup
  KTextEditor::EditInterface * ei = KTextEditor::editInterface(m_view->document());
    // find the word we are typing
  uint cline, ccol;
  viewCursorInterface(m_view)->cursorPositionReal(&cline, &ccol);
  QString wrd = word();
  if (wrd.isEmpty())
    return;

  QValueList < KTextEditor::CompletionEntry > matches = allMatches(wrd);
  if (matches.size() == 0)
    return;
  QString partial = findLongestUnique(matches);
  if (partial.length() == wrd.length())
  {
    KTextEditor::CodeCompletionInterface * cci = codeCompletionInterface(m_view);
    cci->showCompletionBox(matches, wrd.length());
  }
  else
  {
    partial.remove(0, wrd.length());
    ei->insertText(cline, ccol, partial);
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
  viewCursorInterface( m_view )->cursorPositionReal( &cline, &ccol );
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

// Contributed by <brain@hdsnet.hu>
QString DocWordCompletionPluginView::findLongestUnique(const QValueList < KTextEditor::CompletionEntry > &matches)
{
  QString partial = matches.front().text;
  QValueList < KTextEditor::CompletionEntry >::const_iterator i = matches.begin();
  for (++i; i != matches.end(); ++i)
  {
    if (!(*i).text.startsWith(partial))
    {
      while(partial.length() > 0)
      {
        partial.remove(partial.length() - 1, 1);
        if ((*i).text.startsWith(partial))
        {
          break;
        }
      }
      if (partial.length() == 0)
        return QString();
    }
  }

  return partial;
}

// Return the string to complete (the letters behind the cursor)
QString DocWordCompletionPluginView::word()
{
  uint cline, ccol;
  viewCursorInterface( m_view )->cursorPositionReal( &cline, &ccol );
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

void DocWordCompletionPluginView::slotVariableChanged( const QString &var, const QString &val )
{
  if ( var == "wordcompletion-autopopup" )
    d->autopopup->setEnabled( val == "true" );
  else if ( var == "wordcompletion-treshold" )
    d->treshold = val.toInt();
}
//END

//BEGIN DocWordCompletionConfigPage
DocWordCompletionConfigPage::DocWordCompletionConfigPage( DocWordCompletionPlugin *completion, QWidget *parent, const char *name )
  : KTextEditor::ConfigPage( parent, name )
  , m_completion( completion )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  lo->setSpacing( KDialog::spacingHint() );

  cbAutoPopup = new QCheckBox( i18n("Automatically &show completion list"), this );
  lo->addWidget( cbAutoPopup );

  QHBox *hb = new QHBox( this );
  hb->setSpacing( KDialog::spacingHint() );
  lo->addWidget( hb );
  QLabel *l = new QLabel( i18n(
      "Translators: This is the first part of two strings wich will comprise the "
      "sentence 'Show completions when a word is at least N characters'. The first "
      "part is on the right side of the N, which is represented by a spinbox "
      "widget, followed by the second part: 'characters long'. Characters is a "
      "ingeger number between and including 1 and 30. Feel free to leave the "
      "second part of the sentence blank if it suits your language better. ",
      "Show completions &when a word is at least"), hb );
  sbAutoPopup = new QSpinBox( 1, 30, 1, hb );
  l->setBuddy( sbAutoPopup );
  lSbRight = new QLabel( i18n(
      "This is the second part of two strings that will comprise teh sentence "
      "'Show completions when a word is at least N characters'",
      "characters long."), hb );

  QWhatsThis::add( cbAutoPopup, i18n(
      "Enable the automatic completion list popup as default. The popup can "
      "be disabled on a view basis from the 'Tools' menu.") );
  QWhatsThis::add( sbAutoPopup, i18n(
      "Define the length a word should have before the completion list "
      "is displayed.") );

  cbAutoPopup->setChecked( m_completion->autoPopupEnabled() );
  sbAutoPopup->setValue( m_completion->treshold() );

  lo->addStretch();
}

void DocWordCompletionConfigPage::apply()
{
  m_completion->setAutoPopupEnabled( cbAutoPopup->isChecked() );
  m_completion->setTreshold( sbAutoPopup->value() );
  m_completion->writeConfig();
}

void DocWordCompletionConfigPage::reset()
{
  cbAutoPopup->setChecked( m_completion->autoPopupEnabled() );
  sbAutoPopup->setValue( m_completion->treshold() );
}

void DocWordCompletionConfigPage::defaults()
{
  cbAutoPopup->setChecked( true );
  sbAutoPopup->setValue( 3 );
}

//END DocWordCompletionConfigPage

#include "docwordcompletion.moc"
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
