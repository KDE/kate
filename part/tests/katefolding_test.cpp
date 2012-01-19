/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2011 Adrian Lungu <adrian.lungu89@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katefolding_test.h"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>
#include <ktexteditor/movingcursor.h>
#include <kateconfig.h>
#include <ktemporaryfile.h>

using namespace KTextEditor;

QTEST_KDEMAIN(KateFoldingTest, GUI)

    namespace QTest {
  template<>
  char *toString(const KTextEditor::Cursor &cursor)
  {
    QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
                    + ", " + QByteArray::number(cursor.column()) + "]";
    return qstrdup(ba.data());
  }
}

void KateFoldingTest::testFolding_data()
{
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("fileExt");
  QTest::addColumn<QString>("firstActionName");
  QTest::addColumn<QString>("secondActionName");
  QTest::addColumn<unsigned int>("initValue");
  QTest::addColumn<unsigned int>("firstResult");
  QTest::addColumn<unsigned int>("secondResult");

  QString text;
  text += "int main() {\n" ;
  text += "  asdf;\n";
  text += "}\n";

  QTest::newRow("simple cpp folding test") << text << "cpp" << "folding_toplevel" << "folding_expandtoplevel" << 4u << 2u << 4u;
  QTest::newRow("reload test") << text << "cpp" << "folding_toplevel" << "file_reload" << 4u << 2u << 2u;

  text = "";
  text += "int main() {\n" ;
  text += "  asdf;\n";
  text += "  {;\n";
  text += "  }\n";

  QTest::newRow("fold unmatched '{' - test 1") << text << "cpp" << "collapse_level_2" << "folding_toplevel" << 5u << 4u << 1u;
  QTest::newRow("fold unmatched '{' - test 2") << text << "cpp" << "folding_toplevel" << "collapse_level_2" << 5u << 1u << 1u;
}

void KateFoldingTest::testFolding()
{
  QFETCH(QString, text);
  QFETCH(QString, fileExt);
  QFETCH(QString, firstActionName);
  QFETCH(QString, secondActionName);

  QFETCH(unsigned int, initValue);
  QFETCH(unsigned int, firstResult);
  QFETCH(unsigned int, secondResult);

  KTemporaryFile file;
  file.setSuffix("." + fileExt);
  file.open();
  QTextStream stream(&file);

  stream << text;
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  QAction* firstAction = view->action(qPrintable(firstActionName));
  QVERIFY(firstAction);

  QAction* secondAction = view->action(qPrintable(secondActionName));
  QVERIFY(secondAction);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), initValue);

  firstAction->trigger();
  QCOMPARE(doc.visibleLines(), firstResult);

  secondAction->trigger();
  QCOMPARE(doc.visibleLines(), secondResult);
}

void KateFoldingTest::testFoldingReload()
{
  KTemporaryFile file;
  file.setSuffix(".cpp");
  file.open();
  QTextStream stream(&file);
  stream << "int main() {\n"
         << "  asdf;\n"
         << "}\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), 4u);

  QAction* action = view->action("folding_toplevel");
  QVERIFY(action);
  action->trigger();

  doc.foldingTree()->saveFoldingState();

  QList<int> hiddenLines(doc.foldingTree()->m_hiddenLines);
  QList<int> hiddenColumns(doc.foldingTree()->m_hiddenColumns);

  QCOMPARE(doc.visibleLines(), 2u);

  action = view->action("file_reload");
  QVERIFY(action);
  action->trigger();

  QCOMPARE(doc.visibleLines(), 2u);

  QCOMPARE(hiddenLines,doc.foldingTree()->m_hiddenLines);
  QCOMPARE(hiddenColumns,doc.foldingTree()->m_hiddenColumns);
}

void KateFoldingTest::testFolding_rb_lang()
{
  KTemporaryFile file;
  file.setSuffix(".rb");
  file.open();
  QTextStream stream(&file);
  stream << "if customerName == x\n"
         << "  print x\n"
         << "elsif customerName == y\n"
         << "  print y\n"
         << "else print z\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), 6u);

  QAction* action = view->action("folding_toplevel");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 1u);

  action = view->action("folding_expandtoplevel");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 6u);
}

