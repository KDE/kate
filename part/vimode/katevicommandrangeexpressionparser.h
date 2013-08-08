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
#ifndef KATE_VI_COMMAND_RANGE_EXPRESSION_PARSER_INCLUDED
#define KATE_VI_COMMAND_RANGE_EXPRESSION_PARSER_INCLUDED

#include <ktexteditor/range.h>

#include <QtCore/QRegExp>

class KateView;

class CommandRangeExpressionParser
{
public:
  CommandRangeExpressionParser();
  KTextEditor::Range parseRangeExpression(const QString& command, QString& destRangeExpression, QString& destTransformedCommand, KateView* view);
private:
  int calculatePosition( const QString& string, KateView *view );
  QRegExp m_line;
  QRegExp m_lastLine;
  QRegExp m_thisLine;
  QRegExp m_mark;
  QRegExp m_forwardSearch;
  QRegExp m_forwardSearch2;
  QRegExp m_backwardSearch;
  QRegExp m_backwardSearch2;
  QRegExp m_base;
  QRegExp m_offset;
  QRegExp m_position;
  QRegExp m_cmdRange;
};

#endif
