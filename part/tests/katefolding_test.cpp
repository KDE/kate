/* This file is part of the KDE libraries
   Copyright (C) 2011 Adrian Lungu <adrian.lungu89@gmail.com>

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


KateFoldingTest::KateFoldingTest()
  : QObject()
{
  qDebug() << "~~~~~~~" ;
}

KateFoldingTest::~KateFoldingTest()
{
}

void KateFoldingTest::testFolding_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("fileExt");
    QTest::addColumn<QString>("name_firstAction");
    QTest::addColumn<QString>("name_secondAction");
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

    text = "";
    text += "def foo1\n";
    text += "  if x\n";
    text += "    f1()\n";
    text += "  else\n";
    text += "    f2()\n\n";
    text += "def foo2\n";
    text += "  f2()\n";

    QTest::newRow("simple py folding test") << text << "py" << "folding_toplevel" << "folding_expandtoplevel" << 9u << 3u << 9u;
}

void KateFoldingTest::testFolding()
{
    QFETCH(QString, text);
    QFETCH(QString, fileExt);
    QFETCH(QString, name_firstAction);
    QFETCH(QString, name_secondAction);

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

    char action_name [50];
    strcpy(action_name, name_firstAction.toAscii());
    QAction* firstAction = view->action(action_name);
    QVERIFY(firstAction);

    strcpy(action_name, name_secondAction.toAscii());
    QAction* secondAction = view->action(action_name);
    QVERIFY(secondAction);

    // is set to allow kate's hl to be called
    view->config()->setDynWordWrap(true);

    QCOMPARE(doc.visibleLines(), initValue);

    firstAction->trigger();
    QCOMPARE(doc.visibleLines(), firstResult);

    secondAction->trigger();
    QCOMPARE(doc.visibleLines(), secondResult);
}
