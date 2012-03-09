/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Dominik Haumann <dhaumann kde org>
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

#include "foldedselection_test.h"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>
#include <katebuffer.h>
#include <kateglobal.h>

using namespace KTextEditor;

QTEST_KDEMAIN(KateFoldedSelectionTest, GUI)

void KateFoldedSelectionTest::initTestCase()
{
  KateGlobal::self()->incRef();
}

void KateFoldedSelectionTest::cleanupTestCase()
{
    KateGlobal::self()->decRef();
}

//
// when text is folded, and you set the text selection from top to bottom and
// type a character, the resulting text is borked.
//
// See https://bugs.kde.org/show_bug.cgi?id=295632
//
void KateFoldedSelectionTest::foldedSelectionTest()
{
  KateDocument doc(false, false, false);
  QString text = "oooossssssss\n"
                 "{\n"
                 "\n"
                 "}\n"
                 "ssssss----------";
  doc.setText(text);
  doc.setHighlightingMode("C++");
  doc.buffer().ensureHighlighted (doc.lines());
  
  // view must be visible...
  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->show();
  view->resize(400, 300);
  
  QTest::qWait(500);
  doc.foldingTree()->collapseOne(1, 1);
  QTest::qWait(500);

  view->setSelection(Range(Cursor(0,4), Cursor(4, 6)));
  view->setCursorPosition(Cursor(4, 6));
  
  QTest::qWait(500);
  doc.typeChars(view, "x");
  QTest::qWait(500);
  
  QString line = doc.line(0);
  QCOMPARE(line, QString("oooox----------"));
}
