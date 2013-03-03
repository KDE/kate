/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

//BEGIN includes
#include "katewordcompletion.h"
#include "kateview.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"

#include <ktexteditor/variableinterface.h>
#include <ktexteditor/movingrange.h>

#include <kconfig.h>
#include <kdialog.h>
#include <kpluginfactory.h>
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
#include <kcolorscheme.h>
#include <kaboutdata.h>

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

//BEGIN KateWordCompletionModel
KateWordCompletionModel::KateWordCompletionModel( QObject *parent )
  : CodeCompletionModel2( parent ), m_automatic(false)
{
  setHasGroups(false);
}

KateWordCompletionModel::~KateWordCompletionModel()
{
}

void KateWordCompletionModel::saveMatches( KTextEditor::View* view,
                        const KTextEditor::Range& range)
{
  m_matches = allMatches( view, range );
  m_matches.sort();
}

QVariant KateWordCompletionModel::data(const QModelIndex& index, int role) const
{
  if( role == InheritanceDepth )
    return 10000; //Very high value, so the word-completion group and items are shown behind any other groups/items if there is multiple

  if( !index.parent().isValid() ) {
    //It is the group header
    switch ( role )
    {
      case Qt::DisplayRole:
        return i18n("Auto Word Completion");
      case GroupRole:
        return Qt::DisplayRole;
    }
  }

  if( index.column() == KTextEditor::CodeCompletionModel::Name && role == Qt::DisplayRole )
    return m_matches.at( index.row() );

  if( index.column() == KTextEditor::CodeCompletionModel::Icon && role == Qt::DecorationRole ) {
    static QIcon icon(KIcon("insert-text").pixmap(QSize(16, 16)));
    return icon;
  }

  return QVariant();
}

QModelIndex KateWordCompletionModel::parent(const QModelIndex& index) const
{
  if(index.internalId())
    return createIndex(0, 0, 0);
  else
    return QModelIndex();
}

QModelIndex KateWordCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
  if( !parent.isValid()) {
    if(row == 0)
      return createIndex(row, column, 0);
    else
      return QModelIndex();

  }else if(parent.parent().isValid())
    return QModelIndex();


  if (row < 0 || row >= m_matches.count() || column < 0 || column >= ColumnCount )
    return QModelIndex();

  return createIndex(row, column, 1);
}

int KateWordCompletionModel::rowCount ( const QModelIndex & parent ) const
{
  if( !parent.isValid() && !m_matches.isEmpty() )
    return 1; //One root node to define the custom group
  else if(parent.parent().isValid())
    return 0; //Completion-items have no children
  else
    return m_matches.count();
}


bool KateWordCompletionModel::shouldStartCompletion(KTextEditor::View* view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position)
{
    if (!userInsertion) return false;
    if(insertedText.isEmpty())
        return false;


    KateView *v = qobject_cast<KateView*> (view);

    QString text = view->document()->line(position.line()).left(position.column());
    uint check=v->config()->wordCompletionMinimalWordLength();
    if (check<=0) return true;
    int start=text.length();
    int end=text.length()-check;
    if (end<0) return false;
    for (int i=start-1;i>=end;i--) {
      QChar c=text.at(i);
      if (! (c.isLetter() || (c.isNumber()) || c=='_') ) return false;
    }

    return true;
}

bool KateWordCompletionModel::shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range &range, const QString &currentCompletion) {

    if (m_automatic) {
      KateView *v = qobject_cast<KateView*> (view);
      if (currentCompletion.length()<v->config()->wordCompletionMinimalWordLength()) return true;
    }

    return CodeCompletionModelControllerInterface4::shouldAbortCompletion(view,range,currentCompletion);
}



void KateWordCompletionModel::completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType it)
{
  /**
   * auto invoke...
   */
  m_automatic=false;
  if (it==AutomaticInvocation) {
      m_automatic=true;
      KateView *v = qobject_cast<KateView*> (view);

      if (range.columnWidth() >= v->config()->wordCompletionMinimalWordLength())
        saveMatches( view, range );
      else
        m_matches.clear();

      // done here...
      return;
  }

  // normal case ;)
  saveMatches( view, range );
}


// Scan throughout the entire document for possible completions,
// ignoring any dublets
const QStringList KateWordCompletionModel::allMatches( KTextEditor::View *view, const KTextEditor::Range &range ) const
{
  QStringList l;
  QRegExp re( "\\b(" + view->document()->text( range ) + "\\w{1,})" );
  QSet<QString> seen;

  /**
   * scan only a range of the document to not die for LARGE files
   */
  const int lineLimit = 4096;
  int start = qMax(0, range.start().line() - lineLimit);
  int end = qMin(view->document()->lines(), range.start().line() + lineLimit);
  for (int i = start; i < end; ++i)
  {
    QString s = view->document()->line( i );
    int pos = 0;
    while ( pos >= 0 )
    {
      pos = re.indexIn( s, pos );
      if ( pos >= 0 )
      {
        // typing in the middle of a word
        if ( ! ( i == range.start().line() && pos == range.start().column() ) )
        {
          QString m = re.cap( 1 );
          if ( ! seen.contains( m ) ) {
            seen.insert( m );
            l << m;
          }
        }
        pos += re.matchedLength();
      }
    }
  }
  return l;
}

