/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

//BEGIN Includes
#include "kateviewdialog.h"
#include "kateviewdialog.moc"

#include "katesearch.h"
#include "katedocument.h"
#include "kateview.h"
#include "katefactory.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <kaccel.h>
#include <kcharsets.h>
#include <kcolorbutton.h>
#include <kglobal.h>
#include <kkeybutton.h>
#include <kkeydialog.h>
#include <klistview.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kfontdialog.h>
#include <knuminput.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qptrcollection.h>
#include <qdialog.h>
#include <qgrid.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qobjectlist.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
//END Includes

//BEGIN ReplacePrompt
// this dialog is not modal
ReplacePrompt::ReplacePrompt( QWidget *parent )
  : KDialogBase(parent, 0L, true, i18n( "Replace Text" ),
  User3 | User2 | User1 | Close, User3, true,
  i18n("&All"), i18n("&No"), i18n("&Yes")) {

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  QLabel *label = new QLabel(i18n("Replace this occurence?"),page);
  topLayout->addWidget(label );
}

void ReplacePrompt::slotUser1( void ) { // All
  done(KateSearch::srAll);
}

void ReplacePrompt::slotUser2( void ) { // No
  done(KateSearch::srNo);
}

void ReplacePrompt::slotUser3( void ) { // Yes
  accept();
}

void ReplacePrompt::done(int r) {
  setResult(r);
  emit clicked();
}
//END ReplacePrompt

//BEGIN GotoLineDialog
GotoLineDialog::GotoLineDialog(QWidget *parent, int line, int max)
  : KDialogBase(parent, 0L, true, i18n("Goto Line"), Ok | Cancel, Ok) {

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  e1 = new KIntNumInput(line, page);
  e1->setRange(1, max);
  e1->setEditFocus(true);

  QLabel *label = new QLabel( e1,i18n("&Go to line:"), page );
  topLayout->addWidget(label);
  topLayout->addWidget(e1);
  topLayout->addSpacing(spacingHint()); // A little bit extra space
  topLayout->addStretch(10);
  e1->setFocus();
}

int GotoLineDialog::getLine() {
  return e1->value();
}
//END GotoLineDialog

//BEGIN IndentConfigTab
const int IndentConfigTab::flags[] = {KateDocument::cfAutoIndent, KateDocument::cfSpaceIndent,
  KateDocument::cfBackspaceIndents,KateDocument::cfTabIndents, KateDocument::cfKeepIndentProfile, KateDocument::cfKeepExtraSpaces};

IndentConfigTab::IndentConfigTab(QWidget *parent, KateDocument *view)
  : Kate::ConfigPage(parent)
{
  m_doc = view;

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = view->configFlags();

  opt[0] = new QCheckBox(i18n("A&utomatically indent"), this);
  layout->addWidget(opt[0], 0, AlignLeft);
  opt[0]->setChecked(configFlags & flags[0]);

  opt[1] = new QCheckBox(i18n("Use &spaces to indent"), this);
  layout->addWidget(opt[1], 0, AlignLeft);
  opt[1]->setChecked(configFlags & flags[1]);

  opt[3] = new QCheckBox(i18n("&Tab key indents"), this);
  layout->addWidget(opt[3], 0, AlignLeft);
  opt[3]->setChecked(configFlags & flags[3]);

  opt[2] = new QCheckBox(i18n("&Backspace key indents"), this);
  layout->addWidget(opt[2], 0, AlignLeft);
  opt[2]->setChecked(configFlags & flags[2]);

  opt[4] = new QCheckBox(i18n("Keep indent &profile"), this);
  layout->addWidget(opt[4], 0, AlignLeft);
  opt[4]->setChecked(configFlags & flags[4]);
//   opt[4]->setChecked(true);
//   opt[4]->hide();

  opt[5] = new QCheckBox(i18n("&Keep extra spaces"), this);
  layout->addWidget(opt[5], 0, AlignLeft);
  opt[5]->setChecked(configFlags & flags[5]);

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("When <b>Automatically indent</b> is on, KateView will indent new lines to equal the indent on the previous line.<p>If the previous line is blank, the nearest line above with text is used"));
  QWhatsThis::add(opt[1], i18n("Check this if you want to indent with spaces rather than tabs.<br>A Tab will be converted to <u>Tab-width</u> as set in the <b>Edit</b> options"));
  QWhatsThis::add(opt[2], i18n("This allows the <b>Backspace</b> key to be used to decrease the indent level."));
  QWhatsThis::add(opt[3], i18n("This allows the <b>Tab</b> key to be used to increase the indent level."));
  QWhatsThis::add(opt[4], i18n("This retains current indentation settings for future documents."));
  QWhatsThis::add(opt[5], i18n("Indentations of more than the selected number of spaces will not be shortened."));
}

