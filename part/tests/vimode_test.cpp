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
#include "kateconfig.h"
#include <kateglobal.h>
#include "katebuffer.h"
#include "katevikeyparser.h"
#include <kateviglobal.h>
#include <katevinormalmode.h>
#include "kateviewhelpers.h"

QTEST_KDEMAIN(ViModeTest, GUI)

using namespace KTextEditor;

ViModeTest::ViModeTest() {
  kate_document = new KateDocument(false, false, false, 0, NULL);
  kate_view = new KateView(kate_document, 0);
  kate_view->toggleViInputMode();
  Q_ASSERT(kate_view->viInputMode());
  vi_input_mode_manager = kate_view->getViInputModeManager();
}


ViModeTest::~ViModeTest() {
  delete kate_document;
//  delete kate_view;
}

void ViModeTest::BeginTest(const QString& original_text) {
  vi_input_mode_manager->viEnterNormalMode();
  vi_input_mode_manager = kate_view->resetViInputModeManager();
  kate_document->setText(original_text);
  kate_view->setCursorPosition(Cursor(0,0));
}

void ViModeTest::FinishTest(const QString& expected_text, ViModeTest::Expectation expectation, const QString& failureReason)
{
  if (expectation == ShouldFail)
  {
    QEXPECT_FAIL("", failureReason.toLocal8Bit(), Continue);
    qDebug() << "Actual text:\n\t" << kate_document->text() << "\nShould be (for this test to pass):\n\t" << expected_text;
  }
  QCOMPARE(kate_document->text(), expected_text);
}


void ViModeTest::TestPressKey(QString str) {
  qDebug() << "\n\n>>> running command " << str << " on text " << kate_document->text();
  QKeyEvent *key_event;
  QString key;
  Qt::KeyboardModifiers keyboard_modifier;

  for (int i = 0; i< str.length(); i++) {
    key.clear();
    // Looking for keyboard modifiers
    if (str[i] == QChar('\\')) {
        if (str.mid(i,6) == QString("\\ctrl-")){
            keyboard_modifier = Qt::ControlModifier;
            i+=6;
        } else if (str.mid(i,4) == QString("\\alt-")) {
            keyboard_modifier = Qt::AltModifier;
            i+=4;
        } else if (str.mid(i,5) == QString("\\meta-")) {
            keyboard_modifier = Qt::MetaModifier;
            i+=5;
        } else if (str.mid(i,4) == QString("\\esc")) {
            key = QString(Qt::Key_Escape);
            // Move to the end of the esc; next time round the loop will move
            // onto the character after the esc.
            i += 3;
        } else if (str.mid(i,2) == QString("\\:")) {
           int start_cmd = i+2;
           for( i+=2 ; str.at(i) != '\\' ; i++ ) {}
           kate_view->cmdLineBar()->execute(str.mid(start_cmd,i-start_cmd));
           // We've handled the command; go back round the loop, avoiding sending
           // the closing \ to vi_input_mode_manager.
           continue;
        } else {
            assert(false); //Do not use "\" in tests except for modifiers and command mode.
        }
    } else {
        keyboard_modifier = Qt::NoModifier;
    }

    int code;
    if (key != QString(Qt::Key_Escape))
    {
      key = str[i];
      code = key[0].unicode();
      if (keyboard_modifier != Qt::NoModifier)
      {
        // Kate Vim mode's internals identifier e.g. CTRL-C by Qt::Key_C plus the control modifier,
        // so we need to translate e.g. 'c' to Key_C.
        if (key[0].isLetter())
        {
          code = code - 'a' + Qt::Key_A;
        }
      }
    }
    else {
      code = Qt::Key_Escape;
    }

    key_event = new QKeyEvent(QEvent::KeyPress, code, keyboard_modifier, key);
    if (key == QString(Qt::Key_Escape))
    {
      // Sending ESC to KateViewInternal has unfortunate side-effects like cancelling the selection,
      // etc, so send it to the input manager instead.
      kate_view->getViInputModeManager()->handleKeypress(key_event);
    }
    else
    {
      // Allow InsertMode to handle the keys - this has to be done by sending the keys
      // to KateViewInternal's keyPressed handler, which is is unfortunately protected;
      // however, KateViewInternal is kate_view's focus proxy, so  we can achieve this
      // by posting a KeyPress event to it.
      QApplication::postEvent(kate_view->focusProxy(), key_event);
      QApplication::sendPostedEvents();
      // TODO - add test to show why old version was wrong (something like "isausage\\ctrl-c.", perhaps ... ?)
    }
  }
}


/**
 * Starts normal mode.
 * Makes commad on original_text and compare result with expected test.
 * There is a possibility to use keyboard modifiers Ctrl, Alt and Meta,
 * and the ESC key.
 * For example:
 *     DoTest("line 1\nline 2\n","ddu\\ctrl-r","line 2\n");
 */
void ViModeTest::DoTest(QString original_text,
    QString command,
    QString expected_text, Expectation expectation, const QString& failureReason) {

  BeginTest(original_text);
  TestPressKey(command);
  FinishTest(expected_text, expectation, failureReason);
}



