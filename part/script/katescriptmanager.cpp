
/// This file is part of the KDE libraries
/// Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
/// Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
/// Copyright (C) 2006 Dominik Haumann <dhaumann kde org>
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

#include "katescriptmanager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QMap>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

#include "kateglobal.h"

KateScriptManager::KateScriptManager() : KTextEditor::Command()
{
  // false = force (ignore cache)
  collect("katepartscriptrc", "katepart/script/*.js", false);
}

KateScriptManager::~KateScriptManager()
{
  qDeleteAll(m_scripts);
}

KateIndentScript *KateScriptManager::indenter(const QString &language)
{
  KateIndentScript *highestPriorityIndenter = 0;
  foreach(KateIndentScript *indenter, m_languageToIndenters.value(language.toLower())) {
    // don't overwrite if there is already a result with a higher priority
    if(highestPriorityIndenter && indenter->information().priority < highestPriorityIndenter->information().priority) {
      kDebug(13050) << "KateScriptManager::indenter: Not overwriting indenter for '"
                    << language << "' as the priority isn't big enough (" <<
                    indenter->information().priority << " < "
                    << highestPriorityIndenter->information().priority << ')';
    }
    else {
      highestPriorityIndenter = indenter;
    }
  }
  if(!highestPriorityIndenter) {
    kDebug(13050) << "KateScriptManager::indenter: No indenter for " << language;
  }
  return highestPriorityIndenter;
}

void KateScriptManager::collect(const QString& resourceFile,
                                const QString& directory,
                                bool force)
{
  KConfig cfgFile(resourceFile, KConfig::NoGlobals);
  KConfigGroup config = cfgFile.group("General");

  force = false;
  // If KatePart version does not match, better force a true reload
  if(QString(KATEPART_VERSION) != config.readEntry("kate-version", QString("0.0"))) {
    config.writeEntry("kate-version", QString(KATEPART_VERSION));
    force = true;
  }
  // get a list of all .js files
  QStringList list = KGlobal::dirs()->findAllResources("data", directory, KStandardDirs::NoDuplicates);
  // clear out the old scripts and reserve enough space
  qDeleteAll(m_scripts);
  m_scripts.clear();
  m_languageToIndenters.clear();
  m_scripts.reserve(list.size());

  // iterate through the files and read info out of cache or file
  for(QStringList::ConstIterator fileit = list.begin(); fileit != list.end(); ++fileit) {
    // get abs filename....
    QFileInfo fi(*fileit);
    QString absPath = fi.absoluteFilePath();
    QString baseName = fi.baseName ();

    // each file has a group
    QString group = "Cache "+ *fileit;
    config.changeGroup(group);

    // stat the file to get the last-modified-time
    struct stat sbuf;
    memset(&sbuf, 0, sizeof(sbuf));
    stat(QFile::encodeName(*fileit), &sbuf);

    // check whether file is already cached
    bool useCache = false;
    if(!force && cfgFile.hasGroup(group)) {
      useCache = (sbuf.st_mtime == config.readEntry("last-modified", 0));
    }

    // read key/value pairs from the cached file if possible
    // otherwise, parse it and then save the needed infos to the cache.
    QHash<QString, QString> pairs;
    if(useCache) {
      QMap<QString, QString> entries = config.entryMap();
      for(QMap<QString, QString>::ConstIterator entry = entries.begin();
          entry != entries.end();
          ++entry)
        pairs[entry.key()] = entry.value();
    }
    else if(parseMetaInformation(*fileit, pairs)) {
      config.changeGroup(group);
      config.writeEntry("last-modified", int(sbuf.st_mtime));
      // iterate keys and save cache
      for(QHash<QString, QString>::ConstIterator item = pairs.begin();
          item != pairs.end();
          ++item)
        config.writeEntry(item.key(), item.value());
    }
    else {
      // parseMetaInformation will have informed the user of the problem
      continue;
    }
    // make sure we have the necessary meta data items we need for proper execution
    KateScriptInformation information;
    information.baseName = baseName;
    information.name = pairs.take("name");
    if(information.name.isNull()) {
      kDebug( 13050 ) << "Script value error: No name specified in script meta data: "
                << qPrintable(*fileit) << '\n';
      continue;
    }
    information.license = pairs.take("license");
    information.author = pairs.take("author");
    information.version = pairs.take("version");
    information.kateVersion = pairs.take("kate-version");
    QString type = pairs.take("type");
    if(type == "indentation") {
      information.type = Kate::IndentationScript;
    }
    else {
      information.type = Kate::UnknownScript;
    }
    // everything else we don't know about explicitly. It goes into other
    information.other = pairs;
    // now, cast accordingly based on type
    switch(information.type) {
      case Kate::IndentationScript: {
        // which languages does this support?
        QString indentLanguages = pairs.take("indent-languages");
        if(!indentLanguages.isNull()) {
          information.indentLanguages = indentLanguages.split(",");
        }
        else {
          information.indentLanguages = QStringList() << information.name;
          kDebug( 13050 ) << "Script value warning: No indent-languages specified for indent "
                    << "script " << qPrintable(*fileit) << ". Using the name ("
                    << qPrintable(information.name) << ")\n";
        }
        // priority?
        bool convertedToInt;
        int priority = pairs.take("priority").toInt(&convertedToInt);
        if(!convertedToInt) {
          kDebug( 13050 ) << "Script value warning: Unexpected or no priority value "
                    << "in: " << qPrintable(*fileit) << ". Setting priority to 0\n";
        }
        information.priority = convertedToInt ? priority : 0;
        KateIndentScript *script = new KateIndentScript(*fileit, information);
        foreach(const QString &language, information.indentLanguages) {
          m_languageToIndenters[language.toLower()].push_back(script);
        }
        m_scripts.push_back(script);

 m_indentationScripts.insert (information.baseName, script);
    m_indentationScriptsList.append(script);
        break;
      }
      case Kate::UnknownScript:
      default:
        kDebug( 13050 ) << "Script value warning: Unknown type ('" << qPrintable(type) << "'): "
                  << qPrintable(*fileit) << '\n';
        m_scripts.push_back(new KateScript(*fileit, information));
    }
  }
 


 // XX Test
  if(indenter("Python")) {
    kDebug( 13050 ) << "Python: " << indenter("Python")->global("triggerCharacters").isValid() << "\n";
    kDebug( 13050 ) << "Python: " << indenter("Python")->function("triggerCharacters").isValid() << "\n";
    kDebug( 13050 ) << "Python: " << indenter("Python")->global("blafldsjfklas").isValid() << "\n";
    kDebug( 13050 ) << "Python: " << indenter("Python")->function("indent").isValid() << "\n";
  }
  if(indenter("C"))
    kDebug( 13050 ) << "C: " << qPrintable(indenter("C")->url()) << "\n";
  if(indenter("lisp"))
    kDebug( 13050 ) << "LISP: " << qPrintable(indenter("Lisp")->url()) << "\n";
  config.sync();
}


