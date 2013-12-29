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
      if (i < 99)
        buffer.wrapLine (KTextEditor::Cursor (i, 10));
    }
    buffer.finishEditing ();
    QVERIFY (buffer.lines() == 100);
    
    // starting with empty folding!
    folding.debugPrint ("Empty Folding");
    QVERIFY (folding.debugDump() == "tree  - folded ");
    
    // check visibility
    QVERIFY (folding.isLineVisible (0));
    QVERIFY (folding.isLineVisible (99));
    
    // all visible
    QVERIFY (folding.visibleLines() == 100);
    
    // we shall be able to insert new range
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (5,0), KTextEditor::Cursor (10,0))) == 0);
    
    // we shall have now exactly one range toplevel, that is not folded!
    folding.debugPrint ("One Toplevel Fold");
    QVERIFY (folding.debugDump() == "tree [5:0  10:0] - folded ");
    
    // fold the range!
    QVERIFY(folding.foldRange (0));
    
    folding.debugPrint ("One Toplevel Fold - Folded");
    QVERIFY (folding.debugDump() == "tree [5:0 f 10:0] - folded [5:0 f 10:0]");
    
    // check visibility
    QVERIFY (folding.isLineVisible (5));
    for (int i = 6; i <= 10; ++i)
      QVERIFY (!folding.isLineVisible (i));
    QVERIFY (folding.isLineVisible (11));
    
    // 5 lines are hidden
    QVERIFY (folding.visibleLines() == (100 - 5));
    
    // check line mapping
    QVERIFY (folding.visibleLineToLine (5) == 5);
    for (int i = 6; i <= 50; ++i)
      QVERIFY (folding.visibleLineToLine (i) == (i + 5));
    
    // there shall be one range starting at 5
    QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags> > forLine = folding.foldingRangesStartingOnLine (5);
    QVERIFY (forLine.size() == 1);
    QVERIFY (forLine[0].first == 0);
    QVERIFY (forLine[0].second & Kate::TextFolding::Folded);
    
    // we shall be able to insert new range
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (20,0), KTextEditor::Cursor (30,0)), Kate::TextFolding::Folded) == 1);
    
    // we shall have now exactly two range toplevel
    folding.debugPrint ("Two Toplevel Folds");
    QVERIFY (folding.debugDump() == "tree [5:0 f 10:0] [20:0 f 30:0] - folded [5:0 f 10:0] [20:0 f 30:0]");
    
    // check visibility
    QVERIFY (folding.isLineVisible (5));
    for (int i = 6; i <= 10; ++i)
      QVERIFY (!folding.isLineVisible (i));
    QVERIFY (folding.isLineVisible (11));
    
    QVERIFY (folding.isLineVisible (20));
    for (int i = 21; i <= 30; ++i)
      QVERIFY (!folding.isLineVisible (i));
    QVERIFY (folding.isLineVisible (31));
    
    // 15 lines are hidden
    QVERIFY (folding.visibleLines() == (100 - 5 - 10));
    
    // check line mapping
    QVERIFY (folding.visibleLineToLine (5) == 5);
    for (int i = 6; i <= 15; ++i)
      QVERIFY (folding.visibleLineToLine (i) == (i + 5));
    for (int i = 16; i <= 50; ++i)
      QVERIFY (folding.visibleLineToLine (i) == (i + 15));
    
    // check line mapping
    QVERIFY (folding.lineToVisibleLine (5) == 5);
    for (int i = 11; i <= 20; ++i)
      QVERIFY (folding.lineToVisibleLine (i) == (i - 5));
    for (int i = 31; i <= 40; ++i)
      QVERIFY (folding.lineToVisibleLine (i) == (i - 15));
    
    // there shall be one range starting at 20
    forLine = folding.foldingRangesStartingOnLine (20);
    QVERIFY (forLine.size() == 1);
    QVERIFY (forLine[0].first == 1);
    QVERIFY (forLine[0].second & Kate::TextFolding::Folded);
    
    // this shall fail to be inserted, as it badly overlaps with the first range!
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (6,0), KTextEditor::Cursor (15,0)), Kate::TextFolding::Folded) == -1);
    
    // this shall fail to be inserted, as it badly overlaps with the second range!
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (15,0), KTextEditor::Cursor (25,0)), Kate::TextFolding::Folded) == -1);
    
    // we shall still have now exactly two range toplevel
    folding.debugPrint ("Still Two Toplevel Folds");
    QVERIFY (folding.debugDump() == "tree [5:0 f 10:0] [20:0 f 30:0] - folded [5:0 f 10:0] [20:0 f 30:0]");
    
    // still 15 lines are hidden
    QVERIFY (folding.visibleLines() == (100 - 5 - 10));
    
    // we shall be able to insert new range, should lead to nested folds!
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (15,0), KTextEditor::Cursor (35,0)), Kate::TextFolding::Folded) == 2);
    
    // we shall have now exactly two range toplevel and one embedded fold
    folding.debugPrint ("Two Toplevel Folds, One Nested Fold");
    QVERIFY (folding.debugDump() == "tree [5:0 f 10:0] [15:0 f [20:0 f 30:0] 35:0] - folded [5:0 f 10:0] [15:0 f 35:0]");
    
    // 25 lines are hidden
    QVERIFY (folding.visibleLines() == (100 - 5 - 20));
    
    // check line mapping
    QVERIFY (folding.lineToVisibleLine (5) == 5);
    for (int i = 11; i <= 15; ++i)
      QVERIFY (folding.lineToVisibleLine (i) == (i - 5));
    
    // special case: hidden lines, should fall ack to last visible one!
    for (int i = 16; i <= 35; ++i)
      QVERIFY (folding.lineToVisibleLine (i) == 10);
    
    for (int i = 36; i <= 40; ++i)
      QVERIFY (folding.lineToVisibleLine (i) == (i - 25));
    
    // we shall be able to insert new range, should lead to nested folds!
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (0,0), KTextEditor::Cursor (50,0)), Kate::TextFolding::Folded) == 3);
    
    // we shall have now exactly one range toplevel and many embedded fold
    folding.debugPrint ("One Toplevel + Embedded Folds");
    QVERIFY (folding.debugDump() == "tree [0:0 f [5:0 f 10:0] [15:0 f [20:0 f 30:0] 35:0] 50:0] - folded [0:0 f 50:0]");
    
    // there shall still be one range starting at 20
    forLine = folding.foldingRangesStartingOnLine (20);
    QVERIFY (forLine.size() == 1);
    QVERIFY (forLine[0].first == 1);
    QVERIFY (forLine[0].second & Kate::TextFolding::Folded);
    
    // add more regions starting at 20
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (20,5), KTextEditor::Cursor (24,0)), Kate::TextFolding::Folded) == 4);
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (20,3), KTextEditor::Cursor (25,0)), Kate::TextFolding::Folded) == 5);
    folding.debugPrint ("More ranges at 20");
    
    // there shall still be three ranges starting at 20
    forLine = folding.foldingRangesStartingOnLine (20);
    QVERIFY (forLine.size() == 3);
    QVERIFY (forLine[0].first == 1);
    QVERIFY (forLine[0].second & Kate::TextFolding::Folded);
    QVERIFY (forLine[1].first == 5);
    QVERIFY (forLine[1].second & Kate::TextFolding::Folded);
    QVERIFY (forLine[2].first == 4);
    QVERIFY (forLine[2].second & Kate::TextFolding::Folded);
    
    // 50 lines are hidden
    QVERIFY (folding.visibleLines() == (100 - 50));
    
    // save state
    QVariantList folds = folding.exportFoldingRanges ();
    QString textDump = folding.debugDump();
    
    // clear folds
    folding.clear ();
    QVERIFY (folding.debugDump() == "tree  - folded ");
    
    // restore state
    folding.importFoldingRanges (folds);
    QVERIFY (folding.debugDump() == textDump);
    
}

