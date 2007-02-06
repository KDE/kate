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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

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
#include <ktexteditor/variableinterface.h>

#include <kconfig.h>
#include <kdialog.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <knotification.h>
#include <kparts/part.h>
#include <kiconloader.h>
#include <kpagedialog.h>
#include <kpagewidgetmodel.h>
#include <ktoggleaction.h>
#include <kconfiggroup.h>

#include <qregexp.h>
#include <qstring.h>
#include <QSet>
#include <qspinbox.h>
#include <qlabel.h>
#include <qlayout.h>

#include <kvbox.h>
#include <qcheckbox.h>

// #include <kdebug.h>
//END

//BEGIN DocWordCompletionModel
DocWordCompletionModel::DocWordCompletionModel( QObject *parent )
  : CodeCompletionModel( parent )
{
}

DocWordCompletionModel::~DocWordCompletionModel()
{
}

void DocWordCompletionModel::saveMatches( KTextEditor::View* view,
                        const KTextEditor::Range& range)
{
  m_matches = allMatches( view, range, 2 );
  m_matches.sort();
}

QVariant DocWordCompletionModel::data(const QModelIndex& index, int role) const
{
  switch ( role )
  {
    case Qt::DisplayRole:
//       if ( index.column() == Name )
        return m_matches.at( index.row() );
    case CompletionRole:
      return (int)FirstProperty|LastProperty|Public;
    case ScopeIndex:
      return 0;
    case MatchType:
      return true;
    case HighlightingMethod:
      return QVariant::Invalid;
    case InheritanceDepth:
      return 0;
  }

  return QVariant();
}

QModelIndex DocWordCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
  if (row < 0 || row >= m_matches.count() || column < 0 || column >= ColumnCount || parent.isValid())
    return QModelIndex();

  return createIndex(row, column, 0);
}

int DocWordCompletionModel::rowCount ( const QModelIndex & parent ) const
{
  Q_UNUSED( parent );
  return m_matches.count();
}

// Scan throughout the entire document for possible completions,
// ignoring any dublets
const QStringList DocWordCompletionModel::allMatches( KTextEditor::View *view, const KTextEditor::Range &range, int minAdditionalLength ) const
{
  QStringList l;

  // we complete words on a single line, that has a length
  if ( range.numberOfLines() || ! range.columnWidth() )
    return l;

  int i( 0 );
  int pos( 0 );
  KTextEditor::Document *doc = view->document();
  QRegExp re( "\\b(" + doc->text( range ) + "\\w{" + QString::number(minAdditionalLength) + ",})" );
  QString s, m;
  QSet<QString> seen;

  while( i < doc->lines() )
  {
    s = doc->line( i );
    pos = 0;
    while ( pos >= 0 )
    {
      pos = re.indexIn( s, pos );
      if ( pos >= 0 )
      {
        m = re.cap( 1 );
        if ( ! seen.contains( m ) ) {
          seen.insert( m );
          l << m;
        }
        pos += re.matchedLength();
      }
    }
    i++;
  }
  return l;
}

//END DocWordCompletionModel

//BEGIN DocWordCompletionPlugin
K_EXPORT_COMPONENT_FACTORY( ktexteditor_docwordcompletion, KGenericFactory<DocWordCompletionPlugin>( "ktexteditor_docwordcompletion" ) )
DocWordCompletionPlugin::DocWordCompletionPlugin( QObject *parent,
                            const QStringList& /*args*/ )
  : KTextEditor::Plugin ( parent )
{
  m_dWCompletionModel = new DocWordCompletionModel( this );
  readConfig();
}

void DocWordCompletionPlugin::configDialog (QWidget *parent)
{
 // If we have only one page, we use a simple dialog, else an icon list type
  KPageDialog::FaceType ft = configPages() > 1 ? KPageDialog::List :     // still untested
                                                 KPageDialog::Plain;

  KPageDialog *kd = new KPageDialog ( parent );
  kd->setFaceType( ft );
  kd->setCaption( i18n("Configure") );
  kd->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Help );
  kd->setDefaultButton( KDialog::Ok );

  QList<KTextEditor::ConfigPage*> editorPages;

  for (uint i = 0; i < configPages (); i++)
  {
    QWidget *page = new QWidget(0);

    KPageWidgetItem *item = new KPageWidgetItem( page, configPageName( i ) );
    item->setHeader( configPageFullName( i ) );
// FIXME: set the icon here      item->setIcon();

    kd->addPage( item );

    editorPages.append( configPage( i, page ) );
  }

  if (kd->exec())
  {

    for( int i=0; i<editorPages.count(); i++ )
    {
      editorPages.at( i )->apply();
    }
  }

  delete kd;
}

