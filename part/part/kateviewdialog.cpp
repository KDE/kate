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

#include "kateviewdialog.h"
#include "katesearch.h"
#include "katedocument.h"
#include "katefactory.h"

#include <stdio.h>
#include <stdlib.h>

#include <qgrid.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qtabwidget.h>
#include <qspinbox.h>
#include <kcombobox.h>
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcollection.h>
#include <qpushbutton.h>
#include <qobjectlist.h>
#include <qradiobutton.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
#include <qstringlist.h>
#include <klocale.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <qvbox.h>
#include <kfontdialog.h>
#include <kregexpeditorinterface.h>
#include <qdialog.h>
#include <kparts/componentfactory.h>
#include <kkeybutton.h>
#include <klistview.h>
#include <qlayout.h>
#include <kconfig.h>
#include <assert.h>

#include <kmainwindow.h>
#include <kaccel.h>
#include <kkeydialog.h>

SearchDialog::SearchDialog( QWidget *parent, QStringList &searchFor,
     QStringList &replaceWith, SearchFlags flags )
  : KDialogBase( parent, 0L, true, i18n( "Find Text" ), Ok | Cancel, Ok )
    , m_replace( 0L ), m_regExpDialog( 0L )
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  page->setMinimumWidth(350);

  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  m_search = new KComboBox( true, page );
  m_search->insertStringList( searchFor );
  m_search->setMinimumWidth( m_search->sizeHint().width() );
  m_search->lineEdit()->selectAll();
  QLabel *label = new QLabel( m_search, i18n( "Find:" ), page );
  m_optRegExp = new QCheckBox( i18n( "&Regular expression" ), page );

  topLayout->addWidget( label );
  topLayout->addWidget( m_search );

  QHBoxLayout* regexpLayout = new QHBoxLayout( topLayout );
  regexpLayout->addWidget( m_optRegExp );

  // Add the Edit button if KRegExp exists.
  if ( !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty() ) {
    QPushButton* regexpButton = new QPushButton( i18n("&Edit..."), page );

    regexpLayout->addWidget( regexpButton );
    regexpLayout->addStretch(1);

    connect( regexpButton, SIGNAL( clicked() ), this, SLOT( slotEditRegExp() ) );
    connect( m_optRegExp, SIGNAL( toggled(bool) ), regexpButton, SLOT( setEnabled(bool) ) );
    regexpButton->setEnabled( false );
  }

  if( flags.replace )
  {
    // make it a replace dialog
    setCaption( i18n( "Replace Text" ) );
    m_replace = new KComboBox( true, page );
    m_replace->insertStringList( replaceWith );
    m_replace->setMinimumWidth( m_search->sizeHint().width() );
    label = new QLabel( m_replace, i18n( "&Replace with:" ), page );
    topLayout->addWidget( label );
    topLayout->addWidget( m_replace );
  }

  QGroupBox *group = new QGroupBox( i18n( "Options" ), page );
  topLayout->addWidget( group, 10 );

  QGridLayout *gbox = new QGridLayout( group, 5, 2, spacingHint() );
  gbox->addRowSpacing( 0, fontMetrics().lineSpacing() );
  gbox->setRowStretch( 4, 10 );

  m_opt1 = new QCheckBox( i18n( "C&ase sensitive" ), group );
  gbox->addWidget( m_opt1, 1, 0 );

  m_opt2 = new QCheckBox(i18n("&Whole words only" ), group );
  gbox->addWidget( m_opt2, 2, 0 );

  m_opt4 = new QCheckBox(i18n("Find &backwards" ), group );
  gbox->addWidget( m_opt4, 1, 1 );

  m_opt3 = new QCheckBox(i18n("&From beginning" ), group );
  gbox->addWidget( m_opt3, 2, 1 );

  if( flags.replace )
  {
    m_opt5 = new QCheckBox(i18n("&Selected text" ), group );
    gbox->addWidget( m_opt5, 2, 1 );
    m_opt6 = new QCheckBox( i18n( "&Prompt on replace" ), group );
    gbox->addWidget( m_opt6, 3, 1 );
    connect(m_opt5, SIGNAL(stateChanged(int)), this, SLOT(selectedStateChanged(int)));
    connect(m_opt6, SIGNAL(stateChanged(int)), this, SLOT(selectedStateChanged(int)));
    m_opt5->setChecked( flags.selected );
    m_opt6->setChecked( flags.prompt );
  }

  m_opt1->setChecked( flags.caseSensitive );
  m_opt2->setChecked( flags.wholeWords );
  m_opt3->setChecked( flags.fromBeginning );
  m_optRegExp->setChecked( flags.regExp );
  m_opt4->setChecked( flags.backward );

  m_search->setFocus();
}

