/* This file is part of the KDE libraries

   Copyright (C) 2011 Kuzmich Svyatoslav
   Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>

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
#include <kateundomanager.h>
#include <kateview.h>
#include "kateconfig.h"
#include <kateglobal.h>
#include "katebuffer.h"
#include "katevikeyparser.h"
#include <kateviglobal.h>
#include <katevinormalmode.h>
#include <katevikeymapper.h>
#include <kateviemulatedcommandbar.h>
#include "kateviewhelpers.h"
#include "ktexteditor/attribute.h"
#include <ktexteditor/codecompletionmodel.h>
#include <katewordcompletion.h>
#include <katecompletionwidget.h>

#include <QtGui/QLabel>
#include <QtGui/QCompleter>
#include <QtGui/QMainWindow>
#include <qlayout.h>
#include <kcolorscheme.h>
#include <klocalizedstring.h>
#include <kstandardaction.h>
#include <kactioncollection.h>
#include <kconfiggroup.h>

QTEST_KDEMAIN(ViModeTest, GUI)

using namespace KTextEditor;

WindowKeepActive::WindowKeepActive(QMainWindow* mainWindow):  m_mainWindow(mainWindow)
{

}

bool WindowKeepActive::eventFilter(QObject* object, QEvent* event)
{
  Q_UNUSED(object);
  if (event->type() == QEvent::WindowDeactivate)
  {
    // With some combinations of Qt and Xvfb, invoking/ dismissing a popup
    // will deactiveate the m_mainWindow, preventing it from receiving shortcuts.
    // If we detect this, set it back to being the active window again.
    event->ignore();
    QApplication::setActiveWindow(m_mainWindow);
    return true;
  }
  return false;
}

FailsIfSlotNotCalled::FailsIfSlotNotCalled(): QObject(), m_slotWasCalled(false)
{

}

FailsIfSlotNotCalled::~FailsIfSlotNotCalled()
{
  QVERIFY(m_slotWasCalled);
}

void FailsIfSlotNotCalled::slot()
{
  m_slotWasCalled = true;
}

FailsIfSlotCalled::FailsIfSlotCalled(const QString& failureMessage): QObject(), m_failureMessage(failureMessage)
{

}

void FailsIfSlotCalled::slot()
{
  kDebug(13070) << "Here";
  QFAIL(m_failureMessage.toAscii());
}

FakeCodeCompletionTestModel::FakeCodeCompletionTestModel(KTextEditor::View* parent)
  : KTextEditor::CodeCompletionModel(parent),
    m_kateView(qobject_cast<KateView*>(parent)),
    m_kateDoc(parent->document()),
    m_removeTailOnCompletion(false),
    m_failTestOnInvocation(false),
    m_wasInvoked(false)
{
    Q_ASSERT(m_kateView);
    setRowCount(3);
    cc()->setAutomaticInvocationEnabled(false);
    cc()->unregisterCompletionModel(KateGlobal::self()->wordCompletionModel()); //would add additional items, we don't want that in tests
    connect(parent->document(), SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
            this, SLOT(textInserted(KTextEditor::Document*,KTextEditor::Range)));
    connect(parent->document(), SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range)),
            this, SLOT(textRemoved(KTextEditor::Document*,KTextEditor::Range)));
}
void FakeCodeCompletionTestModel::setCompletions(const QStringList& completions)
{
  QStringList sortedCompletions = completions;
  qSort(sortedCompletions);
  Q_ASSERT(completions == sortedCompletions && "QCompleter seems to sort the items, so it's best to provide them pre-sorted so it's easier to predict the order");
  setRowCount(sortedCompletions.length());
  m_completions = completions;
}
void FakeCodeCompletionTestModel::setRemoveTailOnComplete(bool removeTailOnCompletion)
{
  m_removeTailOnCompletion = removeTailOnCompletion;
}
void FakeCodeCompletionTestModel::setFailTestOnInvocation(bool failTestOnInvocation)
{
  m_failTestOnInvocation = failTestOnInvocation;
}
bool FakeCodeCompletionTestModel::wasInvoked()
{
  return m_wasInvoked;
}
void FakeCodeCompletionTestModel::clearWasInvoked()
{
  m_wasInvoked = false;
}
/**
  * A more reliable form of setAutomaticInvocationEnabled().
  */
void FakeCodeCompletionTestModel::forceInvocationIfDocTextIs(const QString& desiredDocText)
{
  m_forceInvocationIfDocTextIs = desiredDocText;
}
void FakeCodeCompletionTestModel::doNotForceInvocation()
{
  m_forceInvocationIfDocTextIs.clear();
}
QVariant FakeCodeCompletionTestModel::data(const QModelIndex& index, int role) const
{
  m_wasInvoked = true;
  if (m_failTestOnInvocation)
  {
    failTest();
    return QVariant();
  }
  // Order is important, here, as the completion widget seems to do its own sorting.
  if (role == Qt::DisplayRole)
  {
    if (index.column() == Name)
        return QString(m_completions[index.row()]);
  }
  return QVariant();
}
void FakeCodeCompletionTestModel::executeCompletionItem(Document* document, const Range& word, int row) const
{
  kDebug(13070) << "word: " << word << "(" << document->text(word) << ")";
  const Cursor origCursorPos = m_kateView->cursorPosition();
  const QString textToInsert = m_completions[row];
  const QString textAfterCursor = document->text(Range(word.end(), Cursor(word.end().line(), document->lineLength(word.end().line()))));
  document->removeText(Range(word.start(), origCursorPos));
  const int lengthStillToRemove = word.end().column() - origCursorPos.column();
  QString actualTextInserted = textToInsert;
  // Merge brackets?
  const QString noArgFunctionCallMarker = "()";
  const QString withArgFunctionCallMarker = "(...)";
  const bool endedWithSemiColon  = textToInsert.endsWith(';');
  if (textToInsert.contains(noArgFunctionCallMarker) || textToInsert.contains(withArgFunctionCallMarker))
  {
    Q_ASSERT(m_removeTailOnCompletion && "Function completion items without removing tail is not yet supported!");
    const bool takesArgument = textToInsert.contains(withArgFunctionCallMarker);
    // The code for a function call to a function taking no arguments.
    const QString justFunctionName = textToInsert.left(textToInsert.indexOf("("));

    QRegExp whitespaceThenOpeningBracket("^\\s*(\\()");
    int openingBracketPos = -1;
    if (textAfterCursor.contains(whitespaceThenOpeningBracket))
    {
      openingBracketPos = whitespaceThenOpeningBracket.pos(1) + word.start().column() + justFunctionName.length() + 1 + lengthStillToRemove;
    }
    const bool mergeOpenBracketWithExisting = (openingBracketPos != -1) && !endedWithSemiColon;
    // Add the function name, for now: we don't yet know if we'll be adding the "()", too.
    document->insertText(word.start(), justFunctionName);
    if (mergeOpenBracketWithExisting)
    {
      // Merge with opening bracket.
      actualTextInserted = justFunctionName;
      m_kateView->setCursorPosition(Cursor(word.start().line(), openingBracketPos));
    }
    else
    {
      // Don't merge.
      const QString afterFunctionName = endedWithSemiColon ? "();" : "()";
      document->insertText(Cursor(word.start().line(), word.start().column() + justFunctionName.length()), afterFunctionName);
      if (takesArgument)
      {
        // Place the cursor immediately after the opening "(" we just added.
        m_kateView->setCursorPosition(Cursor(word.start().line(), word.start().column() + justFunctionName.length() + 1));
      }
    }
  }
  else
  {
    // Plain text.
    document->insertText(word.start(), textToInsert);
  }
  if (m_removeTailOnCompletion)
  {
    const int tailLength = word.end().column() - origCursorPos.column();
    const Cursor tailStart = Cursor(word.start().line(), word.start().column() + actualTextInserted.length());
    const Cursor tailEnd = Cursor(tailStart.line(), tailStart.column() + tailLength);
    document->removeText(Range(tailStart, tailEnd));
  }
}

KTextEditor::CodeCompletionInterface * FakeCodeCompletionTestModel::cc( ) const
{
  return dynamic_cast<KTextEditor::CodeCompletionInterface*>(const_cast<QObject*>(QObject::parent()));
}

void FakeCodeCompletionTestModel::failTest() const
{
  QFAIL("Shouldn't be invoking me!");
}

void FakeCodeCompletionTestModel::textInserted(Document* document, Range range)
{
  Q_UNUSED(document);
  Q_UNUSED(range);
  checkIfShouldForceInvocation();
}

void FakeCodeCompletionTestModel::textRemoved(Document* document, Range range)
{
  Q_UNUSED(document);
  Q_UNUSED(range);
  checkIfShouldForceInvocation();
}

void FakeCodeCompletionTestModel::checkIfShouldForceInvocation()
{
  if (m_forceInvocationIfDocTextIs.isEmpty())
  {
    return;
  }

  if (m_kateDoc->text() == m_forceInvocationIfDocTextIs)
  {
    m_kateView->completionWidget()->userInvokedCompletion();
    ViModeTest::waitForCompletionWidgetToActivate(m_kateView);
  }
}

ViModeTest::ViModeTest() {
  kate_document = 0;
  kate_view = 0;

  mainWindow = new QMainWindow;
  mainWindowLayout = new QVBoxLayout(mainWindow);
  mainWindow->setLayout(mainWindowLayout);

  m_codesToModifiers.insert("ctrl", Qt::ControlModifier);
  m_codesToModifiers.insert("alt", Qt::AltModifier);
  m_codesToModifiers.insert("meta", Qt::MetaModifier);
  m_codesToModifiers.insert("keypad", Qt::KeypadModifier);

  m_codesToSpecialKeys.insert("backspace", Qt::Key_Backspace);
  m_codesToSpecialKeys.insert("esc", Qt::Key_Escape);
  m_codesToSpecialKeys.insert("return", Qt::Key_Return);
  m_codesToSpecialKeys.insert("enter", Qt::Key_Enter);
  m_codesToSpecialKeys.insert("left", Qt::Key_Left);
  m_codesToSpecialKeys.insert("right", Qt::Key_Right);
  m_codesToSpecialKeys.insert("up", Qt::Key_Up);
  m_codesToSpecialKeys.insert("down", Qt::Key_Down);
  m_codesToSpecialKeys.insert("home", Qt::Key_Home);
  m_codesToSpecialKeys.insert("end", Qt::Key_End);
  m_codesToSpecialKeys.insert("delete", Qt::Key_Delete);
  m_codesToSpecialKeys.insert("insert", Qt::Key_Insert);
  m_codesToSpecialKeys.insert("pageup", Qt::Key_PageUp);
  m_codesToSpecialKeys.insert("pagedown", Qt::Key_PageDown);

}

void ViModeTest::init()
{
  delete kate_view;
  delete kate_document;

  kate_document = new KateDocument(false, false, false, 0, NULL);
  kate_view = new KateView(kate_document, mainWindow);
  mainWindowLayout->addWidget(kate_view);
  kate_view->config()->setViInputMode(true);
  Q_ASSERT(kate_view->viInputMode());
  vi_input_mode_manager = kate_view->getViInputModeManager();
  kate_document->config()->setShowSpaces(true); // Flush out some issues in the KateRenderer when rendering spaces.

  connect(kate_document, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
          this, SLOT(textInserted(KTextEditor::Document*,KTextEditor::Range)));
  connect(kate_document, SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range)),
          this, SLOT(textRemoved(KTextEditor::Document*,KTextEditor::Range)));
}

Qt::KeyboardModifier ViModeTest::parseCodedModifier(const QString& string, int startPos, int* destEndOfCodedModifier)
{
  foreach(const QString& modifierCode, m_codesToModifiers.keys())
  {
    // The "+2" is from the leading '\' and the trailing '-'
    if (string.mid(startPos, modifierCode.length() + 2) == QString("\\") + modifierCode + "-")
    {
      if (destEndOfCodedModifier)
      {
        // destEndOfCodeModifier lies on the trailing '-'.
        *destEndOfCodedModifier = startPos + modifierCode.length() + 1;
        Q_ASSERT(string[*destEndOfCodedModifier] == '-');
      }
      return m_codesToModifiers.value(modifierCode);
    }
  }
  return Qt::NoModifier;
}

Qt::Key ViModeTest::parseCodedSpecialKey(const QString& string, int startPos, int* destEndOfCodedKey)
{
  foreach (const QString& specialKeyCode, m_codesToSpecialKeys.keys())
  {
    // "+1" is for the leading '\'.
    if (string.mid(startPos, specialKeyCode.length() + 1) == QString("\\") + specialKeyCode)
    {
      if (destEndOfCodedKey)
      {
        *destEndOfCodedKey = startPos + specialKeyCode.length();
      }
      return m_codesToSpecialKeys.value(specialKeyCode);
    }
  }
  return Qt::Key_unknown;
}


ViModeTest::~ViModeTest() {
  delete kate_document;
//  delete kate_view;
}

void ViModeTest::BeginTest(const QString& original_text) {
  vi_input_mode_manager->viEnterNormalMode();
  vi_input_mode_manager = kate_view->resetViInputModeManager();
  kate_document->setText(original_text);
  kate_document->undoManager()->clearUndo();
  kate_document->undoManager()->clearRedo();
  kate_view->setCursorPosition(Cursor(0,0));
  m_firstBatchOfKeypressesForTest = true;
}

void ViModeTest::FinishTest_(int line, const QString& expected_text, ViModeTest::Expectation expectation, const QString& failureReason)
{
  if (expectation == ShouldFail)
  {
    if (!QTest::qExpectFail("", failureReason.toLocal8Bit(), QTest::Continue, __FILE__, line)) return;
    qDebug() << "Actual text:\n\t" << kate_document->text() << "\nShould be (for this test to pass):\n\t" << expected_text;
  }
  if (!QTest::qCompare(kate_document->text(), expected_text, "kate_document->text()", "expected_text", __FILE__, line)) return;
  Q_ASSERT(!emulatedCommandBarTextEdit()->isVisible() && "Make sure you close the command bar before the end of a test!");
}

#define FinishTest(...) FinishTest_( __LINE__, __VA_ARGS__ )