void ViModeTest::VisualModeTests() {
    DoTest("foobar", "vlllx", "ar");
    DoTest("foo\nbar", "Vd", "bar");
    DoTest("1234\n1234\n1234", "l\\ctrl-vljjd", "14\n14\n14");
    QCOMPARE(kate_view->blockSelectionMode(), false);

    DoTest("12345678", "lv3lyx", "1345678");
    DoTest("12345678", "$hv3hyx", "1235678");
    DoTest("aaa\nbbb", "lvj~x", "aA\nBBb");
    DoTest("123\n456", "jlvkyx", "13\n456");
    DoTest("12\n34","lVjyx", "2\n34");
    DoTest("ab\ncd","jVlkgux", "a\ncd");
    DoTest("ABCD\nABCD\nABCD\nABCD","lj\\ctrl-vjlgux","ABCD\nAcD\nAbcD\nABCD");
    DoTest("abcd\nabcd\nabcd\nabcd","jjjlll\\ctrl-vkkhgUx","abcd\nabD\nabCD\nabCD");
    // Cancelling visual mode should not reset the cursor.
    // TODO - due to a rather weird bit of code in KateViewInternal's Escape handling,
    // this will only work if the Vi bar is not hidden! (If and only if it is hidden, a
    // clearSelection() call is emitted when ESC is pressed).
    // Investigate whether this is the intention - I suspect not!
    DoTest("12345678", "lv3l\\escx", "1234678");
    DoTest("12345678", "lv3l\\ctrl-cx", "1234678");
    // Don't forget to clear the flag that says we shouldn't reset the cursor, though!
    DoTest("12345678", "lv3l\\ctrl-cxv3lyx", "123478");
    DoTest("12345678", "y\\escv3lyx", "2345678");

    // Testing "d"
    DoTest("foobarbaz","lvlkkjl2ld","fbaz");
    DoTest("foobar","v$d","");
    DoTest("foo\nbar\nbaz","jVlld","foo\nbaz");

    // Testing "D"
    DoTest("foo\nbar\nbaz","lvjlD","baz");
    DoTest("foo\nbar", "l\\ctrl-vjD","f\nb");
    DoTest("foo\nbar","VjkD","bar");

    // Testing "gU", "U"
    DoTest("foo bar", "vwgU", "FOO Bar");
    DoTest("foo\nbar\nbaz", "VjjU", "FOO\nBAR\nBAZ");
    DoTest("foo\nbar\nbaz", "\\ctrl-vljjU","FOo\nBAr\nBAz");

    // Testing "gu", "u"
    DoTest("TEST", "Vgu", "test");
    DoTest("TeSt", "vlgu","teSt");
    DoTest("FOO\nBAR\nBAZ", "\\ctrl-vljju","foO\nbaR\nbaZ");

    // Testing "y"
    DoTest("foobar","Vypp","foobar\nfoobar\nfoobar");
    DoTest("foo\nbar","lvjlyp", "fooo\nbaro\nbar");
    DoTest("foo\nbar","Vjlllypddxxxdd","foo\nbar");
    DoTest("12\n12", "\\ctrl-vjyp", "112\n112");
    DoTest("1234\n1234\n1234\n1234","lj\\ctrl-vljyp","1234\n122334\n122334\n1234");

    // Testing "Y"
    DoTest("foo\nbar","llvjypx","foo\nbar\nbar");
    DoTest("foo\nbar","VYp","foo\nfoo\nbar");

    // Testing "m."
    DoTest("foo\nbar","vljmavgg`ax","foo\nbr");
    DoTest("1\n2\n3\n4","Vjmajjmb\\:'a,'bd\\","1");

    // Testing ">"
    DoTest("foo\nbar","vj>","  foo\n  bar");
    DoTest("foo\nbar\nbaz", "jVj>", "foo\n  bar\n  baz");
    DoTest("foo", "vl3>","      foo");

    // Testing "<"
    DoTest(" foo","vl<", "foo");

    // Testing "o"
    DoTest("foobar","lv2lo2ld","fooar");
    DoTest("foo\nbar","jvllokld","f");
    DoTest("12\n12","\\ctrl-vjlold","1\n1");

    // Testing "~"
    DoTest("foobar","lv2l~","fOOBar");
    DoTest("FooBar","V~","fOObAR");
    DoTest("foo\nbar","\\ctrl-vjl~","FOo\nBAr");

    // Testing "r"
    DoTest("foobar","Vra","aaaaaa");
    DoTest("foo\nbar","jlvklrx","fox\nxxr");
    DoTest("123\n123","l\\ctrl-vljrx","1xx\n1xx");
    DoTest("a", "r\\ctrl-c", "a");

    // Testing "gq"
    DoTest("foo\nbar\nbaz","Vgq","foo\nbar\nbaz");
    DoTest("foo\nbar\nbaz","Vjgq","foo bar\nbaz");

    // Testing "<<"/">>"
    kate_document->config()->setReplaceTabsDyn(true);
    DoTest("foo\nbar\nbaz","V>>","  foo\nbar\nbaz");
    DoTest("foo\nbar\nbaz","Vj>>","  foo\n  bar\nbaz");
    DoTest("foo\nbar\nbaz","V2j>>","  foo\n  bar\n  baz");
    DoTest("foo\nbar\nbaz","V10>>","                    foo\nbar\nbaz");
    DoTest("foo\nbar\nbaz","V2j3>>","      foo\n      bar\n      baz");

    DoTest("  foo\nbar\nbaz","V<<","foo\nbar\nbaz");
    DoTest("foo\nbar\nbaz","V>>V<<","foo\nbar\nbaz");
    DoTest("    foo\n    bar\n    baz","V2j<<","  foo\n  bar\n  baz");

    // Testing block append
    DoTest("averyverylongline\nshortline\nshorter\n", "jjV$kkAb\\esc", "averyverylonglineb\nshortlineb\nshorterb\n");
    DoTest("averyverylongline\nshortline\n", "V$jAb\\esc", "averyverylonglineb\nshortlineb\n");

    // Testing undo behaviour with c and cc
    DoTest("foo", "ciwbar\\escu", "foo");
    DoTest("foo", "ccbar\\escu", "foo");
}

