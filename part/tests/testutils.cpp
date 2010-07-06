/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 * Copyright (C) 2003,2004 Stephan Kulow (coolo@kde.org)
 * Copyright (C) 2004 Dirk Mueller ( mueller@kde.org )
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

//BEGIN Includes
#include "testutils.h"

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

#include <QtScript/QScriptEngine>
#include <QTest>

//END Includes

//BEGIN TestScriptEnv

//BEGIN conversion functions for Cursors and Ranges
/** Converstion function from KTextEditor::Cursor to QtScript cursor */
static QScriptValue cursorToScriptValue(QScriptEngine *engine, const KTextEditor::Cursor &cursor)
{
  QString code = QString("new Cursor(%1, %2);").arg(cursor.line())
                                               .arg(cursor.column());
  return engine->evaluate(code);
}

/** Converstion function from QtScript cursor to KTextEditor::Cursor */
static void cursorFromScriptValue(const QScriptValue &obj, KTextEditor::Cursor &cursor)
{
  cursor.setPosition(obj.property("line").toInt32(),
                     obj.property("column").toInt32());
}

/** Converstion function from QtScript range to KTextEditor::Range */
static QScriptValue rangeToScriptValue(QScriptEngine *engine, const KTextEditor::Range &range)
{
  QString code = QString("new Range(%1, %2, %3, %4);").arg(range.start().line())
                                                      .arg(range.start().column())
                                                      .arg(range.end().line())
                                                      .arg(range.end().column());
  return engine->evaluate(code);
}

/** Converstion function from QtScript range to KTextEditor::Range */
static void rangeFromScriptValue(const QScriptValue &obj, KTextEditor::Range &range)
{
  range.start().setPosition(obj.property("start").property("line").toInt32(),
                            obj.property("start").property("column").toInt32());
  range.end().setPosition(obj.property("end").property("line").toInt32(),
                          obj.property("end").property("column").toInt32());
}
//END

TestScriptEnv::TestScriptEnv(KateDocument *part, bool &cflag)
  : m_engine(0), m_viewObj(0), m_docObj(0), m_output(0)
{
  m_engine = new QScriptEngine(this);

  qScriptRegisterMetaType (m_engine, cursorToScriptValue, cursorFromScriptValue);
  qScriptRegisterMetaType (m_engine, rangeToScriptValue, rangeFromScriptValue);

  initApi();

  KateView *view = qobject_cast<KateView *>(part->widget());

  m_viewObj = new KateViewObject(view);
  QScriptValue sv = m_engine->newQObject(m_viewObj);

  m_engine->globalObject().setProperty("view", sv);
  m_engine->globalObject().setProperty("v", sv);

  m_docObj = new KateDocumentObject(view->doc());
  QScriptValue sd = m_engine->newQObject(m_docObj);

  m_engine->globalObject().setProperty("document", sd);
  m_engine->globalObject().setProperty("d", sd);

  m_output = new OutputObject(view, cflag);
  QScriptValue so = m_engine->newQObject(m_output);

  m_engine->globalObject().setProperty("output", so);
  m_engine->globalObject().setProperty("out", so);
  m_engine->globalObject().setProperty("o", so);
}

TestScriptEnv::~TestScriptEnv()
{
  // delete explicitely, as the parent is the KTE::Document kpart, which is
  // reused for all tests. Hence, we explicitely have to delete the bindings.
  delete m_output; m_output = 0;
  delete m_docObj; m_docObj = 0;
  delete m_viewObj; m_viewObj = 0;

  // delete this too, although this should also be automagically be freed
  delete m_engine; m_engine = 0;

//   kDebug() << "deleted";
}