void ViModeTest::TestPressKey(QString str) {
  if (m_firstBatchOfKeypressesForTest)
  {
    qDebug() << "\n\n>>> running command " << str << " on text " << kate_document->text();
  }
  else
  {
    qDebug() << "\n>>> running further keypresses " << str << " on text " << kate_document->text();
  }
  m_firstBatchOfKeypressesForTest = false;

  for (int i = 0; i< str.length(); i++) {
    Qt::KeyboardModifiers keyboard_modifier = Qt::NoModifier;
    QString key;
    int keyCode = -1;
    // Looking for keyboard modifiers
    if (str[i] == QChar('\\')) {
        int endOfModifier = -1;
        Qt::KeyboardModifier parsedModifier = parseCodedModifier(str, i, &endOfModifier);
        int endOfSpecialKey = -1;
        Qt::Key parsedSpecialKey = parseCodedSpecialKey(str, i, &endOfSpecialKey);
        if (parsedModifier != Qt::NoModifier)
        {
          keyboard_modifier = parsedModifier;
          // Move to the character after the '-' in the modifier.
          i = endOfModifier + 1;
          // Is this a modifier plus special key?
          int endOfSpecialKeyAfterModifier = -1;
          const Qt::Key parsedCodedSpecialKeyAfterModifier = parseCodedSpecialKey(str, i, &endOfSpecialKeyAfterModifier);
          if (parsedCodedSpecialKeyAfterModifier != Qt::Key_unknown)
          {
             key = QString(parsedCodedSpecialKeyAfterModifier);
             keyCode = parsedCodedSpecialKeyAfterModifier;
             i = endOfSpecialKeyAfterModifier;
          }
        } else if (parsedSpecialKey != Qt::Key_unknown) {
            key = QString(parsedSpecialKey);
            keyCode = parsedSpecialKey;
            i = endOfSpecialKey;
        } else if (str.mid(i,2) == QString("\\:")) {
           int start_cmd = i+2;
           for( i+=2 ; true ; i++ )
           {
             if (str.at(i) == '\\')
             {
               if (i + 1 < str.length() && str.at(i + 1) == '\\')
               {
                 // A backslash within a command; skip.
                 i += 2;
               }
               else
               {
                 // End of command.
                 break;
               }
             }
           }
           const QString commandToExecute = str.mid(start_cmd,i-start_cmd).replace("\\\\", "\\");
           kDebug(13070) << "Executing command directly from ViModeTest:\n" << commandToExecute;
           kate_view->cmdLineBar()->execute(commandToExecute);
           // We've handled the command; go back round the loop, avoiding sending
           // the closing \ to vi_input_mode_manager.
           continue;
        } else if (str.mid(i, 2) == QString("\\\\"))
        {
            key = QString("\\");
            keyCode = Qt::Key_Backslash;
            i++;
        }
        else {
            assert(false); //Do not use "\" in tests except for modifiers, command mode (\\:) and literal backslashes "\\\\")
        }
    }

    if (keyCode == -1)
    {
      key = str[i];
      keyCode = key[0].unicode();
      // Kate Vim mode's internals identifier e.g. CTRL-C by Qt::Key_C plus the control modifier,
      // so we need to translate e.g. 'c' to Key_C.
      if (key[0].isLetter())
      {
        if (key[0].toLower() == key[0])
        {
          keyCode = keyCode - 'a' + Qt::Key_A;
        }
        else
        {
          keyCode = keyCode - 'A' + Qt::Key_A;
          keyboard_modifier |= Qt::ShiftModifier;
        }
      }
    }

    QKeyEvent *key_event = new QKeyEvent(QEvent::KeyPress, keyCode, keyboard_modifier, key);
    // Attempt to simulate how Qt usually sends events - typically, we want to send them
    // to kate_view->focusProxy() (which is a KateViewInternal).
    QWidget *destWidget = NULL;
    if (QApplication::activePopupWidget())
    {
      // According to the docs, the activePopupWidget, if present, takes all events.
      destWidget = QApplication::activePopupWidget();
    }
    else
      if (QApplication::focusWidget())
    {
      if (QApplication::focusWidget()->focusProxy())
      {
        destWidget = QApplication::focusWidget()->focusProxy();
      }
      else
      {
        destWidget = QApplication::focusWidget();
      }
    }
    else
    {
      destWidget = kate_view->focusProxy();
    }
    QApplication::postEvent(destWidget, key_event);
    QApplication::sendPostedEvents();
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
void ViModeTest::DoTest_(int line, QString original_text,
    QString command,
    QString expected_text, Expectation expectation, const QString& failureReason) {

  BeginTest(original_text);
  TestPressKey(command);
  FinishTest_(line, expected_text, expectation, failureReason);
}

#define DoTest(...) DoTest_(__LINE__, __VA_ARGS__)


void ViModeTest::VisualModeTests() {
    DoTest("foobar", "vlllx", "ar");
    DoTest("foo\nbar", "Vd", "bar");
    DoTest("1234\n1234\n1234", "l\\ctrl-vljjd", "14\n14\n14");
    QCOMPARE(kate_view->blockSelection(), false);

    DoTest("12345678", "lv3lyx", "1345678");
    DoTest("12345678", "$hv3hyx", "1235678");
    DoTest("aaa\nbbb", "lvj~x", "aA\nBBb");
    DoTest("123\n456", "jlvkyx", "13\n456");
    DoTest("12\n34","lVjyx", "2\n34");
    DoTest("ab\ncd","jVlkgux", "a\ncd");
    DoTest("ABCD\nABCD\nABCD\nABCD","lj\\ctrl-vjlgux","ABCD\nAcD\nAbcD\nABCD");
    DoTest("abcd\nabcd\nabcd\nabcd","jjjlll\\ctrl-vkkhgUx","abcd\nabD\nabCD\nabCD");
    // Cancelling visual mode should not reset the cursor.
    DoTest("12345678", "lv3l\\escx", "1234678");
    DoTest("12345678", "lv3l\\ctrl-cx", "1234678");
    // Don't forget to clear the flag that says we shouldn't reset the cursor, though!
    DoTest("12345678", "lv3l\\ctrl-cxv3lyx", "123478");
    DoTest("12345678", "y\\escv3lyx", "2345678");

    // Testing "d"
    DoTest("foobarbaz","lvlkkjl2ld","fbaz");
    DoTest("foobar","v$d","");
    DoTest("foo\nbar\nbaz","jVlld","foo\nbaz");
    DoTest("01\n02\n03\n04\n05","Vjdj.","03");

    // testing Del key
    DoTest("foobarbaz","lvlkkjl2l\\delete","fbaz");

    // Testing "D"
    DoTest("foo\nbar\nbaz","lvjlD","baz");
    DoTest("foo\nbar", "l\\ctrl-vjD","f\nb");
    DoTest("foo\nbar","VjkD","bar");

    // Testing "gU", "U"
    DoTest("foo bar", "vwgU", "FOO Bar");
    DoTest("foo\nbar\nbaz", "VjjU", "FOO\nBAR\nBAZ");
    DoTest("foo\nbar\nbaz", "\\ctrl-vljjU","FOo\nBAr\nBAz");
    DoTest("aaaa\nbbbb\ncccc", "\\ctrl-vljgUjll.", "AAaa\nBBBB\nccCC");

    // Testing "gu", "u"
    DoTest("TEST", "Vgu", "test");
    DoTest("TeSt", "vlgu","teSt");
    DoTest("FOO\nBAR\nBAZ", "\\ctrl-vljju","foO\nbaR\nbaZ");
    DoTest("AAAA\nBBBB\nCCCC\nDDDD", "vjlujjl.", "aaaa\nbbBB\nCccc\ndddD");

    // Testing "g~"
    DoTest("fOo bAr", "Vg~", "FoO BaR");
    DoTest("foo\nbAr\nxyz", "l\\ctrl-vjjg~", "fOo\nbar\nxYz");

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
    DoTest("indent\nrepeat", "V>.", "    indent\nrepeat");
    DoTest("indent\nrepeat", "Vj>.", "    indent\n    repeat");
    DoTest("indent\nrepeat\non\nothers", "Vj>jj.", "  indent\n  repeat\n  on\n  others");

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
    DoTest("a", "r\\ctrl-[", "a");
    DoTest("a", "r\\keypad-0", "0");
    DoTest("a", "r\\keypad-9", "9");
    DoTest("foo\nbar", "l\\ctrl-vjr\\keypad-9", "f9o\nb9r");

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

    // Testing "J"
    DoTest("foo\nbar\nxyz\nbaz\n123\n456", "jVjjjJ", "foo\nbar xyz baz 123\n456");
    DoTest("foo\nbar\nxyz\nbaz\n123\n456", "jjjjVkkkJ", "foo\nbar xyz baz 123\n456");
    DoTest("foo\nbar\nxyz\nbaz\n123456\n789", "jjjjVkkkJrX", "foo\nbar xyz bazX123456\n789");
    DoTest("foo\nbar\nxyz\n", "VGJ", "foo bar xyz ");

    // Testing undo behaviour with c and cc
    DoTest("foo", "ciwbar\\escu", "foo");
    DoTest("foo", "ccbar\\escu", "foo");

    // Regression test for ][ in Visual Mode.
    DoTest("foo {\n\n}", "lV][d", "");

    // Misc tests for motions starting in front of the Visual Mode start point.
    DoTest("{foo}", "lvb%x", "{");
    DoTest("foo bar", "wvbfax", "foo r");
    DoTest("(foo bar)", "wwv^%x", "(foo ");

    // * and #
    DoTest("foo foo", "v*x", "oo");
    DoTest("foo foo", "wv#x", "oo");

    // Regression test for gv.
    DoTest("foo\nbar\nxyz", "l\\ctrl-vjj\\ctrl-cgvr.", "f.o\nb.r\nx.z");

    // Quick test that "{" and "}" motions work in visual mode
    DoTest("foo\n\n\nbar\n","v}}d","");
    DoTest("\n\nfoo\nbar\n","jjjv{d","\nar\n");

    // ctrl-left and ctrl-right
    DoTest("foo bar xyz", "v\\ctrl-\\rightd", "ar xyz");
    DoTest("foo bar xyz", "$v\\ctrl-\\leftd", "foo bar ");

    // Pasting should replace the current selection.
    DoTest("foo bar xyz", "yiwwviwp", "foo foo xyz");
    // Undo should undo both paste and removal of selection.
    DoTest("foo bar xyz", "yiwwviwpu", "foo bar xyz");
    DoTest("foo\nbar\n123\nxyz", "yiwjVjp", "foo\nfoo\nxyz");

    // Set the *whole* selection to the given text object, even if the cursor is no
    // longer at the position where Visual Mode was started.
    // This seems to work (in Vim) only when the start of the given text object occurs before them
    // start position of Visual Mode.
    DoTest("{\nfoo\nbar\nxyz\n}", "jjvliBd", "{\n}");
    DoTest("foo[hello]", "fhlvli[d", "foo[]");
    DoTest("foo(hello)", "fhlvli(d", "foo()");
    DoTest("foo<hello>", "fhlvli<d", "foo<>");
    DoTest("foo\"hello\"", "fhlvli\"d", "foo\"\"");
    DoTest("foo'hello'", "fhlvli'd", "foo''");
    // A couple of spot tests, where the beginning of the text object occurs after the start position of Visual Mode;
    // the selection should  remain unchanged if we the text object motion is triggered, here.
    DoTest("foobarxyz\n(12345)", "llvjibd", "fo345)");
    DoTest("foobarxyz\n{12345}", "llvjiBd", "fo345}");
    // Cursor should end up at the end of the text object.
    DoTest("foo[hello]", "fhlvli[\\escrX", "foo[hellX]");
    // Ensure we reset the flag that says that the current motion is a text object!
    DoTest("foo[hello]", "jfhlvli[^d", "ello]");

    // Test that selecting a range "externally" to Vim (i.e. via the mouse, or one of the ktexteditor api's)
    // switches us into Visual Mode.
    BeginTest("foo bar");
    kate_view->setSelection(Range(0, 1, 0 , 4)); // Actually selects "oo " (i.e. without the "b").
    TestPressKey("d");
    FinishTest("fbar");
    // Undoing a command that we executed in Visual Mode should also return us to Visual Mode.
    BeginTest("foo bar");
    TestPressKey("lvllldu");
    QCOMPARE(kate_view->getCurrentViMode(), VisualMode);
    QCOMPARE(kate_view->selectionText(), QString("oo b"));
    FinishTest("foo bar");

    // Test that, if kate_view has a selection before the Vi mode stuff is loaded, then we
    // end up in Visual Mode: this mimics what happens if we click on a Find result in
    // KDevelop's "grepview" plugin.
    delete kate_view;
    kate_view = new KateView(kate_document, mainWindow);
    mainWindowLayout->addWidget(kate_view);
    kate_document->setText("foo bar");
    kate_view->setSelection(Range(Cursor(0, 1), Cursor(0, 4)));
    QCOMPARE(kate_document->text(kate_view->selectionRange()), QString("oo "));
    kate_view->config()->setViInputMode(true);
    kDebug(13070) << "selected: " << kate_document->text(kate_view->selectionRange());
    QVERIFY(kate_view->viInputMode());
    vi_input_mode_manager = kate_view->getViInputModeManager();
    QVERIFY(vi_input_mode_manager->getCurrentViMode() == VisualMode);
    TestPressKey("l");
    QCOMPARE(kate_document->text(kate_view->selectionRange()), QString("oo b"));
    TestPressKey("d");
    QCOMPARE(kate_document->text(), QString("far"));

    // Regression test for bug 309191
    DoTest("foo bar", "vedud", " bar");

    // test returning to correct mode when selecting ranges with mouse
    BeginTest("foo bar\nbar baz");
    TestPressKey("i"); // get me into insert mode
    kate_view->setSelection(Range(0, 1, 1, 4));
    QCOMPARE((int)vi_input_mode_manager->getCurrentViMode(), (int)VisualMode);
    kate_view->setSelection(Range::invalid());
    QCOMPARE((int)vi_input_mode_manager->getCurrentViMode(), (int)InsertMode);
    TestPressKey("\\esc"); // get me into normal mode
    kate_view->setSelection(Range(0, 1, 1, 4));
    QCOMPARE((int)vi_input_mode_manager->getCurrentViMode(), (int)VisualMode);
    kate_view->setSelection(Range::invalid());
    QCOMPARE((int)vi_input_mode_manager->getCurrentViMode(), (int)NormalMode);

    // proper yanking in block mode
    {
      BeginTest("aaaa\nbbbb\ncccc\ndddd");
      TestPressKey("lj\\ctrl-vljy");
      KateBuffer &buffer = kate_document->buffer();
      QList<Kate::TextRange *> ranges = buffer.rangesForLine(1, kate_view, true);
      QCOMPARE(ranges.size(), 1);
      const KTextEditor::Range &range = ranges[0]->toRange();
      QCOMPARE(range.start().column(), 1);
      QCOMPARE(range.end().column(), 3);
    }

    // proper selection in block mode after switching to cmdline
    {
      BeginTest("aaaa\nbbbb\ncccc\ndddd");
      TestPressKey("lj\\ctrl-vlj:");
      QCOMPARE(kate_view->selectionText(), QString("bb\ncc"));
    }

    // BUG #328277 - make sure kate doesn't crash
    DoTest("aaa\nbbb", "Vj>u>.", "    aaa\n    bbb", ShouldFail, "Crash is fixed, but correct repeat behaviour in this scenario is yet to be implemented");

    // Selection with regular motions.
    DoTest("Three. Different. Sentences.", "v)cX", "Xifferent. Sentences.");
    DoTest("Three. Different. Sentences.", "v)cX", "Xifferent. Sentences.");
    DoTest("Three. Different. Sentences.", "v)cX", "Xifferent. Sentences.");
    DoTest("Three. Different. Sentences.", "viWcX", "X Different. Sentences.");
    DoTest("Three. Different. Sentences.", "viwcX", "X. Different. Sentences.");
    DoTest("Three. Different. Sentences.", "vaWcX", "XDifferent. Sentences.");
    DoTest("Three. Different. Sentences.", "vawcX", "X. Different. Sentences.");
    DoTest("Three. Different. Sentences.", "vascX", "XDifferent. Sentences.");
    DoTest("Three. Different. Sentences.", "viscX", "X Different. Sentences.");
    DoTest("Three. Different. Sentences.", "vapcX", "X");
    DoTest("Three. Different. Sentences.", "vipcX", "X");
}

void ViModeTest::ReplaceModeTests()
{
  // TODO - more of these!
  DoTest("foo bar", "R\\ctrl-\\rightX", "foo Xar");
  DoTest("foo bar", "R\\ctrl-\\right\\ctrl-\\rightX", "foo barX");
  DoTest("foo bar", "R\\ctrl-\\leftX", "Xoo bar");
  DoTest("foo bar", "R\\ctrl-\\left\\delete", "oo bar");

  DoTest("foo\nbar\nbaz", "R\\downX", "foo\nXar\nbaz");
  DoTest("foo\nbar\nbaz", "jjR\\upX", "foo\nXar\nbaz");
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
  // Ensure that the flag that says that counted repeats should begin on a new line is reset.
  DoTest("foo", "obar\\ctrl-c5ixyz\\esc", "foo\nbaxyzxyzxyzxyzxyzr");
  DoTest("foo", "obar\\ctrl-cgg\\ctrl-vlljAxyz\\esc5i123\\esc", "fooxy123123123123123z\nbarxyz");
  DoTest("foo foo foo", "c3wbar\\esc", "bar");
  DoTest("abc", "lOxyz", "xyz\nabc");

  // Testing "Ctrl-w"
  DoTest("foobar", "$i\\ctrl-w", "r");
  DoTest("foobar\n", "A\\ctrl-w", "\n");
  DoTest("   foo", "i\\ctrl-wX\\esc", "X   foo");
  DoTest("   foo", "lli\\ctrl-wX\\esc", "X foo");

  // Testing "Ctrl-u"
  DoTest("", "i\\ctrl-u", "");
  DoTest("foobar", "i\\ctrl-u", "foobar");
  DoTest("foobar", "fbi\\ctrl-u", "bar");
  DoTest("foobar\nsecond", "ji\\ctrl-u", "foobarsecond");
  DoTest("foobar\n  second", "jwi\\ctrl-u", "foobar\nsecond");
  DoTest("foobar\n  second", "jfci\\ctrl-u", "foobar\n  cond");
  DoTest("foobar\n  second", "j$a\\ctrl-u", "foobar\n  ");

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
  DoTest("barbaz", "\"ay3li\\ctrl-raX", "barXbarbaz");
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

  // Testing ctrl-left and ctrl-right.
  DoTest("foo bar", "i\\ctrl-\\rightX\\esc", "foo Xbar");
  DoTest("foo bar", "i\\ctrl-\\right\\ctrl-\\rightX\\esc", "foo barX");
  DoTest("foo", "\\endi\\ctrl-\\left\\ctrl-\\leftX", "Xfoo"); // we crashed here before

  // Enter/ Return.
  DoTest("", "ifoo\\enterbar", "foo\nbar");
  DoTest("", "ifoo\\returnbar", "foo\nbar");

  // Insert key
  DoTest("", "\\insertfoo", "foo");

  // Test that our test driver can handle newlines during insert mode :)
  DoTest("", "ia\\returnb", "a\nb");

  DoTest("foo bar", "i\\home\\delete", "oo bar");
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
  DoTest("bar(\n123", "llwrX", "barX\n123");


  // Testing "W"
  DoTest("bar", "Wx", "ba");
  DoTest("foo bar", "Wx", "foo ar");
  DoTest("foo bar", "2lWx","foo ar");
  DoTest("quux(foo, bar, baz);", "WxWx","quux(foo, ar, az);");
  DoTest("foo\nbar\nbaz", "WxWx","foo\nar\naz");
  DoTest(" foo(bar xyz", "Wx"," oo(bar xyz");

  // Testing "b"
  DoTest("bar", "lbx", "ar");
  DoTest("foo bar baz", "2wbx", "foo ar baz");
  DoTest("foo bar", "w20bx","oo bar");
  DoTest("quux(foo, bar, baz);", "2W4l2bx2bx","quux(foo, ar, az);");
  DoTest("foo\nbar\nbaz", "WWbx","foo\nar\nbaz");
  DoTest("  foo", "lbrX", "X foo");
  DoTest("  foo", "llbrX", "X foo");

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

  // Testing "#" & "*"
  DoTest("1 1 1", "2#x","1  1");
  DoTest("foo bar foo bar", "#xlll#x","foo ar oo bar");
  DoTest("(foo (bar (foo( bar))))", "#xll#x","(foo (ar (oo( bar))))");
  DoTest("(foo (bar (foo( bar))))", "*x","(foo (bar (oo( bar))))");
  DoTest("foo bar foobar foo", "*rX", "foo bar foobar Xoo"); // Whole word only.
  DoTest("foo bar foobar foo", "$#rX", "Xoo bar foobar foo"); // Whole word only.
  DoTest("fOo foo fOo", "*rX", "fOo Xoo fOo"); // Case insensitive.
  DoTest("fOo foo fOo", "$#rX", "fOo Xoo fOo"); // Case insensitive.
  DoTest("fOo foo fOo", "*ggnrX", "fOo Xoo fOo"); // Flag that the search to repeat is case insensitive.
  DoTest("fOo foo fOo", "$#ggNrX", "fOo Xoo fOo"); // Flag that the search to repeat is case insensitive.
  DoTest("bar foo", "$*rX", "bar Xoo");
  DoTest("bar foo", "$#rX", "bar Xoo");
  // Test that calling # on the last, blank line of a document does not go into an infinite loop.
  DoTest("foo\n", "j#", "foo\n");

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
  DoTest("foo", "llgerX", "Xoo");
  DoTest("   foo", "$gerX", "X  foo");
  DoTest("   foo foo", "$2gerX", "X  foo foo");

  // Testing "gE"
  DoTest("quux(foo, bar, baz);", "9lgExgEx$gEx", "quux(fo bar baz);");
  DoTest("   foo", "$gErX", "X  foo");
  DoTest("   foo foo", "$2gErX", "X  foo foo");
  DoTest("   !foo$!\"", "$gErX", "X  !foo$!\"");
  DoTest("   !foo$!\"", "$2gErX", "X  !foo$!\"");

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

  // ctrl-left and ctrl-right.
  DoTest("foo bar xyz", "\\ctrl-\\rightrX", "foo Xar xyz");
  DoTest("foo bar xyz", "$\\ctrl-\\leftrX", "foo bar Xyz");

  // Enter/ Return.
  DoTest("foo\n\t \t bar", "\\enterr.", "foo\n\t \t .ar");
  DoTest("foo\n\t \t bar", "\\returnr.", "foo\n\t \t .ar");

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

  // Inner/ A Word.
  DoTest("", "diw", "");
  DoTest(" ", "diw", "");
  DoTest("  ", "diw", "");
  DoTest("foo", "daw", "");
  DoTest("foo", "ldaw", "");
  DoTest("foo", "cawxyz\\esc", "xyz");
  DoTest("foo bar baz", "daw", "bar baz");
  DoTest("foo bar baz", "cawxyz\\esc", "xyzbar baz");
  DoTest("foo bar baz", "wdaw", "foo baz");
  DoTest("foo bar baz", "wldaw", "foo baz");
  DoTest("foo bar baz", "wlldaw", "foo baz");
  DoTest("foo bar baz", "wcawxyz\\esc", "foo xyzbaz");
  DoTest("foo bar baz", "wwdaw", "foo bar");
  DoTest("foo bar baz   ", "wwdaw", "foo bar ");
  DoTest("foo bar baz", "wwcawxyz\\esc", "foo barxyz");
  DoTest("foo bar baz\n123", "jdaw", "foo bar baz\n");
  DoTest("foo bar baz\n123", "jcawxyz\\esc", "foo bar baz\nxyz");
  DoTest("foo bar baz\n123", "wwdaw", "foo bar\n123");
  DoTest("foo bar baz\n123", "wwcawxyz\\esc", "foo barxyz\n123");
  DoTest("foo bar      baz\n123", "wwdaw", "foo bar\n123");
  DoTest("foo bar      baz\n123", "wwcawxyz\\esc", "foo barxyz\n123");
  DoTest("foo bar baz \n123", "wwdaw", "foo bar \n123");
  DoTest("foo bar baz \n123", "wwcawxyz\\esc", "foo bar xyz\n123");
  DoTest("foo bar      baz \n123", "wwdaw", "foo bar      \n123");
  DoTest("foo bar      baz \n123", "wwcawxyz\\esc", "foo bar      xyz\n123");
  DoTest("foo    bar", "llldaw", "foo");
  DoTest("foo    bar", "lllcawxyz\\esc", "fooxyz");
  DoTest("foo    bar", "lllldaw", "foo");
  DoTest("foo    bar", "llllcawxyz\\esc", "fooxyz");
  DoTest("    bar", "daw", "");
  DoTest("    bar", "ldaw", "");
  DoTest("    bar", "llldaw", "");
  DoTest("    bar", "lllldaw", "    ");
  DoTest("    bar", "cawxyz\\esc", "xyz");
  DoTest("    bar", "lcawxyz\\esc", "xyz");
  DoTest("    bar", "lllcawxyz\\esc", "xyz");
  DoTest("foo   ", "llldaw", "foo   ");
  DoTest("foo   ", "lllldaw", "foo   ");
  DoTest("foo   ", "llllldaw", "foo   ");
  DoTest("foo   ", "lllcawxyz\\esc", "foo  ");
  DoTest("foo   ", "llllcawxyz\\esc", "foo  ");
  DoTest("foo   ", "lllllcawxyz\\esc", "foo  ");
  DoTest("foo   \nbar", "llldaw", "foo");
  DoTest("foo   \nbar", "lllldaw", "foo");
  DoTest("foo   \nbar", "llllldaw", "foo");
  DoTest("foo   \nbar", "lllcawxyz\\esc", "fooxyz");
  DoTest("foo   \nbar", "llllcawxyz\\esc", "fooxyz");
  DoTest("foo   \nbar", "lllllcawxyz\\esc", "fooxyz");
  DoTest("foo   \n   bar", "jdaw", "foo   \n");
  DoTest("foo   \n   bar", "jldaw", "foo   \n");
  DoTest("foo   \n   bar", "jlldaw", "foo   \n");
  DoTest("foo   \n   bar", "jcawxyz\\esc", "foo   \nxyz");
  DoTest("foo   \n   bar", "jlcawxyz\\esc", "foo   \nxyz");
  DoTest("foo   \n   bar", "jllcawxyz\\esc", "foo   \nxyz");
  DoTest("foo bar", "2daw", "");
  DoTest("foo bar", "2cawxyz\\esc", "xyz");
  DoTest("foo bar baz", "2daw", "baz");
  DoTest("foo bar baz", "2cawxyz\\esc", "xyzbaz");
  DoTest("foo bar baz", "3daw", "");
  DoTest("foo bar baz", "3cawxyz\\esc", "xyz");
  DoTest("foo bar\nbaz", "2daw", "\nbaz");
  DoTest("foo bar\nbaz", "2cawxyz\\esc", "xyz\nbaz");
  DoTest("foo bar\nbaz 123", "3daw", "123");
  DoTest("foo bar\nbaz 123", "3cawxyz\\esc", "xyz123");
  DoTest("foo bar \nbaz 123", "3daw", "123");
  DoTest("foo bar \nbaz 123", "3cawxyz\\esc", "xyz123");
  DoTest("foo bar baz", "lll2daw", "foo");
  DoTest("foo bar baz", "lll2cawxyz\\esc", "fooxyz");
  DoTest("   bar baz", "2daw", "");
  DoTest("   bar baz", "2cawxyz\\esc", "xyz");
  DoTest("   bar baz 123", "2daw", " 123");
  DoTest("   bar baz 123", "2cawxyz\\esc", "xyz 123");
  DoTest("   bar baz\n123", "3daw", "");
  DoTest("   bar baz\n123", "3cawxyz\\esc", "xyz");
  DoTest("   bar baz\n  123", "3daw", "");
  DoTest("   bar baz\n  123", "3cawxyz\\esc", "xyz");
  DoTest("   bar baz\n  123", "2daw", "\n  123");
  DoTest("   bar baz\n  123", "2cawxyz\\esc", "xyz\n  123");
  DoTest("   bar baz\n  123 456 789", "j2daw", "   bar baz\n 789");
  DoTest("   bar baz\n  123 456 789", "j2cawxyz\\esc", "   bar baz\nxyz 789");
  DoTest("foo\nbar\n", "2daw", "");
  DoTest("bar baz\n123 \n456\n789 abc \njkl", "j4daw", "bar baz\njkl");
  DoTest("bar baz\n123 \n456\n789 abc \njkl", "j4cawxyz\\esc", "bar baz\nxyzjkl");
  DoTest("   bar baz\n  123 \n456\n789 abc \njkl", "j4daw", "   bar baz\njkl");
  DoTest("   bar baz\n  123 456 789", "j2cawxyz\\esc", "   bar baz\nxyz 789");
  DoTest("foo b123r xyz", "wdaw", "foo xyz");
  DoTest("foo b123r xyz", "wldaw", "foo xyz");
  DoTest("foo b123r xyz", "wlldaw", "foo xyz");
  DoTest("foo b123r xyz", "wllldaw", "foo xyz");
  DoTest("foo b123r xyz", "wlllldaw", "foo xyz");
  DoTest("1 2 3 4 5 6", "daw", "2 3 4 5 6");
  DoTest("1 2 3 4 5 6", "ldaw", "1 3 4 5 6");
  DoTest("1 2 3 4 5 6", "lldaw", "1 3 4 5 6");
  DoTest("1 2 3 4 5 6", "llldaw", "1 2 4 5 6");
  DoTest("!foo!", "ldaw", "!!");
  DoTest("! foo !", "ldaw", "! !");
  DoTest("! foo !", "lldaw", "! !");
  DoTest("! foo (", "l2daw", "!");
  DoTest("! foo(\n123", "l2daw", "!\n123");
  DoTest("  !foo(\n123", "lll2daw", "  !\n123");
  DoTest("  !!foo(\n123", "llll2daw", "  !!\n123");
  DoTest("  !foo( \n123", "lll2daw", "  !\n123");
  DoTest("  !!!!(", "llldaw", "  ");
  DoTest("  !!!!(", "lll2daw", "  !!!!(");
  DoTest("  !!!!(\n!!!", "lll2daw", "");
  DoTest("  !!!!(\n!!!", "llll2daw", "");

  // Inner/ A WORD
  // Behave the same as a Word if there are no non-word chars.
  DoTest("", "diW", "");
  DoTest(" ", "diW", "");
  DoTest("  ", "diW", "");
  DoTest("foo", "daW", "");
  DoTest("foo", "ldaW", "");
  DoTest("foo", "caWxyz\\esc", "xyz");
  DoTest("foo bar baz", "daW", "bar baz");
  DoTest("foo bar baz", "caWxyz\\esc", "xyzbar baz");
  DoTest("foo bar baz", "wdaW", "foo baz");
  DoTest("foo bar baz", "wldaW", "foo baz");
  DoTest("foo bar baz", "wlldaW", "foo baz");
  DoTest("foo bar baz", "wcaWxyz\\esc", "foo xyzbaz");
  DoTest("foo bar baz", "wwdaW", "foo bar");
  DoTest("foo bar baz   ", "wwdaW", "foo bar ");
  DoTest("foo bar baz", "wwcaWxyz\\esc", "foo barxyz");
  DoTest("foo bar baz\n123", "jdaW", "foo bar baz\n");
  DoTest("foo bar baz\n123", "jcaWxyz\\esc", "foo bar baz\nxyz");
  DoTest("foo bar baz\n123", "wwdaW", "foo bar\n123");
  DoTest("foo bar baz\n123", "wwcaWxyz\\esc", "foo barxyz\n123");
  DoTest("foo bar      baz\n123", "wwdaW", "foo bar\n123");
  DoTest("foo bar      baz\n123", "wwcaWxyz\\esc", "foo barxyz\n123");
  DoTest("foo bar baz \n123", "wwdaW", "foo bar \n123");
  DoTest("foo bar baz \n123", "wwcaWxyz\\esc", "foo bar xyz\n123");
  DoTest("foo bar      baz \n123", "wwdaW", "foo bar      \n123");
  DoTest("foo bar      baz \n123", "wwcaWxyz\\esc", "foo bar      xyz\n123");
  DoTest("foo    bar", "llldaW", "foo");
  DoTest("foo    bar", "lllcaWxyz\\esc", "fooxyz");
  DoTest("foo    bar", "lllldaW", "foo");
  DoTest("foo    bar", "llllcaWxyz\\esc", "fooxyz");
  DoTest("    bar", "daW", "");
  DoTest("    bar", "ldaW", "");
  DoTest("    bar", "llldaW", "");
  DoTest("    bar", "lllldaW", "    ");
  DoTest("    bar", "caWxyz\\esc", "xyz");
  DoTest("    bar", "lcaWxyz\\esc", "xyz");
  DoTest("    bar", "lllcaWxyz\\esc", "xyz");
  DoTest("foo   ", "llldaW", "foo   ");
  DoTest("foo   ", "lllldaW", "foo   ");
  DoTest("foo   ", "llllldaW", "foo   ");
  DoTest("foo   ", "lllcaWxyz\\esc", "foo  ");
  DoTest("foo   ", "llllcaWxyz\\esc", "foo  ");
  DoTest("foo   ", "lllllcaWxyz\\esc", "foo  ");
  DoTest("foo   \nbar", "llldaW", "foo");
  DoTest("foo   \nbar", "lllldaW", "foo");
  DoTest("foo   \nbar", "llllldaW", "foo");
  DoTest("foo   \nbar", "lllcaWxyz\\esc", "fooxyz");
  DoTest("foo   \nbar", "llllcaWxyz\\esc", "fooxyz");
  DoTest("foo   \nbar", "lllllcaWxyz\\esc", "fooxyz");
  DoTest("foo   \n   bar", "jdaW", "foo   \n");
  DoTest("foo   \n   bar", "jldaW", "foo   \n");
  DoTest("foo   \n   bar", "jlldaW", "foo   \n");
  DoTest("foo   \n   bar", "jcaWxyz\\esc", "foo   \nxyz");
  DoTest("foo   \n   bar", "jlcaWxyz\\esc", "foo   \nxyz");
  DoTest("foo   \n   bar", "jllcaWxyz\\esc", "foo   \nxyz");
  DoTest("foo bar", "2daW", "");
  DoTest("foo bar", "2caWxyz\\esc", "xyz");
  DoTest("foo bar baz", "2daW", "baz");
  DoTest("foo bar baz", "2caWxyz\\esc", "xyzbaz");
  DoTest("foo bar baz", "3daW", "");
  DoTest("foo bar baz", "3caWxyz\\esc", "xyz");
  DoTest("foo bar\nbaz", "2daW", "\nbaz");
  DoTest("foo bar\nbaz", "2caWxyz\\esc", "xyz\nbaz");
  DoTest("foo bar\nbaz 123", "3daW", "123");
  DoTest("foo bar\nbaz 123", "3caWxyz\\esc", "xyz123");
  DoTest("foo bar \nbaz 123", "3daW", "123");
  DoTest("foo bar \nbaz 123", "3caWxyz\\esc", "xyz123");
  DoTest("foo bar baz", "lll2daW", "foo");
  DoTest("foo bar baz", "lll2caWxyz\\esc", "fooxyz");
  DoTest("   bar baz", "2daW", "");
  DoTest("   bar baz", "2caWxyz\\esc", "xyz");
  DoTest("   bar baz 123", "2daW", " 123");
  DoTest("   bar baz 123", "2caWxyz\\esc", "xyz 123");
  DoTest("   bar baz\n123", "3daW", "");
  DoTest("   bar baz\n123", "3caWxyz\\esc", "xyz");
  DoTest("   bar baz\n  123", "3daW", "");
  DoTest("   bar baz\n  123", "3caWxyz\\esc", "xyz");
  DoTest("   bar baz\n  123", "2daW", "\n  123");
  DoTest("   bar baz\n  123", "2caWxyz\\esc", "xyz\n  123");
  DoTest("   bar baz\n  123 456 789", "j2daW", "   bar baz\n 789");
  DoTest("   bar baz\n  123 456 789", "j2caWxyz\\esc", "   bar baz\nxyz 789");
  DoTest("foo\nbar\n", "2daW", "");
  DoTest("bar baz\n123 \n456\n789 abc \njkl", "j4daW", "bar baz\njkl");
  DoTest("bar baz\n123 \n456\n789 abc \njkl", "j4caWxyz\\esc", "bar baz\nxyzjkl");
  DoTest("   bar baz\n  123 \n456\n789 abc \njkl", "j4daW", "   bar baz\njkl");
  DoTest("   bar baz\n  123 456 789", "j2caWxyz\\esc", "   bar baz\nxyz 789");
  DoTest("foo b123r xyz", "wdaW", "foo xyz");
  DoTest("foo b123r xyz", "wldaW", "foo xyz");
  DoTest("foo b123r xyz", "wlldaW", "foo xyz");
  DoTest("foo b123r xyz", "wllldaW", "foo xyz");
  DoTest("foo b123r xyz", "wlllldaW", "foo xyz");
  DoTest("1 2 3 4 5 6", "daW", "2 3 4 5 6");
  DoTest("1 2 3 4 5 6", "ldaW", "1 3 4 5 6");
  DoTest("1 2 3 4 5 6", "lldaW", "1 3 4 5 6");
  DoTest("1 2 3 4 5 6", "llldaW", "1 2 4 5 6");
  // Now with non-word characters.
  DoTest("fo(o", "daW", "");
  DoTest("fo(o", "ldaW", "");
  DoTest("fo(o", "lldaW", "");
  DoTest("fo(o", "llldaW", "");
  DoTest("fo(o )!)!)ffo", "2daW", "");
  DoTest("fo(o", "diW", "");
  DoTest("fo(o", "ldiW", "");
  DoTest("fo(o", "lldiW", "");
  DoTest("fo(o", "llldiW", "");
  DoTest("foo \"\"B!!", "fBdaW", "foo");

  // Inner / Sentence text object ("is")
  DoTest("", "cis", "");
  DoTest("hello", "cis", "");
  DoTest("hello", "flcis", "");
  DoTest("hello. bye", "cisX", "X bye");
  DoTest("hello. bye", "f.cisX", "X bye");
  DoTest("hello.  bye", "fbcisX", "hello.  X");
  DoTest("hello\n\nbye.", "cisX", "X\n\nbye.");
  DoTest("Hello. Bye.\n", "GcisX", "Hello. Bye.\nX");
  DoTest("hello. by.. another.", "cisX", "X by.. another.");
  DoTest("hello. by.. another.", "fbcisX", "hello. X another.");
  DoTest("hello. by.. another.\n", "GcisX", "hello. by.. another.\nX");
  DoTest("hello. yay\nis this a string?!?.. another.\n", "fycisX", "hello. X another.\n");
  DoTest("hello. yay\nis this a string?!?.. another.\n", "jcisX", "hello. X another.\n");

  // Around / Sentence text object ("as")
  DoTest("", "cas", "");
  DoTest("hello", "cas", "");
  DoTest("hello", "flcas", "");
  DoTest("hello. bye", "casX", "Xbye");
  DoTest("hello. bye", "f.casX", "Xbye");
  DoTest("hello. bye.", "fbcasX", "hello.X");
  DoTest("hello. bye", "fbcasX", "hello.X");
  DoTest("hello\n\nbye.", "casX", "X\n\nbye.");
  DoTest("Hello. Bye.\n", "GcasX", "Hello. Bye.\nX");
  DoTest("hello. by.. another.", "casX", "Xby.. another.");
  DoTest("hello. by.. another.", "fbcasX", "hello. Xanother.");
  DoTest("hello. by.. another.\n", "GcasX", "hello. by.. another.\nX");
  DoTest("hello. yay\nis this a string?!?.. another.\n", "fycasX", "hello. Xanother.\n");
  DoTest("hello. yay\nis this a string?!?.. another.\n", "jcasX", "hello. Xanother.\n");
  DoTest("hello. yay\nis this a string?!?.. \t       another.\n", "jcasX", "hello. Xanother.\n");

  // Inner / Paragraph text object ("ip")
  DoTest("", "cip", "");
  DoTest("\nhello", "cipX", "X\nhello");
  DoTest("\nhello\n\nanother. text.", "jcipX", "\nX\n\nanother. text.");
  DoTest("\nhello\n\n\nanother. text.", "jjcipX", "\nhello\nX\nanother. text.");
  DoTest("\nhello\n\n\nanother. text.", "jjjcipX", "\nhello\nX\nanother. text.");
  DoTest("\nhello\n\n\nanother. text.", "jjjjcipX", "\nhello\n\n\nX");
  DoTest("hello\n\n", "jcipX", "hello\nX");
  DoTest("hello\n\n", "jjcipX", "hello\nX");

  // Around / Paragraph text object ("ap")
  DoTest("", "cap", "");
  DoTest("\nhello", "capX", "X");
  DoTest("\nhello\n\nanother.text.", "jcapX", "\nX\nanother.text.");
  DoTest("\nhello\n\nanother.text.\n\n\nAnother.", "jjjcapX", "\nhello\n\nX\nAnother.");
  DoTest("\nhello\n\nanother.text.\n\n\nAnother.", "jjjjjcapX", "\nhello\n\nanother.text.\nX");
  DoTest("hello\n\n\n", "jjcapX", "hello\n\n\n");
  DoTest("hello\n\nasd", "jjjcapX", "hello\nX");

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
  // Regression test for viB not working if there is a blank line before the closing }.
  DoTest( "{\nfoo\n\n}", "viBd", "{\n}");
  // The inner block text object does not include the line containing the opening brace if
  // the opening brace is the last character on its line and there is only whitespace before the closing brace.
  // (In particular: >iB should not indent the line containing the opening brace under these conditions).
  DoTest("{\nfoo\n}", "j>iB", "{\n  foo\n}");
  // Similarly, in such conditions, deleting the inner block should leave the cursor on closing brace line, not the
  // opening.
  DoTest("{\nfoo\n}", "jdiBiX", "{\nX}");
  // Yanking and pasting such a text object should be treated as linewise.
  DoTest("{\nfoo\nbar\n}", "jyiBjp", "{\nfoo\nbar\nfoo\nbar\n}");
  // Changing such a text object should delete everything but one line, which we will begin insertion at.
  DoTest("{\nfoo\nbar\n}", "jciBbaz\\esc", "{\nbaz\n}");
  // Make sure we remove the "last motion was a *linewise* curly text object" flag when we next parse a motion!
  DoTest("{\nfoo\n}", "jciBbaz xyz\\escdiw", "{\nbaz \n}");
  DoTest("{\nfoo\nbar\n}", "jviBbd", "{\nar\n}");


  DoTest( "int main() {\n  printf( \"HelloWorld!\\n\" );\n  return 0;\n} ",
          "jda}xr;",
          "int main();");

  DoTest("QList<QString>","wwldi>","QList<>");
  DoTest("QList<QString>","wwlda<","QList");
  DoTest("<>\n<title>Title</title>\n</head>",
         "di<jci>\\ctrl-c",
         "<>\n<>Title</title>\n</head>");

  DoTest( "foo bar baz", "wldiw", "foo  baz");

  DoTest( "foo bar baz", "wldawx", "foo az");

  DoTest( "foo ( \n bar\n)baz","jdi(", "foo ()baz");
  DoTest( "foo ( \n bar\n)baz","jda(", "foo baz");
  DoTest( "(foo(bar)baz)", "ldi)", "()");
  DoTest( "(foo(bar)baz)", "lca(\\ctrl-c", "");
  DoTest( "( foo ( bar ) )baz", "di(", "()baz" );
  DoTest( "( foo ( bar ) )baz", "da(", "baz" );
  DoTest( "[foo [ bar] [(a)b [c]d ]]","$hda]", "[foo [ bar] ]");
  DoTest( "(a)", "di(", "()");
  DoTest( "(ab)", "di(", "()");
  DoTest( "(abc)", "di(", "()");

  DoTest( "hi!))))}}]]","di]di}da)di)da]", "hi!))))}}]]" );

  DoTest("foo \"bar\" baz", "4ldi\"", "foo \"\" baz");
  DoTest("foo \"bar\" baz", "8lca\"\\ctrl-c", "foo  baz");

  DoTest("foo 'bar' baz", "4lca'\\ctrl-c", "foo  baz");
  DoTest("foo 'bar' baz", "8ldi'", "foo '' baz");

  DoTest("foo `bar` baz", "4lca`\\ctrl-c", "foo  baz");
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

  // Don't move if we can't find any matches at all, or fewer than we require.
  DoTest("nocapitalc", "lltCx", "noapitalc");
  DoTest("nocapitalc", "llTCx", "noapitalc");

  DoTest("123c456", "2tcx", "23c456");
  DoTest("123c456", "$2Tcx", "123c45");
  // Commands with searches that do not find anything, or find less than required, should do nothing.
  DoTest("foo", "dtk", "foo");
  DoTest("foomxyz", "d2tm", "foomxyz");
  DoTest("foo", "dfk", "foo");
  DoTest("foomxyz", "d2fm", "foomxyz");
  DoTest("foo", "$dTk", "foo");
  DoTest("foomxyz", "$d2Fm", "foomxyz");
  // They should also return a range marked as invalid.
  DoTest("foo bar", "gUF(", "foo bar");
  DoTest("foo bar", "gUf(", "foo bar");
  DoTest("foo bar", "gUt(", "foo bar");
  DoTest("foo bar", "gUT(", "foo bar");

  // Regression test for special-handling of "/" and "?" keys: these shouldn't interfere
  // with character searches.
  DoTest("foo /", "f/rX", "foo X");

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

  // Testing the "(" motion
  DoTest("", "(", "");
  DoTest("\nhello.", "fh(iX", "X\nhello.");
  DoTest("\n   hello.", "jfe(iX", "X\n   hello.");
  DoTest("hello. world.", "fr(iX", "Xhello. world.");
  DoTest("hello. world.\n", "j(iX", "hello. Xworld.\n");
  DoTest("hello. world\nyay. lol.\n", "jfl(iX", "hello. Xworld\nyay. lol.\n");
  DoTest("Hello.\n\n", "jj(iX", "XHello.\n\n");
  DoTest("\nHello.", "j(iX", "X\nHello.");
  DoTest("\n\n\nHello.", "jj(iX", "X\n\n\nHello.");
  DoTest("Hello! Bye!", "fB(iX", "XHello! Bye!");
  DoTest("Hello! Bye! Hye!", "fH(iX", "Hello! XBye! Hye!");
  DoTest("\nHello. Bye.. Asd.\n\n\n\nAnother.", "jjjj(iX", "\nHello. Bye.. XAsd.\n\n\n\nAnother.");

  // Testing the ")" motion
  DoTest("", ")", "");
  DoTest("\nhello.", ")iX", "\nXhello.");
  DoTest("hello. world.", ")iX", "hello. Xworld.");
  DoTest("hello. world\n\nasd.", "))iX", "hello. world\nX\nasd.");
  DoTest("hello. wor\nld.?? Asd", "))iX", "hello. wor\nld.?? XAsd");
  DoTest("hello. wor\nld.?? Asd", "jfA(iX", "hello. Xwor\nld.?? Asd");
  DoTest("Hello.\n\n\nWorld.", ")iX", "Hello.\nX\n\nWorld.");
  DoTest("Hello.\n\n\nWorld.", "))iX", "Hello.\n\n\nXWorld.");
  DoTest("Hello.\n\n", ")iX", "Hello.\nX\n");
  DoTest("Hello.\n\n", "))iX", "Hello.\n\nX");
  DoTest("Hello. ", ")aX", "Hello. X");
  DoTest("Hello?? Bye!", ")iX", "Hello?? XBye!");

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

  // Testing the position of the cursor in some cases of the "c" command.
  DoTest("(a, b, c)", "cibX", "(X)");
  DoTest("(a, b, c)", "f)cibX", "(X)");
  DoTest("(a, b, c)", "ci(X", "(X)");
  DoTest("(a, b, c)", "ci)X", "(X)");
  DoTest("[a, b, c]", "ci[X", "[X]");
  DoTest("[a, b, c]", "ci]X", "[X]");
  DoTest("{a, b, c}", "ciBX", "{X}");
  DoTest("{a, b, c}", "ci{X", "{X}");
  DoTest("{a, b, c}", "ci}X", "{X}");
  DoTest("<a, b, c>", "ci<X", "<X>");
  DoTest("<a, b, c>", "ci>X", "<X>");
}

void ViModeTest::NormalModeCommandsTest() {

  // Testing "J"
  DoTest("foo\nbar", "J", "foo bar");
  DoTest("foo\nbar", "JrX", "fooXbar");
  DoTest("foo\nbar\nxyz\n123", "3J", "foo bar xyz\n123");
  DoTest("foo\nbar\nxyz\n123", "3JrX", "foo barXxyz\n123");
  DoTest("foo\nbar\nxyz\n12345\n789", "4JrX", "foo bar xyzX12345\n789");
  DoTest("foo\nbar\nxyz\n12345\n789", "6JrX", "Xoo\nbar\nxyz\n12345\n789");
  DoTest("foo\nbar\nxyz\n12345\n789", "j5JrX", "foo\nXar\nxyz\n12345\n789");
  DoTest("foo\nbar\nxyz\n12345\n789", "7JrX", "Xoo\nbar\nxyz\n12345\n789");
  DoTest("\n\n", "J", "\n");
  DoTest("foo\n\t   \t\t  bar", "JrX", "fooXbar");
  DoTest("foo\n\t   \t\t", "J", "foo ");
  DoTest("foo\n\t   \t\t", "JrX", "fooX");

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
  DoTest("foo", "XP", "foo");

  // Testing Del key
  DoTest("foo", "\\home\\delete", "oo");
  DoTest("foo", "$\\delete", "fo");

  // Delete. Note that when sent properly via Qt, the key event text() will inexplicably be "127",
  // which can trip up the key parser. Duplicate this oddity here.
  BeginTest("xyz");
  TestPressKey("l");
  QKeyEvent *deleteKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier, "127");
  QApplication::postEvent(kate_view->focusProxy(), deleteKeyDown);
  QApplication::sendPostedEvents();
  QKeyEvent *deleteKeyUp = new QKeyEvent(QEvent::KeyRelease, Qt::Key_Delete, Qt::NoModifier, "127");
  QApplication::postEvent(kate_view->focusProxy(), deleteKeyUp);
  QApplication::sendPostedEvents();
  FinishTest("xz");

  // Testing "gu"
  DoTest("FOO\nBAR BAZ", "guj", "foo\nbar baz");
  DoTest("AbCDF", "gu3l", "abcDF");

  // Testing "guu"
  DoTest("FOO", "guu", "foo");
  DoTest("FOO\nBAR\nBAZ", "2guu", "foo\nbar\nBAZ");
  DoTest("", "guu", "");


  // Testing "gU"
  DoTest("aBcdf", "gU2l", "ABcdf");
  DoTest("foo\nbar baz", "gUj", "FOO\nBAR BAZ");

  // Testing "gUU"
  DoTest("foo", "gUU", "FOO");
  DoTest("foo\nbar\nbaz", "2gUU", "FOO\nBAR\nbaz");
  DoTest("", "gUU", "");

  // Testing "g~"
  DoTest("fOo BAr", "lg~fA", "foO bar");
  DoTest("fOo BAr", "$hg~FO", "foO bar");
  DoTest("fOo BAr", "lf~fZ", "fOo BAr");
  DoTest("{\nfOo BAr\n}", "jg~iB", "{\nFoO baR\n}");

  // Testing "g~~"
  DoTest("", "g~~", "");
  DoTest("\nfOo\nbAr", "g~~", "\nfOo\nbAr");
  DoTest("fOo\nbAr\nBaz", "g~~", "FoO\nbAr\nBaz");
  DoTest("fOo\nbAr\nBaz\nfAR", "j2g~~", "fOo\nBaR\nbAZ\nfAR");
  DoTest("fOo\nbAr\nBaz", "jlg~~rX", "fOo\nXaR\nBaz");
  DoTest("fOo\nbAr\nBaz\nfAR", "jl2g~~rX", "fOo\nBXR\nbAZ\nfAR");

  // Testing "s"
  DoTest("substitute char repeat", "w4scheck\\esc", "substitute check repeat");

  // Testing "r".
  DoTest("foobar", "l2r.", "f..bar");
  DoTest("foobar", "l5r.", "f.....");
  // Do nothing if the count is too high.
  DoTest("foobar", "l6r.", "foobar");

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

  DoTest("repeat\nindent", "2>>2>>", "    repeat\n    indent");

  // make sure we record correct history when indenting
  DoTest("repeat\nindent and undo", "2>>2>>2>>uu", "  repeat\n  indent and undo");
  DoTest("repeat\nunindent and undo", "2>>2>>2<<u", "    repeat\n    unindent and undo");

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

  // Indented paste.
  // ]p behaves as ordinary paste if not linewise, and on unindented line.
  DoTest("foo bar", "wyiwgg]p", "fbaroo bar");
  // ]p behaves as ordinary paste if not linewise, even on indented line.
  DoTest("  foo bar", "wwyiwggw]p", "  fbaroo bar");
  // [p behaves as ordinary Paste (P) if not linewise, and on unindented line.
  DoTest("foo bar", "wyiwgg[p", "barfoo bar");
  // [p behaves as ordinary Paste (P) if not linewise, even on indented line.
  DoTest("  foo bar", "wwyiw0w[p",   "  barfoo bar");
  // Prepend the spaces from the current line to the beginning of a single, pasted line.
  DoTest("  foo bar\nxyz", "jVygg]p", "  foo bar\n  xyz\nxyz");
  // Prepend the spaces from the current line to the beginning of each pasted line.
  DoTest("  foo bar\nxyz\nnose", "jVjygg]p", "  foo bar\n  xyz\n  nose\nxyz\nnose");
  const bool oldReplaceTabsDyn = kate_document->config()->replaceTabsDyn();
  kate_document->config()->setReplaceTabsDyn(false);
  // Tabs as well as spaces!
  DoTest("  \tfoo bar\nxyz\nnose", "jVjygg]p", "  \tfoo bar\n  \txyz\n  \tnose\nxyz\nnose");
  // Same for [p.
  DoTest("  \tfoo bar\nxyz\nnose", "jVjygg[p", "  \txyz\n  \tnose\n  \tfoo bar\nxyz\nnose");
  // Test if everything works if the current line has no non-whitespace.
  DoTest("\t \nbar", "jVygg]p", "\t \n\t bar\nbar");
  // Test if everything works if the current line is empty.
  DoTest("\nbar", "jVygg]p", "\nbar\nbar");
  // Unindent a pasted indented line if the current line has no indent.
  DoTest("foo\n  \tbar", "jVygg]p", "foo\nbar\n  \tbar");
  // Unindent subsequent lines, too - TODO - this assumes that each subsequent line has
  // *indentical* trailing whitespace to the first pasted line: Vim seems to be able to
  // deal with cases where this does not hold.
  DoTest("foo\n  \tbar\n  \txyz", "jVjygg]p", "foo\nbar\nxyz\n  \tbar\n  \txyz");
  DoTest("foo\n  \tbar\n  \t  xyz", "jVjygg]p", "foo\nbar\n  xyz\n  \tbar\n  \t  xyz");
  kate_document->config()->setReplaceTabsDyn(oldReplaceTabsDyn);

  // Some special cases of cw/ cW.
  DoTest("foo bar", "cwxyz\\esc", "xyz bar");
  DoTest("foo+baz bar", "cWxyz\\esc", "xyz bar");
  DoTest("foo+baz bar", "cwxyz\\esc", "xyz+baz bar");
  DoTest(" foo bar", "cwxyz\\esc", "xyzfoo bar");
  DoTest(" foo+baz bar", "cWxyz\\esc", "xyzfoo+baz bar");
  DoTest(" foo+baz bar", "cwxyz\\esc", "xyzfoo+baz bar");
  DoTest("\\foo bar", "cWxyz\\esc", "xyz bar");
  DoTest("foo   ", "lllcwxyz\\esc", "fooxyz");

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
  DoTest("foo", "yyp`[r[", "foo\n[oo");
  DoTest("xyz\nfoo", "ja\\returnbar\\esc`[r[", "xyz\n[\nbaroo");
  DoTest("foo", "lrayypgg`[r[", "fao\n[ao");
  DoTest("foo", "l~u`[r[", "[oo");
  DoTest("foo", "l~u`.r.", ".oo");
  DoTest("foo", "l~u`]r]", "]oo");
  DoTest("foo", "lia\\escu`[r[", "[oo");
  DoTest("foo", "lia\\escu`.r.", ".oo");
  DoTest("foo", "lia\\escu`]r]", "]oo");
  DoTest("foo", "l~u~`[r[", "f[o");
  DoTest("foo\nbar\nxyz", "jyypu`[r[", "foo\nbar\n[yz");
  DoTest("foo\nbar\nxyz", "jyypu`.r.", "foo\nbar\n.yz");
  DoTest("foo\nbar\nxyz", "jyypu`]r]", "foo\nbar\n]yz");
  DoTest("foo\nbar\nxyz\n123", "jdju`[r[", "foo\n[ar\nxyz\n123");
  DoTest("foo\nbar\nxyz\n123", "jdju`.r.", "foo\n.ar\nxyz\n123");
  DoTest("foo\nbar\nxyz\n123", "jdju`]r]", "foo\nbar\n]yz\n123");
  DoTest("foo\nbar\nxyz\n123", "jVj~u\\esc`[r[", "foo\n[ar\nxyz\n123", ShouldFail, "Vim is weird.");
}


void ViModeTest::NormalModeControlTests() {
  // Testing "Ctrl-x"
  DoTest("150", "101\\ctrl-x", "49");
  DoTest("1", "\\ctrl-x\\ctrl-x\\ctrl-x\\ctrl-x", "-3");
  DoTest("0xabcdef", "1000000\\ctrl-x","0x9c8baf" );
  DoTest("0x0000f", "\\ctrl-x","0x0000e" );
  // Octal numbers should retain leading 0's.
  DoTest("00010", "\\ctrl-x","00007" );

  // Testing "Ctrl-a"
  DoTest("150", "101\\ctrl-a", "251");
  DoTest("1000", "\\ctrl-ax", "100");
  DoTest("-1", "1\\ctrl-a", "0");
  DoTest("-1", "l1\\ctrl-a", "0");
  DoTest("0x0000f", "\\ctrl-a","0x00010" );
  // Decimal with leading 0's - increment, and strip leading 0's, like Vim.
  DoTest("0000193", "\\ctrl-a","194" );
  // If a number begins with 0, parse it as octal if we can. The resulting number should retain the
  // leadingi 0.
  DoTest("07", "\\ctrl-a","010" );
  DoTest("5", "5\\ctrl-a.","15" );
  DoTest("5", "5\\ctrl-a2.","12");
  DoTest("5", "5\\ctrl-a2.10\\ctrl-a","22");
  DoTest(" 5 ", "l\\ctrl-ax","  ");
  // If there's no parseable number under the cursor, look to the right to see if we can find one.
  DoTest("aaaa0xbcX", "\\ctrl-a", "aaaa0xbdX");
  DoTest("1 1", "l\\ctrl-a", "1 2");
  // We can skip across word boundaries in our search if need be.
  DoTest("aaaa 0xbcX", "\\ctrl-a", "aaaa 0xbdX");
  // If we can't find a parseable number anywhere, don't change anything.
  DoTest("foo", "\\ctrl-a", "foo");
  // Don't hang if the cursor is at the end of the line and the only number is to the immediate left of the cursor.
  DoTest("1 ", "l\\ctrl-a", "1 ");
  // ctrl-a/x algorithm involves stepping back to the previous word: don't crash if this is on the previous line
  // and at a column greater than the length of the current line.
  DoTest(" a a\n1", "j\\ctrl-a", " a a\n2");
  DoTest(" a a    a\n  1", "jll\\ctrl-a", " a a    a\n  2");
  // Regression test.
  DoTest("1w3", "l\\ctrl-a", "1w4");

  // Test "Ctrl-a/x" on a blank document/ blank line.
  DoTest("", "\\ctrl-a","");
  DoTest("", "\\ctrl-x","");
  DoTest("foo\n", "j\\ctrl-x","foo\n");
  DoTest("foo\n", "j\\ctrl-a","foo\n");

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

void ViModeTest::FakeCodeCompletionTests()
{
  // Test that FakeCodeCompletionTestModel behaves similar to the code-completion in e.g. KDevelop.
  const bool oldStealKeys = KateViewConfig::global()->viInputModeStealKeys();
  KateViewConfig::global()->setViInputModeStealKeys(true); // For Ctrl-P, Ctrl-N etc
  ensureKateViewVisible(); // KateView needs to be visible for the completion widget.
  FakeCodeCompletionTestModel *fakeCodeCompletionModel = new FakeCodeCompletionTestModel(kate_view);
  kate_view->registerCompletionModel(fakeCodeCompletionModel);
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  DoTest("", "i\\ctrl-p\\enter", "completionC");
  DoTest("", "i\\ctrl-p\\ctrl-p\\enter", "completionB");
  DoTest("", "i\\ctrl-p\\ctrl-p\\ctrl-p\\enter", "completionA");
  DoTest("", "i\\ctrl-p\\ctrl-p\\ctrl-p\\ctrl-p\\enter", "completionC");
  // If no word before cursor, don't delete any text.
  BeginTest("");
  clearTrackedDocumentChanges();
  TestPressKey("i\\ctrl- \\enter");
  QCOMPARE(m_docChanges.length(), 1);
  FinishTest("completionA");
  // Apparently, we must delete the word before the cursor upon completion
  // (even if we replace it with identical text!)
  BeginTest("compl");
  TestPressKey("ea");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 0), Cursor(0, 5)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 0), Cursor(0, 11)));
  QCOMPARE(m_docChanges[1].newText(), QString("completionA"));
  FinishTest("completionA");
  // A "word" is currently alphanumeric, plus underscore.
  fakeCodeCompletionModel->setCompletions(QStringList() << "w_123completion");
  BeginTest("(w_123");
  TestPressKey("ea");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 1), Cursor(0, 6)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 1), Cursor(0, 16)));
  QCOMPARE(m_docChanges[1].newText(), QString("w_123completion"));
  FinishTest("(w_123completion");
  // "Removing tail on complete" is apparently done in three stages:
  // delete word up to the cursor; insert new word; then delete remainder.
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  BeginTest("(w_123comp");
  TestPressKey("6li");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 3);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 1), Cursor(0, 6)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 1), Cursor(0, 16)));
  QCOMPARE(m_docChanges[1].newText(), QString("w_123completion"));
  QCOMPARE(m_docChanges[2].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[2].changeRange(), Range(Cursor(0, 16), Cursor(0, 20)));
  FinishTest("(w_123completion");

  // If we don't remove tail, just delete up to the cursor and insert.
  fakeCodeCompletionModel->setRemoveTailOnComplete(false);
  BeginTest("(w_123comp");
  TestPressKey("6li");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 1), Cursor(0, 6)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 1), Cursor(0, 16)));
  QCOMPARE(m_docChanges[1].newText(), QString("w_123completion"));
  FinishTest("(w_123completioncomp");

  // If no opening bracket after the cursor, a function taking no arguments
  // is added as "function()", and the cursor placed after the closing ")".
  // The addition of "function()" is done in two steps: first "function", then "()".
  BeginTest("object->");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall()");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("$a\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 20), Cursor(0, 22)));
  QCOMPARE(m_docChanges[1].newText(), QString("()"));
  TestPressKey("X");
  FinishTest("object->functionCall()X");

  // If no opening bracket after the cursor, a function taking at least one argument
  // is added as "function()", and the cursor placed after the opening "(".
  // The addition of "function()" is done in two steps: first "function", then "()".
  kDebug(13070) << "Fleep";
  BeginTest("object->");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...)");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("$a\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 20), Cursor(0, 22)));
  QCOMPARE(m_docChanges[1].newText(), QString("()"));
  TestPressKey("X");
  FinishTest("object->functionCall(X)");

  // If there is an opening bracket after the cursor, we merge the function call
  // with that.
  // Even if the function takes no arguments, we still place the cursor after the opening bracket,
  // in contrast to the case where there is no opening bracket after the cursor.
  // No brackets are added.  No removals occur if there is no word before the cursor.
  BeginTest("object->(");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall()");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f(i\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 1);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  TestPressKey("X");
  FinishTest("object->functionCall(X");

  // There can't be any non-whitespace between cursor position and opening bracket, though!
  BeginTest("object->|(   (");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall()");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f>a\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 20), Cursor(0, 22)));
  QCOMPARE(m_docChanges[1].newText(), QString("()"));
  TestPressKey("X");
  FinishTest("object->functionCall()X|(   (");

  // Whitespace before the bracket is fine, though.
  BeginTest("object->    (<-Cursor here!");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall()");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f>a\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 1);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  TestPressKey("X");
  FinishTest("object->functionCall    (X<-Cursor here!");

  // Be careful with positioning the cursor if we delete leading text!
  BeginTest("object->    (<-Cursor here!");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall()");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f>afunct");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 13)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[1].newText(), QString("functionCall"));
  TestPressKey("X");
  FinishTest("object->functionCall    (X<-Cursor here!");

  // If we're removing tail on complete, it's whether there is a suitable opening
  // bracket *after* the word (not the cursor) that's important.
  BeginTest("object->function    (<-Cursor here!");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall()");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("12li"); // Start inserting before the "t" in "function"
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 3);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 12)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[1].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[2].changeType(), DocChange::TextRemoved);
  kDebug(13070) << "m_docChanges[2].changeRange(): " << m_docChanges[2].changeRange();
  QCOMPARE(m_docChanges[2].changeRange(), Range(Cursor(0, 20), Cursor(0, 24)));
  TestPressKey("X");
  FinishTest("object->functionCall    (X<-Cursor here!");

  // Repeat of bracket-merging stuff, this time for functions that take at least one argument.
  BeginTest("object->(");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...)");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f(i\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 1);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  kDebug(13070) << "Range: " << m_docChanges[0].changeRange();
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  TestPressKey("X");
  FinishTest("object->functionCall(X");

  // There can't be any non-whitespace between cursor position and opening bracket, though!
  BeginTest("object->|(   (");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...)");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f>a\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 20), Cursor(0, 22)));
  QCOMPARE(m_docChanges[1].newText(), QString("()"));
  TestPressKey("X");
  FinishTest("object->functionCall(X)|(   (");

  // Whitespace before the bracket is fine, though.
  BeginTest("object->    (<-Cursor here!");
  qDebug() << "NooooO";
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...)");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f>a\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 1);
  QCOMPARE(m_docChanges[0].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[0].newText(), QString("functionCall"));
  TestPressKey("X");
  FinishTest("object->functionCall    (X<-Cursor here!");

  // Be careful with positioning the cursor if we delete leading text!
  BeginTest("object->    (<-Cursor here!");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...)");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("f>afunct");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 2);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 13)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[1].newText(), QString("functionCall"));
  TestPressKey("X");
  FinishTest("object->functionCall    (X<-Cursor here!");

  // If we're removing tail on complete, it's whether there is a suitable opening
  // bracket *after* the word (not the cursor) that's important.
  BeginTest("object->function    (<-Cursor here!");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...)");
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  clearTrackedDocumentChanges();
  TestPressKey("12li"); // Start inserting before the "t" in "function"
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 3);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 8), Cursor(0, 12)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 8), Cursor(0, 20)));
  QCOMPARE(m_docChanges[1].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[2].changeType(), DocChange::TextRemoved);
  kDebug(13070) << "m_docChanges[2].changeRange(): " << m_docChanges[2].changeRange();
  QCOMPARE(m_docChanges[2].changeRange(), Range(Cursor(0, 20), Cursor(0, 24)));
  TestPressKey("X");
  FinishTest("object->functionCall    (X<-Cursor here!");

  // Deal with function completions which add a ";".
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall();");
  clearTrackedDocumentChanges();
  TestPressKey("ifun");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 3);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 0), Cursor(0, 3)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 0), Cursor(0, 12)));
  QCOMPARE(m_docChanges[1].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[2].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[2].changeRange(), Range(Cursor(0, 12), Cursor(0, 15)));
  QCOMPARE(m_docChanges[2].newText(), QString("();"));
  FinishTest("functionCall();");

  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall();");
  TestPressKey("ifun\\ctrl- \\enterX");
  FinishTest("functionCall();X");

  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...);");
  clearTrackedDocumentChanges();
  TestPressKey("ifun");
  clearTrackedDocumentChanges();
  TestPressKey("\\ctrl- \\enter");
  QCOMPARE(m_docChanges.size(), 3);
  QCOMPARE(m_docChanges[0].changeType(), DocChange::TextRemoved);
  QCOMPARE(m_docChanges[0].changeRange(), Range(Cursor(0, 0), Cursor(0, 3)));
  QCOMPARE(m_docChanges[1].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[1].changeRange(), Range(Cursor(0, 0), Cursor(0, 12)));
  QCOMPARE(m_docChanges[1].newText(), QString("functionCall"));
  QCOMPARE(m_docChanges[2].changeType(),DocChange::TextInserted);
  QCOMPARE(m_docChanges[2].changeRange(), Range(Cursor(0, 12), Cursor(0, 15)));
  QCOMPARE(m_docChanges[2].newText(), QString("();"));
  FinishTest("functionCall();");

  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...);");
  TestPressKey("ifun\\ctrl- \\enterX");
  FinishTest("functionCall(X);");

  // Completions ending with ";" do not participate in bracket merging.
  BeginTest("(<-old bracket");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall();");
  TestPressKey("ifun\\ctrl- \\enterX");
  FinishTest("functionCall();X(<-old bracket");
  BeginTest("(<-old bracket");
  fakeCodeCompletionModel->setCompletions(QStringList() << "functionCall(...);");
  TestPressKey("ifun\\ctrl- \\enterX");
  FinishTest("functionCall(X);(<-old bracket");

  KateViewConfig::global()->setViInputModeStealKeys(oldStealKeys);
  kate_view->hide();
  mainWindow->hide();
  kate_view->unregisterCompletionModel(fakeCodeCompletionModel);
  delete fakeCodeCompletionModel;
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
    DoTest("abc", "\\:s/\\\\s*/x/g\\", "xaxbxc");
    //DoTest("abc\n123", "\\:s/\\\\s*/x/g\\", "xaxbxc\nx1x2x3"); // currently not working properly

    DoTest("foo/bar", "\\:s-/--\\","foobar");
    DoTest("foo/bar", "\\:s_/__\\","foobar");

    DoTest("foo\nfoo\nfoo","\\:2s/foo/bar\\", "foo\nbar\nfoo");
    DoTest("foo\nfoo\nfoo","2jmagg\\:'as/foo/bar\\","foo\nfoo\nbar");
    DoTest("foo\nfoo\nfoo", "\\:$s/foo/bar\\","foo\nfoo\nbar");

    // Testing ":d", ":delete"
    DoTest("foo\nbar\nbaz","\\:2d\\","foo\nbaz");
    DoTest("foo\nbar\nbaz","\\:%d\\","");
    BeginTest("foo\nbar\nbaz");
    TestPressKey("\\:$d\\"); // Work around ambiguity in the code that parses commands to execute.
    TestPressKey("\\:$d\\");
    FinishTest("foo");
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

    DoTest("foo\nbar","\\:2>\\","foo\n  bar");
    DoTest("   foo\nbaz","\\:1<\\","  foo\nbaz");

    DoTest("foo\nundo","\\:2>\\u","foo\nundo");
    DoTest("  foo\nundo","\\:1<\\u","  foo\nundo");

    DoTest("indent\nmultiline\ntext", "\\:1,2>\\", "  indent\n  multiline\ntext");
    DoTest("indent\nmultiline\n+undo", "\\:1,2>\\:1,2>\\:1,2>\\u", "    indent\n    multiline\n+undo");
    // doesn't test correctly, why?
    // DoTest("indent\nmultiline\n+undo", "\\:1,2>\\:1,2<\\u", "  indent\n  multiline\n+undo");

    // Testing ":c", ":change"
    DoTest("foo\nbar\nbaz","\\:2change\\","foo\n\nbaz");
    DoTest("foo\nbar\nbaz","\\:%c\\","");
    BeginTest("foo\nbar\nbaz");
    TestPressKey("\\:$c\\"); // Work around ambiguity in the code that parses commands to execute.
    TestPressKey("\\:$change\\");
    FinishTest("foo\nbar\n");
    DoTest("foo\nbar\nbaz","ma\\:2,'achange\\","\nbaz");
    DoTest("foo\nbar\nbaz","\\:2,3c\\","foo\n");

    // Testing ":j"
    DoTest("1\n2\n3\n4\n5","\\:2,4j\\","1\n2 3 4\n5");


    DoTest("1\n2\n3\n4","jvj\\ctrl-c\\:'<,'>d\\","1\n4");
    DoTest("1\n2\n3\n4","\\:1+1+1+1d\\","1\n2\n3");
    DoTest("1\n2\n3\n4","2j\\:.,.-1d\\","1\n4");
    DoTest("1\n2\n3\n4","\\:.+200-100-100+20-5-5-5-5+.-.,$-1+1-2+2-3+3-4+4-5+5-6+6-7+7-1000+1000+0-0-$+$-.+.-1d\\","4");
    DoTest("1\n2\n3\n4","majmbjmcjmdgg\\:'a+'b+'d-'c,.d\\","");
}

