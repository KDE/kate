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
#include <QGroupBox>

#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <kvbox.h>

#define KATE_FT_HOWMANY 1024
//END Includes

//BEGIN KateFileTypeManager
KateFileTypeManager::KateFileTypeManager ()
{
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

    KateFileType type;
    type.number = z;
    type.name = g[z];
    type.section = config.readEntry ("Section");
    type.wildcards = config.readListEntry ("Wildcards", ';');
    type.mimetypes = config.readListEntry ("Mimetypes", ';');
    type.priority = config.readNumEntry ("Priority");
    type.varLine = config.readEntry ("Variables");

    m_types.append(type);
  }
}

//
// save the given list to config file + update
//
void KateFileTypeManager::save (const QList<KateFileType>& v)
{
  KConfig config ("katefiletyperc", false, false);

  QStringList newg;
  foreach (const KateFileType& type, v)
  {
    config.setGroup(type.name);

    config.writeEntry ("Section", type.section);
    config.writeEntry ("Wildcards", type.wildcards, ';');
    config.writeEntry ("Mimetypes", type.mimetypes, ';');
    config.writeEntry ("Priority", type.priority);

    QString varLine = type.varLine;
    if (QRegExp("kate:(.*)").search(varLine) < 0)
      varLine.prepend ("kate: ");

    config.writeEntry ("Variables", varLine);

    newg << type.name;
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

  QList<KateFileType> types;

  foreach (const KateFileType& type, m_types)
  {
    if (type.mimetypes.findIndex (mt->name()) > -1)
      types.append (type);
  }

  if ( !types.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    foreach (const KateFileType& type, types)
    {
      if (type.priority > pri)
      {
        pri = type.priority;
        hl = type.number;
      }
    }

    return hl;
  }


  return -1;
}

int KateFileTypeManager::wildcardsFind (const QString &fileName)
{
  QList<KateFileType> types;

  foreach (const KateFileType& type, m_types)
  {
    foreach (QString wildcard, type.wildcards)
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(wildcard, true, true);
      if ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        types.append (type);
    }
  }

  if ( !types.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    foreach (const KateFileType& type, types)
    {
      if (type.priority > pri)
      {
        pri = type.priority;
        hl = type.number;
      }
    }

    return hl;
  }

  return -1;
}

bool KateFileTypeManager::isValidType( int number ) const
{
  return number >= 0 && number < m_types.count();
}

const KateFileType& KateFileTypeManager::fileType(int number) const
{
  if (number >= 0 && number < m_types.count())
    return m_types.at(number);

  static KateFileType notype;
  return notype;
}
//END KateFileTypeManager

//BEGIN KateFileTypeConfigTab
KateFileTypeConfigTab::KateFileTypeConfigTab( QWidget *parent )
  : KateConfigPage( parent )
{
  m_lastType = -1;

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

  gbProps = new QGroupBox( this );
  gbProps->setTitle(i18n("Properties"));
  QGridLayout* grid = new QGridLayout(gbProps);
  layout->add (gbProps);

  // file & mime types
  QLabel *lname = new QLabel( i18n("N&ame:"), gbProps );
  grid->addWidget(lname, 0, 0);
  name  = new QLineEdit( gbProps );
  grid->addWidget(name, 0, 1);
  lname->setBuddy( name );

  // file & mime types
  QLabel *lsec = new QLabel( i18n("&Section:"), gbProps );
  grid->addWidget(lsec, 1, 0);
  section  = new QLineEdit( gbProps );
  grid->addWidget(section, 1, 1);
  lsec->setBuddy( section );

  // file & mime types
  QLabel *lvar = new QLabel( i18n("&Variables:"), gbProps );
  grid->addWidget(lvar, 2, 0);
  varLine  = new QLineEdit( gbProps );
  grid->addWidget(varLine, 2, 1);
  lvar->setBuddy( varLine );

  // file & mime types
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), gbProps );
  grid->addWidget(lFileExts, 3, 0);
  wildcards  = new QLineEdit( gbProps );
  grid->addWidget(wildcards, 3, 1);
  lFileExts->setBuddy( wildcards );

  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), gbProps);
  grid->addWidget(lMimeTypes, 4, 0);
  KHBox *hbMT = new KHBox (gbProps);
  hbMT->setMargin(0);
  grid->addWidget(hbMT, 4, 1);
  mimetypes = new QLineEdit( hbMT );
  lMimeTypes->setBuddy( mimetypes );

  QToolButton *btnMTW = new QToolButton(hbMT);
  btnMTW->setIcon(QIcon(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));

  QLabel *lprio = new QLabel( i18n("Prio&rity:"), gbProps);
  grid->addWidget(lprio, 5, 0);
  priority = new KIntNumInput( gbProps );
  grid->addWidget(priority, 5, 1);
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

  KateGlobal::self()->fileTypeManager()->save(m_types);
}

