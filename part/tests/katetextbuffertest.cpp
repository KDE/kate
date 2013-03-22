/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
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

#include "katetextbuffertest.h"
#include "katetextbuffer.h"
#include "katetextcursor.h"
#include "katetextfolding.h"

QTEST_MAIN(KateTextBufferTest)

KateTextBufferTest::KateTextBufferTest()
  : QObject()
{
}

KateTextBufferTest::~KateTextBufferTest()
{
}

void KateTextBufferTest::basicBufferTest()
{
  // construct an empty text buffer
  Kate::TextBuffer buffer (0, 1);

  // one line per default
  QVERIFY (buffer.lines() == 1);
  QVERIFY (buffer.text () == "");

  //FIXME: use QTestLib macros for checking the correct state
  // start editing
  buffer.startEditing ();

  // end editing
  buffer.finishEditing ();
}

void KateTextBufferTest::wrapLineTest()
{
  // construct an empty text buffer
  Kate::TextBuffer buffer (0, 1);

  // wrap first empty line -> we should have two empty lines
  buffer.startEditing ();
  buffer.wrapLine(KTextEditor::Cursor(0, 0));
  buffer.finishEditing ();
  buffer.debugPrint ("Two empty lines");
  QVERIFY (buffer.text () == "\n");

  // unwrap again -> only one empty line
  buffer.startEditing ();
  buffer.unwrapLine(1);
  buffer.finishEditing ();

  // print debug
  buffer.debugPrint ("Empty Buffer");
  QVERIFY (buffer.text () == "");
}

void KateTextBufferTest::insertRemoveTextTest()
{
  // construct an empty text buffer
  Kate::TextBuffer buffer (0, 1);

  // wrap first line
  buffer.startEditing ();
  buffer.wrapLine (KTextEditor::Cursor (0, 0));
  buffer.finishEditing ();
  buffer.debugPrint ("Two empty lines");
  QVERIFY (buffer.text () == "\n");

  // remember second line
  Kate::TextLine second = buffer.line (1);

  // unwrap second line
  buffer.startEditing ();
  buffer.unwrapLine (1);
  buffer.finishEditing ();
  buffer.debugPrint ("One empty line");
  QVERIFY (buffer.text () == "");

  // second text line should be still there
  //const QString &secondText = second->text ();
  //QVERIFY (secondText == "")

  // insert text
  buffer.startEditing ();
  buffer.insertText (KTextEditor::Cursor (0, 0), "testremovetext");
  buffer.finishEditing ();
  buffer.debugPrint ("One line");
  QVERIFY (buffer.text () == "testremovetext");

  // remove text
  buffer.startEditing ();
  buffer.removeText (KTextEditor::Range (KTextEditor::Cursor (0, 4), KTextEditor::Cursor (0, 10)));
  buffer.finishEditing ();
  buffer.debugPrint ("One line");
  QVERIFY (buffer.text () == "testtext");

  // wrap text
  buffer.startEditing ();
  buffer.wrapLine (KTextEditor::Cursor (0, 2));
  buffer.finishEditing ();
  buffer.debugPrint ("Two line");
  QVERIFY (buffer.text () == "te\nsttext");

  // unwrap text
  buffer.startEditing ();
  buffer.unwrapLine (1);
  buffer.finishEditing ();
  buffer.debugPrint ("One line");
  QVERIFY (buffer.text () == "testtext");
}

