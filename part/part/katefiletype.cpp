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
  m_types.resize (g.count());

  for (uint z=0; z < g.count(); z++)
  {
    m_config->setGroup (g[z]);

    KateFileType *type = new KateFileType ();

    type->number = z;
    type->name = g[z];
    type->section = m_config->readEntry ("Section");
    type->wildcards = m_config->readListEntry ("Wildcards", ';');
    type->mimetypes = m_config->readListEntry ("Mimetypes", ';');
    type->priority = m_config->readNumEntry ("Priority");

    m_types.insert (z, type);

    kdDebug(13020) << "INIT LIST: " << type->name << endl;
  }
}

int KateFileTypeManager::fileType (KateDocument *doc)
{
  if (!doc)
    return -1;

  QString fileName = doc->url().prettyURL();
  int length = doc->url().prettyURL().length();

  //
  // first use the wildcards
  //
  static QStringList commonSuffixes = QStringList::split (";", ".orig;.new;~;.bak;.BAK");

  int result;
  if ((result = wildcardsFind(fileName)) != -1)
    return result;

  QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
  if (fileName.endsWith(backupSuffix)) {
    if ((result = wildcardsFind(fileName.left(length - backupSuffix.length()))) != -1)
      return result;
  }

  for (QStringList::Iterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
    if (*it != backupSuffix && fileName.endsWith(*it)) {
      if ((result = wildcardsFind(fileName.left(length - (*it).length()))) != -1)
        return result;
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

  for (uint z=0; z < m_types.size(); z++)
  {
    if (m_types[z]->mimetypes.findIndex (mt->name()) > -1)
      types.append (m_types[z]);
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

int KateFileTypeManager::wildcardsFind (const QString &fileName)
{
  QPtrList<KateFileType> types;

  for (uint z=0; z < m_types.size(); z++)
  {
    for( QStringList::Iterator it = m_types[z]->wildcards.begin(); it != m_types[z]->wildcards.end(); ++it )
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(*it, true, true);
      if ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        types.append (m_types[z]);
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

KateFileType *KateFileTypeManager::fileType (uint number)
{
  return m_types[number];
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
