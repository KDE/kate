/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

   Based on KHTML Factory from Simon Hausmann <hausmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include "katefiletype.h"
#include "katefiletype.moc"

#include "katedocument.h"
#include "kateconfig.h"

#include <kconfig.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kdebug.h>

#include <qregexp.h>

KateFileTypeManager::KateFileTypeManager ()
 : m_config (new KConfig ("katepart/filetypesrc"))
{
  m_types.setAutoDelete (true);
  m_typesNum.setAutoDelete (false);

  update ();
}

KateFileTypeManager::~KateFileTypeManager ()
{
  delete m_config;
}

void KateFileTypeManager::update ()
{
  m_config->reparseConfiguration();

  QStringList g (m_config->groupList());

  m_types.clear ();
  m_typesNum.resize (g.count()+1);

  KateFileType *def = new KateFileType ();
  def->number = 0;
  def->highlighting = "Normal";
  m_types.insert (QString ("K"), def);
  m_typesNum.insert (0, def);

  uint i = 1;
  for (uint z=0; z < g.count(); z++)
  {
    m_config->setGroup (g[z]);

    if (!g[z].isEmpty() && m_config->readBoolEntry ("Active"))
    {
      KateFileType *type = new KateFileType ();

      type->number = i;
      type->name = g[z];
      type->section = m_config->readEntry ("Section");
      type->wildcards = m_config->readListEntry ("Wildcards", ';');
      type->mimetypes = m_config->readListEntry ("Mimetypes", ';');
      type->priority = m_config->readNumEntry ("Priority");
      type->highlighting = m_config->readEntry ("Highlighting");

      m_types.insert (QString ("K") + type->name, type);
      m_typesNum.insert (i, type);

      kdDebug(13020) << "INIT LIST: " << type->name << endl;

      i++;
    }
  }

  m_typesNum.resize (m_types.count());
}

bool KateFileTypeManager::exists (const QString &fileType)
{
  return (m_types[QString ("K") + fileType] != 0);
}

bool KateFileTypeManager::isDefault (const QString &fileType)
{
  return fileType.isEmpty();
}

QString KateFileTypeManager::fileType (KateDocument *doc)
{
  if (!doc)
    return QString::null;

  QString fileName = doc->url().prettyURL();
  int length = doc->url().prettyURL().length();

  //
  // first use the wildcards
  //
  static QStringList commonSuffixes = QStringList::split (";", ".orig;.new;~;.bak;.BAK");

  int result;
  if ((result = wildcardsFind(fileName)) != -1)
    return m_typesNum[result]->name;

  QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
  if (fileName.endsWith(backupSuffix)) {
    if ((result = wildcardsFind(fileName.left(length - backupSuffix.length()))) != -1)
      return m_typesNum[result]->name;
  }

  for (QStringList::Iterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
    if (*it != backupSuffix && fileName.endsWith(*it)) {
      if ((result = wildcardsFind(fileName.left(length - (*it).length()))) != -1)
        return m_typesNum[result]->name;
    }
  }

  //
  // now use the KMimeType POWER ;)
  //
  const int HOWMANY = 16384;
  QByteArray buf(HOWMANY);

  int bufpos = 0;
  for (uint i=0; i < doc->numLines(); i++)
  {
    QString line = doc->textLine(i);
    int len = line.length() + 1; // space for a newline - seemingly not required by kmimemagic, but nicer for debugging.

    if (bufpos + len > HOWMANY)
      len = HOWMANY - bufpos;

    memcpy(&buf[bufpos], (line+"\n").latin1(), len);
    bufpos += len;

    if (bufpos >= HOWMANY)
      break;
  }

  int accuracy;
  KMimeType::Ptr mt = KMimeType::findByContent( buf, &accuracy );

  kdDebug (13020) << "Mime type :::)) " << mt->name() << endl;

  QPtrList<KateFileType> types;

  for (uint z=0; z < m_typesNum.size(); z++)
  {
    if (m_typesNum[z]->mimetypes.findIndex (mt->name()) > -1)
      types.append (m_typesNum[z]);
  }

  if ( !types.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    for (KateFileType *type = types.first(); type != 0L; type = types.next())
    {
      if (type->priority > pri)
      {
        pri = type->priority;
        hl = type->number;
      }
    }

    if (hl > -1)
      return m_typesNum[hl]->name;
  }

  return QString::null;
}

int KateFileTypeManager::wildcardsFind (const QString &fileName)
{
  QPtrList<KateFileType> types;

  for (uint z=0; z < m_typesNum.size(); z++)
  {
    for( QStringList::Iterator it = m_typesNum[z]->wildcards.begin(); it != m_typesNum[z]->wildcards.end(); ++it )
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(*it, true, true);
      if ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        types.append (m_typesNum[z]);
    }
  }

  if ( !types.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    for (KateFileType *type = types.first(); type != 0L; type = types.next())
    {
      if (type->priority > pri)
      {
        pri = type->priority;
        hl = type->number;
      }
    }

    return hl;
  }

  return -1;
}

QString KateFileTypeManager::fileType (uint number)
{
  return ((number < m_typesNum.size()) ? m_typesNum[number]->name : QString::null);
}

KateFileType *KateFileTypeManager::fileType (const QString &name)
{
  return m_types[QString ("K") + name];
}

KateFileTypeConfigTab::KateFileTypeConfigTab( QWidget *parent )
  : Kate::ConfigPage( parent )
{
  reload();
}

void KateFileTypeConfigTab::apply()
{
}

void KateFileTypeConfigTab::reload()
{
}

void KateFileTypeConfigTab::reset()
{
}

void KateFileTypeConfigTab::defaults()
{
}

// kate: space-indent on; indent-width 2; replace-tabs on;
