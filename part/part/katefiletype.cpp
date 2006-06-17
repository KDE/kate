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

#include "ui_filetypeconfigwidget.h"

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
    type.wildcards = config.readEntry ("Wildcards", QStringList(), ';');
    type.mimetypes = config.readEntry ("Mimetypes", QStringList(), ';');
    type.priority = config.readEntry ("Priority", 0);
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
    if (QRegExp("kate:(.*)").indexIn(varLine) < 0)
      varLine.prepend ("kate: ");

    config.writeEntry ("Variables", varLine);

    newg << type.name;
  }

  QStringList g (config.groupList());

  for (int z=0; z < g.count(); z++)
  {
    if (newg.indexOf (g[z]) == -1)
      config.deleteGroup (g[z]);
  }

  config.sync ();

  update ();
}

int KateFileTypeManager::fileType (KateDocument *doc)
{
  kDebug(13020)<<k_funcinfo<<endl;
  if (!doc)
    return -1;

  if (m_types.isEmpty())
    return -1;

  QString fileName = doc->url().prettyUrl();
  int length = doc->url().prettyUrl().length();

  int result;

  // Try wildcards
  if ( ! fileName.isEmpty() )
  {
    static QStringList commonSuffixes = QString(".orig;.new;~;.bak;.BAK").split (";");

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
    kDebug(13020)<<"KateFiletype::filetype(): got type "<<result<<" using docName '"<<doc->documentName()<<"'"<<endl;
    return result;
  }

  // Try content-based mimetype
  KMimeType::Ptr mt = doc->mimeTypeForContent();
  
  if (!mt)
    return -1;

  QList<KateFileType> types;

  foreach (const KateFileType& type, m_types)
  {
    if (type.mimetypes.indexOf (mt->name()) > -1)
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
      QRegExp re(wildcard, Qt::CaseSensitive, QRegExp::Wildcard);
      if ( ( re.indexIn( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
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

  ui = new Ui::FileTypeConfigWidget();
  ui->setupUi( this );

  connect( ui->cmbFiletypes, SIGNAL(activated(int)), this, SLOT(typeChanged(int)) );
  connect( ui->btnNew, SIGNAL(clicked()), this, SLOT(newType()) );
  connect( ui->btnDelete, SIGNAL(clicked()), this, SLOT(deleteType()) );
  ui->btnMimeTypes->setIcon(QIcon(SmallIcon("wizard")));
  connect(ui->btnMimeTypes, SIGNAL(clicked()), this, SLOT(showMTDlg()));

  reload();

  connect( ui->edtName, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( ui->edtSection, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( ui->edtVariables, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( ui->edtFileExtensions, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( ui->edtMimeTypes, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( ui->sbPriority, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );
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

  ui->cmbFiletypes->clear ();

  foreach (const KateFileType& type, m_types) {
    if (type.section.length() > 0)
      ui->cmbFiletypes->addItem(type.section + QString ("/") + type.name);
    else
      ui->cmbFiletypes->addItem(type.name);
  }

  ui->cmbFiletypes->setCurrentIndex (0);

  typeChanged (0);

  ui->cmbFiletypes->setEnabled (ui->cmbFiletypes->count() > 0);
}

void KateFileTypeConfigTab::deleteType ()
{
  int type = ui->cmbFiletypes->currentIndex ();

  if (type > -1 && type < m_types.count())
  {
    m_types.removeAt(type);
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
      ui->cmbFiletypes->setCurrentIndex (i);
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
    m_types[m_lastType].name = ui->edtName->text ();
    m_types[m_lastType].section = ui->edtSection->text ();
    m_types[m_lastType].varLine = ui->edtVariables->text ();
    m_types[m_lastType].wildcards = ui->edtFileExtensions->text().split (";", QString::SkipEmptyParts);
    m_types[m_lastType].mimetypes = ui->edtMimeTypes->text().split (";", QString::SkipEmptyParts);
    m_types[m_lastType].priority = ui->sbPriority->value();
  }
}

void KateFileTypeConfigTab::typeChanged (int type)
{
  save ();

  if (type > -1 && type < m_types.count())
  {
    const KateFileType& t = m_types.at(type);

    ui->gbProperties->setTitle (i18n("Properties of %1",  ui->cmbFiletypes->currentText()));

    ui->gbProperties->setEnabled (true);
    ui->btnDelete->setEnabled (true);

    ui->edtName->setText(t.name);
    ui->edtSection->setText(t.section);
    ui->edtVariables->setText(t.varLine);
    ui->edtFileExtensions->setText(t.wildcards.join (";"));
    ui->edtMimeTypes->setText(t.mimetypes.join (";"));
    ui->sbPriority->setValue(t.priority);
  }
  else
  {
    ui->gbProperties->setTitle (i18n("Properties"));

    ui->gbProperties->setEnabled (false);
    ui->btnDelete->setEnabled (false);

    ui->edtName->clear();
    ui->edtSection->clear();
    ui->edtVariables->clear();
    ui->edtFileExtensions->clear();
    ui->edtMimeTypes->clear();
    ui->sbPriority->setValue(0);
  }

  m_lastType = type;
}

void KateFileTypeConfigTab::showMTDlg()
{
  QString text = i18n("Select the MimeTypes you want for this file type.\nPlease note that this will automatically edit the associated file extensions as well.");
  QStringList list = ui->edtMimeTypes->text().split( QRegExp("\\s*;\\s*"), QString::SkipEmptyParts );
  KMimeTypeChooserDialog *d = new KMimeTypeChooserDialog( i18n("Select Mime Types"), text, list, "text", this );
  if ( d->exec() == KDialog::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    ui->edtFileExtensions->setText( d->chooser()->patterns().join(";") );
    ui->edtMimeTypes->setText( d->chooser()->mimeTypes().join(";") );
  }
}
//END KateFileTypeConfigTab

//BEGIN KateViewFileTypeAction
void KateViewFileTypeAction::init()
{
  m_doc = 0;

  connect( kMenu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );
  QAction *action = kMenu()->addAction ( i18n("None") );
  action->setData( 0 );
  action->setCheckable( true );

  connect(kMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
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
        QMenu *menu = new QMenu (hlSection);
        connect( menu, SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );
        subMenus.append(menu);
        kMenu()->addMenu (menu);
      }

      int m = subMenusName.indexOf (hlSection);
      names << hlName;
      QAction *action = subMenus.at(m)->addAction ( hlName );
      action->setCheckable( true );
      action->setData( z+1 );
    }
    else if (!names.contains(hlName))
    {
      names << hlName;

      disconnect( kMenu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );
      connect( kMenu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );

      QAction *action = kMenu()->addAction ( hlName );
      action->setCheckable( true );
      action->setData( z+1 );
    }
  }

  if (!doc) return;

  for (int i=0;i<subMenus.count();i++)
  {
    QList<QAction*> actions = subMenus.at( i )->actions();
    for ( int j = 0; j < actions.count(); ++j )
      actions[ j ]->setChecked( false );
  }

  QList<QAction*> actions = kMenu()->actions();
  for ( int i = 0; i < actions.count(); ++i )
    actions[ i ]->setChecked( false );

  if (doc->fileType() == -1) {
    for ( int i = 0; i < actions.count(); ++i ) {
      if ( actions[ i ]->data().toInt() == 0 )
        actions[ i ]->setChecked( true );
    }
  } else {
    if (KateGlobal::self()->fileTypeManager()->isValidType(doc->fileType()))
    {
      const KateFileType& t = KateGlobal::self()->fileTypeManager()->fileType(doc->fileType());
      int i = subMenusName.indexOf (t.section);
      if (i >= 0 && subMenus.at(i)) {
        QList<QAction*> actions = subMenus.at( i )->actions();
        for ( int j = 0; j < actions.count(); ++j ) {
          if ( actions[ j ]->data().toInt() == (doc->fileType()+1) )
            actions[ j ]->setChecked( true );
        }
      } else {
        QList<QAction*> actions = kMenu()->actions();
        for ( int j = 0; j < actions.count(); ++j ) {
          if ( actions[ j ]->data().toInt() == 0 )
            actions[ j ]->setChecked( true );
        }
      }
    }
  }
}

void KateViewFileTypeAction::setType (QAction *action)
{
  KateDocument *doc=m_doc;

  if (doc) {
    int mode = action->data().toInt();
    doc->updateFileType(mode-1, true);
  }
}

//END KateViewFileTypeAction
// kate: space-indent on; indent-width 2; replace-tabs on;
