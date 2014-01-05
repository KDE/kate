/* This file is part of the KDE libraries
 *   Copyright (C) 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public License
 *   along with this library; see the file COPYING.LIB.  If not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#include "bug313759.h"

#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>
#include <kmainwindow.h>

#include <QtScript/QScriptEngine>
#include <QtTestWidgets>

#include "testutils.h"

QTEST_MAIN(BugTest)

using namespace KTextEditor;

BugTest::BugTest()
: QObject()
{
}

BugTest::~BugTest()
{
}

void BugTest::initTestCase()
{
}

void BugTest::cleanupTestCase()
{
}

void BugTest::tryCrash()
{
  // set up document and view
  KMainWindow* toplevel = new KMainWindow();
  KateDocument* doc = new KateDocument(true, false, false, toplevel);
  KateView* view = static_cast<KateView *>(doc->createView(0));
  bool outputWasCustomised = false;
  TestScriptEnv* env = new TestScriptEnv(doc, outputWasCustomised);
  const QUrl url = QUrl::fromLocalFile(QLatin1String(TEST_DATA_DIR"bug313759.txt"));
  doc->openUrl(url);

  // load moveLinesDown and moveLinesUp
  QFile scriptFile(QLatin1String(JS_DATA_DIR "commands/utils.js"));
  QVERIFY(scriptFile.exists());
  QVERIFY(scriptFile.open(QFile::ReadOnly));
  QScriptValue result = env->engine()->evaluate(QString::fromLatin1(scriptFile.readAll()), scriptFile.fileName());
  QVERIFY2(!result.isError(), qPrintable(QString(result.toString() + QLatin1String("\nat ")
  + env->engine()->uncaughtExceptionBacktrace().join(QLatin1String("\n")))) );

  // enable on the fly spell checking
  doc->onTheFlySpellCheckingEnabled(true);

  // view must be visible...
  view->show();
  view->resize(900, 800);
  view->setCursorPosition(Cursor(0, 0));
  doc->editBegin();
  // QTest::qWait(200);

  // evaluate test-script
  qDebug() << "attempting crash by moving lines w/ otf spell checking enabled";
  QFile sourceFile(QLatin1String(TEST_DATA_DIR"bug313759.js"));
  QVERIFY(sourceFile.open(QFile::ReadOnly));
  QTextStream stream(&sourceFile);
  stream.setCodec("UTF8");
  QString code = stream.readAll();
  sourceFile.close();
  // execute script
  result = env->engine()->evaluate(code, QLatin1String(TEST_DATA_DIR"bug313759.txt"), 1);
  QVERIFY2( !result.isError(), result.toString().toUtf8().constData() );

  doc->editEnd();
  qDebug() << "PASS (no crash)";
}
