/* This file is part of the KDE libraries
   Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

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

#include "bug213964.h"
#include "moc_bug213964.cpp"

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

static const char* text(
"# Instructions\n"
"# 1) Load this file. Do NOT edit anything.\n"
"# 2) Collapse the top level folding (Ctrl+Shift+-)\n"
"# 3) Type \"('\" on line 7 (without the double quotes, with the single quote) -> CRASH\n"
"# 4) Next time around, type the \"('\" on line 30 -> no crash\n"
"# 5) Delete the two chars again, and try step 3, now -> no crash\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"xxx (x xx 2:xxxx(xx3)) {\n"
"  xx3[,x] <- xx2[,x] + (xxx(xx2[,x-1])-xxx(xx2[,x-1]))/2 + xx3[1,x-1] - xx2[1,x-1]\n"
"}\n");

BugTest::BugTest()
  : QObject()
{
}

BugTest::~BugTest()
{
}

void BugTest::reproduceCrash()
{
  KateGlobal::self()->incRef();
  KateDocument doc(false, false, false);
  doc.setText(text);
  doc.setHighlightingMode("R Script");
  doc.buffer().ensureHighlighted (75);
  doc.foldingTree()->collapseToplevelNodes();
  doc.buffer().ensureHighlighted (75);

  doc.insertText(Cursor(6, 0), "(");
  qDebug() << "!!! The next line usually crashes in the code folding code";
  QTest::qWait(500);
  doc.insertText(Cursor(6, 1), "'");
  qDebug() << "!!! Huh, no crash. qWait above not long enough?";
}
