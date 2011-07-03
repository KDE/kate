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
#include "kateviewhelpers.h"

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
  QString key;
  Qt::KeyboardModifiers keyboard_modifier;

  for (int i = 0; i< str.length(); i++) {
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
        } else if (str.mid(i,2) == QString("\\:")) {
           int start_cmd = i+2;
           for( i+=2 ; str.at(i) != '\\' ; i++ ) {}
           kate_view->cmdLineBar()->execute(str.mid(start_cmd,i-start_cmd));
        } else {
            assert(false); //Do not use "\" in tests except for modifiers.
        }
    } else {
        keyboard_modifier = Qt::NoModifier;
    }

    key = str[i];

    int code;
    if (vi_input_mode_manager->getCurrentViMode() == InsertMode){
        code = key[0].unicode() - 'a' + Qt::Key_A;
    } else {
        code = key[0].unicode() - '0' + Qt::Key_0;
    }

    key_event = new QKeyEvent(QEvent::KeyRelease, code, keyboard_modifier, key);
    vi_input_mode_manager->handleKeypress(key_event);
  }
}


/**
 * Starts normal mode.
 * Makes commad on original_text and compare result with expected test.
 * There is a possibility to use keyboard modifiers Ctrl, Alt and Meta.
 * For example:
 *     DoTest("line 1\nline 2\n","ddu\\ctrl-r","line 2\n");
 */
void ViModeTest::DoTest(QString original_text,
    QString command,
    QString expected_text) {

  vi_input_mode_manager->viEnterNormalMode();
  vi_input_mode_manager = kate_view->resetViInputModeManager();
  kate_document->setText(original_text);
  kate_view->setCursorPosition(Cursor(0,0));
  TestPressKey(command);
  qDebug() << "\nrunning command " << command << " on text \"" << original_text
    << "\"\n";
  QCOMPARE(kate_document->text(), expected_text);

}


void ViModeTest::VisualModeTests(){
    DoTest("foobar", "vlllx", "ar");
    DoTest("foo\nbar", "Vd", "bar");
    DoTest("1234\n1234\n1234", "l\\ctrl-vljjd", "14\n14\n14");
}

void ViModeTest::InsertModeTests(){

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

  // Testing "Ctrl-D" "Ctrl-T"
  DoTest("foo", "i\\ctrl-t" , "  foo");
  DoTest(" foo", "i\\ctrl-d", "foo");
  DoTest("foo\nbar", "i\\ctrl-t\\ctrl-d","foo\nbar" );
}


 // There are written tests that fall.
 // They are disabled in order to be able to check all others working tests.
void ViModeTest::NormalModeFallingTests()
{
//  DoTest("1 2\n2 1", "lld#", "1 \n2 1");
//  DoTest("12345678", "lv3lyx", "1345678");
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
  DoTest("1 1 1", "2#x","1 1 ");
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

}

void ViModeTest::NormalModeCommandsTest() {

  // Testing "J"
  DoTest("foo\nbar", "J", "foo bar");

  // Testing "dd"
  DoTest("foo\nbar", "dd", "bar");
  DoTest("foo\nbar", "2dd", "");

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

}


void ViModeTest::NormalModeControlTests() {
  // Testing "Ctrl-x"
  DoTest("150", "101\\ctrl-x", "49");
  DoTest("1", "\\ctrl-x\\ctrl-x\\ctrl-x\\ctrl-x", "-3");
  DoTest("0xabcdef", "1000000\\ctrl-x","0x9c8baf" );

  // Testing "Ctrl-a"
  DoTest("150", "101\\ctrl-a", "251");
  DoTest("1000", "\\ctrl-ax", "100");
  DoTest("-1", "1\\ctrl-a", "0");
  DoTest("-1", "l1\\ctrl-a", "0");

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

    // Testing ":s" (sed)
    DoTest("foo","\\:s/foo/bar\\","bar");
    DoTest("foobarbaz","\\:s/bar/***\\","foo***baz");
    DoTest("foo","\\:s/bar/baz\\","foo");
    DoTest("foo\nfoo\nfoo","j\\:s/foo/bar\\", "foo\nbar\nfoo");
    DoTest("foo\nfoo\nfoo","2jma2k\\:'a,'as/foo/bar\\", "foo\nfoo\nbar");
    DoTest("foo\nfoo\nfoo","\\:%s/foo/bar\\","bar\nbar\nbar");
    DoTest("foo\nfoo\nfoo","\\:2,3s/foo/bar\\","foo\nbar\nbar");
    DoTest("foo\nfoo\nfoo\nfoo", "j2lmajhmbgg\\:'a,'bs/foo/bar\\","foo\nbar\nbar\nfoo");
    DoTest("foo\nfoo\nfoo\nfoo", "jlma2jmbgg\\:'b,'as/foo/bar\\","foo\nbar\nbar\nbar");


    DoTest("foo\nfoo\nfoo","\\:2s/foo/bar\\", "foo\nbar\nfoo");
    DoTest("foo\nfoo\nfoo","2jmagg\\:'as/foo/bar\\","foo\nfoo\nbar");
    DoTest("foo\nfoo\nfoo", "\\:$s/foo/bar","foo\nfoo\nbar");

}

// kate: space-indent on; indent-width 2; replace-tabs on;
