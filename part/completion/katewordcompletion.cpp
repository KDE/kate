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
#include "katewordcompletion.h"

#include <ktexteditor/document.h>
#include <ktexteditor/variableinterface.h>
#include <ktexteditor/smartinterface.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/rangefeedback.h>

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
  : CodeCompletionModel( parent )
{
  setHasGroups(false);
}

KateWordCompletionModel::~KateWordCompletionModel()
{
}

void KateWordCompletionModel::saveMatches( KTextEditor::View* view,
                        const KTextEditor::Range& range)
{
  m_matches = allMatches( view, range, 2 );
  m_matches.sort();
}

QVariant KateWordCompletionModel::data(const QModelIndex& index, int role) const
{
  if ( index.column() !=  KTextEditor::CodeCompletionModel::Name ) return QVariant();

  switch ( role )
  {
    case Qt::DisplayRole:
//       kDebug( 13040 ) << ">>" << m_matches.at( index.row() ) << "<<";
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

QModelIndex KateWordCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
  if (row < 0 || row >= m_matches.count() || column < 0 || column >= ColumnCount || parent.isValid())
    return QModelIndex();

  return createIndex(row, column, 0);
}

int KateWordCompletionModel::rowCount ( const QModelIndex & parent ) const
{
  if( parent.isValid() )
    return 0; //Do not make the model look hierarchical
  else
    return m_matches.count();
}

void KateWordCompletionModel::completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType it)
{
 /* if (it==AutomaticInvocation) {
    DocWordCompletionPluginView *v=m_plugin->m_views[view];
    
      if ((range.columnWidth())>=3)
        saveMatches( view, range );
      else
        m_matches.clear();
  } else saveMatches( view, range );*/
}


// Scan throughout the entire document for possible completions,
// ignoring any dublets
const QStringList KateWordCompletionModel::allMatches( KTextEditor::View *view, const KTextEditor::Range &range, int minAdditionalLength ) const
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

//END KateWordCompletionModel

#include "katewordcompletion.moc"
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
