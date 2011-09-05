/* This file is part of the KDE libraries
   Copyright (C) 2011 Dominik Haumann <dhaumann kde org>

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

#include "modificationsystem_test.h"
#include "moc_modificationsystem_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>
#include <kateundomanager.h>
#include <kateglobal.h>

QTEST_KDEMAIN(ModificationSystemTest, GUI)

using namespace KTextEditor;

void ModificationSystemTest::initTestCase()
{
  KateGlobal::self()->incRef();
}

void ModificationSystemTest::cleanupTestCase()
{
    KateGlobal::self()->decRef();
}

static void clearModificationFlags(KateDocument* doc)
{
  for (int i = 0; i < doc->lines(); ++i) {
    Kate::TextLine line = doc->plainKateTextLine(i);
    line->markAsModified(false);
    line->markAsSavedOnDisk(false);
  }
}

static void markModifiedLinesAsSaved(KateDocument* doc)
{
  for (int i = 0; i < doc->lines(); ++i) {
    Kate::TextLine textLine = doc->plainKateTextLine(i);
    if (textLine->markedAsModified())
      textLine->markAsSavedOnDisk(true);
  }
}

void ModificationSystemTest::testInsertText()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("first line\n");
  doc->setText(content);

  Kate::TextLine line0 = doc->plainKateTextLine(0);

  // now all lines should have state "Modified"
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());


  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);

  QVERIFY(!line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());

  // now we have an a unmodified file, start real tests
  // insert text in line 0, then undo and redo
  doc->insertText(Cursor(0, 2), "_");
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!line0->markedAsModified());
  QVERIFY(line0->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!line0->markedAsModified());
  QVERIFY(line0->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!line0->markedAsModified());
  QVERIFY(line0->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testRemoveText()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("first line\n");
  doc->setText(content);

  Kate::TextLine line0 = doc->plainKateTextLine(0);

  // now all lines should have state "Modified"
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());


  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);

  QVERIFY(!line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());

  // now we have an a unmodified file, start real tests
  // remove text in line 0, then undo and redo
  doc->removeText(Range(Cursor(0, 1), Cursor(0, 2)));
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!line0->markedAsModified());
  QVERIFY(line0->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!line0->markedAsModified());
  QVERIFY(line0->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(line0->markedAsModified());
  QVERIFY(!line0->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!line0->markedAsModified());
  QVERIFY(line0->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testInsertLine()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("0\n"
                        "2");
  doc->setText(content);

  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);

  // insert at line 1
  doc->insertLine(1, "1");

  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testRemoveLine()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("0\n"
                        "1\n"
                        "2");
  doc->setText(content);

  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);

  // remove at line 1
  doc->removeLine(1);

  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testWrapLineMid()
{
  for (int i = 0; i < 2; ++i) {
    bool insertNewLine = (i == 1);
    KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

    const QString content("aaaa\n"
                          "bbbb\n"
                          "cccc");
    doc->setText(content);

    // clear all modification flags, forces no flags
    doc->setModified(false);
    doc->undoManager()->updateLineModifications();
    clearModificationFlags(doc);
    
    // wrap line 1 at |: bb|bb
    doc->editWrapLine(1, 2, insertNewLine);

    QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
    QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
    QVERIFY(doc->plainKateTextLine(2)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
    QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
    QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

    doc->undo();
    QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
    QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());

    doc->redo();
    QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
    QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
    QVERIFY(doc->plainKateTextLine(2)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
    QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
    QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());


    //
    // now simulate "save", then do the undo/redo tests again
    //
    doc->setModified(false);
    markModifiedLinesAsSaved(doc);
    doc->undoManager()->updateLineModifications();

    // now no line should have state "Modified"
    QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
    QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
    QVERIFY(doc->plainKateTextLine(2)->markedAsSavedOnDisk());

    // undo the text insertion
    doc->undo();
    QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
    QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
    QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

    doc->redo();
    QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
    QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
    QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
    QVERIFY(doc->plainKateTextLine(2)->markedAsSavedOnDisk());

    delete doc;
  }
}

void ModificationSystemTest::testWrapLineAtEnd()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("aaaa\n"
                        "bbbb");
  doc->setText(content);

  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);
  
  // wrap line 0 at end
  doc->editWrapLine(0, 4);

  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testWrapLineAtStart()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("aaaa\n"
                        "bbbb");
  doc->setText(content);

  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);
  
  // wrap line 0 at end
  doc->editWrapLine(0, 0);

  QVERIFY(doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testUnWrapLine()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("aaaa\n"
                        "bbbb\n"
                        "cccc");
  doc->setText(content);

  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);

  // join line 0 and 1
  doc->editUnWrapLine(0);

  QVERIFY(doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testUnWrapLine1Empty()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("aaaa\n"
                        "\n"
                        "bbbb");
  doc->setText(content);

  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);

  // join line 1 and 2
  doc->editUnWrapLine(1);

  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  delete doc;
}

void ModificationSystemTest::testUnWrapLine2Empty()
{
  KateDocument* doc = qobject_cast<KateDocument*>(KateGlobal::self()->createDocument(0));

  const QString content("aaaa\n"
                        "\n"
                        "bbbb");
  doc->setText(content);

  // clear all modification flags, forces no flags
  doc->setModified(false);
  doc->undoManager()->updateLineModifications();
  clearModificationFlags(doc);

  // join line 0 and 1
  doc->editUnWrapLine(0);

  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());


  //
  // now simulate "save", then do the undo/redo tests again
  //
  doc->setModified(false);
  markModifiedLinesAsSaved(doc);
  doc->undoManager()->updateLineModifications();

  // now no line should have state "Modified"
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  // undo the text insertion
  doc->undo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(2)->markedAsSavedOnDisk());

  doc->redo();
  QVERIFY(!doc->plainKateTextLine(0)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsModified());
  QVERIFY(!doc->plainKateTextLine(0)->markedAsSavedOnDisk());
  QVERIFY(!doc->plainKateTextLine(1)->markedAsSavedOnDisk());

  delete doc;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
