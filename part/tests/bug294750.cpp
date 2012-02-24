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

#include "bug294750.h"
#include "moc_bug294750.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <katecodefolding.h>
#include <katebuffer.h>
#include <kateglobal.h>
#include <kateview.h>
#include <kateconfig.h>
#include <katesearchbar.h>
#include <ktexteditor/movingrange.h>

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

void BugTest::tryCrash()
{
  KateDocument doc(false, false, false);
  QString url = KDESRCDIR + QString("folding-crash.py");
  doc.openUrl(url);
  doc.discardDataRecovery();
  doc.setHighlightingMode("Python");
  doc.buffer().ensureHighlighted (doc.lines());

  // view must be visible...
  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->show();
  view->resize(400, 300);

  // fold all
  doc.foldingTree()->collapseToplevelNodes();
  QTest::qWait(1000);

  view->down();
  view->down();
  view->end();
  view->cursorRight();
  view->down();
  view->down();
  view->end();
  QTest::qWait(1000);

  qDebug() << "!!! The next line usually crashes in the code folding code";
  view->backspace();

  doc.buffer().ensureHighlighted (doc.lines());
  QTest::qWait(1000);
}