void DocWordCompletionPlugin::readConfig()
{
  KConfigGroup cg(KGlobal::config(), "DocWordCompletion Plugin" );
  m_treshold = cg.readEntry( "treshold", 3 );
  m_autopopup = cg.readEntry( "autopopup", true );
}

void DocWordCompletionPlugin::writeConfig()
{
  KConfigGroup cg(KGlobal::config(), "DocWordCompletion Plugin" );
  cg.writeEntry("autopopup", m_autopopup );
  cg.writeEntry("treshold", m_treshold );
}

void DocWordCompletionPlugin::addView(KTextEditor::View *view)
{
  DocWordCompletionPluginView *nview = new DocWordCompletionPluginView (m_treshold, m_autopopup, view, m_dWCompletionModel );
  m_views.append (nview);
}

void DocWordCompletionPlugin::removeView(KTextEditor::View *view)
{
  for (int z=0; z < m_views.size(); ++z)
    if (m_views.at(z)->parentClient() == view)
    {
       DocWordCompletionPluginView *nview = m_views.at(z);
       m_views.removeAll (nview);
       delete nview;
    }
}

KTextEditor::ConfigPage* DocWordCompletionPlugin::configPage( uint, QWidget *parent )
{
  return new DocWordCompletionConfigPage( this, parent );
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
  uint treshold;        // the required length of a word before popping up the completion list automatically
  int directionalPos;   // be able to insert "" at the correct time
};

DocWordCompletionPluginView::DocWordCompletionPluginView( uint treshold,
                                                          bool autopopup,
                                                          KTextEditor::View *view,
                                                          DocWordCompletionModel *completionModel )
  : QObject( view ),
    KXMLGUIClient( view ),
    m_view( view ),
    m_dWCompletionModel( completionModel ),
    d( new DocWordCompletionPluginViewPrivate )
{
//   setObjectName( name );

  d->treshold = treshold;
  view->insertChildClient( this );
  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);

  if (cci)
  {
    cci->registerCompletionModel( m_dWCompletionModel );
  }

  setComponentData( KGenericFactory<DocWordCompletionPlugin>::componentData() );

  KAction *action = new KAction( i18n("Reuse Word Above"), this );
  actionCollection()->addAction( "doccomplete_bw", action );
  action->setShortcut( Qt::CTRL+Qt::Key_8 );
  connect( action, SIGNAL( triggered() ), this, SLOT(completeBackwards()) );

  action = new KAction( i18n("Reuse Word Below"), this );
  actionCollection()->addAction( "doccomplete_fw", action );
  action->setShortcut( Qt::CTRL+Qt::Key_9 );
  connect( action, SIGNAL( triggered() ), this, SLOT(completeForwards()) );

  action = new KAction( i18n("Pop Up Completion List"), this );
  actionCollection()->addAction( "doccomplete_pu", action );
  connect( action, SIGNAL( triggered() ), this, SLOT(popupCompletionList()) );

  action = new KAction( i18n("Shell Completion"), this );
  actionCollection()->addAction( "doccomplete_sh", action );
  connect( action, SIGNAL( triggered() ), this, SLOT(shellComplete()) );

  d->autopopup = new KToggleAction( i18n("Automatic Completion Popup"), this );
  actionCollection()->addAction( "enable_autopopup", d->autopopup );
  connect( d->autopopup, SIGNAL( triggered() ), this, SLOT(toggleAutoPopup()) );

  d->autopopup->setChecked( autopopup );
  toggleAutoPopup();

  setXMLFile("docwordcompletionui.rc");

  KTextEditor::VariableInterface *vi = qobject_cast<KTextEditor::VariableInterface *>( view->document() );
  if ( vi )
  {
    QString e = vi->variable("wordcompletion-autopopup");
    if ( ! e.isEmpty() )
      d->autopopup->setEnabled( e == "true" );

    connect( view->document(), SIGNAL(variableChanged(const QString &, const QString &)),
             this, SLOT(slotVariableChanged(const QString &, const QString &)) );
  }
}

