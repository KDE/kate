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
#include <ktexteditor/codecompletioninterface.h>

CodeCompletionTest::CodeCompletionTest(KTextEditor::View* parent)
  : KTextEditor::CodeCompletionModel(parent)
{
  setRowCount(40);

  Q_ASSERT(cc());
  cc()->setAutomaticInvocationEnabled(true);
  cc()->registerCompletionModel(this);
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
          return m_startText + QString("%1%2%3").arg(QChar('a' + (index.row() % 3))).arg(QChar('a' + index.row())).arg(index.row());

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
          return m_startText + QString("%1%2%3").arg(QChar('a' + (index.row() % 3))).arg(QChar('a' + index.row())).arg(index.row());

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
          p |= Const | Public;
          break;
        case 1:
          p |= Protected;
          break;
        case 2:
          p |= Private;
          break;
      }
      return (int)p;
    }

    case ScopeIndex:
      return (index.row() % 4 ) - 1;
  }

  return QVariant();
}

KTextEditor::View* CodeCompletionTest::view( ) const
{
  return static_cast<KTextEditor::View*>(const_cast<QObject*>(QObject::parent()));
}

KTextEditor::CodeCompletionInterface * CodeCompletionTest::cc( ) const
{
  return dynamic_cast<KTextEditor::CodeCompletionInterface*>(const_cast<QObject*>(QObject::parent()));
}

void CodeCompletionTest::completionInvoked(KTextEditor::View* view, const KTextEditor::Range& range, InvocationType invocationType)
{
  Q_UNUSED(invocationType)

  m_startText = view->document()->text(KTextEditor::Range(range.start(), view->cursorPosition()));
  kDebug() << k_funcinfo << m_startText << endl;
}

#include "codecompletiontest.moc"
