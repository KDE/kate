/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2007 Anders Lund <anders@alweb.dk>

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
#include "katefilelist.h"
#include "katefilelist.moc"
#include "kateviewdocumentproxymodel.h"
#include "katedocmanager.h"

#include <kstandardaction.h>
#include <kdialog.h>
#include <kcolorbutton.h>
#include <KLocale>
#include <KColorScheme>
#include <KComboBox>

#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QContextMenuEvent>
#include <kdebug.h>
//END Includes


//BEGIN KateFileList

KateFileList::KateFileList(QWidget *parent, KActionCollection *actionCollection): QListView(parent)
{
  m_windowNext = actionCollection->addAction(KStandardAction::Back, this, SLOT(slotPrevDocument()));
  m_windowPrev = actionCollection->addAction(KStandardAction::Forward, this, SLOT(slotNextDocument()));

  m_filelistCloseDocument=actionCollection->addAction( "filelist_close" );
  m_filelistCloseDocument->setText( i18n( "Close" ) );
  m_filelistCloseDocument->setIcon( SmallIcon("window-close") );
  connect( m_filelistCloseDocument, SIGNAL( triggered() ), this, SLOT( slotDocumentClose() ) );
  m_filelistCloseDocument->setWhatsThis(i18n("Close the current document."));

  m_filelistCloseDocumentOther = actionCollection->addAction( "filelist_close_other" );
  m_filelistCloseDocumentOther->setText( i18n( "Close Other" ) );
  connect( m_filelistCloseDocumentOther, SIGNAL( triggered() ), this, SLOT( slotDocumentCloseOther() ) );
  m_filelistCloseDocumentOther->setWhatsThis(i18n("Close other open documents."));

  m_sortAction = new KSelectAction( i18n("Sort &By"), this );
  actionCollection->addAction( "filelist_sortby", m_sortAction );

  m_sortAction->setItems(QStringList()<<i18n("Opening Order")<<i18n("Document Name")<<i18n("URL")<<i18n("Custom"));
  m_sortAction->action(0)->setData(SortOpening);
  m_sortAction->action(1)->setData(SortName);
  m_sortAction->action(2)->setData(SortUrl);
  m_sortAction->action(3)->setData(SortCustom);
  KConfigGroup config(KGlobal::config(), "FileList");
  setSortRole( config.readEntry("SortRole", (int)KateDocManager::OpeningOrderRole) );

  connect( m_sortAction, SIGNAL(triggered(QAction*)), this, SLOT(setSortRoleFromAction(QAction*)) );

  QPalette p = palette();
  p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
  p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
  setPalette(p);
}  

KateFileList::~KateFileList()
{}

void KateFileList::mousePressEvent ( QMouseEvent * event ) {
  m_previouslySelected = selectionModel()->currentIndex();
  QListView::mousePressEvent(event);
}

void KateFileList::mouseReleaseEvent ( QMouseEvent * event ) {
  kDebug(13001)<<"KateFileList::mouseReleaseEvent";
  m_previouslySelected = QModelIndex();
  QListView::mouseReleaseEvent(event);
}

void KateFileList::contextMenuEvent ( QContextMenuEvent * event ) {
  m_indexContextMenu=selectionModel()->currentIndex();
  emit customContextMenuRequested(static_cast<QContextMenuEvent *>(event)->pos());
  if (m_previouslySelected.isValid()) {
    selectionModel()->select(m_previouslySelected,QItemSelectionModel::SelectCurrent);
    selectionModel()->setCurrentIndex(m_previouslySelected,QItemSelectionModel::SelectCurrent);
  }
  event->accept();
}

void KateFileList::slotDocumentClose() {
  m_previouslySelected = QModelIndex();
  QVariant v = m_indexContextMenu.data(KateDocManager::DocumentRole);
  if (!v.isValid()) return;
  emit closeDocument(v.value<KTextEditor::Document*>());
}

void KateFileList::slotDocumentCloseOther() {
  QVariant v = m_indexContextMenu.data(KateDocManager::DocumentRole);
  if (!v.isValid()) return;
  m_previouslySelected = m_indexContextMenu;
  emit closeOtherDocument(v.value<KTextEditor::Document*>());
}


void KateFileList::setSortRole(int role)
{
  if (model())
    qobject_cast<KateViewDocumentProxyModel*>(model())->setSortRole(role);

  if (role == SortOpening)
    m_sortAction->setCurrentItem(0);
  else if (role == SortName)
    m_sortAction->setCurrentItem(1);
  else if (role == SortUrl)
    m_sortAction->setCurrentItem(2);
  else
    m_sortAction->setCurrentItem(3);
}

int KateFileList::sortRole()
{
  return qobject_cast<KateViewDocumentProxyModel*>(model())->sortRole();
}

void KateFileList::setShadingEnabled(bool enable)
{
  return qobject_cast<KateViewDocumentProxyModel*>(model())->setShadingEnabled(enable);
}

bool KateFileList::shadingEnabled() const
{
  return qobject_cast<KateViewDocumentProxyModel*>(model())->shadingEnabled();
}

const QColor& KateFileList::editShade() const
{
  return qobject_cast<KateViewDocumentProxyModel*>(model())->editShade();
}

const QColor& KateFileList::viewShade() const
{
  return qobject_cast<KateViewDocumentProxyModel*>(model())->viewShade();
}

void KateFileList::setEditShade( const QColor &shade )
{
  qobject_cast<KateViewDocumentProxyModel*>(model())->setEditShade( shade );
}
void KateFileList::setViewShade( const QColor &shade )
{
  qobject_cast<KateViewDocumentProxyModel*>(model())->setViewShade( shade );
}