class VimStyleCommandBarTestsSetUpAndTearDown
{
public:
  VimStyleCommandBarTestsSetUpAndTearDown(KateView *kateView, QMainWindow* mainWindow)
    : m_kateView(kateView), m_mainWindow(mainWindow), m_windowKeepActive(mainWindow)
  {
    m_mainWindow->show();
    m_kateView->show();
    QApplication::setActiveWindow(m_mainWindow);
    m_kateView->setFocus();
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
    KateViewConfig::global()->setViInputModeEmulateCommandBar(true);
    QVERIFY(KateViewConfig::global()->viInputModeEmulateCommandBar());
    KateViewConfig::global()->setViInputModeStealKeys(true);
    mainWindow->installEventFilter(&m_windowKeepActive);
  }
  ~VimStyleCommandBarTestsSetUpAndTearDown()
  {
    m_mainWindow->removeEventFilter(&m_windowKeepActive);
    // Use invokeMethod to avoid having to export KateViewBar for testing.
    QMetaObject::invokeMethod(m_kateView->viModeEmulatedCommandBar(), "hideMe");
    m_kateView->hide();
    m_mainWindow->hide();
    KateViewConfig::global()->setViInputModeEmulateCommandBar(false);
    KateViewConfig::global()->setViInputModeStealKeys(false);
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
  }
private:
  KateView *m_kateView;
  QMainWindow *m_mainWindow;
  WindowKeepActive m_windowKeepActive;
};

void ViModeTest::MappingTests()
{
  const int mappingTimeoutMSOverride = QString::fromAscii(qgetenv("KATE_VIMODE_TEST_MAPPINGTIMEOUTMS")).toInt();
  const int mappingTimeoutMS = (mappingTimeoutMSOverride > 0) ? mappingTimeoutMSOverride : 2000;
  KateViewConfig::global()->setViInputModeStealKeys(true); // For tests involving e.g. <c-a>
  {
    // Check storage and retrieval of mapping recursion.
    clearAllMappings();

    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "ihello", KateViGlobal::Recursive);
    QVERIFY(KateGlobal::self()->viInputModeGlobal()->isMappingRecursive(KateViGlobal::NormalModeMapping, "'"));

    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "ihello", KateViGlobal::NonRecursive);
    QVERIFY(!KateGlobal::self()->viInputModeGlobal()->isMappingRecursive(KateViGlobal::NormalModeMapping, "a"));
  }

  clearAllMappings();

  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "<esc>ihello<esc>^aworld<esc>", KateViGlobal::Recursive);
  DoTest("", "'", "hworldello");

  // Ensure that the non-mapping logged keypresses are cleared before we execute a mapping
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'a", "rO", KateViGlobal::Recursive);
  DoTest("X", "'a", "O");

  {
    // Check that '123 is mapped after the timeout, given that we also have mappings that
    // extend it (e.g. '1234, '12345, etc) and which it itself extends ('1, '12, etc).
    clearAllMappings();
    BeginTest("");
    kate_view->getViInputModeManager()->keyMapper()->setMappingTimeout(mappingTimeoutMS);;
    QString consectiveDigits;
    for (int i = 1; i < 9; i++)
    {
      consectiveDigits += QString::number(i);
      KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'" + consectiveDigits, "iMapped from " + consectiveDigits + "<esc>", KateViGlobal::Recursive);
    }
    TestPressKey("'123");
    QCOMPARE(kate_document->text(), QString("")); // Shouldn't add anything until after the timeout!
    QTest::qWait(2 * mappingTimeoutMS);
    FinishTest("Mapped from 123");
  }

  // Mappings are not "counted": any count entered applies to the first command/ motion in the mapped sequence,
  // and is not used to replay the entire mapped sequence <count> times in a row.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'downmapping", "j", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'testmapping", "ifoo<esc>ibar<esc>", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'testmotionmapping", "lj", KateViGlobal::Recursive);
  DoTest("AAAA\nXXXX\nXXXX\nXXXX\nXXXX\nBBBB\nCCCC\nDDDD", "jd3'downmapping", "AAAA\nBBBB\nCCCC\nDDDD");
  DoTest("", "5'testmapping", "foofoofoofoofobaro");
  DoTest("XXXX\nXXXX\nXXXX\nXXXX", "3'testmotionmappingrO", "XXXX\nXXXO\nXXXX\nXXXX");

  // Regression test for a weird mistake I made: *completely* remove all counting for the
  // first command in the sequence; don't just set it to 1! If it is set to 1, then "%"
  // will mean "go to position 1 percent of the way through the document" rather than
  // go to matching item.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "gl", "%", KateViGlobal::Recursive);
  DoTest("0\n1\n2\n3\n4\n5\nfoo bar(xyz) baz", "jjjjjjwdgl", "0\n1\n2\n3\n4\n5\nfoo  baz");

  // Test that countable mappings work even when triggered by timeouts.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'testmapping", "ljrO", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'testmappingdummy", "dummy", KateViGlobal::Recursive);
  BeginTest("XXXX\nXXXX\nXXXX\nXXXX");
  kate_view->getViInputModeManager()->keyMapper()->setMappingTimeout(mappingTimeoutMS);;
  TestPressKey("3'testmapping");
  QTest::qWait(2 * mappingTimeoutMS);
  FinishTest("XXXX\nXXXO\nXXXX\nXXXX");

  // Test that telescoping mappings don't interfere with built-in commands. Assumes that gp
  // is implemented and working.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "gdummy", "idummy", KateViGlobal::Recursive);
  DoTest("hello", "yiwgpx", "hhellollo");

  // Test that we can map a sequence of keys that extends a built-in command and use
  // that sequence without the built-in command firing.
  // Once again, assumes that gp is implemented and working.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "gpa", "idummy", KateViGlobal::Recursive);
  DoTest("hello", "yiwgpa", "dummyhello");

  // Test that we can map a sequence of keys that extends a built-in command and still
  // have the original built-in command fire if we timeout after entering that command.
  // Once again, assumes that gp is implemented and working.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "gpa", "idummy", KateViGlobal::Recursive);
  BeginTest("hello");
  kate_view->getViInputModeManager()->keyMapper()->setMappingTimeout(mappingTimeoutMS);;
  TestPressKey("yiwgp");
  QTest::qWait(2 * mappingTimeoutMS);
  TestPressKey("x");
  FinishTest("hhellollo");

  // Test that something that starts off as a partial mapping following a command
  // (the "g" in the first "dg" is a partial mapping of "gj"), when extended to something
  // that is definitely not a mapping ("gg"), results in the full command being executed ("dgg").
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "gj", "aj", KateViGlobal::Recursive);
  DoTest("foo\nbar\nxyz", "jjdgg", "");

  // Make sure that a mapped sequence of commands is merged into a single undo-able edit.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'a", "ofoo<esc>ofoo<esc>ofoo<esc>", KateViGlobal::Recursive);
  DoTest("bar", "'au", "bar");

  // Make sure that a counted mapping is merged into a single undoable edit.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'a", "ofoo<esc>", KateViGlobal::Recursive);
  DoTest("bar", "5'au", "bar");

  // Some test setup for non-recursive mapping g -> gj (cf: bug:314415)
  // Firstly: work out the expected result of gj (this might be fragile as default settings
  // change, etc.).  We use BeginTest & FinishTest for the setup and teardown etc, but this is
  // not an actual test - it's just computing the expected result of the real test!
  const QString multiVirtualLineText = "foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo foo";
  ensureKateViewVisible(); // Needs to be visible in order for virtual lines to make sense.
  KateViewConfig::global()->setDynWordWrap(true);
  BeginTest(multiVirtualLineText);
  TestPressKey("gjrX");
  const QString expectedAfterVirtualLineDownAndChange = kate_document->text();
  Q_ASSERT_X(expectedAfterVirtualLineDownAndChange.contains("X") && !expectedAfterVirtualLineDownAndChange.startsWith('X'), "setting up j->gj testcase data", "gj doesn't seem to have worked correctly!");
  FinishTest(expectedAfterVirtualLineDownAndChange);

  // Test that non-recursive mappings are not expanded.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "j", "gj", KateViGlobal::NonRecursive);
  DoTest(multiVirtualLineText, "jrX", expectedAfterVirtualLineDownAndChange);
  KateViewConfig::global()->setDynWordWrap(false);

  // Test that recursive mappings are expanded.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "X", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "X", "rx", KateViGlobal::Recursive);
  DoTest("foo", "la", "fxo");

  // Test that the flag that stops mappings being expanded is reset after the mapping has been executed.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "j", "gj", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "X", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "X", "rx", KateViGlobal::Recursive);
  DoTest("foo", "jla", "fxo");

  // Even if we start with a recursive mapping, as soon as we hit one that is not recursive, we should stop
  // expanding.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "X", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "X", "r.", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "i", "a", KateViGlobal::Recursive);
  DoTest("foo", "li", "oo");

  // Regression test: Using a mapping may trigger a call to updateSelection(), which can change the mode
  // from VisualLineMode to plain VisualMode.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "gA", "%", KateViGlobal::NonRecursive);
  DoTest("xyz\nfoo\n{\nbar\n}", "jVjgAdgglP", "foo\n{\nbar\n}\nxyz");
  // Piggy back on the previous test with a regression test for issue where, if gA is mapped to %, vgly
  // will yank one more character than it should.
  DoTest("foo(bar)X", "vgAyp", "ffoo(bar)oo(bar)X");
  // Make sure that a successful mapping does not break the "if we select stuff externally in Normal mode,
  // we should switch to Visual Mode" thing.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "gA", "%", KateViGlobal::NonRecursive);
  BeginTest("foo bar xyz()");
  TestPressKey("gAr.");
  kate_view->setSelection(Range(0, 1, 0 , 4)); // Actually selects "oo " (i.e. without the "b").
  TestPressKey("d");
  FinishTest("fbar xyz(.");

  // Regression tests for BUG:260655
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "f", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "d", "i", KateViGlobal::NonRecursive);
  DoTest("foo dar", "adr.", "foo .ar");
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "F", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "d", "i", KateViGlobal::NonRecursive);
  DoTest("foo dar", "$adr.", "foo .ar");
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "t", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "d", "i", KateViGlobal::NonRecursive);
  DoTest("foo dar", "adr.", "foo.dar");
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "T", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "d", "i", KateViGlobal::NonRecursive);
  DoTest("foo dar", "$adr.", "foo d.r");
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "r", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "d", "i", KateViGlobal::NonRecursive);
  DoTest("foo dar", "ad", "doo dar");
  // Feel free to map the keypress after that, though.
  DoTest("foo dar", "addber\\esc", "berdoo dar");
  // Also, be careful about how we interpret "waiting for find char/ replace char"
  DoTest("foo dar", "ffas", "soo dar");

  // Ignore raw "Ctrl", "Shift", "Meta" and "Alt" keys, which will almost certainly end up being pressed as
  // we try to trigger mappings that contain these keys.
  clearAllMappings();
  {
    // Ctrl.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "<c-a><c-b>", "ictrl<esc>", KateViGlobal::NonRecursive);
    BeginTest("");
    QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("\\ctrl-a");
    ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("\\ctrl-b");
    FinishTest("ctrl");
  }
  {
    // Shift.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "<c-a>C", "ishift<esc>", KateViGlobal::NonRecursive);
    BeginTest("");
    QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("\\ctrl-a");
    QKeyEvent *shiftKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), shiftKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("C");
    FinishTest("shift");
  }
  {
    // Alt.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "<c-a><a-b>", "ialt<esc>", KateViGlobal::NonRecursive);
    BeginTest("");
    QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("\\ctrl-a");
    QKeyEvent *altKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Alt, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), altKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("\\alt-b");
    FinishTest("alt");
  }
  {
    // Meta.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "<c-a><m-b>", "imeta<esc>", KateViGlobal::NonRecursive);
    BeginTest("");
    QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), ctrlKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("\\ctrl-a");
    QKeyEvent *metaKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Meta, Qt::NoModifier);
    QApplication::postEvent(kate_view->focusProxy(), metaKeyDown);
    QApplication::sendPostedEvents();
    TestPressKey("\\meta-b");
    FinishTest("meta");
  }
  {
    // Can have mappings in Visual mode, distinct from Normal mode..
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "a", "3l", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "inose<esc>", KateViGlobal::NonRecursive);
    DoTest("0123456", "lvad", "056");

    // The recursion in Visual Mode is distinct from that of  Normal mode.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "b", "<esc>", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "a", "b", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "b", KateViGlobal::Recursive);
    DoTest("XXX\nXXX", "lvajd", "XXX");
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "b", "<esc>", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "a", "b", KateViGlobal::Recursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "b", KateViGlobal::NonRecursive);
    DoTest("XXX\nXXX", "lvajd", "XXX\nXXX");

    // A Visual mode mapping applies to all Visual modes (line, block, etc).
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "a", "2j", KateViGlobal::NonRecursive);
    DoTest("123\n456\n789", "lvad", "19");
    DoTest("123\n456\n789", "l\\ctrl-vad", "13\n46\n79");
    DoTest("123\n456\n789", "lVad", "");
    // Same for recursion.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "b", "2j", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "a", "b", KateViGlobal::Recursive);
    DoTest("123\n456\n789", "lvad", "19");
    DoTest("123\n456\n789", "l\\ctrl-vad", "13\n46\n79");
    DoTest("123\n456\n789", "lVad", "");

    // Can clear Visual mode mappings.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "h", "l", KateViGlobal::Recursive);
    KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::VisualModeMapping);
    DoTest("123\n456\n789", "lvhd", "3\n456\n789");
    DoTest("123\n456\n789", "l\\ctrl-vhd", "3\n456\n789");
    DoTest("123\n456\n789", "lVhd", "456\n789");
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "h", "l", KateViGlobal::Recursive);
    KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::VisualModeMapping);
    DoTest("123\n456\n789", "lvhd", "3\n456\n789");
    DoTest("123\n456\n789", "l\\ctrl-vhd", "3\n456\n789");
    DoTest("123\n456\n789", "lVhd", "456\n789");
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "h", "l", KateViGlobal::Recursive);
    KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::VisualModeMapping);
    DoTest("123\n456\n789", "lvhd", "3\n456\n789");
    DoTest("123\n456\n789", "l\\ctrl-vhd", "3\n456\n789");
    DoTest("123\n456\n789", "lVhd", "456\n789");
  }
  {
    // Can have mappings in Insert mode.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "a", "xyz", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "inose<esc>", KateViGlobal::NonRecursive);
    DoTest("foo", "ia\\esc", "xyzfoo");

    // Recursion for Insert mode.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "b", "c", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "a", "b", KateViGlobal::NonRecursive);
    DoTest("", "ia\\esc", "b");
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "b", "c", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "a", "b", KateViGlobal::Recursive);
    DoTest("", "ia\\esc", "c");

    clearAllMappings();
    // Clear mappings for Insert mode.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "a", "b", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::InsertModeMapping);
    DoTest("", "ia\\esc", "a");
  }

  {
    VimStyleCommandBarTestsSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(kate_view, mainWindow);
    // Can have mappings in Emulated Command Bar.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "a", "xyz", KateViGlobal::NonRecursive);
    DoTest(" a xyz", "/a\\enterrX", " a Xyz");
    // Use mappings from Normal mode as soon as we exit command bar via Enter.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "a", "ixyz<c-c>", KateViGlobal::NonRecursive);
    DoTest(" a xyz", "/a\\entera", " a xyzxyz");
    // Multiple mappings.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "b", "123", KateViGlobal::NonRecursive);
    DoTest("  xyz123", "/ab\\enterrX", "  Xyz123");
    // Recursive mappings.
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "b", "a", KateViGlobal::Recursive);
    DoTest("  xyz", "/b\\enterrX", "  Xyz");
    // Can clear all.
    KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::CommandModeMapping);
    DoTest("  ab xyz xyz123", "/ab\\enterrX", "  Xb xyz xyz123");
  }

  // Test that not *both* of the mapping and the mapped keys are logged for repetition via "."
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "ixyz", "iabc", KateViGlobal::NonRecursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "gl", "%", KateViGlobal::NonRecursive);
  DoTest("", "ixyz\\esc.", "ababcc");
  DoTest("foo()X\nbarxyz()Y", "cglbaz\\escggj.", "bazX\nbazY");

  // Regression test for a crash when executing a mapping that switches to Normal mode.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "h", "d", KateViGlobal::Recursive);
  DoTest("foo", "vlh", "o");

  {
    // Test that we can set/ unset mappings from the command-line.
    clearAllMappings();
    DoTest("", "\\:nn foo ibar<esc>\\foo", "bar");

    // "nn" is not recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "l", "iabc<esc>", KateViGlobal::NonRecursive);
    DoTest("xxx", "\\:nn foo l\\foorX", "xXx");

    // "no" is not recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "l", "iabc<esc>", KateViGlobal::NonRecursive);
    DoTest("xxx", "\\:no foo l\\foorX", "xXx");

    // "noremap" is not recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "l", "iabc<esc>", KateViGlobal::NonRecursive);
    DoTest("xxx", "\\:noremap foo l\\foorX", "xXx");

    // "nm" is recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "l", "iabc<esc>", KateViGlobal::NonRecursive);
    DoTest("xxx", "\\:nm foo l\\foorX", "abXxxx");

    // "nmap" is recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "l", "iabc<esc>", KateViGlobal::NonRecursive);
    DoTest("xxx", "\\:nmap foo l\\foorX", "abXxxx");

    // Unfortunately, "map" is a reserved word :/
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "l", "iabc<esc>", KateViGlobal::NonRecursive);
    DoTest("xxx", "\\:map foo l\\foorX", "abXxxx", ShouldFail, "'map' is reserved for other stuff in Kate command line");

    // nunmap works in normal mode.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "w", "ciwabc<esc>", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "b", "ciwxyz<esc>", KateViGlobal::NonRecursive);
    DoTest(" 123 456 789", "\\:nunmap b\\WWwbrX", " 123 Xbc 789");

    // vmap works in Visual mode and is recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("abco", "\\:vmap foo l\\v\\rightfoogU", "co");

    // vmap does not work in Normal mode.
    clearAllMappings();
    DoTest("xxx", "\\:vmap foo l\\foorX", "xxx\nrX");

    // vm works in Visual mode and is recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("abco", "\\:vm foo l\\v\\rightfoogU", "co");

    // vn works in Visual mode and is not recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("abco", "\\:vn foo l\\v\\rightfoogU", "ABCo");

    // vnoremap works in Visual mode and is not recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("abco", "\\:vnoremap foo l\\v\\rightfoogU", "ABCo");

    // vunmap works in Visual Mode.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "l", "w", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::VisualModeMapping, "gU", "2b", KateViGlobal::NonRecursive);
    DoTest("foo bar xyz", "\\:vunmap gU\\wvlgUd", "foo BAR Xyz");

    // imap works in Insert mode and is recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("", "\\:imap foo l\\ifoo\\esc", "d");

    // im works in Insert mode and is recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("", "\\:im foo l\\ifoo\\esc", "d");

    // ino works in Insert mode and is not recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("", "\\:ino foo l\\ifoo\\esc", "l");

    // inoremap works in Insert mode and is not recursive.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "l", "d", KateViGlobal::NonRecursive);
    DoTest("", "\\:inoremap foo l\\ifoo\\esc", "l");

    // iunmap works in Insert mode.
    clearAllMappings();
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "l", "d", KateViGlobal::NonRecursive);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "m", "e", KateViGlobal::NonRecursive);
    DoTest("", "\\:iunmap l\\ilm\\esc", "le");

    {
      VimStyleCommandBarTestsSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(kate_view, mainWindow);
      // cmap works in emulated command bar and is recursive.
      // NOTE: need to do the cmap call using the direct execution (i.e. \\:cmap blah blah\\), *not* using
      // the emulated command bar (:cmap blah blah\\enter), as this will be subject to mappings, which
      // can interfere with the tests!
      clearAllMappings();
      KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "l", "d", KateViGlobal::NonRecursive);
      DoTest(" l d foo", "\\:cmap foo l\\/foo\\enterrX", " l X foo");

      // cm works in emulated command bar and is recursive.
      clearAllMappings();
      KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "l", "d", KateViGlobal::NonRecursive);
      DoTest(" l d foo", "\\:cm foo l\\/foo\\enterrX", " l X foo");

      // cnoremap works in emulated command bar and is recursive.
      clearAllMappings();
      KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "l", "d", KateViGlobal::NonRecursive);
      DoTest(" l d foo", "\\:cnoremap foo l\\/foo\\enterrX", " X d foo");

      // cno works in emulated command bar and is recursive.
      clearAllMappings();
      KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "l", "d", KateViGlobal::NonRecursive);
      DoTest(" l d foo", "\\:cno foo l\\/foo\\enterrX", " X d foo");

      // cunmap works in emulated command bar.
      clearAllMappings();
      KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "l", "d", KateViGlobal::NonRecursive);
      KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::CommandModeMapping, "m", "e", KateViGlobal::NonRecursive);
      DoTest(" de le", "\\:cunmap l\\/lm\\enterrX", " de Xe");
    }

    // Can use <space> to signify a space.
    clearAllMappings();
    DoTest("", "\\:nn h<space> i<space>a<space>b<esc>\\h ", " a b");
  }

  // More recursion tests - don't lose characters from a Recursive mapping if it looks like they might
  // be part of a different mapping (but end up not being so).
  // (Here, the leading "i" in "irecursive<c-c>" could be part of the mapping "ihello<c-c>").
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "ihello<c-c>", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "ihello<c-c>", "irecursive<c-c>", KateViGlobal::Recursive);
  DoTest("", "'", "recursive");

  // Capslock in insert mode is not handled by Vim nor by KateViewInternal, and ends up
  // being sent to KateViInputModeManager::handleKeypress twice (it could be argued that this is
  // incorrect behaviour on the part of KateViewInternal), which can cause infinite
  // recursion if we are not careful about identifying replayed rejected keypresses.
  BeginTest("foo bar");
  TestPressKey("i");
  QKeyEvent *capslockKeyPress = new QKeyEvent(QEvent::KeyPress, Qt::Key_CapsLock, Qt::NoModifier);
  QApplication::postEvent(kate_view->focusProxy(), capslockKeyPress);
  QApplication::sendPostedEvents();
  FinishTest("foo bar");

  // Clear mappings for subsequent tests.
  clearAllMappings();
}

void ViModeTest::yankHighlightingTests()
{
  const QColor yankHighlightColour = kate_view->renderer()->config()->savedLineColor();

  BeginTest("foo bar xyz");
  const QList<Kate::TextRange*> rangesInitial = rangesOnFirstLine();
  Q_ASSERT(rangesInitial.isEmpty() && "Assumptions about ranges are wrong - this test is invalid and may need updating!");
  TestPressKey("wyiw");
  {
    const QList<Kate::TextRange*> rangesAfterYank = rangesOnFirstLine();
    QCOMPARE(rangesAfterYank.size(), rangesInitial.size() + 1);
    QCOMPARE(rangesAfterYank.first()->attribute()->background().color(), yankHighlightColour);
    QCOMPARE(rangesAfterYank.first()->start().line(), 0);
    QCOMPARE(rangesAfterYank.first()->start().column(), 4);
    QCOMPARE(rangesAfterYank.first()->end().line(), 0);
    QCOMPARE(rangesAfterYank.first()->end().column(), 7);
  }
  FinishTest("foo bar xyz");

  BeginTest("foom bar xyz");
  TestPressKey("wY");
  {
    const QList<Kate::TextRange*> rangesAfterYank = rangesOnFirstLine();
    QCOMPARE(rangesAfterYank.size(), rangesInitial.size() + 1);
    QCOMPARE(rangesAfterYank.first()->attribute()->background().color(), yankHighlightColour);
    QCOMPARE(rangesAfterYank.first()->start().line(), 0);
    QCOMPARE(rangesAfterYank.first()->start().column(), 5);
    QCOMPARE(rangesAfterYank.first()->end().line(), 0);
    QCOMPARE(rangesAfterYank.first()->end().column(), 12);
  }
  FinishTest("foom bar xyz");

  // Unhighlight on keypress.
  DoTest("foo bar xyz", "yiww", "foo bar xyz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());

  // Update colour on config change.
  DoTest("foo bar xyz", "yiw", "foo bar xyz");
  const QColor newYankHighlightColour = QColor(255, 0, 0);
  kate_view->renderer()->config()->setSavedLineColor(newYankHighlightColour);
  QCOMPARE(rangesOnFirstLine().first()->attribute()->background().color(), newYankHighlightColour);

  // Visual Mode.
  DoTest("foo", "viwy", "foo");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);

  // Unhighlight on keypress in Visual Mode
  DoTest("foo", "viwyw", "foo");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());

  // Add a yank highlight and directly (i.e. without using Vim commands,
  // which would clear the highlight) delete all text; if this deletes the yank highlight behind our back
  // and we don't respond correctly to this, it will be double-deleted by KateViNormalMode.
  // Currently, this seems like it doesn't occur, but better safe than sorry :)
  BeginTest("foo bar xyz");
  TestPressKey("yiw");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  kate_document->documentReload();
  kate_document->clear();
  vi_input_mode_manager = kate_view->resetViInputModeManager(); // This implicitly deletes KateViNormal
  FinishTest("");
}