void IndentConfigTab::getData(KateDocument *view) {
  int configFlags, z;

  configFlags = view->configFlags();
  for (z = 0; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }
  view->setConfigFlags(configFlags);
}

void IndentConfigTab::apply ()
{
  getData(m_doc);
}


void IndentConfigTab::reload ()
{

}
//END IncentConfigTab

//BEGIN SelectConfigTab
const int SelectConfigTab::flags[] = {KateDocument::cfPersistent, KateDocument::cfDelOnInput};

SelectConfigTab::SelectConfigTab(QWidget *parent, KateDocument *view)
  : Kate::ConfigPage(parent)
{
  m_doc = view;

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = view->configFlags();

  opt[0] = new QCheckBox(i18n("&Persistent selections"), this);
  layout->addWidget(opt[0], 0, AlignLeft);
  opt[0]->setChecked(configFlags & flags[0]);

  opt[1] = new QCheckBox(i18n("O&verwrite selected text"), this);
  layout->addWidget(opt[1], 0, AlignLeft);
  opt[1]->setChecked(configFlags & flags[1]);

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("Enabling this prevents key input or cursor movement by way of the arrow keys from causing the elimination of text selection.<p><b>Note:</b> If the Overwrite Selections option is activated then any typed character input or paste operation will replace the selected text."));
  QWhatsThis::add(opt[1], i18n("When this is on, any keyed character input or paste operation will replace the selected text."));
}

void SelectConfigTab::getData(KateDocument *view) {
  int configFlags, z;

  configFlags = view->configFlags();
  for (z = 0; z < numFlags; z++) {
    configFlags &= ~flags[z]; // clear flag
    if (opt[z]->isChecked()) configFlags |= flags[z]; // set flag if checked
  }
  view->setConfigFlags(configFlags);
}

void SelectConfigTab::apply ()
{
  getData (m_doc);
}

void SelectConfigTab::reload ()
{

}
//END SelectConfigTab

//BEGIN EditConfigTab
const int EditConfigTab::flags[] = {KateDocument::cfWordWrap, KateDocument::cfReplaceTabs, KateDocument::cfRemoveSpaces,
  KateDocument::cfAutoBrackets, KateDocument::cfShowTabs, KateDocument::cfSmartHome, KateDocument::cfWrapCursor};

