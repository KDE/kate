/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 * Copyright (C) 2003,2004 Stephan Kulow (coolo@kde.org)
 * Copyright (C) 2004 Dirk Mueller ( mueller@kde.org )
 * Copyright 2006, 2007 Leo Savernik (l.savernik@aon.at)
 * Copyright (C) 2010 Milian Wolff <mail@milianw.de>
 * Copyright (C) 2013 Gerald Senarclens de Grancy <oss@senarclens.eu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

//BEGIN Includes

#include "kateview.h"
#include "katedocument.h"
#include "kateglobal.h"

#include <QProcess>
#include <QDirIterator>
#include <QMainWindow>
#include <QScriptEngine>
#include <QTest>

#include "testutils.h"

#include "script_test_base.h"


const QString testDataPath(TEST_DATA_DIR);


QtMsgHandler ScriptTestBase::m_msgHandler = 0;
void noDebugMessageOutput(QtMsgType type, const char *msg)
{
  switch (type) {
  case QtDebugMsg:
    break;
  default:
    ScriptTestBase::m_msgHandler(type, msg);
  }
}


void ScriptTestBase::initTestCase()
{
  m_msgHandler = qInstallMsgHandler(noDebugMessageOutput);
  m_toplevel = new QMainWindow();
  m_document = new KateDocument(true, false, false, m_toplevel);
  m_view = static_cast<KateView *>(m_document->widget());
  m_env = new TestScriptEnv(m_document, m_outputWasCustomised);
}

void ScriptTestBase::cleanupTestCase()
{
  qInstallMsgHandler(m_msgHandler);
}

void ScriptTestBase::getTestData(const QString& script)
{
  QTest::addColumn<QString>("testcase");

  // make sure the script files are valid
  if (!m_script_dir.isEmpty()) {
    QFile scriptFile(JS_DATA_DIR + m_script_dir + '/' + script + ".js");
    if (!scriptFile.exists()) {
      QSKIP(qPrintable(QString(scriptFile.fileName() + " does not exist")), SkipAll);
    }
    QVERIFY(scriptFile.open(QFile::ReadOnly));
    QScriptValue result = m_env->engine()->evaluate(scriptFile.readAll(), scriptFile.fileName());
    QVERIFY2(!result.isError(), qPrintable(QString(result.toString() + "\nat "
                                            + m_env->engine()->uncaughtExceptionBacktrace().join("\n"))) );
  }

  const QString testDir(testDataPath + m_section + '/' + script + '/');
  if ( !QFile::exists(testDir) ) {
    QSKIP(qPrintable(QString(testDir + " does not exist")), SkipAll);
  }
  QDirIterator contents( testDir );
  while ( contents.hasNext() ) {
    QString entry = contents.next();
    if ( entry.endsWith('.') ) {
      continue;
    }
    QFileInfo info(entry);
    if ( !info.isDir() ) {
      continue;
    }
    QTest::newRow( info.baseName().toLocal8Bit() ) << info.absoluteFilePath();
  }
}

void ScriptTestBase::runTest(const ExpectedFailures& failures)
{
  if ( !QFile::exists(testDataPath + m_section) )
    QSKIP(qPrintable(QString(testDataPath + m_section + " does not exist")), SkipAll);

  QFETCH(QString, testcase);

  m_toplevel->resize( 800, 600); // restore size

  // load page
  QUrl url;
  url.setScheme("file");
  url.setPath(testcase + "/origin");
  m_document->openUrl(url);

  // evaluate test-script
  QFile sourceFile(testcase + "/input.js");
  if ( !sourceFile.open(QFile::ReadOnly) ) {
    QFAIL(qPrintable(QString("Failed to open file: %1").arg(sourceFile.fileName())));
  }

  QTextStream stream(&sourceFile);
  stream.setCodec("UTF8");
  QString code = stream.readAll();
  sourceFile.close();

  // Execute script
  QScriptValue result = m_env->engine()->evaluate(code, testcase + "/input.js", 1);
  QVERIFY2( !result.isError(), result.toString().toUtf8().constData() );

  url.setPath(testcase + "/actual");
  m_document->saveAs(url);

  // diff actual and expected
  QProcess diff;
  QStringList args;
  args << "-u" << (testcase + "/expected") << (testcase + "/actual");
  diff.start("diff", args);
  diff.waitForFinished();
  QByteArray out = diff.readAllStandardOutput();
  QByteArray err = diff.readAllStandardError();
  if ( !err.isEmpty() ) {
    qWarning() << err;
  }
  foreach( const Failure& failure, failures ) {
    QEXPECT_FAIL(failure.first, failure.second, Abort);
  }
  QCOMPARE(QString::fromLocal8Bit(out), QString());
  QCOMPARE(diff.exitCode(), EXIT_SUCCESS);

  m_document->closeUrl();
}
