// This file is part of the KDE libraries
// Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
// Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
// Copyright (C) 2006, 2009 Dominik Haumann <dhaumann kde org>
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>
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

#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QMap>
#include <QUuid>

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>

#include "kateglobal.h"
#include "katecmd.h"
#include "katepartdebug.h"

KateScriptManager* KateScriptManager::m_instance = 0;

KateScriptManager::KateScriptManager() : QObject(), KTextEditor::Command()
{
  KateCmd::self()->registerCommand (this);

  // use cached info
  collect();
}

KateScriptManager::~KateScriptManager()
{
  KateCmd::self()->unregisterCommand (this);
  qDeleteAll(m_indentationScripts);
  qDeleteAll(m_commandLineScripts);
  m_instance = 0;
}

KateIndentScript *KateScriptManager::indenter(const QString &language)
{
  KateIndentScript *highestPriorityIndenter = 0;
  foreach(KateIndentScript *indenter, m_languageToIndenters.value(language.toLower())) {
    // don't overwrite if there is already a result with a higher priority
    if(highestPriorityIndenter && indenter->indentHeader().priority() < highestPriorityIndenter->indentHeader().priority()) {
#ifdef DEBUG_SCRIPTMANAGER
      qCDebug(LOG_PART) << "Not overwriting indenter for"
                    << language << "as the priority isn't big enough (" <<
                    indenter->indentHeader().priority() << '<'
                    << highestPriorityIndenter->indentHeader().priority() << ')';
#endif
    }
    else {
      highestPriorityIndenter = indenter;
    }
  }

#ifdef DEBUG_SCRIPTMANAGER
  if(highestPriorityIndenter) {
    qCDebug(LOG_PART) << "Found indenter" << highestPriorityIndenter->url() << "for" << language;
  } else {
    qCDebug(LOG_PART) << "No indenter for" << language;
  }
#endif

  return highestPriorityIndenter;
}