EditConfigTab::EditConfigTab(QWidget *parent, KateDocument *view)
  : Kate::ConfigPage(parent) {

  QVBoxLayout *mainLayout;
  int configFlags;
  m_doc = view;

  mainLayout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  configFlags = view->configFlags();

  QVGroupBox *gbWordWrap = new QVGroupBox(i18n("Word Wrap"), this);

  opt[0] = new QCheckBox(i18n("Enable &word wrap"), gbWordWrap);
  opt[0]->setChecked(view->wordWrap());
  connect(opt[0], SIGNAL(toggled(bool)), this, SLOT(wordWrapToggled()));

  e1 = new KIntNumInput(view->wordWrapAt(), gbWordWrap);
  e1->setRange(20, 200, 1, false);
  e1->setLabel(i18n("Wrap words at:"), AlignVCenter);

  mainLayout->addWidget(gbWordWrap);

  QVGroupBox *gbWhiteSpace = new QVGroupBox(i18n("Whitespace"), this);

  opt[4] = new QCheckBox(i18n("&Show tabs"), gbWhiteSpace);
  opt[4]->setChecked(configFlags & flags[4]);

  opt[1] = new QCheckBox(i18n("Replace &tabs with spaces"), gbWhiteSpace);
  opt[1]->setChecked(configFlags & flags[1]);

  opt[2] = new QCheckBox(i18n("&Remove trailing spaces"), gbWhiteSpace);
  opt[2]->setChecked(configFlags & flags[2]);

  e2 = new KIntNumInput(e1, view->tabWidth(), gbWhiteSpace);
  e2->setRange(1, 16, 1, false);
  e2->setLabel(i18n("Tab and indent width:"), AlignVCenter);

  mainLayout->addWidget(gbWhiteSpace);

  opt[3] = new QCheckBox(i18n("Auto &brackets"), this);
  mainLayout->addWidget(opt[3]);
  opt[3]->setChecked(configFlags & flags[3]);

  opt[5] = new QCheckBox(i18n("Smart ho&me"), this);
  mainLayout->addWidget(opt[5]);
  opt[5]->setChecked(configFlags & flags[5]);

  opt[6] = new QCheckBox(i18n("Wrap c&ursor"), this);
  mainLayout->addWidget(opt[6]);
  opt[6]->setChecked(configFlags & flags[6]);

  e3 = new KIntNumInput(e2, view->undoSteps(), this);
  e3->setRange(0, 1000000, 1, false);
  e3->setSpecialValueText( i18n("Unlimited") );
  e3->setLabel(i18n("Maximum undo steps:"), AlignVCenter);
  mainLayout->addWidget(e3);

  mainLayout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("Word wrap is a feature that causes the editor to automatically start a new line of text and move (wrap) the cursor to the beginning of that new line. KateView will automatically start a new line of text when the current line reaches the length specified by the Wrap Words At: option.<p><b>NOTE:</b> Word Wrap will not change existing lines or wrap them for easy reading as in some applications."));
  QWhatsThis::add(e1, i18n("If the Word Wrap option is selected this entry determines the length (in characters) at which the editor will automatically start a new line."));
  QWhatsThis::add(opt[1], i18n("KateView will replace any tabs with the number of spaces indicated in the Tab Width: entry."));
  QWhatsThis::add(e2, i18n("If the Replace Tabs By Spaces option is selected this entry determines the number of spaces with which the editor will automatically replace tabs."));
  QWhatsThis::add(opt[2], i18n("KateView will automatically eliminate extra spaces at the ends of lines of text."));
  QWhatsThis::add(opt[3], i18n("When the user types a left bracket ([,(, or {) KateView automatically enters the right bracket (}, ), or ]) to the right of the cursor."));
  QWhatsThis::add(opt[4], i18n("The editor will display a symbol to indicate the presence of a tab in the text."));
  QWhatsThis::add(opt[5], i18n("Not yet implemented."));
  QWhatsThis::add(e3, i18n("Sets the number of undo/redo steps to record. More steps uses more memory."));
  QWhatsThis::add(opt[6], i18n("When on, moving the insertion cursor using the <b>Left</b> and <b>Right</b> keys will go on to previous/next line at beginning/end of the line, similar to most editors.<p>When off, the insertion cursor cannot be moved left of the line start, but it can be moved off the line end, which can be very handy for programmers."));

  wordWrapToggled();
}

void EditConfigTab::getData(KateDocument *view)
{
  int configFlags, z;

  configFlags = view->configFlags();
  for (z = 1; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }
  view->setConfigFlags(configFlags);

  view->setWordWrapAt(e1->value());
  view->setWordWrap (opt[0]->isChecked());
  view->setTabWidth(e2->value());

  if (e3->value() <= 0)
    view->setUndoSteps(0);
  else
    view->setUndoSteps(e3->value());
}

void EditConfigTab::apply ()
{
  getData (m_doc);
}

void EditConfigTab::reload ()
{

}

void EditConfigTab::wordWrapToggled() {
  e1->setEnabled(opt[0]->isChecked());
}
//END EditConfigTab