void ViModeTest::VimStyleCommandBarTests()
{
  // Ensure that some preconditions for these tests are setup, and (more importantly)
  // ensure that they are reverted no matter how these tests end.
  VimStyleCommandBarTestsSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(kate_view, mainWindow);


  // Verify that we can get a non-null pointer to the emulated command bar.
  KateViEmulatedCommandBar *emulatedCommandBar = kate_view->viModeEmulatedCommandBar();
  QVERIFY(emulatedCommandBar);

  // Should initially be hidden.
  QVERIFY(!emulatedCommandBar->isVisible());

  // Test that "/" invokes the emulated command bar (if we are configured to use it)
  BeginTest("");
  TestPressKey("/");
  QVERIFY(emulatedCommandBar->isVisible());
  QCOMPARE(emulatedCommandTypeIndicator()->text(), QString("/"));
  QVERIFY(emulatedCommandTypeIndicator()->isVisible());
  QVERIFY(emulatedCommandBarTextEdit());
  QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
  // Make sure the keypresses end up changing the text.
  QVERIFY(emulatedCommandBarTextEdit()->isVisible());
  TestPressKey("foo");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  // Make sure ctrl-c dismisses it (assuming we allow Vim to steal the ctrl-c shortcut).
  TestPressKey("\\ctrl-c");
  QVERIFY(!emulatedCommandBar->isVisible());

  // Ensure that ESC dismisses it, too.
  BeginTest("");
  TestPressKey("/");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\esc");
  QVERIFY(!emulatedCommandBar->isVisible());
  FinishTest("");

  // Ensure that Ctrl-[ dismisses it, too.
  BeginTest("");
  TestPressKey("/");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\ctrl-[");
  QVERIFY(!emulatedCommandBar->isVisible());
  FinishTest("");

  // Ensure that Enter dismisses it, too.
  BeginTest("");
  TestPressKey("/");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\enter");
  QVERIFY(!emulatedCommandBar->isVisible());
  FinishTest("");

  // Ensure that Return dismisses it, too.
  BeginTest("");
  TestPressKey("/");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\return");
  QVERIFY(!emulatedCommandBar->isVisible());
  FinishTest("");

  // Ensure that text is always initially empty.
  BeginTest("");
  TestPressKey("/a\\enter");
  TestPressKey("/");
  QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
  TestPressKey("\\enter");
  FinishTest("");

  // Check backspace works.
  BeginTest("");
  TestPressKey("/foo\\backspace");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fo"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-h works.
  BeginTest("");
  TestPressKey("/bar\\ctrl-h");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("ba"));
  TestPressKey("\\enter");
  FinishTest("");

  // ctrl-h should dismiss bar when empty.
  BeginTest("");
  TestPressKey("/\\ctrl-h");
  QVERIFY(!emulatedCommandBar->isVisible());
  FinishTest("");

  // ctrl-h should not dismiss bar when there is stuff to the left of cursor.
  BeginTest("");
  TestPressKey("/a\\ctrl-h");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\enter");
  FinishTest("");

  // ctrl-h should not dismiss bar when bar is not empty, even if there is nothing to the left of cursor.
  BeginTest("");
  TestPressKey("/a\\left\\ctrl-h");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\enter");
  FinishTest("");

  // Same for backspace.
  BeginTest("");
  TestPressKey("/bar\\backspace");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("ba"));
  TestPressKey("\\enter");
  FinishTest("");
  BeginTest("");
  TestPressKey("/\\backspace");
  QVERIFY(!emulatedCommandBar->isVisible());
  FinishTest("");
  BeginTest("");
  TestPressKey("/a\\backspace");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\enter");
  FinishTest("");
  BeginTest("");
  TestPressKey("/a\\left\\backspace");
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-b works.
  BeginTest("");
  TestPressKey("/bar foo xyz\\ctrl-bX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("Xbar foo xyz"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-e works.
  BeginTest("");
  TestPressKey("/bar foo xyz\\ctrl-b\\ctrl-eX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar foo xyzX"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w works.
  BeginTest("");
  TestPressKey("/foo bar\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w works on empty command bar.
  BeginTest("");
  TestPressKey("/\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w works in middle of word.
  BeginTest("");
  TestPressKey("/foo bar\\left\\left\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo ar"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w leaves the cursor in the right place when in the middle of word.
  BeginTest("");
  TestPressKey("/foo bar\\left\\left\\ctrl-wX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo Xar"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w works when at the beginning of the text.
  BeginTest("");
  TestPressKey("/foo\\left\\left\\left\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w works when the character to the left is a space.
  BeginTest("");
  TestPressKey("/foo bar   \\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w works when all characters to the left of the cursor are spaces.
  BeginTest("");
  TestPressKey("/   \\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w works when all characters to the left of the cursor are non-spaces.
  BeginTest("");
  TestPressKey("/foo\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w does not continue to delete subsequent alphanumerics if the characters to the left of the cursor
  // are non-space, non-alphanumerics.
  BeginTest("");
  TestPressKey("/foo!!!\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  TestPressKey("\\enter");
  FinishTest("");
  // Check ctrl-w does not continue to delete subsequent alphanumerics if the characters to the left of the cursor
  // are non-space, non-alphanumerics.
  BeginTest("");
  TestPressKey("/foo!!!\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w deletes underscores and alphanumerics to the left of the cursor, but stops when it reaches a
  // character that is none of these.
  BeginTest("");
  TestPressKey("/foo!!!_d1\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo!!!"));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w doesn't swallow the spaces preceding the block of non-word chars.
  BeginTest("");
  TestPressKey("/foo !!!\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
  TestPressKey("\\enter");
  FinishTest("");

  // Check ctrl-w doesn't swallow the spaces preceding the word.
  BeginTest("");
  TestPressKey("/foo 1d_\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo "));
  TestPressKey("\\enter");
  FinishTest("");

  // Check there is a "waiting for register" indicator, initially hidden.
  BeginTest("");
  TestPressKey("/");
  QLabel* waitingForRegisterIndicator = emulatedCommandBar->findChild<QLabel*>("waitingforregisterindicator");
  QVERIFY(waitingForRegisterIndicator);
  QVERIFY(!waitingForRegisterIndicator->isVisible());
  QCOMPARE(waitingForRegisterIndicator->text(), QString("\""));
  TestPressKey("\\enter");
  FinishTest("");

  // Test that ctrl-r causes it to become visible.  It is displayed to the right of the text edit.
  BeginTest("");
  TestPressKey("/\\ctrl-r");
  QVERIFY(waitingForRegisterIndicator->isVisible());
  QVERIFY(waitingForRegisterIndicator->x() >= emulatedCommandBarTextEdit()->x() + emulatedCommandBarTextEdit()->width());
  TestPressKey("\\ctrl-c");
  TestPressKey("\\ctrl-c");
  FinishTest("");

  // The first ctrl-c after ctrl-r (when no register entered) hides the waiting for register
  // indicator, but not the bar.
  BeginTest("");
  TestPressKey("/\\ctrl-r");
  QVERIFY(waitingForRegisterIndicator->isVisible());
  TestPressKey("\\ctrl-c");
  QVERIFY(!waitingForRegisterIndicator->isVisible());
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss the bar.
  FinishTest("");

  // The first ctrl-c after ctrl-r (when no register entered) aborts waiting for register.
  BeginTest("foo");
  TestPressKey("\"cyiw/\\ctrl-r\\ctrl-ca");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("a"));
  TestPressKey("\\ctrl-c"); // Dismiss the bar.
  FinishTest("foo");

  // Same as above, but for ctrl-[ instead of ctrl-c.
  BeginTest("");
  TestPressKey("/\\ctrl-r");
  QVERIFY(waitingForRegisterIndicator->isVisible());
  TestPressKey("\\ctrl-[");
  QVERIFY(!waitingForRegisterIndicator->isVisible());
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss the bar.
  FinishTest("");
  BeginTest("foo");
  TestPressKey("\"cyiw/\\ctrl-r\\ctrl-[a");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("a"));
  TestPressKey("\\ctrl-c"); // Dismiss the bar.
  FinishTest("foo");

  // Check ctrl-r works with registers, and hides the "waiting for register" indicator.
  BeginTest("xyz");
  TestPressKey("\"ayiw/foo\\ctrl-ra");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fooxyz"));
  QVERIFY(!waitingForRegisterIndicator->isVisible());
  TestPressKey("\\enter");
  FinishTest("xyz");

  // Check ctrl-r inserts text at the current cursor position.
  BeginTest("xyz");
  TestPressKey("\"ayiw/foo\\left\\ctrl-ra");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foxyzo"));
  TestPressKey("\\enter");
  FinishTest("xyz");

  // Check ctrl-r ctrl-w inserts word under the cursor, and hides the "waiting for register" indicator.
  BeginTest("foo bar xyz");
  TestPressKey("w/\\left\\ctrl-r\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
  QVERIFY(!waitingForRegisterIndicator->isVisible());
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Check ctrl-r ctrl-w doesn't insert the contents of register w!
  BeginTest("foo baz xyz");
  TestPressKey("\"wyiww/\\left\\ctrl-r\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("baz"));
  TestPressKey("\\enter");
  FinishTest("foo baz xyz");

  // Check ctrl-r ctrl-w inserts at the current cursor position.
  BeginTest("foo nose xyz");
  TestPressKey("w/bar\\left\\ctrl-r\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("banoser"));
  TestPressKey("\\enter");
  FinishTest("foo nose xyz");

  // Cursor position is at the end of the inserted text after ctrl-r ctrl-w.
  BeginTest("foo nose xyz");
  TestPressKey("w/bar\\left\\ctrl-r\\ctrl-wX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("banoseXr"));
  TestPressKey("\\enter");
  FinishTest("foo nose xyz");

  // Cursor position is at the end of the inserted register contents after ctrl-r.
  BeginTest("xyz");
  TestPressKey("\"ayiw/foo\\left\\ctrl-raX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foxyzXo"));
  TestPressKey("\\enter");
  FinishTest("xyz");

  // Insert clipboard contents on ctrl-r +.  We implicitly need to test the ability to handle
  // shift key key events when waiting for register (they should be ignored).
  BeginTest("xyz");
  QApplication::clipboard()->setText("vimodetestclipboardtext");
  TestPressKey("/\\ctrl-r");
  QKeyEvent *shiftKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
  QApplication::postEvent(emulatedCommandBarTextEdit(), shiftKeyDown);
  QApplication::sendPostedEvents();
  TestPressKey("+");
  QKeyEvent *shiftKeyUp = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
  QApplication::postEvent(emulatedCommandBarTextEdit(), shiftKeyUp);
  QApplication::sendPostedEvents();
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("vimodetestclipboardtext"));
  TestPressKey("\\enter");
  FinishTest("xyz");

  // Similarly, test that we can press "ctrl" after ctrl-r without it being taken for a register.
  BeginTest("wordundercursor");
  TestPressKey("/\\ctrl-r");
  QKeyEvent *ctrlKeyDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Control, Qt::NoModifier);
  QApplication::postEvent(emulatedCommandBarTextEdit(), ctrlKeyDown);
  QApplication::sendPostedEvents();
  QKeyEvent *ctrlKeyUp = new QKeyEvent(QEvent::KeyRelease, Qt::Key_Control, Qt::NoModifier);
  QApplication::postEvent(emulatedCommandBarTextEdit(), ctrlKeyUp);
  QApplication::sendPostedEvents();
  QVERIFY(waitingForRegisterIndicator->isVisible());
  TestPressKey("\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("wordundercursor"));
  TestPressKey("\\ctrl-c"); // Dismiss the bar.
  FinishTest("wordundercursor");

  // Begin tests for ctrl-g, which is almost identical to ctrl-r save that the contents, when added,
  // are escaped for searching.
  // Normal register contents/ word under cursor are added as normal.
  BeginTest("wordinregisterb wordundercursor");
  TestPressKey("\"byiw");
  TestPressKey("/\\ctrl-g");
  QVERIFY(waitingForRegisterIndicator->isVisible());
  QVERIFY(waitingForRegisterIndicator->x() >= emulatedCommandBarTextEdit()->x() + emulatedCommandBarTextEdit()->width());
  TestPressKey("b");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("wordinregisterb"));
  QVERIFY(!waitingForRegisterIndicator->isVisible());
  TestPressKey("\\ctrl-c\\ctrl-cw/\\ctrl-g\\ctrl-w");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("wordundercursor"));
  QVERIFY(!waitingForRegisterIndicator->isVisible());
  TestPressKey("\\ctrl-c");
  TestPressKey("\\ctrl-c");
  FinishTest("wordinregisterb wordundercursor");

  // \'s must be escaped when inserted via ctrl-g.
  DoTest("foo a\\b\\\\c\\\\\\d", "wYb/\\ctrl-g0\\enterrX", "foo X\\b\\\\c\\\\\\d");
  // $'s must be escaped when inserted via ctrl-g.
  DoTest("foo a$b", "wYb/\\ctrl-g0\\enterrX", "foo X$b");
  DoTest("foo a$b$c", "wYb/\\ctrl-g0\\enterrX", "foo X$b$c");
  DoTest("foo a\\$b\\$c", "wYb/\\ctrl-g0\\enterrX", "foo X\\$b\\$c");
  // ^'s must be escaped when inserted via ctrl-g.
  DoTest("foo a^b", "wYb/\\ctrl-g0\\enterrX", "foo X^b");
  DoTest("foo a^b^c", "wYb/\\ctrl-g0\\enterrX", "foo X^b^c");
  DoTest("foo a\\^b\\^c", "wYb/\\ctrl-g0\\enterrX", "foo X\\^b\\^c");
  // .'s must be escaped when inserted via ctrl-g.
  DoTest("foo axb a.b", "wwYgg/\\ctrl-g0\\enterrX", "foo axb X.b");
  DoTest("foo a\\xb Na\\.b", "fNlYgg/\\ctrl-g0\\enterrX", "foo a\\xb NX\\.b");
  // *'s must be escaped when inserted via ctrl-g
  DoTest("foo axxxxb ax*b", "wwYgg/\\ctrl-g0\\enterrX", "foo axxxxb Xx*b");
  DoTest("foo a\\xxxxb Na\\x*X", "fNlYgg/\\ctrl-g0\\enterrX", "foo a\\xxxxb NX\\x*X");
  // /'s must be escaped when inserted via ctrl-g.
  DoTest("foo a a/b", "wwYgg/\\ctrl-g0\\enterrX", "foo a X/b");
  DoTest("foo a a/b/c", "wwYgg/\\ctrl-g0\\enterrX", "foo a X/b/c");
  DoTest("foo a a\\/b\\/c", "wwYgg/\\ctrl-g0\\enterrX", "foo a X\\/b\\/c");
  // ['s and ]'s must be escaped when inserted via ctrl-g.
  DoTest("foo axb a[xyz]b", "wwYgg/\\ctrl-g0\\enterrX", "foo axb X[xyz]b");
  DoTest("foo a[b", "wYb/\\ctrl-g0\\enterrX", "foo X[b");
  DoTest("foo a[b[c", "wYb/\\ctrl-g0\\enterrX", "foo X[b[c");
  DoTest("foo a\\[b\\[c", "wYb/\\ctrl-g0\\enterrX", "foo X\\[b\\[c");
  DoTest("foo a]b", "wYb/\\ctrl-g0\\enterrX", "foo X]b");
  DoTest("foo a]b]c", "wYb/\\ctrl-g0\\enterrX", "foo X]b]c");
  DoTest("foo a\\]b\\]c", "wYb/\\ctrl-g0\\enterrX", "foo X\\]b\\]c");
  // Test that expressions involving {'s and }'s work when inserted via ctrl-g.
  DoTest("foo {", "wYgg/\\ctrl-g0\\enterrX", "foo X");
  DoTest("foo }", "wYgg/\\ctrl-g0\\enterrX", "foo X");
  DoTest("foo aaaaa \\aaaaa a\\{5}", "WWWYgg/\\ctrl-g0\\enterrX", "foo aaaaa \\aaaaa X\\{5}");
  DoTest("foo }", "wYgg/\\ctrl-g0\\enterrX", "foo X");
  // Transform newlines into "\\n" when inserted via ctrl-g.
  DoTest(" \nfoo\nfoo\nxyz\nbar\n123", "jjvjjllygg/\\ctrl-g0\\enterrX", " \nfoo\nXoo\nxyz\nbar\n123");
  DoTest(" \nfoo\nfoo\nxyz\nbar\n123", "jjvjjllygg/\\ctrl-g0/e\\enterrX", " \nfoo\nfoo\nxyz\nbaX\n123");
  // Don't do any escaping for ctrl-r, though.
  BeginTest("foo .*$^\\/");
  TestPressKey("wY/\\ctrl-r0");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".*$^\\/"));
  TestPressKey("\\ctrl-c");
  TestPressKey("\\ctrl-c");
  FinishTest("foo .*$^\\/");
  // Ensure that the flag that says "next register insertion should be escaped for searching"
  // is cleared if we do ctrl-g but then abort with ctrl-c.
  DoTest("foo a$b", "/\\ctrl-g\\ctrl-c\\ctrl-cwYgg/\\ctrl-r0\\enterrX", "Xoo a$b");

   // Ensure that we actually perform a search while typing.
  BeginTest("abcd");
  TestPressKey("/c");
  verifyCursorAt(Cursor(0, 2));
  TestPressKey("\\enter");
  FinishTest("abcd");

  // Ensure that the search is from the cursor.
  BeginTest("acbcd");
  TestPressKey("ll/c");
  verifyCursorAt(Cursor(0, 3));
  TestPressKey("\\enter");
  FinishTest("acbcd");

  // Reset the cursor to the original position on Ctrl-C
  BeginTest("acbcd");
  TestPressKey("ll/c\\ctrl-crX");
  FinishTest("acXcd");

  // Reset the cursor to the original position on Ctrl-[
  BeginTest("acbcd");
  TestPressKey("ll/c\\ctrl-[rX");
  FinishTest("acXcd");

  // Reset the cursor to the original position on ESC
  BeginTest("acbcd");
  TestPressKey("ll/c\\escrX");
  FinishTest("acXcd");

  // *Do not* reset the cursor to the original position on Enter.
  BeginTest("acbcd");
  TestPressKey("ll/c\\enterrX");
  FinishTest("acbXd");

  // *Do not* reset the cursor to the original position on Return.
  BeginTest("acbcd");
  TestPressKey("ll/c\\returnrX");
  FinishTest("acbXd");

  // Should work with mappings.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'testmapping", "/c<enter>rX", KateViGlobal::Recursive);
  BeginTest("acbcd");
  TestPressKey("'testmapping");
  FinishTest("aXbcd");
  clearAllMappings();
  // Don't send keys that were part of a mapping to the emulated command bar.
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "H", "/a", KateViGlobal::Recursive);
  BeginTest("foo a aH");
  TestPressKey("H\\enterrX");
  FinishTest("foo X aH");
  clearAllMappings();

  // Incremental searching from the original position.
  BeginTest("foo bar foop fool food");
  TestPressKey("ll/foo");
  verifyCursorAt(Cursor(0, 8));
  TestPressKey("l");
  verifyCursorAt(Cursor(0, 13));
  TestPressKey("\\backspace");
  verifyCursorAt(Cursor(0, 8));
  TestPressKey("\\enter");
  FinishTest("foo bar foop fool food");

  // End up back at the start if no match found
  BeginTest("foo bar foop fool food");
  TestPressKey("ll/fool");
  verifyCursorAt(Cursor(0, 13));
  TestPressKey("\\backspacex");
  verifyCursorAt(Cursor(0, 2));
  TestPressKey("\\enter");
  FinishTest("foo bar foop fool food");

  // Wrap around if no match found.
  BeginTest("afoom bar foop fool food");
  TestPressKey("lll/foom");
  verifyCursorAt(Cursor(0, 1));
  TestPressKey("\\enter");
  FinishTest("afoom bar foop fool food");

  // SmartCase: match case-insensitively if the search text is all lower-case.
  DoTest("foo BaR", "ll/bar\\enterrX", "foo XaR");

  // SmartCase: match case-sensitively if the search text is mixed case.
  DoTest("foo BaR bAr", "ll/bAr\\enterrX", "foo BaR XAr");

  // Assume regex by default.
  DoTest("foo bwibblear", "ll/b.*ar\\enterrX", "foo Xwibblear");

  // Set the last search pattern.
  DoTest("foo bar", "ll/bar\\enterggnrX", "foo Xar");

  // Make sure the last search pattern is a regex, too.
  DoTest("foo bwibblear", "ll/b.*ar\\enterggnrX", "foo Xwibblear");

  // 'n' should search case-insensitively if the original search was case-insensitive.
  DoTest("foo bAR", "ll/bar\\enterggnrX", "foo XAR");

  // 'n' should search case-sensitively if the original search was case-sensitive.
  DoTest("foo bar bAR", "ll/bAR\\enterggnrX", "foo bar XAR");

  // 'N' should search case-insensitively if the original search was case-insensitive.
  DoTest("foo bAR xyz", "ll/bar\\enter$NrX", "foo XAR xyz");

  // 'N' should search case-sensitively if the original search was case-sensitive.
  DoTest("foo bAR bar", "ll/bAR\\enter$NrX", "foo XAR bar");

  // Don't forget to set the last search to case-insensitive.
  DoTest("foo bAR bar", "ll/bAR\\enter^/bar\\enter^nrX", "foo XAR bar");

  // Usage of \C for manually specifying case sensitivity.
  // Strip occurrences of "\C" from the pattern to find.
  DoTest("foo bar", "/\\\\Cba\\\\Cr\\enterrX", "foo Xar");
  // Be careful about escaping, though!
  DoTest("foo \\Cba\\Cr", "/\\\\\\\\Cb\\\\Ca\\\\\\\\C\\\\C\\\\Cr\\enterrX", "foo XCba\\Cr");
  // The item added to the search history should contain all the original \C's.
  clearSearchHistory();
  BeginTest("foo \\Cba\\Cr");
  TestPressKey("/\\\\\\\\Cb\\\\Ca\\\\\\\\C\\\\C\\\\Cr\\enterrX");
  QCOMPARE(searchHistory().first(), QString("\\\\Cb\\Ca\\\\C\\C\\Cr"));
  FinishTest("foo XCba\\Cr");
  // If there is an escaped C, assume case sensitivity.
  DoTest("foo bAr BAr bar", "/ba\\\\Cr\\enterrX", "foo bAr BAr Xar");
  // The last search pattern should be the last search with escaped C's stripped.
  DoTest("foo \\Cbar\nfoo \\Cbar", "/\\\\\\\\Cba\\\\C\\\\Cr\\enterggjnrX", "foo \\Cbar\nfoo XCbar");
  // If the last search pattern had an escaped "\C", then the next search should be case-sensitive.
  DoTest("foo bar\nfoo bAr BAr bar", "/ba\\\\Cr\\enterggjnrX", "foo bar\nfoo bAr BAr Xar");

  // Don't set the last search parameters if we abort, though.
  DoTest("foo bar xyz", "/bar\\enter/xyz\\ctrl-cggnrX", "foo Xar xyz");
  DoTest("foo bar bAr", "/bar\\enter/bA\\ctrl-cggnrX", "foo Xar bAr");
  DoTest("foo bar bar", "/bar\\enter?ba\\ctrl-cggnrX", "foo Xar bar");

  // Don't let ":" trample all over the search parameters, either.
  DoTest("foo bar xyz foo", "/bar\\entergg*:yank\\enterggnrX", "foo bar xyz Xoo");

  // Some mirror tests for "?"

  // Test that "?" summons the search bar, with empty text and with the "?" indicator.
  QVERIFY(!emulatedCommandBar->isVisible());
  BeginTest("");
  TestPressKey("?");
  QVERIFY(emulatedCommandBar->isVisible());
  QCOMPARE(emulatedCommandTypeIndicator()->text(), QString("?"));
  QVERIFY(emulatedCommandTypeIndicator()->isVisible());
  QVERIFY(emulatedCommandBarTextEdit());
  QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
  TestPressKey("\\enter");
  FinishTest("");

  // Search backwards.
  DoTest("foo foo bar foo foo", "ww?foo\\enterrX", "foo Xoo bar foo foo");

  // Reset cursor if we find nothing.
  BeginTest("foo foo bar foo foo");
  TestPressKey("ww?foo");
  verifyCursorAt(Cursor(0, 4));
  TestPressKey("d");
  verifyCursorAt(Cursor(0, 8));
  TestPressKey("\\enter");
  FinishTest("foo foo bar foo foo");

  // Wrap to the end if we find nothing.
  DoTest("foo foo bar xyz xyz", "ww?xyz\\enterrX", "foo foo bar xyz Xyz");

  // Specify that the last was backwards when using '?'
  DoTest("foo foo bar foo foo", "ww?foo\\enter^wwnrX", "foo Xoo bar foo foo");

  // ... and make sure we do  the equivalent with "/"
  BeginTest("foo foo bar foo foo");
  TestPressKey("ww?foo\\enter^ww/foo");
  QCOMPARE(emulatedCommandTypeIndicator()->text(), QString("/"));
  TestPressKey("\\enter^wwnrX");
  FinishTest("foo foo bar Xoo foo");

  // If we are at the beginning of a word, that word is not the first match in a search
  // for that word.
  DoTest("foo foo foo", "w/foo\\enterrX", "foo foo Xoo");
  DoTest("foo foo foo", "w?foo\\enterrX", "Xoo foo foo");
  // When searching backwards, ensure we can find a match whose range includes the starting cursor position,
  // if we allow it to wrap around.
  DoTest("foo foofoofoo bar", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo bar");
  // When searching backwards, ensure we can find a match whose range includes the starting cursor position,
  // even if we don't allow it to wrap around.
  DoTest("foo foofoofoo foofoofoo", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo foofoofoo");
  // The same, but where we the match ends at the end of the line or document.
  DoTest("foo foofoofoo\nfoofoofoo", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo\nfoofoofoo");
  DoTest("foo foofoofoo", "wlll?foofoofoo\\enterrX", "foo Xoofoofoo");

  // Searching forwards for just "/" repeats last search.
  DoTest("foo bar", "/bar\\entergg//\\enterrX", "foo Xar");
  // The "last search" can be one initiated via e.g. "*".
  DoTest("foo bar foo", "/bar\\entergg*gg//\\enterrX", "foo bar Xoo");
  // Searching backwards for just "?" repeats last search.
  DoTest("foo bar bar", "/bar\\entergg??\\enterrX", "foo bar Xar");
  // Search forwards treats "?" as a literal.
  DoTest("foo ?ba?r", "/?ba?r\\enterrX", "foo Xba?r");
  // As always, be careful with escaping!
  DoTest("foo ?ba\\?r", "/?ba\\\\\\\\\\\\?r\\enterrX", "foo Xba\\?r");
  // Searching forwards for just "?" finds literal question marks.
  DoTest("foo ??", "/?\\enterrX", "foo X?");
  // Searching backwards for just "/" finds literal forward slashes.
  DoTest("foo //", "?/\\enterrX", "foo /X");
  // Searching forwards, stuff after (and including) an unescaped "/" is ignored.
  DoTest("foo ba bar bar/xyz", "/bar/xyz\\enterrX", "foo ba Xar bar/xyz");
  // Needs to be unescaped, though!
  DoTest("foo bar bar/xyz", "/bar\\\\/xyz\\enterrX", "foo bar Xar/xyz");
  DoTest("foo bar bar\\/xyz", "/bar\\\\\\\\/xyz\\enterrX", "foo bar Xar\\/xyz");
  // Searching backwards, stuff after (and including) an unescaped "?" is ignored.
  DoTest("foo bar bar?xyz bar ba", "?bar?xyz\\enterrX", "foo bar bar?xyz Xar ba");
  // Needs to be unescaped, though!
  DoTest("foo bar bar?xyz bar ba", "?bar\\\\?xyz\\enterrX", "foo bar Xar?xyz bar ba");
  DoTest("foo bar bar\\?xyz bar ba", "?bar\\\\\\\\?xyz\\enterrX", "foo bar Xar\\?xyz bar ba");
  // If, in a forward search, the first character after the first unescaped "/" is an e, then
  // we place the cursor at the end of the word.
  DoTest("foo ba bar bar/eyz", "/bar/e\\enterrX", "foo ba baX bar/eyz");
  // Needs to be unescaped, though!
  DoTest("foo bar bar/eyz", "/bar\\\\/e\\enterrX", "foo bar Xar/eyz");
  DoTest("foo bar bar\\/xyz", "/bar\\\\\\\\/e\\enterrX", "foo bar barX/xyz");
  // If, in a backward search, the first character after the first unescaped "?" is an e, then
  // we place the cursor at the end of the word.
  DoTest("foo bar bar?eyz bar ba", "?bar?e\\enterrX", "foo bar bar?eyz baX ba");
  // Needs to be unescaped, though!
  DoTest("foo bar bar?eyz bar ba", "?bar\\\\?e\\enterrX", "foo bar Xar?eyz bar ba");
  DoTest("foo bar bar\\?eyz bar ba", "?bar\\\\\\\\?e\\enterrX", "foo bar barX?eyz bar ba");
  // Quick check that repeating the last search and placing the cursor at the end of the match works.
  DoTest("foo bar bar", "/bar\\entergg//e\\enterrX", "foo baX bar");
  DoTest("foo bar bar", "?bar\\entergg??e\\enterrX", "foo bar baX");
  // When repeating a change, don't try to convert from Vim to Qt regex again.
  DoTest("foo bar()", "/bar()\\entergg//e\\enterrX", "foo bar(X");
  DoTest("foo bar()", "?bar()\\entergg??e\\enterrX", "foo bar(X");
  // If the last search said that we should place the cursor at the end of the match, then
  // do this with n & N.
  DoTest("foo bar bar foo", "/bar/e\\enterggnrX", "foo baX bar foo");
  DoTest("foo bar bar foo", "/bar/e\\enterggNrX", "foo bar baX foo");
  // Don't do this if that search was aborted, though.
  DoTest("foo bar bar foo", "/bar\\enter/bar/e\\ctrl-cggnrX", "foo Xar bar foo");
  DoTest("foo bar bar foo", "/bar\\enter/bar/e\\ctrl-cggNrX", "foo bar Xar foo");
  // "#" and "*" reset the "place cursor at the end of the match" to false.
  DoTest("foo bar bar foo", "/bar/e\\enterggw*nrX", "foo Xar bar foo");
  DoTest("foo bar bar foo", "/bar/e\\enterggw#nrX", "foo Xar bar foo");

  // "/" and "?" should be usable as motions.
  DoTest("foo bar", "ld/bar\\enter", "fbar");
  // They are not linewise.
  DoTest("foo bar\nxyz", "ld/yz\\enter", "fyz");
  DoTest("foo bar\nxyz", "jld?oo\\enter", "fyz");
  // Should be usable in Visual Mode without aborting Visual Mode.
  DoTest("foo bar", "lv/bar\\enterd", "far");
  // Same for ?.
  DoTest("foo bar", "$hd?oo\\enter", "far");
  DoTest("foo bar", "$hv?oo\\enterd", "fr");
  DoTest("foo bar", "lv?bar\\enterd", "far");
  // If we abort the "/" / "?" motion, the command should be aborted, too.
  DoTest("foo bar", "d/bar\\esc", "foo bar");
  DoTest("foo bar", "d/bar\\ctrl-c", "foo bar");
  DoTest("foo bar", "d/bar\\ctrl-[", "foo bar");
  // We should be able to repeat a command using "/" or "?" as the motion.
  DoTest("foo bar bar bar", "d/bar\\enter.", "bar bar");
  // The "synthetic" Enter keypress should not be logged as part of the command to be repeated.
  DoTest("foo bar bar bar\nxyz", "d/bar\\enter.rX", "Xar bar\nxyz");
  // Counting.
  DoTest("foo bar bar bar", "2/bar\\enterrX", "foo bar Xar bar");
  // Counting with wraparound.
  DoTest("foo bar bar bar", "4/bar\\enterrX", "foo Xar bar bar");
  // Counting in Visual Mode.
  DoTest("foo bar bar bar", "v2/bar\\enterd", "ar bar");
  // Should update the selection in Visual Mode as we search.
  BeginTest("foo bar bbc");
  TestPressKey("vl/b");
  QCOMPARE(kate_view->selectionText(), QString("foo b"));
  TestPressKey("b");
  QCOMPARE(kate_view->selectionText(), QString("foo bar b"));
  TestPressKey("\\ctrl-h");
  QCOMPARE(kate_view->selectionText(), QString("foo b"));
  TestPressKey("notexists");
  QCOMPARE(kate_view->selectionText(), QString("fo"));
  TestPressKey("\\enter"); // Dismiss bar.
  QCOMPARE(kate_view->selectionText(), QString("fo"));
  FinishTest("foo bar bbc");
  BeginTest("foo\nxyz\nbar\nbbc");
  TestPressKey("Vj/b");
  QCOMPARE(kate_view->selectionText(), QString("foo\nxyz\nbar"));
  TestPressKey("b");
  QCOMPARE(kate_view->selectionText(), QString("foo\nxyz\nbar\nbbc"));
  TestPressKey("\\ctrl-h");
  QCOMPARE(kate_view->selectionText(), QString("foo\nxyz\nbar"));
  TestPressKey("notexists");
  QCOMPARE(kate_view->selectionText(), QString("foo\nxyz"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo\nxyz\nbar\nbbc");
  // Dismissing the search bar in visual mode should leave original selection.
  BeginTest("foo bar bbc");
  TestPressKey("vl/\\ctrl-c");
  QCOMPARE(kate_view->selectionText(), QString("fo"));
  FinishTest("foo bar bbc");
  BeginTest("foo bar bbc");
  TestPressKey("vl?\\ctrl-c");
  QCOMPARE(kate_view->selectionText(), QString("fo"));
  FinishTest("foo bar bbc");
  BeginTest("foo bar bbc");
  TestPressKey("vl/b\\ctrl-c");
  QCOMPARE(kate_view->selectionText(), QString("fo"));
  FinishTest("foo bar bbc");
  BeginTest("foo\nbar\nbbc");
  TestPressKey("Vl/b\\ctrl-c");
  QCOMPARE(kate_view->selectionText(), QString("foo"));
  FinishTest("foo\nbar\nbbc");

  // Search-highlighting tests.
  const QColor searchHighlightColour = kate_view->renderer()->config()->searchHighlightColor();
  BeginTest("foo bar xyz");
  // Sanity test.
  const QList<Kate::TextRange*> rangesInitial = rangesOnFirstLine();
  Q_ASSERT(rangesInitial.isEmpty() && "Assumptions about ranges are wrong - this test is invalid and may need updating!");
  FinishTest("foo bar xyz");

  // Test highlighting single character match.
  BeginTest("foo bar xyz");
  TestPressKey("/b");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->attribute()->background().color(), searchHighlightColour);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 4);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 5);
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Test highlighting two character match.
  BeginTest("foo bar xyz");
  TestPressKey("/ba");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 4);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 6);
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Test no highlighting if no longer a match.
  BeginTest("foo bar xyz");
  TestPressKey("/baz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

 // Test highlighting on wraparound.
  BeginTest(" foo bar xyz");
  TestPressKey("ww/foo");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 1);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 4);
  TestPressKey("\\enter");
  FinishTest(" foo bar xyz");

  // Test highlighting backwards
  BeginTest("foo bar xyz");
  TestPressKey("$?ba");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 4);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 6);
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Test no highlighting when no match is found searching backwards
  BeginTest("foo bar xyz");
  TestPressKey("$?baz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Test highlight when wrapping around after searching backwards.
  BeginTest("foo bar xyz");
  TestPressKey("w?xyz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 8);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 11);
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Test no highlighting when bar is dismissed.
  DoTest("foo bar xyz", "/bar\\ctrl-c", "foo bar xyz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  DoTest("foo bar xyz", "/bar\\enter", "foo bar xyz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  DoTest("foo bar xyz", "/bar\\ctrl-[", "foo bar xyz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  DoTest("foo bar xyz", "/bar\\return", "foo bar xyz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  DoTest("foo bar xyz", "/bar\\esc", "foo bar xyz");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());

  // Update colour on config change.
  BeginTest("foo bar xyz");
  TestPressKey("/xyz");
  const QColor newSearchHighlightColour = QColor(255, 0, 0);
  kate_view->renderer()->config()->setSearchHighlightColor(newSearchHighlightColour);
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->attribute()->background().color(), newSearchHighlightColour);
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Set the background colour appropriately.
  KColorScheme currentColorScheme(QPalette::Normal);
  const QColor normalBackgroundColour = QPalette().brush(QPalette::Base).color();
  const QColor matchBackgroundColour =  currentColorScheme.background(KColorScheme::PositiveBackground).color();
  const QColor noMatchBackgroundColour =  currentColorScheme.background(KColorScheme::NegativeBackground).color();
  BeginTest("foo bar xyz");
  TestPressKey("/xyz");
  verifyTextEditBackgroundColour(matchBackgroundColour);
  TestPressKey("a");
  verifyTextEditBackgroundColour(noMatchBackgroundColour);
  TestPressKey("\\ctrl-w");
  verifyTextEditBackgroundColour(normalBackgroundColour);
  TestPressKey("/xyz\\enter/");
  verifyTextEditBackgroundColour(normalBackgroundColour);
  TestPressKey("\\enter");
  FinishTest("foo bar xyz");

  // Escape regex's in a Vim-ish style.
  // Unescaped ( and ) are always literals.
  DoTest("foo bar( xyz", "/bar(\\enterrX", "foo Xar( xyz");
  DoTest("foo bar) xyz", "/bar)\\enterrX", "foo Xar) xyz");
  // + is literal, unless it is already escaped.
  DoTest("foo bar+ xyz", "/bar+ \\enterrX", "foo Xar+ xyz");
  DoTest("  foo+AAAAbar", "/foo+A\\\\+bar\\enterrX", "  Xoo+AAAAbar");
  DoTest("  foo++++bar", "/foo+\\\\+bar\\enterrX", "  Xoo++++bar");
  DoTest("  foo++++bar", "/+\\enterrX", "  fooX+++bar");
  // An escaped "\" is a literal, of course.
  DoTest("foo x\\y", "/x\\\\\\\\y\\enterrX", "foo X\\y");
  // ( and ), if escaped, are not literals.
  DoTest("foo  barbarxyz", "/ \\\\(bar\\\\)\\\\+xyz\\enterrX", "foo Xbarbarxyz");
  // Handle escaping correctly if we have an escaped and unescaped bracket next to each other.
  DoTest("foo  x(A)y", "/x(\\\\(.\\\\))y\\enterrX", "foo  X(A)y");
  // |, if unescaped, is literal.
  DoTest("foo |bar", "/|\\enterrX", "foo Xbar");
  // |, if escaped, is not a literal.
  DoTest("foo xfoo\\y xbary", "/x\\\\(foo\\\\|bar\\\\)y\\enterrX", "foo xfoo\\y Xbary");
  // A single [ is a literal.
  DoTest("foo bar[", "/bar[\\enterrX", "foo Xar[");
  // A single ] is a literal.
  DoTest("foo bar]", "/bar]\\enterrX", "foo Xar]");
  // A matching [ and ] are *not* literals.
  DoTest("foo xbcay", "/x[abc]\\\\+y\\enterrX", "foo Xbcay");
  DoTest("foo xbcay", "/[abc]\\\\+y\\enterrX", "foo xXcay");
  DoTest("foo xbaadcdcy", "/x[ab]\\\\+[cd]\\\\+y\\enterrX", "foo Xbaadcdcy");
  // Need to be an unescaped match, though.
  DoTest("foo xbcay", "/x[abc\\\\]\\\\+y\\enterrX", "Xoo xbcay");
  DoTest("foo xbcay", "/x\\\\[abc]\\\\+y\\enterrX", "Xoo xbcay");
  DoTest("foo x[abc]]]]]y", "/x\\\\[abc]\\\\+y\\enterrX", "foo X[abc]]]]]y");
  // An escaped '[' between matching unescaped '[' and ']' is treated as a literal '['
  DoTest("foo xb[cay", "/x[a\\\\[bc]\\\\+y\\enterrX", "foo Xb[cay");
  // An escaped ']' between matching unescaped '[' and ']' is treated as a literal ']'
  DoTest("foo xb]cay", "/x[a\\\\]bc]\\\\+y\\enterrX", "foo Xb]cay");
  // An escaped '[' not between other square brackets is a literal.
  DoTest("foo xb[cay", "/xb\\\\[\\enterrX", "foo Xb[cay");
  DoTest("foo xb[cay", "/\\\\[ca\\enterrX", "foo xbXcay");
  // An escaped ']' not between other square brackets is a literal.
  DoTest("foo xb]cay", "/xb\\\\]\\enterrX", "foo Xb]cay");
  DoTest("foo xb]cay", "/\\\\]ca\\enterrX", "foo xbXcay");
  // An unescaped '[' not between other square brackets is a literal.
  DoTest("foo xbaba[y", "/x[ab]\\\\+[y\\enterrX", "foo Xbaba[y");
  DoTest("foo xbaba[dcdcy", "/x[ab]\\\\+[[cd]\\\\+y\\enterrX", "foo Xbaba[dcdcy");
  // An unescaped ']' not between other square brackets is a literal.
  DoTest("foo xbaba]y", "/x[ab]\\\\+]y\\enterrX", "foo Xbaba]y");
  DoTest("foo xbaba]dcdcy", "/x[ab]\\\\+][cd]\\\\+y\\enterrX", "foo Xbaba]dcdcy");
  // Be more clever about how we indentify escaping: the presence of a preceding
  // backslash is not always sufficient!
  DoTest("foo x\\babay", "/x\\\\\\\\[ab]\\\\+y\\enterrX", "foo X\\babay");
  DoTest("foo x\\[abc]]]]y", "/x\\\\\\\\\\\\[abc]\\\\+y\\enterrX", "foo X\\[abc]]]]y");
  DoTest("foo xa\\b\\c\\y", "/x[abc\\\\\\\\]\\\\+y\\enterrX", "foo Xa\\b\\c\\y");
  DoTest("foo x[abc\\]]]]y", "/x[abc\\\\\\\\\\\\]\\\\+y\\enterrX", "foo X[abc\\]]]]y");
  DoTest("foo xa[\\b\\[y", "/x[ab\\\\\\\\[]\\\\+y\\enterrX", "foo Xa[\\b\\[y");
  DoTest("foo x\\[y", "/x\\\\\\\\[y\\enterrX", "foo X\\[y");
  DoTest("foo x\\]y", "/x\\\\\\\\]y\\enterrX", "foo X\\]y");
  DoTest("foo x\\+y", "/x\\\\\\\\+y\\enterrX", "foo X\\+y");
  // A dot is not a literal, nor is a star.
  DoTest("foo bar", "/o.*b\\enterrX", "fXo bar");
  // Escaped dots and stars are literals, though.
  DoTest("foo xay x.y", "/x\\\\.y\\enterrX", "foo xay X.y");
  DoTest("foo xaaaay xa*y", "/xa\\\\*y\\enterrX", "foo xaaaay Xa*y");
  // Unescaped curly braces are literals.
  DoTest("foo x{}y", "/x{}y\\enterrX", "foo X{}y");
  // Escaped curly brackets are quantifers.
  DoTest("foo xaaaaay", "/xa\\\\{5\\\\}y\\enterrX", "foo Xaaaaay");
  // Matching curly brackets where only the first is escaped are also quantifiers.
  DoTest("foo xaaaaaybbbz", "/xa\\\\{5}yb\\\\{3}z\\enterrX", "foo Xaaaaaybbbz");
  // Make sure it really is escaped, though!
  DoTest("foo xa\\{5}", "/xa\\\\\\\\{5}\\enterrX", "foo Xa\\{5}");
  // Don't crash if the first character is a }
  DoTest("foo aaaaay", "/{\\enterrX", "Xoo aaaaay");
  // Vim's '\<' and '\>' map, roughly, to Qt's '\b'
  DoTest("foo xbar barx bar", "/bar\\\\>\\enterrX", "foo xXar barx bar");
  DoTest("foo xbar barx bar", "/\\\\<bar\\enterrX", "foo xbar Xarx bar");
  DoTest("foo xbar barx bar ", "/\\\\<bar\\\\>\\enterrX", "foo xbar barx Xar ");
  DoTest("foo xbar barx bar", "/\\\\<bar\\\\>\\enterrX", "foo xbar barx Xar");
  DoTest("foo xbar barx\nbar", "/\\\\<bar\\\\>\\enterrX", "foo xbar barx\nXar");
  // Escaped "^" and "$" are treated as literals.
  DoTest("foo x^$y", "/x\\\\^\\\\$y\\enterrX", "foo X^$y");
  // Ensure that it is the escaped version of the pattern that is recorded as the last search pattern.
  DoTest("foo bar( xyz", "/bar(\\enterggnrX", "foo Xar( xyz");

  // Don't log keypresses sent to the emulated command bar as commands to be repeated via "."!
  DoTest("foo", "/diw\\enterciwbar\\ctrl-c.", "bar");

  // Don't leave Visual mode on aborting a search.
  DoTest("foo bar", "vw/\\ctrl-cd", "ar");
  DoTest("foo bar", "vw/\\ctrl-[d", "ar");

  // Don't crash on leaving Visual Mode on aborting a search. This is perhaps the most opaque regression
  // test ever; what it's testing for is the situation where the synthetic keypress issue by the emulated
  // command bar on the "ctrl-[" is sent to the key mapper.  This in turn converts it into a weird character
  // which is then, upon not being recognised as part of a mapping, sent back around the keypress processing,
  // where it ends up being sent to the emulated command bar's text edit, which in turn issues a "text changed"
  // event where the text is still empty, which tries to move the cursor to (-1, -1), which causes a crash deep
  // within Kate. So, in a nutshell: this test ensures that the keymapper never handles the synthetic keypress :)
  DoTest("", "ifoo\\ctrl-cv/\\ctrl-[", "foo");

  // History auto-completion tests.
  clearSearchHistory();
  QVERIFY(searchHistory().isEmpty());
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("bar");
  QCOMPARE(searchHistory(), QStringList() << "foo" << "bar");
  clearSearchHistory();
  QVERIFY(searchHistory().isEmpty());

  // Ensure current search bar text is added to the history if we press enter.
  DoTest("foo bar", "/bar\\enter", "foo bar");
  DoTest("foo bar", "/xyz\\enter", "foo bar");
  QCOMPARE(searchHistory(), QStringList() << "bar" << "xyz");
  // Interesting - Vim adds the search bar text to the history even if we abort via e.g. ctrl-c, ctrl-[, etc.
  clearSearchHistory();
  DoTest("foo bar", "/baz\\ctrl-[", "foo bar");
  QCOMPARE(searchHistory(), QStringList() << "baz");
  clearSearchHistory();
  DoTest("foo bar", "/foo\\esc", "foo bar");
  QCOMPARE(searchHistory(), QStringList() << "foo");
  clearSearchHistory();
  DoTest("foo bar", "/nose\\ctrl-c", "foo bar");
  QCOMPARE(searchHistory(), QStringList() << "nose");

  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("bar");
  QVERIFY(emulatedCommandBarCompleter() != NULL);
  BeginTest("foo bar");
  TestPressKey("/\\ctrl-p");
  verifyCommandBarCompletionVisible();
  // Make sure the completion appears in roughly the correct place: this is a little fragile :/
  const QPoint completerRectTopLeft = emulatedCommandBarCompleter()->popup()->mapToGlobal(emulatedCommandBarCompleter()->popup()->rect().topLeft()) ;
  const QPoint barEditBottomLeft = emulatedCommandBarTextEdit()->mapToGlobal(emulatedCommandBarTextEdit()->rect().bottomLeft());
  QCOMPARE(completerRectTopLeft.x(), barEditBottomLeft.x());
  QVERIFY(qAbs(completerRectTopLeft.y() - barEditBottomLeft.y()) <= 1);
  // Will activate the current completion item, activating the search, and dismissing the bar.
  TestPressKey("\\enter");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  // Close the command bar.
  FinishTest("foo bar");

  // Don't show completion with an empty search bar.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  BeginTest("foo bar");
  TestPressKey("/");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\enter");
  FinishTest("foo bar");

  // Don't auto-complete, either.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  BeginTest("foo bar");
  TestPressKey("/f");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\enter");
  FinishTest("foo bar");

  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("bar");
  QVERIFY(emulatedCommandBarCompleter() != NULL);
  BeginTest("foo bar");
  TestPressKey("/\\ctrl-p");
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("bar"));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("foo bar");

  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("bar");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  QVERIFY(emulatedCommandBarCompleter() != NULL);
  BeginTest("foo bar");
  TestPressKey("/\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo"));
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 0);
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("bar"));
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 1);
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("xyz"));
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 2);
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo")); // Wrap-around
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 0);
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("foo bar");

  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("bar");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  QVERIFY(emulatedCommandBarCompleter() != NULL);
  BeginTest("foo bar");
  TestPressKey("/\\ctrl-n");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("xyz"));
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 2);
  TestPressKey("\\ctrl-n");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("bar"));
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 1);
  TestPressKey("\\ctrl-n");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo"));
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 0);
  TestPressKey("\\ctrl-n");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("xyz")); // Wrap-around.
  QCOMPARE(emulatedCommandBarCompleter()->popup()->currentIndex().row(), 2);
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("foo bar");

  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("bar");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  BeginTest("foo bar");
  TestPressKey("/\\ctrl-n\\ctrl-n");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("bar"));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("foo bar");

  // If we add something to the history, remove any earliest occurrences (this is what Vim appears to do)
  // and append to the end.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("bar");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  QCOMPARE(searchHistory(), QStringList() << "bar" << "foo" << "xyz");

  // Push out older entries if we have too many search items in the history.
  const int HISTORY_SIZE_LIMIT = 100;
  clearSearchHistory();
  for (int i = 1; i <= HISTORY_SIZE_LIMIT; i++)
  {
    KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem(QString("searchhistoryitem %1").arg(i));
  }
  QCOMPARE(searchHistory().size(), HISTORY_SIZE_LIMIT);
  QCOMPARE(searchHistory().first(), QString("searchhistoryitem 1"));
  QCOMPARE(searchHistory().last(), QString("searchhistoryitem 100"));
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem(QString("searchhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));
  QCOMPARE(searchHistory().size(), HISTORY_SIZE_LIMIT);
  QCOMPARE(searchHistory().first(), QString("searchhistoryitem 2"));
  QCOMPARE(searchHistory().last(), QString("searchhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));



  // Don't add empty searches to the history.
  clearSearchHistory();
  DoTest("foo bar", "/\\enter", "foo bar");
  QVERIFY(searchHistory().isEmpty());

  // "*" and "#" should add the relevant word to the search history, enclosed between \< and \>
  clearSearchHistory();
  BeginTest("foo bar");
  TestPressKey("*");
  QVERIFY(!searchHistory().isEmpty());
  QCOMPARE(searchHistory().last(), QString("\\<foo\\>"));
  TestPressKey("w#");
  QCOMPARE(searchHistory().size(), 2);
  QCOMPARE(searchHistory().last(), QString("\\<bar\\>"));

  // Auto-complete words from the document on ctrl-space.
  // Test that we can actually find a single word and add it to the list of completions.
  BeginTest("foo");
  TestPressKey("/\\ctrl- ");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo"));
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo");

  // Count digits and underscores as being part of a word.
  BeginTest("foo_12");
  TestPressKey("/\\ctrl- ");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo_12"));
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo_12");

  // This feels a bit better to me, usability-wise: in the special case of completion from document, where
  // the completion list is manually summoned, allow one to press Enter without the bar being dismissed
  // (just dismiss the completion list instead).
  BeginTest("foo_12");
  TestPressKey("/\\ctrl- \\ctrl-p\\enter");
  QVERIFY(emulatedCommandBar->isVisible());
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("foo_12");

  // Check that we can find multiple words on one line.
  BeginTest("bar (foo) [xyz]");
  TestPressKey("/\\ctrl- ");
  QStringListModel *completerStringListModel = dynamic_cast<QStringListModel*>(emulatedCommandBarCompleter()->model());
  Q_ASSERT(completerStringListModel);
  QCOMPARE(completerStringListModel->stringList(), QStringList() << "bar" << "foo" << "xyz");
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("bar (foo) [xyz]");

  // Check that we arrange the found words in case-insensitive sorted order.
  BeginTest("D c e a b f");
  TestPressKey("/\\ctrl- ");
  verifyCommandBarCompletionsMatches(QStringList() << "a" << "b" << "c" << "D" << "e" << "f");
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("D c e a b f");

  // Check that we don't include the same word multiple times.
  BeginTest("foo bar bar bar foo");
  TestPressKey("/\\ctrl- ");
  verifyCommandBarCompletionsMatches(QStringList() << "bar" << "foo");
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo bar bar bar foo");

  // Check that we search only a narrow portion of the document, around the cursor (4096 lines either
  // side, say).
  QStringList manyLines;
  for (int i = 1; i < 2 * 4096 + 3; i++)
  {
    // Pad the digits so that when sorted alphabetically, they are also sorted numerically.
    manyLines << QString("word%1").arg(i, 5, 10, QChar('0'));
  }
  QStringList allButFirstAndLastOfManyLines = manyLines;
  allButFirstAndLastOfManyLines.removeFirst();
  allButFirstAndLastOfManyLines.removeLast();

  BeginTest(manyLines.join("\n"));
  TestPressKey("4097j/\\ctrl- ");
  verifyCommandBarCompletionsMatches(allButFirstAndLastOfManyLines);
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest(manyLines.join("\n"));


  // "The current word" means the word before the cursor in the command bar, and includes numbers
  // and underscores. Make sure also that the completion prefix is set when the completion is first invoked.
  BeginTest("foo fee foa_11 foa_11b");
  // Write "bar(foa112$nose" and position cursor before the "2", then invoke completion.
  TestPressKey("/bar(foa_112$nose\\left\\left\\left\\left\\left\\left\\ctrl- ");
  verifyCommandBarCompletionsMatches(QStringList() << "foa_11" << "foa_11b");
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo fee foa_11 foa_11b");

  // But don't count "-" as being part of the current word.
  BeginTest("foo_12");
  TestPressKey("/bar-foo\\ctrl- ");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("foo_12"));
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo_12");

  // Be case insensitive.
  BeginTest("foo Fo12 fOo13 FO45");
  TestPressKey("/fo\\ctrl- ");
  verifyCommandBarCompletionsMatches(QStringList() << "Fo12" << "FO45" << "foo" << "fOo13");
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo Fo12 fOo13 FO45");

  // Feed the current word to complete to the completer as we type/ edit.
  BeginTest("foo fee foa foab");
  TestPressKey("/xyz|f\\ctrl- o");
  verifyCommandBarCompletionsMatches(QStringList() << "foa" << "foab" << "foo");
  TestPressKey("a");
  verifyCommandBarCompletionsMatches(QStringList() << "foa" << "foab");
  TestPressKey("\\ctrl-h");
  verifyCommandBarCompletionsMatches(QStringList() << "foa" << "foab" << "foo");
  TestPressKey("o");
  verifyCommandBarCompletionsMatches(QStringList() << "foo");
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo fee foa foab");

  // Upon selecting a completion with an empty command bar, add the completed text to the command bar.
  BeginTest("foo fee fob foables");
  TestPressKey("/\\ctrl- foa\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foables"));
  verifyCommandBarCompletionVisible();
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo fee fob foables");

  // If bar is non-empty, replace the word under the cursor.
  BeginTest("foo fee foa foab");
  TestPressKey("/xyz|f$nose\\left\\left\\left\\left\\left\\ctrl- oa\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz|foab$nose"));
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("foo fee foa foab");

  // Place the cursor at the end of the completed text.
  BeginTest("foo fee foa foab");
  TestPressKey("/xyz|f$nose\\left\\left\\left\\left\\left\\ctrl- oa\\ctrl-p\\enterX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz|foabX$nose"));
  TestPressKey("\\ctrl-c"); // Dismiss completion, then bar.
  FinishTest("foo fee foa foab");

  // If we're completing from history, though, the entire text gets set, and the completion prefix
  // is the beginning of the entire text, not the current word before the cursor.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo(bar");
  BeginTest("");
  TestPressKey("/foo(b\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList() << "foo(bar");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo(bar"));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("");

  // If we're completing from history and we abort the completion via ctrl-c or ctrl-[, we revert the whole
  // text to the last manually typed text.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo(b|ar");
  BeginTest("");
  TestPressKey("/foo(b\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList() << "foo(b|ar");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo(b|ar"));
  TestPressKey("\\ctrl-c"); // Dismiss completion.
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo(b"));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("");

  // Scroll completion list if necessary so that currently selected completion is visible.
  BeginTest("a b c d e f g h i j k l m n o p q r s t u v w x y z");
  TestPressKey("/\\ctrl- ");
  const int lastItemRow = 25;
  const QRect initialLastCompletionItemRect = emulatedCommandBarCompleter()->popup()->visualRect(emulatedCommandBarCompleter()->popup()->model()->index(lastItemRow, 0));
  QVERIFY(!emulatedCommandBarCompleter()->popup()->rect().contains(initialLastCompletionItemRect)); // If this fails, then we have an error in the test setup: initally, the last item in the list should be outside of the bounds of the popup.
  TestPressKey("\\ctrl-n");
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("z"));
  const QRect lastCompletionItemRect = emulatedCommandBarCompleter()->popup()->visualRect(emulatedCommandBarCompleter()->popup()->model()->index(lastItemRow, 0));
  QVERIFY(emulatedCommandBarCompleter()->popup()->rect().contains(lastCompletionItemRect));
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("a b c d e f g h i j k l m n o p q r s t u v w x y z");

  // Ensure that the completion list changes size appropriately as the number of candidate completions changes.
  BeginTest("a ab abc");
  TestPressKey("/\\ctrl- ");
  const int initialPopupHeight = emulatedCommandBarCompleter()->popup()->height();
  TestPressKey("ab");
  const int popupHeightAfterEliminatingOne = emulatedCommandBarCompleter()->popup()->height();
  QVERIFY(popupHeightAfterEliminatingOne < initialPopupHeight);
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("a ab abc");

  // Ensure that the completion list disappears when no candidate completions are found, but re-appears
  // when some are found.
  BeginTest("a ab abc");
  TestPressKey("/\\ctrl- ");
  TestPressKey("abd");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-h");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\enter\\enter"); // Dismiss completion, then bar.
  FinishTest("a ab abc");

  // ctrl-c and ctrl-[ when the completion list is visible should dismiss the completion list, but *not*
  // the emulated command bar. TODO - same goes for ESC, but this is harder as KateViewInternal dismisses it
  // itself.
  BeginTest("a ab abc");
  TestPressKey("/\\ctrl- \\ctrl-cdiw");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\enter"); // Dismiss bar.
  TestPressKey("/\\ctrl- \\ctrl-[diw");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("a ab abc");

  // If we implicitly choose an element from the summoned completion list (by highlighting it, then
  // continuing to edit the text), the completion box should not re-appear unless explicitly summoned
  // again, even if the current word has a valid completion.
  BeginTest("a ab abc");
  TestPressKey("/\\ctrl- \\ctrl-p");
  TestPressKey(".a");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("a ab abc");

  // If we dismiss the summoned completion list via ctrl-c or ctrl-[, it should not re-appear unless explicitly summoned
  // again, even if the current word has a valid completion.
  BeginTest("a ab abc");
  TestPressKey("/\\ctrl- \\ctrl-c");
  TestPressKey(".a");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\enter");
  TestPressKey("/\\ctrl- \\ctrl-[");
  TestPressKey(".a");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("a ab abc");

  // If we select a completion from an empty bar, but then dismiss it via ctrl-c or ctrl-[, then we
  // should restore the empty text.
  BeginTest("foo");
  TestPressKey("/\\ctrl- \\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  TestPressKey("\\ctrl-c");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  QVERIFY(emulatedCommandBar->isVisible());
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("foo");
  BeginTest("foo");
  TestPressKey("/\\ctrl- \\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("foo"));
  TestPressKey("\\ctrl-[");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  QVERIFY(emulatedCommandBar->isVisible());
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(""));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("foo");

  // If we select a completion but then dismiss it via ctrl-c or ctrl-[, then we
  // should restore the last manually typed word.
  BeginTest("fooabc");
  TestPressKey("/f\\ctrl- o\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fooabc"));
  TestPressKey("\\ctrl-c");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  QVERIFY(emulatedCommandBar->isVisible());
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("fo"));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("fooabc");

  // If we select a completion but then dismiss it via ctrl-c or ctrl-[, then we
  // should restore the word currently being typed to the last manually typed word.
  BeginTest("fooabc");
  TestPressKey("/ab\\ctrl- |fo\\ctrl-p");
  TestPressKey("\\ctrl-c");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("ab|fo"));
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("fooabc");

  // Set the completion prefix for the search history completion as soon as it is shown.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("foo(bar");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  BeginTest("");
  TestPressKey("/f\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList() << "foo(bar");
  TestPressKey("\\enter"); // Dismiss bar.
  FinishTest("");

  // Command Mode (:) tests.
  // ":" should summon the command bar, with ":" as the label.
  BeginTest("");
  TestPressKey(":");
  QVERIFY(emulatedCommandBar->isVisible());
  QCOMPARE(emulatedCommandTypeIndicator()->text(), QString(":"));
  QVERIFY(emulatedCommandTypeIndicator()->isVisible());
  QVERIFY(emulatedCommandBarTextEdit());
  QVERIFY(emulatedCommandBarTextEdit()->text().isEmpty());
  TestPressKey("\\esc");
  FinishTest("");

  // If we have a selection, it should be encoded as a range in the text edit.
  BeginTest("d\nb\na\nc");
  TestPressKey("Vjjj:");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>"));
  TestPressKey("\\esc");
  FinishTest("d\nb\na\nc");

  // If we have a count, it should be encoded as a range in the text edit.
  BeginTest("d\nb\na\nc");
  TestPressKey("7:");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6"));
  TestPressKey("\\esc");
  FinishTest("d\nb\na\nc");

  // Don't go doing an incremental search when we press keys!
  BeginTest("foo bar xyz");
  TestPressKey(":bar");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  TestPressKey("\\esc");
  FinishTest("foo bar xyz");

  // Execute the command on Enter.
  DoTest("d\nb\na\nc", "Vjjj:sort\\enter", "a\nb\nc\nd");

  // Bar background should always be normal for command bar.
  BeginTest("foo");
  TestPressKey("/foo\\enter:");
  verifyTextEditBackgroundColour(normalBackgroundColour);
  TestPressKey("\\ctrl-c/bar\\enter:");
  verifyTextEditBackgroundColour(normalBackgroundColour);
  TestPressKey("\\esc");
  FinishTest("foo");


  const int commandResponseMessageTimeOutMSOverride = QString::fromAscii(qgetenv("KATE_VIMODE_TEST_COMMANDRESPONSEMESSAGETIMEOUTMS")).toInt();
  const long commandResponseMessageTimeOutMS = (commandResponseMessageTimeOutMSOverride > 0) ? commandResponseMessageTimeOutMSOverride : 2000;
  {
  // If there is any output from the command, show it in a label for a short amount of time
  // (make sure the bar type indicator is hidden, here, as it looks messy).
  emulatedCommandBar->setCommandResponseMessageTimeout(commandResponseMessageTimeOutMS);
  BeginTest("foo bar xyz");
  const QDateTime timeJustBeforeCommandExecuted = QDateTime::currentDateTime();
  TestPressKey(":commandthatdoesnotexist\\enter");
  QVERIFY(emulatedCommandBar->isVisible());
  QVERIFY(commandResponseMessageDisplay());
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  QVERIFY(!emulatedCommandBarTextEdit()->isVisible());
  QVERIFY(!emulatedCommandTypeIndicator()->isVisible());
  // Be a bit vague about the exact message, due to i18n, etc.
  QVERIFY(commandResponseMessageDisplay()->text().contains("commandthatdoesnotexist"));
  waitForEmulatedCommandBarToHide(4 * commandResponseMessageTimeOutMS);
  QVERIFY(timeJustBeforeCommandExecuted.msecsTo(QDateTime::currentDateTime()) >= commandResponseMessageTimeOutMS);
  QVERIFY(!emulatedCommandBar->isVisible());
  // Piggy-back on this test, as the bug we're about to test for would actually make setting
  // up the conditions again in a separate test impossible ;)
  // When we next summon the bar, the response message should be invisible; the editor visible & editable;
  // and the bar type indicator visible again.
  TestPressKey("/");
  QVERIFY(!commandResponseMessageDisplay()->isVisible());
  QVERIFY(emulatedCommandBarTextEdit()->isVisible());
  QVERIFY(emulatedCommandBarTextEdit()->isEnabled());
  QVERIFY(emulatedCommandBar->isVisible());
  TestPressKey("\\esc"); // Dismiss the bar.
  FinishTest("foo bar xyz");
  }

  {
    // Show the same message twice in a row.
    BeginTest("foo bar xyz");
    TestPressKey(":othercommandthatdoesnotexist\\enter");
    QDateTime startWaitingForMessageToHide = QDateTime::currentDateTime();
    waitForEmulatedCommandBarToHide(4 * commandResponseMessageTimeOutMS);
    TestPressKey(":othercommandthatdoesnotexist\\enter");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    // Wait for it to disappear again, as a courtesy for the next test.
    waitForEmulatedCommandBarToHide(4 * commandResponseMessageTimeOutMS);
  }

  {
    // Emulated command bar should not steal keypresses when it is merely showing the results of an executed command.
    BeginTest("foo bar");
    TestPressKey(":commandthatdoesnotexist\\enterrX");
    Q_ASSERT_X(commandResponseMessageDisplay()->isVisible(), "running test", "Need to increase timeJustBeforeCommandExecuted!");
    FinishTest("Xoo bar");
  }

  {
    // Don't send the synthetic "enter" keypress (for making search-as-a-motion work) when we finally hide.
    BeginTest("foo bar\nbar");
    TestPressKey(":commandthatdoesnotexist\\enter");
    Q_ASSERT_X(commandResponseMessageDisplay()->isVisible(), "running test", "Need to increase timeJustBeforeCommandExecuted!");
    waitForEmulatedCommandBarToHide(commandResponseMessageTimeOutMS * 4);
    TestPressKey("rX");
    FinishTest("Xoo bar\nbar");
  }

  {
    // The timeout should be cancelled when we invoke the command bar again.
    BeginTest("");
    TestPressKey(":commandthatdoesnotexist\\enter");
    const QDateTime waitStartedTime = QDateTime::currentDateTime();
    TestPressKey(":");
    // Wait ample time for the timeout to fire.  Do not use waitForEmulatedCommandBarToHide for this!
    while(waitStartedTime.msecsTo(QDateTime::currentDateTime()) < commandResponseMessageTimeOutMS * 2)
    {
      QApplication::processEvents();
    }
    QVERIFY(emulatedCommandBar->isVisible());
    TestPressKey("\\esc"); // Dismiss the bar.
    FinishTest("");
  }

  {
    // The timeout should not cause kate_view to regain focus if we have manually taken it away.
    qDebug()<< " NOTE: this test is weirdly fragile, so if it starts failing, comment it out and e-mail me:  it may well be more trouble that it's worth.";
    BeginTest("");
    TestPressKey(":commandthatdoesnotexist\\enter");
    while (QApplication::hasPendingEvents())
    {
      // Wait for any focus changes to take effect.
      QApplication::processEvents();
    }
    const QDateTime waitStartedTime = QDateTime::currentDateTime();
    QLineEdit *dummyToFocus = new QLineEdit(QString("Sausage"), mainWindow);
    // Take focus away from kate_view by giving it to dummyToFocus.
    QApplication::setActiveWindow(mainWindow);
    kate_view->setFocus();
    mainWindowLayout->addWidget(dummyToFocus);
    dummyToFocus->show();
    dummyToFocus->setEnabled(true);
    dummyToFocus->setFocus();
    // Allow dummyToFocus to receive focus.
    while(!dummyToFocus->hasFocus())
    {
      QApplication::processEvents();
    }
    QVERIFY(dummyToFocus->hasFocus());
    // Wait ample time for the timeout to fire.  Do not use waitForEmulatedCommandBarToHide for this -
    // the bar never actually hides in this instance, and I think it would take some deep changes in
    // Kate to make it do so (the KateCommandLineBar as the same issue).
    while(waitStartedTime.msecsTo(QDateTime::currentDateTime()) < commandResponseMessageTimeOutMS * 2)
    {
      QApplication::processEvents();
    }
    QVERIFY(dummyToFocus->hasFocus());
    QVERIFY(emulatedCommandBar->isVisible());
    mainWindowLayout->removeWidget(dummyToFocus);
    // Restore focus to the kate_view.
    kate_view->setFocus();
    while(!kate_view->hasFocus())
    {
      QApplication::processEvents();
    }
    // *Now* wait for the command bar to disappear - giving kate_view focus should trigger it.
    waitForEmulatedCommandBarToHide(commandResponseMessageTimeOutMS * 4);
    FinishTest("");
  }

  {
    // No completion should be shown when the bar is first shown: this gives us an opportunity
    // to invoke command history via ctrl-p and ctrl-n.
    BeginTest("");
    TestPressKey(":");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");
  }

  {
    // Should be able to switch to completion from document, even when we have a completion from commands.
    BeginTest("soggy1 soggy2");
    TestPressKey(":so");
    verifyCommandBarCompletionContains(QStringList() << "sort");
    TestPressKey("\\ctrl- ");
    verifyCommandBarCompletionsMatches(QStringList() << "soggy1" << "soggy2");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("soggy1 soggy2");
  }

  {
    // If we dismiss the command completion then change the text, it should summon the completion
    // again.
    BeginTest("");
    TestPressKey(":so");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("r");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionContains(QStringList() << "sort");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("");
  }

  {
    // Completion should be dismissed when we are showing command response text.
    BeginTest("");
    TestPressKey(":set-au\\enter");
    QVERIFY(commandResponseMessageDisplay()->isVisible());
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    waitForEmulatedCommandBarToHide(commandResponseMessageTimeOutMS * 4);
    FinishTest("");
  }

  // If we abort completion via ctrl-c or ctrl-[, we should revert the current word to the last
  // manually entered word.
  BeginTest("");
  TestPressKey(":se\\ctrl-p");
  verifyCommandBarCompletionVisible();
  QVERIFY(emulatedCommandBarTextEdit()->text() != "se");
  TestPressKey("\\ctrl-c"); // Dismiss completer
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("se"));
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");

  // In practice, it's annoying if, as we enter ":s/se", completions pop up after the "se":
  // for now, only summon completion if we are on the first word in the text.
  BeginTest("");
  TestPressKey(":s/se");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");
  BeginTest("");
  TestPressKey(":.,.+7s/se");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");

  // Don't blank the text if we activate command history completion with no command history.
  BeginTest("");
  clearCommandHistory();
  TestPressKey(":s/se\\ctrl-p");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/se"));
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");

  // On completion, only update the command in front of the cursor.
  BeginTest("");
  clearCommandHistory();
  TestPressKey(":.,.+6s/se\\left\\left\\leftet-auto-in\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6set-auto-indent/se"));
  TestPressKey("\\ctrl-c"); // Dismiss completer.
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");

  // On completion, place the cursor after the new command.
  BeginTest("");
  clearCommandHistory();
  TestPressKey(":.,.+6s/fo\\left\\left\\leftet-auto-in\\ctrl-pX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6set-auto-indentX/fo"));
  TestPressKey("\\ctrl-c"); // Dismiss completer.
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");

  // "The current word", for Commands, can contain "-".
  BeginTest("");
  TestPressKey(":set-\\ctrl-p");
  verifyCommandBarCompletionVisible();
  QVERIFY(emulatedCommandBarTextEdit()->text() != "set-");
  QVERIFY(emulatedCommandBarCompleter()->currentCompletion().startsWith("set-"));
  QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
  TestPressKey("\\ctrl-c"); // Dismiss completion.
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  {
    // Don't switch from word-from-document to command-completion just because we press a key, though!
    BeginTest("soggy1 soggy2");
    TestPressKey(":\\ctrl- s");
    TestPressKey("o");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "soggy1" << "soggy2");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("soggy1 soggy2");
  }

  {
    // If we're in a place where there is no command completion allowed, don't go hiding the word
    // completion as we type.
    BeginTest("soggy1 soggy2");
    TestPressKey(":s/s\\ctrl- o");
    verifyCommandBarCompletionVisible();
    verifyCommandBarCompletionsMatches(QStringList() << "soggy1" << "soggy2");
    TestPressKey("\\ctrl-c"); // Dismiss completer
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("soggy1 soggy2");
  }

  {
    // Don't show command completion before we start typing a command: we want ctrl-p/n
    // to go through command history instead (we'll test for that second part later).
    BeginTest("soggy1 soggy2");
    TestPressKey(":");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-cvl:");
    QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
    TestPressKey("\\ctrl-c"); // Dismiss bar
    FinishTest("soggy1 soggy2");
  }

  {
    // Aborting ":" should leave us in normal mode with no selection.
    BeginTest("foo bar");
    TestPressKey("vw:\\ctrl-[");
    QVERIFY(kate_view->selectionText().isEmpty());
    TestPressKey("wdiw");
    BeginTest("foo ");
  }

  // Command history tests.
  clearCommandHistory();
  QVERIFY(commandHistory().isEmpty());
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("foo");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("bar");
  QCOMPARE(commandHistory(), QStringList() << "foo" << "bar");
  clearCommandHistory();
  QVERIFY(commandHistory().isEmpty());


  // If we add something to the history, remove any earliest occurrences (this is what Vim appears to do)
  // and append to the end.
  clearCommandHistory();
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("bar");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("xyz");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("foo");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("xyz");
  QCOMPARE(commandHistory(), QStringList() << "bar" << "foo" << "xyz");

  // Push out older entries if we have too many command items in the history.
  clearCommandHistory();
  for (int i = 1; i <= HISTORY_SIZE_LIMIT; i++)
  {
    KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem(QString("commandhistoryitem %1").arg(i));
  }
  QCOMPARE(commandHistory().size(), HISTORY_SIZE_LIMIT);
  QCOMPARE(commandHistory().first(), QString("commandhistoryitem 1"));
  QCOMPARE(commandHistory().last(), QString("commandhistoryitem 100"));
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem(QString("commandhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));
  QCOMPARE(commandHistory().size(), HISTORY_SIZE_LIMIT);
  QCOMPARE(commandHistory().first(), QString("commandhistoryitem 2"));
  QCOMPARE(commandHistory().last(), QString("commandhistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));

  // Don't add empty commands to the history.
  clearCommandHistory();
  DoTest("foo bar", ":\\enter", "foo bar");
  QVERIFY(commandHistory().isEmpty());

  clearCommandHistory();
  BeginTest("");
  TestPressKey(":sort\\enter");
  QCOMPARE(commandHistory(), QStringList() << "sort");
  TestPressKey(":yank\\enter");
  QCOMPARE(commandHistory(), QStringList() << "sort" << "yank");
  // Add to history immediately: don't wait for the command response display to timeout.
  TestPressKey(":commandthatdoesnotexist\\enter");
  QCOMPARE(commandHistory(), QStringList() << "sort" << "yank" << "commandthatdoesnotexist");
  // Vim adds aborted commands to the history too, oddly.
  TestPressKey(":abortedcommand\\ctrl-c");
  QCOMPARE(commandHistory(), QStringList() << "sort" << "yank" << "commandthatdoesnotexist" << "abortedcommand");
  // Only add for commands, not searches!
  TestPressKey("/donotaddme\\enter?donotaddmeeither\\enter/donotaddme\\ctrl-c?donotaddmeeither\\ctrl-c");
  QCOMPARE(commandHistory(), QStringList() << "sort" << "yank" << "commandthatdoesnotexist" << "abortedcommand");
  FinishTest("");

  // Commands should not be added to the search history!
  clearCommandHistory();
  clearSearchHistory();
  BeginTest("");
  TestPressKey(":sort\\enter");
  QVERIFY(searchHistory().isEmpty());
  FinishTest("");

  // With an empty command bar, ctrl-p / ctrl-n should go through history.
  clearCommandHistory();
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("command1");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("command2");
  BeginTest("");
  TestPressKey(":\\ctrl-p");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("command2"));
  QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");
  clearCommandHistory();
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("command1");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("command2");
  BeginTest("");
  TestPressKey(":\\ctrl-n");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("command1"));
  QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");

  // If we're at a place where command completions are not allowed, ctrl-p/n should go through history.
  clearCommandHistory();
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("s/command1");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("s/command2");
  BeginTest("");
  TestPressKey(":s/\\ctrl-p");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("s/command2"));
  QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");
  clearCommandHistory();
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("s/command1");
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("s/command2");
  BeginTest("");
  TestPressKey(":s/\\ctrl-n");
  verifyCommandBarCompletionVisible();
  QCOMPARE(emulatedCommandBarCompleter()->currentCompletion(), QString("s/command1"));
  QCOMPARE(emulatedCommandBarTextEdit()->text(), emulatedCommandBarCompleter()->currentCompletion());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("");

  // Cancelling word-from-document completion should revert the whole text to what it was before.
  BeginTest("sausage bacon");
  TestPressKey(":s/b\\ctrl- \\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/bacon"));
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/b"));
  TestPressKey("\\ctrl-c"); // Dismiss bar
  FinishTest("sausage bacon");

  // "Replace" history tests.
  clearReplaceHistory();
  QVERIFY(replaceHistory().isEmpty());
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("foo");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("bar");
  QCOMPARE(replaceHistory(), QStringList() << "foo" << "bar");
  clearReplaceHistory();
  QVERIFY(replaceHistory().isEmpty());

  // If we add something to the history, remove any earliest occurrences (this is what Vim appears to do)
  // and append to the end.
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("bar");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("xyz");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("foo");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("xyz");
  QCOMPARE(replaceHistory(), QStringList() << "bar" << "foo" << "xyz");

  // Push out older entries if we have too many replace items in the history.
  clearReplaceHistory();
  for (int i = 1; i <= HISTORY_SIZE_LIMIT; i++)
  {
    KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem(QString("replacehistoryitem %1").arg(i));
  }
  QCOMPARE(replaceHistory().size(), HISTORY_SIZE_LIMIT);
  QCOMPARE(replaceHistory().first(), QString("replacehistoryitem 1"));
  QCOMPARE(replaceHistory().last(), QString("replacehistoryitem 100"));
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem(QString("replacehistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));
  QCOMPARE(replaceHistory().size(), HISTORY_SIZE_LIMIT);
  QCOMPARE(replaceHistory().first(), QString("replacehistoryitem 2"));
  QCOMPARE(replaceHistory().last(), QString("replacehistoryitem %1").arg(HISTORY_SIZE_LIMIT + 1));

  // Don't add empty replaces to the history.
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("");
  QVERIFY(replaceHistory().isEmpty());

  // Some misc SedReplace tests.
  DoTest("x\\/y", ":s/\\\\//replace/g\\enter", "x\\replacey");
  DoTest("x\\/y", ":s/\\\\\\\\\\\\//replace/g\\enter", "xreplacey");
  DoTest("x\\/y", ":s:/:replace:g\\enter", "x\\replacey");
  DoTest("foo\nbar\nxyz", ":%delete\\enter", "");
  DoTest("foo\nbar\nxyz\nbaz", "jVj:delete\\enter", "foo\nbaz");
  DoTest("foo\nbar\nxyz\nbaz", "j2:delete\\enter", "foo\nbaz");
  // On ctrl-d, delete the "search" term in a s/search/replace/xx
  BeginTest("foo bar");
  TestPressKey(":s/x\\\\\\\\\\\\/yz/rep\\\\\\\\\\\\/lace/g\\ctrl-d");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//rep\\\\\\/lace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Move cursor to position of deleted search term.
  BeginTest("foo bar");
  TestPressKey(":s/x\\\\\\\\\\\\/yz/rep\\\\\\\\\\\\/lace/g\\ctrl-dX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/X/rep\\\\\\/lace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Do nothing on ctrl-d in search mode.
  BeginTest("foo bar");
  TestPressKey("/s/search/replace/g\\ctrl-d");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
  TestPressKey("\\ctrl-c?s/searchbackwards/replace/g\\ctrl-d");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/searchbackwards/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // On ctrl-f, delete "replace" term in a s/search/replace/xx
  BeginTest("foo bar");
  TestPressKey(":s/a\\\\\\\\\\\\/bc/rep\\\\\\\\\\\\/lace/g\\ctrl-f");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/a\\\\\\/bc//g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Move cursor to position of deleted replace term.
  BeginTest("foo bar");
  TestPressKey(":s:a/bc:replace:g\\ctrl-fX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:a/bc:X:g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Do nothing on ctrl-d in search mode.
  BeginTest("foo bar");
  TestPressKey("/s/search/replace/g\\ctrl-f");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
  TestPressKey("\\ctrl-c?s/searchbackwards/replace/g\\ctrl-f");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/searchbackwards/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Do nothing on ctrl-d / ctrl-f if the current expression is not a sed expression.
  BeginTest("foo bar");
  TestPressKey(":s/notasedreplaceexpression::gi\\ctrl-f\\ctrl-dX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/notasedreplaceexpression::giX"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Need to convert Vim-style regex's to Qt one's in Sed Replace.
  DoTest("foo xbacba(boo)|[y", ":s/x[abc]\\\\+(boo)|[y/boo/g\\enter", "foo boo");
  DoTest("foo xbacba(boo)|[y\nfoo xbacba(boo)|[y", "Vj:s/x[abc]\\\\+(boo)|[y/boo/g\\enter", "foo boo\nfoo boo");
  // Just convert the search term, please :)
  DoTest("foo xbacba(boo)|[y", ":s/x[abc]\\\\+(boo)|[y/boo()/g\\enter", "foo boo()");
  // With an empty search expression, ctrl-d should still position the cursor correctly.
  BeginTest("foo bar");
  TestPressKey(":s//replace/g\\ctrl-dX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/X/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":s::replace:g\\ctrl-dX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:X:replace:g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // With an empty replace expression, ctrl-f should still position the cursor correctly.
  BeginTest("foo bar");
  TestPressKey(":s/sear\\\\/ch//g\\ctrl-fX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/sear\\/ch/X/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":s:sear\\\\:ch::g\\ctrl-fX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:sear\\:ch:X:g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // With both empty search *and* replace expressions, ctrl-f should still position the cursor correctly.
  BeginTest("foo bar");
  TestPressKey(":s///g\\ctrl-fX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//X/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":s:::g\\ctrl-fX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s::X:g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Should be able to undo ctrl-f or ctrl-d.
  BeginTest("foo bar");
  TestPressKey(":s/find/replace/g\\ctrl-d");
  emulatedCommandBarTextEdit()->undo();
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/find/replace/g"));
  TestPressKey("\\ctrl-f");
  emulatedCommandBarTextEdit()->undo();
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/find/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // ctrl-f / ctrl-d should cleanly finish sed find/ replace history completion.
  clearReplaceHistory();
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("searchxyz");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("replacexyz");
  TestPressKey(":s///g\\ctrl-d\\ctrl-p");
  QVERIFY(emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-f");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/searchxyz//g"));
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-p");
  QVERIFY(emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-d");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//replacexyz/g"));
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Don't hang if we execute a sed replace with empty search term.
  DoTest("foo bar", ":s//replace/g\\enter", "foo bar");

  // ctrl-f & ctrl-d should work even when there is a range expression at the beginning of the sed replace.
  BeginTest("foo bar");
  TestPressKey(":'<,'>s/search/replace/g\\ctrl-d");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>s//replace/g"));
  TestPressKey("\\ctrl-c:.,.+6s/search/replace/g\\ctrl-f");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6s/search//g"));
  TestPressKey("\\ctrl-c:%s/search/replace/g\\ctrl-f");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("%s/search//g"));
  // Place the cursor in the right place even when there is a range expression.
  TestPressKey("\\ctrl-c:.,.+6s/search/replace/g\\ctrl-fX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+6s/search/X/g"));
  TestPressKey("\\ctrl-c:%s/search/replace/g\\ctrl-fX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("%s/search/X/g"));
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("foo bar");
  // Don't crash on ctrl-f/d if we have an empty command.
  DoTest("", ":\\ctrl-f\\ctrl-d\\ctrl-c", "");
  // Parser regression test: Don't crash on ctrl-f/d with ".,.+".
  DoTest("", ":.,.+\\ctrl-f\\ctrl-d\\ctrl-c", "");

  // Command-completion should be invoked on the command being typed even when preceded by a range expression.
  BeginTest("");
  TestPressKey(":0,'>so");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Command-completion should ignore the range expression.
  BeginTest("");
  TestPressKey(":.,.+6so");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // A sed-replace should immediately add the search term to the search history.
  clearSearchHistory();
  BeginTest("");
  TestPressKey(":s/search/replace/g\\enter");
  QCOMPARE(searchHistory(), QStringList() << "search");
  FinishTest("");

  // An aborted sed-replace should not add the search term to the search history.
  clearSearchHistory();
  BeginTest("");
  TestPressKey(":s/search/replace/g\\ctrl-c");
  QCOMPARE(searchHistory(), QStringList());
  FinishTest("");

  // A non-sed-replace should leave the search history unchanged.
  clearSearchHistory();
  BeginTest("");
  TestPressKey(":s,search/replace/g\\enter");
  QCOMPARE(searchHistory(), QStringList());
  FinishTest("");

  // A sed-replace should immediately add the replace term to the replace history.
  clearReplaceHistory();
  BeginTest("");
  TestPressKey(":s/search/replace/g\\enter");
  QCOMPARE(replaceHistory(), QStringList() << "replace");
  clearReplaceHistory();
  TestPressKey(":'<,'>s/search/replace1/g\\enter");
  QCOMPARE(replaceHistory(), QStringList() << "replace1");
  FinishTest("");

  // An aborted sed-replace should not add the replace term to the replace history.
  clearReplaceHistory();
  BeginTest("");
  TestPressKey(":s/search/replace/g\\ctrl-c");
  QCOMPARE(replaceHistory(), QStringList());
  FinishTest("");

  // A non-sed-replace should leave the replace history unchanged.
  clearReplaceHistory();
  BeginTest("");
  TestPressKey(":s,search/replace/g\\enter");
  QCOMPARE(replaceHistory(), QStringList());
  FinishTest("");

  // Misc tests for sed replace.  These are for the *generic* Kate sed replace; they should all
  // use ViModeTests' built-in command execution stuff (\\:<commandtoexecute>\\\) rather than
  // invoking a KateViEmulatedCommandBar and potentially doing some Vim-specific transforms to
  // the command.
  DoTest("foo foo foo", "\\:s/foo/bar/\\", "bar foo foo");
  DoTest("foo foo xyz foo", "\\:s/foo/bar/g\\", "bar bar xyz bar");
  DoTest("foofooxyzfoo", "\\:s/foo/bar/g\\", "barbarxyzbar");
  DoTest("foofooxyzfoo", "\\:s/foo/b/g\\", "bbxyzb");
  DoTest("ffxyzf", "\\:s/f/b/g\\", "bbxyzb");
  DoTest("ffxyzf", "\\:s/f/bar/g\\", "barbarxyzbar");
  DoTest("foo Foo fOO FOO foo", "\\:s/foo/bar/\\", "bar Foo fOO FOO foo");
  DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/\\", "Foo bar fOO FOO foo");
  DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/g\\", "Foo bar fOO FOO bar");
  DoTest("foo Foo fOO FOO foo", "\\:s/foo/bar/i\\", "bar Foo fOO FOO foo");
  DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/i\\", "bar foo fOO FOO foo");
  DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/gi\\", "bar bar bar bar bar");
  DoTest("Foo foo fOO FOO foo", "\\:s/foo/bar/ig\\", "bar bar bar bar bar");
  // There are some oddities to do with how ViModeTest's "execute command directly" stuff works with selected ranges:
  // basically, we need to do our selection in Visual mode, then exit back to Normal mode before running the
  //command.
  DoTest("foo foo\nbar foo foo\nxyz foo foo\nfoo bar foo", "jVj\\esc\\:'<,'>s/foo/bar/\\", "foo foo\nbar bar foo\nxyz bar foo\nfoo bar foo");
  DoTest("foo foo\nbar foo foo\nxyz foo foo\nfoo bar foo", "jVj\\esc\\:'<,'>s/foo/bar/g\\", "foo foo\nbar bar bar\nxyz bar bar\nfoo bar foo");
  DoTest("Foo foo fOO FOO foo", "\\:s/foo/barfoo/g\\", "Foo barfoo fOO FOO barfoo");
  DoTest("Foo foo fOO FOO foo", "\\:s/foo/foobar/g\\", "Foo foobar fOO FOO foobar");
  DoTest("axyzb", "\\:s/a(.*)b/d\\\\1f/\\", "dxyzf");
  DoTest("ayxzzyxzfddeefdb", "\\:s/a([xyz]+)([def]+)b/<\\\\1|\\\\2>/\\", "<yxzzyxz|fddeefd>");
  DoTest("foo", "\\:s/.*//g\\", "");
  DoTest("foo", "\\:s/.*/f/g\\", "f");
  DoTest("foo/bar", "\\:s/foo\\\\/bar/123\\\\/xyz/g\\", "123/xyz");
  DoTest("foo:bar", "\\:s:foo\\\\:bar:123\\\\:xyz:g\\", "123:xyz");
  const bool oldReplaceTabsDyn = kate_document->config()->replaceTabsDyn();
  kate_document->config()->setReplaceTabsDyn(false);
  DoTest("foo\tbar", "\\:s/foo\\\\tbar/replace/g\\", "replace");
  DoTest("foo\tbar", "\\:s/foo\\\\tbar/rep\\\\tlace/g\\", "rep\tlace");
  kate_document->config()->setReplaceTabsDyn(oldReplaceTabsDyn);
  DoTest("foo", "\\:s/foo/replaceline1\\\\nreplaceline2/g\\", "replaceline1\nreplaceline2");
  DoTest("foofoo", "\\:s/foo/replaceline1\\\\nreplaceline2/g\\", "replaceline1\nreplaceline2replaceline1\nreplaceline2");
  DoTest("foofoo\nfoo", "\\:s/foo/replaceline1\\\\nreplaceline2/g\\", "replaceline1\nreplaceline2replaceline1\nreplaceline2\nfoo");
  DoTest("fooafoob\nfooc\nfood", "Vj\\esc\\:'<,'>s/foo/replaceline1\\\\nreplaceline2/g\\", "replaceline1\nreplaceline2areplaceline1\nreplaceline2b\nreplaceline1\nreplaceline2c\nfood");
  DoTest("fooafoob\nfooc\nfood", "Vj\\esc\\:'<,'>s/foo/replaceline1\\\\nreplaceline2/\\", "replaceline1\nreplaceline2afoob\nreplaceline1\nreplaceline2c\nfood");
  DoTest("fooafoob\nfooc\nfood", "Vj\\esc\\:'<,'>s/foo/replaceline1\\\\nreplaceline2\\\\nreplaceline3/g\\", "replaceline1\nreplaceline2\nreplaceline3areplaceline1\nreplaceline2\nreplaceline3b\nreplaceline1\nreplaceline2\nreplaceline3c\nfood");
  DoTest("foofoo", "\\:s/foo/replace\\\\nfoo/g\\", "replace\nfooreplace\nfoo");
  DoTest("foofoo", "\\:s/foo/replacefoo\\\\nfoo/g\\", "replacefoo\nfooreplacefoo\nfoo");
  DoTest("foofoo", "\\:s/foo/replacefoo\\\\n/g\\", "replacefoo\nreplacefoo\n");
  DoTest("ff", "\\:s/f/f\\\\nf/g\\", "f\nff\nf");
  DoTest("ff", "\\:s/f/f\\\\n/g\\", "f\nf\n");
  DoTest("foo\nbar", "\\:s/foo\\\\n//g\\", "bar");
  DoTest("foo\n\n\nbar", "\\:s/foo(\\\\n)*bar//g\\", "");
  DoTest("foo\n\n\nbar", "\\:s/foo(\\\\n*)bar/123\\\\1456/g\\", "123\n\n\n456");
  DoTest("xAbCy", "\\:s/x(.)(.)(.)y/\\\\L\\\\1\\\\U\\\\2\\\\3/g\\", "aBC");
  DoTest("foo", "\\:s/foo/\\\\a/g\\", "\x07");
  // End "generic" (i.e. not involving any Vi mode tricks/ transformations) sed replace tests: the remaining
  // ones should go via the KateViEmulatedCommandBar.
  BeginTest("foo foo\nxyz\nfoo");
  TestPressKey(":%s/foo/bar/g\\enter");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(3, 2);
  FinishTest("bar bar\nxyz\nbar");

  // ctrl-p on the first character of the search term in a sed-replace should
  // invoke search history completion.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("search");
  BeginTest("");
  TestPressKey(":s/search/replace/g\\ctrl-b\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s/search/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // ctrl-p on the last character of the search term in a sed-replace should
  // invoke search history completion.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  BeginTest("");
  TestPressKey(":s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // ctrl-p on some arbitrary character of the search term in a sed-replace should
  // invoke search history completion.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyzaaaaaa");
  BeginTest("");
  TestPressKey(":s/xyzaaaaaa/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s/xyzaaaaaa/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // ctrl-p on some character *after" the search term should
  // *not* invoke search history completion.
  // Note: in s/xyz/replace/g, the "/" after the "z" is counted as part of the find term;
  // this allows us to do xyz<ctrl-p> and get completions.
  clearSearchHistory();
  clearCommandHistory();
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyz");
  BeginTest("");
  TestPressKey(":s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\ctrl-p");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  clearSearchHistory();
  clearCommandHistory();
  TestPressKey(":'<,'>s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.

  // Make sure it's the search history we're invoking.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyzaaaaaa");
  BeginTest("");
  TestPressKey(":s//replace/g\\ctrl-b\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList() << "xyzaaaaaa");
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":.,.+6s//replace/g\\ctrl-b\\right\\right\\right\\right\\right\\right\\right\\ctrl-p");
  verifyCommandBarCompletionsMatches(QStringList() << "xyzaaaaaa");
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // (Search history should be reversed).
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyzaaaaaa");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("abc");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("def");
  BeginTest("");
  TestPressKey(":s//replace/g\\ctrl-b\\right\\right\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList()  << "def" << "abc" << "xyzaaaaaa");
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Completion prefix is the current find term.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xy:zaaaaaa");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("abc");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("def");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xy:zbaaaaa");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xy:zcaaaaa");
  BeginTest("");
  TestPressKey(":s//replace/g\\ctrl-dxy:z\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList()  << "xy:zcaaaaa" << "xy:zbaaaaa" << "xy:zaaaaaa");
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Replace entire search term with completion.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("ab,cd");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("ab,xy");
  BeginTest("");
  TestPressKey(":s//replace/g\\ctrl-dab,\\ctrl-p\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/ab,cd/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s//replace/g\\ctrl-dab,\\ctrl-p\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>s/ab,cd/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Place the cursor at the end of find term.
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("ab,xy");
  BeginTest("");
  TestPressKey(":s//replace/g\\ctrl-dab,\\ctrl-pX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/ab,xyX/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":.,.+7s//replace/g\\ctrl-dab,\\ctrl-pX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+7s/ab,xyX/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Leave find term unchanged if there is no search history.
  clearSearchHistory();
  BeginTest("");
  TestPressKey(":s/nose/replace/g\\ctrl-b\\right\\right\\right\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nose/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Leave cursor position unchanged if there is no search history.
  clearSearchHistory();
  BeginTest("");
  TestPressKey(":s/nose/replace/g\\ctrl-b\\right\\right\\right\\ctrl-pX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nXose/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // ctrl-p on the first character of the replace term in a sed-replace should
  // invoke replace history completion.
  clearSearchHistory();
  clearReplaceHistory();
  clearCommandHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("replace");
  BeginTest("");
  TestPressKey(":s/search/replace/g\\left\\left\\left\\left\\left\\left\\left\\left\\left\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s/search/replace/g\\left\\left\\left\\left\\left\\left\\left\\left\\left\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // ctrl-p on the last character of the replace term in a sed-replace should
  // invoke replace history completion.
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("replace");
  BeginTest("");
  TestPressKey(":s/xyz/replace/g\\left\\left\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s/xyz/replace/g\\left\\left\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // ctrl-p on some arbitrary character of the search term in a sed-replace should
  // invoke search history completion.
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("replaceaaaaaa");
  BeginTest("");
  TestPressKey(":s/xyzaaaaaa/replace/g\\left\\left\\left\\left\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s/xyzaaaaaa/replace/g\\left\\left\\left\\left\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // ctrl-p on some character *after" the replace term should
  // *not* invoke replace history completion.
  // Note: in s/xyz/replace/g, the "/" after the "e" is counted as part of the replace term;
  // this allows us to do replace<ctrl-p> and get completions.
  clearSearchHistory();
  clearCommandHistory();
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("xyz");
  BeginTest("");
  TestPressKey(":s/xyz/replace/g\\left\\ctrl-p");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  clearSearchHistory();
  clearCommandHistory();
  TestPressKey(":'<,'>s/xyz/replace/g\\left\\ctrl-p");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.

  // (Replace history should be reversed).
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("xyzaaaaaa");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("abc");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("def");
  BeginTest("");
  TestPressKey(":s/search//g\\left\\left\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList()  << "def" << "abc" << "xyzaaaaaa");
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Completion prefix is the current replace term.
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("xy:zaaaaaa");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("abc");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("def");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("xy:zbaaaaa");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("xy:zcaaaaa");
  BeginTest("");
  TestPressKey(":'<,'>s/replace/search/g\\ctrl-fxy:z\\ctrl-p");
  verifyCommandBarCompletionVisible();
  verifyCommandBarCompletionsMatches(QStringList()  << "xy:zcaaaaa" << "xy:zbaaaaa" << "xy:zaaaaaa");
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Replace entire search term with completion.
  clearReplaceHistory();
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("ab,cd");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("ab,xy");
  BeginTest("");
  TestPressKey(":s/search//g\\ctrl-fab,\\ctrl-p\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/ab,cd/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":'<,'>s/search//g\\ctrl-fab,\\ctrl-p\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("'<,'>s/search/ab,cd/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Place the cursor at the end of replace term.
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("ab,xy");
  BeginTest("");
  TestPressKey(":s/search//g\\ctrl-fab,\\ctrl-pX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/ab,xyX/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  TestPressKey(":.,.+7s/search//g\\ctrl-fab,\\ctrl-pX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString(".,.+7s/search/ab,xyX/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Leave replace term unchanged if there is no replace history.
  clearReplaceHistory();
  BeginTest("");
  TestPressKey(":s/nose/replace/g\\left\\left\\left\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nose/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Leave cursor position unchanged if there is no replace history.
  clearSearchHistory();
  BeginTest("");
  TestPressKey(":s/nose/replace/g\\left\\left\\left\\left\\ctrl-pX");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/nose/replaXce/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Invoke replacement history even when the "find" term is empty.
  BeginTest("");
  clearReplaceHistory();
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("ab,xy");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("whoops");
  TestPressKey(":s///g\\ctrl-f\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s//ab,xy/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Move the cursor back to the last manual edit point when aborting completion.
  BeginTest("");
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("xyzaaaaa");
  TestPressKey(":s/xyz/replace/g\\ctrl-b\\right\\right\\right\\right\\righta\\ctrl-p\\ctrl-[X");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/xyzaX/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Don't blank the "find" term if there is no search history that begins with the
  // current "find" term.
  BeginTest("");
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("doesnothavexyzasaprefix");
  TestPressKey(":s//replace/g\\ctrl-dxyz\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/xyz/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Escape the delimiter if it occurs in a search history term - searching for it likely won't
  // work, but at least it won't crash!
  BeginTest("");
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("search");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("aa/aa\\/a");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("ss/ss");
  TestPressKey(":s//replace/g\\ctrl-d\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/ss\\/ss/replace/g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/aa\\/aa\\/a/replace/g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  clearSearchHistory(); // Now do the same, but with a different delimiter.
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("search");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("aa:aa\\:a");
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("ss:ss");
  TestPressKey(":s::replace:g\\ctrl-d\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:ss\\:ss:replace:g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:aa\\:aa\\:a:replace:g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:replace:g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Remove \C if occurs in search history.
  BeginTest("");
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("s\\Cear\\\\Cch");
  TestPressKey(":s::replace:g\\ctrl-d\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:sear\\\\Cch:replace:g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Don't blank the "replace" term if there is no search history that begins with the
  // current "replace" term.
  BeginTest("");
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("doesnothavexyzasaprefix");
  TestPressKey(":s/search//g\\ctrl-fxyz\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/xyz/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Escape the delimiter if it occurs in a replace history term - searching for it likely won't
  // work, but at least it won't crash!
  BeginTest("");
  clearReplaceHistory();
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("replace");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("aa/aa\\/a");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("ss/ss");
  TestPressKey(":s/search//g\\ctrl-f\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/ss\\/ss/g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/aa\\/aa\\/a/g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/search/replace/g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  clearReplaceHistory(); // Now do the same, but with a different delimiter.
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("replace");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("aa:aa\\:a");
  KateGlobal::self()->viInputModeGlobal()->appendReplaceHistoryItem("ss:ss");
  TestPressKey(":s:search::g\\ctrl-f\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:ss\\:ss:g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:aa\\:aa\\:a:g"));
  TestPressKey("\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s:search:replace:g"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // In search mode, don't blank current text on completion if there is no item in the search history which
  // has the current text as a prefix.
  BeginTest("");
  clearSearchHistory();
  KateGlobal::self()->viInputModeGlobal()->appendSearchHistoryItem("doesnothavexyzasaprefix");
  TestPressKey("/xyz\\ctrl-p");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("xyz"));
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Don't dismiss the command completion just because the cursor ends up *temporarily* at a place where
  // command completion is disallowed when cycling through completions.
  BeginTest("");
  TestPressKey(":set/se\\left\\left\\left-\\ctrl-p");
  verifyCommandBarCompletionVisible();
  TestPressKey("\\ctrl-c"); // Dismiss completer
  TestPressKey("\\ctrl-c"); // Dismiss bar.
  FinishTest("");

  // Don't expand mappings meant for Normal mode in the emulated command bar.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "foo", "xyz", KateViGlobal::NonRecursive);
  DoTest("bar foo xyz", "/foo\\enterrX", "bar Xoo xyz");
  clearAllMappings();

  // Incremental search and replace.
  QLabel* interactiveSedReplaceLabel = emulatedCommandBar->findChild<QLabel*>("interactivesedreplace");
  QVERIFY(interactiveSedReplaceLabel);

  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter");
  QVERIFY(interactiveSedReplaceLabel->isVisible());
  QVERIFY(!commandResponseMessageDisplay()->isVisible());
  QVERIFY(!emulatedCommandBarTextEdit()->isVisible());
  QVERIFY(!emulatedCommandTypeIndicator()->isVisible());
  TestPressKey("\\ctrl-c"); // Dismiss search and replace.
  QVERIFY(!emulatedCommandBar->isVisible());
  FinishTest("foo");

  // Clear the flag that stops the command response from being shown after an incremental search and
  // replace, and also make sure that the edit and bar type indicator are not forcibly hidden.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter\\ctrl-c");
  TestPressKey(":s/foo/bar/");
  QVERIFY(emulatedCommandBarTextEdit()->isVisible());
  QVERIFY(emulatedCommandTypeIndicator()->isVisible());
  TestPressKey("\\enter");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  FinishTest("bar");

  // Hide the incremental search and replace label when we show the bar.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter\\ctrl-c");
  TestPressKey(":");
  QVERIFY(!interactiveSedReplaceLabel->isVisible());
  TestPressKey("\\ctrl-c");
  FinishTest("foo");

  // The "c" marker can be anywhere in the three chars following the delimiter.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/cgi\\enter");
  QVERIFY(interactiveSedReplaceLabel->isVisible());
  TestPressKey("\\ctrl-c");
  FinishTest("foo");
  BeginTest("foo");
  TestPressKey(":s/foo/bar/igc\\enter");
  QVERIFY(interactiveSedReplaceLabel->isVisible());
  TestPressKey("\\ctrl-c");
  FinishTest("foo");
  BeginTest("foo");
  TestPressKey(":s/foo/bar/icg\\enter");
  QVERIFY(interactiveSedReplaceLabel->isVisible());
  TestPressKey("\\ctrl-c");
  FinishTest("foo");
  BeginTest("foo");
  TestPressKey(":s/foo/bar/ic\\enter");
  QVERIFY(interactiveSedReplaceLabel->isVisible());
  TestPressKey("\\ctrl-c");
  FinishTest("foo");
  BeginTest("foo");
  TestPressKey(":s/foo/bar/ci\\enter");
  QVERIFY(interactiveSedReplaceLabel->isVisible());
  TestPressKey("\\ctrl-c");
  FinishTest("foo");

  // Emulated command bar is still active during an incremental search and replace.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter");
  TestPressKey("idef\\esc");
  FinishTest("foo");

  // Emulated command bar text is not edited during an incremental search and replace.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter");
  TestPressKey("def");
  QCOMPARE(emulatedCommandBarTextEdit()->text(), QString("s/foo/bar/c"));
  TestPressKey("\\ctrl-c");
  FinishTest("foo");

  // Pressing "n" when there is only a single  change we can make aborts incremental search
  // and replace.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter");
  TestPressKey("n");
  QVERIFY(!interactiveSedReplaceLabel->isVisible());
  TestPressKey("ixyz\\esc");
  FinishTest("xyzfoo");

  // Pressing "n" when there is only a single  change we can make aborts incremental search
  // and replace, and shows the no replacements on no lines.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter");
  TestPressKey("n");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(0, 0);
  FinishTest("foo");

  // First possible match is highlighted when we start an incremental search and replace, and
  // cleared if we press 'n'.
  BeginTest(" xyz  123 foo bar");
  TestPressKey(":s/foo/bar/gc\\enter");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 10);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 13);
  TestPressKey("n");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  FinishTest(" xyz  123 foo bar");

  // Second possible match highlighted if we start incremental search and replace and press 'n',
  // cleared if we press 'n' again.
  BeginTest(" xyz  123 foo foo bar");
  TestPressKey(":s/foo/bar/gc\\enter");
  TestPressKey("n");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 14);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 17);
  TestPressKey("n");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size());
  FinishTest(" xyz  123 foo foo bar");

  // Perform replacement if we press 'y' on the first match.
  BeginTest(" xyz  foo 123 foo bar");
  TestPressKey(":s/foo/bar/gc\\enter");
  TestPressKey("y");
  TestPressKey("\\ctrl-c");
  FinishTest(" xyz  bar 123 foo bar");

  // Replacement uses grouping, etc.
  BeginTest(" xyz  def 123 foo bar");
  TestPressKey(":s/d\\\\(e\\\\)\\\\(f\\\\)/x\\\\1\\\\U\\\\2/gc\\enter");
  TestPressKey("y");
  TestPressKey("\\ctrl-c");
  FinishTest(" xyz  xeF 123 foo bar");

  // On replacement, highlight next match.
  BeginTest(" xyz  foo 123 foo bar");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("y");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 14);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 17);
  TestPressKey("\\ctrl-c");
  FinishTest(" xyz  bar 123 foo bar");

  // On replacement, if there is no further match, abort incremental search and replace.
  BeginTest(" xyz  foo 123 foa bar");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("y");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  TestPressKey("ggidone\\esc");
  FinishTest("done xyz  bar 123 foa bar");

  // After replacement, the next match is sought after the end of the replacement text.
  BeginTest("foofoo");
  TestPressKey(":s/foo/barfoo/cg\\enter");
  TestPressKey("y");
  QCOMPARE(rangesOnFirstLine().size(), rangesInitial.size() + 1);
  QCOMPARE(rangesOnFirstLine().first()->start().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->start().column(), 6);
  QCOMPARE(rangesOnFirstLine().first()->end().line(), 0);
  QCOMPARE(rangesOnFirstLine().first()->end().column(), 9);
  TestPressKey("\\ctrl-c");
  FinishTest("barfoofoo");
  BeginTest("xffy");
  TestPressKey(":s/f/bf/cg\\enter");
  TestPressKey("yy");
  FinishTest("xbfbfy");

  // Make sure the incremental search bar label contains the "instruction" keypresses.
  const QString interactiveSedReplaceShortcuts = "(y/n/a/q/l)";
  BeginTest("foofoo");
  TestPressKey(":s/foo/barfoo/cg\\enter");
  QVERIFY(interactiveSedReplaceLabel->text().contains(interactiveSedReplaceShortcuts));
  TestPressKey("\\ctrl-c");
  FinishTest("foofoo");

  // Make sure the incremental search bar label contains a reference to the text we're going to
  // replace with.
  // We're going to be a bit vague about the precise text due to localisation issues.
  BeginTest("fabababbbar");
  TestPressKey(":s/f\\\\([ab]\\\\+\\\\)/1\\\\U\\\\12/c\\enter");
  QVERIFY(interactiveSedReplaceLabel->text().contains("1ABABABBBA2"));
  TestPressKey("\\ctrl-c");
  FinishTest("fabababbbar");

  // Replace newlines in the "replace?" message with "\\n"
  BeginTest("foo");
  TestPressKey(":s/foo/bar\\\\nxyz\\\\n123/c\\enter");
  kDebug(13070) << "Blah: " << interactiveSedReplaceLabel->text();
  QVERIFY(interactiveSedReplaceLabel->text().contains("bar\\nxyz\\n123"));
  TestPressKey("\\ctrl-c");
  FinishTest("foo");

  // Update the "confirm replace?" message on pressing "y".
  BeginTest("fabababbbar fabbb");
  TestPressKey(":s/f\\\\([ab]\\\\+\\\\)/1\\\\U\\\\12/gc\\enter");
  TestPressKey("y");
  QVERIFY(interactiveSedReplaceLabel->text().contains("1ABBB2"));
  QVERIFY(interactiveSedReplaceLabel->text().contains(interactiveSedReplaceShortcuts));
  TestPressKey("\\ctrl-c");
  FinishTest("1ABABABBBA2r fabbb");

  // Update the "confirm replace?" message on pressing "n".
  BeginTest("fabababbbar fabab");
  TestPressKey(":s/f\\\\([ab]\\\\+\\\\)/1\\\\U\\\\12/gc\\enter");
  TestPressKey("n");
  QVERIFY(interactiveSedReplaceLabel->text().contains("1ABAB2"));
  QVERIFY(interactiveSedReplaceLabel->text().contains(interactiveSedReplaceShortcuts));
  TestPressKey("\\ctrl-c");
  FinishTest("fabababbbar fabab");

  // Cursor is placed at the beginning of first match.
  BeginTest("  foo foo foo");
  TestPressKey(":s/foo/bar/c\\enter");
  verifyCursorAt(Cursor(0, 2));
  TestPressKey("\\ctrl-c");
  FinishTest("  foo foo foo");

  // "y" and "n" update the cursor pos.
  BeginTest("  foo   foo foo");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("y");
  verifyCursorAt(Cursor(0, 8));
  TestPressKey("n");
  verifyCursorAt(Cursor(0, 12));
  TestPressKey("\\ctrl-c");
  FinishTest("  bar   foo foo");

  // If we end due to a "y" or "n" on the final match, leave the cursor at the beginning of the final match.
  BeginTest("  foo");
  TestPressKey(":s/foo/bar/c\\enter");
  TestPressKey("y");
  verifyCursorAt(Cursor(0, 2));
  FinishTest("  bar");
  BeginTest("  foo");
  TestPressKey(":s/foo/bar/c\\enter");
  TestPressKey("n");
  verifyCursorAt(Cursor(0, 2));
  FinishTest("  foo");

  // Respect ranges.
  BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
  TestPressKey("jVj:s/foo/bar/gc\\enter");
  TestPressKey("ynny");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  TestPressKey("ggidone \\ctrl-c");
  FinishTest("done foo foo\nbar foo\nfoo bar\nfoo foo\n");
  BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
  TestPressKey("jVj:s/foo/bar/gc\\enter");
  TestPressKey("nyyn");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  TestPressKey("ggidone \\ctrl-c");
  FinishTest("done foo foo\nfoo bar\nbar foo\nfoo foo\n");
  BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
  TestPressKey("j:s/foo/bar/gc\\enter");
  TestPressKey("ny");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  TestPressKey("ggidone \\ctrl-c");
  FinishTest("done foo foo\nfoo bar\nfoo foo\nfoo foo\n");
  BeginTest("foo foo\nfoo foo\nfoo foo\nfoo foo\n");
  TestPressKey("j:s/foo/bar/gc\\enter");
  TestPressKey("yn");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  TestPressKey("ggidone \\ctrl-c");
  FinishTest("done foo foo\nbar foo\nfoo foo\nfoo foo\n");

  // If no initial match can be found, abort and show a "no replacements" message.
  // The cursor position should remain unnchanged.
  BeginTest("fab");
  TestPressKey("l:s/fee/bar/c\\enter");
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(0, 0);
  QVERIFY(!interactiveSedReplaceLabel->isVisible());
  TestPressKey("rX");
  BeginTest("fXb");

  // Case-sensitive by default.
  BeginTest("foo Foo FOo foo foO");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar Foo FOo bar foO");

  // Case-insensitive if "i" flag is used.
  BeginTest("foo Foo FOo foo foO");
  TestPressKey(":s/foo/bar/icg\\enter");
  TestPressKey("yyyyyggidone\\esc");
  FinishTest("donebar bar bar bar bar");

  // Only one replacement per-line unless "g" flag is used.
  BeginTest("boo foo 123 foo\nxyz foo foo\nfoo foo foo\nxyz\nfoo foo\nfoo 123 foo");
  TestPressKey("jVjjj:s/foo/bar/c\\enter");
  TestPressKey("yynggidone\\esc");
  FinishTest("doneboo foo 123 foo\nxyz bar foo\nbar foo foo\nxyz\nfoo foo\nfoo 123 foo");
  BeginTest("boo foo 123 foo\nxyz foo foo\nfoo foo foo\nxyz\nfoo foo\nfoo 123 foo");
  TestPressKey("jVjjj:s/foo/bar/c\\enter");
  TestPressKey("nnyggidone\\esc");
  FinishTest("doneboo foo 123 foo\nxyz foo foo\nfoo foo foo\nxyz\nbar foo\nfoo 123 foo");

  // If replacement contains new lines, adjust the end line down.
  BeginTest("foo\nfoo1\nfoo2\nfoo3");
  TestPressKey("jVj:s/foo/bar\\\\n/gc\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donefoo\nbar\n1\nbar\n2\nfoo3");
  BeginTest("foo\nfoo1\nfoo2\nfoo3");
  TestPressKey("jVj:s/foo/bar\\\\nboo\\\\n/gc\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donefoo\nbar\nboo\n1\nbar\nboo\n2\nfoo3");

  // With "g" and a replacement that involves multiple lines, resume search from the end of the last line added.
  BeginTest("foofoo");
  TestPressKey(":s/foo/bar\\\\n/gc\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar\nbar\n");
  BeginTest("foofoo");
  TestPressKey(":s/foo/bar\\\\nxyz\\\\nfoo/gc\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar\nxyz\nfoobar\nxyz\nfoo");

  // Without "g" and with a replacement that involves multiple lines, resume search from the line after the line just added.
  BeginTest("foofoo1\nfoo2\nfoo3");
  TestPressKey("Vj:s/foo/bar\\\\nxyz\\\\nfoo/c\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar\nxyz\nfoofoo1\nbar\nxyz\nfoo2\nfoo3");

  // Regression test: handle 'g' when it occurs before 'i' and 'c'.
  BeginTest("foo fOo");
  TestPressKey(":s/foo/bar/gci\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar bar");

  // When the search terms swallows several lines, move the endline up accordingly.
  BeginTest("foo\nfoo1\nfoo\nfoo2\nfoo\nfoo3");
  TestPressKey("V3j:s/foo\\\\nfoo/bar/cg\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar1\nbar2\nfoo\nfoo3");
  BeginTest("foo\nfoo\nfoo1\nfoo\nfoo\nfoo2\nfoo\nfoo\nfoo3");
  TestPressKey("V5j:s/foo\\\\nfoo\\\\nfoo/bar/cg\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar1\nbar2\nfoo\nfoo\nfoo3");
  // Make sure we still adjust endline down if the replacement text has '\n's.
  BeginTest("foo\nfoo\nfoo1\nfoo\nfoo\nfoo2\nfoo\nfoo\nfoo3");
  TestPressKey("V5j:s/foo\\\\nfoo\\\\nfoo/bar\\\\n/cg\\enter");
  TestPressKey("yyggidone\\esc");
  FinishTest("donebar\n1\nbar\n2\nfoo\nfoo\nfoo3");

  // Status reports.
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter");
  TestPressKey("y");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 1);
  FinishTest("bar");
  BeginTest("foo foo foo");
  TestPressKey(":s/foo/bar/gc\\enter");
  TestPressKey("yyy");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(3, 1);
  FinishTest("bar bar bar");
  BeginTest("foo foo foo");
  TestPressKey(":s/foo/bar/gc\\enter");
  TestPressKey("yny");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 1);
  FinishTest("bar foo bar");
  BeginTest("foo\nfoo");
  TestPressKey(":%s/foo/bar/gc\\enter");
  TestPressKey("yy");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 2);
  FinishTest("bar\nbar");
  BeginTest("foo foo\nfoo foo\nfoo foo");
  TestPressKey(":%s/foo/bar/gc\\enter");
  TestPressKey("yynnyy");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(4, 2);
  FinishTest("bar bar\nfoo foo\nbar bar");
  BeginTest("foofoo");
  TestPressKey(":s/foo/bar\\\\nxyz/gc\\enter");
  TestPressKey("yy");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 1);
  FinishTest("bar\nxyzbar\nxyz");
  BeginTest("foofoofoo");
  TestPressKey(":s/foo/bar\\\\nxyz\\\\nboo/gc\\enter");
  TestPressKey("yyy");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(3, 1);
  FinishTest("bar\nxyz\nboobar\nxyz\nboobar\nxyz\nboo");
  // Tricky one: how many lines are "touched" if a single replacement
  // swallows multiple lines? I'm going to say the number of lines swallowed.
  BeginTest("foo\nfoo\nfoo");
  TestPressKey(":s/foo\\\\nfoo\\\\nfoo/bar/c\\enter");
  TestPressKey("y");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 3);
  FinishTest("bar");
  BeginTest("foo\nfoo\nfoo\n");
  TestPressKey(":s/foo\\\\nfoo\\\\nfoo\\\\n/bar/c\\enter");
  TestPressKey("y");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 4);
  FinishTest("bar");

  // "Undo" undoes last replacement.
  BeginTest("foo foo foo foo");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("nyynu");
  FinishTest("foo bar foo foo");

  // "l" does the current replacement then exits.
  BeginTest("foo foo foo foo foo foo");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("nnl");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(1, 1);
  FinishTest("foo foo bar foo foo foo");

  // "q" just exits.
  BeginTest("foo foo foo foo foo foo");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("yyq");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(2, 1);
  FinishTest("bar bar foo foo foo foo");

  // "a" replaces all remaining, then exits.
  BeginTest("foo foo foo foo foo foo");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("nna");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(4, 1);
  FinishTest("foo foo bar bar bar bar");

  // The results of "a" can be undone in one go.
  BeginTest("foo foo foo foo foo foo");
  TestPressKey(":s/foo/bar/cg\\enter");
  TestPressKey("ya");
  verifyShowsNumberOfReplacementsAcrossNumberOfLines(6, 1);
  TestPressKey("u");
  FinishTest("bar foo foo foo foo foo");

  {
    // Test the test suite: ensure that shortcuts are still being sent and received correctly.
    FailsIfSlotNotCalled failsIfActionNotTriggered;
    QAction *dummyAction = kate_view->actionCollection()->addAction("Woo");
    dummyAction->setShortcut(QKeySequence("Ctrl+e"));
    QVERIFY(connect(dummyAction, SIGNAL(triggered()), &failsIfActionNotTriggered, SLOT(slot())));
    DoTest("foo", "\\ctrl-e", "foo");
    // Processing shortcuts seems to require events to be processed.
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
    delete dummyAction;
  }
  {
    // Test that shortcuts involving ctrl+<digit> work correctly.
    FailsIfSlotNotCalled failsIfActionNotTriggered;
    QAction *dummyAction = kate_view->actionCollection()->addAction("Woo");
    dummyAction->setShortcut(QKeySequence("Ctrl+1"));
    QVERIFY(connect(dummyAction, SIGNAL(triggered()), &failsIfActionNotTriggered, SLOT(slot())));
    DoTest("foo", "\\ctrl-1", "foo");
    // Processing shortcuts seems to require events to be processed.
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
    delete dummyAction;
  }
  {
    // Test that shortcuts involving alt+<digit> work correctly.
    FailsIfSlotNotCalled failsIfActionNotTriggered;
    QAction *dummyAction = kate_view->actionCollection()->addAction("Woo");
    dummyAction->setShortcut(QKeySequence("Alt+1"));
    QVERIFY(connect(dummyAction, SIGNAL(triggered()), &failsIfActionNotTriggered, SLOT(slot())));
    DoTest("foo", "\\alt-1", "foo");
    // Processing shortcuts seems to require events to be processed.
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
    delete dummyAction;
  }

  // Find the "Print" action for later use.
  QAction *printAction = NULL;
  foreach(QAction* action, kate_view->actionCollection()->actions())
  {
    if (action->shortcut() == QKeySequence("Ctrl+p"))
    {
      printAction = action;
      break;
    }
  }

  // Test that we don't inadvertantly trigger shortcuts in kate_view when typing them in the
  // emulated command bar.  Requires the above test for shortcuts to be sent and received correctly
  // to pass.
  {
    QVERIFY(mainWindow->isActiveWindow());
    QVERIFY(printAction);
    FailsIfSlotCalled failsIfActionTriggered("The kate_view shortcut should not be triggered by typing it in emulated  command bar!");
    // Don't invoke Print on failure, as this hangs instead of failing.
    //disconnect(printAction, SIGNAL(triggered(bool)), kate_document, SLOT(print()));
    connect(printAction, SIGNAL(triggered(bool)), &failsIfActionTriggered, SLOT(slot()));
    DoTest("foo bar foo bar", "/bar\\enterggd/\\ctrl-p\\enter.", "bar");
    // Processing shortcuts seems to require events to be processed.
    while (QApplication::hasPendingEvents())
    {
      QApplication::processEvents();
    }
  }

  // Test that the interactive search replace does not handle general keypresses like ctrl-p ("invoke
  // completion in emulated command bar").
  // Unfortunately, "ctrl-p" in kate_view, which is what will be triggered if this
  // test succeeds, hangs due to showing the print dialog, so we need to temporarily
  // block the Print action.
  clearCommandHistory();
  if (printAction)
  {
    printAction->blockSignals(true);
  }
  KateGlobal::self()->viInputModeGlobal()->appendCommandHistoryItem("s/foo/bar/caa");
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\ctrl-b\\enter\\ctrl-p");
  QVERIFY(!emulatedCommandBarCompleter()->popup()->isVisible());
  TestPressKey("\\ctrl-c");
  if (printAction)
  {
    printAction->blockSignals(false);
  }
  FinishTest("foo");

  // The interactive sed replace command is added to the history straight away.
  clearCommandHistory();
  BeginTest("foo");
  TestPressKey(":s/foo/bar/c\\enter");
  QCOMPARE(commandHistory(), QStringList() << "s/foo/bar/c");
  TestPressKey("\\ctrl-c");
  FinishTest("foo");
  clearCommandHistory();
  BeginTest("foo");
  TestPressKey(":s/notfound/bar/c\\enter");
  QCOMPARE(commandHistory(), QStringList() << "s/notfound/bar/c");
  TestPressKey("\\ctrl-c");
  FinishTest("foo");

  // Should be usable in mappings.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "H", ":s/foo/bar/gc<enter>nnyyl", KateViGlobal::Recursive);
  DoTest("foo foo foo foo foo foo", "H", "foo foo bar bar bar foo");
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "H", ":s/foo/bar/gc<enter>nna", KateViGlobal::Recursive);
  DoTest("foo foo foo foo foo foo", "H", "foo foo bar bar bar bar");
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "H", ":s/foo/bar/gc<enter>nnyqggidone<esc>", KateViGlobal::Recursive);
  DoTest("foo foo foo foo foo foo", "H", "donefoo foo bar foo foo foo");

  // Don't swallow "Ctrl+<key>" meant for the text edit.
  if (QKeySequence::keyBindings(QKeySequence::Undo).contains(QKeySequence("Ctrl+Z")))
  {
    DoTest("foo bar", "/bar\\ctrl-z\\enterrX", "Xoo bar");
  }
  else
  {
    qWarning() << "Skipped test: Ctrl+Z is not Undo on this platform";
  }

  // Don't give invalid cursor position to updateCursor in Visual Mode: it will cause a crash!
  DoTest("xyz\nfoo\nbar\n123", "/foo\\\\nbar\\\\n\\enterggv//e\\enter\\ctrl-crX", "xyz\nfoo\nbaX\n123");
  DoTest("\nfooxyz\nbar;\n" , "/foo.*\\\\n.*;\\enterggv//e\\enter\\ctrl-crX", "\nfooxyz\nbarX\n");
}

class VimCodeCompletionTestModel : public CodeCompletionModel
{
public:
    VimCodeCompletionTestModel(KTextEditor::View* parent)
      : KTextEditor::CodeCompletionModel(parent)
    {
        setRowCount(3);
        cc()->setAutomaticInvocationEnabled(true);
        cc()->unregisterCompletionModel(KateGlobal::self()->wordCompletionModel()); //would add additional items, we don't want that in tests
        cc()->registerCompletionModel(this);
    }
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
    {
      // Order is important, here, as the completion widget seems to do its own sorting.
      const char* completions[] = { "completion1", "completion2", "completion3" };
      if (role == Qt::DisplayRole)
      {
        if (index.column() == Name)
            return QString(completions[index.row()]);
      }
      return QVariant();
    }
    KTextEditor::CodeCompletionInterface * cc( ) const
    {
      return dynamic_cast<KTextEditor::CodeCompletionInterface*>(const_cast<QObject*>(QObject::parent()));
    }
};

class FailTestOnInvocationModel : public CodeCompletionModel
{
public:
    FailTestOnInvocationModel(KTextEditor::View* parent)
      : KTextEditor::CodeCompletionModel(parent)
    {
        setRowCount(3);
        cc()->setAutomaticInvocationEnabled(true);
        cc()->unregisterCompletionModel(KateGlobal::self()->wordCompletionModel()); //would add additional items, we don't want that in tests
        cc()->registerCompletionModel(this);
    }
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
    {
      Q_UNUSED(index);
      Q_UNUSED(role);
      failTest();
      return QVariant();
    }
    void failTest() const
    {
      QFAIL("Shouldn't be invoking me!");
    }
    KTextEditor::CodeCompletionInterface * cc( ) const
    {
      return dynamic_cast<KTextEditor::CodeCompletionInterface*>(const_cast<QObject*>(QObject::parent()));
    }
};

void ViModeTest::CompletionTests()
{
    const bool oldRemoveTailOnCompletion = KateViewConfig::global()->wordCompletionRemoveTail();
    // For these tests, assume we don't swallow the tail on completion.
    KateViewConfig::global()->setWordCompletionRemoveTail(false);

    KateViewConfig::global()->setViInputModeStealKeys(true); // For Ctrl-P, Ctrl-N etc
    ensureKateViewVisible(); // KateView needs to be visible for the completion widget.
    VimCodeCompletionTestModel *testModel = new VimCodeCompletionTestModel(kate_view);

    BeginTest("");
    TestPressKey("i\\ctrl-p");
    waitForCompletionWidgetToActivate();
    TestPressKey("\\return");
    FinishTest("completion3");

    BeginTest("");
    TestPressKey("i\\ctrl- ");
    waitForCompletionWidgetToActivate();
    TestPressKey("\\return");
    FinishTest("completion1");

    BeginTest("");
    TestPressKey("i\\ctrl-n");
    waitForCompletionWidgetToActivate();
    TestPressKey("\\return");
    FinishTest("completion1");

    // Test wraps around from top to bottom.
    BeginTest("");
    TestPressKey("i\\ctrl- \\ctrl-p");
    waitForCompletionWidgetToActivate();
    TestPressKey("\\return");
    FinishTest("completion3");

    // Test wraps around from bottom to top.
    BeginTest("");
    TestPressKey("i\\ctrl- \\ctrl-n\\ctrl-n\\ctrl-n");
    waitForCompletionWidgetToActivate();
    TestPressKey("\\return");
    FinishTest("completion1");

    // Test does not re-invoke completion when doing a "." repeat.
    BeginTest("");
    TestPressKey("i\\ctrl- ");
    waitForCompletionWidgetToActivate();
    TestPressKey("\\return\\ctrl-c");
    kate_view->unregisterCompletionModel(testModel);
    FailTestOnInvocationModel *failsTestOnInvocation = new FailTestOnInvocationModel(kate_view);
    TestPressKey("gg.");
    FinishTest("completion1completion1");
    kate_view->unregisterCompletionModel(failsTestOnInvocation);
    kate_view->registerCompletionModel(testModel);

    // Test that the full completion is repeated when repeat an insert that uses completion,
    // where the completion list was not manually invoked.
    BeginTest("");
    TestPressKey("i");
    // Simulate "automatic" invoking of completion.
    kate_view->completionWidget()->userInvokedCompletion();
    waitForCompletionWidgetToActivate();
    TestPressKey("\\return\\ctrl-cgg.");
    FinishTest("completion1completion1");

    clearAllMappings();
    // Make sure the "Enter"/ "Return" used when invoking completions is not swallowed before being
    // passed to the key mapper.
    kate_view->registerCompletionModel(testModel);
    KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::InsertModeMapping, "cb", "mapped-shouldntbehere", KateViGlobal::Recursive);
    BeginTest("");
    TestPressKey("ic");
    kate_view->userInvokedCompletion();
    waitForCompletionWidgetToActivate();
    QVERIFY(kate_view->completionWidget()->isCompletionActive());
    TestPressKey("\\enterb");
    FinishTest("completion1b");
    BeginTest("");
    TestPressKey("ic");
    kate_view->userInvokedCompletion();
    waitForCompletionWidgetToActivate();
    QVERIFY(kate_view->completionWidget()->isCompletionActive());
    TestPressKey("\\returnb");
    FinishTest("completion1b");

    // Make sure the completion widget is dismissed on ESC, ctrl-c and ctrl-[.
    BeginTest("");
    TestPressKey("ic");
    kate_view->userInvokedCompletion();
    waitForCompletionWidgetToActivate();
    QVERIFY(kate_view->completionWidget()->isCompletionActive());
    TestPressKey("\\esc");
    QVERIFY(!kate_view->completionWidget()->isCompletionActive());
    FinishTest("c");
    BeginTest("");
    TestPressKey("ic");
    kate_view->userInvokedCompletion();
    waitForCompletionWidgetToActivate();
    QVERIFY(kate_view->completionWidget()->isCompletionActive());
    TestPressKey("\\ctrl-c");
    QVERIFY(!kate_view->completionWidget()->isCompletionActive());
    FinishTest("c");
    BeginTest("");
    TestPressKey("ic");
    kate_view->userInvokedCompletion();
    waitForCompletionWidgetToActivate();
    QVERIFY(kate_view->completionWidget()->isCompletionActive());
    TestPressKey("\\ctrl-[");
    QVERIFY(!kate_view->completionWidget()->isCompletionActive());
    FinishTest("c");
    kate_view->unregisterCompletionModel(testModel);

    // Check that the repeat-last-change handles Completions in the same way as Macros do
    // i.e. fairly intelligently :)
    FakeCodeCompletionTestModel *fakeCodeCompletionModel = new FakeCodeCompletionTestModel(kate_view);
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    kate_view->registerCompletionModel(fakeCodeCompletionModel);
    clearAllMacros();
    BeginTest("funct\nnoa\ncomtail\ncomtail");
    fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "functionwithargs(...)" << "noargfunction()");
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    // Record 'a'.
    TestPressKey("i\\right\\right\\right\\right\\right\\ctrl- \\enterfirstArg"); // Function with args.
    TestPressKey("\\home\\down\\right\\right\\right\\ctrl- \\enter");            // Function no args.
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    TestPressKey("\\home\\down\\right\\right\\right\\ctrl- \\enter");   // Cut off tail.
    fakeCodeCompletionModel->setRemoveTailOnComplete(false);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    TestPressKey("\\home\\down\\right\\right\\right\\ctrl- \\enter\\ctrl-c");   // Don't cut off tail.
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    // Replay.
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("funct\nnoa\ncomtail\ncomtail");
    TestPressKey("gg.");
    FinishTest("functionwithargs(firstArg)\nnoargfunction()\ncompletionA\ncompletionAtail");

    // Clear our log of completions for each change.
    BeginTest("");
    fakeCodeCompletionModel->setCompletions(QStringList() << "completionA");
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    TestPressKey("ciw\\ctrl- \\enter\\ctrl-c");
    fakeCodeCompletionModel->setCompletions(QStringList() << "completionB");
    TestPressKey("ciw\\ctrl- \\enter\\ctrl-c");
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    TestPressKey(".");
    FinishTest("completionB");

    kate_view->unregisterCompletionModel(fakeCodeCompletionModel);
    delete fakeCodeCompletionModel;
    KateViewConfig::global()->setWordCompletionRemoveTail(oldRemoveTailOnCompletion);

    // Hide the kate_view for subsequent tests.
    kate_view->hide();
    mainWindow->hide();
}

void ViModeTest::visualLineUpDownTests()
{
  // Need to ensure we have dynamic wrap, a fixed width font, and a decent size kate_view.
  ensureKateViewVisible();
  const QSize oldSize = kate_view->size();
  kate_view->resize(400, 400); // Default size is too cramped to have interesting text in.
  const QFont oldFont = kate_view->renderer()->config()->font();
  QFont fixedWidthFont("Monospace");
  fixedWidthFont.setStyleHint(QFont::TypeWriter);
  Q_ASSERT_X(QFontInfo(fixedWidthFont).fixedPitch(), "setting up visual line up down tests", "Need a fixed pitch font!");
  kate_view->renderer()->config()->setFont(fixedWidthFont);
  const bool oldDynWordWrap = KateViewConfig::global()->dynWordWrap();
  KateViewConfig::global()->setDynWordWrap(true);
  const bool oldReplaceTabsDyn = kate_document->config()->replaceTabsDyn();
  kate_document->config()->setReplaceTabsDyn(false);
  const int oldTabWidth = kate_document->config()->tabWidth();
  const int tabWidth = 5;
  kate_document->config()->setTabWidth(tabWidth);
  KateViewConfig::global()->setShowScrollbars(0);

  // Compute the maximum width of text before line-wrapping sets it.
  int textWrappingLength = 1;
  while (true)
  {
    QString text = QString("X").repeated(textWrappingLength) + ' ' + 'O';
    const int posOfO = text.length() - 1;
    kate_document->setText(text);
    if (kate_view->cursorToCoordinate(Cursor(0, posOfO)).y() != kate_view->cursorToCoordinate(Cursor(0, 0)).y())
    {
      textWrappingLength++; // Number of x's, plus space.
      break;
    }
    textWrappingLength++;
  }
  const QString fillsLineAndEndsOnSpace = QString("X").repeated(textWrappingLength - 1) + ' ';

  // Create a QString consisting of enough concatenated fillsLineAndEndsOnSpace to completely
  // fill the viewport of the kate View.
  QString fillsView = fillsLineAndEndsOnSpace;
  while (true)
  {
    kate_document->setText(fillsView);
    const QString visibleText = kate_document->text(kate_view->visibleRange());
    if (fillsView.length() > visibleText.length() * 2) // Overkill.
    {
      break;
    }
    fillsView += fillsLineAndEndsOnSpace;
  }
  const int numVisibleLinesToFillView = fillsView.length() / fillsLineAndEndsOnSpace.length();

  {
    // gk/ gj when there is only one line.
    DoTest("foo", "lgkr.", "f.o");
    DoTest("foo", "lgjr.", "f.o");
  }

  {
    // gk when sticky bit is set to the end.
    const QString originalText = fillsLineAndEndsOnSpace.repeated(2);
    QString expectedText = originalText;
    kate_document->setText(originalText);
    Q_ASSERT(expectedText[textWrappingLength - 1] == ' ');
    expectedText[textWrappingLength - 1] = '.';
    DoTest(originalText, "$gkr.", expectedText);
  }

  {
    // Regression test: more than fill the view up, go to end, and do gk on wrapped text (used to crash).
    // First work out the text that will fill up the view.
    QString expectedText = fillsView;
    Q_ASSERT(expectedText[expectedText.length() - textWrappingLength - 1] == ' ');
    expectedText[expectedText.length() - textWrappingLength - 1] = '.';

    DoTest(fillsView, "$gkr.", expectedText);
  }

  {
    // Jump down a few lines all in one go, where we have some variable length lines to navigate.
    const int numVisualLinesOnLine[] = { 3, 5, 2, 3 };
    const int numLines = sizeof(numVisualLinesOnLine) / sizeof(int);
    const int startVisualLine = 2;
    const int numberLinesToGoDownInOneGo = 10;

    int totalVisualLines = 0;
    for (int i = 0; i < numLines; i++)
    {
      totalVisualLines += numVisualLinesOnLine[i];
    }

    QString startText;
    for (int i = 0; i < numLines; i++)
    {
      QString thisLine = fillsLineAndEndsOnSpace.repeated(numVisualLinesOnLine[i]);
      // Replace trailing space with carriage return.
      thisLine.chop(1);
      thisLine.append('\n');
      startText += thisLine;
    }
    QString expectedText = startText;
    expectedText[((startVisualLine - 1) + numberLinesToGoDownInOneGo) * fillsLineAndEndsOnSpace.length()] = '.';

    Q_ASSERT(numberLinesToGoDownInOneGo + startVisualLine < totalVisualLines);
    Q_ASSERT(numberLinesToGoDownInOneGo + startVisualLine < numVisibleLinesToFillView);
    DoTest(startText, QString("gj").repeated(startVisualLine - 1) + QString::number(numberLinesToGoDownInOneGo) + "gjr.", expectedText);
    // Now go up a few lines.
    const int numLinesToGoBackUp = 7;
    expectedText = startText;
    expectedText[((startVisualLine - 1) + numberLinesToGoDownInOneGo - numLinesToGoBackUp) * fillsLineAndEndsOnSpace.length()] = '.';
    DoTest(startText, QString("gj").repeated(startVisualLine - 1) + QString::number(numberLinesToGoDownInOneGo) + "gj" + QString::number(numLinesToGoBackUp) + "gkr.", expectedText);
  }

  {
    // Move down enough lines in one go to disappear off the view.
    // About half-a-viewport past the end of the current viewport.
    const int numberLinesToGoDown = numVisibleLinesToFillView * 3 / 2;
    const int visualColumnNumber = 7;
    Q_ASSERT(fillsLineAndEndsOnSpace.length() > visualColumnNumber);
    QString expectedText = fillsView.repeated(2);
    Q_ASSERT(expectedText[expectedText.length() - textWrappingLength - 1] == ' ');
    expectedText[visualColumnNumber + fillsLineAndEndsOnSpace.length() * numberLinesToGoDown] = '.';

    DoTest(fillsView.repeated(2), QString("l").repeated(visualColumnNumber) + QString::number(numberLinesToGoDown) + "gjr.", expectedText);

    kDebug(13070) << "Glarb: " << expectedText.indexOf('.') << " glorb: " << kate_document->text().indexOf('.');
    kDebug(13070) << kate_document->text();
  }

  {
    // Deal with dynamic wrapping and indented blocks - continuations of a line are "invisibly" idented by
    // the same amount as the beginning of the line, and we have to subtract this indentation.
    const QString unindentedFirstLine = "stickyhelper\n";
    const int  numIndentationSpaces = 5;
    Q_ASSERT(textWrappingLength >  numIndentationSpaces * 2 /* keep some wriggle room */);
    const QString indentedFillsLineEndsOnSpace = QString(" ").repeated( numIndentationSpaces) + QString("X").repeated(textWrappingLength - 1 - numIndentationSpaces) + ' ';
    DoTest(unindentedFirstLine + indentedFillsLineEndsOnSpace + "LINE3", QString("l").repeated(numIndentationSpaces) + "jgjr.", unindentedFirstLine + indentedFillsLineEndsOnSpace + ".INE3");

    // The first, non-wrapped portion of the line is not invisibly indented, though, so ensure we don't mess that up.
    QString expectedSecondLine = indentedFillsLineEndsOnSpace;
    expectedSecondLine[numIndentationSpaces] = '.';
    DoTest(unindentedFirstLine + indentedFillsLineEndsOnSpace + "LINE3", QString("l").repeated(numIndentationSpaces) + "jgjgkr.", unindentedFirstLine + expectedSecondLine + "LINE3");
  }

  {
    // Take into account any invisible indentation when setting the sticky column.
    const int  numIndentationSpaces = 5;
    Q_ASSERT(textWrappingLength >  numIndentationSpaces * 2 /* keep some wriggle room */);
    const QString indentedFillsLineEndsOnSpace = QString(" ").repeated( numIndentationSpaces) + QString("X").repeated(textWrappingLength - 1 - numIndentationSpaces) + ' ';
    const int posInSecondWrappedLineToChange = 3;
    QString expectedText = indentedFillsLineEndsOnSpace + fillsLineAndEndsOnSpace;
    expectedText[textWrappingLength + posInSecondWrappedLineToChange]= '.';
    DoTest(indentedFillsLineEndsOnSpace + fillsLineAndEndsOnSpace, QString::number(textWrappingLength + posInSecondWrappedLineToChange) + "lgkgjr.", expectedText);
    // Make sure we can do this more than once (i.e. clear any flags that need clearing).
    DoTest(indentedFillsLineEndsOnSpace + fillsLineAndEndsOnSpace, QString::number(textWrappingLength + posInSecondWrappedLineToChange) + "lgkgjr.", expectedText);
  }

  {
    // Take into account any invisible indentation when setting the sticky column as above, but use tabs.
    const QString indentedFillsLineEndsOnSpace = QString("\t") + QString("X").repeated(textWrappingLength - 1 - tabWidth) + ' ';
    const int posInSecondWrappedLineToChange = 3;
    QString expectedText = indentedFillsLineEndsOnSpace + fillsLineAndEndsOnSpace;
    expectedText[textWrappingLength - tabWidth + posInSecondWrappedLineToChange]= '.';
    DoTest(indentedFillsLineEndsOnSpace + fillsLineAndEndsOnSpace, QString("fXf ") + QString::number(posInSecondWrappedLineToChange) + "lgkgjr.", expectedText);
  }

  {
    // Deal with the fact that j/ k may set a sticky column that is impossible to adhere to in visual mode because
    // it is too high.
    // Here, we have one dummy line and one wrapped line.  We start from the beginning of the wrapped line and
    // move right until we wrap and end up at posInWrappedLineToChange one the second line of the wrapped line.
    // We then move up and down with j and k to set the sticky column to a value to large to adhere to in a
    // visual line, and try to move a visual line up.
    const QString dummyLineForUseWithK("dummylineforusewithk\n");
    QString startText = dummyLineForUseWithK + fillsLineAndEndsOnSpace.repeated(2);
    const int posInWrappedLineToChange = 3;
    QString expectedText = startText;
    expectedText[dummyLineForUseWithK.length() + posInWrappedLineToChange]= '.';
    DoTest(startText, "j" + QString::number(textWrappingLength + posInWrappedLineToChange) + "lkjgkr.", expectedText);
  }

  {
    // Ensure gj works in Visual mode.
    Q_ASSERT(fillsLineAndEndsOnSpace.toLower() != fillsLineAndEndsOnSpace);
    QString expectedText = fillsLineAndEndsOnSpace.toLower() + fillsLineAndEndsOnSpace;
    expectedText[textWrappingLength] = expectedText[textWrappingLength].toLower();
    DoTest(fillsLineAndEndsOnSpace.repeated(2), "vgjgu", expectedText);
  }

  {
    // Ensure gk works in Visual mode.
    Q_ASSERT(fillsLineAndEndsOnSpace.toLower() != fillsLineAndEndsOnSpace);
    DoTest(fillsLineAndEndsOnSpace.repeated(2), "$vgkgu", fillsLineAndEndsOnSpace + fillsLineAndEndsOnSpace.toLower());
  }

  {
    // Some tests for how well we handle things with real tabs.
    QString beginsWithTabFillsLineEndsOnSpace = "\t";
    while (beginsWithTabFillsLineEndsOnSpace.length() + (tabWidth - 1) < textWrappingLength - 1)
    {
      beginsWithTabFillsLineEndsOnSpace += 'X';
    }
    beginsWithTabFillsLineEndsOnSpace += ' ';
    const QString unindentedFirstLine = "stockyhelper\n";
    const int posOnThirdLineToChange = 3;
    QString expectedThirdLine = fillsLineAndEndsOnSpace;
    expectedThirdLine[posOnThirdLineToChange] = '.';
    DoTest(unindentedFirstLine + beginsWithTabFillsLineEndsOnSpace + fillsLineAndEndsOnSpace,
           QString("l").repeated(tabWidth + posOnThirdLineToChange) + "gjgjr.",
           unindentedFirstLine + beginsWithTabFillsLineEndsOnSpace + expectedThirdLine);

    // As above, but go down twice and return to the middle line.
    const int posOnSecondLineToChange = 2;
    QString expectedSecondLine = beginsWithTabFillsLineEndsOnSpace;
    expectedSecondLine[posOnSecondLineToChange + 1 /* "+1" as we're not counting the leading tab as a pos */] = '.';
    DoTest(unindentedFirstLine + beginsWithTabFillsLineEndsOnSpace + fillsLineAndEndsOnSpace,
           QString("l").repeated(tabWidth + posOnSecondLineToChange) + "gjgjgkr.",
           unindentedFirstLine + expectedSecondLine + fillsLineAndEndsOnSpace);
  }

  // Restore back to how we were before.
  kate_view->resize(oldSize);
  kate_view->renderer()->config()->setFont(oldFont);
  KateViewConfig::global()->setDynWordWrap(oldDynWordWrap);
  kate_document->config()->setReplaceTabsDyn(oldReplaceTabsDyn);
  kate_document->config()->setTabWidth(oldTabWidth);
}

void ViModeTest::MacroTests()
{
  // Update the status on qa.
  const QString macroIsRecordingStatus = i18n("(recording)");
  clearAllMacros();
  BeginTest("");
  QVERIFY(!kate_view->viewMode().contains(macroIsRecordingStatus));
  TestPressKey("qa");
  QVERIFY(kate_view->viewMode().contains(macroIsRecordingStatus));
  TestPressKey("q");
  QVERIFY(!kate_view->viewMode().contains(macroIsRecordingStatus));
  FinishTest("");

  // The closing "q" is not treated as the beginning of a new "begin recording macro" command.
  clearAllMacros();
  BeginTest("foo");
  TestPressKey("qaqa");
  QVERIFY(!kate_view->viewMode().contains(macroIsRecordingStatus));
  TestPressKey("xyz\\esc");
  FinishTest("fxyzoo");

   // Record and playback a single keypress into macro register "a".
  clearAllMacros();
  DoTest("foo bar", "qawqgg@arX", "foo Xar");
  // Two macros - make sure the old one is cleared.
  clearAllMacros();
  DoTest("123 foo bar xyz", "qawqqabqggww@arX", "123 Xoo bar xyz");

  // Update the status on qb.
  clearAllMacros();
  BeginTest("");
  QVERIFY(!kate_view->viewMode().contains(macroIsRecordingStatus));
  TestPressKey("qb");
  QVERIFY(kate_view->viewMode().contains(macroIsRecordingStatus));
  TestPressKey("q");
  QVERIFY(!kate_view->viewMode().contains(macroIsRecordingStatus));
  FinishTest("");

   // Record and playback a single keypress into macro register "b".
  clearAllMacros();
  DoTest("foo bar", "qbwqgg@brX", "foo Xar");

  // More complex macros.
  clearAllMacros();
  DoTest("foo", "qcrXql@c", "XXo");

  // Re-recording a macro should only clear that macro.
  clearAllMacros();
  DoTest("foo 123", "qaraqqbrbqqbrBqw@a", "Boo a23");

  // Empty macro clears it.
  clearAllMacros();
  DoTest("", "qaixyz\\ctrl-cqqaq@a", "xyz");

  // Hold two macros in memory simultanenously so both can be played.
  clearAllMacros();
  DoTest("foo 123", "qaraqqbrbqw@al@b", "boo ab3");

  // Do more complex things, including switching modes and using ctrl codes.
  clearAllMacros();
  DoTest("foo bar", "qainose\\ctrl-c~qw@a", "nosEfoo nosEbar");
  clearAllMacros();
  DoTest("foo bar", "qayiwinose\\ctrl-r0\\ctrl-c~qw@a", "nosefoOfoo nosebaRbar");
  clearAllMacros();
  DoTest("foo bar", "qavldqw@a", "o r");
  // Make sure we can use "q" in insert mode while recording a macro.
  clearAllMacros();
  DoTest("foo bar", "qaiqueequeg\\ctrl-cqw@a", "queequegfoo queequegbar");
  // Can invoke a macro in Visual Mode.
  clearAllMacros();
  DoTest("foo bar", "qa~qvlll@a", "FOO Bar");
  // Invoking a macro in Visual Mode does not exit Visual Mode.
  clearAllMacros();
  DoTest("foo bar", "qallqggv@a~", "FOO bar");;
  // Can record & macros in Visual Mode for playback in Normal Mode.
  clearAllMacros();
  DoTest("foo bar", "vqblq\\ctrl-c@b~", "foO bar");
  // Recording a macro in Visual Mode does not exit Visual Mode.
  clearAllMacros();
  DoTest("foo bar", "vqblql~", "FOO bar");
  // Recognize correctly numbered registers
  clearAllMacros();
  DoTest("foo", "q1iX\\escq@1", "XXfoo");

  {
    // Ensure that we can call emulated command bar searches, and that we don't record
    // synthetic keypresses.
    VimStyleCommandBarTestsSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(kate_view, mainWindow);
    clearAllMacros();
    DoTest("foo bar\nblank line", "qa/bar\\enterqgg@arX", "foo Xar\nblank line");
    // More complex searching stuff.
    clearAllMacros();
    DoTest("foo 123foo123\nbar 123bar123", "qayiw/\\ctrl-r0\\enterrXqggj@a", "foo 123Xoo123\nbar 123Xar123" );
  }

  // Expand mappings,  but don't do *both* original keypresses and executed keypresses.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "ihello<c-c>", KateViGlobal::Recursive);
  clearAllMacros();
  DoTest("", "qa'q@a", "hellhelloo");
  // Actually, just do the mapped keypresses, not the executed mappings (like Vim).
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "ihello<c-c>", KateViGlobal::Recursive);
  clearAllMacros();
  BeginTest("");
  TestPressKey("qa'q");
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "igoodbye<c-c>", KateViGlobal::Recursive);
  TestPressKey("@a");
  FinishTest("hellgoodbyeo");
  // Clear the "stop recording macro keypresses because we're executing a mapping" when the mapping has finished
  // executing.
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "ihello<c-c>", KateViGlobal::Recursive);
  clearAllMacros();
  DoTest("", "qa'ixyz\\ctrl-cq@a", "hellxyhellxyzozo");
  // ... make sure that *all* mappings have finished, though: take into account recursion.
  clearAllMappings();
  clearAllMacros();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "ihello<c-c>", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "ihello<c-c>", "irecursive<c-c>", KateViGlobal::Recursive);
  DoTest("", "qa'q@a", "recursivrecursivee");
  clearAllMappings();
  clearAllMacros();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "'", "ihello<c-c>ixyz<c-c>", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "ihello<c-c>", "irecursive<c-c>", KateViGlobal::Recursive);
  DoTest("", "qa'q@a", "recursivxyrecursivxyzeze");

  clearAllMappings();
  clearAllMacros();
  // Don't save the trailing "q" with macros, and also test that we can call one macro from another,
  // without one of the macros being repeated.
  DoTest("", "qaixyz\\ctrl-cqqb@aq@b", "xyxyxyzzz");
  clearAllMappings();
  clearAllMacros();
  // More stringent test that macros called from another macro aren't repeated - requires more nesting
  // of macros ('a' calls 'b' calls 'c').
  DoTest("", "qciC\\ctrl-cq"
             "qb@ciB\\ctrl-cq"
             "qa@biA\\ctrl-cq"
             "dd@a", "ABC");
  // Don't crash if we invoke a non-existent macro.
  clearAllMacros();
  DoTest("", "@x", "");
  // Make macros "counted".
  clearAllMacros();
  DoTest("XXXX\nXXXX\nXXXX\nXXXX", "qarOljq3@a", "OXXX\nXOXX\nXXOX\nXXXO");

  // A macro can be undone with one undo.
  clearAllMacros();
  DoTest("foo bar", "qaciwxyz\\ctrl-ci123\\ctrl-cqw@au", "xy123z bar");
  // As can a counted macro.
  clearAllMacros();
  DoTest("XXXX\nXXXX\nXXXX\nXXXX", "qarOljq3@au", "OXXX\nXXXX\nXXXX\nXXXX");

  {
    VimStyleCommandBarTestsSetUpAndTearDown vimStyleCommandBarTestsSetUpAndTearDown(kate_view, mainWindow);
    // Make sure we can macro-ise an interactive sed replace.
    clearAllMacros();
    DoTest("foo foo foo foo\nfoo foo foo foo", "qa:s/foo/bar/gc\\enteryynyAdone\\escqggj@a", "bar bar foo bardone\nbar bar foo bardone");
    // Make sure the closing "q" in the interactive sed replace isn't mistaken for a macro's closing "q".
    clearAllMacros();
    DoTest("foo foo foo foo\nfoo foo foo foo", "qa:s/foo/bar/gc\\enteryyqAdone\\escqggj@a", "bar bar foo foodone\nbar bar foo foodone");
    clearAllMacros();
    DoTest("foo foo foo foo\nfoo foo foo foo", "qa:s/foo/bar/gc\\enteryyqqAdone\\escggj@aAdone\\esc", "bar bar foo foodone\nbar bar foo foodone");
  }

  clearAllMappings();
  clearAllMacros();
  // Expand mapping in an executed macro, if the invocation of the macro "@a" is a prefix of a mapping M, and
  // M ends up not being triggered.
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "@aaaa", "idummy<esc>", KateViGlobal::Recursive);
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, "S", "ixyz<esc>", KateViGlobal::Recursive);
  DoTest("", "qaSq@abrX", "Xyxyzz");
  clearAllMappings();

  // Can't play old version of macro while recording new version.
  clearAllMacros();
  DoTest("", "qaiaaa\\ctrl-cqqa@aq", "aaa");

  // Can't play the macro while recording it.
  clearAllMacros();
  DoTest("", "qaiaaa\\ctrl-c@aq", "aaa");

  // "@@" plays back macro "a" if "a" was the last macro we played back.
  clearAllMacros();
  DoTest("", "qaia\\ctrl-cq@adiw@@", "a");
  // "@@" plays back macro "b" if "b" was the last macro we played back.
  clearAllMacros();
  DoTest("", "qbib\\ctrl-cq@bdiw@@", "b");
  // "@@" does nothing if no macro was previously played.
  clearAllMacros();
  DoTest("", "qaia\\ctrl-cq@@", "a");
  // Nitpick: "@@" replays the last played back macro, even if that macro had not been defined
  // when it was first played back.
  clearAllMacros();
  DoTest("", "@aqaia\\ctrl-cq@@", "aa");
  // "@@" is counted.
  clearAllMacros();
  DoTest("", "qaia\\ctrl-cq@adiw5@@", "aaaaa");

  // Test that we can save and restore a single macro.
  const QString viTestKConfigFileName = "vimodetest-katevimoderc";
  {
    clearAllMacros();
    KConfig viTestKConfig(viTestKConfigFileName);
    KConfigGroup viTestKConfigGroup(&viTestKConfig, "Kate Vi Test Stuff");
    BeginTest("");
    TestPressKey("qaia\\ctrl-cq");
    KateGlobal::self()->viInputModeGlobal()->writeConfig(viTestKConfigGroup);
    viTestKConfig.sync();
    // Overwrite macro "a", and clear the document.
    TestPressKey("qaidummy\\ctrl-cqdd");
    KateGlobal::self()->viInputModeGlobal()->readConfig(viTestKConfigGroup);
    TestPressKey("@a");
    FinishTest("a");
  }

  {
    // Test that we can save and restore several macros.
    clearAllMacros();
    const QString viTestKConfigFileName = "vimodetest-katevimoderc";
    KConfig viTestKConfig(viTestKConfigFileName);
    KConfigGroup viTestKConfigGroup(&viTestKConfig, "Kate Vi Test Stuff");
    BeginTest("");
    TestPressKey("qaia\\ctrl-cqqbib\\ctrl-cq");
    KateGlobal::self()->viInputModeGlobal()->writeConfig(viTestKConfigGroup);
    viTestKConfig.sync();
    // Overwrite macros "a" & "b", and clear the document.
    TestPressKey("qaidummy\\ctrl-cqqbidummy\\ctrl-cqdd");
    KateGlobal::self()->viInputModeGlobal()->readConfig(viTestKConfigGroup);
    TestPressKey("@a@b");
    FinishTest("ba");
  }

  // Ensure that we don't crash when a "repeat change" occurs in a macro we execute.
  clearAllMacros();
  DoTest("", "qqixyz\\ctrl-c.q@qdd", "");
  // Don't record both the "." *and* the last-change keypresses when recording a macro;
  // just record the "."
  clearAllMacros();
  DoTest("", "ixyz\\ctrl-cqq.qddi123\\ctrl-c@q", "121233");

  // Test dealing with auto-completion.
  FakeCodeCompletionTestModel *fakeCodeCompletionModel = new FakeCodeCompletionTestModel(kate_view);
  kate_view->registerCompletionModel(fakeCodeCompletionModel);
  // Completion tests require a visible kate_view.
  ensureKateViewVisible();
  // Want Vim mode to intercept ctrl-p, ctrl-n shortcuts, etc.
  const bool oldStealKeys = KateViewConfig::global()->viInputModeStealKeys();
  KateViewConfig::global()->setViInputModeStealKeys(true);

  // Don't invoke completion via ctrl-space when replaying a macro.
  clearAllMacros();
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  BeginTest("");
  TestPressKey("qqico\\ctrl- \\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("@q");
  FinishTest("ccoo");

  // Don't invoke completion via ctrl-p when replaying a macro.
  clearAllMacros();
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  BeginTest("");
  TestPressKey("qqico\\ctrl-p\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("@q");
  FinishTest("ccoo");

  // Don't invoke completion via ctrl-n when replaying a macro.
  clearAllMacros();
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  BeginTest("");
  TestPressKey("qqico\\ctrl-n\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("@q");
  FinishTest("ccoo");

  // An "enter" in insert mode when no completion is activated (so, a newline)
  // is treated as a newline when replayed as a macro, even if completion is
  // active when the "enter" is replayed.
  clearAllMacros();
  fakeCodeCompletionModel->setCompletions(QStringList()); // Prevent any completions.
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->clearWasInvoked();
  BeginTest("");
  TestPressKey("qqicompl\\enterX\\ctrl-cqdddd");
  QVERIFY(!fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->forceInvocationIfDocTextIs("compl");
  fakeCodeCompletionModel->clearWasInvoked();
  TestPressKey("@q");
  QVERIFY(fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
  fakeCodeCompletionModel->doNotForceInvocation();
  FinishTest("compl\nX");
  // Same for "return".
  clearAllMacros();
  fakeCodeCompletionModel->setCompletions(QStringList()); // Prevent any completions.
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->clearWasInvoked();
  BeginTest("");
  TestPressKey("qqicompl\\returnX\\ctrl-cqdddd");
  QVERIFY(!fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->forceInvocationIfDocTextIs("compl");
  fakeCodeCompletionModel->clearWasInvoked();
  TestPressKey("@q");
  QVERIFY(fakeCodeCompletionModel->wasInvoked()); // Error in test setup!
  fakeCodeCompletionModel->doNotForceInvocation();
  FinishTest("compl\nX");

  // If we do a plain-text completion in a macro, this should be repeated when we replay it.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqicompl\\ctrl- \\enter\\ctrl-cq");
  kate_document->clear();
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("@q");
  FinishTest("completionA");

  // Should replace only the current word when we repeat the completion.
  clearAllMacros();
  BeginTest("compl");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqfla\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(compl)");
  TestPressKey("gg@q");
  FinishTest("(completionA)");

  // Tail-clearing completions should be undoable with one undo.
  clearAllMacros();
  BeginTest("compl");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqfla\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(compl)");
  TestPressKey("gg@qu");
  FinishTest("(compl)");

  // Should be able to store multiple completions.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqicom\\ctrl-p\\enter com\\ctrl-p\\ctrl-p\\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@q");
  FinishTest("completionC completionB");

  // Clear the completions for a macro when we start recording.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionOrig");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqicom\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionSecond");
  TestPressKey("ddqqicom\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@q");
  FinishTest("completionSecond");

  // Completions are per macro.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qaicom\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionB");
  TestPressKey("ddqbicom\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@aA\\enter\\ctrl-c@b");
  FinishTest("completionA\ncompletionB");

  // Make sure completions work with recursive macros.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA1" << "completionA2");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  // Record 'a', which calls the (non-yet-existent) macro 'b'.
  TestPressKey("qaicom\\ctrl- \\enter\\ctrl-cA\\enter\\ctrl-c@bA\\enter\\ctrl-cicom\\ctrl- \\ctrl-p\\enter\\ctrl-cq");
  // Clear document and record 'b'.
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionB");
  TestPressKey("ggdGqbicom\\ctrl- \\enter\\ctrl-cq");
  TestPressKey("dd@a");
  FinishTest("completionA1\ncompletionB\ncompletionA2");

  // Test that non-tail-removing completions are respected.
  // Note that there is no way (in general) to determine if a completion was
  // non-tail-removing, so we explicitly set the config to false.
  const bool oldRemoveTailOnCompletion = KateViewConfig::global()->wordCompletionRemoveTail();
  KateViewConfig::global()->setWordCompletionRemoveTail(false);
  const bool oldReplaceTabsDyn = kate_document->config()->replaceTabsDyn();
  kate_document->config()->setReplaceTabsDyn(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(false);
  clearAllMacros();
  BeginTest("compTail");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "completionB" << "completionC");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("compTail");
  TestPressKey("gg@q");
  FinishTest("completionATail");

  // A "word" consists of letters & numbers, plus "_".
  clearAllMacros();
  BeginTest("(123_compTail");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(123_compTail");
  TestPressKey("gg@q");
  FinishTest("(123_completionATail");

  // Correctly remove word if we are set to remove tail.
  KateViewConfig::global()->setWordCompletionRemoveTail(true);
  clearAllMacros();
  BeginTest("(123_compTail)");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(123_compTail)");
  TestPressKey("gg@q");
  FinishTest("(123_completionA)");

  // Again, a "word" consists of letters & numbers & underscores.
  clearAllMacros();
  BeginTest("(123_compTail_456)");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(123_compTail_456)");
  TestPressKey("gg@q");
  FinishTest("(123_completionA)");

  // Actually, let whether the tail is swallowed or not depend on the value when the
  // completion occurred, not when we replay it.
  clearAllMacros();
  BeginTest("(123_compTail_456)");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  KateViewConfig::global()->setWordCompletionRemoveTail(true);
  TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  KateViewConfig::global()->setWordCompletionRemoveTail(false);
  kate_document->setText("(123_compTail_456)");
  TestPressKey("gg@q");
  FinishTest("(123_completionA)");
  clearAllMacros();
  BeginTest("(123_compTail_456)");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(false);
  KateViewConfig::global()->setWordCompletionRemoveTail(false);
  TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  KateViewConfig::global()->setWordCompletionRemoveTail(true);
  kate_document->setText("(123_compTail_456)");
  TestPressKey("gg@q");
  FinishTest("(123_completionATail_456)");

  // Can have remove-tail *and* non-remove-tail completions in one macro.
  clearAllMacros();
  BeginTest("(123_compTail_456)\n(123_compTail_456)");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  KateViewConfig::global()->setWordCompletionRemoveTail(true);
  TestPressKey("qqfTi\\ctrl- \\enter\\ctrl-c");
  fakeCodeCompletionModel->setRemoveTailOnComplete(false);
  KateViewConfig::global()->setWordCompletionRemoveTail(false);
  TestPressKey("j^fTi\\ctrl- \\enter\\ctrl-cq");
  kate_document->setText("(123_compTail_456)\n(123_compTail_456)");
  TestPressKey("gg@q");
  FinishTest("(123_completionA)\n(123_completionATail_456)");

  // Can repeat plain-text completions when there is no word to the left of the cursor.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqi\\ctrl- \\enter\\ctrl-cq");
  kate_document->clear();
  TestPressKey("gg@q");
  FinishTest("123_completionA");

  // Shouldn't swallow the letter under the cursor if we're not swallowing tails.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(false);
  KateViewConfig::global()->setWordCompletionRemoveTail(false);
  TestPressKey("qqi\\ctrl- \\enter\\ctrl-cq");
  kate_document->setText("oldwordshouldbeuntouched");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("123_completionAoldwordshouldbeuntouched");

  // ... but do if we are swallowing tails.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "123_completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  KateViewConfig::global()->setWordCompletionRemoveTail(true);
  TestPressKey("qqi\\ctrl- \\enter\\ctrl-cq");
  kate_document->setText("oldwordshouldbedeleted");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("123_completionA");

  // Completion of functions.
  // Currently, not removing the tail on function completion is not supported.
  fakeCodeCompletionModel->setRemoveTailOnComplete(true);
  KateViewConfig::global()->setWordCompletionRemoveTail(true);
  // A completed, no argument function "function()" is repeated correctly.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function()");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@q");
  FinishTest("function()");

  // Cursor is placed after the closing bracket when completion a no-arg function.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function()");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enter.something();\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@q");
  FinishTest("function().something();");

  // A function taking some arguments, repeated where there is no opening bracket to
  // merge with, is repeated as "function()").
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enter\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@q");
  FinishTest("function()");

  // A function taking some arguments, repeated where there is no opening bracket to
  // merge with, places the cursor after the opening bracket.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@q");
  FinishTest("function(firstArg)");

  // A function taking some arguments, recorded where there was an opening bracket to merge
  // with but repeated where there is no such bracket, is repeated as "function()" and the
  // cursor placed appropriately.
  BeginTest("(<-Mergeable opening bracket)");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("dd@q");
  FinishTest("function(firstArg)");

  // A function taking some arguments, recorded where there was no opening bracket to merge
  // with but repeated where there is such a bracket, is repeated as "function" and the
  // cursor moved to after the merged opening bracket.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(<-firstArg goes here)");
  TestPressKey("gg@q");
  FinishTest("function(firstArg<-firstArg goes here)");

  // A function taking some arguments, recorded where there was an opening bracket to merge
  // with and repeated where there is also such a bracket, is repeated as "function" and the
  // cursor moved to after the merged opening bracket.
  BeginTest("(<-mergeablebracket)");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(<-firstArg goes here)");
  TestPressKey("gg@q");
  FinishTest("function(firstArg<-firstArg goes here)");

  // The mergeable bracket can be separated by whitespace; the cursor is still placed after the
  // opening bracket.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("   \t (<-firstArg goes here)");
  TestPressKey("gg@q");
  FinishTest("function   \t (firstArg<-firstArg goes here)");

  // Whitespace only, though!
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enterfirstArg\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("|   \t ()");
  TestPressKey("gg@q");
  FinishTest("function(firstArg)|   \t ()");

  // The opening bracket can actually be after the current word (with optional whitespace).
  // Note that this wouldn't be the case if we weren't swallowing tails when completion functions,
  // but this is not currently supported.
  BeginTest("function");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqfta\\ctrl- \\enterfirstArg\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("functxyz    (<-firstArg goes here)");
  TestPressKey("gg@q");
  FinishTest("function    (firstArg<-firstArg goes here)");

  // Regression test for weird issue with replaying completions when the character to the left of the cursor
  // is not a word char.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqciw\\ctrl- \\enter\\ctrl-cq");
  TestPressKey("ddi.xyz\\enter123\\enter456\\ctrl-cggl"); // Position cursor just after the "."
  TestPressKey("@q");
  FinishTest(".completionA\n123\n456");
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionA");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqciw\\ctrl- \\enter\\ctrl-cq");
  TestPressKey("ddi.xyz.abc\\enter123\\enter456\\ctrl-cggl"); // Position cursor just after the "."
  TestPressKey("@q");
  FinishTest(".completionA.abc\n123\n456");


  // Functions taking no arguments are never bracket-merged.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function()");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifunc\\ctrl- \\enter.something();\\ctrl-cq");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  kate_document->setText("(<-don't merge this bracket)");
  TestPressKey("gg@q");
  FinishTest("function().something();(<-don't merge this bracket)");

  // Not-removing-tail when completing functions is not currently supported,
  // so ignore the "do-not-remove-tail" settings when we try this.
  BeginTest("funct");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...)");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  KateViewConfig::global()->setWordCompletionRemoveTail(false);
  TestPressKey("qqfta\\ctrl- \\enterfirstArg\\ctrl-cq");
  kate_document->setText("functxyz");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function(firstArg)");
  BeginTest("funct");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function()");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  KateViewConfig::global()->setWordCompletionRemoveTail(false);
  TestPressKey("qqfta\\ctrl- \\enter\\ctrl-cq");
  kate_document->setText("functxyz");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function()");
  KateViewConfig::global()->setWordCompletionRemoveTail(true);

  // Deal with cases where completion ends with ";".
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function();");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifun\\ctrl- \\enter\\ctrl-cq");
  kate_document->clear();
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function();");
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function();");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
  kate_document->clear();
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function();X");
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...);");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifun\\ctrl- \\enter\\ctrl-cq");
  kate_document->clear();
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function();");
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...);");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
  kate_document->clear();
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function(X);");
  // Tests for completions ending in ";" where bracket merging should happen on replay.
  // NB: bracket merging when recording is impossible with completions that end in ";".
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...);");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifun\\ctrl- \\enter\\ctrl-cq");
  kate_document->setText("(<-mergeable bracket");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function(<-mergeable bracket");
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function(...);");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
  kate_document->setText("(<-mergeable bracket");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function(X<-mergeable bracket");
  // Don't merge no arg functions.
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "function();");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqifun\\ctrl- \\enterX\\ctrl-cq");
  kate_document->setText("(<-mergeable bracket");
  fakeCodeCompletionModel->setFailTestOnInvocation(true);
  TestPressKey("gg@q");
  FinishTest("function();X(<-mergeable bracket");

  {
    const QString viTestKConfigFileName = "vimodetest-katevimoderc";
    KConfig viTestKConfig(viTestKConfigFileName);
    KConfigGroup viTestKConfigGroup(&viTestKConfig, "Kate Vi Test Stuff");
    // Test loading and saving of macro completions.
    clearAllMacros();
    BeginTest("funct\nnoa\ncomtail\ncomtail\ncom");
    fakeCodeCompletionModel->setCompletions(QStringList() << "completionA" << "functionwithargs(...)" << "noargfunction()");
    fakeCodeCompletionModel->setFailTestOnInvocation(false);
    // Record 'a'.
    TestPressKey("qafta\\ctrl- \\enterfirstArg\\ctrl-c"); // Function with args.
    TestPressKey("\\enterea\\ctrl- \\enter\\ctrl-c");     // Function no args.
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    TestPressKey("\\enterfti\\ctrl- \\enter\\ctrl-c");   // Cut off tail.
    fakeCodeCompletionModel->setRemoveTailOnComplete(false);
    KateViewConfig::global()->setWordCompletionRemoveTail(false);
    TestPressKey("\\enterfti\\ctrl- \\enter\\ctrl-cq");   // Don't cut off tail.
    fakeCodeCompletionModel->setRemoveTailOnComplete(true);
    KateViewConfig::global()->setWordCompletionRemoveTail(true);
    // Record 'b'.
    fakeCodeCompletionModel->setCompletions(QStringList() << "completionB" << "semicolonfunctionnoargs();" << "semicolonfunctionwithargs(...);");
    TestPressKey("\\enterqbea\\ctrl- \\enter\\ctrl-cosemicolonfunctionw\\ctrl- \\enterX\\ctrl-cosemicolonfunctionn\\ctrl- \\enterX\\ctrl-cq");
    // Save.
    KateGlobal::self()->viInputModeGlobal()->writeConfig(viTestKConfigGroup);
    viTestKConfig.sync();
    // Overwrite 'a' and 'b' and their completions.
    fakeCodeCompletionModel->setCompletions(QStringList() << "blah1");
    kate_document->setText("");
    TestPressKey("ggqaiblah\\ctrl- \\enter\\ctrl-cq");
    TestPressKey("ddqbiblah\\ctrl- \\enter\\ctrl-cq");
    // Reload.
    KateGlobal::self()->viInputModeGlobal()->readConfig(viTestKConfigGroup);
    // Replay reloaded.
    fakeCodeCompletionModel->setFailTestOnInvocation(true);
    kate_document->setText("funct\nnoa\ncomtail\ncomtail\ncom");
    TestPressKey("gg@a\\enter@b");
    FinishTest("functionwithargs(firstArg)\nnoargfunction()\ncompletionA\ncompletionAtail\ncompletionB\nsemicolonfunctionwithargs(X);\nsemicolonfunctionnoargs();X");
  }

  // Check that undo/redo operations work properly with macros.
  {
    clearAllMacros();
    BeginTest("");
    TestPressKey("ihello\\ctrl-cqauq");
    TestPressKey("@a\\enter");
    FinishTest("");
  }
  {
    clearAllMacros();
    BeginTest("");
    TestPressKey("ihello\\ctrl-cui.bye\\ctrl-cu");
    TestPressKey("qa\\ctrl-r\\enterq");
    TestPressKey("@a\\enter");
    FinishTest(".bye");
  }

  // When replaying a last change in the process of replaying a macro, take the next completion
  // event from the last change completions log, rather than the macro completions log.
  // Ensure that the last change completions log is kept up to date even while we're replaying the macro.
  clearAllMacros();
  BeginTest("");
  fakeCodeCompletionModel->setCompletions(QStringList() << "completionMacro" << "completionRepeatLastChange");
  fakeCodeCompletionModel->setFailTestOnInvocation(false);
  TestPressKey("qqicompletionM\\ctrl- \\enter\\ctrl-c");
  TestPressKey("a completionRep\\ctrl- \\enter\\ctrl-c");
  TestPressKey(".q");
  kDebug(13070) << "text: " << kate_document->text();
  kate_document->clear();
  TestPressKey("gg@q");
  FinishTest("completionMacro completionRepeatLastChange completionRepeatLastChange");

  KateViewConfig::global()->setWordCompletionRemoveTail(oldRemoveTailOnCompletion);
  kate_document->config()->setReplaceTabsDyn(oldReplaceTabsDyn);

  kate_view->unregisterCompletionModel(fakeCodeCompletionModel);
  delete fakeCodeCompletionModel;
  fakeCodeCompletionModel = 0;
  // Hide the kate_view for subsequent tests.
  kate_view->hide();
  mainWindow->hide();
  KateViewConfig::global()->setViInputModeStealKeys(oldStealKeys);
}

// Special area for tests where you want to set breakpoints etc without all the other tests
// triggering them.  Run with ./vimode_test debuggingTests
void ViModeTest::debuggingTests()
{

}

QList< Kate::TextRange* > ViModeTest::rangesOnFirstLine()
{
  return kate_document->buffer().rangesForLine(0, kate_view, true);
}

void ViModeTest::ensureKateViewVisible()
{
    mainWindow->show();
    kate_view->show();
    QApplication::setActiveWindow(mainWindow);
    kate_view->setFocus();
    const QDateTime startTime = QDateTime::currentDateTime();
    while (startTime.msecsTo(QDateTime::currentDateTime()) < 3000 && !mainWindow->isActiveWindow())
    {
      QApplication::processEvents();
    }
    QVERIFY(kate_view->isVisible());
    QVERIFY(mainWindow->isActiveWindow());
}

void ViModeTest::waitForCompletionWidgetToActivate()
{
  waitForCompletionWidgetToActivate(kate_view);
}

void ViModeTest::waitForCompletionWidgetToActivate(KateView* kate_view)
{
  const QDateTime start = QDateTime::currentDateTime();
  while (start.msecsTo(QDateTime::currentDateTime()) < 1000)
  {
    if (kate_view->isCompletionActive())
      break;
    QApplication::processEvents();
  }
  QVERIFY(kate_view->isCompletionActive());
}

KateViEmulatedCommandBar* ViModeTest::emulatedCommandBar()
{
  KateViEmulatedCommandBar *emulatedCommandBar = kate_view->viModeEmulatedCommandBar();
  Q_ASSERT(emulatedCommandBar);
  return emulatedCommandBar;
}

QLabel* ViModeTest::emulatedCommandTypeIndicator()
{
  return emulatedCommandBar()->findChild<QLabel*>("bartypeindicator");
}

QLineEdit* ViModeTest::emulatedCommandBarTextEdit()
{
  QLineEdit *emulatedCommandBarText = emulatedCommandBar()->findChild<QLineEdit*>("commandtext");
  Q_ASSERT(emulatedCommandBarText);
  return emulatedCommandBarText;
}

QLabel* ViModeTest::commandResponseMessageDisplay()
{
  QLabel* commandResponseMessageDisplay = emulatedCommandBar()->findChild<QLabel*>("commandresponsemessage");
  Q_ASSERT(commandResponseMessageDisplay);
  return commandResponseMessageDisplay;
}

void ViModeTest::verifyShowsNumberOfReplacementsAcrossNumberOfLines(int numReplacements, int acrossNumLines)
{
  QVERIFY(commandResponseMessageDisplay()->isVisible());
  QVERIFY(!emulatedCommandTypeIndicator()->isVisible());
  const QString commandMessageResponseText = commandResponseMessageDisplay()->text();
  const QString expectedNumReplacementsAsString = QString::number(numReplacements);
  const QString expectedAcrossNumLinesAsString = QString::number(acrossNumLines);
  // Be a bit vague about the actual contents due to e.g. localisation.
  // TODO - see if we can insist that en_US is available on the Kate Jenkins server and
  // insist that we use it ... ?
  QRegExp numReplacementsMessageRegex("^.*(\\d+).*(\\d+).*$");
  QVERIFY(numReplacementsMessageRegex.exactMatch(commandMessageResponseText));
  const QString actualNumReplacementsAsString = numReplacementsMessageRegex.cap(1);
  const QString actualAcrossNumLinesAsString = numReplacementsMessageRegex.cap(2);
  QCOMPARE(actualNumReplacementsAsString, expectedNumReplacementsAsString);
  QCOMPARE(actualAcrossNumLinesAsString, expectedAcrossNumLinesAsString);
}

void ViModeTest::waitForEmulatedCommandBarToHide(long int timeout)
{
  const QDateTime waitStartedTime = QDateTime::currentDateTime();
  while(emulatedCommandBar()->isVisible() && waitStartedTime.msecsTo(QDateTime::currentDateTime()) < timeout)
  {
    QApplication::processEvents();
  }
  QVERIFY(!emulatedCommandBar()->isVisible());
}


void ViModeTest::verifyCursorAt(const Cursor& expectedCursorPos)
{
  QCOMPARE(kate_view->cursorPosition().line(), expectedCursorPos.line());
  QCOMPARE(kate_view->cursorPosition().column(), expectedCursorPos.column());
}

void ViModeTest::verifyTextEditBackgroundColour(const QColor& expectedBackgroundColour)
{
  QCOMPARE(emulatedCommandBarTextEdit()->palette().brush(QPalette::Base).color(), expectedBackgroundColour);
}

void ViModeTest::clearAllMappings()
{
  KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::NormalModeMapping);
  KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::VisualModeMapping);
  KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::InsertModeMapping);
  KateGlobal::self()->viInputModeGlobal()->clearMappings(KateViGlobal::CommandModeMapping);
}

