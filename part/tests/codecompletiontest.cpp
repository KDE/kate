/* This file is part of the KDE project
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#include "codecompletiontest.h"

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

CodeCompletionTest::CodeCompletionTest(KTextEditor::View* parent)
  : KTextEditor::CodeCompletionModel(parent)
{
  setRowCount(40);

  connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(cursorPositionChanged()));

  Q_ASSERT(cc());
}

// Fake a series of completions
QVariant CodeCompletionTest::data( const QModelIndex & index, int role ) const
{
  switch (role) {
    case Qt::DisplayRole:
      if (index.row() < rowCount() / 2)
      switch (index.column()) {
        case Prefix:
          switch (index.row() % 3) {
            default:
              return "void ";
            case 1:
              return "const QString& ";
            case 2:
              if (index.row() % 6)
                return "inline virtual bool ";
              return "virtual bool ";
          }

        case Scope:
          switch (index.row() % 4) {
            default:
              return QString();
            case 1:
              return "KTextEditor::";
            case 2:
              return "::";
            case 3:
              return "std::";
          }

        case Name:
          return m_startText + QString("%1%2%3").arg(QChar('a' + index.row())).arg(QChar('a' + (index.row() % 3))).arg(index.row());

        case Arguments:
          switch (index.row() % 5) {
            default:
              return "()";
            case 1:
              return "(bool trigger)";
            case 4:
              return "(const QString& name, Qt::CaseSensitivity cs)";
            case 5:
              return "(int count)";
          }

        case Postfix:
          switch (index.row() % 3) {
            default:
              return " const";
            case 1:
              return " KDE_DEPRECATED";
            case 2:
              return "";
          }
      }
      else
      switch (index.column()) {
        case Prefix:
          switch (index.row() % 3) {
            default:
              return "void ";
            case 1:
              return "const QString ";
            case 2:
              return "bool ";
          }

        case Scope:
          switch (index.row() % 4) {
            default:
              return QString();
            case 1:
              return "KTextEditor::";
            case 2:
              return "::";
            case 3:
              return "std::";
          }

        case Name:
          return m_startText + QString("%1%2%3").arg(QChar('a' + index.row())).arg(QChar('a' + (index.row() % 3))).arg(index.row());

        default:
          return "";
      }
        break;

    case Qt::DecorationRole:
      break;

    case CompletionRole: {
      CompletionProperties p;
      if (index.row() < rowCount() / 2)
        p |= Function;
      else
        p |= Variable;
      switch (index.row() % 3) {
        case 0:
          p |= Const;
      }
      return (int)p;
    }

    case ScopeIndex:
      return (index.row() % 4 ) - 1;
  }

  return QVariant();
}

void CodeCompletionTest::cursorPositionChanged( )
{
  if (cc()->isCompletionActive())
    return;

  KTextEditor::Cursor end = view()->cursorPosition();

  if (!end.column())
    return;

  QString text = view()->document()->line(end.line());

  static QRegExp findWordStart( "\\b(\\w+)$" );
  static QRegExp findWordEnd( "^(\\w*)\\b" );

  if ( findWordStart.lastIndexIn(text.left(end.column())) < 0 )
    return;

  KTextEditor::Cursor start(end.line(), findWordStart.pos(1));

  if ( findWordEnd.indexIn(text.mid(end.column())) < 0 )
    return;

  end.setColumn(end.column() + findWordEnd.cap(1).length());

  m_startText = text.mid(start.column(), end.column() - start.column());

  cc()->startCompletion(KTextEditor::Range(start, end), this);
}

KTextEditor::View* CodeCompletionTest::view( ) const
{
  return static_cast<KTextEditor::View*>(const_cast<QObject*>(QObject::parent()));
}

KTextEditor::CodeCompletionInterface2 * CodeCompletionTest::cc( ) const
{
  return dynamic_cast<KTextEditor::CodeCompletionInterface2*>(const_cast<QObject*>(QObject::parent()));
}

#include "codecompletiontest.moc"