void ViModeTest::InsertModeTests() {

  DoTest("bar", "s\\ctrl-c", "ar");
  DoTest("bar", "ls\\ctrl-cx", "r");
  DoTest("foo\nbar", "S\\ctrl-c", "\nbar");
  DoTest("baz bar", "lA\\ctrl-cx", "baz ba");
  DoTest("baz bar", "la\\ctrl-cx", "bz bar");
  DoTest("foo\nbar\nbaz", "C\\ctrl-c", "\nbar\nbaz");
  DoTest("foo bar baz", "c2w\\ctrl-c", " baz");
  DoTest("foo\nbar\nbaz", "jo\\ctrl-c", "foo\nbar\n\nbaz");
  DoTest("foo\nbar\nbaz", "jO\\ctrl-c", "foo\n\nbar\nbaz");
  DoTest("foo\nbar", "O\\ctrl-c", "\nfoo\nbar");
  DoTest("foo\nbar", "o\\ctrl-c", "foo\n\nbar");
  DoTest("foo bar", "wlI\\ctrl-cx", "oo bar");
  DoTest("foo bar", "wli\\ctrl-cx", "foo ar");
  DoTest("foo bar", "wlihello\\ctrl-c", "foo bhelloar");
  DoTest("", "5ihello\\esc", "hellohellohellohellohello");
  DoTest("bar", "5ahello\\esc", "bhellohellohellohellohelloar");
  DoTest("   bar", "5Ihello\\esc", "   hellohellohellohellohellobar");
  DoTest("bar", "5Ahello\\esc", "barhellohellohellohellohello");
  DoTest("", "5ihello\\ctrl-c", "hello");
  DoTest("bar", "5ohello\\esc", "bar\nhello\nhello\nhello\nhello\nhello");
  DoTest("bar", "5Ohello\\esc", "hello\nhello\nhello\nhello\nhello\nbar");
  DoTest("bar", "Ohello\\escu", "bar");
  DoTest("bar", "5Ohello\\escu", "bar");
  DoTest("bar", "ohello\\escu", "bar");
  DoTest("bar", "5ohello\\escu", "bar");
  DoTest("foo\nbar", "j5Ohello\\esc", "foo\nhello\nhello\nhello\nhello\nhello\nbar");
  DoTest("bar", "5ohello\\esc2ixyz\\esc", "bar\nhello\nhello\nhello\nhello\nhellxyzxyzo");
  DoTest("", "ihello\\esc5.", "hellhellohellohellohellohelloo");
  DoTest("foo foo foo", "c3wbar\\esc", "bar");
  DoTest("abc", "lOxyz", "xyz\nabc");

  // Testing "Ctrl-w"
  DoTest("foobar", "$i\\ctrl-w", "r");
  DoTest("foobar\n", "A\\ctrl-w", "\n");

  // Testing "Ctrl-e"
  DoTest("foo\nbar", "i\\ctrl-e", "bfoo\nbar");
  DoTest("foo\nbar", "i\\ctrl-e\\ctrl-e\\ctrl-e", "barfoo\nbar");
  DoTest("foo\nb", "i\\ctrl-e\\ctrl-e", "bfoo\nb");

  // Testing "Ctrl-y"
  DoTest("foo\nbar", "ji\\ctrl-y", "foo\nfbar");
  DoTest("foo\nbar", "ji\\ctrl-y\\ctrl-y\\ctrl-y", "foo\nfoobar");
  DoTest("f\nbar", "ji\\ctrl-y\\ctrl-y", "f\nfbar");

  // Testing "Ctrl-R"
  DoTest("barbaz", "\"ay3li\\ctrl-ra", "barbarbaz");
  DoTest("bar\nbaz", "\"byylli\\ctrl-rb", "bar\nbar\nbaz" );

  // Testing "Ctrl-O"
  DoTest("foo bar baz","3li\\ctrl-od2w","foobaz");
  DoTest("foo bar baz","3li\\ctrl-od2w\\ctrl-w","baz");
  DoTest("foo bar baz","i\\ctrl-o3l\\ctrl-w"," bar baz");
  DoTest("foo\nbar\nbaz","li\\ctrl-oj\\ctrl-w\\ctrl-oj\\ctrl-w","foo\nar\naz");
  // Test that the text written after the Ctrl-O command completes is treated as
  // an insertion of text (rather than a sequence of commands) when repeated via "."
  DoTest("", "isausage\\ctrl-obugo\\esc.", "ugugoosausage");
  // 'Step back' on Ctrl-O if at the end of the line
  DoTest("foo bar baz","A\\ctrl-ox","foo bar ba");
  // Paste acts as gp when executing in a Ctrl-O
  DoTest("foo bar baz","yiwea\\ctrl-opd","foo foodbar baz");
  DoTest("bar","A\\ctrl-o\\ctrl-chx","br");
  DoTest("bar","A\\ctrl-o\\eschx","br");

  // Testing "Ctrl-D" "Ctrl-T"
  DoTest("foo", "i\\ctrl-t" , "  foo");
  DoTest(" foo", "i\\ctrl-d", "foo");
  DoTest("foo\nbar", "i\\ctrl-t\\ctrl-d","foo\nbar" );

  // Testing "Ctrl-H"
  DoTest("foo", "i\\ctrl-h" , "foo");
  DoTest(" foo", "li\\ctrl-h", "foo");
  DoTest("foo\nbar", "ji\\ctrl-h","foobar" );
  DoTest("1234567890", "A\\ctrl-h\\ctrl-h\\ctrl-h\\ctrl-h\\ctrl-h","12345");
  DoTest("1\n2\n3", "GA\\ctrl-h\\ctrl-h\\ctrl-h\\ctrl-h","1");

  // Testing "Ctrl-J"
  DoTest("foo", "i\\ctrl-j" , "\nfoo");
  DoTest("foo", "lli\\ctrl-j", "fo\no");
  DoTest("foo\nbar", "ji\\ctrl-j","foo\n\nbar");
  DoTest("foobar", "A\\ctrl-j","foobar\n" );
  DoTest("foobar", "li\\ctrl-j\\ctrl-cli\\ctrl-j\\ctrl-cli\\ctrl-j\\ctrl-cli\\ctrl-j\\ctrl-cli\\ctrl-j\\ctrl-c","f\no\no\nb\na\nr");
}