void ViModeTest::clearSearchHistory()
{
  KateGlobal::self()->viInputModeGlobal()->clearSearchHistory();
}

QStringList ViModeTest::searchHistory()
{
  return KateGlobal::self()->viInputModeGlobal()->searchHistory();
}

void ViModeTest::clearCommandHistory()
{
  KateGlobal::self()->viInputModeGlobal()->clearCommandHistory();
}

QStringList ViModeTest::commandHistory()
{
  return KateGlobal::self()->viInputModeGlobal()->commandHistory();
}

void ViModeTest::clearReplaceHistory()
{
  KateGlobal::self()->viInputModeGlobal()->clearReplaceHistory();
}

QStringList ViModeTest::replaceHistory()
{
  return KateGlobal::self()->viInputModeGlobal()->replaceHistory();
}

void ViModeTest::clearAllMacros()
{
  KateGlobal::self()->viInputModeGlobal()->clearAllMacros();
}

QCompleter* ViModeTest::emulatedCommandBarCompleter()
{
  return kate_view->viModeEmulatedCommandBar()->findChild<QCompleter*>("completer");
}

void ViModeTest::verifyCommandBarCompletionVisible()
{
  if (!emulatedCommandBarCompleter()->popup()->isVisible())
  {
    qDebug() << "Emulated command bar completer not visible.";
    QStringListModel *completionModel = qobject_cast<QStringListModel*>(emulatedCommandBarCompleter()->model());
    Q_ASSERT(completionModel);
    QStringList allAvailableCompletions = completionModel->stringList();
    qDebug() << " Completion list: " << allAvailableCompletions;
    qDebug() << " Completion prefix: " << emulatedCommandBarCompleter()->completionPrefix();
    bool candidateCompletionFound = false;
    foreach (const QString& availableCompletion, allAvailableCompletions)
    {
      if (availableCompletion.startsWith(emulatedCommandBarCompleter()->completionPrefix()))
      {
        candidateCompletionFound = true;
        break;
      }
    }
    if (candidateCompletionFound)
    {
      qDebug() << " The current completion prefix is a prefix of one of the available completions, so either complete() was not called, or the popup was manually hidden since then";
    }
    else
    {
      qDebug() << " The current completion prefix is not a prefix of one of the available completions; this may or may not be why it is not visible";
    }
  }
  QVERIFY(emulatedCommandBarCompleter()->popup()->isVisible());
}