//BEGIN ViewDefaultsConfig
ViewDefaultsConfig::ViewDefaultsConfig(QWidget *parent, const char*, KateDocument *doc)
	:Kate::ConfigPage(parent)
{

	QRadioButton *rb1;
	QRadioButton *rb2;

	m_doc = doc;

	QVBoxLayout *blay=new QVBoxLayout(this,KDialog::spacingHint());
  m_dynwrap=new QCheckBox(i18n("&Dynamic Word Wrap"),this);
  m_wwmarker = new QCheckBox( i18n("Show Word Wrap Marker (if applicable)"), this ); 
	m_line=new QCheckBox(i18n("Show &line numbers"),this);
	m_icons=new QCheckBox(i18n("Show &icon border"),this);
	m_folding=new QCheckBox(i18n("Show &folding markers if available"),this);
        m_bmSort = new QButtonGroup( 1, Qt::Horizontal, i18n("Sort Bookmarks Menu"), this );
        m_bmSort->setRadioButtonExclusive( true );
        m_bmSort->insert( rb1=new QRadioButton( i18n("By &position"), m_bmSort ), 0 );
        m_bmSort->insert( rb2=new QRadioButton( i18n("By c&reation"), m_bmSort ), 1 );
	blay->addWidget(m_dynwrap,0);
        blay->addWidget( m_wwmarker, 0 );
  blay->addWidget(m_line,0);
	blay->addWidget(m_icons,0);
	blay->addWidget(m_folding,0);
        blay->addWidget( m_bmSort, 0 );
	blay->addStretch(1000);

  QWhatsThis::add(m_dynwrap,i18n("If this option is checked, the text lines will be wrapped at the view border on the screen."));
  QWhatsThis::add( m_wwmarker, i18n(
        "<p>If this option is checked, a vertical line will be drawn at the word "
        "wrap column as defined  in the <strong>Editing</streong> properties."
        "<p>Note that the word wrap marker is only drawn if you use a fixed "
        "pitch font." ));
	QWhatsThis::add(m_line,i18n("If this option is checked, every new view will display line numbers on the left hand side."));
	QWhatsThis::add(m_icons,i18n("If this option is checked, every new view will display an icon border on the left hand side.<br><br>The icon border shows bookmark signs, for instance."));
	QWhatsThis::add(m_folding,i18n("If this option is checked, every new view will display marks for code folding, if code folding is available."));

	QWhatsThis::add(m_bmSort,i18n("Choose how the bookmarks should be ordered in the <b>Bookmarks</b> menu."));
	QWhatsThis::add(rb1,i18n("The bookmarks will be ordered by the line numbers they are placed at."));
	QWhatsThis::add(rb2,i18n("Each new bookmark will be added to the bottom, independently from where it is placed in the document."));
	reload();
}

ViewDefaultsConfig::~ViewDefaultsConfig()
{
}

void ViewDefaultsConfig::apply ()
{
  m_doc->m_dynWordWrap = m_dynwrap->isChecked();
  m_doc->m_wordWrapMarker = m_wwmarker->isChecked();
  m_doc->m_lineNumbers = m_line->isChecked();
  m_doc->m_iconBar = m_icons->isChecked();
  m_doc->m_foldingBar = m_folding->isChecked();
  m_doc->m_bookmarkSort = m_bmSort->id (m_bmSort->selected());
}

void ViewDefaultsConfig::reload ()
{
  m_dynwrap->setChecked(m_doc->m_dynWordWrap);
  m_wwmarker->setChecked( m_doc->m_wordWrapMarker );
  m_line->setChecked(m_doc->m_lineNumbers);
  m_icons->setChecked(m_doc->m_iconBar);
  m_folding->setChecked(m_doc->m_foldingBar);
  m_bmSort->setButton( m_doc->m_bookmarkSort  );
}

void ViewDefaultsConfig::reset () {;}

void ViewDefaultsConfig::defaults (){;}
//END ViewDefaultsConfig

