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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katefiletype.h"
#include "katefiletype.moc"

#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "kateglobal.h"

#include <kconfig.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kmimetypechooser.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <knuminput.h>
#include <klocale.h>
#include <kmenu.h>

#include <qregexp.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <q3groupbox.h>
#include <q3hbox.h>
#include <q3header.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <q3widgetstack.h>

#define KATE_FT_HOWMANY 1024
//END Includes

//BEGIN KateFileTypeManager
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
  for (int z=0; z < g.count(); z++)
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

    m_types.append (type);
  }
}

//
// save the given list to config file + update
//
void KateFileTypeManager::save (Q3PtrList<KateFileType> *v)
{
  KConfig config ("katefiletyperc", false, false);

  QStringList newg;
  for (uint z=0; z < v->count(); z++)
  {
    config.setGroup (v->at(z)->name);

    config.writeEntry ("Section", v->at(z)->section);
    config.writeEntry ("Wildcards", v->at(z)->wildcards, ';');
    config.writeEntry ("Mimetypes", v->at(z)->mimetypes, ';');
    config.writeEntry ("Priority", v->at(z)->priority);

    QString varLine = v->at(z)->varLine;
    if (QRegExp("kate:(.*)").search(varLine) < 0)
      varLine.prepend ("kate: ");

    config.writeEntry ("Variables", varLine);

    newg << v->at(z)->name;
  }

  QStringList g (config.groupList());

  for (int z=0; z < g.count(); z++)
  {
    if (newg.findIndex (g[z]) == -1)
      config.deleteGroup (g[z]);
  }

  config.sync ();

  update ();
}