void KateWordCompletionModel::executeCompletionItem2(
    KTextEditor::Document* document
  , const KTextEditor::Range& word
  , const QModelIndex& index
  ) const
{
  KateView *v = qobject_cast<KateView*> (document->activeView());
  KTextEditor::Range r = word;
  if (v->config()->wordCompletionRemoveTail())
  {
    const QString& line = document->line(word.end().line());
    int real_word_size = line.length();
    for (int i = word.end().column(); i < real_word_size; ++i)
      // Letters, numbers and underscore are part of a word!
      /// \todo Introduce configurable \e word-separators??
      if (!line[i].isLetterOrNumber() && line[i] != '_')
        real_word_size = i;
    r.end().setColumn(real_word_size);
  }

  document->replaceText(r, m_matches.at(index.row()));
}

KTextEditor::CodeCompletionModelControllerInterface3::MatchReaction KateWordCompletionModel::matchingItem(const QModelIndex& /*matched*/)
{
  return HideListIfAutomaticInvocation;
}

bool KateWordCompletionModel::shouldHideItemsWithEqualNames() const
{
  // We don't want word-completion items if the same items
  // are available through more sophisticated completion models
  return true;
}

// Return the range containing the word left of the cursor
KTextEditor::Range KateWordCompletionModel::completionRange(KTextEditor::View* view, const KTextEditor::Cursor &position)
{
  int line = position.line();
  int col = position.column();

  KTextEditor::Document *doc = view->document();
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

  return KTextEditor::Range( KTextEditor::Cursor( line, col ), position );
}
//END KateWordCompletionModel


//BEGIN KateWordCompletionView
struct KateWordCompletionViewPrivate
{
  KTextEditor::MovingRange* liRange;       // range containing last inserted text
  KTextEditor::Range dcRange;  // current range to be completed by directional completion
  KTextEditor::Cursor dcCursor;     // directional completion search cursor
  QRegExp re;           // hrm
  int directionalPos;   // be able to insert "" at the correct time
  bool isCompleting; // true when the directional completion is doing a completion
};

KateWordCompletionView::KateWordCompletionView( KTextEditor::View *view, KActionCollection* ac )
  : QObject( view ),
    m_view( view ),
    m_dWCompletionModel( KateGlobal::self()->wordCompletionModel() ),
    d( new KateWordCompletionViewPrivate )
{
  d->isCompleting = false;
  d->dcRange = KTextEditor::Range::invalid();

  d->liRange = static_cast<KateDocument*>(m_view->document())->newMovingRange(KTextEditor::Range::invalid(), KTextEditor::MovingRange::DoNotExpand);

  KColorScheme colors(QPalette::Active);
  KTextEditor::Attribute::Ptr a = KTextEditor::Attribute::Ptr( new KTextEditor::Attribute() );
  a->setBackground( colors.background(KColorScheme::ActiveBackground) );
  a->setForeground( colors.foreground(KColorScheme::ActiveText) ); // ### this does 0
  d->liRange->setAttribute( a );

  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);

  KAction *action;

  if (cci)
  {
    cci->registerCompletionModel( m_dWCompletionModel );

    action = new KAction( i18n("Shell Completion"), this );
    ac->addAction( "doccomplete_sh", action );
    connect( action, SIGNAL(triggered()), this, SLOT(shellComplete()) );
  }


  action = new KAction( i18n("Reuse Word Above"), this );
  ac->addAction( "doccomplete_bw", action );
  action->setShortcut( Qt::CTRL+Qt::Key_8 );
  connect( action, SIGNAL(triggered()), this, SLOT(completeBackwards()) );

  action = new KAction( i18n("Reuse Word Below"), this );
  ac->addAction( "doccomplete_fw", action );
  action->setShortcut( Qt::CTRL+Qt::Key_9 );
  connect( action, SIGNAL(triggered()), this, SLOT(completeForwards()) );
}

KateWordCompletionView::~KateWordCompletionView()
{
  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(m_view);

  if (cci) cci->unregisterCompletionModel(m_dWCompletionModel);

  delete d;
}

void KateWordCompletionView::completeBackwards()
{
  complete( false );
}

void KateWordCompletionView::completeForwards()
{
  complete();
}

