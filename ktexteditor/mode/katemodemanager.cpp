/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
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

//BEGIN Includes
#include "katemodemanager.h"
#include "katewildcardmatcher.h"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "katesyntaxdocument.h"
#include "katepartdebug.h"

#include "ui_filetypeconfigwidget.h"

#include <KMimeTypeChooser>
#include <KIconLoader>
#include <KConfigGroup>

#include <QRegExp>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QToolButton>
#include <QMimeDatabase>
//END Includes

KateModeManager::KateModeManager ()
{
  update ();
}

KateModeManager::~KateModeManager ()
{
  qDeleteAll (m_types);
}

bool compareKateFileType(const KateFileType* const left, const KateFileType* const right)
{
  int comparison = left->section.compare(right->section, Qt::CaseInsensitive);
  if (comparison == 0) {
    comparison = left->name.compare(right->name, Qt::CaseInsensitive);
  }
  return comparison < 0;
}

//
// read the types from config file and update the internal list
//
void KateModeManager::update ()
{
  KConfig config (QLatin1String("katemoderc"), KConfig::NoGlobals);

  QStringList g (config.groupList());

  qDeleteAll (m_types);
  m_types.clear ();
  m_name2Type.clear ();
  for (int z=0; z < g.count(); z++)
  {
    KConfigGroup cg(&config, g[z]);

    KateFileType *type = new KateFileType ();
    type->number = z;
    type->name = g[z];
    type->section = cg.readEntry (QLatin1String("Section"));
    type->wildcards = cg.readXdgListEntry (QLatin1String("Wildcards"));
    type->mimetypes = cg.readXdgListEntry (QLatin1String("Mimetypes"));
    type->priority = cg.readEntry (QLatin1String("Priority"), 0);
    type->varLine = cg.readEntry (QLatin1String("Variables"));
    type->indenter = cg.readEntry (QLatin1String("Indenter"));
    
    type->hl = cg.readEntry (QLatin1String("Highlighting"));
   
    // only for generated types...
    type->hlGenerated = cg.readEntry (QLatin1String("Highlighting Generated"), false);
    type->version = cg.readEntry (QLatin1String("Highlighting Version"));
 
    // insert into the list + hash...
    m_types.append(type);
    m_name2Type.insert (type->name, type);
  }
  
  // try if the hl stuff is up to date...
  const KateSyntaxModeList &modes = KateHlManager::self()->syntaxDocument()->modeList();
  for (int i = 0; i < modes.size(); ++i)
  {
    KateFileType *type = 0;
    bool newType = false;
    if (m_name2Type.contains (modes[i]->name))
      type = m_name2Type[modes[i]->name];
    else
    {
      newType = true;
      type = new KateFileType ();
      type->name = modes[i]->name;
      type->priority = 0;
      m_types.append (type);
      m_name2Type.insert (type->name, type);
    }

    if (newType || type->version != modes[i]->version)
    {
      type->name = modes[i]->name;
      type->section = modes[i]->section;
      type->wildcards = modes[i]->extension.split (QLatin1Char(';'), QString::SkipEmptyParts);
      type->mimetypes = modes[i]->mimetype.split (QLatin1Char(';'), QString::SkipEmptyParts);
      type->priority = modes[i]->priority.toInt();
      type->version = modes[i]->version;
      type->indenter = modes[i]->indenter;
      type->hl = modes[i]->name;
      type->hlGenerated = true;
    }
  }

  // sort the list...
  qSort(m_types.begin(), m_types.end(), compareKateFileType);

  // add the none type...
  KateFileType *t = new KateFileType ();
  
  // DO NOT TRANSLATE THIS, DONE LATER, marked by hlGenerated
  t->name = QLatin1String("Normal");
  t->hl = QLatin1String("None");
  t->hlGenerated = true;

  m_types.prepend (t);
}

//
// save the given list to config file + update
//
void KateModeManager::save (const QList<KateFileType *>& v)
{
  KConfig katerc(QLatin1String("katemoderc"), KConfig::NoGlobals);

  QStringList newg;
  foreach (const KateFileType *type, v)
  {
    KConfigGroup config(&katerc, type->name);

    config.writeEntry ("Section", type->section);
    config.writeXdgListEntry ("Wildcards", type->wildcards);
    config.writeXdgListEntry ("Mimetypes", type->mimetypes);
    config.writeEntry ("Priority", type->priority);
    config.writeEntry ("Indenter", type->indenter);

    QString varLine = type->varLine;
    if (QRegExp(QLatin1String("kate:(.*)")).indexIn(varLine) < 0)
      varLine.prepend (QLatin1String("kate: "));

    config.writeEntry ("Variables", varLine);
    
    config.writeEntry ("Highlighting", type->hl);
    
    // only for generated types...
    config.writeEntry ("Highlighting Generated", type->hlGenerated);
    config.writeEntry ("Highlighting Version", type->version);

    newg << type->name;
  }

  foreach (const QString &groupName, katerc.groupList())
  {
    if (newg.indexOf (groupName) == -1)
    {
      katerc.deleteGroup (groupName);
    }
  }

  katerc.sync ();

  update ();
}

QString KateModeManager::fileType (KateDocument *doc, const QString &fileToReadFrom)
{
  qCDebug(LOG_PART);
  if (!doc)
    return QString();

  if (m_types.isEmpty())
    return QString();

  QString fileName = doc->url().toString();
  int length = doc->url().toString().length();

  QString result;

  // Try wildcards
  if ( ! fileName.isEmpty() )
  {
    static const QStringList commonSuffixes = QString::fromLatin1(".orig;.new;~;.bak;.BAK").split (QLatin1Char(';'));

    if (!(result = wildcardsFind(fileName)).isEmpty())
      return result;

    QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
    if (fileName.endsWith(backupSuffix)) {
      if (!(result = wildcardsFind(fileName.left(length - backupSuffix.length()))).isEmpty())
        return result;
    }

    for (QStringList::ConstIterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
      if (*it != backupSuffix && fileName.endsWith(*it)) {
        if (!(result = wildcardsFind(fileName.left(length - (*it).length()))).isEmpty())
          return result;
      }
    }
  }

  // Try content-based mimetype
  QMimeType mt;
  QMimeDatabase db;

  if ( !fileToReadFrom.isEmpty() )
    mt = db.mimeTypeForFile( fileToReadFrom );
  else
    mt = doc->mimeTypeForContent();

  QList<KateFileType*> types;

  foreach (KateFileType *type, m_types)
  {
    if (type->mimetypes.indexOf (mt.name()) > -1)
      types.append (type);
  }

  if ( !types.isEmpty() )
  {
    int pri = -1;
    QString name;

    foreach (KateFileType *type, types)
    {
      if (type->priority > pri)
      {
        pri = type->priority;
        name = type->name;
      }
    }

    return name;
  }


  return QString();
}

QString KateModeManager::wildcardsFind (const QString &fileName)
{
  KateFileType * match = NULL;
  int minPrio = -1;
  foreach (KateFileType *type, m_types)
  {
    if (type->priority <= minPrio) {
      continue;
    }

    foreach (const QString &wildcard, type->wildcards)
    {
      if (KateWildcardMatcher::exactMatch(fileName, wildcard)) {
        match = type;
        minPrio = type->priority;
        break;
      }
    }
  }

  return (match == NULL) ? QString() : match->name;
}

const KateFileType& KateModeManager::fileType(const QString &name) const
{
  for (int i = 0; i < m_types.size(); ++i)
    if (m_types[i]->name == name)
      return *m_types[i];
      
  static KateFileType notype;
  return notype;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
