/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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

//BEGIN Includes
#include "katedialogs.h"
#include "katedialogs.moc"

#include "katesyntaxdocument.h"
#include "katedocument.h"
#include "katefactory.h"
#include "kateconfig.h"
#include "kateschema.h"
#include "kateautoindent.h"
#include "kateview.h"

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>

#include <kapplication.h>
#include <kspell.h>
#include <kbuttonbox.h>
#include <kcharsets.h>
#include <kcolorcombo.h>
#include <kcolordialog.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <kaccel.h>
#include <kcharsets.h>
#include <kcolorbutton.h>
#include <kglobal.h>
#include <kkeybutton.h>
#include <kkeydialog.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kfontdialog.h>
#include <knuminput.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qheader.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmap.h>
#include <qpainter.h>
#include <qpointarray.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>
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
#include <qdom.h>

#define HLDOWNLOADPATH "http://www.kde.org/apps/kate/hl/update.xml"
//END

SpellConfigPage::SpellConfigPage( QWidget* parent )
  : Kate::ConfigPage( parent)
{
  QVBoxLayout* l = new QVBoxLayout( this );
  cPage = new KSpellConfig( this, 0L, 0L, false );
  l->addWidget( cPage );
  connect( cPage, SIGNAL( configChanged() ), this, SLOT( slotChanged() ) );
}

void SpellConfigPage::apply ()
{
  // kspell
  cPage->writeGlobalSettings ();
}

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

IndentConfigTab::IndentConfigTab(QWidget *parent)
  : Kate::ConfigPage(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = KateDocumentConfig::global()->configFlags();

  QVGroupBox *gbAuto = new QVGroupBox(i18n("Automatic Indentation"), this);

  opt[0] = new QCheckBox(i18n("A&ctivated"), gbAuto);
  opt[0]->setChecked(configFlags & flags[0]);
  //layout->addWidget(opt[0]);
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

  opt[1] = new QCheckBox(i18n("Use &spaces instead of tabs to indent"), gbWordWrap );
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
  m_tabs->insert( rb1=new QRadioButton( i18n("Insert indent &characters"), m_tabs ), 0 );
  m_tabs->insert( rb2=new QRadioButton( i18n("I&nsert tab character"), m_tabs ), 1 );
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
SelectConfigTab::SelectConfigTab(QWidget *parent)
  : Kate::ConfigPage(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  QRadioButton *rb1, *rb2;

  m_tabs = new QButtonGroup( 1, Qt::Horizontal, i18n("Selection Mode"), this );
  layout->add (m_tabs);

  m_tabs->setRadioButtonExclusive( true );
  m_tabs->insert( rb1=new QRadioButton( i18n("&Normal"), m_tabs ), 0 );
  m_tabs->insert( rb2=new QRadioButton( i18n("&Persistent"), m_tabs ), 1 );

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  layout->addStretch();

  QWhatsThis::add(rb1, i18n("Selections will be overwritten by typed text and will be lost on cursor movement."));
  QWhatsThis::add(rb2, i18n("Selections will stay even after cursor movement and typing."));

  reload ();
}

void SelectConfigTab::apply ()
{
  int configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocumentConfig::cfPersistent; // clear persistent

  if (m_tabs->id (m_tabs->selected()) == 1)
    configFlags |= KateDocumentConfig::cfPersistent; // set flag if checked

  KateDocumentConfig::global()->setConfigFlags(configFlags);
}

void SelectConfigTab::reload ()
{
  if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfPersistent)
    m_tabs->setButton (1);
  else
    m_tabs->setButton (0);
}
//END SelectConfigTab

//BEGIN EditConfigTab
const int EditConfigTab::flags[] = {KateDocument::cfWordWrap,
  KateDocument::cfAutoBrackets, KateDocument::cfShowTabs, KateDocument::cfSmartHome, KateDocument::cfWrapCursor};

EditConfigTab::EditConfigTab(QWidget *parent)
  : Kate::ConfigPage(parent)
{
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
  e5->setCurrentItem(KateViewConfig::global()->textToSearchMode());
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

void EditConfigTab::apply ()
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
  KateViewConfig::global()->setTextToSearchMode(e5->currentItem());
  KateDocumentConfig::global()->setPageUpDownMovesCursor(e6->isChecked());
}

void EditConfigTab::reload ()
{

}

