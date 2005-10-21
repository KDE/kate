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
#include <kmenu.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kregexpeditorinterface.h>
#include <krun.h>
#include <kseparator.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kpushbutton.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qdom.h>
#include <qfile.h>
#include <qgroupbox.h>
#include <q3header.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <q3listbox.h>
#include <q3listview.h>
#include <qmap.h>
#include <qobject.h>
#include <qpainter.h>
#include <kpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qtextcodec.h>
#include <qtextstream.h>
#include <qtoolbutton.h>
#include <kvbox.h>


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
  kdDebug (13000) << "TEST: something changed on the config page: " << this << endl;
}
//END KateConfigPage

//BEGIN KateIndentConfigTab
const int KateIndentConfigTab::flags[] = {
    KateDocumentConfig::cfSpaceIndent,
    KateDocumentConfig::cfKeepIndentProfile,
    KateDocumentConfig::cfKeepExtraSpaces,
    KateDocumentConfig::cfTabIndents,
    KateDocumentConfig::cfBackspaceIndents,
    KateDocumentConfig::cfDoxygenAutoTyping,
    KateDocumentConfig::cfMixedIndent
};

KateIndentConfigTab::KateIndentConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing( KDialog::spacingHint() );
  int configFlags = KateDocumentConfig::global()->configFlags();

  QGroupBox *gbAuto = new QGroupBox(i18n("Automatic Indentation"), this);
  QVBoxLayout *vb = new QVBoxLayout (gbAuto);

  QHBoxLayout *indentLayout = new QHBoxLayout();
  vb->addItem(indentLayout);
  indentLayout->setSpacing(KDialog::spacingHint());
  QLabel *indentLabel = new QLabel(i18n("Default &Indentation mode:"),gbAuto);
  indentLayout->addWidget(indentLabel);
  m_indentMode = new KComboBox (gbAuto);
  indentLayout->addWidget(m_indentMode);
  m_indentMode->addItems (KateAutoIndent::listModes());
  indentLabel->setBuddy(m_indentMode);
  m_configPage = new QPushButton(SmallIconSet("configure"), i18n("Configure..."), gbAuto);
  indentLayout->addWidget(m_configPage);

  opt[5] = new QCheckBox(i18n("Insert leading Doxygen \"*\" when typing"), gbAuto);
  vb->addWidget (opt[5]);

  QGroupBox *gbSpaces = new QGroupBox(i18n("Indentation with Spaces"), this);
  vb = new QVBoxLayout (gbSpaces);
  opt[0] = new QCheckBox(i18n("Use &spaces instead of tabs to indent"), gbSpaces );
  vb->addWidget (opt[0]);
  opt[6] = new QCheckBox(i18n("Emacs style mixed mode"), gbSpaces);
  vb->addWidget (opt[6]);

  indentationWidth = new KIntNumInput(KateDocumentConfig::global()->indentationWidth(),gbSpaces);
  indentationWidth->setRange(1, 16, 1, false);
  indentationWidth->setLabel(i18n("Number of spaces:"), Qt::AlignVCenter);
  vb->addWidget (indentationWidth);

  opt[1] = new QCheckBox(i18n("Keep indent &profile"), this);
  opt[2] = new QCheckBox(i18n("&Keep extra spaces"), this);

  QGroupBox *keys = new QGroupBox(i18n("Keys to Use"), this);
  vb = new QVBoxLayout (keys);
  opt[3] = new QCheckBox(i18n("&Tab key indents"), keys);
  vb->addWidget (opt[3]);
  opt[4] = new QCheckBox(i18n("&Backspace key indents"), keys);
  vb->addWidget (opt[4]);

  m_tabs = new QGroupBox(i18n("Tab Key Mode if Nothing Selected"), this );
  QVBoxLayout *tablayout=new QVBoxLayout(m_tabs);

  tablayout->addWidget( rb1=new QRadioButton( i18n("Insert indent &characters"), m_tabs ));
  tablayout->addWidget( rb2=new QRadioButton( i18n("I&nsert tab character"), m_tabs ));
  tablayout->addWidget( rb3=new QRadioButton( i18n("Indent current &line"), m_tabs ));

  opt[0]->setChecked(configFlags & flags[0]);
  opt[1]->setChecked(configFlags & flags[1]);
  opt[2]->setChecked(configFlags & flags[2]);
  opt[3]->setChecked(configFlags & flags[3]);
  opt[4]->setChecked(configFlags & flags[4]);
  opt[5]->setChecked(configFlags & flags[5]);
  opt[6]->setChecked(configFlags & flags[6]);

  layout->addWidget(gbAuto);
  layout->addWidget(gbSpaces);
  layout->addWidget(opt[1]);
  layout->addWidget(opt[2]);
  layout->addWidget(keys);
  layout->addWidget(m_tabs, 0);

  layout->addStretch();

  // What is this? help
  opt[0]->setWhatsThis( i18n(
        "Check this if you want to indent with spaces rather than tabs."));
  opt[2]->setWhatsThis( i18n(
        "Indentations of more than the selected number of spaces will not be "
        "shortened."));
  opt[3]->setWhatsThis( i18n(
        "This allows the <b>Tab</b> key to be used to increase the indentation "
        "level."));
  opt[4]->setWhatsThis( i18n(
        "This allows the <b>Backspace</b> key to be used to decrease the "
        "indentation level."));
  opt[5]->setWhatsThis( i18n(
        "Automatically inserts a leading \"*\" while typing within a Doxygen "
        "style comment."));
  opt[6]->setWhatsThis( i18n(
      "Use a mix of tab and space characters for indentation.") );
  indentationWidth->setWhatsThis( i18n("The number of spaces to indent with."));

  m_configPage->setWhatsThis( i18n(
        "If this button is enabled, additional indenter specific options are "
        "available and can be configured in an extra dialog.") );

  m_indentMode->setWhatsThis( i18n(
        "The specified indentation mode will be used for all new documents. Be aware "
        "that it is also possible to set the indentation mode with document variables, "
        "filetypes or a .kateconfig file." ) );

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
  connect( opt[6], SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );

  connect(indentationWidth, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  connect(m_configPage, SIGNAL(clicked()), this, SLOT(configPage()));
}

