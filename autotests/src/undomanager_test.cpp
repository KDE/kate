/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "undomanager_test.h"

#include <katedocument.h>
#include <kateview.h>
#include <kateundomanager.h>

#include <QtTestWidgets>

QTEST_MAIN(UndoManagerTest)

using namespace KTextEditor;

class UndoManagerTest::TestDocument : public KateDocument
{
public:
  TestDocument()
    : KateDocument(false, false, false, 0, 0)
  {}
};

void UndoManagerTest::testUndoRedoCount()
{
  TestDocument doc;
  KateUndoManager *undoManager = doc.undoManager();

  // no undo/redo items at the beginning
  QCOMPARE(undoManager->undoCount(), 0u);
  QCOMPARE(undoManager->redoCount(), 0u);

  doc.insertText(Cursor(0, 0), QLatin1String("a"));

  // create one insert-group
  QCOMPARE(undoManager->undoCount(), 1u);
  QCOMPARE(undoManager->redoCount(), 0u);

  doc.undo();

  // move insert-group to redo items
  QCOMPARE(undoManager->undoCount(), 0u);
  QCOMPARE(undoManager->redoCount(), 1u);

  doc.redo();

  // move insert-group back to undo items
  QCOMPARE(undoManager->undoCount(), 1u);
  QCOMPARE(undoManager->redoCount(), 0u);

  doc.insertText(Cursor(0, 1), QLatin1String("b"));

  // merge "b" into insert-group
  QCOMPARE(undoManager->undoCount(), 1u);
  QCOMPARE(undoManager->redoCount(), 0u);

  doc.removeText(Range(0, 1, 0, 2));

  // create an additional remove-group
  QCOMPARE(undoManager->undoCount(), 2u);
  QCOMPARE(undoManager->redoCount(), 0u);

  doc.undo();

  // move remove-group to redo items
  QCOMPARE(undoManager->undoCount(), 1u);
  QCOMPARE(undoManager->redoCount(), 1u);

  doc.insertText(Cursor(0, 1), QLatin1String("b"));

  // merge "b" into insert-group
  // and remove remove-group
  QCOMPARE(undoManager->undoCount(), 1u);
  QCOMPARE(undoManager->redoCount(), 0u);
}

void UndoManagerTest::testSafePoint()
{
  TestDocument doc;
  KateUndoManager *undoManager = doc.undoManager();

  doc.insertText(Cursor(0, 0), QLatin1String("a"));

  // create one undo group
  QCOMPARE(undoManager->undoCount(), 1u);
  QCOMPARE(undoManager->redoCount(), 0u);

  undoManager->undoSafePoint();
  doc.insertText(Cursor(0, 1), QLatin1String("b"));

  // create a second undo group (don't merge)
  QCOMPARE(undoManager->undoCount(), 2u);

  doc.undo();

  // move second undo group to redo items
  QCOMPARE(undoManager->undoCount(), 1u);
  QCOMPARE(undoManager->redoCount(), 1u);

  doc.insertText(Cursor(0, 1), QLatin1String("b"));

  // create a second undo group again, (don't merge)
  QCOMPARE(undoManager->undoCount(), 2u);
  QCOMPARE(undoManager->redoCount(), 0u);

  doc.editStart();
  doc.insertText(Cursor(0, 2), QLatin1String("c"));
  undoManager->undoSafePoint();
  doc.insertText(Cursor(0, 3), QLatin1String("d"));
  doc.editEnd();

  // merge both edits into second undo group
  QCOMPARE(undoManager->undoCount(), 2u);
  QCOMPARE(undoManager->redoCount(), 0u);

  doc.insertText(Cursor(0, 4), QLatin1String("e"));

  // create a third undo group (don't merge)
  QCOMPARE(undoManager->undoCount(), 3u);
  QCOMPARE(undoManager->redoCount(), 0u);
}

void UndoManagerTest::testCursorPosition()
{
  TestDocument doc;
  KateView *view = static_cast<KateView*>(doc.createView(0));

  doc.setText(QLatin1String("aaaa bbbb cccc\n"
              "dddd  ffff"));
  view->setCursorPosition(KTextEditor::Cursor(1, 5));

  doc.typeChars(view, QLatin1String("eeee"));

  // cursor position: "dddd eeee| ffff"
  QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 9));

  // undo once to remove "eeee", cursor position: "dddd | ffff"
  doc.undo();
  QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 5));

  // redo once to insert "eeee" again. cursor position: "dddd eeee| ffff"
  doc.redo();
  QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 9));

  delete view;
}

void UndoManagerTest::testSelectionUndo()
{
  TestDocument doc;
  KateView *view = static_cast<KateView*>(doc.createView(0));

  doc.setText(QLatin1String("aaaa bbbb cccc\n"
              "dddd eeee ffff"));
  view->setCursorPosition(KTextEditor::Cursor(1, 9));
  KTextEditor::Range selectionRange(KTextEditor::Cursor(0, 5),
                                    KTextEditor::Cursor(1, 9));
  view->setSelection(selectionRange);

  doc.typeChars(view, QLatin1String(QLatin1String("eeee")));

  // cursor position: "aaaa eeee| ffff", no selection anymore
  QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, 9));
  QCOMPARE(view->selection(), false);

  // undo to remove "eeee" and add selection and text again
  doc.undo();
  QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 9));
  QCOMPARE(view->selection(), true);
  QCOMPARE(view->selectionRange(), selectionRange);

  // redo to insert "eeee" again and remove selection
  // cursor position: "aaaa eeee| ffff", no selection anymore
  doc.redo();
  QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, 9));
  QCOMPARE(view->selection(), false);

  delete view;
}

void UndoManagerTest::testUndoWordWrapBug301367()
{
  TestDocument doc;
  doc.setWordWrap(true);
  doc.setWordWrapAt(20);
  KateView *view = static_cast<KateView*>(doc.createView(0));

  QString text = QString::fromLatin1("1234 1234 1234 1234\n"
                 "1234 1234 1234 1234");

  doc.setText(text);
  view->setCursorPosition(KTextEditor::Cursor(0, 0));

  doc.typeChars(view, QLatin1String("           "));

  while (doc.undoCount() > 1)
    doc.undo();

  // test must be exactly the same as before
  QCOMPARE(doc.text (), text);

  while (doc.redoCount() > 1)
    doc.redo();

  while (doc.undoCount() > 1)
    doc.undo();

  // test must be exactly the same as before
  QCOMPARE(doc.text (), text);

  delete view;
}

#include "moc_undomanager_test.cpp"

// kate: space-indent on; indent-width 2; replace-tabs on;
