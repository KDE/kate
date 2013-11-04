/* This file is part of the KDE libraries
   Copyright (C) 2008 Niko Sams <niko.sams\gmail.com>

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

#include "completion_test.h"

#include "codecompletiontestmodels.h"
#include "codecompletiontestmodels.moc"

#include <qtest_kde.h>
#include <ksycoca.h>

#include <ktexteditor/document.h>
#include <ktexteditor/editorchooser.h>

#include <kateview.h>
#include <katecompletionwidget.h>
#include <katecompletionmodel.h>
#include <katecompletiontree.h>
#include <katerenderer.h>
#include <kateconfig.h>

QTEST_KDEMAIN(CompletionTest, GUI)


using namespace KTextEditor;

int countItems(KateCompletionModel *model)
{
    int ret = 0;
    for (int i=0; i < model->rowCount(QModelIndex()); ++i) {
        ret += model->rowCount(model->index(i, 0));
    }
    return ret;
}

static void verifyCompletionStarted(KateView* view)
{
  const QDateTime startTime = QDateTime::currentDateTime();
  while (startTime.msecsTo(QDateTime::currentDateTime()) < 1000)
  {
    QApplication::processEvents();
    if (view->completionWidget()->isCompletionActive())
    {
      break;
    }
  }
  QVERIFY(view->completionWidget()->isCompletionActive());
}

static void verifyCompletionAborted(KateView* view)
{
  const QDateTime startTime = QDateTime::currentDateTime();
  while (startTime.msecsTo(QDateTime::currentDateTime()) < 1000)
  {
    QApplication::processEvents();
    if (!view->completionWidget()->isCompletionActive())
    {
      break;
    }
  }
  QVERIFY(!view->completionWidget()->isCompletionActive());
}

static void invokeCompletionBox(KateView* view)
{
  view->userInvokedCompletion();
  verifyCompletionStarted(view);
}

void CompletionTest::init()
{
    if ( !KSycoca::isAvailable() )
        QSKIP( "ksycoca not available", SkipAll );

    Editor* editor = EditorChooser::editor();
    QVERIFY(editor);

    m_doc = editor->createDocument(this);
    QVERIFY(m_doc);
    m_doc->setText("aa bb cc\ndd");

    KTextEditor::View *v = m_doc->createView(0);
    QApplication::setActiveWindow(v);
    m_view = static_cast<KateView*>(v);
    Q_ASSERT(m_view);

    //view needs to be shown as completion won't work if the cursor is off screen
    m_view->show();
}

void CompletionTest::cleanup()
{
    delete m_view;
    delete m_doc;
}

void CompletionTest::testFilterEmptyRange()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 0));
    invokeCompletionBox(m_view);

    QCOMPARE(countItems(model), 40);
    m_view->insertText("aa");
    QTest::qWait(1000); // process events
    QCOMPARE(countItems(model), 14);
}

void CompletionTest::testFilterWithRange()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 2));
    invokeCompletionBox(m_view);

    Range complRange = *m_view->completionWidget()->completionRange(testModel);
    QCOMPARE(complRange, Range(Cursor(0, 0), Cursor(0, 2)));
    QCOMPARE(countItems(model), 14);

    m_view->insertText("a");
    QTest::qWait(1000); // process events
    QCOMPARE(countItems(model), 1);
}


void CompletionTest::testAbortCursorMovedOutOfRange()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 2));
    invokeCompletionBox(m_view);

    QCOMPARE(countItems(model), 14);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->setCursorPosition(Cursor(0, 4));
    QTest::qWait(1000); // process events
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}

void CompletionTest::testAbortInvalidText()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 2));
    invokeCompletionBox(m_view);

    QCOMPARE(countItems(model), 14);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText(".");
    verifyCompletionAborted(m_view);
}

void CompletionTest::testCustomRange1()
{
    m_doc->setText("$aa bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new CustomRangeModel(m_view, "$a");
    m_view->setCursorPosition(Cursor(0, 3));
    invokeCompletionBox(m_view);

    Range complRange = *m_view->completionWidget()->completionRange(testModel);
    kDebug() << complRange;
    QCOMPARE(complRange, Range(Cursor(0, 0), Cursor(0, 3)));
    QCOMPARE(countItems(model), 14);

    m_view->insertText("a");
    QTest::qWait(1000); // process events
    QCOMPARE(countItems(model), 1);
}

void CompletionTest::testCustomRange2()
{
    m_doc->setText("$ bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new CustomRangeModel(m_view, "$a");
    m_view->setCursorPosition(Cursor(0, 1));
    invokeCompletionBox(m_view);

    Range complRange = *m_view->completionWidget()->completionRange(testModel);
    QCOMPARE(complRange, Range(Cursor(0, 0), Cursor(0, 1)));
    QCOMPARE(countItems(model), 40);

    m_view->insertText("aa");
    QTest::qWait(1000); // process events
    QCOMPARE(countItems(model), 14);
}

void CompletionTest::testCustomRangeMultipleModels()
{
    m_doc->setText("$a bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel1 = new CustomRangeModel(m_view, "$a");
    CodeCompletionTestModel* testModel2 = new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 1));
    invokeCompletionBox(m_view);

    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel1)), Range(Cursor(0, 0), Cursor(0, 2)));
    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel2)), Range(Cursor(0, 1), Cursor(0, 2)));
    QCOMPARE(model->currentCompletion(testModel1), QString("$"));
    QCOMPARE(model->currentCompletion(testModel2), QString(""));
    QCOMPARE(countItems(model), 80);


    m_view->insertText("aa");
    QTest::qWait(1000); // process events
    QCOMPARE(model->currentCompletion(testModel1), QString("$aa"));
    QCOMPARE(model->currentCompletion(testModel2), QString("aa"));
    QCOMPARE(countItems(model), 14*2);
}

void CompletionTest::testAbortController()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new CustomRangeModel(m_view, "$a");
    m_view->setCursorPosition(Cursor(0, 0));
    invokeCompletionBox(m_view);

    QCOMPARE(countItems(model), 40);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText("$a");
    QTest::qWait(1000); // process events
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText(".");
    verifyCompletionAborted(m_view);
}

void CompletionTest::testAbortControllerMultipleModels()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel1 = new CodeCompletionTestModel(m_view, "aa");
    CodeCompletionTestModel* testModel2 = new CustomAbortModel(m_view, "a-");
    m_view->setCursorPosition(Cursor(0, 0));
    invokeCompletionBox(m_view);

    QCOMPARE(countItems(model), 80);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText("a");
    QTest::qWait(1000); // process events
    QVERIFY(m_view->completionWidget()->isCompletionActive());
    QCOMPARE(countItems(model), 80);

    m_view->insertText("-");
    QTest::qWait(1000); // process events
    QVERIFY(m_view->completionWidget()->isCompletionActive());
    QVERIFY(!m_view->completionWidget()->completionRanges().contains(testModel1));
    QVERIFY(m_view->completionWidget()->completionRanges().contains(testModel2));

    QCOMPARE(countItems(model), 40);

    m_view->insertText(" ");
    QTest::qWait(1000); // process events
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}

void CompletionTest::testEmptyFilterString()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new EmptyFilterStringModel(m_view, "aa");
    m_view->setCursorPosition(Cursor(0, 0));
    invokeCompletionBox(m_view);

    QCOMPARE(countItems(model), 40);

    m_view->insertText("a");
    QTest::qWait(1000); // process events
    QCOMPARE(countItems(model), 40);

    m_view->insertText("bam");
    QTest::qWait(1000); // process events
    QCOMPARE(countItems(model), 40);
}

void CompletionTest::testUpdateCompletionRange()
{
    m_doc->setText("ab    bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new UpdateCompletionRangeModel(m_view, "ab ab");
    m_view->setCursorPosition(Cursor(0, 3));
    invokeCompletionBox(m_view);

    QCOMPARE(countItems(model), 40);
    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel)), Range(Cursor(0, 3), Cursor(0, 3)));

    m_view->insertText("ab");
    QTest::qWait(1000); // process events
    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel)), Range(Cursor(0, 0), Cursor(0, 5)));
    QCOMPARE(countItems(model), 40);
}

void CompletionTest::testCustomStartCompl()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    m_view->completionWidget()->setAutomaticInvocationDelay(1);

    new StartCompletionModel(m_view, "aa");

    m_view->setCursorPosition(Cursor(0, 0));
    m_view->insertText("%");
    QTest::qWait(1000);

    QVERIFY(m_view->completionWidget()->isCompletionActive());
    QCOMPARE(countItems(model), 40);
}

void CompletionTest::testKateCompletionModel()
{
    KateCompletionModel *model = m_view->completionWidget()->model();
    CodeCompletionTestModel* testModel1 = new CodeCompletionTestModel(m_view, "aa");
    CodeCompletionTestModel* testModel2 = new CodeCompletionTestModel(m_view, "bb");

    model->setCompletionModel(testModel1);
    QCOMPARE(countItems(model), 40);

    model->addCompletionModel(testModel2);
    QCOMPARE(countItems(model), 80);

    model->removeCompletionModel(testModel2);
    QCOMPARE(countItems(model), 40);
}

void CompletionTest::testAbortImmideatelyAfterStart()
{
    new ImmideatelyAbortCompletionModel(m_view);
    m_view->setCursorPosition(Cursor(0, 3));
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
    emit m_view->userInvokedCompletion();
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}

void CompletionTest::testJumpToListBottomAfterCursorUpWhileAtTop()
{
    new CodeCompletionTestModel(m_view, "aa");
    invokeCompletionBox(m_view);

    m_view->completionWidget()->cursorUp();
    m_view->completionWidget()->bottom();
    // TODO - better way of finding the index?
    QCOMPARE(m_view->completionWidget()->treeView()->selectionModel()->currentIndex().row(), 39);
}

void CompletionTest::testAbbreviationEngine()
{
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBar", "fb"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBar", "foob"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBar", "fbar"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBar", "fba"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBar", "foba"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBarBazBang", "fbbb"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("foo_bar_cat", "fbc"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("foo_bar_cat", "fb"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBarArr", "fba"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBarArr", "fbara"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBarArr", "fobaar"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("FooBarArr", "fb"));

  QVERIFY(KateCompletionModel::matchesAbbreviation("QualifiedIdentifier", "qid"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("QualifiedIdentifier", "qualid"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("QualifiedIdentifier", "qualidentifier"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("QualifiedIdentifier", "qi"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("KateCompletionModel", "kcmodel"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("KateCompletionModel", "kc"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("KateCompletionModel", "kcomplmodel"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("KateCompletionModel", "kacomplmodel"));
  QVERIFY(KateCompletionModel::matchesAbbreviation("KateCompletionModel", "kacom"));

  QVERIFY(! KateCompletionModel::matchesAbbreviation("QualifiedIdentifier", "identifier"));
  QVERIFY(! KateCompletionModel::matchesAbbreviation("FooBarArr", "fobaara"));
  QVERIFY(! KateCompletionModel::matchesAbbreviation("FooBarArr", "fbac"));
  QVERIFY(! KateCompletionModel::matchesAbbreviation("KateCompletionModel", "kamodel"));

  QVERIFY(KateCompletionModel::matchesAbbreviation("AbcdefBcdefCdefDefEfFzZ", "AbcdefBcdefCdefDefEfFzZ"));
  QVERIFY(! KateCompletionModel::matchesAbbreviation("AbcdefBcdefCdefDefEfFzZ", "ABCDEFX"));
  QVERIFY(! KateCompletionModel::matchesAbbreviation("AaaaaaBbbbbCcccDddEeFzZ", "XZYBFA"));
}

void CompletionTest::benchAbbreviationEngineGoodCase()
{
  QBENCHMARK {
    for ( int i = 0; i < 10000; i++ ) {
      QVERIFY(! KateCompletionModel::matchesAbbreviation("AaaaaaBbbbbCcccDddEeFzZ", "XZYBFA"));
    }
  }
}

void CompletionTest::benchAbbreviationEngineNormalCase()
{
  QBENCHMARK {
    for ( int i = 0; i < 10000; i++ ) {
      QVERIFY(! KateCompletionModel::matchesAbbreviation("AaaaaaBbbbbCcccDddEeFzZ", "ABCDEFX"));
    }
  }
}

void CompletionTest::benchAbbreviationEngineWorstCase()
{
  QBENCHMARK {
    for ( int i = 0; i < 10000; i++ ) {
      // This case is quite horrible, because it requires a branch at every letter.
      // The current code will at some point drop out and just return false.
      KateCompletionModel::matchesAbbreviation("XxBbbbbbBbbbbbBbbbbBbbbBbbbbbbBbbbbbBbbbbbBbbbFox", "XbbbbbbbbbbbbbbbbbbbbFx");
    }
  }
}

void CompletionTest::testAbbrevAndContainsMatching()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new AbbreviationCodeCompletionTestModel(m_view, QString());

    m_view->document()->setText("SCA");
    invokeCompletionBox(m_view);
    QCOMPARE(model->filteredItemCount(), (uint) 6);

    m_view->document()->setText("SC");
    invokeCompletionBox(m_view);
    QCOMPARE(model->filteredItemCount(), (uint) 6);

    m_view->document()->setText("sca");
    invokeCompletionBox(m_view);
    QCOMPARE(model->filteredItemCount(), (uint) 6);

    m_view->document()->setText("contains");
    invokeCompletionBox(m_view);
    QCOMPARE(model->filteredItemCount(), (uint) 2);

    m_view->document()->setText("CONTAINS");
    invokeCompletionBox(m_view);
    QCOMPARE(model->filteredItemCount(), (uint) 2);

    m_view->document()->setText("containssome");
    invokeCompletionBox(m_view);
    QCOMPARE(model->filteredItemCount(), (uint) 1);

    m_view->document()->setText("matched");
    m_view->userInvokedCompletion();
    QApplication::processEvents();
    QCOMPARE(model->filteredItemCount(), (uint) 0);
}

void CompletionTest::benchCompletionModel()
{
  const QString text("abcdefg abcdef");
  m_doc->setText(text);
  CodeCompletionTestModel* testModel1 = new CodeCompletionTestModel(m_view, "abcdefg");
  testModel1->setRowCount(500);
  CodeCompletionTestModel* testModel2 = new CodeCompletionTestModel(m_view, "abcdef");
  testModel2->setRowCount(500);
  CodeCompletionTestModel* testModel3 = new CodeCompletionTestModel(m_view, "abcde");
  testModel3->setRowCount(500);
  CodeCompletionTestModel* testModel4 = new CodeCompletionTestModel(m_view, "abcd");
  testModel4->setRowCount(5000);
  QBENCHMARK_ONCE {
    for(int i = 0; i < text.size(); ++i) {
      m_view->setCursorPosition(Cursor(0, i));
      invokeCompletionBox(m_view);
    }
  }
}

#include "completion_test.moc"
