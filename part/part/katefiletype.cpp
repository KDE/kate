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
#include "katedialogs.h"
#include "katefactory.h"

#include <kconfig.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <knuminput.h>
#include <klocale.h>

#include <qregexp.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qheader.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>

KateFileTypeManager::KateFileTypeManager ()
{
  m_types.setAutoDelete (true);

  update ();
}

KateFileTypeManager::~KateFileTypeManager ()
{
}

//
// read the types from config file and update the internal list
//
void KateFileTypeManager::update ()
{
  KConfig config ("katefiletyperc", false, false);

  QStringList g (config.groupList());

  g.sort ();

  m_types.clear ();
  m_types.resize (g.count());

  for (uint z=0; z < g.count(); z++)
  {
    config.setGroup (g[z]);

    KateFileType *type = new KateFileType ();

    type->number = z;
    type->name = g[z];
    type->section = config.readEntry ("Section");
    type->wildcards = config.readListEntry ("Wildcards", ';');
    type->mimetypes = config.readListEntry ("Mimetypes", ';');
    type->priority = config.readNumEntry ("Priority");
    type->varLine = config.readEntry ("Variables");

    m_types.insert (z, type);

    kdDebug(13020) << "INIT LIST: " << type->name << endl;
  }
}

//
// save the given list to config file + update
//
void KateFileTypeManager::save (QPtrVector<KateFileType> *v)
{
  KConfig config ("katefiletyperc", false, false);

  QStringList newg;
  for (uint z=0; z < v->count(); z++)
  {
    config.setGroup (v->operator[](z)->name);

    config.writeEntry ("Section", v->operator[](z)->section);
    config.writeEntry ("Wildcards", v->operator[](z)->wildcards, ';');
    config.writeEntry ("Mimetypes", v->operator[](z)->mimetypes, ';');
    config.writeEntry ("Priority", v->operator[](z)->priority);
    config.writeEntry ("Variables", v->operator[](z)->varLine);

    newg << v->operator[](z)->name;
  }

  QStringList g (config.groupList());

  for (uint z=0; z < g.count(); z++)
  {
    if (newg.findIndex (g[z]) == -1)
      config.deleteGroup (g[z]);
  }

  config.sync ();

  update ();
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
  m_types.setAutoDelete (true);

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  QHBox *hbHl = new QHBox( this );
  layout->add (hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("&Filetype:"), hbHl );
  typeCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( typeCombo );
  connect( typeCombo, SIGNAL(activated(int)),
           this, SLOT(typeChanged(int)) );

  QPushButton *btndel = new QPushButton( i18n("&Delete"), hbHl );
  connect( btndel, SIGNAL(clicked()), this, SLOT(deleteType()) );

  QPushButton *btnnew = new QPushButton( i18n("&New..."), hbHl );
  connect( btnnew, SIGNAL(clicked()), this, SLOT(newType()) );

  QGroupBox *gbProps = new QGroupBox( 1, Qt::Horizontal, i18n("Properties"), this );
  layout->add (gbProps);

  // file & mime types
  QHBox *hbFE = new QHBox( gbProps);
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), hbFE );
  wildcards  = new QLineEdit( hbFE );
  lFileExts->setBuddy( wildcards );
  connect( wildcards, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  QHBox *hbMT = new QHBox( gbProps );
  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), hbMT);
  mimetypes = new QLineEdit( hbMT );
  connect( mimetypes, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  lMimeTypes->setBuddy( mimetypes );

  QHBox *hbMT2 = new QHBox( gbProps );
  QLabel *lprio = new QLabel( i18n("Prio&rity:"), hbMT2);
  priority = new KIntNumInput( hbMT2 );
  connect( priority, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );
  lprio->setBuddy( priority );

  layout->addStretch();

  reload();
}

void KateFileTypeConfigTab::apply()
{
  KateFactory::fileTypeManager()->save(&m_types);
}

void KateFileTypeConfigTab::reload()
{
  m_types.resize (KateFactory::fileTypeManager()->list()->size());

  for (uint z=0; z < m_types.size(); z++)
  {
    KateFileType *type = new KateFileType ();

    *type = *KateFactory::fileTypeManager()->list()->operator[](z);

    m_types.insert (z, type);
  }

  update ();
}

void KateFileTypeConfigTab::reset()
{
}

void KateFileTypeConfigTab::defaults()
{
}

void KateFileTypeConfigTab::update ()
{
  for( uint i = 0; i < m_types.size(); i++) {
    if (m_types[i]->section.length() > 0)
      typeCombo->insertItem(m_types[i]->section + QString ("/") + m_types[i]->name);
    else
      typeCombo->insertItem(m_types[i]->name);
  }

  typeCombo->setCurrentItem(-1);
}

void KateFileTypeConfigTab::deleteType ()
{

}

void KateFileTypeConfigTab::newType ()
{

}

void KateFileTypeConfigTab::typeChanged (int type)
{
}

// kate: space-indent on; indent-width 2; replace-tabs on;
