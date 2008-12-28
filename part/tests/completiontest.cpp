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

#include "tests/completiontest.h"

#include <qtest_kde.h>
#include <ksycoca.h>

#include <ktexteditor/editorchooser.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/document.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

#include <kateview.h>
#include <katecompletionwidget.h>
#include <katecompletionmodel.h>
#include <katerenderer.h>
#include <kateconfig.h>
#include <kateglobal.h>

#include "codecompletiontestmodel.h"
#include <katesmartrange.h>

using namespace KTextEditor;

int countItems(KateCompletionModel *model)
{
    int ret = 0;
    for (int i=0; i < model->rowCount(QModelIndex()); ++i) {
        ret += model->rowCount(model->index(i, 0));
    }
    return ret;
}


void CompletionTest::init()
{
    if ( !KSycoca::isAvailable() )
        QSKIP( "ksycoca not available", SkipAll );

    KateGlobal::self();

    Editor* editor = EditorChooser::editor();
    QVERIFY(editor);
    m_doc = editor->createDocument(this);
    QVERIFY(m_doc);
    m_doc->setText("aa bb cc\ndd");

    KTextEditor::View *v = m_doc->createView(0);
    QApplication::setActiveWindow(v);
    m_view = static_cast<KateView*>(v);
    Q_ASSERT(m_view);
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
    emit m_view->userInvokedCompletion();
    QCOMPARE(countItems(model), 40);

    m_view->insertText("aa");
    QApplication::processEvents();
    QCOMPARE(countItems(model), 14);
}

void CompletionTest::testFilterWithRange()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 2));
    emit m_view->userInvokedCompletion();
    Range complRange = *m_view->completionWidget()->completionRange(testModel);
    QCOMPARE(complRange, Range(Cursor(0, 0), Cursor(0, 2)));
    QCOMPARE(countItems(model), 14);

    m_view->insertText("a");
    QApplication::processEvents();
    QCOMPARE(countItems(model), 1);
}


void CompletionTest::testAbortCursorMovedOutOfRange()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 2));
    emit m_view->userInvokedCompletion();
    QCOMPARE(countItems(model), 14);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->setCursorPosition(Cursor(0, 4));
    QApplication::processEvents();
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}

void CompletionTest::testAbortInvalidText()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 2));
    emit m_view->userInvokedCompletion();
    QCOMPARE(countItems(model), 14);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText(".");
    QApplication::processEvents();
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}

class CustomRangeModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    CustomRangeModel(KTextEditor::View* parent = 0L, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {}
    Range completionRange(View* view, const Cursor &position)
    {
        Range range = CodeCompletionModelControllerInterface::completionRange(view, position);
        if (range.start().column() > 0) {
            KTextEditor::Range preRange(Cursor(range.start().line(), range.start().column()-1),
                                        Cursor(range.start().line(), range.start().column()));
            kDebug() << preRange << view->document()->text(preRange);
            if (view->document()->text(preRange) == "$") {
                range.expandToRange(preRange);
                kDebug() << "using custom completion range" << range;
            }
        }
        return range;
    }

    bool shouldAbortCompletion(View* view, const SmartRange &range, const QString &currentCompletion)
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        static const QRegExp allowedText("^\\$?(\\w*)");
        return !allowedText.exactMatch(currentCompletion);
    }
};

void CompletionTest::testCustomRange1()
{
    m_doc->setText("$aa bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new CustomRangeModel(m_view, "$a");
    m_view->setCursorPosition(Cursor(0, 3));
    emit m_view->userInvokedCompletion();
    Range complRange = *m_view->completionWidget()->completionRange(testModel);
    kDebug() << complRange;
    QCOMPARE(complRange, Range(Cursor(0, 0), Cursor(0, 3)));
    QCOMPARE(countItems(model), 14);

    m_view->insertText("a");
    QApplication::processEvents();
    QCOMPARE(countItems(model), 1);
}

void CompletionTest::testCustomRange2()
{
    m_doc->setText("$ bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new CustomRangeModel(m_view, "$a");
    m_view->setCursorPosition(Cursor(0, 1));
    emit m_view->userInvokedCompletion();
    Range complRange = *m_view->completionWidget()->completionRange(testModel);
    QCOMPARE(complRange, Range(Cursor(0, 0), Cursor(0, 1)));
    QCOMPARE(countItems(model), 40);

    m_view->insertText("aa");
    QApplication::processEvents();
    QCOMPARE(countItems(model), 14);
}

void CompletionTest::testCustomRangeMultipleModels()
{
    m_doc->setText("$a bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel1 = new CustomRangeModel(m_view, "$a");
    CodeCompletionTestModel* testModel2 = new CodeCompletionTestModel(m_view, "a");
    m_view->setCursorPosition(Cursor(0, 1));
    emit m_view->userInvokedCompletion();
    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel1)), Range(Cursor(0, 0), Cursor(0, 2)));
    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel2)), Range(Cursor(0, 1), Cursor(0, 2)));
    QCOMPARE(model->currentCompletion(testModel1), QString("$"));
    QCOMPARE(model->currentCompletion(testModel2), QString(""));
    QCOMPARE(countItems(model), 80);
    

    m_view->insertText("aa");
    QApplication::processEvents();
    QCOMPARE(model->currentCompletion(testModel1), QString("$aa"));
    QCOMPARE(model->currentCompletion(testModel2), QString("aa"));
    QCOMPARE(countItems(model), 14*2);
}