int KateFileTypeManager::fileType (KateDocument *doc)
{
  kdDebug(13020)<<k_funcinfo<<endl;
  if (!doc)
    return -1;

  if (m_types.isEmpty())
    return -1;

  QString fileName = doc->url().prettyURL();
  int length = doc->url().prettyURL().length();

  int result;

  // Try wildcards
  if ( ! fileName.isEmpty() )
  {
    static QStringList commonSuffixes = QStringList::split (";", ".orig;.new;~;.bak;.BAK");

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
  }

  // Even try the document name, if the URL is empty
  // This is usefull if the document name is set for example by a plugin which
  // created the document
  else if ( (result = wildcardsFind(doc->documentName())) != -1)
  {
    kdDebug(13020)<<"KateFiletype::filetype(): got type "<<result<<" using docName '"<<doc->documentName()<<"'"<<endl;
    return result;
  }

  // Try content-based mimetype
  KMimeType::Ptr mt = doc->mimeTypeForContent();

  Q3PtrList<KateFileType> types;

  for (uint z=0; z < m_types.count(); z++)
  {
    if (m_types.at(z)->mimetypes.findIndex (mt->name()) > -1)
      types.append (m_types.at(z));
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
  Q3PtrList<KateFileType> types;

  for (uint z=0; z < m_types.count(); z++)
  {
    for( QStringList::Iterator it = m_types.at(z)->wildcards.begin(); it != m_types.at(z)->wildcards.end(); ++it )
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(*it, true, true);
      if ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        types.append (m_types.at(z));
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

const KateFileType *KateFileTypeManager::fileType (uint number)
{
  if (number < m_types.count())
    return m_types.at(number);

  return 0;
}
//END KateFileTypeManager

//BEGIN KateFileTypeConfigTab
KateFileTypeConfigTab::KateFileTypeConfigTab( QWidget *parent )
  : KateConfigPage( parent )
{
  m_types.setAutoDelete (true);
  m_lastType = 0;

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  QHBoxLayout *hbHl = new QHBoxLayout();
  layout->addLayout (hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("&Filetype:"), this );
  hbHl->addWidget(lHl);
  hbHl->addWidget(typeCombo = new QComboBox( false, this ));
  lHl->setBuddy( typeCombo );
  connect( typeCombo, SIGNAL(activated(int)),
           this, SLOT(typeChanged(int)) );

  QPushButton *btnnew = new QPushButton( i18n("&New"), this );
  hbHl->addWidget(btnnew);
  connect( btnnew, SIGNAL(clicked()), this, SLOT(newType()) );

  btndel = new QPushButton( i18n("&Delete"), this );
  hbHl->addWidget(btndel);
  connect( btndel, SIGNAL(clicked()), this, SLOT(deleteType()) );

  gbProps = new Q3GroupBox( 2, Qt::Horizontal, i18n("Properties"), this );
  layout->add (gbProps);

  // file & mime types
  QLabel *lname = new QLabel( i18n("N&ame:"), gbProps );
  name  = new QLineEdit( gbProps );
  lname->setBuddy( name );

  // file & mime types
  QLabel *lsec = new QLabel( i18n("&Section:"), gbProps );
  section  = new QLineEdit( gbProps );
  lsec->setBuddy( section );

  // file & mime types
  QLabel *lvar = new QLabel( i18n("&Variables:"), gbProps );
  varLine  = new QLineEdit( gbProps );
  lvar->setBuddy( varLine );

  // file & mime types
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), gbProps );
  wildcards  = new QLineEdit( gbProps );
  lFileExts->setBuddy( wildcards );

  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), gbProps);
  Q3HBox *hbMT = new Q3HBox (gbProps);
  mimetypes = new QLineEdit( hbMT );
  lMimeTypes->setBuddy( mimetypes );

  QToolButton *btnMTW = new QToolButton(hbMT);
  btnMTW->setIconSet(QIcon(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));

  QLabel *lprio = new QLabel( i18n("Prio&rity:"), gbProps);
  priority = new KIntNumInput( gbProps );
  lprio->setBuddy( priority );

  layout->addStretch();

  reload();

  connect( name, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( section, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( varLine, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( wildcards, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( mimetypes, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( priority, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );

  btnnew->setWhatsThis(i18n("Create a new file type.") );
  btndel->setWhatsThis(i18n("Delete the current file type.") );
  name->setWhatsThis(i18n(
      "The name of the filetype will be the text of the corresponding menu item.") );
  section->setWhatsThis(i18n(
      "The section name is used to organize the file types in menus.") );
  varLine->setWhatsThis(i18n(
      "<p>This string allows you to configure Kate's settings for the files "
      "selected by this mimetype using Kate variables. You can set almost any "
      "configuration option, such as highlight, indent-mode, encoding, etc.</p>"
      "<p>For a full list of known variables, see the manual.</p>") );
  wildcards->setWhatsThis(i18n(
      "The wildcards mask allows you to select files by filename. A typical "
      "mask uses an asterisk and the file extension, for example "
      "<code>*.txt; *.text</code>. The string is a semicolon-separated list "
      "of masks.") );
  mimetypes->setWhatsThis(i18n(
      "The mime type mask allows you to select files by mimetype. The string is "
      "a semicolon-separated list of mimetypes, for example "
      "<code>text/plain; text/english</code>.") );
  btnMTW->setWhatsThis(i18n(
      "Displays a wizard that helps you easily select mimetypes.") );
  priority->setWhatsThis(i18n(
      "Sets a priority for this file type. If more than one file type selects the same "
      "file, the one with the highest priority will be used." ) );
}

void KateFileTypeConfigTab::apply()
{
  if (!hasChanged())
    return;

  save ();

  KateGlobal::self()->fileTypeManager()->save(&m_types);
}

void KateFileTypeConfigTab::reload()
{
  m_types.clear();
  for (uint z=0; z < KateGlobal::self()->fileTypeManager()->list()->count(); z++)
  {
    KateFileType *type = new KateFileType ();

    *type = *KateGlobal::self()->fileTypeManager()->list()->at(z);

    m_types.append (type);
  }

  update ();
}

void KateFileTypeConfigTab::reset()
{
  reload ();
}

void KateFileTypeConfigTab::defaults()
{
  reload ();
}

void KateFileTypeConfigTab::update ()
{
  m_lastType = 0;

  typeCombo->clear ();

  for( uint i = 0; i < m_types.count(); i++) {
    if (m_types.at(i)->section.length() > 0)
      typeCombo->insertItem(m_types.at(i)->section + QString ("/") + m_types.at(i)->name);
    else
      typeCombo->insertItem(m_types.at(i)->name);
  }

  typeCombo->setCurrentItem (0);

  typeChanged (0);

  typeCombo->setEnabled (typeCombo->count() > 0);
}

void KateFileTypeConfigTab::deleteType ()
{
  int type = typeCombo->currentItem ();

  if ((type > -1) && ((uint)type < m_types.count()))
  {
    m_types.remove (type);
    update ();
  }
}

void KateFileTypeConfigTab::newType ()
{
  QString newN = i18n("New Filetype");

  for( uint i = 0; i < m_types.count(); i++) {
    if (m_types.at(i)->name == newN)
    {
      typeCombo->setCurrentItem (i);
      typeChanged (i);
      return;
    }
  }

  KateFileType *newT = new KateFileType ();
  newT->priority = 0;
  newT->name = newN;

  m_types.prepend (newT);

  update ();
}

void KateFileTypeConfigTab::save ()
{
  if (m_lastType)
  {
    m_lastType->name = name->text ();
    m_lastType->section = section->text ();
    m_lastType->varLine = varLine->text ();
    m_lastType->wildcards = QStringList::split (";", wildcards->text ());
    m_lastType->mimetypes = QStringList::split (";", mimetypes->text ());
    m_lastType->priority = priority->value();
  }
}

void KateFileTypeConfigTab::typeChanged (int type)
{
  save ();

  KateFileType *t = 0;

  if ((type > -1) && ((uint)type < m_types.count()))
    t = m_types.at(type);

  if (t)
  {
    gbProps->setTitle (i18n("Properties of %1").arg (typeCombo->currentText()));

    gbProps->setEnabled (true);
    btndel->setEnabled (true);

    name->setText(t->name);
    section->setText(t->section);
    varLine->setText(t->varLine);
    wildcards->setText(t->wildcards.join (";"));
    mimetypes->setText(t->mimetypes.join (";"));
    priority->setValue(t->priority);
  }
  else
  {
    gbProps->setTitle (i18n("Properties"));

    gbProps->setEnabled (false);
    btndel->setEnabled (false);

    name->clear();
    section->clear();
    varLine->clear();
    wildcards->clear();
    mimetypes->clear();
    priority->setValue(0);
  }

  m_lastType = t;
}

void KateFileTypeConfigTab::showMTDlg()
{

  QString text = i18n("Select the MimeTypes you want for this file type.\nPlease note that this will automatically edit the associated file extensions as well.");
  QStringList list = QStringList::split( QRegExp("\\s*;\\s*"), mimetypes->text() );
  KMimeTypeChooserDialog *d = new KMimeTypeChooserDialog( i18n("Select Mime Types"), text, list, "text", this );
  if ( d->exec() == KDialogBase::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    wildcards->setText( d->chooser()->patterns().join(";") );
    mimetypes->setText( d->chooser()->mimeTypes().join(";") );
  }
}
//END KateFileTypeConfigTab

//BEGIN KateViewFileTypeAction
void KateViewFileTypeAction::init()
{
  m_doc = 0;
  subMenus.setAutoDelete( true );

  popupMenu()->insertItem ( i18n("None"), this, SLOT(setType(int)), 0,  0);

  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewFileTypeAction::updateMenu (KTextEditor::Document *doc)
{
  m_doc = (KateDocument *)doc;
}

void KateViewFileTypeAction::slotAboutToShow()
{
  KateDocument *doc=m_doc;
  int count = KateGlobal::self()->fileTypeManager()->list()->count();

  for (int z=0; z<count; z++)
  {
    QString hlName = KateGlobal::self()->fileTypeManager()->list()->at(z)->name;
    QString hlSection = KateGlobal::self()->fileTypeManager()->list()->at(z)->section;

    if ( !hlSection.isEmpty() && !names.contains(hlName) )
    {
      if (!subMenusName.contains(hlSection))
      {
        subMenusName << hlSection;
        QMenu *menu = new QMenu ();
        subMenus.append(menu);
        popupMenu()->insertItem (hlSection, menu);
      }

      int m = subMenusName.findIndex (hlSection);
      names << hlName;
      subMenus.at(m)->insertItem ( hlName, this, SLOT(setType(int)), 0,  z+1);
    }
    else if (!names.contains(hlName))
    {
      names << hlName;
      popupMenu()->insertItem ( hlName, this, SLOT(setType(int)), 0,  z+1);
    }
  }

  if (!doc) return;

  for (uint i=0;i<subMenus.count();i++)
  {
    for (uint i2=0;i2<subMenus.at(i)->count();i2++)
      subMenus.at(i)->setItemChecked(subMenus.at(i)->idAt(i2),false);
  }
  popupMenu()->setItemChecked (0, false);

  if (doc->fileType() == -1)
    popupMenu()->setItemChecked (0, true);
  else
  {
    const KateFileType *t = 0;
    if ((t = KateGlobal::self()->fileTypeManager()->fileType (doc->fileType())))
    {
      int i = subMenusName.findIndex (t->section);
      if (i >= 0 && subMenus.at(i))
        subMenus.at(i)->setItemChecked (doc->fileType()+1, true);
      else
        popupMenu()->setItemChecked (0, true);
    }
  }
}

void KateViewFileTypeAction::setType (int mode)
{
  KateDocument *doc=m_doc;

  if (doc)
    doc->updateFileType(mode-1, true);
}
//END KateViewFileTypeAction
// kate: space-indent on; indent-width 2; replace-tabs on;