void ViModeTest::NormalModeMotionsTest() {

  // Test moving around an empty document (nothing should happen)
  DoTest("", "jkhl", "");
  DoTest("", "ggG$0", "");

  //Testing "l"
  DoTest("bar", "lx", "br");
  DoTest("bar", "2lx", "ba");
  DoTest("0123456789012345", "13lx", "012345678901245");
  DoTest("bar", "10lx", "ba");

  // Testing "h"
  DoTest("bar", "llhx", "br");
  DoTest("bar", "10l10hx", "ar");
  DoTest("0123456789012345", "13l10hx", "012456789012345");
  DoTest("bar", "ll5hx", "ar");

  // Testing "j"
  DoTest("bar\nbar", "jx", "bar\nar");
  DoTest("bar\nbar", "10jx", "bar\nar");
  DoTest("bar\nbara", "lljx", "bar\nbaa");
  DoTest("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n5\n",
      "13jx",
      "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n1\n2\n\n4\n5\n");

  // Testing "k"
  DoTest("bar\nbar", "jx", "bar\nar");
  DoTest("bar\nbar\nbar", "jj100kx", "ar\nbar\nbar");
  DoTest("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n5\n",
      "13j10kx",
      "0\n1\n2\n\n4\n5\n6\n7\n8\n9\n0\n1\n2\n3\n4\n5\n");

  // Testing "w"
  DoTest("bar", "wx", "ba");
  DoTest("foo bar", "wx", "foo ar");
  DoTest("foo bar", "lwx","foo ar");
  DoTest("quux(foo, bar, baz);", "wxwxwxwx2wx","quuxfoo ar baz;");
  DoTest("foo\nbar\nbaz", "wxwx","foo\nar\naz");
  DoTest("1 2 3\n4 5 6", "ld3w", "1\n4 5 6");
  DoTest("foo\nbar baz", "gU2w", "FOO\nBAR baz");
  DoTest("FOO\nBAR BAZ", "gu2w", "foo\nbar BAZ");


  // Testing "W"
  DoTest("bar", "Wx", "ba");
  DoTest("foo bar", "Wx", "foo ar");
  DoTest("foo bar", "2lWx","foo ar");
  DoTest("quux(foo, bar, baz);", "WxWx","quux(foo, ar, az);");
  DoTest("foo\nbar\nbaz", "WxWx","foo\nar\naz");

  // Testing "b"
  DoTest("bar", "lbx", "ar");
  DoTest("foo bar baz", "2wbx", "foo ar baz");
  DoTest("foo bar", "w20bx","oo bar");
  DoTest("quux(foo, bar, baz);", "2W4l2bx2bx","quux(foo, ar, az);");
  DoTest("foo\nbar\nbaz", "WWbx","foo\nar\nbaz");

  // Testing "B"
  DoTest("bar", "lBx", "ar");
  DoTest("foo bar baz", "2wBx", "foo ar baz");
  DoTest("foo bar", "w20Bx","oo bar");
  DoTest("quux(foo, bar, baz);", "2W4lBBx","quux(foo, ar, baz);");
  DoTest("foo\nbar", "WlBx","foo\nar");

  // Testing "e"
  DoTest("quux(foo, bar, baz);", "exex2ex10ex","quu(fo, bar baz)");

  // Testing "E"
  DoTest("quux(foo, bar, baz);", "ExEx10Ex","quux(foo bar baz)");

  // Testing "$"
  DoTest("foo\nbar\nbaz", "$x3$x","fo\nbar\nba");

  // Testing "0"
  DoTest(" foo", "$0x","foo");

  // Testing "#"
  DoTest("1 1 1", "2#x","1  1");
  DoTest("foo bar foo bar", "#xlll#x","foo ar oo bar");
  DoTest("(foo (bar (foo( bar))))", "#xll#x","(foo (ar (oo( bar))))");

  // Testing "*"
  DoTest("(foo (bar (foo( bar))))", "*x","(foo (bar (oo( bar))))");

  // Testing "-"
  DoTest("0\n1\n2\n3\n4\n5", "5j-x2-x", "0\n1\n\n3\n\n5");

  // Testing "^"
  DoTest(" foo bar", "$^x", " oo bar");

  // Testing "gg"
  DoTest("1\n2\n3\n4\n5", "4jggx", "\n2\n3\n4\n5");

  // Testing "G"
  DoTest("1\n2\n3\n4\n5", "Gx", "1\n2\n3\n4\n");

  // Testing "ge"
  DoTest("quux(foo, bar, baz);", "9lgexgex$gex", "quux(fo bar, ba);");

  // Testing "gE"
  DoTest("quux(foo, bar, baz);", "9lgExgEx$gEx", "quux(fo bar baz);");

  // Testing "|"
  DoTest("123456789", "3|rx4|rx8|rx1|rx", "x2xx567x9");

  // Testing "`"
  DoTest("foo\nbar\nbaz", "lmaj`arx", "fxo\nbar\nbaz");

  // Testing "'"
  DoTest("foo\nbar\nbaz", "lmaj'arx", "xoo\nbar\nbaz");

  // Testing "%"
  DoTest("foo{\n}\n", "$d%", "foo\n");
  DoTest("FOO{\nBAR}BAZ", "lllgu%", "FOO{\nbar}BAZ");
  DoTest("foo{\nbar}baz", "lllgU%", "foo{\nBAR}baz");
  DoTest("foo{\nbar\n}", "llly%p", "foo{{\nbar\n}\nbar\n}");
  // Regression bug for test where yanking with % would actually move the cursor.
  DoTest("a()", "y%x", "()");
  // Regression test for the bug I added fixing the bug above ;)
  DoTest("foo(bar)", "y%P", "foo(bar)foo(bar)");

  // Testing percentage "<N>%"
  DoTest("10%\n20%\n30%\n40%\n50%\n60%\n70%\n80%\n90%\n100%",
         "20%dd",
         "10%\n30%\n40%\n50%\n60%\n70%\n80%\n90%\n100%");

  DoTest("10%\n20%\n30%\n40%\n50%\n60%\n70%\n80%\n90%\n100%",
         "50%dd",
         "10%\n20%\n30%\n40%\n60%\n70%\n80%\n90%\n100%");

  DoTest("10%\n20%\n30%\n40%\n50%\n60%\n70\n80%\n90%\n100%",
         "65%dd",
         "10%\n20%\n30%\n40%\n50%\n60%\n80%\n90%\n100%");

  DoTest("10%\n20%\n30%\n40%\n50%\n60%\n70%\n80%\n90%\n100%",
         "5j10%dd",
         "20%\n30%\n40%\n50%\n60%\n70%\n80%\n90%\n100%");

   // TEXT OBJECTS
  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lci'",
          "foo \"bar baz ('first', '' or 'third')\"");

  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lca'",
          "foo \"bar baz ('first',  or 'third')\"");

  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lci(",
          "foo \"bar baz ()\"");

  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lci(",
          "foo \"bar baz ()\"");

  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lcib",
          "foo \"bar baz ()\"");
  // Quick test that bracket object works in visual mode.
  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lvibd",
          "foo \"bar baz ()\"");
  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lvabd",
          "foo \"bar baz \"");

  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lca)",
          "foo \"bar baz \"");

  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lci\"",
          "foo \"\"");

  DoTest( "foo \"bar baz ('first', 'second' or 'third')\"",
          "8w2lda\"",
          "foo ");

  DoTest( "foo \"bar [baz ({'first', 'second'} or 'third')]\"",
          "9w2lci[",
          "foo \"bar []\"");

  DoTest( "foo \"bar [baz ({'first', 'second'} or 'third')]\"",
          "9w2lci]",
          "foo \"bar []\"");

  DoTest( "foo \"bar [baz ({'first', 'second'} or 'third')]\"",
          "9w2lca[",
          "foo \"bar \"");

  DoTest( "foo \"bar [baz ({'first', 'second'} or 'third')]\"",
          "9w2lci{",
          "foo \"bar [baz ({} or 'third')]\"");

  DoTest( "foo \"bar [baz ({'first', 'second'} or 'third')]\"",
          "7w2lca}",
          "foo \"bar [baz ( or 'third')]\"");

  DoTest( "{foo { bar { (baz) \"asd\" }} {1} {2} {3} {4} {5} }",
          "ldiB",
          "{}");

  DoTest( "{\nfoo\n}", "jdiB", "{\n}");
  DoTest( "{\n}", "diB", "{\n}");
  DoTest( "{\nfoo}", "jdiB", "{\n}");
  DoTest( "{foo\nbar\nbaz}", "jdiB", "{}");
  DoTest( "{foo\nbar\n  \t\t }", "jdiB", "{\n  \t\t }");
  DoTest( "{foo\nbar\n  \t\ta}", "jdiB", "{}");
  DoTest( "\t{\n\t}", "ldiB", "\t{\n\t}");
  // Quick test to see whether inner curly bracket works in visual mode.
  DoTest( "{\nfoo}", "jviBd", "{\n}");
  DoTest( "{\nfoo}", "jvaBd", "");

  DoTest( "int main() {\n  printf( \"HelloWorld!\\n\" );\n  return 0;\n} ",
          "jda}xr;",
          "int main();");

  DoTest("QList<QString>","wwldi>","QList<>");
  DoTest("QList<QString>","wwlda<","QList");
  DoTest("<head>\n<title>Title</title>\n</head>",
         "di<jci>",
         "<>\n<>Title</title>\n</head>");

  DoTest( "foo bar baz", "wldiw", "foo  baz");

  DoTest( "foo bar baz", "wldawx", "foo az");

  DoTest( "foo ( \n bar\n)baz","jdi(", "foo ()baz");
  DoTest( "foo ( \n bar\n)baz","jda(", "foo baz");
  DoTest( "(foo(bar)baz)", "ldi)", "()");
  DoTest( "(foo(bar)baz)", "lca(", "");
  DoTest( "( foo ( bar ) )baz", "di(", "()baz" );
  DoTest( "( foo ( bar ) )baz", "da(", "baz" );
  DoTest( "[foo [ bar] [(a)b [c]d ]]","$hda]", "[foo [ bar] ]");
  DoTest( "(a)", "di(", "()");
  DoTest( "(ab)", "di(", "()");
  DoTest( "(abc)", "di(", "()");

  DoTest( "hi!))))}}]]","di]di}da)di)da]", "hi!))))}}]]" );

  DoTest("foo \"bar\" baz", "4ldi\"", "foo \"\" baz");
  DoTest("foo \"bar\" baz", "8lca\"", "foo  baz");

  DoTest("foo 'bar' baz", "4lca'", "foo  baz");
  DoTest("foo 'bar' baz", "8ldi'", "foo '' baz");

  DoTest("foo `bar` baz", "4lca`", "foo  baz");
  DoTest("foo `bar` baz", "8ldi`", "foo `` baz");

  DoTest("()","di(","()");
  DoTest("\"\"","di\"","\"\"");

  // Comma text object
  DoTest("func(aaaa);", "llllldi,", "func();");
  DoTest("func(aaaa);", "lllllda,", "func;");
  DoTest("//Hello, world!\nfunc(a[0] > 2);", "jf>di,", "//Hello, world!\nfunc();");
  DoTest("//Hello, world!\nfunc(a[0] > 2);", "jf>da,", "//Hello, world!\nfunc;");
  DoTest("//Hello, world!\na[] = {135};", "jf3di,", "//Hello, world!\na[] = {};");

  // Some corner case tests for t/ T, mainly dealing with how a ; after e.g. a ta will
  // start searching for the next a *after* the character after the cursor.
  // Hard to explain; I'll let the test-cases do the talking :)
  DoTest("bar baz", "ta;x" ,"bar az");
  // Ensure we reset the flag that says we must search starting from the character after the cursor!
  DoTest("bar baz", "ta;^tax" ,"ar baz");
  // Corresponding tests for T
  DoTest("bar baz", "$Ta;x" ,"ba baz");
  // Ensure we reset the flag that says we must search starting from the character before the cursor!
  DoTest("bar baz", "$Ta;$Tax" ,"bar ba");
  // Ensure that command backwards works, too - only one test, as any additional ones would
  // just overlap with our previous ones.
  DoTest("aba bar", "lTa,x", "aba ar");
  // Some tests with counting.
  DoTest("aba bar", "2tax", "aba ar");
  // If we can't find 3 further a's, don't move at all...
  DoTest("aba bar", "3tax", "ba bar");
  // ... except if we are repeating the last search, in which case stop at the last
  // one that we do find.
  DoTest("aba bar", "ta2;x", "aba ar");

  // Don't move if we can't find any matches at all.
  DoTest("nocapitalc", "lltCx", "noapitalc");
  DoTest("nocapitalc", "llTCx", "noapitalc");

  // Motion to lines starting with { or }
  DoTest("{\nfoo\n}", "][x", "{\nfoo\n");
  DoTest("{\nfoo\n}", "j[[x", "\nfoo\n}");
  DoTest("bar\n{\nfoo\n}", "]]x", "bar\n\nfoo\n}");
  DoTest("{\nfoo\n}\nbar", "jjj[]x", "{\nfoo\n\nbar");
  DoTest("bar\nfoo\n}", "d][", "}");
  DoTest("bar\n{\nfoo\n}", "d]]", "{\nfoo\n}");
  DoTest("bar\nfoo\n}", "ld][", "b\n}");
  DoTest("{\nfoo\n}", "jld[[", "oo\n}");
  DoTest("bar\n{\nfoo\n}", "ld]]", "b\n{\nfoo\n}");
  DoTest("{\nfoo\n}\nbar", "jjjld[]", "{\nfoo\nar");
}