void KateScriptManager::collect(bool force)
{
  // local information cache
  KConfig cfgFile(QLatin1String("katescriptingrc"), KConfig::NoGlobals);

  // we might need to enforce reload!
  {
    KConfigGroup config(&cfgFile, QLatin1String("General"));
    // If KatePart version does not match, better force a true reload
    if(QLatin1String(KATE_VERSION) != config.readEntry(QLatin1String("kate-version"))) {
      config.writeEntry(QLatin1String("kate-version"), KATE_VERSION);
      force = true;
    }
  }

  // clear out the old scripts and reserve enough space
  qDeleteAll(m_indentationScripts);
  qDeleteAll(m_commandLineScripts);
  m_indentationScripts.clear();
  m_commandLineScripts.clear();

  m_languageToIndenters.clear();
  m_indentationScriptMap.clear();
  
  /**
   * now, we search all kinds of known scripts
   */
  foreach (const QString &type, QStringList () << QLatin1String("indentation") << QLatin1String("commands")) {
    // get a list of all unique .js files for the current type
    const QString basedir = QLatin1String("katepart/script/") + type;
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, basedir, QStandardPaths::LocateDirectory);

    QStringList list;

    foreach (const QString& dir, dirs) {
      const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.js"));
      foreach (const QString& file, fileNames) {
        list.append(dir + QLatin1Char('/') + file);
      }
    }
    
    // iterate through the files and read info out of cache or file
    foreach(const QString &fileName, list) {
      // get abs filename....
      QFileInfo fi(fileName);
      const QString absPath = fi.absoluteFilePath();
      const QString baseName = fi.baseName ();

      // each file has a group
      const QString group(QLatin1String("Cache ") + fileName);
      KConfigGroup config(&cfgFile, group);

      // stat the file to get the last-modified-time
      const int lastModified = QFileInfo(fileName).lastModified().toTime_t();

      // check whether file is already cached
      bool useCache = false;
      if(!force && cfgFile.hasGroup(group)) {
        useCache = (lastModified == config.readEntry("last-modified", 0));
      }

      // read key/value pairs from the cached file if possible
      // otherwise, parse it and then save the needed information to the cache.
      QHash<QString, QString> pairs;
      if(useCache) {
        const QMap<QString, QString> entries = config.entryMap();
        for(QMap<QString, QString>::ConstIterator entry = entries.constBegin();
            entry != entries.constEnd();
            ++entry)
          pairs[entry.key()] = entry.value();
      }
      else if(parseMetaInformation(fileName, pairs)) {
        config.writeEntry("last-modified", lastModified);
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
      
      /**
       * remember type
       */
      KateScriptHeader generalHeader;
      if(type == QLatin1String("indentation")) {
        generalHeader.setScriptType(Kate::IndentationScript);
      } else if (type == QLatin1String("commands")) {
        generalHeader.setScriptType(Kate::CommandLineScript);
      } else {
        // should never happen, we dictate type by directory
        Q_ASSERT (false);
      }

      generalHeader.setLicense(pairs.take(QLatin1String("license")));
      generalHeader.setAuthor(pairs.take(QLatin1String("author")));
      generalHeader.setRevision(pairs.take(QLatin1String("revision")).toInt());
      generalHeader.setKateVersion(pairs.take(QLatin1String("kate-version")));
      generalHeader.setCatalog(pairs.take(QLatin1String("i18n-catalog")));

      // now, cast accordingly based on type
      switch(generalHeader.scriptType()) {
        case Kate::IndentationScript: {
          KateIndentScriptHeader indentHeader;
          indentHeader.setName(pairs.take(QLatin1String("name")));
          indentHeader.setBaseName(baseName);
          if (indentHeader.name().isNull()) {
            qCDebug(LOG_PART) << "Script value error: No name specified in script meta data: "
                  << qPrintable(fileName) << '\n' << "-> skipping indenter" << '\n';
            continue;
          }

          // required style?
          indentHeader.setRequiredStyle(pairs.take(QLatin1String("required-syntax-style")));
          // which languages does this support?
          QString indentLanguages = pairs.take(QLatin1String("indent-languages"));
          if(!indentLanguages.isNull()) {
            indentHeader.setIndentLanguages(indentLanguages.split(QLatin1Char(',')));
          }
          else {
            indentHeader.setIndentLanguages(QStringList() << indentHeader.name());

  #ifdef DEBUG_SCRIPTMANAGER
            qCDebug(LOG_PART) << "Script value warning: No indent-languages specified for indent "
                      << "script " << qPrintable(fileName) << ". Using the name ("
                      << qPrintable(indentHeader.name()) << ")\n";
  #endif
          }
          // priority?
          bool convertedToInt;
          int priority = pairs.take(QLatin1String("priority")).toInt(&convertedToInt);

  #ifdef DEBUG_SCRIPTMANAGER
          if(!convertedToInt) {
            qCDebug(LOG_PART) << "Script value warning: Unexpected or no priority value "
                      << "in: " << qPrintable(fileName) << ". Setting priority to 0\n";
          }
  #endif

          indentHeader.setPriority(convertedToInt ? priority : 0);
          KateIndentScript *script = new KateIndentScript(fileName, indentHeader);
          script->setGeneralHeader(generalHeader);
          foreach(const QString &language, indentHeader.indentLanguages()) {
            m_languageToIndenters[language.toLower()].push_back(script);
          }

          m_indentationScriptMap.insert(indentHeader.baseName(), script);
          m_indentationScripts.append(script);
          break;
        }
        case Kate::CommandLineScript: {
          KateCommandLineScriptHeader commandHeader;
          commandHeader.setFunctions(pairs.take(QLatin1String("functions")).split(QRegExp(QLatin1String("\\s*,\\s*")), QString::SkipEmptyParts));
          if (commandHeader.functions().isEmpty()) {
            qCDebug(LOG_PART) << "Script value error: No functions specified in script meta data: "
                          << qPrintable(fileName) << '\n' << "-> skipping script" << '\n';
            continue;
          }
          KateCommandLineScript* script = new KateCommandLineScript(fileName, commandHeader);
          script->setGeneralHeader(generalHeader);
          m_commandLineScripts.push_back(script);
          break;
        }
        case Kate::UnknownScript:
        default:
          qCDebug(LOG_PART) << "Script value warning: Unknown type ('" << qPrintable(type) << "'): "
                    << qPrintable(fileName) << '\n';
      }
    }
  }



#ifdef DEBUG_SCRIPTMANAGER
 // XX Test
  if(indenter("Python")) {
    qCDebug(LOG_PART) << "Python: " << indenter("Python")->global("triggerCharacters").isValid() << "\n";
    qCDebug(LOG_PART) << "Python: " << indenter("Python")->function("triggerCharacters").isValid() << "\n";
    qCDebug(LOG_PART) << "Python: " << indenter("Python")->global("blafldsjfklas").isValid() << "\n";
    qCDebug(LOG_PART) << "Python: " << indenter("Python")->function("indent").isValid() << "\n";
  }
  if(indenter("C"))
    qCDebug(LOG_PART) << "C: " << qPrintable(indenter("C")->url()) << "\n";
  if(indenter("lisp"))
    qCDebug(LOG_PART) << "LISP: " << qPrintable(indenter("Lisp")->url()) << "\n";
#endif

  cfgFile.sync();
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
    qCDebug(LOG_PART) << "Script parse error: Cannot open file " << qPrintable(url) << '\n';
    return false;
  }

  qCDebug(LOG_PART) << "Update script: " << url;
  QTextStream ts(&file);
  ts.setCodec("UTF-8");
  if(!ts.readLine().contains(QLatin1String("kate-script"))) {
    qCDebug(LOG_PART) << "Script parse error: No header found in " << qPrintable(url) << '\n';
    file.close();
    return false;
  }

  QString line;
  while(!(line = ts.readLine()).isNull()) {
    int colon = line.indexOf(QLatin1Char(':'));
    if(colon <= 0)
      break; // no colon -> end of header found

    // if -1 then 0. if >= 0, move after star.
    int start = 0; // start points to first letter. idea: skip '*' and '//'
    while(start < line.length() && !line.at(start).isLetter())
      ++start;

    QString key = line.mid(start, colon - start).trimmed();
    QString value = line.right(line.length() - (colon + 1)).trimmed();
    pairs[key] = value;

#ifdef DEBUG_SCRIPTMANAGER
    qCDebug(LOG_PART) << "KateScriptManager::parseMetaInformation: found pair: "
                  << "(" << key << " | " << value << ")";
#endif
  }

  file.close();
  return true;
}