//BEGIN ColorConfig
ColorConfig::ColorConfig( QWidget *parent, const char *, KateDocument *doc )
  : Kate::ConfigPage(parent)
{
  m_doc = doc;

  QGridLayout *glay = new QGridLayout( this, 8, 2, 0, KDialog::spacingHint());
  glay->setColStretch(1,1);
  glay->setRowStretch(7,1);

  QLabel *label;

  label = new QLabel( i18n("Background:"), this);
  label->setAlignment( AlignRight|AlignVCenter );
  m_back = new KColorButton( this );
  glay->addWidget( label, 0, 0 );
  glay->addWidget( m_back, 0, 1 );

  label = new QLabel( i18n("Selected:"), this);
  label->setAlignment( AlignRight|AlignVCenter );
  m_selected = new KColorButton( this );
  glay->addWidget( label, 2, 0 );
  glay->addWidget( m_selected, 2, 1 );

  label = new QLabel( i18n("Current line:"), this);
  label->setAlignment( AlignRight|AlignVCenter );
  m_current = new KColorButton( this );
  glay->addWidget( label, 4, 0 );
  glay->addWidget( m_current, 4, 1 );

  label = new QLabel( i18n("Bracket highlight:"), this );
  label->setAlignment( AlignRight|AlignVCenter );
  m_bracket = new KColorButton( this );
  glay->addWidget( label, 6, 0 );
  glay->addWidget( m_bracket, 6, 1 );

  // QWhatsThis help
  QWhatsThis::add(m_back, i18n("Sets the background color of the editing area"));
  QWhatsThis::add(m_selected, i18n("Sets the background color of the selection. To set the text color for selected text, use the \"<b>Configure Highlighting</b>\" dialog."));
  QWhatsThis::add(m_current, i18n("Sets the background color of the currently active line, which means the line where your cursor is positioned."));
  QWhatsThis::add(m_bracket, i18n("Sets the bracket matching color. This means, if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will be highlighted with this color."));
  reload ();
}


ColorConfig::~ColorConfig()
{
}

void ColorConfig::setColors(QColor *colors)
{
  m_back->setColor( colors[0] );
  m_selected->setColor( colors[1] );
  m_current->setColor( colors[2] );
  m_bracket->setColor( colors[3] );
}

void ColorConfig::getColors(QColor *colors)
{
  colors[0] = m_back->color();
  colors[1] = m_selected->color();
  colors[2] = m_current->color();
  colors[3] = m_bracket->color();
}

void ColorConfig::apply ()
{
  getColors(m_doc->colors);
}

void ColorConfig::reload ()
{
  setColors(m_doc->colors);
}
//END ColorConfig

//BEGIN FontConfig
FontConfig::FontConfig( QWidget *parent, const char *, KateDocument *doc )
  : Kate::ConfigPage(parent)
{
  m_doc = doc;

    // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  QTabWidget *tab = new QTabWidget (this);
  tab->setMargin(KDialog::marginHint());
  grid->addWidget( tab, 0, 0);

  m_fontchooser = new KFontChooser ( tab, 0L, false, QStringList(), false );
  m_fontchooser->enableColumn(KFontChooser::StyleList, false);
  tab->addTab (m_fontchooser, i18n("Display Font"));

  m_fontchooserPrint = new KFontChooser ( tab, 0L, false, QStringList(), false );
  m_fontchooserPrint->enableColumn(KFontChooser::StyleList, false);
  tab->addTab (m_fontchooserPrint, i18n("Printer Font"));

  tab->show ();

  connect (m_fontchooser, SIGNAL (fontSelected( const QFont & )), this, SLOT (slotFontSelected( const QFont & )));
  connect (m_fontchooserPrint, SIGNAL (fontSelected( const QFont & )), this, SLOT (slotFontSelectedPrint( const QFont & )));

  reload ();
}

FontConfig::~FontConfig()
{
}

void FontConfig::setFont ( const QFont &font )
{
  m_fontchooser->setFont (font);
  myFont = font;
}

void FontConfig::slotFontSelected( const QFont &font )
{
  myFont = font;
}

void FontConfig::setFontPrint ( const QFont &font )
{
  m_fontchooserPrint->setFont (font);
  myFontPrint = font;
}

