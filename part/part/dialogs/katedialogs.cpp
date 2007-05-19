/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>

   Based on work of:
     Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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
#include "katedialogs.h"
#include "katedialogs.moc"

#include "kateautoindent.h"
#include "katebuffer.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "kateglobal.h"
#include "kateschema.h"
#include "katesyntaxdocument.h"
#include "kateview.h"

// auto generated ui files
#include "ui_modonhdwidget.h"
#include "ui_appearanceconfigwidget.h"
#include "ui_cursorconfigwidget.h"
#include "ui_editconfigwidget.h"
#include "ui_hlconfigwidget.h"
#include "ui_indentationconfigwidget.h"
#include "ui_opensaveconfigwidget.h"

#include <ktexteditor/plugin.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>

#include <kapplication.h>
#include <kcharsets.h>
#include <kcolorbutton.h>
#include <kcolorcombo.h>
#include <kcolordialog.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfontdialog.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kshortcutsdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypechooser.h>
#include <knuminput.h>
#include <kparts/componentfactory.h>
#include <kmenu.h>
#include <k3process.h>
#include <k3procio.h>
#include <krun.h>
#include <kseparator.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kpushbutton.h>
#include <kvbox.h>
#include <kactioncollection.h>
//#include <knewstuff/knewstuff.h>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <Qt/qdom.h>
#include <QtCore/QFile>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtGui/QPainter>
#include <QtGui/QRadioButton>
#include <QtGui/QSlider>
#include <QtGui/QSpinBox>
#include <QtCore/QStringList>
#include <QtGui/QTabWidget>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
#include <QtGui/QToolButton>
#include <QtGui/QWhatsThis>
#include <QtGui/QKeyEvent>

// trailing slash is important
#define HLDOWNLOADPATH "http://kate.kde.org/syntax/"

//END

//BEGIN KateConfigPage
KateConfigPage::KateConfigPage ( QWidget *parent, const char * )
  : KTextEditor::ConfigPage (parent)
  , m_changed (false)
{
  connect (this, SIGNAL(changed()), this, SLOT(somethingHasChanged ()));
}

KateConfigPage::~KateConfigPage ()
{
}

void KateConfigPage::slotChanged()
{
  emit changed();
}

void KateConfigPage::somethingHasChanged ()
{
  m_changed = true;
  kDebug (13000) << "TEST: something changed on the config page: " << this << endl;
}
//END KateConfigPage

