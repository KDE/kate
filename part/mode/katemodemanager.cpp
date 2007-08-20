/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katemodemanager.h"
#include "katemodemanager.moc"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"
#include "katesyntaxmanager.h"
#include "katesyntaxdocument.h"

#include "ui_filetypeconfigwidget.h"

#include <kconfig.h>
#include <kmimetype.h>
#include <kmimetypechooser.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <knuminput.h>
#include <klocale.h>
#include <kmenu.h>

#include <QtCore/QRegExp>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <kvbox.h>

#define KATE_FT_HOWMANY 1024
//END Includes

KateModeManager::KateModeManager ()
{
  update ();
}

KateModeManager::~KateModeManager ()
{
  qDeleteAll (m_types);
}

//
// read the types from config file and update the internal list
//
void KateModeManager::update ()
{
  KConfig config ("katemoderc", KConfig::NoGlobals);

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
    type->section = cg.readEntry ("Section");
    type->wildcards = cg.readEntry ("Wildcards", QStringList(), ';');
    type->mimetypes = cg.readEntry ("Mimetypes", QStringList(), ';');
    type->priority = cg.readEntry ("Priority", 0);
    type->varLine = cg.readEntry ("Variables");
    
    type->hl = cg.readEntry ("Highlighting");
   
    // only for generated types...
    type->hlGenerated = cg.readEntry ("Highlighting Generated", false);
    type->version = cg.readEntry ("Highlighting Version");
 
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
      m_types.append (type);
      m_name2Type.insert (type->name, type);
    }

    if (newType || type->version != modes[i]->version)
    {
      type->name = modes[i]->name;
      type->section = modes[i]->section;
      type->wildcards = modes[i]->extension.split (';', QString::SkipEmptyParts);
      type->mimetypes = modes[i]->mimetype.split (';', QString::SkipEmptyParts);
      type->priority = modes[i]->priority.toInt();
      type->version = modes[i]->version;
      type->hl = modes[i]->name;
      type->hlGenerated = true;
    }
  }

  // sort the list...
  QList<KateFileType *> newList;  
  for (int i=0; i < m_types.count(); i++)
  {
    KateFileType *t = m_types[i];

    int insert = 0;
    for (; insert <= newList.count(); insert++)
    {
      if (insert == newList.count())
        break;

      if ( QString(newList.at(insert)->section + newList.at(insert)->name).toLower()
            > QString(t->section + t->name).toLower() )
        break;
    }

    newList.insert (insert, t);
  }

  // replace old list with new sorted list...
  m_types = newList;

  // add the none type...
  KateFileType *t = new KateFileType ();
  t->name = "Normal";
  t->hl = "None";
  t->hlGenerated = true;

  m_types.prepend (t);
}

//
// save the given list to config file + update
//
void KateModeManager::save (const QList<KateFileType *>& v)
{
  KConfig katerc("katemoderc", KConfig::NoGlobals);
  KConfigGroup config(&katerc, QString());

  QStringList newg;
  foreach (const KateFileType *type, v)
  {
    config.changeGroup(type->name);

    config.writeEntry ("Section", type->section);
    config.writeEntry ("Wildcards", type->wildcards, ';');
    config.writeEntry ("Mimetypes", type->mimetypes, ';');
    config.writeEntry ("Priority", type->priority);

    QString varLine = type->varLine;
    if (QRegExp("kate:(.*)").indexIn(varLine) < 0)
      varLine.prepend ("kate: ");

    config.writeEntry ("Variables", varLine);
    
    config.writeEntry ("Highlighting", type->hl);
    
    // only for generated types...
    config.writeEntry ("Highlighting Generated", type->hlGenerated);
    config.writeEntry ("Highlighting Version", type->version);

    newg << type->name;
  }

  QStringList g (katerc.groupList());

  for (int z=0; z < g.count(); z++)
  {
    if (newg.indexOf (g[z]) == -1)
    {
      katerc.deleteGroup (g[z]);
    }
  }

  config.sync ();

  update ();
}

QString KateModeManager::fileType (KateDocument *doc)
{
  kDebug(13020);
  if (!doc)
    return "";

  if (m_types.isEmpty())
    return "";

  QString fileName = doc->url().prettyUrl();
  int length = doc->url().prettyUrl().length();

  QString result;

  // Try wildcards
  if ( ! fileName.isEmpty() )
  {
    static QStringList commonSuffixes = QString(".orig;.new;~;.bak;.BAK").split (";");

    if (!(result = wildcardsFind(fileName)).isEmpty())
      return result;

    QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
    if (fileName.endsWith(backupSuffix)) {
      if (!(result = wildcardsFind(fileName.left(length - backupSuffix.length()))).isEmpty())
        return result;
    }

    for (QStringList::Iterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
      if (*it != backupSuffix && fileName.endsWith(*it)) {
        if (!(result = wildcardsFind(fileName.left(length - (*it).length()))).isEmpty())
          return result;
      }
    }
  }

  // Try content-based mimetype
  KMimeType::Ptr mt = doc->mimeTypeForContent();

  QList<KateFileType*> types;

  foreach (KateFileType *type, m_types)
  {
    if (type->mimetypes.indexOf (mt->name()) > -1)
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


  return "";
}

QString KateModeManager::wildcardsFind (const QString &fileName)
{
  QList<KateFileType*> types;

  foreach (KateFileType *type, m_types)
  {
    foreach (QString wildcard, type->wildcards)
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(wildcard, Qt::CaseSensitive, QRegExp::Wildcard);
      if ( ( re.indexIn( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        types.append (type);
    }
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

  return "";
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
