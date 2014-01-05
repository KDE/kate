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
#include "katescripthelpers.h"

#include <QtScript/QScriptEngine>
#include <QTest>

//END Includes

//BEGIN TestScriptEnv

//BEGIN conversion functions for Cursors and Ranges
/** Converstion function from KTextEditor::Cursor to QtScript cursor */
static QScriptValue cursorToScriptValue(QScriptEngine *engine, const KTextEditor::Cursor &cursor)
{
  QString code = QString::fromLatin1("new Cursor(%1, %2);").arg(cursor.line())
                                               .arg(cursor.column());
  return engine->evaluate(code);
}

/** Converstion function from QtScript cursor to KTextEditor::Cursor */
static void cursorFromScriptValue(const QScriptValue &obj, KTextEditor::Cursor &cursor)
{
  cursor.setPosition(obj.property(QLatin1String("line")).toInt32(),
                     obj.property(QLatin1String("column")).toInt32());
}

/** Converstion function from QtScript range to KTextEditor::Range */
static QScriptValue rangeToScriptValue(QScriptEngine *engine, const KTextEditor::Range &range)
{
  QString code = QString::fromLatin1("new Range(%1, %2, %3, %4);").arg(range.start().line())
                                                      .arg(range.start().column())
                                                      .arg(range.end().line())
                                                      .arg(range.end().column());
  return engine->evaluate(code);
}

/** Converstion function from QtScript range to KTextEditor::Range */
static void rangeFromScriptValue(const QScriptValue &obj, KTextEditor::Range &range)
{
  range.setStart(KTextEditor::Cursor(
    obj.property(QLatin1String("start")).property(QLatin1String("line")).toInt32(),
    obj.property(QLatin1String("start")).property(QLatin1String("column")).toInt32()
  ));
  range.setEnd(KTextEditor::Cursor(
    obj.property(QLatin1String("end")).property(QLatin1String("line")).toInt32(),
    obj.property(QLatin1String("end")).property(QLatin1String("column")).toInt32()
  ));
}
//END

TestScriptEnv::TestScriptEnv(KateDocument *part, bool &cflag)
  : m_engine(0), m_viewObj(0), m_docObj(0), m_output(0)
{
  m_engine = new QScriptEngine(this);

  qScriptRegisterMetaType (m_engine, cursorToScriptValue, cursorFromScriptValue);
  qScriptRegisterMetaType (m_engine, rangeToScriptValue, rangeFromScriptValue);

  // export read & require function and add the require guard object
  m_engine->globalObject().setProperty(QLatin1String("read"), m_engine->newFunction(Kate::Script::read));
  m_engine->globalObject().setProperty(QLatin1String("require"), m_engine->newFunction(Kate::Script::require));
  m_engine->globalObject().setProperty(QLatin1String("require_guard"), m_engine->newObject());
  
  // export debug function
  m_engine->globalObject().setProperty(QLatin1String("debug"), m_engine->newFunction(Kate::Script::debug));

  // export translation functions
  m_engine->globalObject().setProperty(QLatin1String("i18n"), m_engine->newFunction(Kate::Script::i18n));
  m_engine->globalObject().setProperty(QLatin1String("i18nc"), m_engine->newFunction(Kate::Script::i18nc));
  m_engine->globalObject().setProperty(QLatin1String("i18ncp"), m_engine->newFunction(Kate::Script::i18ncp));
  m_engine->globalObject().setProperty(QLatin1String("i18np"), m_engine->newFunction(Kate::Script::i18np));


  KateView *view = qobject_cast<KateView *>(part->widget());

  m_viewObj = new KateViewObject(view);
  QScriptValue sv = m_engine->newQObject(m_viewObj);

  m_engine->globalObject().setProperty(QLatin1String("view"), sv);
  m_engine->globalObject().setProperty(QLatin1String("v"), sv);

  m_docObj = new KateDocumentObject(view->doc());
  QScriptValue sd = m_engine->newQObject(m_docObj);

  m_engine->globalObject().setProperty(QLatin1String("document"), sd);
  m_engine->globalObject().setProperty(QLatin1String("d"), sd);

  m_output = new OutputObject(view, cflag);
  QScriptValue so = m_engine->newQObject(m_output);

  m_engine->globalObject().setProperty(QLatin1String("output"), so);
  m_engine->globalObject().setProperty(QLatin1String("out"), so);
  m_engine->globalObject().setProperty(QLatin1String("o"), so);
}

TestScriptEnv::~TestScriptEnv()
{
  // delete explicitly, as the parent is the KTE::Document kpart, which is
  // reused for all tests. Hence, we explicitly have to delete the bindings.
  delete m_output; m_output = 0;
  delete m_docObj; m_docObj = 0;
  delete m_viewObj; m_viewObj = 0;

  // delete this too, although this should also be automagically be freed
  delete m_engine; m_engine = 0;

//   kDebug() << "deleted";
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
  QString str;
  for (int i = 0; i < context()->argumentCount(); ++i) {
    QScriptValue arg = context()->argument(i);
    str += arg.toString();
  }

  if (cp) {
    KTextEditor::Cursor c = view->cursorPosition();
    str += QLatin1Char('(') + QString::number(c.line()) + QLatin1Char(',') + QString::number(c.column()) + QLatin1Char(')');
  }

  if (ln) {
    str += QLatin1Char('\n');
  }

  view->insertText(str);

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

//END OutputObject