void KateIndentConfigTab::somethingToggled() {
  indentationWidth->setEnabled(opt[0]->isChecked());
  opt[6]->setEnabled(opt[0]->isChecked());
}

void KateIndentConfigTab::indenterSelected (int index)
{
  if (index == KateDocumentConfig::imCStyle || index == KateDocumentConfig::imCSAndS)
    opt[5]->setEnabled(true);
  else
    opt[5]->setEnabled(false);

  m_configPage->setEnabled( KateAutoIndent::hasConfigPage(index) );
}

void KateIndentConfigTab::configPage()
{
  uint index = m_indentMode->currentIndex();
  if ( KateAutoIndent::hasConfigPage(index) )
  {
    KDialogBase dlg(this, "indenter_config_dialog", true, i18n("Configure Indenter"),
      KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Cancel, true);

    KVBox *box = new KVBox(&dlg);
    box->setSpacing( KDialog::spacingHint() );
    dlg.setMainWidget(box);
    new QLabel("<qt><b>" + KateAutoIndent::modeDescription(index) + "</b></qt>", box);
    new KSeparator(box);

    IndenterConfigPage* page = KateAutoIndent::configPage(box, index);

    if (!page) return;
    box->setStretchFactor(page, 1);

    connect( &dlg, SIGNAL(okClicked()), page, SLOT(apply()) );

    dlg.resize(400, 300);
    dlg.exec();
  }
}

void KateIndentConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  KateDocumentConfig::global()->configStart ();

  int configFlags, z;

  configFlags = KateDocumentConfig::global()->configFlags();
  for (z = 0; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }

  KateDocumentConfig::global()->setConfigFlags(configFlags);
  KateDocumentConfig::global()->setIndentationWidth(indentationWidth->value());

  KateDocumentConfig::global()->setIndentationMode(m_indentMode->currentIndex());

  KateDocumentConfig::global()->setConfigFlags (KateDocumentConfig::cfTabIndentsMode, rb3->isChecked());
  KateDocumentConfig::global()->setConfigFlags (KateDocumentConfig::cfTabInsertsTab, rb2->isChecked());

  KateDocumentConfig::global()->configEnd ();
}

void KateIndentConfigTab::reload ()
{
  if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfTabIndentsMode)
    ((QRadioButton*)(m_tabs->layout()->itemAt(2)->widget()))->setChecked(true);
  else if (KateDocumentConfig::global()->configFlags() & KateDocumentConfig::cfTabInsertsTab)
    ((QRadioButton*)(m_tabs->layout()->itemAt(1)->widget()))->setChecked(true);
  else
    ((QRadioButton*)(m_tabs->layout()->itemAt(0)->widget()))->setChecked(true);

  m_indentMode->setCurrentIndex (KateDocumentConfig::global()->indentationMode());

  somethingToggled ();
  indenterSelected (m_indentMode->currentIndex());
}
//END KateIndentConfigTab

//BEGIN KateSelectConfigTab
const int KateSelectConfigTab::flags[] = {KateDocumentConfig::cfSmartHome, KateDocumentConfig::cfWrapCursor};