void KateTextBufferTest::nestedFoldingTest()
{
    // construct an empty text buffer & folding info
    Kate::TextBuffer buffer (0, 1);
    Kate::TextFolding folding (buffer);

    // insert two nested folds in 5 lines
    buffer.startEditing ();
    for (int i = 0; i < 4; ++i)
      buffer.wrapLine(KTextEditor::Cursor(0, 0));
    buffer.finishEditing ();

    QVERIFY (buffer.lines() == 5);

    // folding for line 1
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (0,0), KTextEditor::Cursor (3,0)), Kate::TextFolding::Folded) == 0);
    QVERIFY (folding.newFoldingRange (KTextEditor::Range (KTextEditor::Cursor (1,0), KTextEditor::Cursor (2,0)), Kate::TextFolding::Folded) == 1);

    QVERIFY(folding.foldRange (1));
    QVERIFY(folding.foldRange (0));

    QVERIFY(folding.unfoldRange (0));
    QVERIFY(folding.unfoldRange (1));
}

void KateTextBufferTest::saveFileInUnwritableFolder()
{
  const QString folder_name = QString("katetest_%1").arg(QCoreApplication::applicationPid());
  const QString file_path = QDir::tempPath() + '/' + folder_name + "/foo";
  Q_ASSERT(QDir::temp().mkdir(folder_name));

  QFile f(file_path);
  Q_ASSERT(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  f.write("1234567890");
  Q_ASSERT(f.flush());
  f.close();

  QFile::setPermissions(QDir::tempPath() + '/' + folder_name, QFile::ExeOwner);

  Kate::TextBuffer buffer(0, 1);
  buffer.setTextCodec(QTextCodec::codecForName("UTF-8"));
  buffer.setFallbackTextCodec(QTextCodec::codecForName("UTF-8"));
  bool a, b;
  buffer.load(file_path, a, b, true);
  buffer.clear();
  buffer.startEditing();
  buffer.insertText (KTextEditor::Cursor (0, 0), "ABC");
  buffer.finishEditing();
  qDebug() << buffer.text();
  buffer.save(file_path);

  f.open(QIODevice::ReadOnly);
  QCOMPARE(f.readAll(), QByteArray("ABC"));
  f.close();

  QFile::setPermissions(QDir::tempPath() + '/' + folder_name, QFile::WriteOwner | QFile::ExeOwner);
  Q_ASSERT(f.remove());
  Q_ASSERT(QDir::temp().rmdir(folder_name));
}