void EditConfigTab::wordWrapToggled() {
  e1->setEnabled(opt[0]->isChecked());
}
//END EditConfigTab

//BEGIN ViewDefaultsConfig
ViewDefaultsConfig::ViewDefaultsConfig(QWidget *parent)
  :Kate::ConfigPage(parent)
{
  QRadioButton *rb1;
  QRadioButton *rb2;

  QVBoxLayout *blay=new QVBoxLayout(this,0,KDialog::spacingHint());

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
  m_dynwrapAlignLevel->setLabel(i18n("Vertically align dynamically wrapped lines to indentation depth:"));
  m_dynwrapAlignLevel->setRange(0, 80, 10);
  m_dynwrapAlignLevel->setSuffix(i18n("% of view width"));
  m_dynwrapAlignLevel->setSpecialValueText(i18n("Disabled"));

  m_wwmarker = new QCheckBox( i18n("Show static word wrap marker (if applicable)"), gbWordWrap );

  blay->addWidget(gbWordWrap);

  QVGroupBox *gbFold = new QVGroupBox(i18n("Code Folding"), this);

  m_folding=new QCheckBox(i18n("Show &folding markers (if available)"), gbFold );
  m_collapseTopLevel = new QCheckBox( i18n("Collapse toplevel folding nodes"), gbFold );
  m_collapseTopLevel->hide ();
  
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
  KateViewConfig::global()->setDynWordWrap (m_dynwrap->isChecked());
  KateViewConfig::global()->setDynWordWrapIndicators (m_dynwrapIndicatorsCombo->currentItem ());
  KateViewConfig::global()->setDynWordWrapAlignIndent(m_dynwrapAlignLevel->value());
  KateRendererConfig::global()->setWordWrapMarker (m_wwmarker->isChecked());
  KateViewConfig::global()->setLineNumbers (m_line->isChecked());
  KateViewConfig::global()->setIconBar (m_icons->isChecked());
  KateViewConfig::global()->setFoldingBar (m_folding->isChecked());
  KateViewConfig::global()->setBookmarkSort (m_bmSort->id (m_bmSort->selected()));
}