void ViModeTest::NormalModeCommandsTest() {

  // Testing "J"
  DoTest("foo\nbar", "J", "foo bar");

  // Testing "dd"
  DoTest("foo\nbar", "dd", "bar");
  DoTest("foo\nbar", "2dd", "");
  DoTest("foo\nbar\n", "Gdd", "foo\nbar");

  // Testing "D"
  DoTest("foo bar", "lllD", "foo");
  DoTest("foo\nfoo2\nfoo3", "l2D", "f\nfoo3");

  // Testing "d"
  DoTest("foobar", "ld2l", "fbar");
  DoTest("1 2 3\n4 5 6", "ld100l", "1\n4 5 6");

  DoTest("123\n", "d10l", "\n");
  DoTest("123\n", "10lx", "12\n");

  // Testing "X"
  DoTest("ABCD", "$XX", "AD");

  // Testing "gu"
  DoTest("FOO\nBAR BAZ", "guj", "foo\nbar baz");
  DoTest("AbCDF", "gu3l", "abcDF");

  // Testing "guu"
  DoTest("FOO", "guu", "foo");
  DoTest("FOO\nBAR\nBAZ", "2guu", "foo\nbar\nBAZ");


  // Testing "gU"
  DoTest("aBcdf", "gU2l", "ABcdf");
  DoTest("foo\nbar baz", "gUj", "FOO\nBAR BAZ");

  // Testing "gUU"
  DoTest("foo", "gUU", "FOO");
  DoTest("foo\nbar\nbaz", "2gUU", "FOO\nBAR\nbaz");

  // Testing "Ctrl-o" and "Ctrl-i"
  DoTest("abc\ndef\nghi","Gx\\ctrl-ox","bc\ndef\nhi");
  DoTest("{\n}","%\\ctrl-ox","\n}");
  DoTest("Foo foo.\nBar bar.\nBaz baz.",
                   "lmajlmb`a`b\\ctrl-ox",
                   "Fo foo.\nBar bar.\nBaz baz.");
  DoTest("Foo foo.\nBar bar.\nBaz baz.",
                   "lmajlmb`a`bj\\ctrl-o\\ctrl-ix",
                   "Foo foo.\nBar bar.\nBa baz.");


  // Testing "gq" (reformat) text
  DoTest("foo\nbar", "gqq", "foo\nbar");
  DoTest("foo\nbar", "2gqq", "foo bar");
  DoTest("foo\nbar\nbaz", "jgqj", "foo\nbar baz");

  // when setting the text to wrap at column 10, this should be re-formatted to
  // span several lines ...
  kate_document->setWordWrapAt( 10 );
  DoTest("foo bar foo bar foo bar", "gqq", "foo bar \nfoo bar \nfoo bar");

  // ... and when re-setting it to column 80 again, they should be joined again
  kate_document->setWordWrapAt( 80 );
  DoTest("foo bar\nfoo bar\nfoo bar", "gqG", "foo bar foo bar foo bar");

  // test >> and << (indent and de-indent)
  kate_document->config()->setReplaceTabsDyn(true);

  DoTest("foo\nbar", ">>", "  foo\nbar");
  DoTest("foo\nbar", "2>>", "  foo\n  bar");
  DoTest("foo\nbar", "100>>", "  foo\n  bar");

  DoTest("fop\nbar", "yiwjlgpx", "fop\nbafop");
  DoTest("fop\nbar", "yiwjlgPx", "fop\nbfopr");
  // Yank and paste op\ngid into bar i.e. text spanning lines, but not linewise.
  DoTest("fop\ngid\nbar", "lvjyjjgpx", "fop\ngid\nbaop\ngi");
  DoTest("fop\ngid\nbar", "lvjyjjgPx", "fop\ngid\nbop\ngir");
  DoTest("fop\ngid\nbar", "lvjyjjpx", "fop\ngid\nbap\ngir");
  DoTest("fop\ngid\nbar", "lvjyjjPx", "fop\ngid\nbp\ngiar");
  // Linewise
  DoTest("fop\ngid\nbar\nhuv", "yjjjgpx", "fop\ngid\nbar\nfop\ngid\nuv");
  DoTest("fop\ngid\nbar\nhuv", "yjjjgPx", "fop\ngid\nfop\ngid\nar\nhuv");
  DoTest("fop\ngid", "yjjgpx", "fop\ngid\nfop\nid");
  DoTest("fop\ngid\nbar\nhuv", "yjjjPx", "fop\ngid\nop\ngid\nbar\nhuv");

  DoTest("fop\nbar", "yiwjlpx", "fop\nbafor");
  DoTest("fop\nbar", "yiwjlPx", "fop\nbfoar");

  // Last edit markers.
  DoTest("foo", "ean\\escgg`.r.", "foo.");
  DoTest("foo", "ean\\escgg`[r[", "foo[");
  DoTest("foo", "ean\\escgg`]r]", "foo]");
  DoTest("foo bar", "ean\\escgg`]r]", "foon]bar");
  DoTest("", "ibar\\escgg`.r.", "ba.");
  DoTest("", "ibar\\escgggUiw`.r.", ".AR");
  DoTest("", "2ibar\\escgg`]r]", "barba]");
  DoTest("", "2ibar\\escgg`[r[", "[arbar");
  DoTest("", "3ibar\\escgg`.r.", "barbar.ar"); // Vim is weird.
  DoTest("", "abar\\esc.gg`]r]", "barba]");
  DoTest("foo bar", "wgUiwgg`]r]", "foo BA]");
  DoTest("foo bar", "wgUiwgg`.r.", "foo .AR");
  DoTest("foo bar", "gUiwgg`]r.", "FO. bar");
  DoTest("foo bar", "wdiwgg`[r[", "foo[");
  DoTest("foo bar", "wdiwgg`]r]", "foo]");
  DoTest("foo bar", "wdiwgg`.r.", "foo.");
  DoTest("foo bar", "wciwnose\\escgg`.r.", "foo nos.");
  DoTest("foo bar", "wciwnose\\escgg`[r[", "foo [ose");
  DoTest("foo bar", "wciwnose\\escgg`]r]", "foo nos]");
  DoTest("foo", "~ibar\\escgg`[r[", "F[aroo");
  DoTest("foo bar", "lragg`.r.", "f.o bar");
  DoTest("foo bar", "lragg`[r[", "f[o bar");
  DoTest("foo bar", "lragg`]r]", "f]o bar");
  DoTest("", "ifoo\\ctrl-hbar\\esc`[r[", "[obar");
  DoTest("", "ifoo\\ctrl-wbar\\esc`[r[", "[ar");
  DoTest("", "if\\ctrl-hbar\\esc`[r[", "[ar");
  DoTest("", "5ofoo\\escgg`[r[", "\n[oo\nfoo\nfoo\nfoo\nfoo");
  DoTest("", "5ofoo\\escgg`]r]", "\nfoo\nfoo\nfoo\nfoo\nfo]");
  DoTest("", "5ofoo\\escgg`.r.", "\nfoo\nfoo\nfoo\nfoo\n.oo");
}