void ViModeTest::verifyCommandBarCompletionsMatches(const QStringList& expectedCompletionList)
{
  verifyCommandBarCompletionVisible();
  QStringList actualCompletionList;
  for (int i = 0; emulatedCommandBarCompleter()->setCurrentRow(i); i++)
    actualCompletionList << emulatedCommandBarCompleter()->currentCompletion();
  if (expectedCompletionList != actualCompletionList)
  {
    qDebug() << "Actual completions:\n " << actualCompletionList << "\n\ndo not match expected:\n" << expectedCompletionList;
  }

  QCOMPARE(actualCompletionList, expectedCompletionList);
}

void ViModeTest::verifyCommandBarCompletionContains(const QStringList& expectedCompletionList)
{
  verifyCommandBarCompletionVisible();
  QStringList actualCompletionList;

  for (int i = 0; emulatedCommandBarCompleter()->setCurrentRow(i); i++)
  {
    actualCompletionList << emulatedCommandBarCompleter()->currentCompletion();
  }

  foreach(const QString& expectedCompletion, expectedCompletionList)
  {
    if (!actualCompletionList.contains(expectedCompletion))
    {
      qDebug() << "Whoops: " << actualCompletionList << " does not contain " << expectedCompletion;
    }
    QVERIFY(actualCompletionList.contains(expectedCompletion));
  }
}

