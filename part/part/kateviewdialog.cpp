/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

//BEGIN Includes
#include "kateviewdialog.h"
#include "kateviewdialog.moc"

#include "katesearch.h"
#include "katedocument.h"
#include "kateconfig.h"
#include "kateview.h"
#include "katefactory.h"
#include "katerenderer.h"
#include "kateautoindent.h"
#include "kateschema.h"

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
#include <qtextcodec.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qhbox.h>
#include <qobjectlist.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
//END Includes

//BEGIN ReplacePrompt
// this dialog is not modal
ReplacePrompt::ReplacePrompt( QWidget *parent )
  : KDialogBase(parent, 0L, false, i18n( "Replace Text" ),
  User3 | User2 | User1 | Close | Ok , Ok, true,
  i18n("&All"), i18n("&Last"), i18n("&No") ) {

  setButtonOKText( i18n("&Yes") );
  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  QLabel *label = new QLabel(i18n("Replace this occurrence?"),page);
  topLayout->addWidget(label );
}

void ReplacePrompt::slotOk( void ) { // Yes
  done(KateSearch::srYes);
}

void ReplacePrompt::slotClose( void ) { // Close
  done(KateSearch::srCancel);
}

void ReplacePrompt::slotUser1( void ) { // All
  done(KateSearch::srAll);
}

void ReplacePrompt::slotUser2( void ) { // Last
  done(KateSearch::srLast);
}

void ReplacePrompt::slotUser3( void ) { // No
  done(KateSearch::srNo);
}

void ReplacePrompt::done(int r) {
  setResult(r);
  emit clicked();
}
//END ReplacePrompt