void ViModeTest::NormalModeControlTests() {
  // Testing "Ctrl-x"
  DoTest("150", "101\\ctrl-x", "49");
  DoTest("1", "\\ctrl-x\\ctrl-x\\ctrl-x\\ctrl-x", "-3");
  DoTest("0xabcdef", "1000000\\ctrl-x","0x9c8baf" );
  DoTest("0x0000f", "\\ctrl-x","0x0000e" );

  // Testing "Ctrl-a"
  DoTest("150", "101\\ctrl-a", "251");
  DoTest("1000", "\\ctrl-ax", "100");
  DoTest("-1", "1\\ctrl-a", "0");
  DoTest("-1", "l1\\ctrl-a", "0");
  DoTest("0x0000f", "\\ctrl-a","0x00010" );
  DoTest("5", "5\\ctrl-a.","15" );
  DoTest("5", "5\\ctrl-a2.","12");
  DoTest("5", "5\\ctrl-a2.10\\ctrl-a","22");
  DoTest(" 5 ", "l\\ctrl-ax","  ");

  // Testing "Ctrl-r"
  DoTest("foobar", "d3lu\\ctrl-r", "bar");
  DoTest("line 1\nline 2\n","ddu\\ctrl-r","line 2\n");
}

void ViModeTest::NormalModeNotYetImplementedFeaturesTest() {
  // Testing "))"
//    DoTest("Foo foo. Bar bar.","))\\ctrl-ox","Foo foo. ar bar.");
//    DoTest("Foo foo.\nBar bar.\nBaz baz.",")))\\ctrl-ox\\ctrl-ox", "Foo foo.\nar bar.\nBaz baz.");
//    DoTest("Foo foo.\nBar bar.\nBaz baz.","))\\ctrl-ox\\ctrl-ix","Foo foo.\nBar bar.\naz baz.");
//    DoTest("Foo foo.\nBar bar.\nBaz baz.","))\\ctrl-ox\\ctrl-ix","Foo foo.\nBar bar.\naz baz.");

}