DocWordCompletionPluginView::~DocWordCompletionPluginView()
{
  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(m_view);

  if (cci) cci->unregisterCompletionModel(m_dWCompletionModel);

  delete d;
  d=0;
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
void DocWordCompletionPluginView::popupCompletionList()
{
  KTextEditor::Range r = range();

  if ( r.isEmpty() )
    return;

  m_dWCompletionModel->saveMatches( m_view, r );

  if ( ! m_dWCompletionModel->rowCount(QModelIndex()) ) return;

  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>( m_view );
  if ( cci )
    cci->startCompletion( r, m_dWCompletionModel );
}

void DocWordCompletionPluginView::toggleAutoPopup()
{
  if ( d->autopopup->isChecked() ) {
    if ( ! connect( m_view, SIGNAL(textInserted ( KTextEditor::View *, const KTextEditor::Cursor &, const QString & )),
         this, SLOT(autoPopupCompletionList()) ))
    {
      connect( m_view->document(), SIGNAL(textChanged(KTextEditor::View *)), this, SLOT(autoPopupCompletionList()) );
    }
  } else {
    disconnect( m_view->document(), SIGNAL(textChanged(KTextEditor::View *)), this, SLOT(autoPopupCompletionList()) );
    disconnect( m_view, SIGNAL(textInserted( KTextEditor::View *, const KTextEditor::Cursor &, const QString &)),
                this, SLOT(autoPopupCompletionList()) );

  }
}

// for autopopup FIXME - don't pop up if reuse word is inserting
void DocWordCompletionPluginView::autoPopupCompletionList()
{
  if ( ! m_view->hasFocus() ) return;
  KTextEditor::Range r = range();
  if ( r.columnWidth() >= (int)d->treshold )
  {
      popupCompletionList();
  }
}

// Contributed by <brain@hdsnet.hu>
void DocWordCompletionPluginView::shellComplete()
{
  KTextEditor::Range r = range();
  if (r.isEmpty())
    return;

  QStringList matches = m_dWCompletionModel->allMatches( m_view, r );

  if (matches.size() == 0)
    return;

  QString partial = findLongestUnique( matches, r.columnWidth() );

  if ( partial.length() == r.columnWidth() )
    popupCompletionList();

  else
  {
    m_view->document()->insertText( r.end(), partial.mid( r.columnWidth() ) );
  }
}

// Do one completion, searching in the desired direction,
// if possible
void DocWordCompletionPluginView::complete( bool fw )
{
  // find the word we are typing
  int cline, ccol;
  m_view->cursorPosition().position ( cline, ccol );
  QString wrd = word();
  if ( wrd.isEmpty() )
    return;

  int inc = fw ? 1 : -1;

  /* IF the current line is equal to the previous line
     AND the position - the length of the last inserted string
          is equal to the old position
     AND the lastinsertedlength last characters of the word is
          equal to the last inserted string
          */
  if ( cline == (int)d-> cline &&
          ccol - d->lilen == d->ccol &&
          wrd.endsWith( d->lastIns ) )
  {
    // this is a repeted activation
    ccol = d->ccol;
    wrd = d->last;

    // if we are back to where we started, reset.
    if ( ( fw && d->directionalPos == -1 ) ||
         ( !fw && d->directionalPos == 1 ) )
    {
      if ( d->lilen )
        m_view->document()->removeText( KTextEditor::Range(d->cline, d->ccol, d->cline, d->ccol + d->lilen) );

      d->lastIns = "";
      d->lilen = 0;
      d->line = d->cline;
      d->col = d->ccol;
      d->directionalPos = 0;

      return;
    }

    d->directionalPos += inc;
  }
  else
  {
    d->cline = cline;
    d->ccol = ccol;
    d->last = wrd;
    d->lastIns.clear();
    d->line = cline;
    d->col = ccol - wrd.length();
    d->lilen = 0;
    d->directionalPos = inc;
  }

  d->re.setPattern( "\\b" + wrd + "(\\w+)" );
  int pos ( 0 );
  QString ln = m_view->document()->line( d->line );

  if ( ! fw )
    ln = ln.left( d->col );

  while ( true )
  {
    pos = fw ?
      d->re.indexIn( ln, d->col ) :
      d->re.lastIndexIn( ln, d->col );

    if ( pos > -1 ) // we matched a word
    {
      QString m = d->re.cap( 1 );
      if ( m != d->lastIns )
      {
        // we got good a match! replace text and return.
        if ( d->lilen )
          m_view->document()->removeText( KTextEditor::Range(d->cline, d->ccol, d->cline, d->ccol + d->lilen) );
        m_view->document()->insertText( KTextEditor::Cursor (d->cline, d->ccol), m );

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
          d->col += d->re.matchedLength();

        else
        {
          if ( pos == 0 )
          {
            if ( d->line > 0 )
            {
              d->line += inc;
              ln = m_view->document()->line( d->line );
              d->col = ln.length();
            }
            else
            {
              KNotification::beep();
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
      if ( (! fw && d->line == 0 ) || ( fw && d->line >= (uint)m_view->document()->lines() ) )
      {
        KNotification::beep();
        return;
      }

      d->line += inc;

      ln = m_view->document()->line( d->line );
      d->col = fw ? 0 : ln.length();
    }
  } // while true
}

// Contributed by <brain@hdsnet.hu> FIXME
QString DocWordCompletionPluginView::findLongestUnique( const QStringList &matches, int lead ) const
{
  QString partial = matches.first();

  QStringListIterator it( matches );
  QString current;
  while ( it.hasNext() )
  {
    current = it.next();
    if ( !current.startsWith( partial ) )
    {
      while( partial.length() > lead )
      {
        partial.remove( partial.length() - 1, 1 );
        if ( current.startsWith( partial ) )
          break;
      }

      if ( partial.length() == lead )
        return QString();
    }
  }

  return partial;
}


// Return the string to complete (the letters behind the cursor)
const QString DocWordCompletionPluginView::word() const
{
  return m_view->document()->text( range() );
}

// Return the range containing the word behind the cursor
const KTextEditor::Range DocWordCompletionPluginView::range() const
{
  KTextEditor::Cursor end = m_view->cursorPosition();

  if ( ! end.column() ) return KTextEditor::Range::Range(); // no word
  int line = end.line();
  int col = end.column();

  KTextEditor::Document *doc = m_view->document();
  while ( col > 0 )
  {
    QChar c = ( doc->character( KTextEditor::Cursor( line, col-1 ) ) );
    if ( c.isLetterOrNumber() || c.isMark() || c == '_' )
    {
      col--;
      continue;
    }

    break;
  }

  return KTextEditor::Range::Range( KTextEditor::Cursor( line, col ), end );
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
DocWordCompletionConfigPage::DocWordCompletionConfigPage( DocWordCompletionPlugin *completion, QWidget *parent )
  : KTextEditor::ConfigPage( parent )
  , m_completion( completion )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  lo->setSpacing( KDialog::spacingHint() );

  cbAutoPopup = new QCheckBox( i18n("Automatically &show completion list"), this );
  lo->addWidget( cbAutoPopup );

  KHBox *hb = new KHBox( this );
  hb->setSpacing( KDialog::spacingHint() );
  lo->addWidget( hb );
  QLabel *l = new QLabel( i18nc(
      "Translators: This is the first part of two strings which will comprise the "
      "sentence 'Show completions when a word is at least N characters'. The first "
      "part is on the right side of the N, which is represented by a spinbox "
      "widget, followed by the second part: 'characters long'. Characters is a "
      "ingeger number between and including 1 and 30. Feel free to leave the "
      "second part of the sentence blank if it suits your language better. ",
      "Show completions &when a word is at least"), hb );
  sbAutoPopup = new QSpinBox( hb );
  sbAutoPopup->setRange( 1, 30 );
  sbAutoPopup->setSingleStep( 1 );
  l->setBuddy( sbAutoPopup );
  lSbRight = new QLabel( i18nc(
      "This is the second part of two strings that will comprise the sentence "
      "'Show completions when a word is at least N characters'",
      "characters long."), hb );

  cbAutoPopup->setWhatsThis(i18n(
      "Enable the automatic completion list popup as default. The popup can "
      "be disabled on a view basis from the 'Tools' menu.") );
  sbAutoPopup->setWhatsThis(i18n(
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
