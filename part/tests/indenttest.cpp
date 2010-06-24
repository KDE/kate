/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 * Copyright (C) 2003,2004 Stephan Kulow (coolo@kde.org)
 * Copyright (C) 2004 Dirk Mueller ( mueller@kde.org )
 * Copyright 2006, 2007 Leo Savernik (l.savernik@aon.at)
 * Copyright (C) 2010 Milian Wolff <mail@milianw.de>
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
#include "indenttest.h"

#include "kateview.h"
#include "katedocument.h"
#include "katedocumenthelpers.h"
#include "kateconfig.h"
#include "katecmd.h"
#include "kateglobal.h"
#include <ktexteditor/commandinterface.h>

#include <kapplication.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kcmdlineargs.h>
#include <kmainwindow.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobalsettings.h>
#include <kdefakes.h>
#include <kstatusbar.h>
#include <kio/job.h>

#include <memory>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>

#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtCore/QList>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/Q_PID>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>

#include <QtScript/QScriptEngine>
#include <QTest>
#include <qtest_kde.h>

#include "testutils.h"

QTEST_KDEMAIN(IndentTest, GUI)

/// somepath/part/tests/
const QString srcPath(KDESRCDIR);

#define FAILURE( test, comment ) qMakePair<const char*, const char*>( (test), (comment) )

void IndentTest::initTestCase()
{
  m_toplevel = new KMainWindow();
  m_document = new KateDocument(true, false, false, m_toplevel);
  m_view = static_cast<KateView *>(m_document->widget());
  m_env = new TestScriptEnv(m_document, m_outputCustomised);
}

void IndentTest::cleanupTestCase()
{
}

void IndentTest::getTestData(const QString& indenter)
{
  QTest::addColumn<QString>("testcase");

  // make sure the indenters are valid
  QFile indenterFile(srcPath + "/../script/data/" + indenter + ".js");
  QVERIFY(indenterFile.exists());
  QVERIFY(indenterFile.open(QFile::ReadOnly));
  QScriptValue result = m_env->engine()->evaluate(indenterFile.readAll(), indenterFile.fileName());
  QVERIFY2( !result.isError(), qPrintable(result.toString() + "\nat "
                                          + m_env->engine()->uncaughtExceptionBacktrace().join("\n")) );

  const QString testDir( srcPath + "/../../testdata/indent/" + indenter + '/' );
  Q_ASSERT( QFile::exists(testDir) );
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

void IndentTest::runTest(const ExpectedFailures& failures)
{
  QFETCH(QString, testcase);

  m_toplevel->resize( 800, 600); // restore size

  // load page
  KUrl url;
  url.setProtocol("file");
  url.setPath(testcase + "/origin");
  m_document->openUrl(url);

  // evaluate test-script
  QFile sourceFile(testcase + "/input.js");
  QVERIFY( sourceFile.open(QFile::ReadOnly) );

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

void IndentTest::cstyle_data()
{
  getTestData( "cstyle" );
}

void IndentTest::cstyle()
{
  runTest( ExpectedFailures() << FAILURE( "using1", "this is insane, those who write such code can cope with it :P" )
                              << FAILURE( "using2", "this is insane, those who write such code can cope with it :P" )
                              << FAILURE( "plist14", "in function signatures it might be wanted to use the indentation of the\n"
                                                     "opening paren instead of just increasing the indentation level like in function calls" )
                              << FAILURE( "switch10", "test for case where cfgSwitchIndent = false; needs proper config-interface" )
                              << FAILURE( "switch11", "test for case where cfgSwitchIndent = false; needs proper config-interface" )
                              << FAILURE( "visib2", "test for access modifier where cfgAccessModifiers = 1;needs proper config interface" )
                              << FAILURE( "visib3", "test for access modifier where cfgAccessModifiers = 1;needs proper config interface" )
                              << FAILURE( "plist10", "low low prio, maybe wontfix: if the user wants to add a arg, he should do so and press enter afterwards" )
                              << FAILURE( "switch13", "pure insanity, whoever wrote this test and expects that to be indented properly should stop writing code" )
  );
}

void IndentTest::haskell_data()
{
  getTestData( "haskell" );
}

void IndentTest::haskell()
{
  runTest( ExpectedFailures() );
}


void IndentTest::ruby_data()
{
  getTestData( "ruby" );
}

void IndentTest::ruby()
{
  runTest( ExpectedFailures() << FAILURE( "block01", "Multiline blocks using {} is not supported" )
                              << FAILURE( "block02", "Multiline blocks using {} is not supported" )
  );
}

void IndentTest::normal_data()
{
  getTestData( "normal" );
}

void IndentTest::normal()
{
  runTest( ExpectedFailures() << FAILURE( "emptyline1", "is that really what we expect?" )
                              << FAILURE( "emptyline3", "is that really what we expext?" )
  );
}