//BEGIN KateIndentConfigTab
KateIndentConfigTab::KateIndentConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  ui = new Ui::IndentationConfigWidget();
  ui->setupUi( this );

  ui->cmbMode->addItems (KateAutoIndent::listModes());

  ui->label->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
  connect(ui->label, SIGNAL(linkActivated(const QString&)), this, SLOT(showWhatsThis(const QString&)));

  // What's This? help can be found in the ui file

  reload ();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(ui->cmbMode, SIGNAL(activated(int)), this, SLOT(slotChanged()));

  connect(ui->chkKeepExtraSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->chkIndentPaste, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->chkBackspaceUnindents, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  connect(ui->sbIndentWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  connect(ui->rbTabAdvances, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->rbTabIndents, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->rbTabSmart, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
}

void KateIndentConfigTab::showWhatsThis(const QString& text)
{
  QWhatsThis::showText(QCursor::pos(), text);
}

void KateIndentConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  KateDocumentConfig::global()->configStart ();

  uint configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocumentConfig::cfKeepExtraSpaces;
  configFlags &= ~KateDocumentConfig::cfIndentPastedText;
  configFlags &= ~KateDocumentConfig::cfBackspaceIndents;

  if (ui->chkKeepExtraSpaces->isChecked()) configFlags |= KateDocumentConfig::cfKeepExtraSpaces;
  if (ui->chkIndentPaste->isChecked()) configFlags |= KateDocumentConfig::cfIndentPastedText;
  if (ui->chkBackspaceUnindents->isChecked()) configFlags |= KateDocumentConfig::cfBackspaceIndents;

  KateDocumentConfig::global()->setConfigFlags(configFlags);
  KateDocumentConfig::global()->setIndentationWidth(ui->sbIndentWidth->value());
  KateDocumentConfig::global()->setIndentationMode(KateAutoIndent::modeName(ui->cmbMode->currentIndex()));

  if (ui->rbTabAdvances->isChecked())
    KateDocumentConfig::global()->setTabHandling( KateDocumentConfig::tabInsertsTab );
  else if (ui->rbTabIndents->isChecked())
    KateDocumentConfig::global()->setTabHandling( KateDocumentConfig::tabIndents );
  else
    KateDocumentConfig::global()->setTabHandling( KateDocumentConfig::tabSmart );

  KateDocumentConfig::global()->configEnd ();
}

void KateIndentConfigTab::reload ()
{
  uint configFlags = KateDocumentConfig::global()->configFlags();

  ui->sbIndentWidth->setValue(KateDocumentConfig::global()->indentationWidth());
  ui->chkKeepExtraSpaces->setChecked(configFlags & KateDocumentConfig::cfKeepExtraSpaces);
  ui->chkIndentPaste->setChecked(configFlags & KateDocumentConfig::cfIndentPastedText);
  ui->chkBackspaceUnindents->setChecked(configFlags & KateDocumentConfig::cfBackspaceIndents);

  ui->rbTabAdvances->setChecked( KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabInsertsTab );
  ui->rbTabIndents->setChecked( KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabIndents );
  ui->rbTabSmart->setChecked( KateDocumentConfig::global()->tabHandling() == KateDocumentConfig::tabSmart );

  ui->cmbMode->setCurrentIndex (KateAutoIndent::modeNumber (KateDocumentConfig::global()->indentationMode()));
}
//END KateIndentConfigTab

//BEGIN KateSelectConfigTab
KateSelectConfigTab::KateSelectConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  uint configFlags = KateDocumentConfig::global()->configFlags();

  ui = new Ui::CursorConfigWidget();
  ui->setupUi( this );

  ui->chkSmartHome->setChecked(configFlags & KateDocumentConfig::cfSmartHome);
  connect(ui->chkSmartHome, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->chkWrapCursor->setChecked(configFlags & KateDocumentConfig::cfWrapCursor);
  connect(ui->chkWrapCursor, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->chkPagingMovesCursor->setChecked(KateDocumentConfig::global()->pageUpDownMovesCursor());
  connect(ui->chkPagingMovesCursor, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->sbAutoCenterCursor->setValue(KateViewConfig::global()->autoCenterLines());
  connect(ui->sbAutoCenterCursor, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  // What's This? Help is in the ui-files

  reload ();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(ui->rbNormal, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->rbPersistent, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
}

void KateSelectConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  KateViewConfig::global()->configStart ();
  KateDocumentConfig::global()->configStart ();

  uint configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocumentConfig::cfSmartHome;
  configFlags &= ~KateDocumentConfig::cfWrapCursor;

  if (ui->chkSmartHome->isChecked()) configFlags |= KateDocumentConfig::cfSmartHome;
  if (ui->chkWrapCursor->isChecked()) configFlags |= KateDocumentConfig::cfWrapCursor;

  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateViewConfig::global()->setAutoCenterLines(qMax(0, ui->sbAutoCenterCursor->value()));
  KateDocumentConfig::global()->setPageUpDownMovesCursor(ui->chkPagingMovesCursor->isChecked());

  KateViewConfig::global()->setPersistentSelection (ui->rbPersistent->isChecked());

  KateDocumentConfig::global()->configEnd ();
  KateViewConfig::global()->configEnd ();
}

void KateSelectConfigTab::reload ()
{
  ui->rbNormal->setChecked( ! KateViewConfig::global()->persistentSelection() );
  ui->rbPersistent->setChecked( KateViewConfig::global()->persistentSelection() );
}
//END KateSelectConfigTab

//BEGIN KateEditConfigTab
KateEditConfigTab::KateEditConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  uint configFlags = KateDocumentConfig::global()->configFlags();

  ui = new Ui::EditConfigWidget();
  ui->setupUi( this );

  ui->chkReplaceTabs->setChecked( configFlags & KateDocumentConfig::cfReplaceTabsDyn );
  connect( ui->chkReplaceTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );

  ui->chkShowTabs->setChecked( configFlags & KateDocumentConfig::cfShowTabs );
  connect(ui->chkShowTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->chkShowSpaces->setChecked( configFlags & KateDocumentConfig::cfShowSpaces );
  connect(ui->chkShowSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->sbTabWidth->setValue( KateDocumentConfig::global()->tabWidth() );
  connect(ui->sbTabWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));


  ui->chkStaticWordWrap->setChecked(KateDocumentConfig::global()->wordWrap());
  connect(ui->chkStaticWordWrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->chkShowStaticWordWrapMarker->setChecked( KateRendererConfig::global()->wordWrapMarker() );
  connect(ui->chkShowStaticWordWrapMarker, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->sbWordWrap->setValue( KateDocumentConfig::global()->wordWrapAt() );
  connect(ui->sbWordWrap, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));


  ui->chkRemoveTrailingSpaces->setChecked( configFlags & KateDocumentConfig::cfRemoveTrailingDyn );
  connect( ui->chkRemoveTrailingSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );

  ui->chkAutoBrackets->setChecked( configFlags & KateDocumentConfig::cfAutoBrackets );
  connect(ui->chkAutoBrackets, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  ui->sbMaxUndos->setValue( KateDocumentConfig::global()->undoSteps() );
  connect(ui->sbMaxUndos, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  ui->cmbSmartSearch->addItem( i18n("Nowhere") );
  ui->cmbSmartSearch->addItem( i18n("Selection Only") );
  ui->cmbSmartSearch->addItem( i18n("Selection, then Current Word") );
  ui->cmbSmartSearch->addItem( i18n("Current Word Only") );
  ui->cmbSmartSearch->addItem( i18n("Current Word, then Selection") );
  ui->cmbSmartSearch->setCurrentIndex(KateViewConfig::global()->textToSearchMode());
  connect(ui->cmbSmartSearch, SIGNAL(activated(int)), this, SLOT(slotChanged()));

  // What is this? help is in the ui-file
}

void KateEditConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  KateViewConfig::global()->configStart ();
  KateDocumentConfig::global()->configStart ();

  uint configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocumentConfig::cfAutoBrackets;
  configFlags &= ~KateDocumentConfig::cfShowTabs;
  configFlags &= ~KateDocumentConfig::cfShowSpaces;
  configFlags &= ~KateDocumentConfig::cfReplaceTabsDyn;
  configFlags &= ~KateDocumentConfig::cfRemoveTrailingDyn;

  if (ui->chkAutoBrackets->isChecked()) configFlags |= KateDocumentConfig::cfAutoBrackets;
  if (ui->chkShowTabs->isChecked()) configFlags |= KateDocumentConfig::cfShowTabs;
  if (ui->chkShowSpaces->isChecked()) configFlags |= KateDocumentConfig::cfShowSpaces;
  if (ui->chkReplaceTabs->isChecked()) configFlags |= KateDocumentConfig::cfReplaceTabsDyn;
  if (ui->chkRemoveTrailingSpaces->isChecked()) configFlags |= KateDocumentConfig::cfRemoveTrailingDyn;

  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateDocumentConfig::global()->setWordWrapAt(ui->sbWordWrap->value());
  KateDocumentConfig::global()->setWordWrap(ui->chkStaticWordWrap->isChecked());
  KateDocumentConfig::global()->setTabWidth(ui->sbTabWidth->value());

  KateDocumentConfig::global()->setUndoSteps( qMax(0,ui->sbMaxUndos->value()) );

  KateViewConfig::global()->setTextToSearchMode(ui->cmbSmartSearch->currentIndex());

  KateRendererConfig::global()->setWordWrapMarker (ui->chkShowStaticWordWrapMarker->isChecked());

  KateDocumentConfig::global()->configEnd ();
  KateViewConfig::global()->configEnd ();
}

void KateEditConfigTab::reload ()
{
}
//END KateEditConfigTab

//BEGIN KateViewDefaultsConfig
KateViewDefaultsConfig::KateViewDefaultsConfig(QWidget *parent)
  :KateConfigPage(parent)
{
  ui = new Ui::AppearanceConfigWidget();
  ui->setupUi( this );

  ui->cmbDynamicWordWrapIndicator->addItem( i18n("Off") );
  ui->cmbDynamicWordWrapIndicator->addItem( i18n("Follow Line Numbers") );
  ui->cmbDynamicWordWrapIndicator->addItem( i18n("Always On") );

  ui->chkShowIndentationLines->setChecked(KateRendererConfig::global()->showIndentationLines());

  // What's This? help is in the ui-file

  reload();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(ui->chkDynamicWordWrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->cmbDynamicWordWrapIndicator, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect(ui->sbDynamicWordWrapDepth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect(ui->chkIconBorder, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->chkScrollbarMarks, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->chkLineNumbers, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->chkShowFoldingMarkers, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->rbSortBookmarksByPosition, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->rbSortBookmarksByCreation, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(ui->chkShowIndentationLines, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
}

KateViewDefaultsConfig::~KateViewDefaultsConfig()
{
}

void KateViewDefaultsConfig::apply ()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  KateViewConfig::global()->configStart ();
  KateRendererConfig::global()->configStart ();

  KateViewConfig::global()->setDynWordWrap (ui->chkDynamicWordWrap->isChecked());
  KateViewConfig::global()->setDynWordWrapIndicators (ui->cmbDynamicWordWrapIndicator->currentIndex ());
  KateViewConfig::global()->setDynWordWrapAlignIndent(ui->sbDynamicWordWrapDepth->value());
  KateViewConfig::global()->setLineNumbers (ui->chkLineNumbers->isChecked());
  KateViewConfig::global()->setIconBar (ui->chkIconBorder->isChecked());
  KateViewConfig::global()->setScrollBarMarks (ui->chkScrollbarMarks->isChecked());
  KateViewConfig::global()->setFoldingBar (ui->chkShowFoldingMarkers->isChecked());

  KateViewConfig::global()->setBookmarkSort (ui->rbSortBookmarksByPosition->isChecked()?0:1);
  KateRendererConfig::global()->setShowIndentationLines(ui->chkShowIndentationLines->isChecked());

  KateRendererConfig::global()->configEnd ();
  KateViewConfig::global()->configEnd ();
}

void KateViewDefaultsConfig::reload ()
{
  ui->chkDynamicWordWrap->setChecked(KateViewConfig::global()->dynWordWrap());
  ui->cmbDynamicWordWrapIndicator->setCurrentIndex( KateViewConfig::global()->dynWordWrapIndicators() );
  ui->sbDynamicWordWrapDepth->setValue(KateViewConfig::global()->dynWordWrapAlignIndent());
  ui->chkLineNumbers->setChecked(KateViewConfig::global()->lineNumbers());
  ui->chkIconBorder->setChecked(KateViewConfig::global()->iconBar());
  ui->chkScrollbarMarks->setChecked(KateViewConfig::global()->scrollBarMarks());
  ui->chkShowFoldingMarkers->setChecked(KateViewConfig::global()->foldingBar());
  ui->rbSortBookmarksByPosition->setChecked(KateViewConfig::global()->bookmarkSort()==0);
  ui->rbSortBookmarksByCreation->setChecked(KateViewConfig::global()->bookmarkSort()==1);
  ui->chkShowIndentationLines->setChecked(KateRendererConfig::global()->showIndentationLines());
}

void KateViewDefaultsConfig::reset () {;}

void KateViewDefaultsConfig::defaults (){;}
//END KateViewDefaultsConfig

//BEGIN KateEditKeyConfiguration

KateEditKeyConfiguration::KateEditKeyConfiguration( QWidget* parent, KateDocument* doc )
  : KateConfigPage( parent )
{
  m_doc = doc;
  m_ready = false;
}

void KateEditKeyConfiguration::showEvent ( QShowEvent * )
{
#ifdef __GNUC__
#warning fixme, to work without a document object, perhaps create some own internally
#endif
  return ;

  if (!m_ready)
  {
    QVBoxLayout *l=new QVBoxLayout(this);
    KateView* view = (KateView*)m_doc->views().at(0);
    m_ac = view->editActionCollection();
    l->addWidget(m_shortcutsEditor = new KShortcutsEditor( m_ac, this, false ));
    //is this really needed? if yes, I'll add it to KShortcutsEditor
    //note that changes will immediately become active with KShortcutsEditor -- ahartmetz
    //connect( m_shortcutsEditor, SIGNAL( keyChange() ), this, SLOT( slotChanged() ) );
    m_shortcutsEditor->show ();

    m_ready = true;
  }

  QWidget::show ();
}

void KateEditKeyConfiguration::apply()
{
#ifdef __GNUC__
#warning fixme, to work without a document object, perhaps create some own internally
#endif
  return ;

  if ( ! hasChanged() )
    return;
  m_changed = false;

  if (m_ready)
  {
#ifdef __GNUC__
#warning: semantics of KKeyDialog changed from change/commit to change in-place/revert
#endif
    //m_keyChooser->commitChanges();
    m_ac->writeSettings();
  }
}
//END KateEditKeyConfiguration

//BEGIN KateSaveConfigTab
KateSaveConfigTab::KateSaveConfigTab( QWidget *parent )
  : KateConfigPage( parent )
{
  ui = new Ui::OpenSaveConfigWidget();
  ui->setupUi( this );
//  layout->setSpacing( KDialog::spacingHint() );

  // What's this help is added in ui/opensaveconfigwidget.ui
  reload();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect( ui->cmbEncoding, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect( ui->cmbEncodingDetection, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect( ui->cmbEOL, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect( ui->chkDetectEOL, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( ui->chkRemoveTrailingSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect( ui->chkBackupLocalFiles, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( ui->chkBackupRemoteFiles, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( ui->sbConfigFileSearchDepth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect( ui->edtBackupPrefix, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( ui->edtBackupSuffix, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
}

void KateSaveConfigTab::apply()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  KateDocumentConfig::global()->configStart ();

  if ( ui->edtBackupSuffix->text().isEmpty() && ui->edtBackupPrefix->text().isEmpty() ) {
    KMessageBox::information(
                this,
                i18n("You did not provide a backup suffix or prefix. Using default suffix: '~'"),
                i18n("No Backup Suffix or Prefix")
                        );
    ui->edtBackupSuffix->setText( "~" );
  }

  uint f( 0 );
  if ( ui->chkBackupLocalFiles->isChecked() )
    f |= KateDocumentConfig::LocalFiles;
  if ( ui->chkBackupRemoteFiles->isChecked() )
    f |= KateDocumentConfig::RemoteFiles;

  KateDocumentConfig::global()->setBackupFlags(f);
  KateDocumentConfig::global()->setBackupPrefix(ui->edtBackupPrefix->text());
  KateDocumentConfig::global()->setBackupSuffix(ui->edtBackupSuffix->text());

  KateDocumentConfig::global()->setSearchDirConfigDepth(ui->sbConfigFileSearchDepth->value());

  uint configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocumentConfig::cfRemoveSpaces; // clear flag
  if (ui->chkRemoveTrailingSpaces->isChecked()) configFlags |= KateDocumentConfig::cfRemoveSpaces; // set flag if checked

  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateDocumentConfig::global()->setEncoding((ui->cmbEncoding->currentIndex() == 0) ? "" : KGlobal::charsets()->encodingForName(ui->cmbEncoding->currentText()));
  KateDocumentConfig::global()->setEncodingAutoDetectionScript(
      (KEncodingDetector::AutoDetectScript)ui->cmbEncodingDetection->itemData(ui->cmbEncodingDetection->currentIndex()).toUInt());

  KateDocumentConfig::global()->setEol(ui->cmbEOL->currentIndex());
  KateDocumentConfig::global()->setAllowEolDetection(ui->chkDetectEOL->isChecked());

  KateDocumentConfig::global()->configEnd ();
}

void KateSaveConfigTab::reload()
{
  // encoding
  ui->cmbEncoding->clear ();
  ui->cmbEncoding->addItem (i18n("KDE Default"));
  ui->cmbEncoding->setCurrentIndex(0);
  QStringList encodings (KGlobal::charsets()->descriptiveEncodingNames());
  int insert = 1;
  for (int i=0; i < encodings.count(); i++)
  {
    bool found = false;
    QTextCodec *codecForEnc = KGlobal::charsets()->codecForName(KGlobal::charsets()->encodingForName(encodings[i]), found);

    if (found)
    {
      ui->cmbEncoding->addItem (encodings[i]);

      if ( codecForEnc->name() == KateDocumentConfig::global()->encoding() )
      {
        ui->cmbEncoding->setCurrentIndex(insert);
      }

      insert++;
    }
  }

  // encoding detection
  ui->cmbEncodingDetection->clear ();
  ui->cmbEncodingDetection->addItem (i18n("Disabled"));
  ui->cmbEncodingDetection->setCurrentIndex(0);

  foreach(const QStringList &encodingsForScript, KGlobal::charsets()->encodingsByScript())
  {
    KEncodingDetector::AutoDetectScript scri=KEncodingDetector::scriptForName(encodingsForScript.at(0));
    if (KEncodingDetector::hasAutoDetectionForScript(scri))
    {
      ui->cmbEncodingDetection->addItem (encodingsForScript.at(0),QVariant((uint)scri));
      if (scri==KateDocumentConfig::global()->encodingAutoDetectionScript())
        ui->cmbEncodingDetection->setCurrentIndex(ui->cmbEncodingDetection->count()-1);
    }
  }

  // eol
  ui->cmbEOL->setCurrentIndex(KateDocumentConfig::global()->eol());
  ui->chkDetectEOL->setChecked(KateDocumentConfig::global()->allowEolDetection());

  const uint configFlags = KateDocumentConfig::global()->configFlags();
  ui->chkRemoveTrailingSpaces->setChecked(configFlags & KateDocumentConfig::cfRemoveSpaces);
  ui->sbConfigFileSearchDepth->setValue(KateDocumentConfig::global()->searchDirConfigDepth());

  // other stuff
  uint f ( KateDocumentConfig::global()->backupFlags() );
  ui->chkBackupLocalFiles->setChecked( f & KateDocumentConfig::LocalFiles );
  ui->chkBackupRemoteFiles->setChecked( f & KateDocumentConfig::RemoteFiles );
  ui->edtBackupPrefix->setText( KateDocumentConfig::global()->backupPrefix() );
  ui->edtBackupSuffix->setText( KateDocumentConfig::global()->backupSuffix() );
}

void KateSaveConfigTab::reset()
{
}

void KateSaveConfigTab::defaults()
{
  ui->chkBackupLocalFiles->setChecked( true );
  ui->chkBackupRemoteFiles->setChecked( false );
  ui->edtBackupPrefix->setText( "" );
  ui->edtBackupSuffix->setText( "~" );
}

//END KateSaveConfigTab

//BEGIN PluginListItem
class KatePartPluginListItem : public QTreeWidgetItem
{
  public:
    KatePartPluginListItem(bool checked, uint i, const QString &name, QTreeWidget *parent);
    uint pluginIndex () const { return index; }

  private:
    uint index;
};

KatePartPluginListItem::KatePartPluginListItem(bool checked, uint i, const QString &name, QTreeWidget *parent)
  : QTreeWidgetItem(parent)
  , index(i)
{
  setText(0, name);
  setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
}
//END

//BEGIN PluginListView
KatePartPluginListView::KatePartPluginListView(QWidget *parent)
  : QTreeWidget(parent)
{
}
//END

//BEGIN KatePartPluginConfigPage
KatePartPluginConfigPage::KatePartPluginConfigPage (QWidget *parent) : KateConfigPage (parent, "")
{
  // sizemanagment
  QGridLayout *grid = new QGridLayout( this); //, 1, 1 );
  grid->setSpacing( KDialog::spacingHint() );

  listView = new KatePartPluginListView(this);
  listView->setColumnCount(2);
  listView->setHeaderLabels(QStringList() << i18n("Name") << i18n("Comment"));
  listView->setRootIsDecorated(false);

  grid->addWidget(listView, 0, 0);

  for (int i=0; i<KateGlobal::self()->plugins().count(); i++)
  {
    KatePartPluginListItem *item = new KatePartPluginListItem(KateDocumentConfig::global()->plugin(i), i, (KateGlobal::self()->plugins())[i]->name(), listView);
    item->setText(0, (KateGlobal::self()->plugins())[i]->name());
    item->setText(1, (KateGlobal::self()->plugins())[i]->comment());

    m_items.append (item);
  }
  listView->resizeColumnToContents(0);

  // configure button

  btnConfigure = new QPushButton( i18n("Configure..."), this );
  btnConfigure->setEnabled( false );
  grid->addWidget( btnConfigure, 1, 0, Qt::AlignRight );
  connect( btnConfigure, SIGNAL(clicked()), this, SLOT(slotConfigure()) );

  connect( listView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(slotCurrentChanged(QTreeWidgetItem*)) );
  connect( listView, SIGNAL(itemChanged (QTreeWidgetItem *, int)), this, SLOT(slotChanged()));
}

KatePartPluginConfigPage::~KatePartPluginConfigPage ()
{
  qDeleteAll(m_items);
}

void KatePartPluginConfigPage::apply ()
{
  kDebug()<<"KatePartPluginConfigPage::apply (entered)"<<endl;
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  kDebug()<<"KatePartPluginConfigPage::apply (need to store configuration)"<<endl;

  KateDocumentConfig::global()->configStart ();

  for (int i=0; i < m_items.count(); i++)
    KateDocumentConfig::global()->setPlugin (m_items.at(i)->pluginIndex(), m_items.at(i)->checkState(0) == Qt::Checked);

  KateDocumentConfig::global()->configEnd ();
}

void KatePartPluginConfigPage::slotCurrentChanged( QTreeWidgetItem* i )
{
  KatePartPluginListItem *item = static_cast<KatePartPluginListItem *>(i);
  if ( ! item ) return;

    bool b = false;
  if ( item->checkState(0) == Qt::Checked )
  {
    // load this plugin, and see if it has config pages
    KTextEditor::Plugin *plugin = KTextEditor::createPlugin(QFile::encodeName((KateGlobal::self()->plugins())[item->pluginIndex()]->library()), 0);
    if ( plugin ) {
      b = plugin->configDialogSupported();
      delete plugin;
    }
  }

  btnConfigure->setEnabled( b );
}

void KatePartPluginConfigPage::slotConfigure()
{
  KatePartPluginListItem *item = static_cast<KatePartPluginListItem*>(listView->currentItem());
  KTextEditor::Plugin *plugin =
    KTextEditor::createPlugin(QFile::encodeName((KateGlobal::self()->plugins())[item->pluginIndex()]->library()), 0);

  if ( ! plugin ) return;

  plugin->configDialog (this);

  delete plugin;
}
//END KatePartPluginConfigPage

class KateScriptNewStuff {};

/*
class KateScriptNewStuff: public KNewStuff {
  public:
    KateScriptNewStuff(QWidget *parent):KNewStuff("kate/scripts",parent) {}
    virtual ~KateScriptNewStuff() {}
    virtual bool install( const QString &fileName ) {return false;}
    virtual bool createUploadFile( const QString &fileName ) {return false;}
};
*/
//BEGIN KateScriptConfigPage
KateScriptConfigPage::KateScriptConfigPage(QWidget *parent): KateConfigPage(parent,""), m_newStuff(new KateScriptNewStuff())
{
  //m_newStuff->download();
}

KateScriptConfigPage::~KateScriptConfigPage()
{
  delete m_newStuff;
  m_newStuff=0;
}

void KateScriptConfigPage::apply () {
}
void KateScriptConfigPage::reload () {
}

//END KateScriptConfigPage

//BEGIN KateHlConfigPage
KateHlConfigPage::KateHlConfigPage (QWidget *parent, KateDocument *doc)
 : KateConfigPage (parent, "")
 , m_doc (doc)
{
  ui = new Ui::HlConfigWidget();
  ui->setupUi( this );

  for( int i = 0; i < KateHlManager::self()->highlights(); i++) {
    if (KateHlManager::self()->hlSection(i).length() > 0)
      ui->cmbHl->addItem(KateHlManager::self()->hlSection(i) + QString ("/")
          + KateHlManager::self()->hlNameTranslated(i));
    else
      ui->cmbHl->addItem(KateHlManager::self()->hlNameTranslated(i));
  }
  
  connect( ui->btnDownload, SIGNAL(clicked()), this, SLOT(hlDownload()) );
  connect( ui->cmbHl, SIGNAL(activated(int)), this, SLOT(hlChanged(int)) );

  int currentHl = m_doc ? m_doc->hlMode() : 0;
  ui->cmbHl->setCurrentIndex( currentHl );
  hlChanged( currentHl );
}

KateHlConfigPage::~KateHlConfigPage ()
{
}

void KateHlConfigPage::apply ()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;
}

void KateHlConfigPage::reload ()
{
  // TODO: implement me
}

void KateHlConfigPage::hlChanged(int z)
{
  writeback();

  KateHighlighting *hl = KateHlManager::self()->getHl( z );

  if (!hl)
  {
    return;
  }

  // split author string if needed into multiple lines !
  QStringList l= hl->author().split (QRegExp("[,;]"));
  ui->txtAuthor->setText (l.join ("<br>"));

  ui->txtLicense->setText (hl->license());
}

void KateHlConfigPage::writeback()
{
}

void KateHlConfigPage::hlDownload()
{
  KateHlDownloadDialog diag(this,"hlDownload",true);
  diag.exec();
}

void KateHlConfigPage::showMTDlg()
{
}
//END KateHlConfigPage

//BEGIN KateHlDownloadDialog
KateHlDownloadDialog::KateHlDownloadDialog(QWidget *parent, const char *name, bool modal)
  : KDialog( parent )
{
  setCaption( i18n("Highlight Download") );
  setButtons( User1 | Close );
  setButtonGuiItem( User1, KGuiItem(i18n("&Install")) );
  setDefaultButton( User1 );
  setObjectName( name );
  setModal( modal );
  showButtonSeparator( true );

  KVBox* vbox = new KVBox(this);
  setMainWidget(vbox);
  vbox->setSpacing(spacingHint());
  new QLabel(i18n("Select the syntax highlighting files you want to update:"), vbox);
  list = new QTreeWidget(vbox);
  list->setColumnCount(4);
  list->setHeaderLabels(QStringList() << "" << i18n("Name") << i18n("Installed") << i18n("Latest"));
  list->setSelectionMode(QAbstractItemView::MultiSelection);
  // KDE4 replacement?
  //list->setAllColumnsShowFocus(true);

  new QLabel(i18n("<b>Note:</b> New versions are selected automatically."), vbox);
  setButtonIcon(User1, KIcon("ok"));

  transferJob = KIO::get(
    KUrl(QString(HLDOWNLOADPATH)
       + QString("update-")
       + QString(KATEPART_VERSION)
       + QString(".xml")), true, true );
  connect(transferJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
    this, SLOT(listDataReceived(KIO::Job *, const QByteArray &)));
//        void data( KIO::Job *, const QByteArray &data);
  resize(450, 400);
  connect(this,SIGNAL(user1Clicked()),this,SLOT(slotUser1()));
}

KateHlDownloadDialog::~KateHlDownloadDialog(){}

void KateHlDownloadDialog::listDataReceived(KIO::Job *, const QByteArray &data)
{
  if (!transferJob || transferJob->isErrorPage())
  {
    enableButton( User1, false );
    return;
  }

  listData+=QString(data);
  kDebug(13000)<<QString("CurrentListData: ")<<listData<<endl<<endl;
  kDebug(13000)<<QString("Data length: %1").arg(data.size())<<endl;
  kDebug(13000)<<QString("listData length: %1").arg(listData.length())<<endl;
  if (data.size()==0)
  {
    if (listData.length()>0)
    {
      QString installedVersion;
      KateHlManager *hlm=KateHlManager::self();
      QDomDocument doc;
      doc.setContent(listData);
      QDomElement DocElem=doc.documentElement();
      QDomNode n=DocElem.firstChild();
      KateHighlighting *hl = 0;

      if (n.isNull()) kDebug(13000)<<"There is no usable childnode"<<endl;
      while (!n.isNull())
      {
        installedVersion="    --";

        QDomElement e=n.toElement();
        if (!e.isNull())
        kDebug(13000)<<QString("NAME: ")<<e.tagName()<<QString(" - ")<<e.attribute("name")<<endl;
        n=n.nextSibling();

        QString Name=e.attribute("name");

        for (int i=0;i<hlm->highlights();i++)
        {
          hl=hlm->getHl(i);
          if (hl && hl->name()==Name)
          {
            installedVersion="    "+hl->version();
            break;
          }
          else hl = 0;
        }

        // autoselect entry if new or updated.
        QTreeWidgetItem* entry = new QTreeWidgetItem(list);
        entry->setText(0, "");
        entry->setText(1, e.attribute("name"));
        entry->setText(2, installedVersion);
        entry->setText(3, e.attribute("version"));
        entry->setText(4, e.attribute("url"));

        if (!hl || hl->version() < e.attribute("version"))
        {
          entry->treeWidget()->setItemSelected(entry, true);
          entry->setIcon(0, SmallIcon(("get-hot-new-stuff")));
        }
      }
    }
  }
}

void KateHlDownloadDialog::slotUser1()
{
  QString destdir=KGlobal::dirs()->saveLocation("data","katepart/syntax/");
  foreach (QTreeWidgetItem *it, list->selectedItems())
  {
    KUrl src(it->text(4));
    QString filename=src.fileName(KUrl::ObeyTrailingSlash);
    QString dest = destdir+filename;

    KIO::NetAccess::download(src,dest, this);
  }

  // update Config !!
  // this rewrites the cache....
  KateSyntaxDocument doc (KateHlManager::self()->getKConfig(), true);
}
//END KateHlDownloadDialog

//BEGIN KateGotoBar
KateGotoBar::KateGotoBar(KateViewBar *parent)
  : KateViewBarWidget( parent )
{
  QHBoxLayout *topLayout = new QHBoxLayout( centralWidget() );
  topLayout->setMargin(0);
  //topLayout->setSpacing(spacingHint());
  gotoRange = new QSpinBox(centralWidget());

  QLabel *label = new QLabel(i18n("&Go to line:"), centralWidget() );
  label->setBuddy(gotoRange);

  btnOK = new QToolButton();
  btnOK->setAutoRaise(true);
  btnOK->setIcon(QIcon(SmallIcon("goto-page")));
  btnOK->setText(i18n("Go"));
  btnOK->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  connect(btnOK, SIGNAL(clicked()), this, SLOT(gotoLine()));

  topLayout->addWidget(label);
  topLayout->addWidget(gotoRange, 1);
  topLayout->setStretchFactor( gotoRange, 0 );
  topLayout->addWidget(btnOK);
  topLayout->addStretch();
}

void KateGotoBar::showBar()
{
  KateView* view = viewBar()->view();
  gotoRange->setMaximum(view->doc()->lines());
  if (!isVisible())
  {
    gotoRange->setValue(view->cursorPosition().line() + 1);
    gotoRange->adjustSize(); // ### does not respect the range :-(
  }
  gotoRange->setFocus(Qt::OtherFocusReason);

  KateViewBarWidget::showBar();
}

void KateGotoBar::keyPressEvent(QKeyEvent* event)
{
  int key = event->key();
  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    gotoLine();
    return;
  }
  KateViewBarWidget::keyPressEvent(event);
}

void KateGotoBar::gotoLine()
{
  viewBar()->view()->setCursorPosition( KTextEditor::Cursor(gotoRange->value() - 1, 0) );
  viewBar()->view()->setFocus();
  hideBar();
}
//END KateGotoBar

//BEGIN KateModOnHdPrompt
KateModOnHdPrompt::KateModOnHdPrompt( KateDocument *doc,
                                      KTextEditor::ModificationInterface::ModifiedOnDiskReason modtype,
                                      const QString &reason,
                                      QWidget *parent )
  : KDialog( parent ),
    m_doc( doc ),
    m_modtype ( modtype ),
    m_tmpfile( 0 )
{
  setButtons( Ok | Apply | Cancel | User1 );

  QString title, btnOK, whatisok;
  if ( modtype == KTextEditor::ModificationInterface::OnDiskDeleted )
  {
    title = i18n("File Was Deleted on Disk");
    btnOK = i18n("&Save File As...");
    whatisok = i18n("Lets you select a location and save the file again.");
  } else {
    title = i18n("File Changed on Disk");
    btnOK = i18n("&Reload File");
    whatisok = i18n("Reload the file from disk. If you have unsaved changes, "
        "they will be lost.");
  }

  setButtonText( Ok, btnOK );
  setButtonText( Apply, i18n("&Ignore") );

  setButtonWhatsThis( Ok, whatisok );
  setButtonWhatsThis( Apply, i18n("Ignore the changes. You will not be prompted again.") );
  setButtonWhatsThis( Cancel, i18n("Do nothing. Next time you focus the file, "
      "or try to save it or close it, you will be prompted again.") );

  showButtonSeparator( true );
  setCaption( title );

  QWidget *w = new QWidget(this);
  ui = new Ui::ModOnHdWidget();
  ui->setupUi( w );
  setMainWidget( w );

  ui->lblIcon->setPixmap( DesktopIcon("dialog-warning" ) );
  ui->lblText->setText( reason + "\n\n" + i18n("What do you want to do?") );

  // If the file isn't deleted, present a diff button, and a overwrite action.
  if ( modtype != KTextEditor::ModificationInterface::OnDiskDeleted )
  {
    setButtonText( User1, i18n("Overwrite") );
    setButtonWhatsThis( User1, i18n("Overwrite the disk file with the editor content.") );
    connect( ui->btnDiff, SIGNAL(clicked()), this, SLOT(slotDiff()) );
  }
  else
  {
    ui->chkIgnoreWhiteSpaces->setVisible( false );
    ui->btnDiff->setVisible( false );
    showButton( User1, false );
  }
}

KateModOnHdPrompt::~KateModOnHdPrompt()
{
}

void KateModOnHdPrompt::slotDiff()
{
  // Start a K3Process that creates a diff
  K3ProcIO *p = new K3ProcIO();
  p->setComm( K3Process::All );
  *p << "diff" << QString(ui->chkIgnoreWhiteSpaces->isChecked() ? "-ub" : "-u")
     << "-" <<  m_doc->url().path();
  connect( p, SIGNAL(processExited(K3Process*)), this, SLOT(slotPDone(K3Process*)) );
  connect( p, SIGNAL(readReady(K3ProcIO*)), this, SLOT(slotPRead(K3ProcIO*)) );

  setCursor( Qt::WaitCursor );
  // disable the button and checkbox, to hinder the user to run it twice.
  ui->chkIgnoreWhiteSpaces->setEnabled( false );
  ui->btnDiff->setEnabled( false );

  p->start( K3Process::NotifyOnExit, true );

  int lastln =  m_doc->lines();
  for ( int l = 0; l <  lastln; ++l )
    p->writeStdin(  m_doc->line( l ) );

  p->closeWhenDone();
}

void KateModOnHdPrompt::slotPRead( K3ProcIO *p)
{
  // create a file for the diff if we haven't one already
  if ( ! m_tmpfile ) {
    m_tmpfile = new KTemporaryFile();
    m_tmpfile->setAutoRemove(false);
    m_tmpfile->open();
  }

  QTextStream stream(m_tmpfile);

  // put all the data we have in it
  QString stmp;
  bool readData = false;
  while ( p->readln( stmp, false ) > -1 )
  {
    stream << stmp << endl;
    readData = true;
  }
  stream.flush();

  // dominik: only ackRead(), when we *really* read data, otherwise, this slot
  // is called infinity times, which leads to a crash (#123887)
  if( readData )
    p->ackRead();
}

void KateModOnHdPrompt::slotPDone( K3Process *p )
{
  setCursor( Qt::ArrowCursor );
  ui->chkIgnoreWhiteSpaces->setEnabled( true );
  ui->btnDiff->setEnabled( true );

  // dominik: whitespace changes lead to diff with 0 bytes, so that slotPRead is
  // never called. Thus, m_tmpfile can be NULL
  if( m_tmpfile )
    m_tmpfile->close();

  if ( ! p->normalExit() /*|| p->exitStatus()*/ )
  {
    KMessageBox::sorry( this,
                        i18n("The diff command failed. Please make sure that "
                             "diff(1) is installed and in your PATH."),
                        i18n("Error Creating Diff") );
    delete m_tmpfile;
    m_tmpfile = 0;
    return;
  }

  if ( ! m_tmpfile )
  {
    KMessageBox::information( this,
                              i18n("Besides white space changes, the files are identical."),
                              i18n("Diff Output") );
    return;
  }

  KRun::runUrl( KUrl::fromPath(m_tmpfile->fileName()), "text/x-patch", this, true );
  delete m_tmpfile;
  m_tmpfile = 0;
}

void KateModOnHdPrompt::slotButtonClicked(int button)
{
  switch(button)
  {
    case Default:
    case Ok:
      done( (m_modtype == KTextEditor::ModificationInterface::OnDiskDeleted) ?
            Save : Reload );
      break;
    case Apply:
    {
      if ( KMessageBox::warningContinueCancel(
           this,
           i18n("Ignoring means that you will not be warned again (unless "
           "the disk file changes once more): if you save the document, you "
           "will overwrite the file on disk; if you do not save then the disk "
           "file (if present) is what you have."),
           i18n("You Are on Your Own"),
           KStandardGuiItem::cont(),
           KStandardGuiItem::cancel(),
           "kate_ignore_modonhd" ) != KMessageBox::Continue )
        return;
      done( Ignore );
      break;
    }
    case User1:
      done( Overwrite );
      break;
    default:
      KDialog::slotButtonClicked(button);
  }
}

//END KateModOnHdPrompt

// kate: space-indent on; indent-width 2; replace-tabs on;
