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

#include "kateautoindent.h"
#include "katebuffer.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "katefactory.h"
#include "kateschema.h"
#include "katesyntaxdocument.h"
#include "kateview.h"


#include <ktexteditor/configinterfaceextension.h>
#include <ktexteditor/plugin.h>

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>

#include <kaccel.h>
#include <kapplication.h>
#include <kbuttonbox.h>
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
#include <kkeybutton.h>
#include <kkeydialog.h>
#include <klineedit.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypechooser.h>
#include <knuminput.h>
#include <kparts/componentfactory.h>
#include <kpopupmenu.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kregexpeditorinterface.h>
#include <krun.h>
#include <kspell.h>
#include <kstandarddirs.h>
#include <ktempfile.h>

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qdom.h>
#include <qfile.h>
#include <qgrid.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qheader.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qlistview.h>
#include <qmap.h>
#include <qobjectlist.h>
#include <qpainter.h>
#include <qpointarray.h>
#include <qptrcollection.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qtextcodec.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>

// trailing slash is important
#define HLDOWNLOADPATH "http://www.kde.org/apps/kate/syntax/"

//END

//BEGIN KateConfigPage
KateConfigPage::KateConfigPage ( QWidget *parent, const char *name )
  : Kate::ConfigPage (parent, name)
  , m_changed (false)
{
  connect (this, SIGNAL(changed()), this, SLOT(somethingHasChanged ()));
}

KateConfigPage::~KateConfigPage ()
{
}

void KateConfigPage::somethingHasChanged ()
{
  m_changed = true;
  kdDebug (13000) << "TEST: something changed on the config page: " << this << endl;
}
//END KateConfigPage

//BEGIN KateSpellConfigPage
KateSpellConfigPage::KateSpellConfigPage( QWidget* parent )
  : KateConfigPage( parent)
{
  QVBoxLayout* l = new QVBoxLayout( this );
  cPage = new KSpellConfig( this, 0L, 0L, false );
  l->addWidget( cPage );
  connect( cPage, SIGNAL( configChanged() ), this, SLOT( slotChanged() ) );
}

void KateSpellConfigPage::apply ()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  // kspell
  cPage->writeGlobalSettings ();
}
//END KateSpellConfigPage

//BEGIN KateIndentConfigTab
const int KateIndentConfigTab::flags[] = {KateDocument::cfSpaceIndent,
  KateDocument::cfKeepIndentProfile, KateDocument::cfKeepExtraSpaces, KateDocument::cfTabIndents,
  KateDocument::cfBackspaceIndents, KateDocumentConfig::cfDoxygenAutoTyping};