void ViModeTest::clearTrackedDocumentChanges()
{
  m_docChanges.clear();
}

void ViModeTest::textInserted(Document* document, Range range)
{
  m_docChanges.append(DocChange(DocChange::TextInserted, range, document->text(range)));
}

void ViModeTest::textRemoved(Document* document, Range range)
{
  Q_UNUSED(document);
  m_docChanges.append(DocChange(DocChange::TextRemoved, range));
}


void ViModeTest::keyParsingTests()
{

  // BUG #298726
  const QChar char_o_diaeresis(246);

  // Test that we can correctly translate finnish key ö
  QKeyEvent *k = QKeyEvent::createExtendedKeyEvent( QEvent::KeyPress, 214, Qt::NoModifier, 47, 246, 16400, char_o_diaeresis);
  QCOMPARE(KateViKeyParser::self()->KeyEventToQChar(*k), QChar(246));

  // Test that it can be used in mappings
  clearAllMappings();
  KateGlobal::self()->viInputModeGlobal()->addMapping(KateViGlobal::NormalModeMapping, char_o_diaeresis, "ifoo", KateViGlobal::Recursive);
  DoTest("hello", QString("ll%1bar").arg(char_o_diaeresis), "hefoobarllo");

  // Test that <cr> is parsed like <enter>
  QCOMPARE(KateViKeyParser::self()->vi2qt("cr"), int(Qt::Key_Enter));
  const QString &enter = KateViKeyParser::self()->encodeKeySequence(QLatin1String("<cr>"));
  QCOMPARE(KateViKeyParser::self()->decodeKeySequence(enter), QLatin1String("<cr>"));
}