void CompletionTest::testAbortController()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new CustomRangeModel(m_view, "$a");
    m_view->setCursorPosition(Cursor(0, 0));
    emit m_view->userInvokedCompletion();
    QCOMPARE(countItems(model), 40);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText("$a");
    QApplication::processEvents();
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText(".");
    QApplication::processEvents();
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}

class CustomAbortModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    CustomAbortModel(KTextEditor::View* parent = 0L, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {}

    bool shouldAbortCompletion(View* view, const SmartRange &range, const QString &currentCompletion)
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        static const QRegExp allowedText("^([\\w-]*)");
        return !allowedText.exactMatch(currentCompletion);
    }
};

void CompletionTest::testAbortControllerMultipleModels()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel1 = new CodeCompletionTestModel(m_view, "aa");
    CodeCompletionTestModel* testModel2 = new CustomAbortModel(m_view, "a-");
    m_view->setCursorPosition(Cursor(0, 0));
    emit m_view->userInvokedCompletion();
    QCOMPARE(countItems(model), 80);
    QVERIFY(m_view->completionWidget()->isCompletionActive());

    m_view->insertText("a");
    QApplication::processEvents();
    QVERIFY(m_view->completionWidget()->isCompletionActive());
    QCOMPARE(countItems(model), 80);

    m_view->insertText("-");
    QApplication::processEvents();
    QVERIFY(m_view->completionWidget()->isCompletionActive());
    QVERIFY(!m_view->completionWidget()->completionRanges().contains(testModel1));
    QVERIFY(m_view->completionWidget()->completionRanges().contains(testModel2));

    QCOMPARE(countItems(model), 40);

    m_view->insertText(" ");
    QApplication::processEvents();
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}


class EmptyFilterStringModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    EmptyFilterStringModel(KTextEditor::View* parent = 0L, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {}

    QString filterString(View*, const SmartRange&, const Cursor &)
    {
        return QString();
    }
};

void CompletionTest::testEmptyFilterString()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    new EmptyFilterStringModel(m_view, "aa");
    m_view->setCursorPosition(Cursor(0, 0));
    emit m_view->userInvokedCompletion();
    QCOMPARE(countItems(model), 40);

    m_view->insertText("a");
    QApplication::processEvents();
    QCOMPARE(countItems(model), 40);

    m_view->insertText("bam");
    QApplication::processEvents();
    QCOMPARE(countItems(model), 40);
}

class UpdateCompletionRangeModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    UpdateCompletionRangeModel(KTextEditor::View* parent = 0L, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {}

    void updateCompletionRange(View* view, SmartRange& range)
    {
        Q_UNUSED(view);
        if (range.text().first() == QString("ab")) {
            range.setRange(Range(Cursor(range.start().line(), 0), range.end()));
        }
    }
    bool shouldAbortCompletion(View* view, const SmartRange &range, const QString &currentCompletion)
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        Q_UNUSED(currentCompletion);
        return false;
    }
};

void CompletionTest::testUpdateCompletionRange()
{
    m_doc->setText("ab    bb cc\ndd");
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new UpdateCompletionRangeModel(m_view, "ab ab");
    m_view->setCursorPosition(Cursor(0, 3));
    emit m_view->userInvokedCompletion();
    QCOMPARE(countItems(model), 40);
    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel)), Range(Cursor(0, 3), Cursor(0, 3)));

    m_view->insertText("ab");
    QApplication::processEvents();
    QCOMPARE(Range(*m_view->completionWidget()->completionRange(testModel)), Range(Cursor(0, 0), Cursor(0, 5)));
    QCOMPARE(countItems(model), 40);
}

class StartCompletionModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    StartCompletionModel(KTextEditor::View* parent = 0L, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {}

    bool shouldStartCompletion(View* view, const QString &insertedText, bool userInsertion, const Cursor &position)
    {
        Q_UNUSED(view);
        Q_UNUSED(userInsertion);
        Q_UNUSED(position);
        if(insertedText.isEmpty())
            return false;

        QChar lastChar = insertedText.at(insertedText.count() - 1);
        if (lastChar == '%') {
            return true;
        }
        return false;
    }
};

void CompletionTest::testCustomStartCompl()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    m_view->completionWidget()->setAutomaticInvocationDelay(1);

    new StartCompletionModel(m_view, "aa");

    m_view->setCursorPosition(Cursor(0, 0));
    m_view->insertText("%");
    QTest::qWait(100);

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

class ImmideatelyAbortCompletionModel : public CodeCompletionTestModel, public CodeCompletionModelControllerInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
public:
    ImmideatelyAbortCompletionModel(KTextEditor::View* parent = 0L, const QString &startText = QString())
        : CodeCompletionTestModel(parent, startText)
    {}

    virtual bool shouldAbortCompletion(KTextEditor::View* view, const KTextEditor::SmartRange& range, const QString& currentCompletion)
    {
        Q_UNUSED(view);
        Q_UNUSED(range);
        Q_UNUSED(currentCompletion);
        return true;
    }
};

void CompletionTest::testAbortImmideatelyAfterStart()
{
    KateCompletionModel *model = m_view->completionWidget()->model();

    CodeCompletionTestModel* testModel = new ImmideatelyAbortCompletionModel(m_view);
    m_view->setCursorPosition(Cursor(0, 3));
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
    emit m_view->userInvokedCompletion();
    QVERIFY(!m_view->completionWidget()->isCompletionActive());
}

#include "completiontest.moc"
#include "moc_completiontest.cpp"