void ViModeTest::CommandModeTests() {
    // Testing ":<num>"
    DoTest("foo\nbar\nbaz","\\:2\\x","foo\nar\nbaz");
    DoTest("foo\nbar\nbaz","jmak\\:'a\\x","foo\nar\nbaz");
    DoTest("foo\nbar\nbaz","\\:$\\x","foo\nbar\naz");

    // Testing ":s" (sed)
    DoTest("foo","\\:s/foo/bar\\","bar");
    DoTest("foobarbaz","\\:s/bar/xxx\\","fooxxxbaz");
    DoTest("foo","\\:s/bar/baz\\","foo");
    DoTest("foo\nfoo\nfoo","j\\:s/foo/bar\\", "foo\nbar\nfoo");
    DoTest("foo\nfoo\nfoo","2jma2k\\:'a,'as/foo/bar\\", "foo\nfoo\nbar");
    DoTest("foo\nfoo\nfoo","\\:%s/foo/bar\\","bar\nbar\nbar");
    DoTest("foo\nfoo\nfoo","\\:2,3s/foo/bar\\","foo\nbar\nbar");
    DoTest("foo\nfoo\nfoo\nfoo", "j2lmajhmbgg\\:'a,'bs/foo/bar\\","foo\nbar\nbar\nfoo");
    DoTest("foo\nfoo\nfoo\nfoo", "jlma2jmbgg\\:'b,'as/foo/bar\\","foo\nbar\nbar\nbar");
    DoTest("foo", "\\:s/$/x/g\\","foox");
    DoTest("foo", "\\:s/.*/x/g\\","x");

    DoTest("foo/bar", "\\:s-/--\\","foobar");
    DoTest("foo/bar", "\\:s_/__\\","foobar");

    DoTest("foo\nfoo\nfoo","\\:2s/foo/bar\\", "foo\nbar\nfoo");
    DoTest("foo\nfoo\nfoo","2jmagg\\:'as/foo/bar\\","foo\nfoo\nbar");
    DoTest("foo\nfoo\nfoo", "\\:$s/foo/bar\\","foo\nfoo\nbar");

    // Testing ":d", ":delete"
    DoTest("foo\nbar\nbaz","\\:2d\\","foo\nbaz");
    DoTest("foo\nbar\nbaz","\\:%d\\","");
    DoTest("foo\nbar\nbaz","\\:$d\\\\:$d\\","foo");
    DoTest("foo\nbar\nbaz","ma\\:2,'ad\\","baz");
    DoTest("foo\nbar\nbaz","\\:/foo/,/bar/d\\","baz");
    DoTest("foo\nbar\nbaz","\\:2,3delete\\","foo");

    DoTest("foo\nbar\nbaz","\\:d\\","bar\nbaz");
    DoTest("foo\nbar\nbaz","\\:d 33\\","");
    DoTest("foo\nbar\nbaz","\\:3d a\\k\"ap","foo\nbaz\nbar");

    // Testing ":y", ":yank"
    DoTest("foo\nbar\nbaz","\\:3y\\p","foo\nbaz\nbar\nbaz");
    DoTest("foo\nbar\nbaz","\\:2y a 2\\\"ap","foo\nbar\nbaz\nbar\nbaz");
    DoTest("foo\nbar\nbaz","\\:y\\p","foo\nfoo\nbar\nbaz");
    DoTest("foo\nbar\nbaz","\\:3,1y\\p","foo\nfoo\nbar\nbaz\nbar\nbaz");

    // Testing ">"
    DoTest("foo","\\:>\\","  foo");
    DoTest("   foo","\\:<\\","  foo");

    // Testing ":c", ":change"
    DoTest("foo\nbar\nbaz","\\:2change\\","foo\n\nbaz");
    DoTest("foo\nbar\nbaz","\\:%c\\","");
    DoTest("foo\nbar\nbaz","\\:$c\\\\:$change\\","foo\nbar\n");
    DoTest("foo\nbar\nbaz","ma\\:2,'achange\\","\nbaz");
    DoTest("foo\nbar\nbaz","\\:2,3c\\","foo\n");

    // Testing ":j"
    DoTest("1\n2\n3\n4\n5","\\:2,4j\\","1\n2 3 4\n5");


    DoTest("1\n2\n3\n4","jvj\\ctrl-c\\:'<,'>d\\","1\n4");
    DoTest("1\n2\n3\n4","\\:1+1+1+1d\\","1\n2\n3");
    DoTest("1\n2\n3\n4","2j\\:.,.-1d\\","1\n4");
    DoTest("1\n2\n3\n4","\\:.+200-100-100+20-5-5-5-5+.-.,$-1+1-2+2-3+3-4+4-5+5-6+6-7+7-1000+1000+0-0-$+$-.+.-1d\\","4");
    DoTest("1\n2\n3\n4","majmbjmcjmdgg\\:'a+'b+'d-'c,.d\\","");

    // Testing "{" and "}" motions
    DoTest("","{}","");
    DoTest("foo","{}dd","");
    DoTest("foo\n\nbar","}dd","foo\nbar");
    DoTest("foo\n\nbar\n\nbaz","3}x","foo\n\nbar\n\nba");
    DoTest("foo\n\nbar\n\nbaz","3}{dd{dd","foo\nbar\nbaz");
    DoTest("foo\nfoo\n\nbar\n\nbaz","5}{dd{dd","foo\nfoo\nbar\nbaz");
    DoTest("foo\nfoo\n\nbar\n\nbaz","5}3{x","oo\nfoo\n\nbar\n\nbaz");
    DoTest("foo\n\n\nbar","10}{{x","oo\n\n\nbar");
    DoTest("foo\n\n\nbar","}}x","foo\n\n\nba");
    DoTest("foo\n\n\nbar\n","}}dd","foo\n\n\nbar");
    // Quick test that "{" and "}" motions work in visual mode
    DoTest("foo\n\n\nbar\n","v}}d","");
    DoTest("\n\nfoo\nbar\n","jjjv{d","\nar\n");
}

