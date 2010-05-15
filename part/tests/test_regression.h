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

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <kapplication.h>
#include <kurl.h>

class QScriptEngine;

class KateView;
class KateDocument;
class KateViewObject;
class KateDocumentObject;
class KCmdLineArgs;
class OutputObject;

namespace KParts {
  class OpenUrlArguments;
  struct BrowserArguments;
}

/**
 * @internal
 */
class RegressionTest : public QObject
{
    Q_OBJECT

  public:

    RegressionTest(KateDocument *part, const KConfig *baseConfig,
                   const QString &baseDir, const KCmdLineArgs *args);
    ~RegressionTest();

    enum OutputType { ResultDocument };
    void testStaticFile(const QString& filename, const QStringList &commands);
    enum CheckResult { Failure = 0, Success = 1, Ignored = 2 };
    CheckResult checkOutput(const QString& againstFilename);
    enum FailureType { NoFailure = 0, AllFailure = 1, ResultFailure = 4, NewFailure = 65536 };
    bool runTests(QString relPath = QString(), bool mustExist = false, int known_failure = NoFailure);
    bool allTestsSucceeded() const;

    bool reportResult(bool passed, const QString & description = QString(), bool *newfailure = 0 );
    bool reportResult(CheckResult result, const QString & description = QString(), bool *newfailure = 0 );

    void printSummary();

    void rereadConfig();
    static void createMissingDirs(const QString &path);

    void setFailureSnapshotConfig(KConfig *cfg, const QString &snapshotname);
    void setFailureSnapshotSaver(KConfig *cfg, const QString &snapshotname);

    void createLink( const QString& test, int failures );
    void doFailureReport( const QString& test, int failures );

  private:

    KateDocument *const m_part;
    KateView *const m_view;
    const KConfig *const m_baseConfig;
    QString m_baseDir;
    QString m_outputDir;
    bool m_genOutput;
    bool m_fork;
    KConfigGroup *m_failureComp;
    KConfigGroup *m_failureSave;

    QString m_currentCategory;
    QString m_currentTest;

    bool m_keepOutput;
    bool m_getOutput;
    int m_passes_work;
    int m_passes_fail;
    int m_passes_new;
    int m_failures_work;
    int m_failures_fail;
    int m_failures_new;
    int m_errors;
    int m_known_failures;
    bool m_outputCustomised;

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

class KateTestApp : public KApplication
{
  Q_OBJECT

  public:
    KateTestApp(KCmdLineArgs *args, const QString& baseDir, int testcaseIndex);
    virtual ~KateTestApp();

    bool allTestsSucceeded();

  public Q_SLOTS:
    void runTests();

  private:
    KCmdLineArgs *m_args;
    KConfig m_cfg;
    QString m_baseDir;
    int m_testcaseIndex;

    KateDocument *m_document;
    QPointer<RegressionTest> m_regressionTest;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