QString SearchDialog::getSearchFor()
{
  return m_search->currentText();
}

void SearchDialog::selectedStateChanged (int)
{
  if (m_opt6->isChecked())
    m_opt5->setEnabled (false);
  else
    m_opt5->setEnabled (true);

  if (m_opt5->isChecked())
    m_opt6->setEnabled (false);
  else
    m_opt6->setEnabled (true);
}

QString SearchDialog::getReplaceWith()
{
  return m_replace->currentText();
}

SearchFlags SearchDialog::getFlags()
{
  SearchFlags flags;

  flags.caseSensitive   = m_opt1->isChecked();
  flags.wholeWords      = m_opt2->isChecked();
  flags.fromBeginning   = m_opt3->isChecked();
  flags.backward        = m_opt4->isChecked();
  flags.regExp          = m_optRegExp->isChecked();
  flags.prompt          = m_replace && m_opt6->isChecked();
  flags.selected        = m_replace && m_opt5->isChecked();
  flags.replace         = m_replace;

  return flags;
}

void SearchDialog::slotOk()
{
  if ( !m_search->currentText().isEmpty() )
  {
    if ( !m_optRegExp->isChecked() )
    {
      accept();
    }
    else
    {
      // Check for a valid regular expression.

      QRegExp regExp( m_search->currentText() );

      if ( regExp.isValid() )
        accept();
    }
  }
}

void SearchDialog::slotEditRegExp()
{
  if ( m_regExpDialog == 0 )
    m_regExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString::null, this );

  assert( m_regExpDialog );

  KRegExpEditorInterface *iface = static_cast<KRegExpEditorInterface *>( m_regExpDialog->qt_cast( "KRegExpEditorInterface" ) );
  if (!iface)
    return;

  iface->setRegExp( m_search->currentText() );
  int ok = m_regExpDialog->exec();
  if (ok == QDialog::Accepted) {
    m_search->setCurrentText( iface->regExp() );    
  }
}



void SearchDialog::setSearchText( const QString &searchstr )
  {
   m_search->insertItem( searchstr, 0 );
   m_search->setCurrentItem( 0 );
   m_search->lineEdit()->selectAll();
 }

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

GotoLineDialog::GotoLineDialog(QWidget *parent, int line, int max)
  : KDialogBase(parent, 0L, true, i18n("Goto Line"), Ok | Cancel, Ok) {

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  e1 = new KIntNumInput(line, page);
  e1->setRange(1, max);
  e1->setEditFocus(true);

  QLabel *label = new QLabel( e1,i18n("&Goto Line:"), page );
  topLayout->addWidget(label);
  topLayout->addWidget(e1);
  topLayout->addSpacing(spacingHint()); // A little bit extra space
  topLayout->addStretch(10);
  e1->setFocus();
}

int GotoLineDialog::getLine() {
  return e1->value();
}

const int IndentConfigTab::flags[] = {KateDocument::cfAutoIndent, KateDocument::cfSpaceIndent,
  KateDocument::cfBackspaceIndents,KateDocument::cfTabIndents, KateDocument::cfKeepIndentProfile, KateDocument::cfKeepExtraSpaces};

