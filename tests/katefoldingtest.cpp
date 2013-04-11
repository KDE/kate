/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>
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

#include "katefoldingtest.h"

#include <qtest_kde.h>

#include <kateglobal.h>
#include <katebuffer.h>
#include <katedocument.h>
#include <kateview.h>
#include <kateconfig.h>

using namespace KTextEditor;

QTEST_KDEMAIN(KateFoldingTest, GUI)

namespace QTest {
  template<>
  char *toString(const KTextEditor::Cursor &cursor)
  {
    QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
                    + ", " + QByteArray::number(cursor.column()) + "]";
    return qstrdup(ba.data());
  }
}

void KateFoldingTest::initTestCase()
{
  KateGlobal::self()->incRef();
}

void KateFoldingTest::cleanupTestCase()
{
    KateGlobal::self()->decRef();
}

// This is a unit test for bug 311866 (http://bugs.kde.org/show_bug.cgi?id=311866)
// It loads 5 lines of C++ code, places the cursor in line 4, and then folds
// the code.
// Expected behavior: the cursor should be moved so it stays visible
// Buggy behavior: the cursor is hidden, and moving the hidden cursor crashes kate
void KateFoldingTest::testCrash311866()
{
  KateDocument doc(false, false, false);
  QString url = KDESRCDIR + QString("data/bug311866.cpp");
  doc.openUrl(url);
  doc.setHighlightingMode("C++");
  doc.buffer().ensureHighlighted (6);

  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->show();
  view->resize(400, 300);
  view->setCursorPosition(Cursor(3, 0));
  QTest::qWait(100);

  view->slotFoldToplevelNodes();
  doc.buffer().ensureHighlighted (6);

  qDebug() << "!!! Does the next line crash?";
  view->up();
}
