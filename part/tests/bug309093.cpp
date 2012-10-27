/* This file is part of the KDE libraries
   Copyright (C) 2012 Dominik Haumann <dhaumann kde org>

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

#include "bug309093.h"
#include "moc_bug309093.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <katecodefolding.h>
#include <katebuffer.h>
#include <kateglobal.h>
#include <kateview.h>
#include <kateconfig.h>
#include <katesearchbar.h>

QTEST_KDEMAIN(BugTest, GUI)

using namespace KTextEditor;

BugTest::BugTest()
  : QObject()
{
}

BugTest::~BugTest()
{
}

void BugTest::initTestCase()
{
  KateGlobal::self()->incRef();
}

void BugTest::cleanupTestCase()
{
    KateGlobal::self()->decRef();
}

void BugTest::tryCppFoldingCrash()
{
  KateDocument doc(false, false, false);
  QString url = KDESRCDIR + QString("bug309093_example.cpp");
  doc.openUrl(url);
  doc.discardDataRecovery();
  doc.setHighlightingMode("C++");
  doc.buffer().ensureHighlighted (doc.lines());

  // view must be visible...
  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->show();
  view->resize(400, 300);
  view->setSelection(Range(Cursor(93, 0), Cursor(96, 11)));
  view->setCursorPosition(Cursor(96, 11));

  QTest::qWait(10);
  view->backspace();
  QTest::qWait(50);
  qDebug() << "!!! No crash (qWait not long enough)? Nice!";
}