void ViewDefaultsConfig::reload ()
{
  m_dynwrap->setChecked(KateViewConfig::global()->dynWordWrap());
  m_dynwrapIndicatorsCombo->setCurrentItem( KateViewConfig::global()->dynWordWrapIndicators() );
  m_dynwrapAlignLevel->setValue(KateViewConfig::global()->dynWordWrapAlignIndent());
  m_wwmarker->setChecked( KateRendererConfig::global()->wordWrapMarker() );
  m_line->setChecked(KateViewConfig::global()->lineNumbers());
  m_icons->setChecked(KateViewConfig::global()->iconBar());
  m_folding->setChecked(KateViewConfig::global()->foldingBar());
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

// FIXME THE isSomethingSet() calls should partly be replaced by itemSet(XYZ) and
// there is a need for an itemUnset(XYZ)
// a) is done. itemUnset(N) == !itemSet(N); right???
// there is a unsetItem(), just (not all logic) called "KateAttribute::clearAttribute(int)";

//BEGIN PluginListItem
PluginListItem::PluginListItem(const bool _exclusive, bool _checked, Kate::PluginInfo *_info, QListView *_parent)
  : QCheckListItem(_parent, _info->service->name(), CheckBox)
  , mInfo(_info)
  , silentStateChange(false)
  , exclusive(_exclusive)
{
  setChecked(_checked);
  if(_checked) static_cast<PluginListView *>(listView())->count++;
}


void PluginListItem::setChecked(bool b)
{
  silentStateChange = true;
  setOn(b);
  silentStateChange = false;
}

void PluginListItem::stateChange(bool b)
{
  if(!silentStateChange)
    static_cast<PluginListView *>(listView())->stateChanged(this, b);
}

void PluginListItem::paintCell(QPainter *p, const QColorGroup &cg, int a, int b, int c)
{
  if(exclusive) myType = RadioButton;
  QCheckListItem::paintCell(p, cg, a, b, c);
  if(exclusive) myType = CheckBox;
}
//END

//BEGIN PluginListView
PluginListView::PluginListView(unsigned _min, unsigned _max, QWidget *_parent, const char *_name)
  : KListView(_parent, _name)
  , hasMaximum(true)
  , max(_max)
  , min(_min <= _max ? _min : _max)
  , count(0)
{
}

PluginListView::PluginListView(unsigned _min, QWidget *_parent, const char *_name)
  : KListView(_parent, _name)
  , hasMaximum(false)
  , min(_min)
  , count(0)
{
}

PluginListView::PluginListView(QWidget *_parent, const char *_name)
  : KListView(_parent, _name)
  , hasMaximum(false)
  , min(0)
  , count(0)
{
}

void PluginListView::clear()
{
  count = 0;
  KListView::clear();
}

void PluginListView::stateChanged(PluginListItem *item, bool b)
{
  if(b)
  {
    count++;
    emit stateChange(item, b);

    if(hasMaximum && count > max)
    {
      // Find a different one and turn it off

      QListViewItem *cur = firstChild();
      PluginListItem *curItem = dynamic_cast<PluginListItem *>(cur);

      while(cur == item || !curItem || !curItem->isOn())
      {
        cur = cur->nextSibling();
        curItem = dynamic_cast<PluginListItem *>(cur);
      }

      curItem->setOn(false);
    }
  }
  else
  {
    if(count == min)
    {
      item->setChecked(true);
    }
    else
    {
      count--;
      emit stateChange(item, b);
    }
  }
}
//END

//BEGIN PluginConfigPage
PluginConfigPage::PluginConfigPage (QWidget *parent, KateDocument *doc) : Kate::ConfigPage (parent, "")
{
  m_doc = doc;

  // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  PluginListView* listView = new PluginListView(0, this);
  listView->addColumn(i18n("Name"));
  listView->addColumn(i18n("Comment"));
  connect(listView, SIGNAL(stateChange(PluginListItem *, bool)), this, SLOT(stateChange(PluginListItem *, bool)));

  grid->addWidget( listView, 0, 0);

  for (uint i=0; i<m_doc->s_plugins.count(); i++)
  {
    PluginListItem *item = new PluginListItem(false, m_doc->s_plugins.at(i)->load, m_doc->s_plugins.at(i), listView);
    item->setText(0, m_doc->s_plugins.at(i)->service->name());
    item->setText(1, m_doc->s_plugins.at(i)->service->comment());
  }
}

PluginConfigPage::~PluginConfigPage ()
{
}

void PluginConfigPage::stateChange(PluginListItem *item, bool b)
{
  if(b)
    loadPlugin(item);
  else
    unloadPlugin(item);
  emit changed();
}

void PluginConfigPage::loadPlugin (PluginListItem *item)
{
  item->info()->load = true;
  for (uint z=0; z < KateFactory::self()->documents()->count(); z++)
    KateFactory::self()->documents()->at(z)->loadAllEnabledPlugins ();

  item->setOn(true);
}

void PluginConfigPage::unloadPlugin (PluginListItem *item)
{
  item->info()->load = false;
  for (uint z=0; z < KateFactory::self()->documents()->count(); z++)
    KateFactory::self()->documents()->at(z)->loadAllEnabledPlugins ();

  item->setOn(false);
}
//END

//BEGIN HlConfigPage
HlConfigPage::HlConfigPage (QWidget *parent)
 : Kate::ConfigPage (parent, "")
 , hlData (0)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  QHBox *hbHl = new QHBox( this );
  layout->add (hbHl);

  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("H&ighlight:"), hbHl );
  hlCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( hlCombo );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );

  for( int i = 0; i < HlManager::self()->highlights(); i++) {
    if (HlManager::self()->hlSection(i).length() > 0)
      hlCombo->insertItem(HlManager::self()->hlSection(i) + QString ("/") + HlManager::self()->hlName(i));
    else
      hlCombo->insertItem(HlManager::self()->hlName(i));
  }
  hlCombo->setCurrentItem(0);

  QGroupBox *gbProps = new QGroupBox( 1, Qt::Horizontal, i18n("Properties"), this );
  layout->add (gbProps);

  // file & mime types
  QHBox *hbFE = new QHBox( gbProps);
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), hbFE );
  wildcards  = new QLineEdit( hbFE );
  lFileExts->setBuddy( wildcards );
  connect( wildcards, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  QHBox *hbMT = new QHBox( gbProps );
  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), hbMT);
  mimetypes = new QLineEdit( hbMT );
  connect( mimetypes, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  lMimeTypes->setBuddy( mimetypes );

  QHBox *hbMT2 = new QHBox( gbProps );
  QLabel *lprio = new QLabel( i18n("Prio&rity:"), hbMT2);
  priority = new KIntNumInput( hbMT2 );
  connect( priority, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );
  lprio->setBuddy( priority );

  QToolButton *btnMTW = new QToolButton(hbMT);
  btnMTW->setIconSet(QIconSet(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));

  // download/new buttons
  QHBox *hbBtns = new QHBox( this );
  layout->add (hbBtns);

  ((QBoxLayout*)hbBtns->layout())->addStretch(1); // hmm.
  hbBtns->setSpacing( KDialog::spacingHint() );
  QPushButton *btnDl = new QPushButton(i18n("Do&wnload..."), hbBtns);
  connect( btnDl, SIGNAL(clicked()), this, SLOT(hlDownload()) );

  hlCombo->setCurrentItem( 0 );
  hlChanged(0);

  QWhatsThis::add( hlCombo,   i18n("Choose a <em>Syntax Highlight mode</em> from this list to view its properties below.") );
  QWhatsThis::add( wildcards, i18n("The list of file extensions used to determine which files to highlight using the current syntax highlight mode.") );
  QWhatsThis::add( mimetypes, i18n("The list of Mime Types used to determine which files to highlight using the current highlight mode.<p>Click the wizard button on the left of the entry field to display the MimeType selection dialog.") );
  QWhatsThis::add( btnMTW,    i18n("Display a dialog with a list of all available mime types to choose from.<p>The <strong>File Extensions</strong> entry will automatically be edited as well.") );
  QWhatsThis::add( btnDl,     i18n("Click this button to download new or updated syntax highlight descriptions from the Kate website.") );

  layout->addStretch ();
}

