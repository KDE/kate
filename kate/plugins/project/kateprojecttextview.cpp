/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
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

#include "kateprojecttextview.h"

#include <klocale.h>
#include <kicon.h>

KateProjectTextView::KateProjectTextView (KateProjectPluginView *pluginView, KTextEditor::View *view)
  : KTextEditor::CodeCompletionModel (view)
  , m_pluginView (pluginView)
  , m_view (view)
{
 
}

KateProjectTextView::~KateProjectTextView ()
{
}

void KateProjectTextView::saveMatches( KTextEditor::View* view,
                        const KTextEditor::Range& range)
{
  m_matches = allMatches( view, range );
  m_matches.sort();
}

QVariant KateProjectTextView::data(const QModelIndex& index, int role) const
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

QModelIndex KateProjectTextView::parent(const QModelIndex& index) const
{
  if(index.internalId())
    return createIndex(0, 0, 0);
  else
    return QModelIndex();
}

QModelIndex KateProjectTextView::index(int row, int column, const QModelIndex& parent) const
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

int KateProjectTextView::rowCount ( const QModelIndex & parent ) const
{
  if( !parent.isValid() && !m_matches.isEmpty() )
    return 1; //One root node to define the custom group
  else if(parent.parent().isValid())
    return 0; //Completion-items have no children
  else
    return m_matches.count();
}


bool KateProjectTextView::shouldStartCompletion(KTextEditor::View* view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position)
{
    if (!userInsertion) return false;
    if(insertedText.isEmpty())
        return false;

    QString text = view->document()->line(position.line()).left(position.column());
    
    uint check=3;//v->config()->wordCompletionMinimalWordLength();
    
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

bool KateProjectTextView::shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::Range &range, const QString &currentCompletion) {

    if (m_automatic) {
      if (currentCompletion.length() < 3 /*v->config()->wordCompletionMinimalWordLength()*/) return true;
    }

    return CodeCompletionModelControllerInterface3::shouldAbortCompletion(view,range,currentCompletion);
}

void KateProjectTextView::completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType it)
{
  /**
   * auto invoke...
   */
  m_automatic=false;
  if (it==AutomaticInvocation) {
      m_automatic=true;

      if (range.columnWidth() >= 3 /*v->config()->wordCompletionMinimalWordLength()*/)
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
const QStringList KateProjectTextView::allMatches( KTextEditor::View *view, const KTextEditor::Range &range ) const
{

  QStringList l;
#if 0
  int i( 0 );
  int pos( 0 );
  KTextEditor::Document *doc = view->document();
  QRegExp re( "\\b(" + doc->text( range ) + "\\w{1,})" );
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
#endif
  return l;
}

KTextEditor::CodeCompletionModelControllerInterface3::MatchReaction KateProjectTextView::matchingItem(const QModelIndex& /*matched*/)
{
  return HideListIfAutomaticInvocation;
}

// Return the range containing the word left of the cursor
KTextEditor::Range KateProjectTextView::completionRange(KTextEditor::View* view, const KTextEditor::Cursor &position)
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

// kate: space-indent on; indent-width 2; replace-tabs on;
