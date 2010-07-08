/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009 Dominik Haumann <dhaumann kde org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katecommandlinescript.h"

#include <QScriptValue>
#include <QScriptEngine>

#include <klocale.h>

#include "katedocument.h"
#include "kateview.h"
#include "katecmd.h"
#include "kshell.h"

KateCommandLineScript::KateCommandLineScript(const QString &url, const KateCommandLineScriptHeader &header)
  : KateScript(url)
  , m_commandHeader(header)
{
  KateCmd::self()->registerCommand (this);
}

KateCommandLineScript::~KateCommandLineScript()
{
  KateCmd::self()->unregisterCommand (this);
}

const KateCommandLineScriptHeader& KateCommandLineScript::commandHeader()
{
  return m_commandHeader;
}


bool KateCommandLineScript::callFunction(const QString& cmd, const QStringList args, QString &errorMessage)
{
  clearExceptions();
  QScriptValue command = function(cmd);
  if(!command.isValid()) {
    errorMessage = i18n("Function '%1' not found in script: %2", cmd, url());
    return false;
  }

  // add the arguments that we are going to pass to the function
  QScriptValueList arguments;
  foreach (const QString& arg, args) {
    arguments << QScriptValue(m_engine, arg);
  }

  QScriptValue result = command.call(QScriptValue(), arguments);
  // error during the calling?
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(result, i18n("Error calling %1", cmd));
    errorMessage = i18n("Error calling '%1'. Please check for syntax errors.", cmd);
    return false;
  }

  return true;
}

ScriptActionInfo KateCommandLineScript::actionInfo(const QString& cmd)
{
  clearExceptions();
  QScriptValue actionFunc = function("action");
  if(!actionFunc.isValid()) {
    kDebug() << i18n("Function 'action' not found in script: %1", url());
    return ScriptActionInfo();
  }

  // add the arguments that we are going to pass to the function
  QScriptValueList arguments;
  arguments << cmd;

  QScriptValue result = actionFunc.call(QScriptValue(), arguments);
  // error during the calling?
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(result, i18n("Error calling action(%1)", cmd));
    return ScriptActionInfo();
  }

  ScriptActionInfo info;
  info.setCommand(cmd);
  info.setText(result.property("text").toString());
  info.setIcon(result.property("icon").toString());
  info.setCategory(result.property("category").toString());
  info.setInteractive(result.property("interactive").toBool());
  info.setShortcut(result.property("shortcut").toString());

  return info;
}

const QStringList &KateCommandLineScript::cmds ()
{
  return m_commandHeader.functions();
}

bool KateCommandLineScript::exec (KTextEditor::View *view, const QString &_cmd, QString &errorMsg)
{
  KShell::Errors errorCode;
  QStringList args(KShell::splitArgs(_cmd, KShell::NoOptions, &errorCode));

  if (errorCode != KShell::NoError) {
    errorMsg = i18n("Bad quoting in call: %1. Please escape single quotes with a backslash.", _cmd);
    return false;
  }

  QString cmd(args.first());
  args.removeFirst();

  if (!view) {
    errorMsg = i18n("Could not access view");
    return false;
  }

  if (setView(qobject_cast<KateView*>(view))) {
    // setView fails, if the script cannot be loaded
    return callFunction(cmd, args, errorMsg);
  }

  return false;
}


bool KateCommandLineScript::help(KTextEditor::View* view, const QString& cmd, QString &msg)
{
  if (!setView(qobject_cast<KateView*>(view))) {
    // setView fails, if the script cannot be loaded
    return false;
  }

  clearExceptions();
  QScriptValue helpFunction = function("help");
  if(!helpFunction.isValid()) {
    return false;
  }

  // add the arguments that we are going to pass to the function
  QScriptValueList arguments;
  arguments << QScriptValue(m_engine, cmd);

  QScriptValue result = helpFunction.call(QScriptValue(), arguments);

  // error during the calling?
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(result, i18n("Error calling 'help %1'", cmd));
    msg = i18n("Error calling '%1'. Please check for syntax errors.", cmd);
    return false;
  }

  if (result.isUndefined() || !result.isString()) {
    kDebug(13050) << i18n("No help specified for command '%1' in script %2", cmd, url());
    return false;
  }
  msg = result.toString();

  return !msg.isEmpty();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