HlConfigPage::~HlConfigPage ()
{
}

void HlConfigPage::apply ()
{
  writeback();

  for ( QIntDictIterator<HlData> it( hlDataDict ); it.current(); ++it )
    HlManager::self()->getHl( it.currentKey() )->setData( it.current() );

  HlManager::self()->getKConfig()->sync ();
}

void HlConfigPage::reload ()
{
}

void HlConfigPage::hlChanged(int z)
{
  writeback();

  if ( ! hlDataDict.find( z ) )
    hlDataDict.insert( z, HlManager::self()->getHl( z )->getData() );

  hlData = hlDataDict.find( z );
  wildcards->setText(hlData->wildcards);
  mimetypes->setText(hlData->mimetypes);
  priority->setValue(hlData->priority);
}

void HlConfigPage::writeback()
{
  if (hlData)
  {
    hlData->wildcards = wildcards->text();
    hlData->mimetypes = mimetypes->text();
    hlData->priority = priority->value();
  }
}

void HlConfigPage::hlDownload()
{
  HlDownloadDialog diag(this,"hlDownload",true);
  diag.exec();
}

void HlConfigPage::showMTDlg()
{
  QString text = i18n("Select the MimeTypes you want highlighted using the '%1' syntax highlight rules.\nPlease note that this will automatically edit the associated file extensions as well.").arg( hlCombo->currentText() );
  QStringList list = QStringList::split( QRegExp("\\s*;\\s*"), mimetypes->text() );
  KMimeTypeChooserDlg *d = new KMimeTypeChooserDlg( this, i18n("Select Mime Types"), text, list );

  if ( d->exec() == KDialogBase::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    wildcards->setText(d->patterns().join(";"));
    mimetypes->setText(d->mimeTypes().join(";"));
  }
}
//END HlConfigPage

