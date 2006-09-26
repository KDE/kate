/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 * Copyright 2006 Leo Savernik (l.savernik@aon.at)
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

#include <katejscript.h>
#include <kateview.h>
#include <kurl.h>
#include <qobject.h>
#include <qstringlist.h>
#include <kjs/ustring.h>
#include <kjs/object.h>
#include <kjs/interpreter.h>

class KateDocument;
class KateView;
class RegressionTest;
class QTimer;

namespace KParts {
  struct URLArgs;
}

/**
 * @internal
 * The backbone of Kate's automatic regression tests.
 */
class TestJScriptEnv : public KateJScriptInterpreterContext
{
  public:
    TestJScriptEnv(KateDocument *part);
    virtual ~TestJScriptEnv();

    /** returns the global scope */
    KJS::JSObject *global() const { return m_global; }
    /** returns the script interpreter */
    KJS::Interpreter &interpreter() { return *m_interpreter; }
    /** returns the document scope */
    KJS::JSObject *document() const { return m_document; }
    /** returns the view scope */
    KJS::JSObject *view() const { return m_view; }
};

/**
 * @internal
 */
class KateViewObject : public KJS::JSObject
{
  public:
    KateViewObject(KJS::ExecState *exec, KateView *v, KJS::JSObject *fallback);
    virtual ~KateViewObject();

    virtual const KJS::ClassInfo *classInfo() const;
//     virtual KJS::JSValue *get(KJS::ExecState *exec, const KJS::Identifier &propertyName) const;

    virtual bool getOwnPropertySlot(KJS::ExecState *, const KJS::Identifier&, KJS::PropertySlot&);
    virtual bool getOwnPropertySlot(KJS::ExecState *, unsigned index, KJS::PropertySlot&);

  private:
    // evil hack I: class layout of katejscript/KateJSView must be duplicated
    // here, structurally as well as functionally
    KateView *view;
    // end evil hack
    KJS::JSObject *fallback;
};

/**
 * @internal
 */
class KateViewFunction : public KJS::JSObject
{
  public:
    KateViewFunction(KJS::ExecState *exec, KateView *v, int _id, int length);

    bool implementsCall() const;
    virtual KJS::JSValue *callAsFunction(KJS::ExecState *exec, KJS::JSObject *thisObj, const KJS::List &args);

    enum { KeyReturn, Type };
  private:
    KateView *m_view;
    int id;
};

/**
 * @internal
 */
class RegressionTest : public QObject
{
  Q_OBJECT
public:

    RegressionTest(KateDocument *part, const QString &baseDir,
                   const QString &outputDir, bool _genOutput);
    ~RegressionTest();

    enum OutputType { ResultDocument };
    void testStaticFile(const QString& filename, const QStringList &commands);
    enum CheckResult { Failure = 0, Success = 1, Ignored = 2 };
    CheckResult checkOutput(const QString& againstFilename);
    enum FailureType { NoFailure = 0, AllFailure = 1, ResultFailure = 4, NewFailure = 65536 };
    bool runTests(QString relPath = QString::null, bool mustExist = false, int known_failure = NoFailure);
    bool reportResult( bool passed, const QString & description = QString::null, bool *newfailure = 0 );
    bool reportResult(CheckResult result, const QString & description = QString::null, bool *newfailure = 0 );
    void createMissingDirs(const QString &path);

    void setFailureSnapshotConfig(KConfig *cfg, const QString &snapshotname);
    void setFailureSnapshotSaver(KConfig *cfg, const QString &snapshotname);

    void createLink( const QString& test, int failures );
    void doFailureReport( const QString& test, int failures );

    KateDocument *m_part;
    KateView *m_view;
    QString m_baseDir;
    QString m_outputDir;
    bool m_genOutput;
    QString m_currentBase;
    KConfig *m_failureComp;
    KConfig *m_failureSave;

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
    bool evalJS( KJS::Interpreter &interp, const QString &filename, bool ignore = false);
    /**
     * concatenate contents of all list files down to but not including the
     * tests directory.
     * @param relPath relative path against tests-directory
     * @param filename file name of the list files
     */
    QStringList concatListFiles(const QString &relPath, const QString &filename);

private slots:
    void slotOpenURL(const KUrl &url, const KParts::URLArgs &args);
    void resizeTopLevelWidget( int, int );

};

#endif
