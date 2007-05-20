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
#include "katemodeconfigpage.h"
#include "katemodeconfigpage.moc"

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

  KateGlobal::self()->modeManager()->save(m_types);
}

void KateFileTypeConfigTab::reload()
{
  qDeleteAll (m_types);
  m_types.clear();

  // deep copy...
  foreach (KateFileType *type, KateGlobal::self()->modeManager()->list())
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

// kate: space-indent on; indent-width 2; replace-tabs on;