void FontConfig::slotFontSelectedPrint( const QFont &font )
{
  myFontPrint = font;
}

void FontConfig::apply ()
{
  m_doc->setFont (KateDocument::ViewFont,getFont());
  m_doc->setFont (KateDocument::PrintFont,getFontPrint());
}

void FontConfig::reload ()
{
  setFont (m_doc->getFont(KateDocument::ViewFont));
  setFontPrint (m_doc->getFont(KateDocument::PrintFont));
}
//END FontConfig

//BEGIN EditKeyConfiguration


EditKeyConfiguration::EditKeyConfiguration( QWidget* parent, KateDocument* doc )
  : Kate::ConfigPage( parent )
{
  m_doc = doc;
  m_ready = false;
}

void EditKeyConfiguration::showEvent ( QShowEvent * )
{
  if (!m_ready)
  {
    (new QVBoxLayout(this))->setAutoAdd(true);
    KateView* view = (KateView*)m_doc->views().at(0);
    m_keyChooser = new KKeyChooser( view->editActionCollection(), this, false );
    m_keyChooser->show ();

    m_ready = true;
  }

  QWidget::show ();
}

void EditKeyConfiguration::apply()
{
  if (m_ready)
  {
    m_keyChooser->save();
  }
}
//END EditKeyConfiguration

//BEGIN SaveConfigTab
SaveConfigTab::SaveConfigTab( QWidget *parent, KateDocument *doc )
  : Kate::ConfigPage( parent ),
    m_doc( doc )
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  QGroupBox *gb = new QGroupBox( 1, Qt::Horizontal, i18n("Backup on Save"), this );
  layout->addWidget( gb );
  cbLocalFiles = new QCheckBox( i18n("&Local Files"), gb );
  cbRemoteFiles = new QCheckBox( i18n("&Remote Files"), gb );
  QHBox *hbBuSuffix = new QHBox( gb );
  QLabel *lBuSuffix = new QLabel( i18n("&Suffix:"), hbBuSuffix );
  leBuSuffix = new QLineEdit( hbBuSuffix );
  lBuSuffix->setBuddy( leBuSuffix );
  
  layout->addStretch();
  
  QWhatsThis::add( gb, i18n(
        "<p>Backing up on save will cause kate to copy the disk file to "
        "'&lt;filename&gt;&lt;suffix&gt;' before saving changes."
        "<p>The prefix defaults to <strong>~</strong>" ) );
  QWhatsThis::add( cbLocalFiles, i18n(
        "Check this if you want backups of local files when saving") );
  QWhatsThis::add( cbRemoteFiles, i18n(
        "Check this if you want backups of remote files when saving") );
  QWhatsThis::add( leBuSuffix, i18n(
        "Enter the suffix to add to the backup file names" ) );
  
  reload();
}

void SaveConfigTab::apply()
{
  if ( leBuSuffix->text().isEmpty() ) {
    KMessageBox::information(
                this,
                i18n("You didn't provide a backup suffix, using default value of '~'"),
                i18n("No Backup Suffix")
                        );
    leBuSuffix->setText( "~" );     
  }
  uint f( 0 );
  if ( cbLocalFiles->isChecked() )
    f |= KateDocument::LocalFiles;
  if ( cbRemoteFiles->isChecked() )
    f |= KateDocument::RemoteFiles;
  m_doc->setBackupConfig( f );
  m_doc->setBackupSuffix( leBuSuffix->text() );
}

void SaveConfigTab::reload()
{
  uint f ( m_doc->backupConfig() );
  cbLocalFiles->setChecked( f & KateDocument::LocalFiles );
  cbRemoteFiles->setChecked( f & KateDocument::RemoteFiles );
  leBuSuffix->setText( m_doc->backupSuffix() );
}

void SaveConfigTab::reset()
{
}

void SaveConfigTab::defaults()
{
  cbLocalFiles->setChecked( true );
  cbRemoteFiles->setChecked( false );
  leBuSuffix->setText( "~" );
  //apply(); // ? the base classes are terribly undocumented!!!
}

//END SaveConfigTab                            
