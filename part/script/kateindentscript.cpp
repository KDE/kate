
/// This file is part of the KDE libraries
/// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License version 2 as published by the Free Software Foundation.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
///
/// You should have received a copy of the GNU Library General Public License
/// along with this library; see the file COPYING.LIB.  If not, write to
/// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
/// Boston, MA 02110-1301, USA.

#include <QScriptValue>
#include <QScriptEngine>

#include "katedocument.h"
#include "kateview.h"


#include "kateindentscript.h"

KateIndentScript::KateIndentScript(const QString &url, const KateScriptInformation &information)
  : KateScript(url, information)
{
}

int KateIndentScript::indent(KateView* view, const KTextEditor::Cursor& position,
                             QChar typedCharacter, int indentWidth)
{
  // if it hasn't loaded or we can't load, return
  if(!load())
    return -2;
  QScriptValue indentFunction = function("indent");
  if(!indentFunction.isValid()) {
    return -2;
  }
  // set the view/document objects as necessary
  m_engine->globalObject().setProperty("view", m_engine->newQObject(view));
  m_engine->globalObject().setProperty("document", m_engine->newQObject(view->document()));
  // add the arguments that we are going to pass to the function
  QScriptValueList arguments;
  arguments << QScriptValue(m_engine, position.line());
  arguments << QScriptValue(m_engine, indentWidth);
  arguments << QScriptValue(m_engine, typedCharacter.isNull() ? QString("") : QString(typedCharacter));
  // get the required indent
  QScriptValue result = indentFunction.call(QScriptValue(), arguments);
  // error during the calling?
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(QString("Error calling indent()\n"));
    return -2;
  }
  int indentAmount = result.toInteger();
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(QString("Bad return type (must be integer)\n"));
    return -2;
  }
  return indentAmount;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