void TestScriptEnv::initApi()
{
  // cache file names
  static QStringList apiFileBaseNames;
  static QHash<QString, QString> apiBaseName2FileName;
  static QHash<QString, QString> apiBaseName2Content;

  // read katepart javascript api
    apiFileBaseNames.clear ();
    apiBaseName2FileName.clear ();
    apiBaseName2Content.clear ();

    // get all api files
    const QStringList list = KGlobal::dirs()->findAllResources("data","katepart/api/*.js", KStandardDirs::NoDuplicates);

    for ( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it )
    {
      // get abs filename....
      QFileInfo fi(*it);
      const QString absPath = fi.absoluteFilePath();
      const QString baseName = fi.baseName ();

      // remember filenames
      apiFileBaseNames.append (baseName);
      apiBaseName2FileName[baseName] = absPath;

      // read the file
      QFile file(absPath);
      QString content;
      if (!file.open(QIODevice::ReadOnly)) {
        kFatal() << i18n("Unable to find '%1'", absPath);
        return;
      } else {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        content = stream.readAll();
        file.close();
      }
      apiBaseName2Content[baseName] = content;
    }

    // sort...
    apiFileBaseNames.sort ();

  // register all script apis found
  for ( QStringList::ConstIterator it = apiFileBaseNames.constBegin(); it != apiFileBaseNames.constEnd(); ++it )
  {
    // try to load into engine, bail out one error, use fullpath for error messages
    QScriptValue apiObject = m_engine->evaluate(apiBaseName2Content[*it], apiBaseName2FileName[*it]);

    if(m_engine->hasUncaughtException()) {
      kFatal() << "Error loading script" << (*it);
      return;
    }
  }

  // success ;)
}

//END TestScriptEnv

//BEGIN KateViewObject

KateViewObject::KateViewObject(KateView *view)
  : KateScriptView()
{
  setView(view);
}

KateViewObject::~KateViewObject()
{
//   kDebug() << "deleted";
}

// Implements a function that calls an edit function repeatedly as specified by
// its first parameter (once if not specified).
#define REP_CALL(func) \
void KateViewObject::func(int cnt) {  \
  while (cnt--) { view()->func(); }   \
}
REP_CALL(keyReturn)
REP_CALL(backspace)
REP_CALL(deleteWordLeft)
REP_CALL(keyDelete)
REP_CALL(deleteWordRight)
REP_CALL(transpose)
REP_CALL(cursorLeft)
REP_CALL(shiftCursorLeft)
REP_CALL(cursorRight)
REP_CALL(shiftCursorRight)
REP_CALL(wordLeft)
REP_CALL(shiftWordLeft)
REP_CALL(wordRight)
REP_CALL(shiftWordRight)
REP_CALL(home)
REP_CALL(shiftHome)
REP_CALL(end)
REP_CALL(shiftEnd)
REP_CALL(up)
REP_CALL(shiftUp)
REP_CALL(down)
REP_CALL(shiftDown)
REP_CALL(scrollUp)
REP_CALL(scrollDown)
REP_CALL(topOfView)
REP_CALL(shiftTopOfView)
REP_CALL(bottomOfView)
REP_CALL(shiftBottomOfView)
REP_CALL(pageUp)
REP_CALL(shiftPageUp)
REP_CALL(pageDown)
REP_CALL(shiftPageDown)
REP_CALL(top)
REP_CALL(shiftTop)
REP_CALL(bottom)
REP_CALL(shiftBottom)
REP_CALL(toMatchingBracket)
REP_CALL(shiftToMatchingBracket)
#undef REP_CALL

bool KateViewObject::type(const QString& str) {
  return view()->doc()->typeChars(view(), str);
}