// Pop up the editors completion list if applicable
void KateWordCompletionView::popupCompletionList()
{
  kDebug( 13040 ) << "entered ...";
  KTextEditor::Range r = range();

  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>( m_view );
  if(!cci || cci->isCompletionActive())
    return;

  m_dWCompletionModel->saveMatches( m_view, r );

  kDebug( 13040 ) << "after save matches ...";

  if ( ! m_dWCompletionModel->rowCount(QModelIndex()) ) return;

  cci->startCompletion( r, m_dWCompletionModel );
}

// Contributed by <brain@hdsnet.hu>
void KateWordCompletionView::shellComplete()
{
  KTextEditor::Range r = range();

  QStringList matches = m_dWCompletionModel->allMatches( m_view, r );

  if (matches.size() == 0)
    return;

  QString partial = findLongestUnique( matches, r.columnWidth() );

  if ( ! partial.length() )
    popupCompletionList();

  else
  {
    m_view->document()->insertText( r.end(), partial.mid( r.columnWidth() ) );
    d->liRange->setView(m_view);
    d->liRange->setRange( KTextEditor::Range( r.end(), partial.length() - r.columnWidth() ) );
    connect( m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), this, SLOT(slotCursorMoved()) );
  }
}

// Do one completion, searching in the desired direction,
// if possible
void KateWordCompletionView::complete( bool fw )
{
  KTextEditor::Range r = range();

  int inc = fw ? 1 : -1;
  KTextEditor::Document *doc = m_view->document();

  if ( d->dcRange.isValid() )
  {
    //kDebug( 13040 )<<"CONTINUE "<<d->dcRange;
    // this is a repeted activation

    // if we are back to where we started, reset.
    if ( ( fw && d->directionalPos == -1 ) ||
         ( !fw && d->directionalPos == 1 ) )
    {
      const int spansColumns = d->liRange->end().column() - d->liRange->start().column();
      if ( spansColumns > 0 )
        doc->removeText( *d->liRange );

      d->liRange->setRange( KTextEditor::Range::invalid()  );
      d->dcCursor = r.end();
      d->directionalPos = 0;

      return;
    }

    if ( fw ) {
      const int spansColumns = d->liRange->end().column() - d->liRange->start().column();
      d->dcCursor.setColumn( d->dcCursor.column() + spansColumns );
    }

    d->directionalPos += inc;
  }
  else // new completion, reset all
  {
    //kDebug( 13040 )<<"RESET FOR NEW";
    d->dcRange = r;
    d->liRange->setRange( KTextEditor::Range::invalid() );
    d->dcCursor = r.start();
    d->directionalPos = inc;

    d->liRange->setView( m_view );

    connect( m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), this, SLOT(slotCursorMoved()) );

  }

  d->re.setPattern( "\\b" + doc->text( d->dcRange ) + "(\\w+)" );
  int pos ( 0 );
  QString ln = doc->line( d->dcCursor.line() );

  while ( true )
  {
    //kDebug( 13040 )<<"SEARCHING FOR "<<d->re.pattern()<<" "<<ln<<" at "<<d->dcCursor;
    pos = fw ?
      d->re.indexIn( ln, d->dcCursor.column() ) :
      d->re.lastIndexIn( ln, d->dcCursor.column() );

    if ( pos > -1 ) // we matched a word
    {
      //kDebug( 13040 )<<"USABLE MATCH";
      QString m = d->re.cap( 1 );
      if ( m != doc->text( *d->liRange ) && (d->dcCursor.line() != d->dcRange.start().line() || pos != d->dcRange.start().column() ) )
      {
        // we got good a match! replace text and return.
        d->isCompleting = true;
        KTextEditor::Range replaceRange(d->liRange->toRange());
        if (!replaceRange.isValid()) {
          replaceRange.setRange(r.end(), r.end());
        }
        doc->replaceText( replaceRange, m );
        d->liRange->setRange( KTextEditor::Range( d->dcRange.end(), m.length() ) );

        d->dcCursor.setColumn( pos ); // for next try

        d->isCompleting = false;
        return;
      }

      // equal to last one, continue
      else
      {
        //kDebug( 13040 )<<"SKIPPING, EQUAL MATCH";
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
      //kDebug( 13040 )<<"NO MATCH";
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

void KateWordCompletionView::slotCursorMoved()
{
  if ( d->isCompleting) return;

  d->dcRange = KTextEditor::Range::invalid();

  disconnect( m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), this, SLOT(slotCursorMoved()) );

  d->liRange->setView(0);
  d->liRange->setRange(KTextEditor::Range::invalid());
}

// Contributed by <brain@hdsnet.hu> FIXME
QString KateWordCompletionView::findLongestUnique( const QStringList &matches, int lead ) const
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
const QString KateWordCompletionView::word() const
{
  return m_view->document()->text( range() );
}

// Return the range containing the word behind the cursor
const KTextEditor::Range KateWordCompletionView::range() const
{
  return m_dWCompletionModel->completionRange(m_view, m_view->cursorPosition());
}
//END

#include "katewordcompletion.moc"
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
