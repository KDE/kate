/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 * Copyright 2006, 2007 Leo Savernik (l.savernik@aon.at)
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

#ifndef TEST_REGRESSION_H
#define TEST_REGRESSION_H

#include "katescriptview.h"
#include "katescriptdocument.h"

#include <kurl.h>
#include <kconfig.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptable>

class QScriptEngine;

class KateDocument;
class KateView;
class RegressionTest;

namespace KParts {
  class OpenUrlArguments;
  struct BrowserArguments;
}

class KateViewObject;
class KateDocumentObject;
class OutputObject;

/**
 * @internal
 * The backbone of Kate's automatic regression tests.
 */
class TestScriptEnv : public QObject
{
  public:
    explicit TestScriptEnv(KateDocument *part, bool &cflag);

    QScriptEngine *engine() const { return m_engine; }

    /** returns the output object */
    OutputObject *output() const { return m_output; }

  private:
    QScriptEngine *m_engine;
    KateViewObject *m_viewObj;
    KateDocumentObject *m_docObj;

    OutputObject *m_output;
};

/**
 * @internal
 */
class KateViewObject : public KateScriptView
{
    Q_OBJECT

  public:

    explicit KateViewObject(KateView *view);

    Q_INVOKABLE bool setCursorPosition(int row, int col);

    // Edit functions
    Q_INVOKABLE void keyReturn(int cnt = 1);
    Q_INVOKABLE void backspace(int cnt = 1);
    Q_INVOKABLE void deleteWordLeft(int cnt = 1);
    Q_INVOKABLE void keyDelete(int cnt = 1);
    Q_INVOKABLE void deleteWordRight(int cnt = 1);
    Q_INVOKABLE void transpose(int cnt = 1);
    Q_INVOKABLE void cursorLeft(int cnt = 1);
    Q_INVOKABLE void shiftCursorLeft(int cnt = 1);
    Q_INVOKABLE void cursorRight(int cnt = 1);
    Q_INVOKABLE void shiftCursorRight(int cnt = 1);
    Q_INVOKABLE void wordLeft(int cnt = 1);
    Q_INVOKABLE void shiftWordLeft(int cnt = 1);
    Q_INVOKABLE void wordRight(int cnt = 1);
    Q_INVOKABLE void shiftWordRight(int cnt = 1);
    Q_INVOKABLE void home(int cnt = 1);
    Q_INVOKABLE void shiftHome(int cnt = 1);
    Q_INVOKABLE void end(int cnt = 1);
    Q_INVOKABLE void shiftEnd(int cnt = 1);
    Q_INVOKABLE void up(int cnt = 1);
    Q_INVOKABLE void shiftUp(int cnt = 1);
    Q_INVOKABLE void down(int cnt = 1);
    Q_INVOKABLE void shiftDown(int cnt = 1);
    Q_INVOKABLE void scrollUp(int cnt = 1);
    Q_INVOKABLE void scrollDown(int cnt = 1);
    Q_INVOKABLE void topOfView(int cnt = 1);
    Q_INVOKABLE void shiftTopOfView(int cnt = 1);
    Q_INVOKABLE void bottomOfView(int cnt = 1);
    Q_INVOKABLE void shiftBottomOfView(int cnt = 1);
    Q_INVOKABLE void pageUp(int cnt = 1);
    Q_INVOKABLE void shiftPageUp(int cnt = 1);
    Q_INVOKABLE void pageDown(int cnt = 1);
    Q_INVOKABLE void shiftPageDown(int cnt = 1);
    Q_INVOKABLE void top(int cnt = 1);
    Q_INVOKABLE void shiftTop(int cnt = 1);
    Q_INVOKABLE void bottom(int cnt = 1);
    Q_INVOKABLE void shiftBottom(int cnt = 1);
    Q_INVOKABLE void toMatchingBracket(int cnt = 1);
    Q_INVOKABLE void shiftToMatchingBracket(int cnt = 1);

    Q_INVOKABLE bool type(const QString& str);

    // Aliases
    Q_INVOKABLE void enter(int cnt = 1);            // KeyReturn
    Q_INVOKABLE void cursorPrev(int cnt = 1);       // CursorLeft
    Q_INVOKABLE void left(int cnt = 1);             // CursorLeft
    Q_INVOKABLE void prev(int cnt = 1);             // CursorLeft
    Q_INVOKABLE void shiftCursorPrev(int cnt = 1);  // ShiftCursorLeft
    Q_INVOKABLE void shiftLeft(int cnt = 1);        // ShiftCursorLeft
    Q_INVOKABLE void shiftPrev(int cnt = 1);        // ShiftCursorLeft
    Q_INVOKABLE void cursorNext(int cnt = 1);       // CursorRight
    Q_INVOKABLE void right(int cnt = 1);            // CursorRight
    Q_INVOKABLE void next(int cnt = 1);             // CursorRight
    Q_INVOKABLE void shiftCursorNext(int cnt = 1);  // ShiftCursorRight
    Q_INVOKABLE void shiftRight(int cnt = 1);       // ShiftCursorRight
    Q_INVOKABLE void shiftNext(int cnt = 1);        // ShiftCursorRight
    Q_INVOKABLE void wordPrev(int cnt = 1);         // WordLeft
    Q_INVOKABLE void shiftWordPrev(int cnt = 1);    // ShiftWordLeft
    Q_INVOKABLE void wordNext(int cnt = 1);         // WordRight
    Q_INVOKABLE void shiftWordNext(int cnt = 1);    // ShiftWordRight