void ViModeTest::MappingTests()
{
  const int mappingTimeoutMS = 100;

  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'", "<esc>ihello<esc>^aworld<esc>");
  DoTest("", "'", "hworldello");

  // Ensure that the non-mapping logged keypresses are cleared before we execute a mapping
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'a", "rO");
  DoTest("X", "'a", "O");

  {
    // Check that '123 is mapped after the timeout, given that we also have mappings that
    // extend it (e.g. '1234, '12345, etc) and which it itself extends ('1, '12, etc).
    KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
    BeginTest("");
    kate_view->getViInputModeManager()->getViNormalMode()->setMappingTimeout(mappingTimeoutMS);;
    QString consectiveDigits;
    for (int i = 1; i < 9; i++)
    {
      consectiveDigits += QString::number(i);
      KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'" + consectiveDigits, "iMapped from " + consectiveDigits + "<esc>");
    }
    TestPressKey("'123");
    QCOMPARE(kate_document->text(), QString("")); // Shouldn't add anything until after the timeout!
    QTest::qWait(2 * mappingTimeoutMS);
    FinishTest("Mapped from 123");
  }

  // Make mappings countable; the count should be applied to the whole mapped sequence, not
  // just the first command in the sequence.
  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'testmapping", "ljrO");
  DoTest("XXXX\nXXXX\nXXXX\nXXXX", "3'testmapping", "XXXX\nXOXX\nXXOX\nXXXO");

  // Test that countable mappings work even when triggered by timeouts.
  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'testmapping", "ljrO");
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'testmappingdummy", "dummy");
  BeginTest("XXXX\nXXXX\nXXXX\nXXXX");
  kate_view->getViInputModeManager()->getViNormalMode()->setMappingTimeout(mappingTimeoutMS);;
  TestPressKey("3'testmapping");
  QTest::qWait(2 * mappingTimeoutMS);
  FinishTest("XXXX\nXOXX\nXXOX\nXXXO");

  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'testmapping", "ljrO");
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'testmappingdummy", "dummy");
  BeginTest("XXXX\nXXXX\nXXXX\nXXXX");
  kate_view->getViInputModeManager()->getViNormalMode()->setMappingTimeout(mappingTimeoutMS);;
  TestPressKey("3'testmapping");
  QTest::qWait(2 * mappingTimeoutMS);
  FinishTest("XXXX\nXOXX\nXXOX\nXXXO");

  // Test that telescoping mappings don't interfere with built-in commands. Assumes that gp
  // is implemented and working.
  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "gdummy", "idummy");
  DoTest("hello", "yiwgpx", "hhellollo");

  // Test that we can map a sequence of keys that extends a built-in command and use
  // that sequence without the built-in command firing.
  // Once again, assumes that gp is implemented and working.
  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "gpa", "idummy");
  DoTest("hello", "yiwgpa", "dummyhello");

  // Test that we can map a sequence of keys that extends a built-in command and still
  // have the original built-in command fire if we timeout after entering that command.
  // Once again, assumes that gp is implemented and working.
  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "gpa", "idummy");
  BeginTest("hello");
  kate_view->getViInputModeManager()->getViNormalMode()->setMappingTimeout(mappingTimeoutMS);;
  TestPressKey("yiwgp");
  QTest::qWait(2 * mappingTimeoutMS);
  TestPressKey("x");
  FinishTest("hhellollo");

  // Make sure that a mapped sequence of commands is merged into a single undo-able edit.
  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'a", "ofoo<esc>ofoo<esc>ofoo<esc>");
  DoTest("bar", "'au", "bar");

  // Make sure that a counted mapping is merged into a single undoable edit.
  KateGlobal::self()->viInputModeGlobal()->clearMappings(NormalMode);
  KateGlobal::self()->viInputModeGlobal()->addMapping(NormalMode, "'a", "ofoo<esc>");
  DoTest("bar", "5'au", "bar");
}

// Special area for tests where you want to set breakpoints etc without all the other tests
// triggering them.  Run with ./vimode_test debuggingTests
void ViModeTest::debuggingTests()
{

}

// kate: space-indent on; indent-width 2; replace-tabs on;
