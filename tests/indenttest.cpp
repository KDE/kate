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

#include "script_test_base.h"
#include "testutils.h"

QTEST_KDEMAIN(IndentTest, GUI)

#define FAILURE( test, comment ) qMakePair<const char*, const char*>( (test), (comment) )

void IndentTest::initTestCase()
{
  ScriptTestBase::initTestCase();
  m_section = "indent";
  m_script_dir = "indentation";
}


void IndentTest::testCstyle_data()
{
  getTestData( "cstyle" );
}

void IndentTest::testCstyle()
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


void IndentTest::testCppstyle_data()
{
    getTestData( "cppstyle" );
}

void IndentTest::testCppstyle()
{
    runTest(
        ExpectedFailures()
          /// \todo Fix (smth) to make failed test cases really work!
          << FAILURE( "parens1", "dunno why it failed in test! in manual mode everything works fine..." )
      );
}

void IndentTest::testCMake_data()
{
    getTestData( "cmake" );
}

void IndentTest::testCMake()
{
    runTest( ExpectedFailures() );
}


void IndentTest::testPython_data()
{
    getTestData( "python" );
}

void IndentTest::testPython()
{
    runTest( ExpectedFailures() );
}


void IndentTest::testHaskell_data()
{
  getTestData( "haskell" );
}

void IndentTest::testHaskell()
{
  runTest( ExpectedFailures() );
}


void IndentTest::latex_data()
{
  getTestData( "latex" );
}

void IndentTest::latex()
{
  runTest( ExpectedFailures() );
}


void IndentTest::testPascal_data()
{
  getTestData( "pascal" );
}

void IndentTest::testPascal()
{
  runTest( ExpectedFailures() );
}


void IndentTest::testRuby_data()
{
  getTestData( "ruby" );
}

void IndentTest::testRuby()
{
  runTest( ExpectedFailures() << FAILURE( "block01", "Multiline blocks using {} is not supported" )
                              << FAILURE( "block02", "Multiline blocks using {} is not supported" )
                              << FAILURE( "singleline01", "Single line defs are not supported" )
                              << FAILURE( "singleline02", "Single line defs are not supported" )
                              << FAILURE( "wordlist01", "multiline word list is not supported" )
                              << FAILURE( "wordlist02", "multiline word list is not supported" )
                              << FAILURE( "wordlist11", "multiline word list is not supported" )
                              << FAILURE( "wordlist12", "multiline word list is not supported" )
                              << FAILURE( "wordlist21", "multiline word list is not supported" )
                              << FAILURE( "wordlist22", "multiline word list is not supported" )
                              << FAILURE( "if20", "multi line if assignment is not supported" )
                              << FAILURE( "if21", "multi line if assignment is not supported" )
                              << FAILURE( "if30", "single line if is not supported" )
                              << FAILURE( "if31", "single line if is not supported" )
  );
}


void IndentTest::testXml_data()
{
  getTestData( "xml" );
}

void IndentTest::testXml()
{
  runTest( ExpectedFailures() );
}


void IndentTest::testNormal_data()
{
  getTestData( "normal" );
}

void IndentTest::testNormal()
{
  runTest( ExpectedFailures() << FAILURE( "emptyline1", "is that really what we expect?" )
                              << FAILURE( "emptyline3", "is that really what we expext?" )
  );
}