void KateScriptManager::reload()
{
  collect(true);
  emit reloaded();
}

/// Kate::Command stuff

bool KateScriptManager::exec(KTextEditor::View *view, const QString &_cmd, QString &errorMsg)
{
  QStringList args(_cmd.split(QRegExp(QLatin1String("\\s+")), QString::SkipEmptyParts));
  QString cmd(args.first());
  args.removeFirst();

  if(!view) {
    errorMsg = i18n("Could not access view");
    return false;
  }

  if (cmd == QLatin1String("reload-scripts")) {
    reload();
    return true;
  } else {
    errorMsg = i18n("Command not found: %1", cmd);
    return false;
  }
}

bool KateScriptManager::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
  Q_UNUSED(view)

  if (cmd == QLatin1String("reload-scripts")) {
    msg = i18n("Reload all JavaScript files (indenters, command line scripts, etc).");
    return true;
  } else {
    msg = i18n("Command not found: %1", cmd);
    return false;
  }
}

const QStringList &KateScriptManager::cmds()
{
  static QStringList l;

  l.clear();
  l << QLatin1String("reload-scripts");

  return l;
}



KTextEditor::TemplateScript* KateScriptManager::registerTemplateScript (QObject* owner, const QString& script)
{
  KateTemplateScript* kts = new KateTemplateScript(script);
  m_templateScripts.append(kts);

  connect(owner, SIGNAL(destroyed(QObject*)),
          this, SLOT(slotTemplateScriptOwnerDestroyed(QObject*)));
  m_ownerScript.insertMulti(owner, kts);
  return kts;
}

void KateScriptManager::unregisterTemplateScript(KTextEditor::TemplateScript* templateScript)
{
  int index = m_templateScripts.indexOf(templateScript);
  if (index != -1) {
    QObject* k = m_ownerScript.key(templateScript);
    m_ownerScript.remove(k, templateScript);
    m_templateScripts.removeAt(index);
    delete templateScript;
  }
}

void KateScriptManager::slotTemplateScriptOwnerDestroyed(QObject* owner)
{
  while (m_ownerScript.contains(owner)) {
    KTextEditor::TemplateScript* templateScript = m_ownerScript.take(owner);
    qCDebug(LOG_PART) << "Destroying template script" << templateScript;
    m_templateScripts.removeAll(templateScript);
    delete templateScript;
  }
}

KateTemplateScript* KateScriptManager::templateScript(KTextEditor::TemplateScript* templateScript)
{
  if (m_templateScripts.contains(templateScript)) {
    return static_cast<KateTemplateScript*>(templateScript);
  }

  return 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
