/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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

#include "scriptdocument_test.h"
#include "moc_scriptdocument_test.cpp"

#include <qtest_kde.h>

#include <ktexteditor/view.h>

#include <katedocument.h>
#include <katescriptdocument.h>

Q_DECLARE_METATYPE(KTextEditor::Cursor)

QTEST_KDEMAIN(ScriptDocumentTest, GUI)

namespace QTest {
  template<>
  char *toString(const KTextEditor::Cursor &cursor)
  {
    QByteArray ba = "Cursor[";
    ba += QByteArray::number(cursor.line()) + ", " + QByteArray::number(cursor.column());
    ba += "]";
    return qstrdup(ba.data());
  }
}

QtMsgHandler ScriptDocumentTest::s_msgHandler = 0;

void myMessageOutput(QtMsgType type, const char *msg)
{
  switch (type) {
  case QtDebugMsg:
    /* do nothing */
    break;
  default:
    ScriptDocumentTest::s_msgHandler(type, msg);
  }
}

void ScriptDocumentTest::initTestCase()
{
  s_msgHandler = qInstallMsgHandler(myMessageOutput);
}

void ScriptDocumentTest::cleanupTestCase()
{
  qInstallMsgHandler(0);
}

ScriptDocumentTest::ScriptDocumentTest()
  : QObject()
  , m_doc(0)
  , m_view(0)
  , m_scriptDoc(0)
{
}

ScriptDocumentTest::~ScriptDocumentTest()
{
}

void ScriptDocumentTest::init()
{
  m_doc = new KateDocument(false, false, false, 0, this);
  m_view = m_doc->createView(0);
  m_scriptDoc = new KateScriptDocument(this);
  m_scriptDoc->setDocument(m_doc);
}

void ScriptDocumentTest::cleanup()
{
  delete m_scriptDoc;
  delete m_view;
  delete m_doc;
}

#if 0
void ScriptDocumentTest::testRfind_data()
{
  QTest::addColumn<KTextEditor::Range>("searchRange");
  QTest::addColumn<KTextEditor::Range>("expectedResult");

  QTest::newRow("") << KTextEditor::Range(0, 0, 1, 10) << KTextEditor::Range(1,  6, 1, 10);
  QTest::newRow("") << KTextEditor::Range(0, 0, 1,  5) << KTextEditor::Range(1,  0, 1,  4);
  QTest::newRow("") << KTextEditor::Range(0, 0, 1,  0) << KTextEditor::Range(0, 10, 0, 14);
}

void ScriptDocumentTest::testRfind()
{
  QFETCH(KTextEditor::Range, searchRange);
  QFETCH(KTextEditor::Range, expectedResult);

  m_doc->setText("aaaa aaaa aaaa\n"
                 "aaaa  aaaa");

  QCOMPARE(m_search->search(searchRange, "aaaa", true), expectedResult);
}
#endif

void ScriptDocumentTest::testRfind_data()
{
  QTest::addColumn<KTextEditor::Cursor>("searchStart");
  QTest::addColumn<KTextEditor::Cursor>("result");

  QTest::newRow("a a a a a a a a a a a a|") << KTextEditor::Cursor(0, 23) << KTextEditor::Cursor(0, 18);
  QTest::newRow("a a a a a a a a a a a |a") << KTextEditor::Cursor(0, 22) << KTextEditor::Cursor(0, 16);
  QTest::newRow("a a a a| a a a a a a a a") << KTextEditor::Cursor(0,  7) << KTextEditor::Cursor(0,  2);
  QTest::newRow("a a a |a a a a a a a a a") << KTextEditor::Cursor(0,  6) << KTextEditor::Cursor(0,  0);
  QTest::newRow("a a a| a a a a a a a a a") << KTextEditor::Cursor(0,  5) << KTextEditor::Cursor(0,  0);
  QTest::newRow("a a |a a a a a a a a a a") << KTextEditor::Cursor(0,  4) << KTextEditor::Cursor::invalid();
}

void ScriptDocumentTest::testRfind()
{
  QFETCH(KTextEditor::Cursor, searchStart);
  QFETCH(KTextEditor::Cursor, result);

  m_scriptDoc->setText("a a a a a a a a a a a a");

  QCOMPARE(m_scriptDoc->rfind(searchStart.line(), searchStart.column(), "a a a"), result);
}
