/* This file is part of the KDE libraries

   Copyright (C) 2011 Kuzmich Svyatoslav

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

#include "vimode_test.h"
#include "moc_vimode_test.cpp"

#include <qtest_kde.h>
#include <kateviinputmodemanager.h>
#include <katedocument.h>
#include <kateview.h>
#include "katevikeyparser.h"

QTEST_KDEMAIN(ViModeTest, GUI)

using namespace KTextEditor;

ViModeTest::ViModeTest() {
  kate_document = new KateDocument(false, false, false, 0, NULL);
  kate_view = new KateView(kate_document, 0);
  vi_input_mode_manager = kate_view->getViInputModeManager();
}


ViModeTest::~ViModeTest() {
  delete kate_document;
  delete kate_view;
}

void ViModeTest::TestPressKey(QString str) {
  QKeyEvent *key_event;
  for (int i = 0; i< str.length(); i++) {

    QString key(str[i]);
    int code = key[0].unicode() - '0' + Qt::Key_0;
    key_event = new QKeyEvent(QEvent::KeyRelease, code, Qt::NoModifier, key);
    vi_input_mode_manager->handleKeypress(key_event);
  }
}

void ViModeTest::NormalModeTest(QString original_text,
    QString command,
    QString expected_text) {

  vi_input_mode_manager = kate_view->resetViInputModeManager();
  kate_document->setText(original_text);
  kate_view->setCursorPosition(Cursor(0,0));
  TestPressKey(command);
  qDebug() << "\nrunning command " << command << " on text \"" << original_text
    << "\"\n";
  QCOMPARE(kate_document->text(), expected_text);

}


void ViModeTest::NormalModeFallingTests()
{  
  vi_input_mode_manager->viEnterNormalMode();

  NormalModeTest("foo{\n}\n", "$d%", "foo\n");
  NormalModeTest("1 2 3\n4 5 6", "ld3w", "1\n4 5 6");
  NormalModeTest("FOO{\nBAR}BAZ", "lllgu%", "FOO{\nbar}BAZ");
  NormalModeTest("FOO\nBAR BAZ", "gu2w", "foo\nbar BAZ");
  NormalModeTest("FOO\nBAR BAZ", "guj", "foo\nbar baz");
  NormalModeTest("FOO\nBAR\nBAZ", "2guu", "foo\nbar\nBAZ");
  NormalModeTest("foo{\nbar}baz", "lllgU%", "foo{\nBAR}baz");
  NormalModeTest("foo\nbar baz", "gU2w", "FOO\nBAR baz");
  NormalModeTest("foo\nbar baz", "gUj", "FOO\nBAR BAZ");
  NormalModeTest("foo\nbar\nbaz", "2gUU", "FOO\nBAR\nbaz");
  NormalModeTest("foo{\nbar\n}", "llly%p", "foo{{\nbar\n}\nbar\n}");
  NormalModeTest("1 2\n2 1", "lld#", "1 \n2 1");

}

void ViModeTest::NormalModeMotionsTest()
{

  vi_input_mode_manager->viEnterNormalMode();

  //Testing "l"
  NormalModeTest("bar", "lx", "br");
  NormalModeTest("bar", "2lx", "ba");
  NormalModeTest("0123456789012345", "13lx", "012345678901245");
  NormalModeTest("bar", "10lx", "ba");

  // Testing "h"
  NormalModeTest("bar", "llhx", "br");
  NormalModeTest("bar", "10l10hx", "ar");
  NormalModeTest("0123456789012345", "13l10hx", "012456789012345");
  NormalModeTest("bar", "ll5hx", "ar");

  // Testing "j"
  NormalModeTest("bar\nbar", "jx", "bar\nar");
  NormalModeTest("bar\nbar", "10jx", "bar\nar");
  NormalModeTest("bar\nbara", "lljx", "bar\nbaa");
  NormalModeTest("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n5\n",
      "13jx",
      "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n1\n2\n\n4\n5\n");

  // Testing "k"
  NormalModeTest("bar\nbar", "jx", "bar\nar");
  NormalModeTest("bar\nbar\nbar", "jj100kx", "ar\nbar\nbar");
  NormalModeTest("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n5\n",
      "13j10kx",
      "0\n1\n2\n\n4\n5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n5\n");

  // Testing "w"
  NormalModeTest("bar", "wx", "ba");
  NormalModeTest("foo bar", "wx", "foo ar");
  NormalModeTest("foo bar", "lwx","foo ar");
  NormalModeTest("quux(foo, bar, baz);", "wxwxwxwx2wx","quuxfoo ar baz;");
  NormalModeTest("foo\nbar\nbaz", "wxwx","foo\nar\naz");

  // Testing "W"
  NormalModeTest("bar", "Wx", "ba");
  NormalModeTest("foo bar", "Wx", "foo ar");
  NormalModeTest("foo bar", "2lWx","foo ar");
  NormalModeTest("quux(foo, bar, baz);", "WxWx","quux(foo, ar, az);");
  NormalModeTest("foo\nbar\nbaz", "WxWx","foo\nar\naz");

  // Testing "b"
  NormalModeTest("bar", "lbx", "ar");
  NormalModeTest("foo bar baz", "2wbx", "foo ar baz");
  NormalModeTest("foo bar", "w20bx","oo bar");
  NormalModeTest("quux(foo, bar, baz);", "2W4l2bx2bx","quux(foo, ar, az);");
  NormalModeTest("foo\nbar\nbaz", "WWbx","foo\nar\nbaz");

  // Testing "B"
  NormalModeTest("bar", "lBx", "ar");
  NormalModeTest("foo bar baz", "2wBx", "foo ar baz");
  NormalModeTest("foo bar", "w20Bx","oo bar");
  NormalModeTest("quux(foo, bar, baz);", "2W4lBBx","quux(foo, ar, baz);");
  NormalModeTest("foo\nbar", "WlBx","foo\nar");

  // Testing "e"
  NormalModeTest("quux(foo, bar, baz);", "exex2ex10ex","quu(fo, bar baz)");

  // Testing "E"
  NormalModeTest("quux(foo, bar, baz);", "ExEx10Ex","quux(foo bar baz)");

  // Testing "$"
  NormalModeTest("foo\nbar\nbaz", "$x3$x","fo\nbar\nba");

  // Testing "0"
  NormalModeTest(" foo", "$0x","foo");

  // Testing "#"
  NormalModeTest("1 1 1", "2#x","1 1 ");
  NormalModeTest("foo bar foo bar", "#xlll#x","foo ar oo bar");
  NormalModeTest("(foo (bar (foo( bar))))", "#xll#x","(foo (ar (oo( bar))))");

  // Testing "*"
  NormalModeTest("(foo (bar (foo( bar))))", "*x","(foo (bar (oo( bar))))");

  // Testing "-"
  NormalModeTest("0\n1\n2\n3\n4\n5", "5j-x2-x", "0\n1\n\n3\n\n5");

  // Testing "^"
  NormalModeTest(" foo bar", "$^x", " oo bar");

  // Testing "gg"
  NormalModeTest("1\n2\n3\n4\n5", "4jggx", "\n2\n3\n4\n5");

  // Testing "G"
  NormalModeTest("1\n2\n3\n4\n5", "Gx", "1\n2\n3\n4\n");

  // Testing "ge"
  NormalModeTest("quux(foo, bar, baz);", "9lgexgex$gex", "quux(fo bar, ba);");

  // Testing "gE"
  NormalModeTest("quux(foo, bar, baz);", "9lgExgEx$gEx", "quux(fo bar baz);");

  // Testing "|"
  NormalModeTest("123456789", "3|rx4|rx8|rx1|rx", "x2xx567x9");

  // Testing "`"
  NormalModeTest("foo\nbar\nbaz", "lmaj`arx", "fxo\nbar\nbaz");

  // Testing "'"
  NormalModeTest("foo\nbar\nbaz", "lmaj'arx", "xoo\nbar\nbaz");

}

void ViModeTest::NormalModeCommandsTest()
{
  
  vi_input_mode_manager->viEnterNormalMode();

  // Testing "J"
  NormalModeTest("foo\nbar", "J", "foo bar");

  // Testing "dd"
  NormalModeTest("foo\nbar", "dd", "bar");
  NormalModeTest("foo\nbar", "2dd", "");

  // Testing "D"
  NormalModeTest("foo bar", "lllD", "foo");
  NormalModeTest("foo\nfoo2\nfoo3", "l2D", "f\nfoo3");

  // Testing "d"
  NormalModeTest("foobar", "ld2l", "fbar");
  NormalModeTest("1 2 3\n4 5 6", "ld100l", "1\n4 5 6");

  NormalModeTest("123\n", "d10l", "\n");
  NormalModeTest("123\n", "10lx", "12\n");

  // Testing "X"
  NormalModeTest("ABCD", "$XX", "AD");

  // Testing "gu"

  NormalModeTest("AbCDF", "gu3l", "abcDF");

  // Testing "guu"
  NormalModeTest("FOO", "guu", "foo");


  // Testing "gU"
  NormalModeTest("aBcdf", "gU2l", "ABcdf");

  // Testing "gUU"
  NormalModeTest("foo", "gUU", "FOO");

}

// kate: space-indent on; indent-width 2; replace-tabs on;