//BEGIN GotoLineDialog
GotoLineDialog::GotoLineDialog(QWidget *parent, int line, int max)
  : KDialogBase(parent, 0L, true, i18n("Go to Line"), Ok | Cancel, Ok) {

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
  int configFlags = KateDocumentConfig::global()->configFlags();

  QVGroupBox *gbAuto = new QVGroupBox(i18n("Automatic Indentation"), this);

  opt[0] = new QCheckBox(i18n("A&ctivated"), gbAuto);
  opt[0]->setChecked(configFlags & flags[0]);
  layout->addWidget(opt[0]);
  connect( opt[0], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  QHBox *e5Layout = new QHBox(gbAuto);
  QLabel *e5Label = new QLabel(i18n("&Indentation mode:"), e5Layout);
  m_indentMode = new KComboBox (e5Layout);
  m_indentMode->insertStringList (KateAutoIndent::listModes());
  e5Label->setBuddy(m_indentMode);
  connect(m_indentMode, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  layout->addWidget(gbAuto);

  opt[4] = new QCheckBox(i18n("Keep indent &profile"), this);
  opt[4]->setChecked(configFlags & flags[4]);
  layout->addWidget(opt[4]);
  connect( opt[4], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  opt[5] = new QCheckBox(i18n("&Keep extra spaces"), this);
  opt[5]->setChecked(configFlags & flags[5]);
  layout->addWidget(opt[5]);
  connect( opt[5], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  QVGroupBox *gbWordWrap = new QVGroupBox(i18n("Indentation with Spaces"), this);

  opt[1] = new QCheckBox(i18n("Use &spaces instead of tabs to indent"), gbWordWrap);
  opt[1]->setChecked(configFlags & flags[1]);
  connect( opt[1], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( opt[1], SIGNAL(toggled(bool)), this, SLOT(spacesToggled()));

  indentationWidth = new KIntNumInput(KateDocumentConfig::global()->indentationWidth(), gbWordWrap);
  indentationWidth->setRange(1, 16, 1, false);
  indentationWidth->setLabel(i18n("Number of spaces:"), AlignVCenter);
  connect(indentationWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  layout->addWidget(gbWordWrap);

  QVGroupBox *keys = new QVGroupBox(i18n("Keys to Use"), this);

  opt[3] = new QCheckBox(i18n("&Tab key indents"), keys);
  opt[3]->setChecked(configFlags & flags[3]);
  connect( opt[3], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  opt[2] = new QCheckBox(i18n("&Backspace key indents"), keys);
  opt[2]->setChecked(configFlags & flags[2]);
  connect( opt[2], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  layout->addWidget(keys);

  QRadioButton *rb1, *rb2, *rb3;

  m_tabs = new QButtonGroup( 1, Qt::Horizontal, i18n("Tab Key Mode if Nothing Selected"), this );
  m_tabs->setRadioButtonExclusive( true );
  m_tabs->insert( rb1=new QRadioButton( i18n("Insert indent &chars"), m_tabs ), 0 );
  m_tabs->insert( rb2=new QRadioButton( i18n("I&nsert tab char"), m_tabs ), 1 );
  m_tabs->insert( rb3=new QRadioButton( i18n("Indent current &line"), m_tabs ), 2 );

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  layout->addWidget(m_tabs, 0 );

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("When <b>Automatically indent</b> is on, KateView will indent new lines to equal the indentation on the previous line.<p>If the previous line is blank, the nearest line above with text is used."));
  QWhatsThis::add(opt[1], i18n("Check this if you want to indent with spaces rather than tabs."));
  QWhatsThis::add(opt[2], i18n("This allows the <b>Backspace</b> key to be used to decrease the indentation level."));
  QWhatsThis::add(opt[3], i18n("This allows the <b>Tab</b> key to be used to increase the indentation level."));
  QWhatsThis::add(opt[4], i18n("This retains current indentation settings for future documents."));
  QWhatsThis::add(opt[5], i18n("Indentations of more than the selected number of spaces will not be shortened."));
  QWhatsThis::add(indentationWidth, i18n("The number of spaces to indent with."));

  reload ();
}

void IndentConfigTab::spacesToggled() {
  indentationWidth->setEnabled(opt[1]->isChecked());
}

void IndentConfigTab::apply ()
{
  int configFlags, z;

  configFlags = KateDocumentConfig::global()->configFlags();
  for (z = 0; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }
  KateDocumentConfig::global()->setConfigFlags(configFlags);
  KateDocumentConfig::global()->setIndentationWidth(indentationWidth->value());

  KateDocumentConfig::global()->setIndentationMode(m_indentMode->currentItem());

  KateDocumentConfig::global()->setConfigFlags (KateDocumentConfig::cfTabIndentsMode, 2 == m_tabs->id (m_tabs->selected()));
  KateDocumentConfig::global()->setConfigFlags (KateDocumentConfig::cfTabInsertsTab, 1 == m_tabs->id (m_tabs->selected()));
}

void IndentConfigTab::reload ()
{
  if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfTabIndentsMode)
    m_tabs->setButton (2);
  else if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfTabInsertsTab)
    m_tabs->setButton (1);
  else
    m_tabs->setButton (0);

  m_indentMode->setCurrentItem (KateDocumentConfig::global()->indentationMode());

  spacesToggled ();
}
//END IndentConfigTab

//BEGIN SelectConfigTab
const int SelectConfigTab::flags[] = {KateDocument::cfPersistent, KateDocument::cfDelOnInput};

SelectConfigTab::SelectConfigTab(QWidget *parent, KateDocument *view)
  : Kate::ConfigPage(parent)
{
  m_doc = view;

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = KateDocumentConfig::global()->configFlags();

  opt[0] = new QCheckBox(i18n("&Persistent selections"), this);
  layout->addWidget(opt[0], 0, AlignLeft);
  opt[0]->setChecked(configFlags & flags[0]);
  connect( opt[0], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  opt[1] = new QCheckBox(i18n("O&verwrite selected text"), this);
  layout->addWidget(opt[1], 0, AlignLeft);
  opt[1]->setChecked(configFlags & flags[1]);
  connect( opt[1], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("Enabling this prevents key input or cursor movement by way of the arrow keys from causing the elimination of text selection.<p><b>Note:</b> If the Overwrite Selections option is activated then any typed character input or paste operation will replace the selected text."));
  QWhatsThis::add(opt[1], i18n("When this is on, any keyed character input or paste operation will replace the selected text."));
}

void SelectConfigTab::apply ()
{
   int configFlags, z;

  configFlags = KateDocumentConfig::global()->configFlags();
  for (z = 0; z < numFlags; z++) {
    configFlags &= ~flags[z]; // clear flag
    if (opt[z]->isChecked()) configFlags |= flags[z]; // set flag if checked
  }
  KateDocumentConfig::global()->setConfigFlags(configFlags);
}

void SelectConfigTab::reload ()
{

}
//END SelectConfigTab

//BEGIN EditConfigTab
const int EditConfigTab::flags[] = {KateDocument::cfWordWrap,
  KateDocument::cfAutoBrackets, KateDocument::cfShowTabs, KateDocument::cfSmartHome, KateDocument::cfWrapCursor};

EditConfigTab::EditConfigTab(QWidget *parent, KateDocument *view)
  : Kate::ConfigPage(parent)
{
  m_doc = view;

  QVBoxLayout *mainLayout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = KateDocumentConfig::global()->configFlags();

  QVGroupBox *gbWhiteSpace = new QVGroupBox(i18n("Tabulators"), this);

  opt[2] = new QCheckBox(i18n("&Show tabs"), gbWhiteSpace);
  opt[2]->setChecked(configFlags & flags[2]);
  connect(opt[2], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  e2 = new KIntNumInput(KateDocumentConfig::global()->tabWidth(), gbWhiteSpace);
  e2->setRange(1, 16, 1, false);
  e2->setLabel(i18n("Tab width:"), AlignVCenter);
  connect(e2, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  mainLayout->addWidget(gbWhiteSpace);

  QVGroupBox *gbWordWrap = new QVGroupBox(i18n("Static Word Wrap"), this);

  opt[0] = new QCheckBox(i18n("Enable static &word wrap"), gbWordWrap);
  opt[0]->setChecked(KateDocumentConfig::global()->wordWrap());
  connect(opt[0], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(opt[0], SIGNAL(toggled(bool)), this, SLOT(wordWrapToggled()));

  e1 = new KIntNumInput(KateDocumentConfig::global()->wordWrapAt(), gbWordWrap);
  e1->setRange(20, 200, 1, false);
  e1->setLabel(i18n("Wrap words at:"), AlignVCenter);
  connect(e1, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  mainLayout->addWidget(gbWordWrap);

  QVGroupBox *gbCursor = new QVGroupBox(i18n("Text Cursor Movement"), this);

  opt[3] = new QCheckBox(i18n("Smart ho&me"), gbCursor);
  opt[3]->setChecked(configFlags & flags[3]);
  connect(opt[3], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  opt[4] = new QCheckBox(i18n("Wrap c&ursor"), gbCursor);
  opt[4]->setChecked(configFlags & flags[4]);
  connect(opt[4], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  e6 = new QCheckBox(i18n("PageUp/PageDown moves cursor"), gbCursor);
  e6->setChecked(KateDocumentConfig::global()->pageUpDownMovesCursor());
  connect(e6, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  e4 = new KIntNumInput(KateViewConfig::global()->autoCenterLines(), gbCursor);
  e4->setRange(0, 1000000, 1, false);
  e4->setLabel(i18n("Autocenter cursor (lines):"), AlignVCenter);
  connect(e4, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  mainLayout->addWidget(gbCursor);

  opt[1] = new QCheckBox(i18n("Auto &brackets"), this);
  mainLayout->addWidget(opt[1]);
  opt[1]->setChecked(configFlags & flags[1]);
  connect(opt[1], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  e3 = new KIntNumInput(e2, KateDocumentConfig::global()->undoSteps(), this);
  e3->setRange(0, 1000000, 1, false);
  e3->setSpecialValueText( i18n("Unlimited") );
  e3->setLabel(i18n("Maximum undo steps:"), AlignVCenter);
  mainLayout->addWidget(e3);
  connect(e3, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  QHBoxLayout *e5Layout = new QHBoxLayout(mainLayout);
  QLabel *e5Label = new QLabel(i18n("Smart search t&ext from:"), this);
  e5Layout->addWidget(e5Label);
  e5 = new KComboBox (this);
  e5->insertItem( i18n("Nowhere") );
  e5->insertItem( i18n("Selection Only") );
  e5->insertItem( i18n("Selection, then Current Word") );
  e5->insertItem( i18n("Current Word Only") );
  e5->insertItem( i18n("Current Word, then Selection") );
  e5->setCurrentItem(view->getSearchTextFrom());
  e5Layout->addWidget(e5);
  e5Label->setBuddy(e5);
  connect(e5, SIGNAL(activated(int)), this, SLOT(slotChanged()));

  mainLayout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0],
    i18n("Automatically start a new line of text when the current line exceeds the length specified by the <b>Wrap words at:</b> option.<p>This option does not wrap existing lines of text - use the <b>Apply Static Word Wrap</b> option in the <b>Tools</b> menu for that purpose.<p>If you want lines to be <i>visually wrapped</i> instead, according to the width of the view, enable <b>Dynamic Word Wrap</b> in the <b>View Defaults</b> config page."));
  QWhatsThis::add(e1, i18n("If the Word Wrap option is selected this entry determines the length (in characters) at which the editor will automatically start a new line."));
  QWhatsThis::add(opt[1], i18n("When the user types a left bracket ([,(, or {) KateView automatically enters the right bracket (}, ), or ]) to the right of the cursor."));
  QWhatsThis::add(opt[2], i18n("The editor will display a symbol to indicate the presence of a tab in the text."));
  QWhatsThis::add(opt[3], i18n("When selected, pressing the home key will cause the cursor to skip whitespace and go to the start of a line's text."));
  QWhatsThis::add(e3, i18n("Sets the number of undo/redo steps to record. More steps uses more memory."));
  QWhatsThis::add(e4, i18n("Sets the number of lines to maintain visible above and below the cursor when possible."));
  QWhatsThis::add(opt[4], i18n("When on, moving the insertion cursor using the <b>Left</b> and <b>Right</b> keys will go on to previous/next line at beginning/end of the line, similar to most editors.<p>When off, the insertion cursor cannot be moved left of the line start, but it can be moved off the line end, which can be very handy for programmers."));
  QWhatsThis::add(e6, i18n("Selects whether the PageUp and PageDown keys should alter the vertical position of the cursor relative to the top of the view."));
  QString gstfwt = i18n("This determines where KateView will get the search text from "
                        "(this will be automatically entered into the Find Text dialog): "
                        "<br>"
                        "<ul>"
                        "<li><b>Nowhere:</b> Don't guess the search text."
                        "</li>"
                        "<li><b>Selection Only:</b> Use the current text selection, "
                        "if available."
                        "</li>"
                        "<li><b>Selection, then Current Word:</b> Use the current "
                        "selection if available, otherwise use the current word."
                        "</li>"
                        "<li><b>Current Word Only:</b> Use the word that the cursor "
                        "is currently resting on, if available."
                        "</li>"
                        "<li><b>Current Word, then Selection:</b> Use the current "
                        "word if available, otherwise use the current selection."
                        "</li>"
                        "</ul>"
                        "Note that, in all the above modes, if a search string has "
                        "not been or cannot be determined, then the Find Text Dialog "
                        "will fall back to the last search text.");
  QWhatsThis::add(e5Label, gstfwt);
  QWhatsThis::add(e5, gstfwt);

  wordWrapToggled();
}

void EditConfigTab::getData(KateDocument *view)
{
  int configFlags, z;

  configFlags = KateDocumentConfig::global()->configFlags();
  for (z = 1; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }
  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateDocumentConfig::global()->setWordWrapAt(e1->value());
  KateDocumentConfig::global()->setWordWrap (opt[0]->isChecked());
  KateDocumentConfig::global()->setTabWidth(e2->value());

  if (e3->value() <= 0)
    KateDocumentConfig::global()->setUndoSteps(0);
  else
    KateDocumentConfig::global()->setUndoSteps(e3->value());

  KateViewConfig::global()->setAutoCenterLines(QMAX(0, e4->value()));
  view->setGetSearchTextFrom(e5->currentItem());
  KateDocumentConfig::global()->setPageUpDownMovesCursor(e6->isChecked());
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

  QVBoxLayout *blay=new QVBoxLayout(this,0,KDialog::spacingHint());

  QHBox *h = new QHBox (this);
  QLabel *l = new QLabel( i18n("Schema:"), h );
  m_schemaCombo = new KComboBox( h );
  l->setBuddy(m_schemaCombo);
  blay->addWidget(h);

  QVGroupBox *gbWordWrap = new QVGroupBox(i18n("Word Wrap"), this);

  m_dynwrap=new QCheckBox(i18n("&Dynamic word wrap"),gbWordWrap);

  QHBox *m_dynwrapIndicatorsLay = new QHBox (gbWordWrap);
  m_dynwrapIndicatorsLabel = new QLabel( i18n("Dynamic word wrap indicators (if applicable):"), m_dynwrapIndicatorsLay );
  m_dynwrapIndicatorsCombo = new KComboBox( m_dynwrapIndicatorsLay );
  m_dynwrapIndicatorsCombo->insertItem( i18n("Off") );
  m_dynwrapIndicatorsCombo->insertItem( i18n("Follow Line Numbers") );
  m_dynwrapIndicatorsCombo->insertItem( i18n("Always On") );
  m_dynwrapIndicatorsLabel->setBuddy(m_dynwrapIndicatorsCombo);

  m_dynwrapAlignLevel = new KIntNumInput(gbWordWrap);
  m_dynwrapAlignLevel->setLabel(i18n("Vertically align dynamically wrapped lines to indentation depth"));
  m_dynwrapAlignLevel->setRange(0, 80, 10);
  m_dynwrapAlignLevel->setSuffix(i18n("% of view width"));
  m_dynwrapAlignLevel->setSpecialValueText(i18n("Disabled"));

  m_wwmarker = new QCheckBox( i18n("Show static word wrap marker (if applicable)"), gbWordWrap );

  blay->addWidget(gbWordWrap);

  QVGroupBox *gbFold = new QVGroupBox(i18n("Code Folding"), this);

  m_folding=new QCheckBox(i18n("Show &folding markers (if available)"), gbFold );
  m_collapseTopLevel = new QCheckBox( i18n("Collapse toplevel folding nodes"), gbFold );

  blay->addWidget(gbFold);

  QVGroupBox *gbBar = new QVGroupBox(i18n("Left Border"), this);

  m_icons=new QCheckBox(i18n("Show &icon border"),gbBar);
  m_line=new QCheckBox(i18n("Show &line numbers"),gbBar);

  blay->addWidget(gbBar);

  m_bmSort = new QButtonGroup( 1, Qt::Horizontal, i18n("Sort Bookmarks Menu"), this );
  m_bmSort->setRadioButtonExclusive( true );
  m_bmSort->insert( rb1=new QRadioButton( i18n("By &position"), m_bmSort ), 0 );
  m_bmSort->insert( rb2=new QRadioButton( i18n("By c&reation"), m_bmSort ), 1 );

  connect(m_dynwrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_dynwrapIndicatorsCombo, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect(m_dynwrapAlignLevel, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect(m_wwmarker, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_icons, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_line, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_folding, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_collapseTopLevel, SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );
  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  blay->addWidget(m_bmSort, 0 );
  blay->addStretch(1000);

  QWhatsThis::add(m_dynwrap,i18n("If this option is checked, the text lines will be wrapped at the view border on the screen."));
  QString wtstr = i18n("Choose when the Dynamic Word Wrap Indicators should be displayed");
  QWhatsThis::add(m_dynwrapIndicatorsLabel, wtstr);
  QWhatsThis::add(m_dynwrapIndicatorsCombo, wtstr);
  QWhatsThis::add(m_dynwrapAlignLevel, i18n("<p>Enables the start of dynamically wrapped lines to be aligned vertically to the indentation level of the first line.  This can help to make code and markup more readable.</p><p>Additionally, this allows you to set a maximum width of the screen, as a percentage, after which dynamically wrapped lines will no longer be vertically aligned.  For example, at 50%, lines whose indentation levels are deeper than 50% of the width of the screen will not have vertical alignment applied to subsequent wrapped lines.</p>"));
  QWhatsThis::add( m_wwmarker, i18n(
        "<p>If this option is checked, a vertical line will be drawn at the word "
        "wrap column as defined in the <strong>Editing</strong> properties."
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
  KateRendererConfig::global()->setSchema (m_schemaCombo->currentItem());
  KateViewConfig::global()->setDynWordWrap (m_dynwrap->isChecked());
  KateViewConfig::global()->setDynWordWrapIndicators (m_dynwrapIndicatorsCombo->currentItem ());
  KateViewConfig::global()->setDynWordWrapAlignIndent(m_dynwrapAlignLevel->value());
  KateRendererConfig::global()->setWordWrapMarker (m_wwmarker->isChecked());
  KateViewConfig::global()->setLineNumbers (m_line->isChecked());
  KateViewConfig::global()->setIconBar (m_icons->isChecked());
  KateViewConfig::global()->setFoldingBar (m_folding->isChecked());
  m_doc->m_collapseTopLevelOnLoad = m_collapseTopLevel->isChecked();
  KateViewConfig::global()->setBookmarkSort (m_bmSort->id (m_bmSort->selected()));
}

void ViewDefaultsConfig::reload ()
{
  m_schemaCombo->clear ();
  m_schemaCombo->insertStringList (KateFactory::schemaManager()->list());
  m_schemaCombo->setCurrentItem (KateRendererConfig::global()->schema());

  m_dynwrap->setChecked(KateViewConfig::global()->dynWordWrap());
  m_dynwrapIndicatorsCombo->setCurrentItem( KateViewConfig::global()->dynWordWrapIndicators() );
  m_dynwrapAlignLevel->setValue(KateViewConfig::global()->dynWordWrapAlignIndent());
  m_wwmarker->setChecked( KateRendererConfig::global()->wordWrapMarker() );
  m_line->setChecked(KateViewConfig::global()->lineNumbers());
  m_icons->setChecked(KateViewConfig::global()->iconBar());
  m_folding->setChecked(KateViewConfig::global()->foldingBar());
  m_collapseTopLevel->setChecked( m_doc->m_collapseTopLevelOnLoad );
  m_bmSort->setButton( KateViewConfig::global()->bookmarkSort()  );
}

void ViewDefaultsConfig::reset () {;}

void ViewDefaultsConfig::defaults (){;}
//END ViewDefaultsConfig

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
    connect( m_keyChooser, SIGNAL( keyChange() ), this, SLOT( slotChanged() ) );
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
  int configFlags = KateDocumentConfig::global()->configFlags();
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  QVGroupBox *gbEnc = new QVGroupBox(i18n("File Format"), this);
  layout->addWidget( gbEnc );

  QHBox *e5Layout = new QHBox(gbEnc);
  QLabel *e5Label = new QLabel(i18n("&Encoding:"), e5Layout);
  m_encoding = new KComboBox (e5Layout);
  e5Label->setBuddy(m_encoding);
  connect(m_encoding, SIGNAL(activated(int)), this, SLOT(slotChanged()));

  e5Layout = new QHBox(gbEnc);
  e5Label = new QLabel(i18n("End &of line:"), e5Layout);
  m_eol = new KComboBox (e5Layout);
  e5Label->setBuddy(m_eol);
  connect(m_eol, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  m_eol->insertItem (i18n("Unix"));
  m_eol->insertItem (i18n("Dos/Windows"));
  m_eol->insertItem (i18n("Macintosh"));

  QVGroupBox *gbWhiteSpace = new QVGroupBox(i18n("Automatic Cleanups on Save"), this);
  layout->addWidget( gbWhiteSpace );

  replaceTabs = new QCheckBox(i18n("Replace &tabs with spaces"), gbWhiteSpace);
  replaceTabs->setChecked(configFlags & KateDocument::cfReplaceTabs);
  connect(replaceTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  removeSpaces = new QCheckBox(i18n("Re&move trailing spaces"), gbWhiteSpace);
  removeSpaces->setChecked(configFlags & KateDocument::cfRemoveSpaces);
  connect(removeSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  QGroupBox *gb = new QGroupBox( 1, Qt::Horizontal, i18n("Backup on Save"), this );
  layout->addWidget( gb );
  cbLocalFiles = new QCheckBox( i18n("&Local files"), gb );
  cbRemoteFiles = new QCheckBox( i18n("&Remote files"), gb );
  QHBox *hbBuSuffix = new QHBox( gb );
  QLabel *lBuSuffix = new QLabel( i18n("&Suffix:"), hbBuSuffix );
  leBuSuffix = new QLineEdit( hbBuSuffix );
  lBuSuffix->setBuddy( leBuSuffix );

  connect( cbLocalFiles, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( cbRemoteFiles, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( leBuSuffix, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  layout->addStretch();

  QWhatsThis::add(replaceTabs, i18n("KateView will replace any tabs with the number of spaces indicated in the Tab Width: entry."));
  QWhatsThis::add(removeSpaces, i18n("KateView will automatically eliminate extra spaces at the ends of lines of text."));

  QWhatsThis::add( gb, i18n(
        "<p>Backing up on save will cause Kate to copy the disk file to "
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
                i18n("You didn't provide a backup suffix. Using default: '~'"),
                i18n("No Backup Suffix")
                        );
    leBuSuffix->setText( "~" );
  }

  uint f( 0 );
  if ( cbLocalFiles->isChecked() )
    f |= KateDocumentConfig::LocalFiles;
  if ( cbRemoteFiles->isChecked() )
    f |= KateDocumentConfig::RemoteFiles;

  KateDocumentConfig::global()->setBackupFlags(f);
  KateDocumentConfig::global()->setBackupSuffix(leBuSuffix->text());

  int configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocument::cfReplaceTabs; // clear flag
  if (replaceTabs->isChecked()) configFlags |= KateDocument::cfReplaceTabs; // set flag if checked

  configFlags &= ~KateDocument::cfRemoveSpaces; // clear flag
  if (removeSpaces->isChecked()) configFlags |= KateDocument::cfRemoveSpaces; // set flag if checked

  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateDocumentConfig::global()->setEncoding(KGlobal::charsets()->encodingForName(m_encoding->currentText()));

  KateDocumentConfig::global()->setEol(m_eol->currentItem());
}

void SaveConfigTab::reload()
{
  // encoding
  m_encoding->clear ();
  QStringList encodings (KGlobal::charsets()->descriptiveEncodingNames());
  int insert = 0;
  for (uint i=0; i < encodings.count(); i++)
  {
    bool found = false;
    QTextCodec *codecForEnc = KGlobal::charsets()->codecForName(KGlobal::charsets()->encodingForName(encodings[i]), found);

    if (found)
    {
      m_encoding->insertItem (encodings[i]);

      if ( codecForEnc->name() == KateDocumentConfig::global()->encoding() )
      {
        m_encoding->setCurrentItem(insert);
      }

      insert++;
    }
  }

  // eol
  m_eol->setCurrentItem(KateDocumentConfig::global()->eol());

  // other stuff
  uint f ( KateDocumentConfig::global()->backupFlags() );
  cbLocalFiles->setChecked( f & KateDocumentConfig::LocalFiles );
  cbRemoteFiles->setChecked( f & KateDocumentConfig::RemoteFiles );
  leBuSuffix->setText( KateDocumentConfig::global()->backupSuffix() );
}

void SaveConfigTab::reset()
{
}

void SaveConfigTab::defaults()
{
  cbLocalFiles->setChecked( true );
  cbRemoteFiles->setChecked( false );
  leBuSuffix->setText( "~" );
}

//END SaveConfigTab

// kate: space-indent on; indent-width 2; replace-tabs on;
