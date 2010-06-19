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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QMap>
#include <QUuid>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>
#include <kde_file.h>

#include "kateglobal.h"
#include "katecmd.h"

KateScriptManager::KateScriptManager() : QObject(), KTextEditor::Command()
{
  KateCmd::self()->registerCommand (this);

  // false = force (ignore cache)
  collect("katepartscriptrc", "katepart/script/*.js", false);
}

KateScriptManager::~KateScriptManager()
{
  KateCmd::self()->unregisterCommand (this);
  qDeleteAll(m_indentationScripts);
  qDeleteAll(m_commandLineScripts);
}

KateIndentScript *KateScriptManager::indenter(const QString &language)
{
  KateIndentScript *highestPriorityIndenter = 0;
  foreach(KateIndentScript *indenter, m_languageToIndenters.value(language.toLower())) {
    // don't overwrite if there is already a result with a higher priority
    if(highestPriorityIndenter && indenter->indentHeader().priority() < highestPriorityIndenter->indentHeader().priority()) {
#ifdef DEBUG_SCRIPTMANAGER
      kDebug(13050) << "Not overwriting indenter for"
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
    kDebug(13050) << "Found indenter" << highestPriorityIndenter->url() << "for" << language;
  } else {
    kDebug(13050) << "No indenter for" << language;
  }
#endif

  return highestPriorityIndenter;
}

void KateScriptManager::collect(const QString& resourceFile,
                                const QString& directory,
                                bool force)
{
  KConfig cfgFile(resourceFile, KConfig::NoGlobals);

  force = false;
  {
    KConfigGroup config(&cfgFile, "General");
    // If KatePart version does not match, better force a true reload
    if(KateGlobal::katePartVersion() != config.readEntry("kate-version", QString("0.0"))) {
      config.writeEntry("kate-version", KateGlobal::katePartVersion());
      force = true;
    }
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

  // iterate through the files and read info out of cache or file
  foreach(const QString &fileName, list) {
    // get abs filename....
    QFileInfo fi(fileName);
    const QString absPath = fi.absoluteFilePath();
    const QString baseName = fi.baseName ();

    // each file has a group
    const QString group = "Cache "+ fileName;
    KConfigGroup config(&cfgFile, group);

    // stat the file to get the last-modified-time
    KDE_struct_stat sbuf;
    memset(&sbuf, 0, sizeof(sbuf));
    KDE::stat(fileName, &sbuf);

    // check whether file is already cached
    bool useCache = false;
    if(!force && cfgFile.hasGroup(group)) {
      useCache = (sbuf.st_mtime == config.readEntry("last-modified", 0));
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
    KateScriptHeader generalHeader;
    QString type = pairs.take("type");
    if(type == "indentation") {
      generalHeader.setScriptType(Kate::IndentationScript);
    } else if (type == "commands") {
      generalHeader.setScriptType(Kate::CommandLineScript);
    } else {
      kDebug(13050) << "Script value error: No type specified in script meta data: "
                      << qPrintable(fileName) << '\n';
      continue;
    }

    generalHeader.setLicense(pairs.take("license"));
    generalHeader.setAuthor(pairs.take("author"));
    generalHeader.setRevision(pairs.take("revision").toInt());
    generalHeader.setKateVersion(pairs.take("kate-version"));
    generalHeader.setCatalog(pairs.take("i18n-catalog"));

    // now, cast accordingly based on type
    switch(generalHeader.scriptType()) {
      case Kate::IndentationScript: {
        KateIndentScriptHeader indentHeader;
        indentHeader.setName(pairs.take("name"));
        indentHeader.setBaseName(baseName);
        if (indentHeader.name().isNull()) {
          kDebug( 13050 ) << "Script value error: No name specified in script meta data: "
                 << qPrintable(fileName) << '\n' << "-> skipping indenter" << '\n';
          continue;
        }

        // required style?
        indentHeader.setRequiredStyle(pairs.take("required-syntax-style"));
        // which languages does this support?
        QString indentLanguages = pairs.take("indent-languages");
        if(!indentLanguages.isNull()) {
          indentHeader.setIndentLanguages(indentLanguages.split(','));
        }
        else {
          indentHeader.setIndentLanguages(QStringList() << indentHeader.name());

#ifdef DEBUG_SCRIPTMANAGER
          kDebug( 13050 ) << "Script value warning: No indent-languages specified for indent "
                    << "script " << qPrintable(fileName) << ". Using the name ("
                    << qPrintable(indentHeader.name()) << ")\n";
#endif
        }
        // priority?
        bool convertedToInt;
        int priority = pairs.take("priority").toInt(&convertedToInt);

#ifdef DEBUG_SCRIPTMANAGER
        if(!convertedToInt) {
          kDebug( 13050 ) << "Script value warning: Unexpected or no priority value "
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
        commandHeader.setFunctions(pairs.take("functions").split(QRegExp("\\s*,\\s*"), QString::SkipEmptyParts));
        if (commandHeader.functions().isEmpty()) {
          kDebug(13050) << "Script value error: No functions specified in script meta data: "
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
        kDebug( 13050 ) << "Script value warning: Unknown type ('" << qPrintable(type) << "'): "
                  << qPrintable(fileName) << '\n';
    }
  }



#ifdef DEBUG_SCRIPTMANAGER
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

#ifdef DEBUG_SCRIPTMANAGER
    kDebug(13050) << "KateScriptManager::parseMetaInformation: found pair: "
                  << "(" << key << " | " << value << ")";
#endif
  }

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
  } else {
    errorMsg = i18n("Command not found: %1", cmd);
    return false;
  }
}

bool KateScriptManager::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
  Q_UNUSED(view)

  if (cmd == "reload-scripts") {
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
  l << "reload-scripts";

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
    kDebug() << "Destroying template script" << templateScript;
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