void KateFileList::slotNextDocument()
{
  QModelIndex idx = selectionModel()->currentIndex();
  if (idx.isValid())
  {
    QModelIndex newIdx = model()->index(idx.row() + 1, idx.column(), idx.parent());
    if (!newIdx.isValid())
      newIdx = model()->index(0, idx.column(), idx.parent());
    if (newIdx.isValid())
    {
//      selectionModel()->select(newIdx,QItemSelectionModel::SelectCurrent);
//      selectionModel()->setCurrentIndex(newIdx,QItemSelectionModel::SelectCurrent);
      emit activated(newIdx);
    }
  }
}

void KateFileList::slotPrevDocument()
{
  QModelIndex idx = selectionModel()->currentIndex();
  if (idx.isValid())
  {
    int row = idx.row() - 1;
    if (row < 0) row = model()->rowCount(idx.parent()) - 1;
    QModelIndex newIdx = model()->index(row, idx.column(), idx.parent());
    if (newIdx.isValid())
    {
//      selectionModel()->select(newIdx,QItemSelectionModel::SelectCurrent);
//      selectionModel()->setCurrentIndex(newIdx,QItemSelectionModel::SelectCurrent);
      emit activated(newIdx);
    }
  }
}

void KateFileList::setSortRoleFromAction(QAction* action)
{
  setSortRole(action->data().toInt());
}
//END KateFileList

//BEGIN KateFileListConfigPage
KateFileListConfigPage::KateFileListConfigPage( QWidget* parent, KateFileList *fl )
  :  QWidget( parent ),
    m_filelist( fl ),
    m_changed( false )
{
  QVBoxLayout *layout = new QVBoxLayout( this );
  int spacing = KDialog::spacingHint();
  layout->setSpacing( spacing );

  gbEnableShading = new QGroupBox( i18n("Background Shading"), this );
  gbEnableShading->setCheckable(true);
  layout->addWidget( gbEnableShading );

  QGridLayout *lo = new QGridLayout( gbEnableShading, 2, 2 );
  lo->setMargin(KDialog::marginHint());
  lo->setSpacing(KDialog::spacingHint());

  kcbViewShade = new KColorButton( gbEnableShading );
  lViewShade = new QLabel( kcbViewShade, i18n("&Viewed documents' shade:"), gbEnableShading );
  lo->addWidget( lViewShade, 2, 0 );
  lo->addWidget( kcbViewShade, 2, 1 );

  kcbEditShade = new KColorButton( gbEnableShading );
  lEditShade = new QLabel( kcbEditShade, i18n("&Modified documents' shade:"), gbEnableShading );
  lo->addWidget( lEditShade, 3, 0 );
  lo->addWidget( kcbEditShade, 3, 1 );

  // sorting
  QHBoxLayout *lo2 = new QHBoxLayout;
  layout->addItem( lo2 );
  lSort = new QLabel( i18n("&Sort by:"), this );
  lo2->addWidget( lSort );
  cmbSort = new KComboBox( this );
  lo2->addWidget( cmbSort );
  lSort->setBuddy( cmbSort );
  cmbSort->addItem(i18n("Opening Order"), (int)KateFileList::SortOpening);
  cmbSort->addItem(i18n("Document Name"), (int)KateFileList::SortName);
  cmbSort->addItem(i18n("Url"), (int)KateFileList::SortUrl);
  cmbSort->addItem(i18n("Custom"), (int)KateFileList::SortCustom);

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

  reload();

  connect( gbEnableShading, SIGNAL(toggled(bool)), this, SLOT(slotMyChanged()) );
  connect( kcbViewShade, SIGNAL(changed(const QColor&)), this, SLOT(slotMyChanged()) );
  connect( kcbEditShade, SIGNAL(changed(const QColor&)), this, SLOT(slotMyChanged()) );
  connect( cmbSort, SIGNAL(activated(int)), this, SLOT(slotMyChanged()) );
}

void KateFileListConfigPage::apply()
{
  if ( ! m_changed )
    return;
  m_changed = false;

  // Change settings in the filelist
  m_filelist->setViewShade( kcbViewShade->color() );
  m_filelist->setEditShade( kcbEditShade->color() );
  m_filelist->setShadingEnabled( gbEnableShading->isChecked() );
  m_filelist->setSortRole( cmbSort->itemData(cmbSort->currentItem()).toInt() );

  // write config
  KConfigGroup config(KGlobal::config(), "FileList");
  config.writeEntry("Shading Enabled", gbEnableShading->isChecked());
  // save shade colors if they differ from the color scheme
  KColorScheme colors(QPalette::Active);
  if (kcbViewShade->color() != colors.foreground(KColorScheme::VisitedText).color())
    config.writeEntry("View Shade", kcbViewShade->color());
  if (kcbEditShade->color() != colors.foreground(KColorScheme::ActiveText).color())
    config.writeEntry("Edit Shade", kcbEditShade->color());
  config.writeEntry("SortRole", cmbSort->itemData(cmbSort->currentItem()));

  // repaint the affected items
  m_filelist->repaint();
}

void KateFileListConfigPage::reload()
{
  // read in from config file
  KConfigGroup config(KGlobal::config(), "FileList");
  gbEnableShading->setChecked( config.readEntry("Shading Enabled", m_filelist->shadingEnabled()) );
  kcbViewShade->setColor( config.readEntry("View Shade", m_filelist->viewShade() ) );
  kcbEditShade->setColor( config.readEntry("Edit Shade", m_filelist->editShade() ) );
  cmbSort->setCurrentIndex( cmbSort->findData(m_filelist->sortRole()) );
  m_changed = false;
}

void KateFileListConfigPage::slotMyChanged()
{
  m_changed = true;
  emit changed();
}

//END KateFileListConfigPage

// kate: space-indent on; indent-width 2; replace-tabs on;
