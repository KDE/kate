// This file is part of the KDE libraries
// Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "katescripthelpers.h"
#include "katescript.h"
#include "katescriptdocument.h"
#include "katescriptview.h"
#include "kateview.h"
#include "katedocument.h"

#include <iostream>

#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptContext>
#include <QFile>

#include <kdebug.h>
#include <klocale.h>
#include <klocalizedstring.h>
#include <kstandarddirs.h>

namespace Kate {
namespace Script {
  
bool readFile(const QString& sourceUrl, QString& sourceCode)
{
  sourceCode = QString();

  QFile file(sourceUrl);
  if (!file.open(QIODevice::ReadOnly)) {
    kDebug(13050) << i18n("Unable to find '%1'", sourceUrl);
    return false;
  } else {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    sourceCode = stream.readAll();
    file.close();
  }
  return true;
}

QScriptValue read(QScriptContext *context, QScriptEngine *)
{
  /**
   * just search for all given files and read them all
   */
  QString fullContent;
  for(int i = 0; i < context->argumentCount(); ++i) {
    /**
     * get full name of file
     * skip on errors
     */
    const QString name = context->argument(i).toString();
    const QString fullName = KGlobal::dirs()->findResource ("data", "katepart/script/files/" + name);
    if (fullName.isEmpty())
      continue;

    /**
     * try to read complete file
     * skip non-existing files
     */
    QString content;
    if (!readFile (fullName, content))
      continue;

    /**
     * concat to full content
     */
    fullContent += content;
  }
  
  /**
   * return full content
   */
  return QScriptValue (fullContent);
}
  
QScriptValue require(QScriptContext *context, QScriptEngine *engine)
{
  /**
   * just search for all given scripts and eval them
   */
  for(int i = 0; i < context->argumentCount(); ++i) {
    /**
     * get full name of file
     * skip on errors
     */
    const QString name = context->argument(i).toString();
    const QString fullName = KGlobal::dirs()->findResource ("data", "katepart/script/libraries/" + name);
    if (fullName.isEmpty())
      continue;

    /**
     * check include guard
     */
    QScriptValue require_guard = engine->globalObject().property ("require_guard");
    if (require_guard.property (fullName).toBool ())
      continue;
    
    /**
     * try to read complete file
     * skip non-existing files
     */
    QString code;
    if (!readFile (fullName, code))
      continue;

    /**
     * fixup QScriptContext
     * else the nested evaluate will not work :/
     * see http://www.qtcentre.org/threads/31027-QtScript-nesting-with-include-imports-or-spawned-script-engines
     * http://www.qtcentre.org/threads/20432-Can-I-include-a-script-from-script
     */
    QScriptContext *context = engine->currentContext();
    QScriptContext *parent = context->parentContext();
    if (parent) {
        context->setActivationObject(context->parentContext()->activationObject());
        context->setThisObject(context->parentContext()->thisObject());
    }

    /**
     * eval in current script engine
     */
    engine->evaluate (code, fullName);
    
    /**
     * set include guard
     */
    require_guard.setProperty (fullName, QScriptValue (true));
  }
  
  /**
   * no return value
   */
  return engine->nullValue();
}

QScriptValue debug(QScriptContext *context, QScriptEngine *engine)
{
  QStringList message;
  for(int i = 0; i < context->argumentCount(); ++i) {
    message << context->argument(i).toString();
  }
  // debug in blue to distance from other debug output if necessary
  std::cerr << "\033[31m" << qPrintable(message.join(" ")) << "\033[0m\n";
  return engine->nullValue();
}

//BEGIN code adapted from kdelibs/kross/modules/translation.cpp
/// helper function to do the substitution from QtScript > QVariant > real values
KLocalizedString substituteArguments( const KLocalizedString &kls, const QVariantList &arguments, int max = 99 )
{
  KLocalizedString ls = kls;
  int cnt = qMin( arguments.count(), max ); // QString supports max 99
  for ( int i = 0; i < cnt; ++i ) {
    QVariant arg = arguments[i];
    switch ( arg.type() ) {
      case QVariant::Int: ls = ls.subs(arg.toInt()); break;
      case QVariant::UInt: ls = ls.subs(arg.toUInt()); break;
      case QVariant::LongLong: ls = ls.subs(arg.toLongLong()); break;
      case QVariant::ULongLong: ls = ls.subs(arg.toULongLong()); break;
      case QVariant::Double: ls = ls.subs(arg.toDouble()); break;
      default: ls = ls.subs(arg.toString()); break;
    }
  }
  return ls;
}

/// i18n("text", arguments [optional])
QScriptValue i18n( QScriptContext *context, QScriptEngine *engine )
{
  Q_UNUSED(engine)
  QString text;
  QVariantList args;
  const int argCount = context->argumentCount();

  if (argCount == 0) {
    kWarning(13050) << "wrong usage of i18n:" << context->backtrace().join("\n\t");
  }

  if (argCount > 0) {
    text = context->argument(0).toString();
  }

  for (int i = 1; i < argCount; ++i) {
    args << context->argument(i).toVariant();
  }

  KLocalizedString ls = ki18n(text.toUtf8());
  return substituteArguments( ls, args ).toString();
}

/// i18nc("context", "text", arguments [optional])
QScriptValue i18nc( QScriptContext *context, QScriptEngine *engine )
{
  Q_UNUSED(engine)
  QString text;
  QString textContext;
  QVariantList args;
  const int argCount = context->argumentCount();

  if (argCount < 2) {
    kWarning(13050) << "wrong usage of i18nc:" << context->backtrace().join("\n\t");
  }

  if (argCount > 0) {
    textContext = context->argument(0).toString();
  }

  if (argCount > 1) {
    text = context->argument(1).toString();
  }

  for (int i = 2; i < argCount; ++i) {
    args << context->argument(i).toVariant();
  }

  KLocalizedString ls = ki18nc(textContext.toUtf8(), text.toUtf8());
  return substituteArguments( ls, args ).toString();
}

/// i18np("singular", "plural", number, arguments [optional])
QScriptValue i18np( QScriptContext *context, QScriptEngine *engine )
{
  Q_UNUSED(engine)
  QString trSingular;
  QString trPlural;
  int number = 0;
  QVariantList args;
  const int argCount = context->argumentCount();

  if (argCount < 3) {
    kWarning(13050) << "wrong usage of i18np:" << context->backtrace().join("\n\t");
  }

  if (argCount > 0) {
    trSingular = context->argument(0).toString();
  }

  if (argCount > 1) {
    trPlural = context->argument(1).toString();
  }

  if (argCount > 2) {
    number = context->argument(2).toInt32();
  }

  for (int i = 3; i < argCount; ++i) {
    args << context->argument(i).toVariant();
  }

  KLocalizedString ls = ki18np(trSingular.toUtf8(), trPlural.toUtf8()).subs(number);
  return substituteArguments( ls, args, 98 ).toString();
}

/// i18ncp("context", "singular", "plural", number, arguments [optional])
QScriptValue i18ncp( QScriptContext *context, QScriptEngine *engine )
{
  Q_UNUSED(engine)
  QString trContext;
  QString trSingular;
  QString trPlural;
  int number = 0;
  QVariantList args;
  const int argCount = context->argumentCount();

  if (argCount < 4) {
    kWarning(13050) << "wrong usage of i18ncp:" << context->backtrace().join("\n\t");
  }

  if (argCount > 0) {
    trContext = context->argument(0).toString();
  }

  if (argCount > 1) {
    trSingular = context->argument(1).toString();
  }

  if (argCount > 2) {
    trPlural = context->argument(2).toString();
  }

  if (argCount > 3) {
    number = context->argument(3).toInt32();
  }

  for (int i = 4; i < argCount; ++i) {
    args << context->argument(i).toVariant();
  }

  KLocalizedString ls = ki18ncp(trContext.toUtf8(), trSingular.toUtf8(), trPlural.toUtf8()).subs( number );
  return substituteArguments( ls, args, 98 ).toString();
}
//END code adapted from kdelibs/kross/modules/translation.cpp

}
}

// kate: space-indent on; indent-width 2; replace-tabs on;
