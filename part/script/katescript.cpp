// This file is part of the KDE libraries
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2009 Dominik Haumann <dhaumann kde org>
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

#include "katescript.h"
#include "katescriptdocument.h"
#include "katescriptview.h"
#include "kateview.h"
#include "katedocument.h"

#include <iostream>

#include <QFile>

#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptContext>
#include <QFileInfo>

#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

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



namespace Kate {
  namespace Script {

    QScriptValue debug(QScriptContext *context, QScriptEngine *engine) {
      QStringList message;
      for(int i = 0; i < context->argumentCount(); ++i) {
        message << context->argument(i).toString();
      }
      // debug in blue to distance from other debug output if necessary
      std::cerr << "\033[34m" << qPrintable(message.join(" ")) << "\033[0m\n";
      return engine->nullValue();
    }

  }
}



bool KateScript::s_scriptingApiLoaded = false;

void KateScript::reloadScriptingApi()
{
  s_scriptingApiLoaded = false;
}

bool KateScript::readFile(const QString& sourceUrl, QString& sourceCode)
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


KateScript::KateScript(const QString &url)
  : m_loaded(false)
  , m_loadSuccessful(false)
  , m_url(url)
  , m_engine(0)
  , m_document(0)
  , m_view(0)
{
}

KateScript::~KateScript()
{
  if(m_loadSuccessful) {
    // remove data...
    delete m_engine;
    delete m_document;
    delete m_view;
  }
}

void KateScript::displayBacktrace(const QScriptValue &error, const QString &header)
{
  if(!m_engine) {
    std::cerr << "KateScript::displayBacktrace: no engine, cannot display error\n";
    return;
  }
  std::cerr << "\033[31m";
  if(!header.isNull())
    std::cerr << qPrintable(header) << ":\n";
  if(error.isError())
    std::cerr << qPrintable(error.toString()) << '\n';
    std::cerr << qPrintable(m_engine->uncaughtExceptionBacktrace().join("\n"));
    std::cerr << "\033[0m" << '\n';
}

void KateScript::clearExceptions()
{
  m_engine->clearExceptions();
}

QScriptValue KateScript::global(const QString &name)
{
  // load the script if necessary
  if(!load())
    return QScriptValue();
  return m_engine->globalObject().property(name);
}

QScriptValue KateScript::function(const QString &name)
{
  QScriptValue value = global(name);
  if(!value.isFunction())
    return QScriptValue();
  return value;
}

bool KateScript::initApi ()
{
  // cache file names
  static QStringList apiFileBaseNames;
  static QHash<QString, QString> apiBaseName2FileName;
  static QHash<QString, QString> apiBaseName2Content;
  
  // read katepart javascript api
  if (!s_scriptingApiLoaded) {
    s_scriptingApiLoaded = true;
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
      QString content;
      readFile(absPath, content);
      apiBaseName2Content[baseName] = content;
    }
    
    // sort...
    apiFileBaseNames.sort ();
  }
  
  // register all script apis found
  for ( QStringList::ConstIterator it = apiFileBaseNames.begin(); it != apiFileBaseNames.end(); ++it )
  {
    // try to load into engine, bail out one error, use fullpath for error messages
    QScriptValue apiObject = m_engine->evaluate(apiBaseName2Content[*it], apiBaseName2FileName[*it]);
    if (hasException(apiObject, apiBaseName2FileName[*it])) {
      return false;
    }
  }
  
  // success ;)
  return true;
}

bool KateScript::load()
{
  if(m_loaded)
    return m_loadSuccessful;

  m_loaded = true;
  m_loadSuccessful = false; // here set to false, and at end of function to true

  // read the script file into memory
  QString source;
  if (!readFile(m_url, source)) {
    return false;
  }

  // create script engine, register meta types
  m_engine = new QScriptEngine();
  qScriptRegisterMetaType (m_engine, cursorToScriptValue, cursorFromScriptValue);
  qScriptRegisterMetaType (m_engine, rangeToScriptValue, rangeFromScriptValue);

  // init API
  if (!initApi ())
    return false;
  
  // register scripts itself
  QScriptValue result = m_engine->evaluate(source, m_url);
  if (hasException(result, m_url)) {
    return false;
  }

  // yip yip!
  initEngine();
  m_loadSuccessful = true;
  return true;
}

bool KateScript::hasException(const QScriptValue& object, const QString& file)
{
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(object, i18n("Error loading script %1\n", file));
    m_errorMessage = i18n("Error loading script %1", file);
    delete m_engine;
    m_engine = 0;
    m_loadSuccessful = false;
    return true;
  }
  return false;
}


void KateScript::initEngine() {
  // set the view/document objects as necessary
  m_engine->globalObject().setProperty("document", m_engine->newQObject(m_document = new KateScriptDocument()));
  m_engine->globalObject().setProperty("view", m_engine->newQObject(m_view = new KateScriptView()));

  m_engine->globalObject().setProperty("debug", m_engine->newFunction(Kate::Script::debug));
}

bool KateScript::setView(KateView *view)
{
  if (!load())
    return false;
  // setup the stuff
  m_document->setDocument (view->doc());
  m_view->setView (view);
  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