void KateFoldingTest::testFolding_py_lang()
{
  KTemporaryFile file;
  file.setSuffix(".py");
  file.open();
  QTextStream stream(&file);
  stream << "if customerName == x\n"
         << "  print x\n"
         << "elif customerName == y\n"
         << "  print y\n"
         << "else print z\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), 6u);

  QAction* action = view->action("folding_toplevel");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 4u);

  action = view->action("folding_expandtoplevel");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 6u);
}

void KateFoldingTest::testFolding_expand_collapse_level234()
{
  KTemporaryFile file;
  file.setSuffix(".c");
  file.open();
  QTextStream stream(&file);
  stream << "if () {\n"
         << "  if () {\n"
         << "     if () {\n"
         << "        if () {\n"
         << "              }\n"
         << "           }\n"
         << "        }\n"
         << "  if () {\n"
         << "     foo()\n"
         << "        }\n"
         << " }\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), 12u);

  QAction* action = view->action("collapse_level_4");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 11u);

  action = view->action("collapse_level_3");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 9u);

  action = view->action("collapse_level_2");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 5u);

  action = view->action("folding_expandall");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 12u);
}

void KateFoldingTest::testFolding_collapse_expand_local()
{
  KTemporaryFile file;
  file.setSuffix(".c");
  file.open();
  QTextStream stream(&file);
  stream << "if () {\n"
         << "  if () {\n"
         << "     if () {\n"
         << "        if () {\n"
         << "              }\n"
         << "           }\n"
         << "        }\n"
         << "  if () {\n"
         << "     foo()\n"
         << "        }\n"
         << " }\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), 12u);

  view->setCursorPosition(KTextEditor::Cursor(2,12));

  QAction* action = view->action("folding_collapselocal");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 9u);

  view->setCursorPosition(KTextEditor::Cursor(2,11));

  action = view->action("folding_collapselocal");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 7u);

  view->setCursorPosition(KTextEditor::Cursor(1,9));

  action = view->action("folding_expandlocal");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 9u);

}

void KateFoldingTest::testFolding_collapse_dsComments_C()
{
  KTemporaryFile file;
  file.setSuffix(".c");
  file.open();
  QTextStream stream(&file);
  stream << "{\n"
         << "}\n"
         << "/*\n"
         << "{\n"
         << "}\n"
         << "*/\n"
         << "/*\n"
         << "*/\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), 9u);

  QAction* action = view->action("folding_collapse_dsComment");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 5u);
}

void KateFoldingTest::testFolding_collapse_dsComments_XML()
{
  KTemporaryFile file;
  file.setSuffix(".xml");
  file.open();
  QTextStream stream(&file);
  stream << "<test1>\n"
         << "</test1>\n"
         << "<!--\n"
         << "<test2>\n"
         << "</test2>\n"
         << "-->\n"
         << "<!--\n"
         << "-->\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  KateView* view = new KateView(&doc, 0);

  // is set to allow kate's hl to be called
  view->config()->setDynWordWrap(true);

  QCOMPARE(doc.visibleLines(), 9u);

  QAction* action = view->action("folding_collapse_dsComment");
  QVERIFY(action);
  action->trigger();
  QCOMPARE(doc.visibleLines(), 5u);
}

void KateFoldingTest::testFindNodeForPosition()
{
  // see also: https://bugs.kde.org/show_bug.cgi?id=289469

  KTemporaryFile file;
  file.setSuffix(".cpp");
  file.open();
  QTextStream stream(&file);
  stream << "int f() {\n"
         << "  int i;\n"
         << "}\n";
  stream << flush;
  file.close();

  KateDocument doc(false, false, false);
  QVERIFY(doc.openUrl(KUrl(file.fileName())));

  QVERIFY(!doc.foldingTree()->findNodeForPosition(1, 0));
  QVERIFY(doc.foldingTree()->findNodeForLine(1));
  QVERIFY(doc.foldingTree()->findNodeForLine(0));
}
