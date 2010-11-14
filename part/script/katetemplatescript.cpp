/* This file is part of the KDE libraries
   Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katetemplatescript.h"

#include <QScriptValue>
#include <QScriptEngine>

#include "katedocument.h"
#include "kateview.h"

KateTemplateScript::KateTemplateScript(const QString& script)
  : KateScript(script,InputSCRIPT) {}

KateTemplateScript::~KateTemplateScript(){}

QString KateTemplateScript::invoke(KateView* view, const QString& functionName, const QString &srcText) {

  if(!setView(view))
    return QString();

  clearExceptions();
  QScriptValue myFunction = function(functionName);
  if(!myFunction.isValid()) {
    return QString();
  }

  QScriptValueList arguments;
  arguments << QScriptValue(m_engine, srcText);

  QScriptValue result = myFunction.call(QScriptValue(), arguments);
  
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(result, "Error while calling helper function");
    return QString();
  }
 
  
  if (result.isNull()) {
    return QString();
  }

  return result.toString();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