void KateTextBufferTest::cursorTest()
{
  // last buffer content, for consistence checks
  QString lastBufferContent;

  // test with different block sizes
  for (int i = 1; i <= 4; ++i) {
    // construct an empty text buffer
    Kate::TextBuffer buffer (0, i);

    // wrap first line
    buffer.startEditing ();
    buffer.insertText (KTextEditor::Cursor (0, 0), "sfdfjdsklfjlsdfjlsdkfjskldfjklsdfjklsdjkfl");
    buffer.wrapLine (KTextEditor::Cursor (0, 8));
    buffer.wrapLine (KTextEditor::Cursor (1, 8));
    buffer.wrapLine (KTextEditor::Cursor (2, 8));
    buffer.finishEditing ();
    buffer.debugPrint ("Cursor buffer");

    // construct cursor
    Kate::TextCursor *cursor1 = new Kate::TextCursor (buffer, KTextEditor::Cursor (0, 0), Kate::TextCursor::MoveOnInsert);
    QVERIFY (cursor1->toCursor () == KTextEditor::Cursor (0, 0));
    printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());

    Kate::TextCursor *cursor2 = new Kate::TextCursor (buffer, KTextEditor::Cursor (1, 8), Kate::TextCursor::MoveOnInsert);
    printf ("cursor %d, %d\n", cursor2->line(), cursor2->column());

    Kate::TextCursor *cursor3 = new Kate::TextCursor (buffer, KTextEditor::Cursor (0, 123), Kate::TextCursor::MoveOnInsert);
    printf ("cursor %d, %d\n", cursor3->line(), cursor3->column());

    Kate::TextCursor *cursor4 = new Kate::TextCursor (buffer, KTextEditor::Cursor (1323, 1), Kate::TextCursor::MoveOnInsert);
    printf ("cursor %d, %d\n", cursor4->line(), cursor4->column());

    // insert text
    buffer.startEditing ();
    buffer.insertText (KTextEditor::Cursor (0, 0), "hallo");
    buffer.finishEditing ();
    buffer.debugPrint ("Cursor buffer");

    printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
    QVERIFY (cursor1->toCursor () == KTextEditor::Cursor (0, 5));

    // remove text
    buffer.startEditing ();
    buffer.removeText (KTextEditor::Range (KTextEditor::Cursor (0, 4), KTextEditor::Cursor (0, 10)));
    buffer.finishEditing ();
    buffer.debugPrint ("Cursor buffer");

    printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
    QVERIFY (cursor1->toCursor () == KTextEditor::Cursor (0, 4));

    // wrap line
    buffer.startEditing ();
    buffer.wrapLine (KTextEditor::Cursor (0, 3));
    buffer.finishEditing ();
    buffer.debugPrint ("Cursor buffer");

    printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
    QVERIFY (cursor1->toCursor () == KTextEditor::Cursor (1, 1));

    // unwrap line
    buffer.startEditing ();
    buffer.unwrapLine (1);
    buffer.finishEditing ();
    buffer.debugPrint ("Cursor buffer");

    printf ("cursor %d, %d\n", cursor1->line(), cursor1->column());
    QVERIFY (cursor1->toCursor () == KTextEditor::Cursor (0, 4));

    // verify content
    if (i > 1)
      QVERIFY (lastBufferContent == buffer.text ());

    // remember content
    lastBufferContent = buffer.text ();
  }
}

void KateTextBufferTest::foldingTest()
{
    // construct an empty text buffer & folding info
    Kate::TextBuffer buffer (0, 1);
    Kate::TextFolding folding (buffer);
    
    // insert some text
    buffer.startEditing ();
    for (int i = 0; i < 100; ++i) {
      buffer.insertText (KTextEditor::Cursor (i, 0), "1234567890");
      buffer.wrapLine (KTextEditor::Cursor (i, 10));
    }
    buffer.finishEditing ();
    
    // starting with empty folding!
    folding.debugPrint ("Empty Folding");
    QVERIFY (folding.debugDump() == "");
    
    // we shall be able to insert new range
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (0,0), KTextEditor::Cursor (10,0)), Kate::TextFolding::Folded));
    
    // we shall have now exactly one range toplevel
    folding.debugPrint ("One Toplevel Fold");
    QVERIFY (folding.debugDump() == "[0:0 f 10:0]");
    
    // we shall be able to insert new range
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (20,0), KTextEditor::Cursor (30,0)), Kate::TextFolding::Folded));
    
    // we shall have now exactly two range toplevel
    folding.debugPrint ("Two Toplevel Folds");
    QVERIFY (folding.debugDump() == "[0:0 f 10:0] [20:0 f 30:0]");
    
    // this shall fail to be inserted, as it badly overlaps with the first range!
    QVERIFY (!folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (5,0), KTextEditor::Cursor (15,0)), Kate::TextFolding::Folded));
    
    // this shall fail to be inserted, as it badly overlaps with the second range!
    QVERIFY (!folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (15,0), KTextEditor::Cursor (25,0)), Kate::TextFolding::Folded));
    
    // we shall still have now exactly two range toplevel
    folding.debugPrint ("Still Two Toplevel Folds");
    QVERIFY (folding.debugDump() == "[0:0 f 10:0] [20:0 f 30:0]");
    
    // we shall be able to insert new range, should lead to nested folds!
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (15,0), KTextEditor::Cursor (35,0)), Kate::TextFolding::Folded));
    
    // we shall have now exactly two range toplevel and one embedded fold
    folding.debugPrint ("Two Toplevel Folds, One Nested Fold");
    QVERIFY (folding.debugDump() == "[0:0 f 10:0] [15:0 f [20:0 f 30:0] 35:0]");
}
