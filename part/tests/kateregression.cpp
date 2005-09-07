/* This file is part of the KDE libraries
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#define private public
#define protected public

#include "kateregression.h"

#include "kateglobal.h"
#include "katedocument.h"
#include "katesmartmanager.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <klocale.h>

#include <QtTest/qttest_kde.h>

QTTEST_KDEMAIN( KateRegression, NoGUI )

// TODO split it various functions
void KateRegression::testAll()
{
  kdDebug() << k_funcinfo << endl;
  m_doc = dynamic_cast<KateDocument*>(KateGlobal::self()->createDocument(this));
  VERIFY(m_doc);

  // Multi-line insert
  KTextEditor::SmartCursor* cursor1 = m_doc->newSmartCursor(KTextEditor::Cursor(), true);
  KTextEditor::SmartCursor* cursor2 = m_doc->newSmartCursor(KTextEditor::Cursor(), false);

  m_doc->insertText(KTextEditor::Cursor(), "Test Text\nMore Test Text");
  COMPARE(m_doc->end().line(), 1);
  COMPARE(m_doc->end().column(), 14);
  QString text = m_doc->text(KTextEditor::Range(1,0,1,14));
  COMPARE(text, QString("More Test Text"));

  // Check cursors and ranges have moved properly
  COMPARE(*cursor1 == KTextEditor::Cursor(0,0), true);
  COMPARE(*cursor2 == KTextEditor::Cursor(1,14), true);

  // Intra-line insert
  KTextEditor::SmartCursor* cursorStartOfLine = m_doc->newSmartCursor(KTextEditor::Cursor(1,0));

  KTextEditor::SmartCursor* cursorStartOfEdit = m_doc->newSmartCursor(KTextEditor::Cursor(1,5), false);
  KTextEditor::SmartCursor* cursorEndOfEdit = m_doc->newSmartCursor(KTextEditor::Cursor(1,5), true);

  KTextEditor::SmartCursor* cursorPastEdit = m_doc->newSmartCursor(KTextEditor::Cursor(1,7));
  KTextEditor::SmartCursor* cursorEOL = m_doc->newSmartCursor(m_doc->endOfLine(1), false);
  KTextEditor::SmartCursor* cursorEOLMoves = m_doc->newSmartCursor(m_doc->endOfLine(1), true);

  m_doc->insertText(*cursorStartOfEdit, "Additional ");
  COMPARE(*cursorStartOfLine == KTextEditor::Cursor(1,0), true);
  COMPARE(*cursorStartOfEdit == KTextEditor::Cursor(1,5), true);
  COMPARE(*cursorEndOfEdit == KTextEditor::Cursor(1,16), true);
  COMPARE(*cursorPastEdit == KTextEditor::Cursor(1,18), true);
  COMPARE(*cursorEOL == m_doc->endOfLine(1), true);
  COMPARE(*cursorEOLMoves == m_doc->endOfLine(1), true);

  KTextEditor::Cursor oldEOL = *cursorEOL;

  // Insert at EOL
  m_doc->insertText(m_doc->endOfLine(1), " Even More");
  COMPARE(*cursorEOL == oldEOL, true);
  COMPARE(*cursorEOLMoves == m_doc->endOfLine(1), true);

  *cursorEOL = *cursorEOLMoves;

  m_doc->insertText(m_doc->endOfLine(1), "\n");
  COMPARE(*cursorEOL == m_doc->endOfLine(1), true);
  COMPARE(*cursorEOLMoves == KTextEditor::Cursor(2, 0), true);

  checkSmartManager();
  m_doc->smartManager()->debugOutput();
}

void KateRegression::checkSmartManager()
{
  KateSmartGroup* currentGroup = m_doc->smartManager()->groupForLine(0);
  COMPARE(!currentGroup, false);
  forever {
    if (!currentGroup->previous())
      COMPARE(currentGroup->startLine(), 0);

    foreach (KateSmartCursor* cursor, currentGroup->feedbackCursors()) {
      COMPARE(currentGroup->containsLine(cursor->line()), true);
      COMPARE(cursor->m_smartGroup, currentGroup);
    }

    if (!currentGroup->next())
      break;

    COMPARE(currentGroup->endLine(), currentGroup->next()->startLine() - 1);
    COMPARE(currentGroup->next()->previous(), currentGroup);
  }

  COMPARE(currentGroup->endLine(), m_doc->lines() - 1);
}


#include "kateregression.moc"
