// This file is part of the KDE libraries
// Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
// Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
// Copyright (C) 2006, 2009 Dominik Haumann <dhaumann kde org>
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License version 2 as published by the Free Software Foundation.
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

#include "katescriptmanager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QMap>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>
#include <kde_file.h>

#include "kateglobal.h"

KateScriptManager::KateScriptManager() : QObject(), KTextEditor::Command()
{
  // false = force (ignore cache)
  collect("katepartscriptrc", "katepart/script/*.js", false);
}

KateScriptManager::~KateScriptManager()
{
  qDeleteAll(m_indentationScripts);
  qDeleteAll(m_commandLineScripts);
}

KateIndentScript *KateScriptManager::indenter(const QString &language)
{
  KateIndentScript *highestPriorityIndenter = 0;
  foreach(KateIndentScript *indenter, m_languageToIndenters.value(language.toLower())) {
    // don't overwrite if there is already a result with a higher priority
    if(highestPriorityIndenter && indenter->header().priority() < highestPriorityIndenter->header().priority()) {
      kDebug(13050) << "Not overwriting indenter for"
                    << language << "as the priority isn't big enough (" <<
                    indenter->header().priority() << '<'
                    << highestPriorityIndenter->header().priority() << ')';
    }
    else {
      highestPriorityIndenter = indenter;
    }
  }
  if(highestPriorityIndenter) {
    kDebug(13050) << "Found indenter" << highestPriorityIndenter->url() << "for" << language;
  } else {
    kDebug(13050) << "No indenter for" << language;
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
  if(KateGlobal::katePartVersion() != config.readEntry("kate-version", QString("0.0"))) {
    config.writeEntry("kate-version", KateGlobal::katePartVersion());
    force = true;
  }
  // get a list of all .js files
  const QStringList list = KGlobal::dirs()->findAllResources("data", directory, KStandardDirs::NoDuplicates);
  // clear out the old scripts and reserve enough space
  qDeleteAll(m_indentationScripts);
  qDeleteAll(m_commandLineScripts);
  m_indentationScripts.clear();
  m_commandLineScripts.clear();

  m_languageToIndenters.clear();
  m_indentationScriptMap.clear();
  m_commandLineScriptMap.clear();

  // iterate through the files and read info out of cache or file
  for(QStringList::ConstIterator fileit = list.begin(); fileit != list.end(); ++fileit) {
    // get abs filename....
    QFileInfo fi(*fileit);
    const QString absPath = fi.absoluteFilePath();
    const QString baseName = fi.baseName ();

    // each file has a group
    QString group = "Cache "+ *fileit;
    config.changeGroup(group);

    // stat the file to get the last-modified-time
    KDE_struct_stat sbuf;
    memset(&sbuf, 0, sizeof(sbuf));
    KDE::stat(*fileit, &sbuf);

    // check whether file is already cached
    bool useCache = false;
    if(!force && cfgFile.hasGroup(group)) {
      useCache = (sbuf.st_mtime == config.readEntry("last-modified", 0));
    }

    // read key/value pairs from the cached file if possible
    // otherwise, parse it and then save the needed infos to the cache.
    QHash<QString, QString> pairs;
    if(useCache) {
      const QMap<QString, QString> entries = config.entryMap();
      for(QMap<QString, QString>::ConstIterator entry = entries.constBegin();
          entry != entries.constEnd();
          ++entry)
        pairs[entry.key()] = entry.value();
    }
    else if(parseMetaInformation(*fileit, pairs)) {
      config.changeGroup(group);
      config.writeEntry("last-modified", int(sbuf.st_mtime));
      // iterate keys and save cache
      for(QHash<QString, QString>::ConstIterator item = pairs.constBegin();
          item != pairs.constEnd();
          ++item)
        config.writeEntry(item.key(), item.value());
    }
    else {
      // parseMetaInformation will have informed the user of the problem
      continue;
    }
    // make sure we have the necessary meta data items we need for proper execution
    KateScriptHeader commonHeader;
    QString type = pairs.take("type");
    if(type == "indentation") {
      commonHeader.setScriptType(Kate::IndentationScript);
    } else if (type == "commands") {
      commonHeader.setScriptType(Kate::CommandLineScript);
    } else {
      kDebug(13050) << "Script value error: No type specified in script meta data: "
                      << qPrintable(*fileit) << '\n';
      continue;
    }

    commonHeader.setLicense(pairs.take("license"));
    commonHeader.setAuthor(pairs.take("author"));
    commonHeader.setRevision(pairs.take("revision").toInt());
    commonHeader.setKateVersion(pairs.take("kate-version"));

    // now, cast accordingly based on type
    switch(commonHeader.scriptType()) {
      case Kate::IndentationScript: {
        KateIndentScriptHeader header(commonHeader);
        header.setName(pairs.take("name"));
        header.setBaseName(baseName);
        if (header.name().isNull()) {
          kDebug( 13050 ) << "Script value error: No name specified in script meta data: "
                 << qPrintable(*fileit) << '\n' << "-> skipping indenter" << '\n';
          continue;
        }

        // required style?
        header.setRequiredStyle(pairs.take("required-syntax-style"));
        // which languages does this support?
        QString indentLanguages = pairs.take("indent-languages");
        if(!indentLanguages.isNull()) {
          header.setIndentLanguages(indentLanguages.split(','));
        }
        else {
          header.setIndentLanguages(QStringList() << header.name());
          kDebug( 13050 ) << "Script value warning: No indent-languages specified for indent "
                    << "script " << qPrintable(*fileit) << ". Using the name ("
                    << qPrintable(header.name()) << ")\n";
        }
        // priority?
        bool convertedToInt;
        int priority = pairs.take("priority").toInt(&convertedToInt);
        if(!convertedToInt) {
          kDebug( 13050 ) << "Script value warning: Unexpected or no priority value "
                    << "in: " << qPrintable(*fileit) << ". Setting priority to 0\n";
        }
        header.setPriority(convertedToInt ? priority : 0);
        KateIndentScript *script = new KateIndentScript(*fileit, header);
        foreach(const QString &language, header.indentLanguages()) {
          m_languageToIndenters[language.toLower()].push_back(script);
        }

        m_indentationScriptMap.insert(header.baseName(), script);
        m_indentationScripts.append(script);
        break;
      }
      case Kate::CommandLineScript: {
        KateCommandLineScriptHeader header(commonHeader);
        header.setFunctions(pairs.take("functions").split(QRegExp("\\s*,\\s*"), QString::SkipEmptyParts));
        if (header.functions().isEmpty()) {
          kDebug(13050) << "Script value error: No functions specified in script meta data: "
                        << qPrintable(*fileit) << '\n' << "-> skipping script" << '\n';
          continue;
        }
        KateCommandLineScript* script = new KateCommandLineScript(*fileit, header);
        foreach (const QString function, header.functions()) {
          m_commandLineScriptMap.insert(function, script);
        }
        m_commandLineScripts.push_back(script);
        break;
      }
      case Kate::UnknownScript:
      default:
        kDebug( 13050 ) << "Script value warning: Unknown type ('" << qPrintable(type) << "'): "
                  << qPrintable(*fileit) << '\n';
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

  QFile file(url);
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

void KateScriptManager::reload()
{
  KateScript::reloadScriptingApi();
  collect("katepartscriptrc", "katepart/script/*.js", true);
  emit reloaded();
}

/// Kate::Command stuff

bool KateScriptManager::exec(KTextEditor::View *view, const QString &_cmd, QString &errorMsg)
{
  QStringList args(_cmd.split(QRegExp("\\s+"), QString::SkipEmptyParts));
  QString cmd(args.first());
  args.removeFirst();

  if(!view) {
    errorMsg = i18n("Could not access view");
    return false;
  }

  if (cmd == "reload-scripts") {
    reload();
    return true;
  } else if (!m_commandLineScriptMap.contains(cmd)) {
    errorMsg = i18n("Command not found: %1", cmd);
    return false;
  }
  KateCommandLineScript *script = m_commandLineScriptMap[cmd];
  script->setView(qobject_cast<KateView*>(view));

  return script->callFunction(cmd, args, errorMsg);
}

bool KateScriptManager::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
//   if (cmd == "run-buffer") {
//     msg = i18n("This executes the current document or selection as JavaScript within Kate.");
//     return true;
//   }

  if (cmd == "reload-scripts") {
    msg = i18n("Reload all JavaScript files (indenters, command line scripts, etc).");
    return true;
  } else if (!m_commandLineScriptMap.contains(cmd)) {
    msg = i18n("Command not found: %1", cmd);
    return false;
  }

  KateCommandLineScript *script = m_commandLineScriptMap[cmd];
  return script->help(view, cmd, msg);
}

const QStringList &KateScriptManager::cmds()
{
  static QStringList l;

  l.clear();
//   l << "run-buffer"; // not implemented right now
  l << "reload-scripts";

  QVector<KateCommandLineScript*>::ConstIterator it = m_commandLineScripts.constBegin();
  for ( ; it != m_commandLineScripts.constEnd(); ++it) {
    l << (*it)->header().functions();
  }

  return l;
}


// kate: space-indent on; indent-width 2; replace-tabs on;