bool KateScriptManager::parseMetaInformation(const QString& url,
                                             QHash<QString, QString> &pairs)
{
  // a valid script file -must- have the following format:
  // The first line must contain the string 'kate-script'.
  // All following lines have to have the format 'key : value'. So the value
  // is separated by a colon. Leading non-letter characters are ignored, that
  // include C and C++ comments for example.
  // Parsing the header stops at the first line with no ':'.

  QFile file(QFile::encodeName(url));
  if(!file.open(QIODevice::ReadOnly)) {
    kDebug( 13050 ) << "Script parse error: Cannot open file " << qPrintable(url) << '\n';
    return false;
  }

  kDebug(13050) << "Update script: " << url;
  QTextStream ts(&file);
  ts.setCodec("UTF-8");
  if(!ts.readLine().contains("kate-script")) {
    kDebug( 13050 ) << "Script parse error: No header found in " << qPrintable(url) << '\n';
    file.close();
    return false;
  }

  QString line;
  while(!(line = ts.readLine()).isNull()) {
    int colon = line.indexOf(':');
    if(colon <= 0)
      break; // no colon -> end of header found

    // if -1 then 0. if >= 0, move after star.
    int start = 0; // start points to first letter. idea: skip '*' and '//'
    while(start < line.length() && !line.at(start).isLetter())
      ++start;

    QString key = line.mid(start, colon - start).trimmed();
    QString value = line.right(line.length() - (colon + 1)).trimmed();
    pairs[key] = value;

    kDebug(13050) << "KateScriptManager::parseMetaInformation: found pair: "
                  << "(" << key << " | " << value << ")";
  }
  file.close();
  return true;
}


/// Kate::Command stuff

bool KateScriptManager::exec(KTextEditor::View *view, const QString &_cmd, QString &errorMsg)
{
  QStringList args(_cmd.split(QRegExp("\\s+"), QString::SkipEmptyParts));
  QString cmd(args.first());
  args.removeFirst();
#if 0
  if(!view) {
    errorMsg = i18n("Could not access view");
    return false;
  }


  KateView* kateView = qobject_cast<KateView*>(view);

  if(cmd == QLatin1String("js-run-myself"))
  {
    KateJSInterpreterContext script("");
    return script.evalSource(kateView, kateView->doc()->text(), errorMsg);
  }

  KateJScriptManager::Script *script = m_function2Script.value(cmd);

  if(!script) {
    errorMsg = i18n("Command not found: %1", cmd);
    return false;
  }

  KateJSInterpreterContext *inter = interpreter(script->basename);

  if(!inter)
  {
    errorMsg = i18n("Failed to start interpreter for script %1, command %2", script->basename, cmd);
    return false;
  }

  KJS::List params;

  foreach(const QString &a, args)
    params.append(KJS::jsString(a));

  KJS::JSValue *val = inter->callFunction(kateView, inter->interpreter()->globalObject(), KJS::Identifier(cmd),
                                   params, errorMsg);
#else
  if(!view) {
    errorMsg = i18n("Could not access view");
    return false;
  }
  errorMsg = i18n("Command not found: %1", cmd);
  return false;
#endif
}

bool KateScriptManager::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
#if 0
  if (cmd == "js-run-myself") {
    msg = i18n("This executes the current document as JavaScript within Kate.");
    return true;
  }

  if (!m_scripts.contains(cmd))
    return false;

  msg = m_scripts[cmd]->help;

  return !msg.isEmpty();
#endif
  return true;
}

const QStringList &KateScriptManager::cmds()
{
  static QStringList l;
#if 0
  l.clear();
  l << "js-run-myself";

  QHashIterator<QString, KateJScriptManager::Script*> i(m_function2Script);
  while (i.hasNext()) {
      i.next();
      l << i.key();
  }

#endif
  return l;
}


// kate: space-indent on; indent-width 2; replace-tabs on;