void KateFileTypeConfigTab::reload()
{
  m_types.clear();
  foreach (const KateFileType& type, KateGlobal::self()->fileTypeManager()->list())
  {
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
  m_lastType = -1;

  typeCombo->clear ();

  foreach (const KateFileType& type, m_types) {
    if (type.section.length() > 0)
      typeCombo->addItem(type.section + QString ("/") + type.name);
    else
      typeCombo->addItem(type.name);
  }

  typeCombo->setCurrentIndex (0);

  typeChanged (0);

  typeCombo->setEnabled (typeCombo->count() > 0);
}

void KateFileTypeConfigTab::deleteType ()
{
  int type = typeCombo->currentIndex ();

  if (type > -1 && type < m_types.count())
  {
    m_types.takeAt(type);
    update ();
  }
}

void KateFileTypeConfigTab::newType ()
{
  QString newN = i18n("New Filetype");

  for (int i = 0; i < m_types.count(); ++i) {
    const KateFileType& type = m_types.at(i);
    if (type.name == newN)
    {
      typeCombo->setCurrentIndex (i);
      typeChanged (i);
      return;
    }
  }

  KateFileType newT;
  newT.priority = 0;
  newT.name = newN;

  m_types.prepend (newT);

  update ();
}

void KateFileTypeConfigTab::save ()
{
  if (m_lastType != -1)
  {
    m_types[m_lastType].name = name->text ();
    m_types[m_lastType].section = section->text ();
    m_types[m_lastType].varLine = varLine->text ();
    m_types[m_lastType].wildcards = QStringList::split (";", wildcards->text ());
    m_types[m_lastType].mimetypes = QStringList::split (";", mimetypes->text ());
    m_types[m_lastType].priority = priority->value();
  }
}

void KateFileTypeConfigTab::typeChanged (int type)
{
  save ();

  if (type > -1 && type < m_types.count())
  {
    const KateFileType& t = m_types.at(type);

    gbProps->setTitle (i18n("Properties of %1").arg (typeCombo->currentText()));

    gbProps->setEnabled (true);
    btndel->setEnabled (true);

    name->setText(t.name);
    section->setText(t.section);
    varLine->setText(t.varLine);
    wildcards->setText(t.wildcards.join (";"));
    mimetypes->setText(t.mimetypes.join (";"));
    priority->setValue(t.priority);
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

  m_lastType = type;
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

  popupMenu()->insertItem ( i18n("None"), this, SLOT(setType(int)), 0,  0);

  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

KateViewFileTypeAction::~ KateViewFileTypeAction( )
{
  qDeleteAll(subMenus);
}

void KateViewFileTypeAction::updateMenu (KTextEditor::Document *doc)
{
  m_doc = (KateDocument *)doc;
}

void KateViewFileTypeAction::slotAboutToShow()
{
  KateDocument *doc=m_doc;
  int count = KateGlobal::self()->fileTypeManager()->list().count();

  for (int z=0; z<count; z++)
  {
    QString hlName = KateGlobal::self()->fileTypeManager()->list().at(z).name;
    QString hlSection = KateGlobal::self()->fileTypeManager()->list().at(z).section;

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

  for (int i=0;i<subMenus.count();i++)
  {
    for (int i2=0;i2<(int)subMenus.at(i)->count();i2++)
      subMenus.at(i)->setItemChecked(subMenus.at(i)->idAt(i2),false);
  }
  popupMenu()->setItemChecked (0, false);

  if (doc->fileType() == -1)
    popupMenu()->setItemChecked (0, true);
  else
  {
    if (KateGlobal::self()->fileTypeManager()->isValidType(doc->fileType()))
    {
      const KateFileType& t = KateGlobal::self()->fileTypeManager()->fileType(doc->fileType());
      int i = subMenusName.findIndex (t.section);
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