IndentConfigTab::IndentConfigTab(QWidget *parent, KateDocument *view)
  : Kate::ConfigPage(parent)
{
  m_doc = view;

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = view->configFlags();

  opt[0] = new QCheckBox(i18n("&Automatically indent"), this);
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
//  opt[4]->setChecked(configFlags & flags[4]);
  opt[4]->setChecked(true);
  opt[4]->hide();

  opt[5] = new QCheckBox(i18n("&Keep extra spaces"), this);
  layout->addWidget(opt[5], 0, AlignLeft);
  opt[5]->setChecked(configFlags & flags[5]);

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("When <b>Automatically indent</b> is on, KateView will indent new lines to equal the indent on the previous line.<p>If the previous line is blank, the nearest line above with text is used"));
  QWhatsThis::add(opt[1], i18n("Check this if you want to indent with spaces rather than tabs.<br>A Tab will be converted to <u>Tab-width</u> as set in the <b>Edit</b> options"));
  QWhatsThis::add(opt[2], i18n("This allows the <b>backspace</b> key to be used to decrease the indent level."));
  QWhatsThis::add(opt[3], i18n("This allows the <b>tab</b> key to be used to increase the indent level."));
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

  opt[1] = new QCheckBox(i18n("&Overwrite selected text"), this);
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

  opt[3] = new QCheckBox(i18n("&Auto brackets"), this);
  mainLayout->addWidget(opt[3]);
  opt[3]->setChecked(configFlags & flags[3]);

  opt[5] = new QCheckBox(i18n("Smart &home"), this);
  mainLayout->addWidget(opt[5]);
  opt[5]->setChecked(configFlags & flags[5]);

  opt[6] = new QCheckBox(i18n("Wrap &cursor"), this);
  mainLayout->addWidget(opt[6]);
  opt[6]->setChecked(configFlags & flags[6]);

  e3 = new KIntNumInput(e2, view->undoSteps(), this);
  e3->setRange(0, 1000, 1, false);
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


ViewDefaultsConfig::ViewDefaultsConfig(QWidget *parent, const char*, KateDocument *doc)
	:Kate::ConfigPage(parent)
{
	m_doc = doc;
	
	QVBoxLayout *blay=new QVBoxLayout(this,KDialog::spacingHint());
	m_line=new QCheckBox(i18n("Show line numbers"),this);
	m_icons=new QCheckBox(i18n("Show iconborder"),this);
	m_folding=new QCheckBox(i18n("Show folding markers if available"),this);
	blay->addWidget(m_line,0);
	blay->addWidget(m_icons,0);
	blay->addWidget(m_folding,0);	
	blay->addStretch(1000);
	reload();
	}


ViewDefaultsConfig::~ViewDefaultsConfig()
{
}


void ViewDefaultsConfig::apply ()
{
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate ViewDefaults");
  config->writeEntry( "LineNumbers", m_line->isChecked() );
  config->writeEntry( "Iconbar", m_icons->isChecked() );  
  config->writeEntry( "FoldingMarkers", m_folding->isChecked() );  
  config->sync();
}

void ViewDefaultsConfig::reload ()
{
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate ViewDefaults");
  m_line->setChecked(config->readBoolEntry( "LineNumbers", false ));
  m_icons->setChecked(config->readBoolEntry( "Iconbar", false ));  
  m_folding->setChecked(config->readBoolEntry( "FoldingMarkers", true ));  
}    

void ViewDefaultsConfig::reset () {;}

void ViewDefaultsConfig::defaults (){;}

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
  
  label = new QLabel( i18n("Current Line:"), this);
  label->setAlignment( AlignRight|AlignVCenter );
  m_current = new KColorButton( this );
  glay->addWidget( label, 4, 0 );
  glay->addWidget( m_current, 4, 1 );

  label = new QLabel( i18n("Bracket Highlight:"), this );
  label->setAlignment( AlignRight|AlignVCenter );
  m_bracket = new KColorButton( this );
  glay->addWidget( label, 6, 0 );
  glay->addWidget( m_bracket, 6, 1 );

  // QWhatsThis help
  QWhatsThis::add(m_back, i18n("Sets the background color of the editing area"));
  QWhatsThis::add(m_selected, i18n("Sets the background color of the selection. To set the text color for selected text, use the \"<b>Configure Highlighting</b>\" dialog."));
  QWhatsThis::add(m_current, i18n("Sets the background color of the currently active line, which means the line where your cursor is positioned."));

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

