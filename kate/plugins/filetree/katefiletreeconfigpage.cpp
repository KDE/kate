/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2007 Anders Lund <anders@alweb.dk>
   Copyright (C) 2009, Abhishek Patil <abhishekworld@gmail.com>
   Copyright (C) 2010, Thomas Fjellstrom <thomas@fjellstrom.ca>
   
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

#include "katefiletreeconfigpage.h"
#include "katefiletreeplugin.h"
#include "katefiletreedebug.h"
#include "katefiletreemodel.h"
#include "katefiletreeproxymodel.h"

#include <QGroupBox>
#include <KColorButton>
#include <QLabel>
#include <KComboBox>
#include <KColorUtils>
#include <QVBoxLayout>
#include <KLocale>
#include <KColorScheme>

KateFileTreeConfigPage::KateFileTreeConfigPage( QWidget* parent, KateFileTreePluginView *fl )
  :  Kate::PluginConfigPage( parent ),
    m_view( fl ),
    m_changed( false )
{
  kDebug(debugArea()) << "BEGIN";
  
  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->setMargin( 0 );

  gbEnableShading = new QGroupBox( i18n("Background Shading"), this );
  gbEnableShading->setCheckable(true);
  layout->addWidget( gbEnableShading );

  QGridLayout *lo = new QGridLayout( gbEnableShading);

  kcbViewShade = new KColorButton( gbEnableShading );
  lViewShade = new QLabel( i18n("&Viewed documents' shade:"), gbEnableShading );
  lViewShade->setBuddy( kcbViewShade );
  lo->addWidget( lViewShade, 2, 0 );
  lo->addWidget( kcbViewShade, 2, 1 );

  kcbEditShade = new KColorButton( gbEnableShading );
  lEditShade = new QLabel( i18n("&Modified documents' shade:"), gbEnableShading );
  lEditShade->setBuddy( kcbEditShade );
  lo->addWidget( lEditShade, 3, 0 );
  lo->addWidget( kcbEditShade, 3, 1 );

  // sorting
  QHBoxLayout *lo2 = new QHBoxLayout;
  layout->addLayout( lo2 );
  lSort = new QLabel( i18n("&Sort by:"), this );
  lo2->addWidget( lSort );
  cmbSort = new KComboBox( this );
  lo2->addWidget( cmbSort );
  lSort->setBuddy( cmbSort );
  cmbSort->addItem(i18n("Opening Order"), (int)KateFileTreeModel::OpeningOrderRole);
  cmbSort->addItem(i18n("Document Name"), (int)Qt::DisplayRole);
  cmbSort->addItem(i18n("Url"), (int)KateFileTreeModel::PathRole);


  // view mode
  QHBoxLayout *lo3 = new QHBoxLayout;
  layout->addLayout( lo3 );
  lMode = new QLabel( i18n("&View Mode:"), this );
  lo3->addWidget( lMode );
  cmbMode = new KComboBox( this );
  lo3->addWidget( cmbMode );
  lMode->setBuddy( cmbMode );
  cmbMode->addItem(i18n("Tree View"), QVariant(false));
  cmbMode->addItem(i18n("List View"), QVariant(true));

  layout->insertStretch( -1, 10 );

  gbEnableShading->setWhatsThis( i18n(
      "When background shading is enabled, documents that have been viewed "
      "or edited within the current session will have a shaded background. "
      "The most recent documents have the strongest shade.") );
  kcbViewShade->setWhatsThis( i18n(
      "Set the color for shading viewed documents.") );
  kcbEditShade->setWhatsThis( i18n(
      "Set the color for modified documents. This color is blended into "
      "the color for viewed files. The most recently edited documents get "
      "most of this color.") );

//   cmbSort->setWhatsThis( i18n(
//       "Set the sorting method for the documents.") );

  reset();

  connect( gbEnableShading, SIGNAL(toggled(bool)), this, SLOT(slotMyChanged()) );
  connect( kcbViewShade, SIGNAL(changed(const QColor&)), this, SLOT(slotMyChanged()) );
  connect( kcbEditShade, SIGNAL(changed(const QColor&)), this, SLOT(slotMyChanged()) );
  connect( cmbSort, SIGNAL(activated(int)), this, SLOT(slotMyChanged()) );
  connect( cmbMode, SIGNAL(activated(int)), this, SLOT(slotMyChanged()) );

  kDebug(debugArea()) << "END";
}

void KateFileTreeConfigPage::apply()
{
  kDebug(debugArea()) << "BEGIN";
  if ( ! m_changed ) {
    kDebug(debugArea()) << "END !changed";
    return;
  }
  
  m_changed = false;

  // Change settings in the filelist
  m_view->model()->setViewShade( kcbViewShade->color() );
  m_view->model()->setEditShade( kcbEditShade->color() );
  m_view->model()->setShadingEnabled( gbEnableShading->isChecked() );
  m_view->setListMode( cmbMode->itemData(cmbMode->currentIndex()).toBool() );
  m_view->proxy()->setSortRole( cmbSort->itemData(cmbSort->currentIndex()).toInt() );

  kDebug(debugArea()) << "END";
}

void KateFileTreeConfigPage::reset()
{
  kDebug(debugArea()) << "BEGIN";
  gbEnableShading->setChecked( m_view->model()->shadingEnabled() );
  kcbViewShade->setColor( m_view->model()->viewShade() );
  kcbEditShade->setColor( m_view->model()->editShade() );
  cmbSort->setCurrentIndex( cmbSort->findData( m_view->proxy()->sortRole() ) );
  cmbMode->setCurrentIndex( cmbMode->findData( m_view->model()->listMode() ) );
  
  m_changed = false;
  kDebug(debugArea()) << "END";
}

void KateFileTreeConfigPage::defaults()
{
  kDebug(debugArea()) << "BEGIN";
  gbEnableShading->setChecked( true );

  KColorScheme colors(QPalette::Active);
  QColor bg = colors.background().color();
  QColor editShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::ActiveText).color(), 0.5);
  QColor viewShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::VisitedText).color(), 0.5);
  kcbViewShade->setColor( viewShade );
  kcbEditShade->setColor( editShade );
  
  cmbSort->setCurrentIndex( Qt::DisplayRole );
  //m_changed = true;
  kDebug(debugArea()) << "END";
}

void KateFileTreeConfigPage::slotMyChanged()
{
  kDebug(debugArea()) << "BEGIN";
  m_changed = true;
  emit changed();
  kDebug(debugArea()) << "END";
}

// kate: space-indent on; indent-width 2; replace-tabs on;