//BEGIN KMimeTypeChooser
/*********************************************************************/
/*               KMimeTypeChooser Implementation                     */
/*********************************************************************/
KMimeTypeChooser::KMimeTypeChooser( QWidget *parent, const QString &text, const QStringList &selectedMimeTypes, bool editbutton, bool showcomment, bool showpatterns)
    : QVBox( parent )
{
  setSpacing( KDialogBase::spacingHint() );

/* HATE!!!! geometry management seems BADLY broken :(((((((((((
   Problem: if richtext is used (or Qt::WordBreak is on in the label),
   the list view is NOT resized when the parent box is.
   No richtext :(((( */
  if ( !text.isEmpty() ) {
    new QLabel( text, this );
  }

  lvMimeTypes = new QListView( this );
  lvMimeTypes->addColumn( i18n("Mime Type") );
  if ( showcomment )
    lvMimeTypes->addColumn( i18n("Comment") );
  if ( showpatterns )
    lvMimeTypes->addColumn( i18n("Patterns") );
  lvMimeTypes->setRootIsDecorated( true );

  //lvMimeTypes->clear(); WHY?!
  QMap<QString,QListViewItem*> groups;
  // thanks to kdebase/kcontrol/filetypes/filetypesview
  KMimeType::List mimetypes = KMimeType::allMimeTypes();
  QValueListIterator<KMimeType::Ptr> it(mimetypes.begin());

  QListViewItem *groupItem;
  bool agroupisopen = false;
  QListViewItem *idefault = 0; //open this, if all other fails
  for (; it != mimetypes.end(); ++it) {
    QString mimetype = (*it)->name();
    int index = mimetype.find("/");
    QString maj = mimetype.left(index);
    QString min = mimetype.right(mimetype.length() - (index+1));

    QMapIterator<QString,QListViewItem*> mit = groups.find( maj );
    if ( mit == groups.end() ) {
        groupItem = new QListViewItem( lvMimeTypes, maj );
        groups.insert( maj, groupItem );
        if (maj == "text")
          idefault = groupItem;
    }
    else
        groupItem = mit.data();

    QCheckListItem *item = new QCheckListItem( groupItem, min, QCheckListItem::CheckBox );
    item->setPixmap( 0, SmallIcon( (*it)->icon(QString::null,false) ) );
    int cl = 1;
    if ( showcomment ) {
      item->setText( cl, (*it)->comment(QString::null, false) );
      cl++;
    }
    if ( showpatterns )
      item->setText( cl, (*it)->patterns().join("; ") );
    if ( selectedMimeTypes.contains(mimetype) ) {
      item->setOn( true );
      groupItem->setOpen( true );
      agroupisopen = true;
      lvMimeTypes->ensureItemVisible( item );// actually, i should do this for the first item only.
    }
  }

  if (! agroupisopen)
    idefault->setOpen( true );

  if (editbutton) {
    QHBox *btns = new QHBox( this );
    ((QBoxLayout*)btns->layout())->addStretch(1); // hmm.
    btnEditMimeType = new QPushButton( i18n("&Edit..."), btns );
    connect( btnEditMimeType, SIGNAL(clicked()), this, SLOT(editMimeType()) );
    btnEditMimeType->setEnabled( false );
    connect( lvMimeTypes, SIGNAL( doubleClicked ( QListViewItem * )), this, SLOT( editMimeType()));
    connect( lvMimeTypes, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(slotCurrentChanged(QListViewItem*)) );
    QWhatsThis::add( btnEditMimeType, i18n("Click this button to display the familiar KDE File Type Editor.<p><strong>Warning:</strong> if you change the file extensions, you need to restart this dialog, as it will not be aware that they have changed.") );
  }
}

void KMimeTypeChooser::editMimeType()
{
  if ( !(lvMimeTypes->currentItem() && (lvMimeTypes->currentItem())->parent()) ) return;
  QString mt = (lvMimeTypes->currentItem()->parent())->text( 0 ) + "/" + (lvMimeTypes->currentItem())->text( 0 );
  // thanks to libkonq/konq_operations.cc
  QString keditfiletype = QString::fromLatin1("keditfiletype");
  KRun::runCommand( keditfiletype + " " + KProcess::quote(mt),
                    keditfiletype, keditfiletype /*unused*/);
}

void KMimeTypeChooser::slotCurrentChanged(QListViewItem* i)
{
  btnEditMimeType->setEnabled( i->parent() );
}

QStringList KMimeTypeChooser::selectedMimeTypesStringList()
{
  QStringList l;
  QListViewItemIterator it( lvMimeTypes );
  for (; it.current(); ++it) {
    if ( it.current()->parent() && ((QCheckListItem*)it.current())->isOn() )
      l << it.current()->parent()->text(0) + "/" + it.current()->text(0); // FIXME: uncecked, should be Ok unless someone changes mimetypes during this!
  }
  return l;
}

QStringList KMimeTypeChooser::patterns()
{
  QStringList l;
  KMimeType::Ptr p;
  QString defMT = KMimeType::defaultMimeType();
  QListViewItemIterator it( lvMimeTypes );
  for (; it.current(); ++it) {
    if ( it.current()->parent() && ((QCheckListItem*)it.current())->isOn() ) {
      p = KMimeType::mimeType( it.current()->parent()->text(0) + "/" + it.current()->text(0) );
      if ( p->name() != defMT )
        l += p->patterns();
    }
  }
  return l;
}
//END