KateSelectConfigTab::KateSelectConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  int configFlags = KateDocumentConfig::global()->configFlags();

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(KDialog::spacingHint() );

  QGroupBox *gbCursor = new QGroupBox( i18n("Text Cursor Movement"), this);
  QVBoxLayout *layout1=new QVBoxLayout(gbCursor);
  opt[0] = new QCheckBox(i18n("Smart ho&me"), gbCursor);
  opt[0]->setChecked(configFlags & flags[3]);
  connect(opt[0], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  layout1->addWidget(opt[0]);

  opt[1] = new QCheckBox(i18n("Wrap c&ursor"), gbCursor);
  opt[1]->setChecked(configFlags & flags[4]);
  connect(opt[1], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  layout1->addWidget(opt[1]);
  
  e6 = new QCheckBox(i18n("&PageUp/PageDown moves cursor"), gbCursor);
  e6->setChecked(KateDocumentConfig::global()->pageUpDownMovesCursor());
  connect(e6, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  layout1->addWidget(e6);

  e4 = new KIntNumInput(KateViewConfig::global()->autoCenterLines(),gbCursor);
  e4->setRange(0, 1000000, 1, false);
  e4->setLabel(i18n("Autocenter cursor (lines):"), Qt::AlignVCenter);
  connect(e4, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  layout1->addWidget(e4);

  layout->addWidget(gbCursor);

  m_tabs = new QGroupBox(i18n("Selection Mode"), this );
  layout->addWidget (m_tabs);
  QVBoxLayout *tablayout=new QVBoxLayout(m_tabs);

  tablayout->addWidget( rb1=new QRadioButton( i18n("&Normal"), m_tabs ));
  tablayout->addWidget( rb2=new QRadioButton( i18n("&Persistent"), m_tabs ));

  layout->addStretch();

  rb1->setWhatsThis(i18n(
        "Selections will be overwritten by typed text and will be lost on "
        "cursor movement."));
  rb2->setWhatsThis(i18n(
        "Selections will stay even after cursor movement and typing."));

  e4->setWhatsThis(i18n(
        "Sets the number of lines to maintain visible above and below the "
        "cursor when possible."));

  opt[0]->setWhatsThis(i18n(
        "When selected, pressing the home key will cause the cursor to skip "
        "whitespace and go to the start of a line's text."));

    opt[1]->setWhatsThis(i18n(
        "When on, moving the insertion cursor using the <b>Left</b> and "
        "<b>Right</b> keys will go on to previous/next line at beginning/end of "
        "the line, similar to most editors.<p>When off, the insertion cursor "
        "cannot be moved left of the line start, but it can be moved off the "
        "line end, which can be very handy for programmers."));

  e6->setWhatsThis(i18n("Selects whether the PageUp and PageDown keys should alter the vertical position of the cursor relative to the top of the view."));


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
  if (!hasChanged())
    return;
  m_changed = false;

  KateViewConfig::global()->configStart ();
  KateDocumentConfig::global()->configStart ();

  int configFlags = KateDocumentConfig::global()->configFlags();
  for (int z = 1; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }
  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateViewConfig::global()->setAutoCenterLines(qMax(0, e4->value()));
  KateDocumentConfig::global()->setPageUpDownMovesCursor(e6->isChecked());

  KateViewConfig::global()->setPersistentSelection (rb2->isChecked());

  KateDocumentConfig::global()->configEnd ();
  KateViewConfig::global()->configEnd ();
}

void KateSelectConfigTab::reload ()
{
  if (KateViewConfig::global()->persistentSelection())
    ((QRadioButton*)(m_tabs->layout()->itemAt(0)->widget()))->setChecked(true);
  else
    ((QRadioButton*)(m_tabs->layout()->itemAt(1)->widget()))->setChecked(true);
}
//END KateSelectConfigTab

//BEGIN KateEditConfigTab
const int KateEditConfigTab::flags[] = {KateDocumentConfig::cfWordWrap,
  KateDocumentConfig::cfAutoBrackets, KateDocumentConfig::cfShowTabs,
  KateDocumentConfig::cfReplaceTabsDyn, KateDocumentConfig::cfRemoveTrailingDyn};

KateEditConfigTab::KateEditConfigTab(QWidget *parent)
  : KateConfigPage(parent)
{
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin( 0);
  mainLayout->setSpacing(KDialog::spacingHint() );
  int configFlags = KateDocumentConfig::global()->configFlags();

  QGroupBox *gbWhiteSpace = new QGroupBox(i18n("Tabulators"), this);
  QVBoxLayout *layout1=new QVBoxLayout(gbWhiteSpace);
  opt[3] = new QCheckBox( i18n("&Insert spaces instead of tabulators"), gbWhiteSpace );
  opt[3]->setChecked( configFlags & KateDocumentConfig::cfReplaceTabsDyn );
  connect( opt[3], SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );
  layout1->addWidget(opt[3]);

  opt[2] = new QCheckBox(i18n("&Show tabulators"), gbWhiteSpace);
  opt[2]->setChecked(configFlags & flags[2]);
  connect(opt[2], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  layout1->addWidget(opt[2]);

  e2 = new KIntNumInput(KateDocumentConfig::global()->tabWidth(),gbWhiteSpace);
  e2->setRange(1, 16, 1, false);
  e2->setLabel(i18n("Tab width:"), Qt::AlignVCenter);
  connect(e2, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  layout1->addWidget(e2);

  mainLayout->addWidget(gbWhiteSpace);

  QGroupBox *gbWordWrap = new QGroupBox(i18n("Static Word Wrap"), this);
  layout1=new QVBoxLayout(gbWordWrap);

  opt[0] = new QCheckBox(i18n("Enable static &word wrap"), gbWordWrap);
  opt[0]->setChecked(KateDocumentConfig::global()->wordWrap());
  connect(opt[0], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  layout1->addWidget(opt[0]);

  m_wwmarker = new QCheckBox( i18n("&Show static word wrap marker (if applicable)"), gbWordWrap );
  m_wwmarker->setChecked( KateRendererConfig::global()->wordWrapMarker() );
  connect(m_wwmarker, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  layout1->addWidget(m_wwmarker);


  e1 = new KIntNumInput(KateDocumentConfig::global()->wordWrapAt(),gbWordWrap);
  e1->setRange(20, 200, 1, false);
  e1->setLabel(i18n("Wrap words at:"), Qt::AlignVCenter);
  connect(e1, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  layout1->addWidget(e1);

  mainLayout->addWidget(gbWordWrap);

  opt[4] = new QCheckBox( i18n("Remove &trailing spaces"), this );
  mainLayout->addWidget( opt[4] );
  opt[4]->setChecked( configFlags & KateDocumentConfig::cfRemoveTrailingDyn );
  connect( opt[4], SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );

  opt[1] = new QCheckBox(i18n("Auto &brackets"), this);
  mainLayout->addWidget(opt[1]);
  opt[1]->setChecked(configFlags & flags[1]);
  connect(opt[1], SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  e3 = new KIntNumInput(e2, KateDocumentConfig::global()->undoSteps(),this);
  e3->setRange(0, 1000000, 1, false);
  e3->setSpecialValueText( i18n("Unlimited") );
  e3->setLabel(i18n("Maximum undo steps:"), Qt::AlignVCenter);
  mainLayout->addWidget(e3);
  connect(e3, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

  QHBoxLayout *e5Layout = new QHBoxLayout();
  mainLayout->addItem(e5Layout);
  QLabel *e5Label = new QLabel(i18n("Smart search t&ext from:"), this);
  e5Layout->addWidget(e5Label);
  e5 = new KComboBox (this);
  e5->addItem( i18n("Nowhere") );
  e5->addItem( i18n("Selection Only") );
  e5->addItem( i18n("Selection, then Current Word") );
  e5->addItem( i18n("Current Word Only") );
  e5->addItem( i18n("Current Word, then Selection") );
  e5->setCurrentIndex(KateViewConfig::global()->textToSearchMode());
  e5Layout->addWidget(e5);
  e5Label->setBuddy(e5);
  connect(e5, SIGNAL(activated(int)), this, SLOT(slotChanged()));

  mainLayout->addStretch();

  // What is this? help
  opt[0]->setWhatsThis(i18n(
        "Automatically start a new line of text when the current line exceeds "
        "the length specified by the <b>Wrap words at:</b> option."
        "<p>This option does not wrap existing lines of text - use the <b>Apply "
        "Static Word Wrap</b> option in the <b>Tools</b> menu for that purpose."
        "<p>If you want lines to be <i>visually wrapped</i> instead, according "
        "to the width of the view, enable <b>Dynamic Word Wrap</b> in the "
        "<b>View Defaults</b> config page."));
  e1->setWhatsThis(i18n(
        "If the Word Wrap option is selected this entry determines the length "
        "(in characters) at which the editor will automatically start a new line."));
  opt[1]->setWhatsThis(i18n(
        "When the user types a left bracket ([,(, or {) KateView automatically "
        "enters the right bracket (}, ), or ]) to the right of the cursor."));
  opt[2]->setWhatsThis(i18n(
        "The editor will display a symbol to indicate the presence of a tab in "
        "the text."));

  e3->setWhatsThis(i18n(
        "Sets the number of undo/redo steps to record. More steps uses more memory."));

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
  e5Label->setWhatsThis(gstfwt);
  e5->setWhatsThis(gstfwt);
  opt[3]->setWhatsThis(i18n(
      "If this is enabled, the editor will calculate the number of spaces up to "
      "the next tab position as defined by the tab width, and insert that number "
      "of spaces instead of a TAB character." ) );
  opt[4]->setWhatsThis(i18n(
      "If this is enabled, the editor will remove any trailing whitespace on "
      "lines when they are left by the insertion cursor.") );
  m_wwmarker->setWhatsThis(i18n(
        "<p>If this option is checked, a vertical line will be drawn at the word "
        "wrap column as defined in the <strong>Editing</strong> properties."
        "<p>Note that the word wrap marker is only drawn if you use a fixed "
        "pitch font." ));
}

void KateEditConfigTab::apply ()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

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

  KateViewConfig::global()->setTextToSearchMode(e5->currentIndex());

  KateRendererConfig::global()->setWordWrapMarker (m_wwmarker->isChecked());

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

  QVBoxLayout *blay=new QVBoxLayout(this);
  blay->setMargin(0);
  blay->setSpacing(KDialog::spacingHint());

  QGroupBox *gbWordWrap = new QGroupBox(i18n("Word Wrap"), this);
  QVBoxLayout *layout=new QVBoxLayout(gbWordWrap);
  m_dynwrap=new QCheckBox(i18n("&Dynamic word wrap"),gbWordWrap);
  layout->addWidget(m_dynwrap);

  QHBoxLayout *sublayout=new QHBoxLayout();
  m_dynwrapIndicatorsLabel = new QLabel( i18n("Dynamic word wrap indicators (if applicable):"),this);
  m_dynwrapIndicatorsCombo = new KComboBox( this);
  m_dynwrapIndicatorsCombo->addItem( i18n("Off") );
  m_dynwrapIndicatorsCombo->addItem( i18n("Follow Line Numbers") );
  m_dynwrapIndicatorsCombo->addItem( i18n("Always On") );
  m_dynwrapIndicatorsLabel->setBuddy(m_dynwrapIndicatorsCombo);
  layout->addItem(sublayout);
  sublayout->addWidget(m_dynwrapIndicatorsLabel);
  sublayout->addWidget(m_dynwrapIndicatorsCombo);

  m_dynwrapAlignLevel = new KIntNumInput(gbWordWrap);
  m_dynwrapAlignLevel->setLabel(i18n("Vertically align dynamically wrapped lines to indentation depth:"));
  m_dynwrapAlignLevel->setRange(0, 80, 10);
  // xgettext:no-c-format
  m_dynwrapAlignLevel->setSuffix(i18n("% of View Width"));
  m_dynwrapAlignLevel->setSpecialValueText(i18n("Disabled"));
  layout->addWidget(m_dynwrapAlignLevel);
  blay->addWidget(gbWordWrap);

  QGroupBox *gbFold = new QGroupBox(i18n("Code Folding"), this);
  QVBoxLayout *gbFoldLayout=new QVBoxLayout(gbFold);
  gbFoldLayout->addWidget(m_folding=new QCheckBox(i18n("Show &folding markers (if available)"), gbFold ));
  gbFoldLayout->addWidget(m_collapseTopLevel = new QCheckBox( i18n("Collapse toplevel folding nodes"), gbFold ));
  m_collapseTopLevel->hide ();

  blay->addWidget(gbFold);

  QGroupBox *gbBar = new QGroupBox( i18n("Borders"), this);
  QVBoxLayout *gbBarLayout=new QVBoxLayout(gbBar);
  gbBarLayout->addWidget(m_icons=new QCheckBox(i18n("Show &icon border"),gbBar));
  gbBarLayout->addWidget(m_line=new QCheckBox(i18n("Show &line numbers"),gbBar));
  gbBarLayout->addWidget(m_scrollBarMarks=new QCheckBox(i18n("Show &scrollbar marks"),gbBar));

  blay->addWidget(gbBar);

  m_bmSort = new QGroupBox(i18n("Sort Bookmarks Menu"), this );
  QVBoxLayout *bmSortLayout=new QVBoxLayout(m_bmSort);

  bmSortLayout->addWidget( rb1=new QRadioButton( i18n("By &position"), m_bmSort ));
  bmSortLayout->addWidget( rb2=new QRadioButton( i18n("By c&reation"), m_bmSort ));

  blay->addWidget(m_bmSort, 0 );

  m_showIndentLines = new QCheckBox(i18n("Show indentation lines"), this);
  m_showIndentLines->setChecked(KateRendererConfig::global()->showIndentationLines());
  blay->addWidget(m_showIndentLines);

  blay->addStretch(1000);

  m_dynwrap->setWhatsThis(i18n(
        "If this option is checked, the text lines will be wrapped at the view "
        "border on the screen."));
  QString wtstr = i18n("Choose when the Dynamic Word Wrap Indicators should be displayed");
  m_dynwrapIndicatorsLabel->setWhatsThis(wtstr);
  m_dynwrapIndicatorsCombo->setWhatsThis(wtstr);
  // xgettext:no-c-format
  m_dynwrapAlignLevel->setWhatsThis(i18n(
        "<p>Enables the start of dynamically wrapped lines to be aligned "
        "vertically to the indentation level of the first line.  This can help "
        "to make code and markup more readable.</p><p>Additionally, this allows "
        "you to set a maximum width of the screen, as a percentage, after which "
        "dynamically wrapped lines will no longer be vertically aligned.  For "
        "example, at 50%, lines whose indentation levels are deeper than 50% of "
        "the width of the screen will not have vertical alignment applied to "
        "subsequent wrapped lines.</p>"));
  m_line->setWhatsThis(i18n(
        "If this option is checked, every new view will display line numbers "
        "on the left hand side."));
  m_icons->setWhatsThis(i18n(
        "If this option is checked, every new view will display an icon border "
        "on the left hand side.<br><br>The icon border shows bookmark signs, "
        "for instance."));
  m_scrollBarMarks->setWhatsThis(i18n(
        "If this option is checked, every new view will show marks on the "
        "vertical scrollbar.<br><br>These marks will, for instance, show "
        "bookmarks."));
  m_folding->setWhatsThis(i18n(
        "If this option is checked, every new view will display marks for code "
        "folding, if code folding is available."));
  m_bmSort->setWhatsThis(i18n(
        "Choose how the bookmarks should be ordered in the <b>Bookmarks</b> menu."));
  rb1->setWhatsThis(i18n(
        "The bookmarks will be ordered by the line numbers they are placed at."));
  rb2->setWhatsThis(i18n(
        "Each new bookmark will be added to the bottom, independently from "
        "where it is placed in the document."));
  m_showIndentLines->setWhatsThis(i18n(
        "If this is enabled, the editor will display vertical line to help "
        "identifying indent lines.") );

  reload();

  //
  // after initial reload, connect the stuff for the changed () signal
  //

  connect(m_dynwrap, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_dynwrapIndicatorsCombo, SIGNAL(activated(int)), this, SLOT(slotChanged()));
  connect(m_dynwrapAlignLevel, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect(m_icons, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_scrollBarMarks, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_line, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_folding, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_collapseTopLevel, SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );
  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_showIndentLines, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
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

  KateViewConfig::global()->setDynWordWrap (m_dynwrap->isChecked());
  KateViewConfig::global()->setDynWordWrapIndicators (m_dynwrapIndicatorsCombo->currentIndex ());
  KateViewConfig::global()->setDynWordWrapAlignIndent(m_dynwrapAlignLevel->value());
  KateViewConfig::global()->setLineNumbers (m_line->isChecked());
  KateViewConfig::global()->setIconBar (m_icons->isChecked());
  KateViewConfig::global()->setScrollBarMarks (m_scrollBarMarks->isChecked());
  KateViewConfig::global()->setFoldingBar (m_folding->isChecked());

  KateViewConfig::global()->setBookmarkSort (
((QRadioButton*)(m_bmSort->layout()->itemAt(0)->widget()))->isChecked()?0:1);
  KateRendererConfig::global()->setShowIndentationLines(m_showIndentLines->isChecked());

  KateRendererConfig::global()->configEnd ();
  KateViewConfig::global()->configEnd ();
}

void KateViewDefaultsConfig::reload ()
{
  m_dynwrap->setChecked(KateViewConfig::global()->dynWordWrap());
  m_dynwrapIndicatorsCombo->setCurrentIndex( KateViewConfig::global()->dynWordWrapIndicators() );
  m_dynwrapAlignLevel->setValue(KateViewConfig::global()->dynWordWrapAlignIndent());
  m_line->setChecked(KateViewConfig::global()->lineNumbers());
  m_icons->setChecked(KateViewConfig::global()->iconBar());
  m_scrollBarMarks->setChecked(KateViewConfig::global()->scrollBarMarks());
  m_folding->setChecked(KateViewConfig::global()->foldingBar());
  //m_bmSort->setButton( KateViewConfig::global()->bookmarkSort() );
  ((QRadioButton*)(m_bmSort->layout()->itemAt(KateViewConfig::global()->bookmarkSort())->widget()))->setChecked(true);
  m_showIndentLines->setChecked(KateRendererConfig::global()->showIndentationLines());
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
#warning fixme, to work without a document object, perhaps create some own internally
  return ;

  if (!m_ready)
  {
    QVBoxLayout *l=new QVBoxLayout(this);
    KateView* view = (KateView*)m_doc->views().at(0);
    m_ac = view->editActionCollection();
    l->addWidget(m_keyChooser = new KKeyChooser( m_ac, this, false ));
    connect( m_keyChooser, SIGNAL( keyChange() ), this, SLOT( slotChanged() ) );
    m_keyChooser->show ();

    m_ready = true;
  }

  QWidget::show ();
}

void KateEditKeyConfiguration::apply()
{
#warning fixme, to work without a document object, perhaps create some own internally
  return ;

  if ( ! hasChanged() )
    return;
  m_changed = false;

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
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin( 0);
  layout->setSpacing( KDialog::spacingHint() );

  QGroupBox *gbEnc = new QGroupBox(i18n("File Format"), this);
  layout->addWidget( gbEnc );

  QVBoxLayout *gbEncLayout=new QVBoxLayout(gbEnc);

  QHBoxLayout *e5Layout = new QHBoxLayout();
  QLabel *e5Label = new QLabel(i18n("&Encoding:"), gbEnc);
  m_encoding = new KComboBox (gbEnc);
  e5Label->setBuddy(m_encoding);
  e5Layout->addWidget(e5Label);
  e5Layout->addWidget(m_encoding);
  gbEncLayout->addLayout(e5Layout);

  e5Layout = new QHBoxLayout();
  e5Label = new QLabel(i18n("End &of line:"), gbEnc);
  m_eol = new KComboBox (gbEnc);
  e5Label->setBuddy(m_eol);
  e5Layout->addWidget(e5Label);
  e5Layout->addWidget(m_eol);
  gbEncLayout->addLayout(e5Layout);

  allowEolDetection = new QCheckBox(i18n("&Automatic end of line detection"), gbEnc);
  gbEncLayout->addWidget(allowEolDetection);

  m_eol->addItem (i18n("UNIX"));
  m_eol->addItem (i18n("DOS/Windows"));
  m_eol->addItem (i18n("Macintosh"));

  QGroupBox *gbMem = new QGroupBox(i18n("Memory Usage"), this);
  layout->addWidget( gbMem );

  e5Layout = new QHBoxLayout(gbMem);
  e5Layout->setSpacing (32);
  blockCountLabel = new QLabel(i18n("Maximum loaded &blocks per file:"), gbMem);
  blockCount = new QSpinBox (gbMem);
  blockCount->setRange(4, 512);
  blockCount->setSingleStep( 4);
  blockCount->setValue (KateBuffer::maxLoadedBlocks());
  blockCountLabel->setBuddy(blockCount);

  e5Layout->addWidget(blockCountLabel);
  e5Layout->addWidget(blockCount);

  QGroupBox *gbWhiteSpace = new QGroupBox( i18n("Automatic Cleanups on Load/Save"), this);
  layout->addWidget( gbWhiteSpace );

  removeSpaces = new QCheckBox(i18n("Re&move trailing spaces"), gbWhiteSpace);
  (new QVBoxLayout(gbWhiteSpace))->addWidget(removeSpaces);
  removeSpaces->setChecked(configFlags & KateDocumentConfig::cfRemoveSpaces);

  QGroupBox *dirConfigBox = new QGroupBox(i18n("Folder Config File"), this);
  layout->addWidget( dirConfigBox );
  QVBoxLayout *dirConfigBoxLayout=new QVBoxLayout(dirConfigBox);
  dirSearchDepth = new KIntNumInput(KateDocumentConfig::global()->searchDirConfigDepth(),dirConfigBox);
  dirSearchDepth->setRange(-1, 64, 1, false);
  dirSearchDepth->setSpecialValueText( i18n("Do not use config file") );
  dirSearchDepth->setLabel(i18n("Se&arch depth for config file:"), Qt::AlignVCenter);
  dirConfigBoxLayout->addWidget(dirSearchDepth);

  QGroupBox *gb = new QGroupBox(i18n("Backup on Save"), this );
  layout->addWidget( gb );
  QVBoxLayout *gbLayout=new QVBoxLayout(gb);
  cbLocalFiles = new QCheckBox( i18n("&Local files"), gb );
  cbRemoteFiles = new QCheckBox( i18n("&Remote files"), gb );
  gbLayout->addWidget(cbLocalFiles);
  gbLayout->addWidget(cbRemoteFiles);

  QHBoxLayout *hbBuPrefix = new QHBoxLayout();
  QLabel *lBuPrefix = new QLabel( i18n("&Prefix:"), gb );
  leBuPrefix = new QLineEdit( gb);
  lBuPrefix->setBuddy( leBuPrefix );
  hbBuPrefix->addWidget(lBuPrefix);
  hbBuPrefix->addWidget(leBuPrefix);
  gbLayout->addLayout(hbBuPrefix);

  QHBoxLayout *hbBuSuffix = new QHBoxLayout();
  QLabel *lBuSuffix = new QLabel( i18n("&Suffix:"), gb );
  leBuSuffix = new QLineEdit( gb );
  lBuSuffix->setBuddy( leBuSuffix );
  hbBuSuffix->addWidget(lBuSuffix);
  hbBuSuffix->addWidget(leBuSuffix);
  gbLayout->addLayout(hbBuSuffix);

  layout->addStretch();

  removeSpaces->setWhatsThis(i18n(
        "The editor will automatically eliminate extra spaces at the ends of "
        "lines of text while loading/saving the file."));
  gb->setWhatsThis(i18n(
        "<p>Backing up on save will cause Kate to copy the disk file to "
        "'&lt;prefix&gt;&lt;filename&gt;&lt;suffix&gt;' before saving changes."
        "<p>The suffix defaults to <strong>~</strong> and prefix is empty by default" ) );
  allowEolDetection->setWhatsThis(i18n(
        "Check this if you want the editor to autodetect the end of line type."
        "The first found end of line type will be used for the whole file.") );
  cbLocalFiles->setWhatsThis(i18n(
        "Check this if you want backups of local files when saving") );
  cbRemoteFiles->setWhatsThis(i18n(
        "Check this if you want backups of remote files when saving") );
  leBuPrefix->setWhatsThis(i18n(
        "Enter the prefix to prepend to the backup file names" ) );
  leBuSuffix->setWhatsThis(i18n(
        "Enter the suffix to add to the backup file names" ) );
  dirSearchDepth->setWhatsThis(i18n(
        "The editor will search the given number of folder levels upwards for .kateconfig file"
        " and load the settings line from it." ));
  blockCount->setWhatsThis(i18n(
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
  connect( allowEolDetection, SIGNAL( toggled(bool) ), this, SLOT( slotChanged() ) );
  connect(blockCount, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
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
  if (!hasChanged())
    return;
  m_changed = false;

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

  configFlags &= ~KateDocumentConfig::cfRemoveSpaces; // clear flag
  if (removeSpaces->isChecked()) configFlags |= KateDocumentConfig::cfRemoveSpaces; // set flag if checked

  KateDocumentConfig::global()->setConfigFlags(configFlags);

  KateDocumentConfig::global()->setEncoding((m_encoding->currentIndex() == 0) ? "" : KGlobal::charsets()->encodingForName(m_encoding->currentText()));

  KateDocumentConfig::global()->setEol(m_eol->currentIndex());
  KateDocumentConfig::global()->setAllowEolDetection(allowEolDetection->isChecked());

  KateDocumentConfig::global()->configEnd ();
}

void KateSaveConfigTab::reload()
{
  // encoding
  m_encoding->clear ();
  m_encoding->addItem (i18n("KDE Default"));
  m_encoding->setCurrentIndex(0);
  QStringList encodings (KGlobal::charsets()->descriptiveEncodingNames());
  int insert = 1;
  for (int i=0; i < encodings.count(); i++)
  {
    bool found = false;
    QTextCodec *codecForEnc = KGlobal::charsets()->codecForName(KGlobal::charsets()->encodingForName(encodings[i]), found);

    if (found)
    {
      m_encoding->addItem (encodings[i]);

      if ( codecForEnc->name() == KateDocumentConfig::global()->encoding() )
      {
        m_encoding->setCurrentIndex(insert);
      }

      insert++;
    }
  }

  // eol
  m_eol->setCurrentIndex(KateDocumentConfig::global()->eol());
  allowEolDetection->setChecked(KateDocumentConfig::global()->allowEolDetection());

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
class KatePartPluginListItem : public Q3CheckListItem
{
  public:
    KatePartPluginListItem(bool checked, uint i, const QString &name, Q3ListView *parent);
    uint pluginIndex () const { return index; }

  protected:
    void stateChange(bool);

  private:
    uint index;
    bool silentStateChange;
};

KatePartPluginListItem::KatePartPluginListItem(bool checked, uint i, const QString &name, Q3ListView *parent)
  : Q3CheckListItem(parent, name, CheckBox)
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
KatePartPluginListView::KatePartPluginListView(QWidget *parent)
  : KListView(parent)
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
  QGridLayout *grid = new QGridLayout( this); //, 1, 1 );
  grid->setSpacing( KDialogBase::spacingHint() );

  listView = new KatePartPluginListView(this);
  listView->addColumn(i18n("Name"));
  listView->addColumn(i18n("Comment"));

  grid->addWidget( listView, 0, 0);

  for (int i=0; i<KateGlobal::self()->plugins().count(); i++)
  {
    KatePartPluginListItem *item = new KatePartPluginListItem(KateDocumentConfig::global()->plugin(i), i, (KateGlobal::self()->plugins())[i]->name(), listView);
    item->setText(0, (KateGlobal::self()->plugins())[i]->name());
    item->setText(1, (KateGlobal::self()->plugins())[i]->comment());

    m_items.append (item);
  }

  // configure button

  btnConfigure = new QPushButton( i18n("Configure..."), this );
  btnConfigure->setEnabled( false );
  grid->addWidget( btnConfigure, 1, 0, Qt::AlignRight );
  connect( btnConfigure, SIGNAL(clicked()), this, SLOT(slotConfigure()) );

  connect( listView, SIGNAL(selectionChanged(Q3ListViewItem*)), this, SLOT(slotCurrentChanged(Q3ListViewItem*)) );
  connect( listView, SIGNAL(stateChange(KatePartPluginListItem *, bool)),
    this, SLOT(slotStateChanged(KatePartPluginListItem *, bool)));
  connect(listView, SIGNAL(stateChange(KatePartPluginListItem *, bool)), this, SLOT(slotChanged()));
}

KatePartPluginConfigPage::~KatePartPluginConfigPage ()
{
}

void KatePartPluginConfigPage::apply ()
{
  kdDebug()<<"KatePartPluginConfigPage::apply (entered)"<<endl;
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  kdDebug()<<"KatePartPluginConfigPage::apply (need to store configuration)"<<endl;

  KateDocumentConfig::global()->configStart ();

  for (uint i=0; i < m_items.count(); i++)
    KateDocumentConfig::global()->setPlugin (m_items.at(i)->pluginIndex(), m_items.at(i)->isOn());

  KateDocumentConfig::global()->configEnd ();
}

void KatePartPluginConfigPage::slotStateChanged( KatePartPluginListItem *item, bool b )
{
  if ( b )
    slotCurrentChanged( (Q3ListViewItem*)item );
}

void KatePartPluginConfigPage::slotCurrentChanged( Q3ListViewItem* i )
{
  KatePartPluginListItem *item = static_cast<KatePartPluginListItem *>(i);
  if ( ! item ) return;

    bool b = false;
  if ( item->isOn() )
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

//BEGIN KateHlConfigPage
KateHlConfigPage::KateHlConfigPage (QWidget *parent)
 : KateConfigPage (parent, "")
 , m_currentHlData (-1)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin( 0);
  layout->setSpacing( KDialog::spacingHint() );

  // hl chooser
  QHBoxLayout *hbl=new QHBoxLayout();
  layout->addItem(hbl);

  hbl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("H&ighlight:"), this);
  hbl->addWidget(lHl);
  hlCombo = new QComboBox(this);
  hlCombo->setEditable(false);
  hbl->addWidget(hlCombo);
  lHl->setBuddy( hlCombo );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );

  for( int i = 0; i < KateHlManager::self()->highlights(); i++) {
    if (KateHlManager::self()->hlSection(i).length() > 0)
      hlCombo->addItem(KateHlManager::self()->hlSection(i) + QString ("/") + KateHlManager::self()->hlNameTranslated(i));
    else
      hlCombo->addItem(KateHlManager::self()->hlNameTranslated(i));
  }
  hlCombo->setCurrentIndex(0);

  QGroupBox *gbInfo = new QGroupBox(i18n("Information"), this );
  layout->addWidget (gbInfo);
  QVBoxLayout *subLayout=new QVBoxLayout(gbInfo);
  // author
  hbl = new QHBoxLayout();
  subLayout->addItem(hbl);
  hbl->addWidget(new QLabel( i18n("Author:"), gbInfo ));
  hbl->addWidget(author  = new QLabel (gbInfo));
  author->setTextFormat (Qt::RichText);

  // license
  QHBoxLayout *hb2 = new QHBoxLayout();
  subLayout->addItem(hb2);
  hb2->addWidget(new QLabel( i18n("License:"), gbInfo));
  hb2->addWidget(license  = new QLabel (gbInfo));

  QGroupBox *gbProps = new QGroupBox(i18n("Properties"), this );
  layout->addWidget (gbProps);
 
  QGridLayout *gl=new QGridLayout(gbProps);

  // file & mime types
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), gbProps);
  gl->addWidget(lFileExts,0,0);
  gl->addWidget(wildcards  = new QLineEdit(gbProps ),0,1);
  lFileExts->setBuddy( wildcards );

  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), gbProps);
  gl->addWidget(lMimeTypes,1,0);
  QHBoxLayout *hbx=new QHBoxLayout();
  gl->addItem(hbx,1,1);
  hbx->addWidget(mimetypes = new QLineEdit( gbProps));
  lMimeTypes->setBuddy( mimetypes );

  QToolButton *btnMTW = new QToolButton(gbProps);
  hbx->addWidget(btnMTW);
  btnMTW->setIcon(QIcon(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));

  QLabel *lprio = new QLabel( i18n("Prio&rity:"), gbProps);
  gl->addWidget(lprio,2,0);
  priority = new KIntNumInput( gbProps);
  gl->addWidget(priority,2,1);
  lprio->setBuddy( priority );


  // download/new buttons
  QHBoxLayout *hbBtns = new QHBoxLayout();
  layout->addItem (hbBtns);

  hbBtns->addStretch(1); // hmm.
  hbBtns->setSpacing( KDialog::spacingHint() );
  QPushButton *btnDl = new QPushButton(i18n("Do&wnload..."), this);
  hbBtns->addWidget(btnDl);
  connect( btnDl, SIGNAL(clicked()), this, SLOT(hlDownload()) );

  hlCombo->setCurrentIndex( 0 );
  hlChanged(0);

  hlCombo->setWhatsThis(i18n(
        "Choose a <em>Syntax Highlight mode</em> from this list to view its "
        "properties below.") );
  wildcards->setWhatsThis(i18n(
        "The list of file extensions used to determine which files to highlight "
        "using the current syntax highlight mode.") );
  mimetypes->setWhatsThis(i18n(
        "The list of Mime Types used to determine which files to highlight "
        "using the current highlight mode.<p>Click the wizard button on the "
        "left of the entry field to display the MimeType selection dialog.") );
  btnMTW->setWhatsThis(i18n(
        "Display a dialog with a list of all available mime types to choose from."
        "<p>The <strong>File Extensions</strong> entry will automatically be "
        "edited as well.") );
  btnDl->setWhatsThis(i18n(
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
  if (!hasChanged())
    return;
  m_changed = false;

  writeback();

  for(QHash<int,KateHlData>::const_iterator it=hlDataDict.constBegin();it!=hlDataDict.constEnd();++it)
    KateHlManager::self()->getHl( it.key() )->setData( it.value() );

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
    m_currentHlData = -1;
    return;
  }

  if ( !hlDataDict.contains( z ) )
    hlDataDict.insert( z, hl->getData() );

  m_currentHlData = z;
  const KateHlData& hlData=hlDataDict[ z ];
  wildcards->setText(hlData.wildcards);
  mimetypes->setText(hlData.mimetypes);
  priority->setValue(hlData.priority);

  // split author string if needed into multiple lines !
  //QStringList l= QStringList::split (QRegExp("[,;]"), hl->author());
  QStringList l= hl->author().split (QRegExp("[,;]"));
  author->setText (l.join ("<br>"));

  license->setText (hl->license());
}

void KateHlConfigPage::writeback()
{
  if (m_currentHlData!=-1)
  {
    KateHlData &hlData=hlDataDict[m_currentHlData];    
    hlData.wildcards = wildcards->text();
    hlData.mimetypes = mimetypes->text();
    hlData.priority = priority->value();
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
  //QStringList list = QStringList::split( QRegExp("\\s*;\\s*"), mimetypes->text() );
  QStringList list = mimetypes->text().split( QRegExp("\\s*;\\s*") );
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
  KVBox* vbox = new KVBox(this);
  setMainWidget(vbox);
  vbox->setSpacing(spacingHint());
  new QLabel(i18n("Select the syntax highlighting files you want to update:"), vbox);
  list = new Q3ListView(vbox);
  list->addColumn("");
  list->addColumn(i18n("Name"));
  list->addColumn(i18n("Installed"));
  list->addColumn(i18n("Latest"));
  list->setSelectionMode(Q3ListView::Multi);
  list->setAllColumnsShowFocus(true);

  new QLabel(i18n("<b>Note:</b> New versions are selected automatically."), vbox);
  actionButton (User1)->setIcon(SmallIconSet("ok"));

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
        Q3ListViewItem* entry = new Q3ListViewItem(
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
  for (Q3ListViewItem *it=list->firstChild();it;it=it->nextSibling())
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

  QVBoxLayout *topLayout = new QVBoxLayout( page);
  topLayout->setMargin( 0);
  topLayout->setSpacing(spacingHint());
  e1 = new KIntNumInput(line,page);
  e1->setRange(1, max);
  e1->setEditFocus(true);

  QLabel *label = new QLabel(i18n("&Go to line:"), page );
  label->setBuddy(e1);
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
  QHBoxLayout *lo1 = new QHBoxLayout();
  lo->addItem(lo1);
  QLabel *icon = new QLabel( w );
  icon->setPixmap( DesktopIcon("messagebox_warning" ) );
  lo1->addWidget( icon );
  lo1->addWidget( new QLabel( reason + "\n\n" + i18n("What do you want to do?"), w ) );

  // If the file isn't deleted, present a diff button, and a overwrite action.
  if ( modtype != 3 )
  {
    QHBoxLayout *lo2 = new QHBoxLayout();
    lo->addItem(lo2);
    QPushButton *btnDiff = new QPushButton( i18n("&View Difference"), w );
    lo2->addStretch( 1 );
    lo2->addWidget( btnDiff );
    connect( btnDiff, SIGNAL(clicked()), this, SLOT(slotDiff()) );
    btnDiff->setWhatsThis(i18n(
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

  setCursor( Qt::WaitCursor );

  p->start( KProcess::NotifyOnExit, true );

  int lastln =  m_doc->lines();
  for ( int l = 0; l <  lastln; ++l )
    p->writeStdin(  m_doc->line( l ), l < lastln );

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
  setCursor( Qt::ArrowCursor );
  m_tmpfile->close();

  if ( ! p->normalExit() /*|| p->exitStatus()*/ )
  {
    KMessageBox::sorry( this,
                        i18n("The diff command failed. Please make sure that "
                             "diff(1) is installed and in your PATH."),
                        i18n("Error Creating Diff") );
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
       i18n("You Are on Your Own"),
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
