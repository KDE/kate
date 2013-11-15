/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 Vegard Øye
 *  Copyright (C) 2013 Simon St James <kdedevel@etotheipiplusone.com>
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
#include "katevicommandrangeexpressionparser.h"

#include <kateview.h>

#include <QtCore/QStringList>

using KTextEditor::Range;
using KTextEditor::Cursor;

namespace
{
  CommandRangeExpressionParser rangeExpressionParser;
}

CommandRangeExpressionParser::CommandRangeExpressionParser()
{
  m_line.setPattern("\\d+");
  m_lastLine.setPattern("\\$");
  m_thisLine.setPattern("\\.");
  m_mark.setPattern("\\'[0-9a-z><\\+\\*\\_]");
  m_forwardSearch.setPattern("/([^/]*)/?");
  m_forwardSearch2.setPattern("/[^/]*/?"); // no group
  m_backwardSearch.setPattern("\\?([^?]*)\\??");
  m_backwardSearch2.setPattern("\\?[^?]*\\??"); // no group
  m_base.setPattern("(?:" + m_mark.pattern() + ")|(?:" +
                        m_line.pattern() + ")|(?:" +
                        m_thisLine.pattern() + ")|(?:" +
                        m_lastLine.pattern() + ")|(?:" +
                        m_forwardSearch2.pattern() + ")|(?:" +
                        m_backwardSearch2.pattern() + ")");
  m_offset.setPattern("[+-](?:" + m_base.pattern() + ")?");

  // The position regexp contains two groups: the base and the offset.
  // The offset may be empty.
  m_position.setPattern("(" + m_base.pattern() + ")((?:" + m_offset.pattern() + ")*)");

  // The range regexp contains seven groups: the first is the start position, the second is
  // the base of the start position, the third is the offset of the start position, the
  // fourth is the end position including a leading comma, the fifth is end position
  // without the comma, the sixth is the base of the end position, and the seventh is the
  // offset of the end position. The third and fourth groups may be empty, and the
  // fifth, sixth and seventh groups are contingent on the fourth group.
  m_cmdRange.setPattern("^(" + m_position.pattern() + ")((?:,(" + m_position.pattern() + "))?)");
}

Range CommandRangeExpressionParser::parseRangeExpression(const QString& command, KateView* view, QString& destRangeExpression, QString& destTransformedCommand)
{
  return rangeExpressionParser.parseRangeExpression(command, destRangeExpression, destTransformedCommand, view);
}

Range CommandRangeExpressionParser::parseRangeExpression(const QString& command, QString& destRangeExpression, QString& destTransformedCommand, KateView* view)
{
  Range parsedRange(0, -1, 0, -1);
  if (command.isEmpty())
  {
    return parsedRange;
  }
  QString commandTmp = command;
  bool leadingRangeWasPercent = false;
  // expand '%' to '1,$' ("all lines") if at the start of the line
  if ( commandTmp.at( 0 ) == '%' ) {
    commandTmp.replace( 0, 1, "1,$" );
    leadingRangeWasPercent = true;
  }
  if (m_cmdRange.indexIn(commandTmp) != -1 && m_cmdRange.matchedLength() > 0) {
    commandTmp.remove(m_cmdRange);

    QString position_string1 = m_cmdRange.capturedTexts().at(1);
    QString position_string2 = m_cmdRange.capturedTexts().at(4);
    int position1 = calculatePosition(position_string1, view);

    int position2;
    if (!position_string2.isEmpty()) {
      // remove the comma
      position_string2 = m_cmdRange.capturedTexts().at(5);
      position2 = calculatePosition(position_string2, view);
    } else {
      position2 = position1;
    }

    // special case: if the command is just a number with an optional +/- prefix, rewrite to "goto"
    if (commandTmp.isEmpty()) {
      commandTmp = QString("goto %1").arg(position1);
    } else {
      parsedRange.setRange(KTextEditor::Range(position1 - 1, 0, position2 - 1, 0));
    }

    destRangeExpression = (leadingRangeWasPercent ? "%" : m_cmdRange.cap(0));
    destTransformedCommand = commandTmp;
  }

  return parsedRange;
}

int CommandRangeExpressionParser::calculatePosition(const QString& string, KateView* view ) {

  int pos = 0;
  QList<bool> operators_list;
  QStringList split = string.split(QRegExp("[-+](?!([+-]|$))"));
  QList<int> values;

  foreach ( QString line, split ) {
    pos += line.size();

    if ( pos < string.size() ) {
      if ( string.at(pos) == '+' ) {
        operators_list.push_back( true );
      } else if ( string.at(pos) == '-' ) {
        operators_list.push_back( false );
      } else {
        Q_ASSERT( false );
      }
    }

    ++pos;

    if ( m_line.exactMatch(line) ) {
      values.push_back( line.toInt() );
    } else if ( m_lastLine.exactMatch(line) ) {
      values.push_back( view->doc()->lines() );
    } else if ( m_thisLine.exactMatch(line) ) {
      values.push_back( view->cursorPosition().line() + 1 );
    } else if ( m_mark.exactMatch(line) ) {
      values.push_back( view->getViInputModeManager()->getMarkPosition(line.at(1)).line() + 1 );
    } else if ( m_forwardSearch.exactMatch(line) ) {
      m_forwardSearch.indexIn(line);
      QString pattern = m_forwardSearch.capturedTexts().at(1);
      int match = view->doc()->searchText( Range( view->cursorPosition(), view->doc()->documentEnd() ),
                                             pattern, KTextEditor::Search::Regex ).first().start().line();
      values.push_back( (match < 0) ? -1 : match + 1 );
    } else if ( m_backwardSearch.exactMatch(line) ) {
      m_backwardSearch.indexIn(line);
      QString pattern = m_backwardSearch.capturedTexts().at(1);
      int match = view->doc()->searchText( Range( Cursor( 0, 0), view->cursorPosition() ),
                                             pattern, KTextEditor::Search::Regex ).first().start().line();
      values.push_back( (match < 0) ? -1 : match + 1 );
    }
  }

  if (values.isEmpty())
  {
    return -1;
  }

  int result = values.at(0);
  for (int i = 0; i < operators_list.size(); ++i) {
    if ( operators_list.at(i) == true ) {
      result += values.at(i + 1);
    } else {
      result -= values.at(i + 1);
    }
  }

  return result;
}