#define ALIAS(alias, func) \
void KateViewObject::alias(int cnt) { \
  func(cnt);                          \
}
ALIAS(enter, keyReturn)
ALIAS(cursorPrev, cursorLeft)
ALIAS(left, cursorLeft)
ALIAS(prev, cursorLeft)
ALIAS(shiftCursorPrev, shiftCursorLeft)
ALIAS(shiftLeft, shiftCursorLeft)
ALIAS(shiftPrev, shiftCursorLeft)
ALIAS(cursorNext, cursorRight)
ALIAS(right, cursorRight)
ALIAS(next, cursorRight)
ALIAS(shiftCursorNext, shiftCursorRight)
ALIAS(shiftRight, shiftCursorRight)
ALIAS(shiftNext, shiftCursorRight)
ALIAS(wordPrev, wordLeft)
ALIAS(shiftWordPrev, shiftWordLeft)
ALIAS(wordNext, wordRight)
ALIAS(shiftWordNext, shiftWordRight)
#undef ALIAS

//END KateViewObject

//BEGIN KateDocumentObject

KateDocumentObject::KateDocumentObject(KateDocument *doc)
  : KateScriptDocument()
{
  setDocument(doc);
}

KateDocumentObject::~KateDocumentObject()
{
//   kDebug() << "deleted";
}
//END KateDocumentObject

//BEGIN OutputObject

OutputObject::OutputObject(KateView *v, bool &cflag)
  : view(v), cflag(cflag)
{
}

OutputObject::~OutputObject()
{
//   kDebug() << "deleted";
}

void OutputObject::output(bool cp, bool ln)
{
  QFile out(filename);

  QString str;
  for (int i = 0; i < context()->argumentCount(); ++i) {
    QScriptValue arg = context()->argument(i);
    str += arg.toString();
  }

  if (cp) {
    KTextEditor::Cursor c = view->cursorPosition();
    str += '(' + QString::number(c.line()) + ',' + QString::number(c.column()) + ')';
  }

  if (ln) {
    str += '\n';
  }

  QFile::OpenMode mode = QFile::WriteOnly | (cflag ? QFile::Append : QFile::Truncate);
  if (!out.open(mode)) {
    fprintf(stderr, "ERROR: Could not append to %s\n", filename.toLatin1().constData());
  }
  out.write(str.toUtf8());
  cflag = true;
}

void OutputObject::write()
{
  output(false, false);
}

void OutputObject::writeln()
{
  output(false, true);
}

void OutputObject::writeLn()
{
  output(false, true);
}

void OutputObject::print()
{
  output(false, false);
}

void OutputObject::println()
{
  output(false, true);
}

void OutputObject::printLn()
{
  output(false, true);
}

void OutputObject::writeCursorPosition()
{
  output(true, false);
}

void OutputObject::writeCursorPositionln()
{
  output(true, true);
}

void OutputObject::cursorPosition()
{
  output(true, false);
}

void OutputObject::cursorPositionln()
{
  output(true, true);
}

void OutputObject::cursorPositionLn()
{
  output(true, true);
}

void OutputObject::pos()
{
  output(true, false);
}

void OutputObject::posln()
{
  output(true, true);
}

void OutputObject::posLn()
{
  output(true, true);
}

void OutputObject::createMissingDirs(const QString& filename)
{
  QFileInfo dif(filename);
  QFileInfo dirInfo( dif.dir().path() );
  if (dirInfo.exists())
    return;

  QStringList pathComponents;
  QFileInfo parentDir = dirInfo;
  pathComponents.prepend(parentDir.absoluteFilePath());
  while (!parentDir.exists()) {
    QString parentPath = parentDir.absoluteFilePath();
    int slashPos = parentPath.lastIndexOf('/');
    if (slashPos < 0)
      break;
    parentPath = parentPath.left(slashPos);
    pathComponents.prepend(parentPath);
    parentDir = QFileInfo(parentPath);
  }
  for (int pathno = 1; pathno < pathComponents.count(); pathno++) {
    if (!QFileInfo(pathComponents[pathno]).exists() &&
        !QDir(pathComponents[pathno-1]).mkdir(pathComponents[pathno])) {
      fprintf(stderr,"Error creating directory %s\n",pathComponents[pathno].toLatin1().constData());
      exit(1);
    }
  }
}

//END OutputObject