//BEGIN KMimeTypeChooserDlg
/*********************************************************************/
/*               KMimeTypeChooserDlg Implementation                  */
/*********************************************************************/
KMimeTypeChooserDlg::KMimeTypeChooserDlg(QWidget *parent,
                         const QString &caption, const QString& text,
                         const QStringList &selectedMimeTypes,
                         bool editbutton, bool showcomment, bool showpatterns )
    : KDialogBase(parent, 0, true, caption, Cancel|Ok, Ok)
{
  KConfig *config = kapp->config();

  chooser = new KMimeTypeChooser( this, text, selectedMimeTypes, editbutton, showcomment, showpatterns);
  setMainWidget(chooser);

  config->setGroup("KMimeTypeChooserDlg");
  resize( config->readSizeEntry("size", new QSize(400,300)) );
}

KMimeTypeChooserDlg::~KMimeTypeChooserDlg()
{
  KConfig *config = kapp->config();
  config->setGroup("KMimeTypeChooserDlg");
  config->writeEntry("size", size());
}

QStringList KMimeTypeChooserDlg::mimeTypes()
{
  return chooser->selectedMimeTypesStringList();
}

QStringList KMimeTypeChooserDlg::patterns()
{
  return chooser->patterns();
}
//END

HlDownloadDialog::HlDownloadDialog(QWidget *parent, const char *name, bool modal)
  :KDialogBase(KDialogBase::Swallow, i18n("Highlight Download"), User1|Cancel, User1, parent, name, modal,false,i18n("&Install"))
{
  setMainWidget( list=new QListView(this));
  list->addColumn(i18n("Name"));
  list->addColumn(i18n("Installed"));
  list->addColumn(i18n("Latest"));
  list->addColumn(i18n("Release Date"));
  list->setSelectionMode(QListView::Multi);
  KIO::TransferJob *getIt=KIO::get(KURL(HLDOWNLOADPATH), true, true );
  connect(getIt,SIGNAL(data(KIO::Job *, const QByteArray &)),
    this, SLOT(listDataReceived(KIO::Job *, const QByteArray &)));
//        void data( KIO::Job *, const QByteArray &data);

}

HlDownloadDialog::~HlDownloadDialog(){}

void HlDownloadDialog::listDataReceived(KIO::Job *, const QByteArray &data)
{
  listData+=QString(data);
  kdDebug(13000)<<QString("CurrentListData: ")<<listData<<endl<<endl;
  kdDebug(13000)<<QString("Data length: %1").arg(data.size())<<endl;
  kdDebug(13000)<<QString("listData length: %1").arg(listData.length())<<endl;
  if (data.size()==0)
  {
    if (listData.length()>0)
    {
      QString installedVersion;
      HlManager *hlm=HlManager::self();
      QDomDocument doc;
      doc.setContent(listData);
      QDomElement DocElem=doc.documentElement();
      QDomNode n=DocElem.firstChild();
      Highlight *hl;

      if (n.isNull()) kdDebug(13000)<<"There is no usable childnode"<<endl;
      while (!n.isNull())
      {
        installedVersion="    --";

        QDomElement e=n.toElement();
        if (!e.isNull())
        kdDebug(13000)<<QString("NAME: ")<<e.tagName()<<QString(" - ")<<e.attribute("name")<<endl;
        n=n.nextSibling();

        QString Name=e.attribute("name");

        for (int i=0;i<hlm->highlights();i++)
        {
          hl=hlm->getHl(i);
          if (hl->name()==Name)
          {
            installedVersion="    "+hl->version();
            break;
          }
        }

        (void) new QListViewItem(list,e.attribute("name"),installedVersion,e.attribute("version"),e.attribute("date"),e.attribute("url"));
      }
    }
  }
}

void HlDownloadDialog::slotUser1()
{
  QString destdir=KGlobal::dirs()->saveLocation("data","katepart/syntax/");
  for (QListViewItem *it=list->firstChild();it;it=it->nextSibling())
  {
    if (list->isSelected(it))
    {
      KURL src(it->text(4));
      QString filename=src.fileName(false);
      QString dest = destdir+filename;

      KIO::NetAccess::download(src,dest, this);
    }
  }

  // update Config !!
  SyntaxDocument doc (true);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
