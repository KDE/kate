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

#include "bug294241.h"
#include "moc_bug294241.cpp"

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

void BugTest::tryXmlCrash()
{
  KateDocument doc(false, false, false);
  QString url = KDESRCDIR + QString("bug294241.xml");
  doc.openUrl(url);
  doc.discardDataRecovery();
  doc.setHighlightingMode("XML");
  doc.buffer().ensureHighlighted (doc.lines());

  // view must be visible...
  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->show();
  view->resize(400, 300);
  view->setCursorPosition(Cursor(502, 1));

  doc.typeChars(view, " ");
  doc.buffer().ensureHighlighted (doc.lines());
  qDebug() << "!!! The next line usually crashes in the code folding code";

  QTest::qWait(2000);
  doc.undo();
  doc.buffer().ensureHighlighted (doc.lines());
  
  QTest::qWait(2000);
  qDebug() << "!!! No crash (qWait not long enough)? Nice!";
}

void BugTest::tryPhpCrash()
{
  KateDocument doc(false, false, false);
  QString url = KDESRCDIR + QString("bug294241.php");
  doc.openUrl(url);
  doc.discardDataRecovery();
  doc.setHighlightingMode("PHP/PHP");
  doc.buffer().ensureHighlighted (doc.lines());

  // view must be visible...
  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->show();
  view->resize(400, 300);
  view->setCursorPosition(Cursor(22, 25));
  QTest::qWait(2000);

  qDebug() << "!!! The next line usually crashes in the code folding code";
  doc.typeChars(view, "h");
  doc.buffer().ensureHighlighted (doc.lines());
  QTest::qWait(1000);
  doc.typeChars(view, "o");
  doc.buffer().ensureHighlighted (doc.lines());
  QTest::qWait(1000);
  doc.typeChars(view, "?");
  doc.buffer().ensureHighlighted (doc.lines());
  QTest::qWait(1000);
  doc.typeChars(view, ">");
  doc.buffer().ensureHighlighted (doc.lines());
  QTest::qWait(2000);

  qDebug() << "!!! No crash (qWait not long enough)? Nice!";
}

void BugTest::tryRubyCrash()
{
  int i = 0;
  KateDocument doc(false, false, false);
  QString url = KDESRCDIR + QString("bug294241.rb");
  doc.openUrl(url);
  doc.discardDataRecovery();
  doc.setHighlightingMode("Ruby");
  doc.buffer().ensureHighlighted (doc.lines());

  // view must be visible...
  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->show();
  view->resize(400, 300);

  // Select ruby code and uncomment it
  view->setSelection(Range(6, 0, 26, 0));
  QTest::qWait(500);
  view->toggleComment();
  QTest::qWait(500);

  // Now unindent
  for (i = 0; i < 5; i++)
    view->unIndent();
  QTest::qWait(2000);

  view->setCursorPosition(Cursor(5, 0));
  view->setSelection(Range());
  QTest::qWait(2000);

  for (i = 4; i >= 0; --i) {
    view->setCursorPosition(Cursor(i, 0));
    view->keyDelete();
  }
  view->keyDelete();
  QTest::qWait(1000);

  view->keyReturn();
  view->keyReturn();
  QTest::qWait(2000);

  qDebug() << "!!! The next line usually crashes in the code folding code";
  doc.undo();
  QTest::qWait(2000);

  qDebug() << "!!! No crash (qWait not long enough)? Nice!";
}