KateIndentConfigTab::KateIndentConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = KateDocumentConfig::global()->configFlags();

  QVGroupBox *gbAuto = new QVGroupBox(i18n("Automatic Indentation"), this);

  QHBox *indentLayout = new QHBox(gbAuto);
  QLabel *indentLabel = new QLabel(i18n("&Indentation mode:"), indentLayout);
  m_indentMode = new KComboBox (indentLayout);
  m_indentMode->insertStringList (KateAutoIndent::listModes());
  indentLabel->setBuddy(m_indentMode);

  opt[5] = new QCheckBox(i18n("Insert leading Doxygen \"*\" when typing"), gbAuto);

  QVGroupBox *gbSpaces = new QVGroupBox(i18n("Indentation with Spaces"), this);
  QVBox *spaceLayout = new QVBox(gbSpaces);
  opt[0] = new QCheckBox(i18n("Use &spaces instead of tabs to indent"), spaceLayout );

  indentationWidth = new KIntNumInput(KateDocumentConfig::global()->indentationWidth(), spaceLayout);
  indentationWidth->setRange(1, 16, 1, false);
  indentationWidth->setLabel(i18n("Number of spaces:"), AlignVCenter);

  opt[1] = new QCheckBox(i18n("Keep indent &profile"), this);
  opt[2] = new QCheckBox(i18n("&Keep extra spaces"), this);

  QVGroupBox *keys = new QVGroupBox(i18n("Keys to Use"), this);
  opt[3] = new QCheckBox(i18n("&Tab key indents"), keys);
  opt[4] = new QCheckBox(i18n("&Backspace key indents"), keys);

  QRadioButton *rb1, *rb2, *rb3;
  m_tabs = new QButtonGroup( 1, Qt::Horizontal, i18n("Tab Key Mode if Nothing Selected"), this );
  m_tabs->setRadioButtonExclusive( true );
  m_tabs->insert( rb1=new QRadioButton( i18n("Insert indent &characters"), m_tabs ), 0 );
  m_tabs->insert( rb2=new QRadioButton( i18n("I&nsert tab character"), m_tabs ), 1 );
  m_tabs->insert( rb3=new QRadioButton( i18n("Indent current &line"), m_tabs ), 2 );

  opt[0]->setChecked(configFlags & flags[0]);
  opt[1]->setChecked(configFlags & flags[1]);
  opt[2]->setChecked(configFlags & flags[2]);
  opt[3]->setChecked(configFlags & flags[3]);
  opt[4]->setChecked(configFlags & flags[4]);
  opt[5]->setChecked(configFlags & flags[5]);

  layout->addWidget(gbAuto);
  layout->addWidget(gbSpaces);
  layout->addWidget(opt[1]);
  layout->addWidget(opt[2]);
  layout->addWidget(keys);
  layout->addWidget(m_tabs, 0);

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n(
        "Check this if you want to indent with spaces rather than tabs."));
  QWhatsThis::add(opt[2], i18n(
        "Indentations of more than the selected number of spaces will not be "
        "shortened."));
  QWhatsThis::add(opt[3], i18n(
        "This allows the <b>Tab</b> key to be used to increase the indentation "
        "level."));
  QWhatsThis::add(opt[4], i18n(
        "This allows the <b>Backspace</b> key to be used to decrease the "
        "indentation level."));
  QWhatsThis::add(opt[5], i18n(
        "Automatically inserts a leading \"*\" while typing within a Doxygen "
        "style comment."));
  QWhatsThis::add(indentationWidth, i18n("The number of spaces to indent with."));

  reload ();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(m_indentMode, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect(m_indentMode, SIGNAL(activated(int)), this, SLOT(indenterSelected(int)));

  connect( opt[0], SIGNAL(toggled(bool)), this, SLOT(somethingToggled()));

  connect( opt[0], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( opt[1], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( opt[2], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( opt[3], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( opt[4], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( opt[5], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  connect(indentationWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
}

void KateIndentConfigTab::somethingToggled() {
  indentationWidth->setEnabled(opt[0]->isChecked());
}

void KateIndentConfigTab::indenterSelected (int index)
{
  if (index == KateDocumentConfig::imCStyle || index == KateDocumentConfig::imCSAndS)
    opt[5]->setEnabled(true);
  else
    opt[5]->setEnabled(false);
}

void KateIndentConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  KateDocumentConfig::global()->configStart ();

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

  KateDocumentConfig::global()->configEnd ();
}

void KateIndentConfigTab::reload ()
{
  if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfTabIndentsMode)
    m_tabs->setButton (2);
  else if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfTabInsertsTab)
    m_tabs->setButton (1);
  else
    m_tabs->setButton (0);

  m_indentMode->setCurrentItem (KateDocumentConfig::global()->indentationMode());

  somethingToggled ();
  indenterSelected (m_indentMode->currentItem());
}
//END KateIndentConfigTab

//BEGIN KateSelectConfigTab
KateSelectConfigTab::KateSelectConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  QRadioButton *rb1, *rb2;

  m_tabs = new QButtonGroup( 1, Qt::Horizontal, i18n("Selection Mode"), this );
  layout->add (m_tabs);

  m_tabs->setRadioButtonExclusive( true );
  m_tabs->insert( rb1=new QRadioButton( i18n("&Normal"), m_tabs ), 0 );
  m_tabs->insert( rb2=new QRadioButton( i18n("&Persistent"), m_tabs ), 1 );


  layout->addStretch();

  QWhatsThis::add(rb1, i18n(
        "Selections will be overwritten by typed text and will be lost on "
        "cursor movement."));
  QWhatsThis::add(rb2, i18n(
        "Selections will stay even after cursor movement and typing."));

  reload ();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
}

void KateSelectConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  KateDocumentConfig::global()->configStart ();

  int configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocumentConfig::cfPersistent; // clear persistent

  if (m_tabs->id (m_tabs->selected()) == 1)
    configFlags |= KateDocumentConfig::cfPersistent; // set flag if checked

  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateDocumentConfig::global()->configEnd ();
}

void KateSelectConfigTab::reload ()
{
  if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfPersistent)
    m_tabs->setButton (1);
  else
    m_tabs->setButton (0);
}
//END KateSelectConfigTab

//BEGIN KateEditConfigTab
const int KateEditConfigTab::flags[] = {KateDocument::cfWordWrap,
  KateDocument::cfAutoBrackets, KateDocument::cfShowTabs, KateDocument::cfSmartHome,
  KateDocument::cfWrapCursor, KateDocumentConfig::cfReplaceTabsDyn, KateDocumentConfig::cfRemoveTrailingDyn};

KateEditConfigTab::KateEditConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = KateDocumentConfig::global()->configFlags();

  QVGroupBox *gbWhiteSpace = new QVGroupBox(i18n("Tabulators"), this);

  opt[2] = new QCheckBox(i18n("&Show tabs"), gbWhiteSpace);
  opt[2]->setChecked(configFlags & flags[2]);
  connect(opt[2], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  opt[5] = new QCheckBox( i18n("&Replace tabs with spaces"), gbWhiteSpace );
  opt[5]->setChecked( configFlags & KateDocumentConfig::cfReplaceTabsDyn );
  connect( opt[5], SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );

  e2 = new KIntNumInput(KateDocumentConfig::global()->tabWidth(), gbWhiteSpace);
  e2->setRange(1, 16, 1, false);
  e2->setLabel(i18n("Tab width:"), AlignVCenter);
  connect(e2, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  mainLayout->addWidget(gbWhiteSpace);

  QVGroupBox *gbWordWrap = new QVGroupBox(i18n("Static Word Wrap"), this);

  opt[0] = new QCheckBox(i18n("Enable static &word wrap"), gbWordWrap);
  opt[0]->setChecked(KateDocumentConfig::global()->wordWrap());
  connect(opt[0], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

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

  e6 = new QCheckBox(i18n("&PageUp/PageDown moves cursor"), gbCursor);
  e6->setChecked(KateDocumentConfig::global()->pageUpDownMovesCursor());
  connect(e6, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  e4 = new KIntNumInput(KateViewConfig::global()->autoCenterLines(), gbCursor);
  e4->setRange(0, 1000000, 1, false);
  e4->setLabel(i18n("Autocenter cursor (lines):"), AlignVCenter);
  connect(e4, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  mainLayout->addWidget(gbCursor);

  opt[6] = new QCheckBox( i18n("Remove &trailing spaces"), this );
  mainLayout->addWidget( opt[6] );
  opt[6]->setChecked( configFlags & KateDocumentConfig::cfRemoveTrailingDyn );
  connect( opt[6], SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );

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
  QWhatsThis::add(opt[0], i18n(
        "Automatically start a new line of text when the current line exceeds "
        "the length specified by the <b>Wrap words at:</b> option."
        "<p>This option does not wrap existing lines of text - use the <b>Apply "
        "Static Word Wrap</b> option in the <b>Tools</b> menu for that purpose."
        "<p>If you want lines to be <i>visually wrapped</i> instead, according "
        "to the width of the view, enable <b>Dynamic Word Wrap</b> in the "
        "<b>View Defaults</b> config page."));
  QWhatsThis::add(e1, i18n(
        "If the Word Wrap option is selected this entry determines the length "
        "(in characters) at which the editor will automatically start a new line."));
  QWhatsThis::add(opt[1], i18n(
        "When the user types a left bracket ([,(, or {) KateView automatically "
        "enters the right bracket (}, ), or ]) to the right of the cursor."));
  QWhatsThis::add(opt[2], i18n(
        "The editor will display a symbol to indicate the presence of a tab in "
        "the text."));
  QWhatsThis::add(opt[3], i18n(
        "When selected, pressing the home key will cause the cursor to skip "
        "whitespace and go to the start of a line's text."));
  QWhatsThis::add(e3, i18n(
        "Sets the number of undo/redo steps to record. More steps uses more memory."));
  QWhatsThis::add(e4, i18n(
        "Sets the number of lines to maintain visible above and below the "
        "cursor when possible."));
  QWhatsThis::add(opt[4], i18n(
        "When on, moving the insertion cursor using the <b>Left</b> and "
        "<b>Right</b> keys will go on to previous/next line at beginning/end of "
        "the line, similar to most editors.<p>When off, the insertion cursor "
        "cannot be moved left of the line start, but it can be moved off the "
        "line end, which can be very handy for programmers."));
  QWhatsThis::add(e6, i18n("Selects whether the PageUp and PageDown keys should alter the vertical position of the cursor relative to the top of the view."));
  QString gstfwt = i18n(
        "This determines where KateView will get the search text from "
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
  QWhatsThis::add( opt[5], i18n(
      "If this is enabled, the editor will calculate the number of spaces up to "
      "the next tab position as defined by the tab width, and insert that number "
      "of spaces instead of a TAB character." ) );
  QWhatsThis::add( opt[6], i18n(
      "If this is enabled, the editor will remove any trailing whitespace on "
      "lines when they are left by the insertion cursor.") );
}

void KateEditConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  KateViewConfig::global()->configStart ();
  KateDocumentConfig::global()->configStart ();

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
  // xgettext:no-c-format
  m_dynwrapAlignLevel->setSuffix(i18n("% of View Width"));
  m_dynwrapAlignLevel->setSpecialValueText(i18n("Disabled"));

  m_wwmarker = new QCheckBox( i18n("&Show static word wrap marker (if applicable)"), gbWordWrap );

  blay->addWidget(gbWordWrap);

  QVGroupBox *gbFold = new QVGroupBox(i18n("Code Folding"), this);

  m_folding=new QCheckBox(i18n("Show &folding markers (if available)"), gbFold );
  m_collapseTopLevel = new QCheckBox( i18n("Collapse toplevel folding nodes"), gbFold );
  m_collapseTopLevel->hide ();

  blay->addWidget(gbFold);

  QVGroupBox *gbBar = new QVGroupBox(i18n("Borders"), this);

  m_icons=new QCheckBox(i18n("Show &icon border"),gbBar);
  m_line=new QCheckBox(i18n("Show &line numbers"),gbBar);
  m_scrollBarMarks=new QCheckBox(i18n("Show &scrollbar marks"),gbBar);

  blay->addWidget(gbBar);

  m_bmSort = new QButtonGroup( 1, Qt::Horizontal, i18n("Sort Bookmarks Menu"), this );
  m_bmSort->setRadioButtonExclusive( true );
  m_bmSort->insert( rb1=new QRadioButton( i18n("By &position"), m_bmSort ), 0 );
  m_bmSort->insert( rb2=new QRadioButton( i18n("By c&reation"), m_bmSort ), 1 );

  blay->addWidget(m_bmSort, 0 );
  blay->addStretch(1000);

  QWhatsThis::add(m_dynwrap,i18n(
        "If this option is checked, the text lines will be wrapped at the view "
        "border on the screen."));
  QString wtstr = i18n("Choose when the Dynamic Word Wrap Indicators should be displayed");
  QWhatsThis::add(m_dynwrapIndicatorsLabel, wtstr);
  QWhatsThis::add(m_dynwrapIndicatorsCombo, wtstr);
  // xgettext:no-c-format
  QWhatsThis::add(m_dynwrapAlignLevel, i18n(
        "<p>Enables the start of dynamically wrapped lines to be aligned "
        "vertically to the indentation level of the first line.  This can help "
        "to make code and markup more readable.</p><p>Additionally, this allows "
        "you to set a maximum width of the screen, as a percentage, after which "
        "dynamically wrapped lines will no longer be vertically aligned.  For "
        "example, at 50%, lines whose indentation levels are deeper than 50% of "
        "the width of the screen will not have vertical alignment applied to "
        "subsequent wrapped lines.</p>"));
  QWhatsThis::add( m_wwmarker, i18n(
        "<p>If this option is checked, a vertical line will be drawn at the word "
        "wrap column as defined in the <strong>Editing</strong> properties."
        "<p>Note that the word wrap marker is only drawn if you use a fixed "
        "pitch font." ));
  QWhatsThis::add(m_line,i18n(
        "If this option is checked, every new view will display line numbers "
        "on the left hand side."));
  QWhatsThis::add(m_icons,i18n(
        "If this option is checked, every new view will display an icon border "
        "on the left hand side.<br><br>The icon border shows bookmark signs, "
        "for instance."));
  QWhatsThis::add(m_scrollBarMarks,i18n(
        "If this option is checked, every new view will show marks on the "
        "vertical scrollbar.<br><br>These marks will, for instance, show "
        "bookmarks."));
  QWhatsThis::add(m_folding,i18n(
        "If this option is checked, every new view will display marks for code "
        "folding, if code folding is available."));
  QWhatsThis::add(m_bmSort,i18n(
        "Choose how the bookmarks should be ordered in the <b>Bookmarks</b> menu."));
  QWhatsThis::add(rb1,i18n(
        "The bookmarks will be ordered by the line numbers they are placed at."));
  QWhatsThis::add(rb2,i18n(
        "Each new bookmark will be added to the bottom, independently from "
        "where it is placed in the document."));

  reload();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(m_dynwrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_dynwrapIndicatorsCombo, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect(m_dynwrapAlignLevel, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect(m_wwmarker, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_icons, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_scrollBarMarks, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_line, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_folding, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_collapseTopLevel, SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );
  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
}

KateViewDefaultsConfig::~KateViewDefaultsConfig()
{
}

void KateViewDefaultsConfig::apply ()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  KateViewConfig::global()->configStart ();
  KateRendererConfig::global()->configStart ();

  KateViewConfig::global()->setDynWordWrap (m_dynwrap->isChecked());
  KateViewConfig::global()->setDynWordWrapIndicators (m_dynwrapIndicatorsCombo->currentItem ());
  KateViewConfig::global()->setDynWordWrapAlignIndent(m_dynwrapAlignLevel->value());
  KateRendererConfig::global()->setWordWrapMarker (m_wwmarker->isChecked());
  KateViewConfig::global()->setLineNumbers (m_line->isChecked());
  KateViewConfig::global()->setIconBar (m_icons->isChecked());
  KateViewConfig::global()->setScrollBarMarks (m_scrollBarMarks->isChecked());
  KateViewConfig::global()->setFoldingBar (m_folding->isChecked());
  KateViewConfig::global()->setBookmarkSort (m_bmSort->id (m_bmSort->selected()));

  KateRendererConfig::global()->configEnd ();
  KateViewConfig::global()->configEnd ();
}

void KateViewDefaultsConfig::reload ()
{
  m_dynwrap->setChecked(KateViewConfig::global()->dynWordWrap());
  m_dynwrapIndicatorsCombo->setCurrentItem( KateViewConfig::global()->dynWordWrapIndicators() );
  m_dynwrapAlignLevel->setValue(KateViewConfig::global()->dynWordWrapAlignIndent());
  m_wwmarker->setChecked( KateRendererConfig::global()->wordWrapMarker() );
  m_line->setChecked(KateViewConfig::global()->lineNumbers());
  m_icons->setChecked(KateViewConfig::global()->iconBar());
  m_scrollBarMarks->setChecked(KateViewConfig::global()->scrollBarMarks());
  m_folding->setChecked(KateViewConfig::global()->foldingBar());
  m_bmSort->setButton( KateViewConfig::global()->bookmarkSort()  );
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
  if (!m_ready)
  {
    (new QVBoxLayout(this))->setAutoAdd(true);
    KateView* view = (KateView*)m_doc->views().at(0);
    m_ac = view->editActionCollection();
    m_keyChooser = new KKeyChooser( m_ac, this, false );
    connect( m_keyChooser, SIGNAL( keyChange() ), this, SLOT( slotChanged() ) );
    m_keyChooser->show ();

    m_ready = true;
  }

  QWidget::show ();
}

void KateEditKeyConfiguration::apply()
{
  if (m_ready)
  {
    m_keyChooser->commitChanges();
    m_ac->writeShortcutSettings( "Katepart Shortcuts" );
  }
}
//END KateEditKeyConfiguration

//BEGIN KateSaveConfigTab
KateSaveConfigTab::KateSaveConfigTab( QWidget *parent )
  : KateConfigPage( parent )
{
  int configFlags = KateDocumentConfig::global()->configFlags();
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  QVGroupBox *gbEnc = new QVGroupBox(i18n("File Format"), this);
  layout->addWidget( gbEnc );

  QHBox *e5Layout = new QHBox(gbEnc);
  QLabel *e5Label = new QLabel(i18n("&Encoding:"), e5Layout);
  m_encoding = new KComboBox (e5Layout);
  e5Label->setBuddy(m_encoding);

  e5Layout = new QHBox(gbEnc);
  e5Label = new QLabel(i18n("End &of line:"), e5Layout);
  m_eol = new KComboBox (e5Layout);
  e5Label->setBuddy(m_eol);

  m_eol->insertItem (i18n("UNIX"));
  m_eol->insertItem (i18n("DOS/Windows"));
  m_eol->insertItem (i18n("Macintosh"));

  QVGroupBox *gbMem = new QVGroupBox(i18n("Memory Usage"), this);
  layout->addWidget( gbMem );

  e5Layout = new QHBox(gbMem);
  e5Layout->setSpacing (32);
  blockCountLabel = new QLabel(i18n("Maximum loaded &blocks per file:"), e5Layout);
  blockCount = new QSpinBox (4, 512, 4, e5Layout);

  blockCount->setValue (KateBuffer::maxLoadedBlocks());
  blockCountLabel->setBuddy(blockCount);

  QVGroupBox *gbWhiteSpace = new QVGroupBox(i18n("Automatic Cleanups on Save"), this);
  layout->addWidget( gbWhiteSpace );

  replaceTabs = new QCheckBox(i18n("Replace &tabs with spaces"), gbWhiteSpace);
  replaceTabs->setChecked(configFlags & KateDocument::cfReplaceTabs);

  removeSpaces = new QCheckBox(i18n("Re&move trailing spaces"), gbWhiteSpace);
  removeSpaces->setChecked(configFlags & KateDocument::cfRemoveSpaces);

  QVGroupBox *dirConfigBox = new QVGroupBox(i18n("Folder Config File"), this);
  layout->addWidget( dirConfigBox );

  dirSearchDepth = new KIntNumInput(KateDocumentConfig::global()->searchDirConfigDepth(), dirConfigBox);
  dirSearchDepth->setRange(-1, 64, 1, false);
  dirSearchDepth->setSpecialValueText( i18n("Do not use config file") );
  dirSearchDepth->setLabel(i18n("Se&arch depth for config file:"), AlignVCenter);

  QGroupBox *gb = new QGroupBox( 1, Qt::Horizontal, i18n("Backup on Save"), this );
  layout->addWidget( gb );
  cbLocalFiles = new QCheckBox( i18n("&Local files"), gb );
  cbRemoteFiles = new QCheckBox( i18n("&Remote files"), gb );

  QHBox *hbBuPrefix = new QHBox( gb );
  QLabel *lBuPrefix = new QLabel( i18n("&Prefix:"), hbBuPrefix );
  leBuPrefix = new QLineEdit( hbBuPrefix );
  lBuPrefix->setBuddy( leBuPrefix );

  QHBox *hbBuSuffix = new QHBox( gb );
  QLabel *lBuSuffix = new QLabel( i18n("&Suffix:"), hbBuSuffix );
  leBuSuffix = new QLineEdit( hbBuSuffix );
  lBuSuffix->setBuddy( leBuSuffix );

  layout->addStretch();

  QWhatsThis::add(replaceTabs, i18n(
        "KateView will replace any tabs with the number of spaces indicated in "
        "the Tab Width: entry."));
  QWhatsThis::add(removeSpaces, i18n(
        "KateView will automatically eliminate extra spaces at the ends of "
        "lines of text."));
  QWhatsThis::add( gb, i18n(
        "<p>Backing up on save will cause Kate to copy the disk file to "
        "'&lt;prefix&gt;&lt;filename&gt;&lt;suffix&gt;' before saving changes."
        "<p>The suffix defaults to <strong>~</strong> and prefix is empty by default" ) );
  QWhatsThis::add( cbLocalFiles, i18n(
        "Check this if you want backups of local files when saving") );
  QWhatsThis::add( cbRemoteFiles, i18n(
        "Check this if you want backups of remote files when saving") );
  QWhatsThis::add( leBuPrefix, i18n(
        "Enter the prefix to prepend to the backup file names" ) );
  QWhatsThis::add( leBuSuffix, i18n(
        "Enter the suffix to add to the backup file names" ) );
  QWhatsThis::add(dirSearchDepth, i18n(
        "The editor will search the given number of folder levels upwards for .kateconfig file"
        " and load the settings line from it." ));
  QWhatsThis::add(blockCount, i18n(
        "The editor will load given number of blocks (of around 2048 lines) of text into memory;"
        " if the filesize is bigger than this the other blocks are swapped "
        " to disk and loaded transparently as-needed.<br>"
        " This can cause little delays while navigating in the document; a larger block count"
        " increases the editing speed at the cost of memory. <br>For normal usage, just choose the highest possible"
        " block count: limit it only if you have problems with the memory usage."));

  reload();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(m_encoding, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect(m_eol, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect(blockCount, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect(replaceTabs, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(removeSpaces, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect( cbLocalFiles, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect( cbRemoteFiles, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect(dirSearchDepth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect( leBuPrefix, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( leBuSuffix, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
}

void KateSaveConfigTab::apply()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  KateBuffer::setMaxLoadedBlocks (blockCount->value());

  KateDocumentConfig::global()->configStart ();

  if ( leBuSuffix->text().isEmpty() && leBuPrefix->text().isEmpty() ) {
    KMessageBox::information(
                this,
                i18n("You did not provide a backup suffix or prefix. Using default suffix: '~'"),
                i18n("No Backup Suffix or Prefix")
                        );
    leBuSuffix->setText( "~" );
  }

  uint f( 0 );
  if ( cbLocalFiles->isChecked() )
    f |= KateDocumentConfig::LocalFiles;
  if ( cbRemoteFiles->isChecked() )
    f |= KateDocumentConfig::RemoteFiles;

  KateDocumentConfig::global()->setBackupFlags(f);
  KateDocumentConfig::global()->setBackupPrefix(leBuPrefix->text());
  KateDocumentConfig::global()->setBackupSuffix(leBuSuffix->text());

  KateDocumentConfig::global()->setSearchDirConfigDepth(dirSearchDepth->value());

  int configFlags = KateDocumentConfig::global()->configFlags();

  configFlags &= ~KateDocument::cfReplaceTabs; // clear flag
  if (replaceTabs->isChecked()) configFlags |= KateDocument::cfReplaceTabs; // set flag if checked

  configFlags &= ~KateDocument::cfRemoveSpaces; // clear flag
  if (removeSpaces->isChecked()) configFlags |= KateDocument::cfRemoveSpaces; // set flag if checked

  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateDocumentConfig::global()->setEncoding((m_encoding->currentItem() == 0) ? "" : KGlobal::charsets()->encodingForName(m_encoding->currentText()));

  KateDocumentConfig::global()->setEol(m_eol->currentItem());

  KateDocumentConfig::global()->configEnd ();
}

void KateSaveConfigTab::reload()
{
  // encoding
  m_encoding->clear ();
  m_encoding->insertItem (i18n("KDE Default"));
  m_encoding->setCurrentItem(0);
  QStringList encodings (KGlobal::charsets()->descriptiveEncodingNames());
  int insert = 1;
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

  dirSearchDepth->setValue(KateDocumentConfig::global()->searchDirConfigDepth());

  // other stuff
  uint f ( KateDocumentConfig::global()->backupFlags() );
  cbLocalFiles->setChecked( f & KateDocumentConfig::LocalFiles );
  cbRemoteFiles->setChecked( f & KateDocumentConfig::RemoteFiles );
  leBuPrefix->setText( KateDocumentConfig::global()->backupPrefix() );
  leBuSuffix->setText( KateDocumentConfig::global()->backupSuffix() );
}

void KateSaveConfigTab::reset()
{
}

void KateSaveConfigTab::defaults()
{
  cbLocalFiles->setChecked( true );
  cbRemoteFiles->setChecked( false );
  leBuPrefix->setText( "" );
  leBuSuffix->setText( "~" );
}

//END KateSaveConfigTab

//BEGIN PluginListItem
class KatePartPluginListItem : public QCheckListItem
{
  public:
    KatePartPluginListItem(bool checked, uint i, const QString &name, QListView *parent);
    uint pluginIndex () const { return index; }

  protected:
    void stateChange(bool);

  private:
    uint index;
    bool silentStateChange;
};

KatePartPluginListItem::KatePartPluginListItem(bool checked, uint i, const QString &name, QListView *parent)
  : QCheckListItem(parent, name, CheckBox)
  , index(i)
  , silentStateChange(false)
{
  silentStateChange = true;
  setOn(checked);
  silentStateChange = false;
}

void KatePartPluginListItem::stateChange(bool b)
{
  if(!silentStateChange)
    static_cast<KatePartPluginListView *>(listView())->stateChanged(this, b);
}
//END

//BEGIN PluginListView
KatePartPluginListView::KatePartPluginListView(QWidget *parent, const char *name)
  : KListView(parent, name)
{
}

void KatePartPluginListView::stateChanged(KatePartPluginListItem *item, bool b)
{
  emit stateChange(item, b);
}
//END

//BEGIN KatePartPluginConfigPage
KatePartPluginConfigPage::KatePartPluginConfigPage (QWidget *parent) : KateConfigPage (parent, "")
{
  // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );
  grid->setSpacing( KDialogBase::spacingHint() );

  listView = new KatePartPluginListView(this);
  listView->addColumn(i18n("Name"));
  listView->addColumn(i18n("Comment"));

  grid->addWidget( listView, 0, 0);

  for (uint i=0; i<KateFactory::self()->plugins().count(); i++)
  {
    KatePartPluginListItem *item = new KatePartPluginListItem(KateDocumentConfig::global()->plugin(i), i, (KateFactory::self()->plugins())[i]->name(), listView);
    item->setText(0, (KateFactory::self()->plugins())[i]->name());
    item->setText(1, (KateFactory::self()->plugins())[i]->comment());

    m_items.append (item);
  }

  // configure button

  btnConfigure = new QPushButton( i18n("Configure..."), this );
  btnConfigure->setEnabled( false );
  grid->addWidget( btnConfigure, 1, 0, Qt::AlignRight );
  connect( btnConfigure, SIGNAL(clicked()), this, SLOT(slotConfigure()) );

  connect( listView, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(slotCurrentChanged(QListViewItem*)) );
  connect( listView, SIGNAL(stateChange(KatePartPluginListItem *, bool)),
    this, SLOT(slotStateChanged(KatePartPluginListItem *, bool)));
  connect(listView, SIGNAL(stateChange(KatePartPluginListItem *, bool)), this, SLOT(slotChanged()));
}

KatePartPluginConfigPage::~KatePartPluginConfigPage ()
{
}

void KatePartPluginConfigPage::apply ()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  KateDocumentConfig::global()->configStart ();

  for (uint i=0; i < m_items.count(); i++)
    KateDocumentConfig::global()->setPlugin (m_items.at(i)->pluginIndex(), m_items.at(i)->isOn());

  KateDocumentConfig::global()->configEnd ();
}

void KatePartPluginConfigPage::slotStateChanged( KatePartPluginListItem *item, bool b )
{
  if ( b )
    slotCurrentChanged( (QListViewItem*)item );
}

void KatePartPluginConfigPage::slotCurrentChanged( QListViewItem* i )
{
  KatePartPluginListItem *item = static_cast<KatePartPluginListItem *>(i);
  if ( ! item ) return;

    bool b = false;
  if ( item->isOn() )
  {

    // load this plugin, and see if it has config pages
    KTextEditor::Plugin *plugin = KTextEditor::createPlugin(QFile::encodeName((KateFactory::self()->plugins())[item->pluginIndex()]->library()));
    if ( plugin ) {
      KTextEditor::ConfigInterfaceExtension *cie = KTextEditor::configInterfaceExtension( plugin );
      b = ( cie && cie->configPages() );
    }

  }
    btnConfigure->setEnabled( b );
}

void KatePartPluginConfigPage::slotConfigure()
{
  KatePartPluginListItem *item = static_cast<KatePartPluginListItem*>(listView->currentItem());
  KTextEditor::Plugin *plugin =
    KTextEditor::createPlugin(QFile::encodeName((KateFactory::self()->plugins())[item->pluginIndex()]->library()));

  if ( ! plugin ) return;

  KTextEditor::ConfigInterfaceExtension *cife =
    KTextEditor::configInterfaceExtension( plugin );

  if ( ! cife )
    return;

  if ( ! cife->configPages() )
    return;

  // If we have only one page, we use a simple dialog, else an icon list type
  KDialogBase::DialogType dt =
    cife->configPages() > 1 ?
      KDialogBase::IconList :     // still untested
      KDialogBase::Plain;

  QString name = (KateFactory::self()->plugins())[item->pluginIndex()]->name();
  KDialogBase *kd = new KDialogBase ( dt,
              i18n("Configure %1").arg( name ),
              KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Help,
              KDialogBase::Ok,
              this );

  QPtrList<KTextEditor::ConfigPage> editorPages;

  for (uint i = 0; i < cife->configPages (); i++)
  {
    QWidget *page;
    if ( dt == KDialogBase::IconList )
    {
      QStringList path;
      path.clear();
      path << cife->configPageName( i );
      page = kd->addVBoxPage( path, cife->configPageFullName (i),
                                cife->configPagePixmap(i, KIcon::SizeMedium) );
    }
    else
    {
      page = kd->plainPage();
      QVBoxLayout *_l = new QVBoxLayout( page );
      _l->setAutoAdd( true );
    }

    editorPages.append( cife->configPage( i, page ) );
  }

  if (kd->exec())
  {

    for( uint i=0; i<editorPages.count(); i++ )
    {
      editorPages.at( i )->apply();
    }
  }

  delete kd;
}
//END KatePartPluginConfigPage

//BEGIN KateHlConfigPage
KateHlConfigPage::KateHlConfigPage (QWidget *parent)
 : KateConfigPage (parent, "")
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

  for( int i = 0; i < KateHlManager::self()->highlights(); i++) {
    if (KateHlManager::self()->hlSection(i).length() > 0)
      hlCombo->insertItem(KateHlManager::self()->hlSection(i) + QString ("/") + KateHlManager::self()->hlNameTranslated(i));
    else
      hlCombo->insertItem(KateHlManager::self()->hlNameTranslated(i));
  }
  hlCombo->setCurrentItem(0);

  QGroupBox *gbInfo = new QGroupBox( 1, Qt::Horizontal, i18n("Information"), this );
  layout->add (gbInfo);

  // author
  QHBox *hb1 = new QHBox( gbInfo);
  new QLabel( i18n("Author:"), hb1 );
  author  = new QLabel (hb1);
  author->setTextFormat (Qt::RichText);

  // license
  QHBox *hb2 = new QHBox( gbInfo);
  new QLabel( i18n("License:"), hb2 );
  license  = new QLabel (hb2);

  QGroupBox *gbProps = new QGroupBox( 1, Qt::Horizontal, i18n("Properties"), this );
  layout->add (gbProps);

  // file & mime types
  QHBox *hbFE = new QHBox( gbProps);
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), hbFE );
  wildcards  = new QLineEdit( hbFE );
  lFileExts->setBuddy( wildcards );

  QHBox *hbMT = new QHBox( gbProps );
  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), hbMT);
  mimetypes = new QLineEdit( hbMT );
  lMimeTypes->setBuddy( mimetypes );

  QHBox *hbMT2 = new QHBox( gbProps );
  QLabel *lprio = new QLabel( i18n("Prio&rity:"), hbMT2);
  priority = new KIntNumInput( hbMT2 );

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

  QWhatsThis::add( hlCombo, i18n(
        "Choose a <em>Syntax Highlight mode</em> from this list to view its "
        "properties below.") );
  QWhatsThis::add( wildcards, i18n(
        "The list of file extensions used to determine which files to highlight "
        "using the current syntax highlight mode.") );
  QWhatsThis::add( mimetypes, i18n(
        "The list of Mime Types used to determine which files to highlight "
        "using the current highlight mode.<p>Click the wizard button on the "
        "left of the entry field to display the MimeType selection dialog.") );
  QWhatsThis::add( btnMTW, i18n(
        "Display a dialog with a list of all available mime types to choose from."
        "<p>The <strong>File Extensions</strong> entry will automatically be "
        "edited as well.") );
  QWhatsThis::add( btnDl, i18n(
        "Click this button to download new or updated syntax highlight "
        "descriptions from the Kate website.") );

  layout->addStretch ();

  connect( wildcards, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( mimetypes, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );
  connect( priority, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );
}

KateHlConfigPage::~KateHlConfigPage ()
{
}

void KateHlConfigPage::apply ()
{
  // nothing changed, no need to apply stuff
  if (!changed())
    return;

  writeback();

  for ( QIntDictIterator<KateHlData> it( hlDataDict ); it.current(); ++it )
    KateHlManager::self()->getHl( it.currentKey() )->setData( it.current() );

  KateHlManager::self()->getKConfig()->sync ();
}

void KateHlConfigPage::reload ()
{
}

void KateHlConfigPage::hlChanged(int z)
{
  writeback();

  KateHighlighting *hl = KateHlManager::self()->getHl( z );

  if (!hl)
  {
    hlData = 0;
    return;
  }

  if ( !hlDataDict.find( z ) )
    hlDataDict.insert( z, hl->getData() );

  hlData = hlDataDict.find( z );
  wildcards->setText(hlData->wildcards);
  mimetypes->setText(hlData->mimetypes);
  priority->setValue(hlData->priority);

  // split author string if needed into multiple lines !
  QStringList l= QStringList::split (QRegExp("[,;]"), hl->author());
  author->setText (l.join ("<br>"));

  license->setText (hl->license());
}

void KateHlConfigPage::writeback()
{
  if (hlData)
  {
    hlData->wildcards = wildcards->text();
    hlData->mimetypes = mimetypes->text();
    hlData->priority = priority->value();
  }
}

void KateHlConfigPage::hlDownload()
{
  KateHlDownloadDialog diag(this,"hlDownload",true);
  diag.exec();
}

void KateHlConfigPage::showMTDlg()
{
  QString text = i18n("Select the MimeTypes you want highlighted using the '%1' syntax highlight rules.\nPlease note that this will automatically edit the associated file extensions as well.").arg( hlCombo->currentText() );
  QStringList list = QStringList::split( QRegExp("\\s*;\\s*"), mimetypes->text() );
  KMimeTypeChooserDialog *d = new KMimeTypeChooserDialog( i18n("Select Mime Types"), text, list, "text", this );

  if ( d->exec() == KDialogBase::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    wildcards->setText(d->chooser()->patterns().join(";"));
    mimetypes->setText(d->chooser()->mimeTypes().join(";"));
  }
}
//END KateHlConfigPage

//BEGIN KateHlDownloadDialog
KateHlDownloadDialog::KateHlDownloadDialog(QWidget *parent, const char *name, bool modal)
  :KDialogBase(KDialogBase::Swallow, i18n("Highlight Download"), User1|Close, User1, parent, name, modal, true, i18n("&Install"))
{
  QVBox* vbox = new QVBox(this);
  setMainWidget(vbox);
  vbox->setSpacing(spacingHint());
  new QLabel(i18n("Select the syntax highlighting files you want to update:"), vbox);
  list = new QListView(vbox);
  list->addColumn("");
  list->addColumn(i18n("Name"));
  list->addColumn(i18n("Installed"));
  list->addColumn(i18n("Latest"));
  list->setSelectionMode(QListView::Multi);
  list->setAllColumnsShowFocus(true);

  new QLabel(i18n("<b>Note:</b> New versions are selected automatically."), vbox);
  actionButton (User1)->setIconSet(SmallIconSet("ok"));

  transferJob = KIO::get(
    KURL(QString(HLDOWNLOADPATH)
       + QString("update-")
       + QString(KATEPART_VERSION)
       + QString(".xml")), true, true );
  connect(transferJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
    this, SLOT(listDataReceived(KIO::Job *, const QByteArray &)));
//        void data( KIO::Job *, const QByteArray &data);
  resize(450, 400);
}

KateHlDownloadDialog::~KateHlDownloadDialog(){}

void KateHlDownloadDialog::listDataReceived(KIO::Job *, const QByteArray &data)
{
  if (!transferJob || transferJob->isErrorPage())
  {
    actionButton(User1)->setEnabled(false);
    return;
  }

  listData+=QString(data);
  kdDebug(13000)<<QString("CurrentListData: ")<<listData<<endl<<endl;
  kdDebug(13000)<<QString("Data length: %1").arg(data.size())<<endl;
  kdDebug(13000)<<QString("listData length: %1").arg(listData.length())<<endl;
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
          if (hl && hl->name()==Name)
          {
            installedVersion="    "+hl->version();
            break;
          }
          else hl = 0;
        }

        // autoselect entry if new or updated.
        QListViewItem* entry = new QListViewItem(
          list, "", e.attribute("name"), installedVersion,
          e.attribute("version"),e.attribute("url"));
        if (!hl || hl->version() < e.attribute("version"))
        {
          entry->setSelected(true);
          entry->setPixmap(0, SmallIcon(("knewstuff")));
        }
      }
    }
  }
}

void KateHlDownloadDialog::slotUser1()
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
  KateSyntaxDocument doc (true);
}
//END KateHlDownloadDialog

//BEGIN KateGotoLineDialog
KateGotoLineDialog::KateGotoLineDialog(QWidget *parent, int line, int max)
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

int KateGotoLineDialog::getLine() {
  return e1->value();
}
//END KateGotoLineDialog

//BEGIN KateModOnHdPrompt
KateModOnHdPrompt::KateModOnHdPrompt( KateDocument *doc,
                                      int modtype,
                                      const QString &reason,
                                      QWidget *parent )
  : KDialogBase( parent, "", true, "", Ok|Apply|Cancel|User1 ),
    m_doc( doc ),
    m_modtype ( modtype ),
    m_tmpfile( 0 )
{
  QString title, btnOK, whatisok;
  if ( modtype == 3 ) // deleted
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

  setButtonText( Ok, btnOK);
  setButtonText( Apply, i18n("&Ignore") );

  setButtonWhatsThis( Ok, whatisok );
  setButtonWhatsThis( Apply, i18n("Ignore the changes. You will not be prompted again.") );
  setButtonWhatsThis( Cancel, i18n("Do nothing. Next time you focus the file, "
      "or try to save it or close it, you will be prompted again.") );

  enableButtonSeparator( true );
  setCaption( title );

  QFrame *w = makeMainWidget();
  QVBoxLayout *lo = new QVBoxLayout( w );
  QHBoxLayout *lo1 = new QHBoxLayout( lo );
  QLabel *icon = new QLabel( w );
  icon->setPixmap( DesktopIcon("messagebox_warning" ) );
  lo1->addWidget( icon );
  lo1->addWidget( new QLabel( reason + "\n\n" + i18n("What do you want to do?"), w ) );

  // If the file isn't deleted, present a diff button, and a overwrite action.
  if ( modtype != 3 )
  {
    QHBoxLayout *lo2 = new QHBoxLayout( lo );
    QPushButton *btnDiff = new QPushButton( i18n("&View Difference..."), w );
    lo2->addStretch( 1 );
    lo2->addWidget( btnDiff );
    connect( btnDiff, SIGNAL(clicked()), this, SLOT(slotDiff()) );
    QWhatsThis::add( btnDiff, i18n(
        "Calculates the difference between the editor contents and the disk "
        "file using diff(1) and opens the diff file with the default application "
        "for that.") );

    setButtonText( User1, i18n("Overwrite") );
    setButtonWhatsThis( User1, i18n("Overwrite the disk file with the editor content.") );
  }
  else
    showButton( User1, false );
}

KateModOnHdPrompt::~KateModOnHdPrompt()
{
}

void KateModOnHdPrompt::slotDiff()
{
  // Start a KProcess that creates a diff
  KProcIO *p = new KProcIO();
  p->setComm( KProcess::All );
  *p << "diff" << "-ub" << "-" <<  m_doc->url().path();
  connect( p, SIGNAL(processExited(KProcess*)), this, SLOT(slotPDone(KProcess*)) );
  connect( p, SIGNAL(readReady(KProcIO*)), this, SLOT(slotPRead(KProcIO*)) );

  setCursor( WaitCursor );

  p->start( KProcess::NotifyOnExit, true );

  uint lastln =  m_doc->numLines();
  for ( uint l = 0; l <  lastln; l++ )
    p->writeStdin(  m_doc->textLine( l ), l < lastln );

  p->closeWhenDone();
}

void KateModOnHdPrompt::slotPRead( KProcIO *p)
{
  // create a file for the diff if we haven't one allready
  if ( ! m_tmpfile )
    m_tmpfile = new KTempFile();
  // put all the data we have in it
  QString stmp;
  while ( p->readln( stmp, false ) > -1 )
    *m_tmpfile->textStream() << stmp << endl;

  p->ackRead();
}

void KateModOnHdPrompt::slotPDone( KProcess *p )
{
  setCursor( ArrowCursor );
  m_tmpfile->close();

  if ( ! p->normalExit() /*|| p->exitStatus()*/ )
  {
    KMessageBox::sorry( this,
                        i18n("The diff command failed. Please make sure that"
                             "diff(1) is installed and in your PATH."),
                        i18n("Error creating diff") );
    return;
  }

  KRun::runURL( m_tmpfile->name(), "text/x-diff", true );
  delete m_tmpfile;
  m_tmpfile = 0;
}

void KateModOnHdPrompt::slotApply()
{
  if ( KMessageBox::warningContinueCancel(
       this,
       i18n("Ignoring means that you will not be warned again (unless "
            "the disk file changes once more): if you save the document, you "
            "will overwrite the file on disk; if you do not save then the disk file "
            "(if present) is what you have."),
       i18n("You are on your own"),
       KStdGuiItem::cont(),
       "kate_ignore_modonhd" ) != KMessageBox::Continue )
    return;

  done(Ignore);
}

void KateModOnHdPrompt::slotOk()
{
  done( m_modtype == 3 ? Save : Reload );
}

void KateModOnHdPrompt::slotUser1()
{
  done( Overwrite );
}

//END KateModOnHdPrompt

// kate: space-indent on; indent-width 2; replace-tabs on;