void ViModeTest::AltGr()
{
  QKeyEvent *altGrDown;
  QKeyEvent *altGrUp;

  // Test Alt-gr still works - this isn't quite how things work in "real-life": in real-life, something like
  // Alt-gr+7 would be a "{", but I don't think this can be reproduced without sending raw X11
  // keypresses to Qt, so just duplicate the keypress events we would receive if we pressed
  // Alt-gr+7 (that is: Alt-gr down; "{"; Alt-gr up).
  BeginTest("");
  TestPressKey("i");
  altGrDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_AltGr, Qt::NoModifier);
  QApplication::postEvent(kate_view->focusProxy(), altGrDown);
  QApplication::sendPostedEvents();
  // Not really Alt-gr and 7, but this is the key event that is reported by Qt if we press that.
  QKeyEvent *altGrAnd7 = new QKeyEvent(QEvent::KeyPress, Qt::Key_BraceLeft, Qt::GroupSwitchModifier, "{" );
  QApplication::postEvent(kate_view->focusProxy(), altGrAnd7);
  QApplication::sendPostedEvents();
  altGrUp = new QKeyEvent(QEvent::KeyRelease, Qt::Key_AltGr, Qt::NoModifier);
  QApplication::postEvent(kate_view->focusProxy(), altGrUp);
  QApplication::sendPostedEvents();
  TestPressKey("\\ctrl-c");
  FinishTest("{");

  // French Bepo keyabord AltGr + Shift + s = Ù = Unicode(0x00D9);
  const QString ugrave = QString(QChar(0x00D9));
  BeginTest("");
  TestPressKey("i");
  altGrDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_AltGr, Qt::NoModifier);
  QApplication::postEvent(kate_view->focusProxy(), altGrDown);
  QApplication::sendPostedEvents();
  altGrDown = new QKeyEvent(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier | Qt::GroupSwitchModifier);
  QApplication::postEvent(kate_view->focusProxy(), altGrDown);
  QApplication::sendPostedEvents();
  QKeyEvent *altGrAndUGrave = new QKeyEvent(QEvent::KeyPress, Qt::Key_Ugrave, Qt::ShiftModifier | Qt::GroupSwitchModifier, ugrave);
  qDebug() << QString("%1").arg(altGrAndUGrave->modifiers(), 10, 16);
  QApplication::postEvent(kate_view->focusProxy(), altGrAndUGrave);
  QApplication::sendPostedEvents();
  altGrUp = new QKeyEvent(QEvent::KeyRelease, Qt::Key_AltGr, Qt::NoModifier);
  QApplication::postEvent(kate_view->focusProxy(), altGrUp);
  QApplication::sendPostedEvents();
  FinishTest(ugrave);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
