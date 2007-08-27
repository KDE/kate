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
#include <ktexteditor/smartinterface.h>
#include <ktexteditor/smartrange.h>

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

#include <QtCore/QRegExp>
#include <QtCore/QString>
#include <QtCore/QSet>
#include <QtGui/QSpinBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

#include <kvbox.h>
#include <QtGui/QCheckBox>

#include <kdebug.h>
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
    case MatchQuality:
      return 10;
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
        // typing in the middle of a word
        if ( ! ( i == range.start().line() && pos == range.start().column() ) )
        {
          m = re.cap( 1 );
          if ( ! seen.contains( m ) ) {
            seen.insert( m );
            l << m;
          }
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
DocWordCompletionPlugin *DocWordCompletionPlugin::plugin = 0;
K_PLUGIN_FACTORY_DECLARATION(DocWordCompletionFactory)
DocWordCompletionPlugin::DocWordCompletionPlugin( QObject *parent,
                            const QVariantList& /*args*/ )
  : KTextEditor::Plugin ( parent )
{
  plugin = this;
  m_dWCompletionModel = new DocWordCompletionModel( this );
  readConfig();
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

uint DocWordCompletionPlugin::treshold() const
{
    return m_treshold;
}

void DocWordCompletionPlugin::setTreshold(uint t)
{
    m_treshold = t;

    // If the property has been set for the plugin in general, let's set that
    // property to that value on all views where the plugin has been loaded.
    foreach (DocWordCompletionPluginView *view, m_views)
    {
        view->setTreshold(t);
    }
}

bool DocWordCompletionPlugin::autoPopupEnabled() const
{
    return m_autopopup;
}

void DocWordCompletionPlugin::setAutoPopupEnabled(bool enable)
{
    m_autopopup = enable;

    // If the property has been set for the plugin in general, let's set that
    // property to that value on all views where the plugin has been loaded.
    foreach (DocWordCompletionPluginView *view, m_views)
    {
        view->setAutoPopupEnabled(enable);
        view->toggleAutoPopup();
    }
}

//END

//BEGIN DocWordCompletionPluginView
struct DocWordCompletionPluginViewPrivate
{
  KTextEditor::SmartRange* liRange;       // range containing last inserted text
  KTextEditor::Range dcRange;  // current range to be completed by directional completion
  KTextEditor::Cursor dcCursor;     // directional completion search cursor
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
  d->dcRange = KTextEditor::Range();
  KTextEditor::SmartInterface *si =
     qobject_cast<KTextEditor::SmartInterface*>( m_view->document() );

  if( ! si )
    return;

  d->liRange = si->newSmartRange();

  KTextEditor::Attribute::Ptr a( new KTextEditor::Attribute() );
  a->setBackground( QColor("red") );
  a->setForeground( QColor("black") );
  d->liRange->setAttribute( a );
  si->addHighlightToView( m_view, d->liRange, false );

  view->insertChildClient( this );

  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);

  KAction *action;

  if (cci)
  {
    cci->registerCompletionModel( m_dWCompletionModel );

    action = new KAction( i18n("Pop Up Completion List"), this );
    actionCollection()->addAction( "doccomplete_pu", action );
    connect( action, SIGNAL( triggered() ), this, SLOT(popupCompletionList()) );

    d->autopopup = new KToggleAction( i18n("Automatic Completion Popup"), this );
    actionCollection()->addAction( "enable_autopopup", d->autopopup );
    connect( d->autopopup, SIGNAL( triggered() ), this, SLOT(toggleAutoPopup()) );

    d->autopopup->setChecked( autopopup );
    toggleAutoPopup();

    action = new KAction( i18n("Shell Completion"), this );
    actionCollection()->addAction( "doccomplete_sh", action );
    connect( action, SIGNAL( triggered() ), this, SLOT(shellComplete()) );
  }

  setComponentData( DocWordCompletionFactory::componentData() );

  action = new KAction( i18n("Reuse Word Above"), this );
  actionCollection()->addAction( "doccomplete_bw", action );
  action->setShortcut( Qt::CTRL+Qt::Key_8 );
  connect( action, SIGNAL( triggered() ), this, SLOT(completeBackwards()) );

  action = new KAction( i18n("Reuse Word Below"), this );
  actionCollection()->addAction( "doccomplete_fw", action );
  action->setShortcut( Qt::CTRL+Qt::Key_9 );
  connect( action, SIGNAL( triggered() ), this, SLOT(completeForwards()) );

  setXMLFile("docwordcompletionui.rc");

  KTextEditor::VariableInterface *vi = qobject_cast<KTextEditor::VariableInterface *>( view->document() );
  if ( vi )
  {
    QString e = vi->variable("wordcompletion-autopopup");
    if ( ! e.isEmpty() )
      d->autopopup->setEnabled( e == "true" );

    connect( view->document(), SIGNAL(variableChanged(KTextEditor::Document*,const QString &, const QString &)),
             this, SLOT(slotVariableChanged(KTextEditor::Document *,const QString &, const QString &)) );
  }
}

DocWordCompletionPluginView::~DocWordCompletionPluginView()
{
  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(m_view);

  if (cci) cci->unregisterCompletionModel(m_dWCompletionModel);

  delete d;
  d=0;
}

void DocWordCompletionPluginView::setTreshold( uint t )
{
  d->treshold = t;
}

void DocWordCompletionPluginView::setAutoPopupEnabled( bool enable )
{
  d->autopopup->setChecked(enable);
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
  KTextEditor::Range r = range();
  if ( r.isEmpty() )
    return;

  int inc = fw ? 1 : -1;
  KTextEditor::Document *doc = m_view->document();

  /* IF the current line is equal to the previous line
     AND the position - the length of the last inserted string
          is equal to the old position
     AND the lastinsertedlength last characters of the word is
          equal to the last inserted string
          */
  if ( r.start() == d->dcRange.start() && r.end() >= d->dcRange.end() )
  {
    //kDebug()<<"CONTINUE "<<d->dcRange;
    // this is a repeted activation

    // if we are back to where we started, reset.
    if ( ( fw && d->directionalPos == -1 ) ||
         ( !fw && d->directionalPos == 1 ) )
    {
      if ( d->liRange->columnWidth() )
        doc->removeText( *d->liRange );

      d->liRange->setRange( KTextEditor::Range( d->liRange->start(), 0 )  );
      d->dcCursor = r.end();
      d->directionalPos = 0;

      return;
    }

    if ( fw )
      d->dcCursor.setColumn( d->dcCursor.column() + d->liRange->columnWidth() );

    d->directionalPos += inc;
  }
  else // new completion, reset all
  {
    //kDebug()<<"RESET FOR NEW";
    d->dcRange = r;
    d->liRange->setRange( KTextEditor::Range( r.end(), 0 ) );
    d->dcCursor = r.start();
    d->directionalPos = inc;
  }

  d->re.setPattern( "\\b" + doc->text( d->dcRange ) + "(\\w+)" );
  int pos ( 0 );
  QString ln = doc->line( d->dcCursor.line() );

  while ( true )
  {
    //kDebug()<<"SEARCHING FOR "<<d->re.pattern()<<" "<<ln<<" at "<<d->dcCursor;
    pos = fw ?
      d->re.indexIn( ln, d->dcCursor.column() ) :
      d->re.lastIndexIn( ln, d->dcCursor.column() );

    if ( pos > -1 ) // we matched a word
    {
      //kDebug()<<"USABLE MATCH";
      QString m = d->re.cap( 1 );
      if ( m != doc->text( *d->liRange ) )
      {
        // we got good a match! replace text and return.
        doc->replaceText( *d->liRange, m );

        d->liRange->setRange( KTextEditor::Range( d->dcRange.end(), m.length() ) );
        d->dcCursor.setColumn( pos ); // for next try

        return;
      }

      // equal to last one, continue
      else
      {
        //kDebug()<<"SKIPPING, EQUAL MATCH";
        d->dcCursor.setColumn( pos ); // for next try

        if ( fw )
          d->dcCursor.setColumn( pos + m.length() );

        else
        {
          if ( pos == 0 )
          {
            if ( d->dcCursor.line() > 0 )
            {
              int l = d->dcCursor.line() + inc;
              ln = doc->line( l );
              d->dcCursor.setPosition( l, ln.length() );
            }
            else
            {
              KNotification::beep();
              return;
            }
          }

          else
            d->dcCursor.setColumn( d->dcCursor.column()-1 );
        }
      }
    }

    else  // no match
    {
      //kDebug()<<"NO MATCH";
      if ( (! fw && d->dcCursor.line() == 0 ) || ( fw && d->dcCursor.line() >= doc->lines() ) )
      {
        KNotification::beep();
        return;
      }

      int l = d->dcCursor.line() + inc;
      ln = doc->line( l );
      d->dcCursor.setPosition( l, fw ? 0 : ln.length() );
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

  if ( ! end.column() ) return KTextEditor::Range(); // no word
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

  return KTextEditor::Range( KTextEditor::Cursor( line, col ), end );
}

void DocWordCompletionPluginView::slotVariableChanged( KTextEditor::Document*,const QString &var, const QString &val )
{
  if ( var == "wordcompletion-autopopup" )
    d->autopopup->setEnabled( val == "true" );
  else if ( var == "wordcompletion-treshold" )
    d->treshold = val.toInt();
}
//END

#include "docwordcompletion_config.h"
K_PLUGIN_FACTORY_DEFINITION(DocWordCompletionFactory,
        registerPlugin<DocWordCompletionConfig>();
        registerPlugin<DocWordCompletionPlugin>();
        )
K_EXPORT_PLUGIN(DocWordCompletionFactory("ktexteditor_docwordcompletion", "ktexteditor_plugins"))

#include "docwordcompletion.moc"
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