EditKeyConfiguration::EditKeyConfiguration(QWidget *parent, const char *): Kate::ConfigPage(parent)
{
	(new QVBoxLayout(this))->setAutoAdd(true);
	tmpWin=new KMainWindow(0);
	tmpWin->hide();
	setupEditKeys();
  	KConfig config("kateeditkeysrc");
	m_editAccels->readSettings(&config);
	chooser=new KKeyChooser(m_editAccels,this);
}

void EditKeyConfiguration::dummy()
{}


void EditKeyConfiguration::setupEditKeys()
{
  m_editAccels=new KAccel(tmpWin);
  m_editAccels->insert("KATE_CURSOR_LEFT",i18n("Cursor left"),"",Key_Left,this,SLOT(dummy()));
  m_editAccels->insert("KATE_WORD_LEFT",i18n("One word left"),"",CTRL+Key_Left,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_LEFT_SELECT",i18n("Cursor left + SELECT"),"",SHIFT+Key_Left,this,SLOT(dummy()));
  m_editAccels->insert("KATE_WORD_LEFT_SELECT",i18n("One word left + SELECT"),"",SHIFT+CTRL+Key_Left,this,SLOT(dummy()));


  m_editAccels->insert("KATE_CURSOR_RIGHT",i18n("Cursor right"),"",Key_Right,this,SLOT(dummy()));
  m_editAccels->insert("KATE_WORD_RIGHT",i18n("One word right"),"",CTRL+Key_Right,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_RIGHT_SELECT",i18n("Cursor right + SELECT"),"",SHIFT+Key_Right,this,SLOT(dummy()));
  m_editAccels->insert("KATE_WORD_RIGHT_SELECT",i18n("One word right + SELECT"),"",SHIFT+CTRL+Key_Right,this,SLOT(dummy()));

  m_editAccels->insert("KATE_CURSOR_HOME",i18n("Home"),"",Key_Home,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_TOP",i18n("Top"),"",CTRL+Key_Home,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_HOME_SELECT",i18n("Home + SELECT"),"",SHIFT+Key_Home,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_TOP_SELECT",i18n("Top + SELECT"),"",SHIFT+CTRL+Key_Home,this,SLOT(dummy()));

  m_editAccels->insert("KATE_CURSOR_END",i18n("End"),"",Key_End,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_BOTTOM",i18n("Bottom"),"",CTRL+Key_End,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_END_SELECT",i18n("End + SELECT"),"",SHIFT+Key_End,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_BOTTOM_SELECT",i18n("Bottom + SELECT"),"",SHIFT+CTRL+Key_End,this,SLOT(dummy()));

  m_editAccels->insert("KATE_CURSOR_UP",i18n("Cursor up"),"",Key_Up,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_UP_SELECT",i18n("Cursor up + SELECT"),"",SHIFT+Key_Up,this,SLOT(dummy()));
  m_editAccels->insert("KATE_SCROLL_UP",i18n("Scroll one line up"),"",CTRL+Key_Up,this,SLOT(dummy()));

  m_editAccels->insert("KATE_CURSOR_DOWN",i18n("Cursor down"),"",Key_Down,this,SLOT(dummy()));
  m_editAccels->insert("KATE_CURSOR_DOWN_SELECT",i18n("Cursor down + SELECT"),"",SHIFT+Key_Down,this,SLOT(dummy()));
  m_editAccels->insert("KATE_SCROLL_DOWN",i18n("Scroll one line down"),"",CTRL+Key_Down,this,SLOT(dummy()));
  m_editAccels->insert("KATE TRANSPOSE", i18n("Transpose two adjacent characters"),"",CTRL+Key_T,this,SLOT(transpose()));
}

void EditKeyConfiguration::save()
{
  chooser->commitChanges();
  KConfig config("kateeditkeysrc");
  m_editAccels->updateConnections();
  m_editAccels->writeSettings(&config);
  config.sync();
}

void EditKeyConfiguration::apply()
{
  save ();
}

void EditKeyConfiguration::reload ()
{
}

EditKeyConfiguration::~EditKeyConfiguration()
{
	delete tmpWin;
}

#include "kateviewdialog.moc"