  private:
    Q_DISABLE_COPY(KateViewObject)
};

/**
 * @internal
 */
class KateDocumentObject : public KateScriptDocument
{
    Q_OBJECT

  public:
    explicit KateDocumentObject(KateDocument* doc);

  private:
    Q_DISABLE_COPY(KateDocumentObject)
};

/**
 * Customizing output to result-files. Writing any output into result files
 * inhibits outputting the content of the katepart after script execution,
 * enabling one to check for coordinates and the like.
 * @internal
 */
class OutputObject : public QObject, protected QScriptable
{
    Q_OBJECT

  public:
    OutputObject(KateView *v, bool &cflag);

    void setOutputFile(const QString &filename) { this->filename = filename; }

    void output(bool cp, bool ln);

    Q_INVOKABLE void write();
    Q_INVOKABLE void writeln();
    Q_INVOKABLE void writeLn();

    Q_INVOKABLE void print();
    Q_INVOKABLE void println();
    Q_INVOKABLE void printLn();

    Q_INVOKABLE void writeCursorPosition();
    Q_INVOKABLE void writeCursorPositionln();

    Q_INVOKABLE void cursorPosition();
    Q_INVOKABLE void cursorPositionln();
    Q_INVOKABLE void cursorPositionLn();

    Q_INVOKABLE void pos();
    Q_INVOKABLE void posln();
    Q_INVOKABLE void posLn();

  private:
    KateView *view;
    bool &cflag;
    QString filename;
};

/**
 * @internal
 */
class RegressionTest : public QObject
{
  Q_OBJECT
  public:

    RegressionTest(KateDocument *part, KConfig *baseConfig,
                   const QString &baseDir, const QString &outputDir,
                   bool _genOutput, bool fork);
    ~RegressionTest();

    enum OutputType { ResultDocument };
    void testStaticFile(const QString& filename, const QStringList &commands);
    enum CheckResult { Failure = 0, Success = 1, Ignored = 2 };
    CheckResult checkOutput(const QString& againstFilename);
    enum FailureType { NoFailure = 0, AllFailure = 1, ResultFailure = 4, NewFailure = 65536 };
    bool runTests(QString relPath = QString(), bool mustExist = false, int known_failure = NoFailure);
    bool reportResult( bool passed, const QString & description = QString(), bool *newfailure = 0 );
    bool reportResult(CheckResult result, const QString & description = QString(), bool *newfailure = 0 );
    void rereadConfig();
    static void createMissingDirs(const QString &path);

    void setFailureSnapshotConfig(KConfig *cfg, const QString &snapshotname);
    void setFailureSnapshotSaver(KConfig *cfg, const QString &snapshotname);

    void createLink( const QString& test, int failures );
    void doFailureReport( const QString& test, int failures );

    KateDocument *m_part;
    KateView *m_view;
    KConfig *m_baseConfig;
    QString m_baseDir;
    QString m_outputDir;
    bool m_genOutput;
    bool m_fork;
    QString m_currentBase;
    KConfigGroup *m_failureComp;
    KConfigGroup *m_failureSave;

    QString m_currentOutput;
    QString m_currentCategory;
    QString m_currentTest;

    bool m_keepOutput;
    bool m_getOutput;
    bool m_showGui;
    int m_passes_work;
    int m_passes_fail;
    int m_passes_new;
    int m_failures_work;
    int m_failures_fail;
    int m_failures_new;
    int m_errors;
    bool saw_failure;
    bool ignore_errors;
    int m_known_failures;
    bool m_outputCustomised;

    static RegressionTest *curr;

  private:
    void printDescription(const QString& description);

    static bool svnIgnored( const QString &filename );

  private:

    /**
     * evaluate script given by \c filename within the context of \c interp.
     * @param ignore if \c true don't evaluate if script does not exist but
     * return true nonetheless.
     * @return true if script was valid, false otherwise
     */
    bool evalJS(QScriptEngine *engine, const QString &filename, bool ignore = false);

    /**
     * concatenate contents of all list files down to but not including the
     * tests directory.
     * @param relPath relative path against tests-directory
     * @param filename file name of the list files
     */
    QStringList concatListFiles(const QString &relPath, const QString &filename);

  private Q_SLOTS:
    void slotOpenURL(const KUrl &url, const KParts::OpenUrlArguments&, const KParts::BrowserArguments&);
    void resizeTopLevelWidget( int, int );

};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
