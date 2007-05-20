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

//BEGIN KateFileTypeManager
KateFileTypeManager::KateFileTypeManager ()
{
  update ();
}

KateFileTypeManager::~KateFileTypeManager ()
{
  qDeleteAll (m_types);
}

//
// read the types from config file and update the internal list
//
void KateFileTypeManager::update ()
{
  KConfig config ("katefiletyperc", KConfig::NoGlobals);

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
    
    type->hl = cg.readEntry ("Highlighting", "None");
    
    if (type->hl.isEmpty())
      type->hl = "None";
    
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
  t->name = "None";
  t->hl = "None";
  t->hlGenerated = true;

  m_types.prepend (t);
}

//
// save the given list to config file + update
//
void KateFileTypeManager::save (const QList<KateFileType *>& v)
{
  KConfig katerc("katefiletyperc", KConfig::NoGlobals);
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

QString KateFileTypeManager::fileType (KateDocument *doc)
{
  kDebug(13020)<<k_funcinfo<<endl;
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

QString KateFileTypeManager::wildcardsFind (const QString &fileName)
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

const KateFileType& KateFileTypeManager::fileType(const QString &name) const
{
  for (int i = 0; i < m_types.size(); ++i)
    if (m_types[i]->name == name)
      return *m_types[i];
      
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

 for( int i = 0; i < KateHlManager::self()->highlights(); i++) {
    if (KateHlManager::self()->hlSection(i).length() > 0)
      ui->cmbHl->addItem(KateHlManager::self()->hlSection(i) + QString ("/")
          + KateHlManager::self()->hlNameTranslated(i), QVariant(KateHlManager::self()->hlName(i)));
    else
      ui->cmbHl->addItem(KateHlManager::self()->hlNameTranslated(i), QVariant(KateHlManager::self()->hlName(i)));
  }

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
  connect( ui->cmbHl, SIGNAL(activated(int)), this, SLOT(slotChanged()) );
}

KateFileTypeConfigTab::~KateFileTypeConfigTab ()
{
  qDeleteAll (m_types);
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
  qDeleteAll (m_types);
  m_types.clear();

  // deep copy...
  foreach (KateFileType *type, KateGlobal::self()->fileTypeManager()->list())
  {
    KateFileType *t = new KateFileType ();
    *t = *type;
    m_types.append (t);
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

  foreach (KateFileType *type, m_types) {
    if (type->section.length() > 0)
      ui->cmbFiletypes->addItem(type->section + QString ("/") + type->name);
    else
      ui->cmbFiletypes->addItem(type->name);
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
    delete m_types[type];
    m_types.removeAt(type);
    update ();
  }
}

void KateFileTypeConfigTab::newType ()
{
  QString newN = i18n("New Filetype");

  for (int i = 0; i < m_types.count(); ++i) {
    KateFileType *type = m_types.at(i);
    if (type->name == newN)
    {
      ui->cmbFiletypes->setCurrentIndex (i);
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
  if (m_lastType != -1)
  {
    m_types[m_lastType]->name = ui->edtName->text ();
    m_types[m_lastType]->section = ui->edtSection->text ();
    m_types[m_lastType]->varLine = ui->edtVariables->text ();
    m_types[m_lastType]->wildcards = ui->edtFileExtensions->text().split (";", QString::SkipEmptyParts);
    m_types[m_lastType]->mimetypes = ui->edtMimeTypes->text().split (";", QString::SkipEmptyParts);
    m_types[m_lastType]->priority = ui->sbPriority->value();
    m_types[m_lastType]->hl = ui->cmbHl->itemData(ui->cmbHl->currentIndex()).toString();
  }
}

void KateFileTypeConfigTab::typeChanged (int type)
{
  save ();
    
  ui->cmbHl->setEnabled (true);
  ui->btnDelete->setEnabled (true);
  ui->edtName->setEnabled (true);
  ui->edtSection->setEnabled (true);

  if (type > -1 && type < m_types.count())
  {
    KateFileType *t = m_types.at(type);

    ui->gbProperties->setTitle (i18n("Properties of %1",  ui->cmbFiletypes->currentText()));

    ui->gbProperties->setEnabled (true);
    ui->btnDelete->setEnabled (true);

    ui->edtName->setText(t->name);
    ui->edtSection->setText(t->section);
    ui->edtVariables->setText(t->varLine);
    ui->edtFileExtensions->setText(t->wildcards.join (";"));
    ui->edtMimeTypes->setText(t->mimetypes.join (";"));
    ui->sbPriority->setValue(t->priority);
    
    ui->cmbHl->setEnabled (!t->hlGenerated);
    ui->btnDelete->setEnabled (!t->hlGenerated);
    ui->edtName->setEnabled (!t->hlGenerated);
    ui->edtSection->setEnabled (!t->hlGenerated);

    // activate current hl...
    for (int i = 0; i < ui->cmbHl->count(); ++i)
      if (ui->cmbHl->itemData (i).toString() == t->hl)
        ui->cmbHl->setCurrentIndex (i);
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
    ui->cmbHl->setCurrentIndex (0);
  }

  m_lastType = type;
}

void KateFileTypeConfigTab::showMTDlg()
{
  QString text = i18n("Select the MimeTypes you want for this file type.\nPlease note that this will automatically edit the associated file extensions as well.");
  QStringList list = ui->edtMimeTypes->text().split( QRegExp("\\s*;\\s*"), QString::SkipEmptyParts );
  KMimeTypeChooserDialog d( i18n("Select Mime Types"), text, list, "text", this );
  if ( d.exec() == KDialog::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    ui->edtFileExtensions->setText( d.chooser()->patterns().join(";") );
    ui->edtMimeTypes->setText( d.chooser()->mimeTypes().join(";") );
  }
}
//END KateFileTypeConfigTab

//BEGIN KateViewFileTypeAction
void KateViewFileTypeAction::init()
{
  m_doc = 0;

  connect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );

  connect(menu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
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
    QString hlName = KateGlobal::self()->fileTypeManager()->list().at(z)->name;
    QString hlSection = KateGlobal::self()->fileTypeManager()->list().at(z)->section;

    if ( !hlSection.isEmpty() && !names.contains(hlName) )
    {
      if (!subMenusName.contains(hlSection))
      {
        subMenusName << hlSection;
        QMenu *qmenu = new QMenu (hlSection);
        connect( qmenu, SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );
        subMenus.append(qmenu);
        menu()->addMenu (qmenu);
      }

      int m = subMenusName.indexOf (hlSection);
      names << hlName;
      QAction *action = subMenus.at(m)->addAction ( hlName );
      action->setCheckable( true );
      action->setData( hlName );
    }
    else if (!names.contains(hlName))
    {
      names << hlName;

      disconnect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );
      connect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setType( QAction* ) ) );

      QAction *action = menu()->addAction ( hlName );
      action->setCheckable( true );
      action->setData( hlName );
    }
  }

  if (!doc) return;

  for (int i=0;i<subMenus.count();i++)
  {
    QList<QAction*> actions = subMenus.at( i )->actions();
    for ( int j = 0; j < actions.count(); ++j )
      actions[ j ]->setChecked( false );
  }

  QList<QAction*> actions = menu()->actions();
  for ( int i = 0; i < actions.count(); ++i )
    actions[ i ]->setChecked( false );

  if (doc->fileType().isEmpty() || doc->fileType() == "None") {
    for ( int i = 0; i < actions.count(); ++i ) {
      if ( actions[ i ]->data().toString() == "None" )
        actions[ i ]->setChecked( true );
    }
  } else {
    if (!doc->fileType().isEmpty())
    {
      const KateFileType& t = KateGlobal::self()->fileTypeManager()->fileType(doc->fileType());
      int i = subMenusName.indexOf (t.section);
      if (i >= 0 && subMenus.at(i)) {
        QList<QAction*> actions = subMenus.at( i )->actions();
        for ( int j = 0; j < actions.count(); ++j ) {
          if ( actions[ j ]->data().toString() == doc->fileType() )
            actions[ j ]->setChecked( true );
        }
      } else {
        QList<QAction*> actions = menu()->actions();
        for ( int j = 0; j < actions.count(); ++j ) {
          if ( actions[ j ]->data().toString() == "" )
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
    doc->updateFileType(action->data().toString(), true);
  }
}

//END KateViewFileTypeAction
// kate: space-indent on; indent-width 2; replace-tabs on;
