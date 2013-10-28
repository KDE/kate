/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  Sven Brauch <svenbrauch@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "wordcompletiontest.h"

#include <katewordcompletion.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <KTextEditor/EditorChooser>

#include <qtest_kde.h>

QTEST_KDEMAIN(WordCompletionTest, GUI)

static const int count = 500000;

using namespace KTextEditor;

void WordCompletionTest::initTestCase()
{
  Editor* editor = EditorChooser::editor();
  QVERIFY(editor);

  m_doc = editor->createDocument(this);
  QVERIFY(m_doc);
}

void WordCompletionTest::cleanupTestCase()
{
}

void WordCompletionTest::init()
{
  m_doc->clear();
}

void WordCompletionTest::cleanup()
{
}

void WordCompletionTest::benchWordRetrievalMixed()
{
  const int distinctWordRatio = 100;
  QStringList s;
  s.reserve(count);
  for ( int i = 0; i < count; i++ ) {
    s.append(QLatin1String("HelloWorld") + QString::number(i / distinctWordRatio));
  }
  s.prepend("\n");
  m_doc->setText(s);

  // creating the view only after inserting the text makes test execution much faster
  QSharedPointer<KTextEditor::View> v(m_doc->createView(0));
  QBENCHMARK {
    KateWordCompletionModel m(0);
    QCOMPARE(m.allMatches(v.data(), KTextEditor::Range()).size(), count / distinctWordRatio);
  }
}

void WordCompletionTest::benchWordRetrievalSame()
{
  QStringList s;
  s.reserve(count);
  // add a number so the words have roughly the same length as in the other tests
  const QString str = QLatin1String("HelloWorld") + QString::number(count);
  for ( int i = 0; i < count; i++ ) {
    s.append(str);
  }
  s.prepend("\n");
  m_doc->setText(s);

  QSharedPointer<KTextEditor::View> v(m_doc->createView(0));
  QBENCHMARK {
    KateWordCompletionModel m(0);
    QCOMPARE(m.allMatches(v.data(), KTextEditor::Range()).size(), 1);
  }
}

void WordCompletionTest::benchWordRetrievalDistinct()
{
  QStringList s;
  s.reserve(count);
  for ( int i = 0; i < count; i++ ) {
    s.append(QLatin1String("HelloWorld") + QString::number(i));
  }
  s.prepend("\n");
  m_doc->setText(s);

  QSharedPointer<KTextEditor::View> v(m_doc->createView(0));
  QBENCHMARK {
    KateWordCompletionModel m(0);
    QCOMPARE(m.allMatches(v.data(), KTextEditor::Range()).size(), count);
  }
}

#include "wordcompletiontest.moc"

// kate: indent-width 2
