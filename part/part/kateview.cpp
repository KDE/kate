/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
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

//BEGIN includes
#include "kateview.h"
#include "kateview.moc"

#include "kateviewinternal.h"
#include "kateviewhelpers.h"
#include "katerenderer.h"
#include "katedocument.h"
#include "katedocumenthelpers.h"
#include "kateglobal.h"
#include "katehighlight.h"
#include "katedialogs.h"
#include "katetextline.h"
#include "katecodefoldinghelpers.h"
#include "kateschema.h"
#include "katebookmarks.h"
#include "katesearch.h"
#include "kateconfig.h"
#include "katefiletype.h"
#include "kateautoindent.h"
#include "katespell.h"
#include "katecompletionwidget.h"
#include "katesmartmanager.h"
#include "katesmartrange.h"
#include "katesearchbar.h"

#include <ktexteditor/plugin.h>
#include <ktexteditor/cursorfeedback.h>

#include <kparts/event.h>

#include <kio/netaccess.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kcursor.h>
#include <kicon.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kstandardaction.h>
#include <kxmlguifactory.h>
#include <kxmlguiclient.h>
#include <klibloader.h>
#include <kencodingfiledialog.h>
#include <ktemporaryfile.h>
#include <ksavefile.h>
#include <kstandardshortcut.h>
#include <kmenu.h>
#include <ktoggleaction.h>
#include <kselectaction.h>
#include <kactioncollection.h>

#include <qfont.h>
#include <qfileinfo.h>
#include <qstyle.h>
#include <qevent.h>
#include <QtGui/QLayout>
#include <qclipboard.h>
#include <qtextdocument.h>
#include <qtextstream.h>
#include <qmimedata.h>
#include <QTextCodec>
//END includes

static void blockFix(KTextEditor::Range& range)
{
  if (range.start().column() > range.end().column())
  {
    int tmp = range.start().column();
    range.start().setColumn(range.end().column());
    range.end().setColumn(tmp);
  }
}

KateView::KateView( KateDocument *doc, QWidget *parent )
    : KTextEditor::View( parent )
    , m_destructing(false)
    , m_completionWidget(0)
    , m_editActions (0)
    , m_doc( doc )
    , m_search( new KateSearch( this ) )
    , m_spell( new KateSpell( this ) )
    , m_bookmarks( new KateBookmarks( this ) )
    , m_cmdLine (0)
    , m_searchBar (0)
    , m_gotoBar (0)
    , m_cmdLineOn (false)
    , m_hasWrap( false )
    , m_startingUp (true)
    , m_updatingDocumentConfig (false)
    , m_selection(m_doc->smartManager()->newSmartRange(KTextEditor::Range::invalid(), 0L, KTextEditor::SmartRange::ExpandRight))
    , blockSelect (false)
    , m_imComposeEvent( false )
{
  KateGlobal::self()->registerView( this );

  m_config = new KateViewConfig (this);

  m_renderer = new KateRenderer(doc, this);

  m_viewInternal = new KateViewInternal( this, doc );

  // layouting ;)
  m_vBox = new QVBoxLayout (this);
  m_vBox->setMargin (0);
  m_vBox->setSpacing (0);

  QHBoxLayout *hbox = new QHBoxLayout ();
  m_vBox->addLayout (hbox, 100);
  hbox->setMargin (0);
  hbox->setSpacing (0);

  hbox->addWidget (m_viewInternal->m_leftBorder);
  hbox->addWidget (m_viewInternal);
  hbox->addWidget (m_viewInternal->m_lineScroll);

  hbox = new QHBoxLayout ();
  m_vBox->addLayout (hbox);
  hbox->setMargin (0);
  hbox->setSpacing (0);

  hbox->addWidget (m_viewInternal->m_columnScroll);
  hbox->addWidget (m_viewInternal->m_dummy);

  // this really is needed :)
  m_viewInternal->updateView ();

  setComponentData( KateGlobal::self()->componentData() );
  doc->addView( this );

  setFocusProxy( m_viewInternal );
  setFocusPolicy( Qt::StrongFocus );

  if (!doc->singleViewMode()) {
    setXMLFile( "katepartui.rc" );
  } else {
    if( doc->readOnly() )
      setXMLFile( "katepartreadonlyui.rc" );
    else
      setXMLFile( "katepartui.rc" );
  }

  setupConnections();
  setupActions();
  setupEditActions();
  setupCodeFolding();

  // enable the plugins of this view
  m_doc->enableAllPluginsGUI (this);

  // update the enabled state of the undo/redo actions...
  slotNewUndo();

  m_viewBar = new KateViewBar (this);
  m_vBox->addWidget(m_viewBar);

  // create the searchbar...
  m_searchBar = new KateSearchBar(m_viewBar);

  m_startingUp = false;
  updateConfig ();

  slotHlChanged();
  /*test texthint
  connect(this,SIGNAL(needTextHint(int, int, QString &)),
  this,SLOT(slotNeedTextHint(int, int, QString &)));
  enableTextHints(1000);
  test texthint*/
  setFocus();
}

KateView::~KateView()
{
  if (!m_destructing) {
    m_destructing=true;

    if (!m_doc->singleViewMode())
      m_doc->disableAllPluginsGUI (this);

    m_doc->removeView( this );
  }

  delete m_selection;
  m_selection = 0L;

  foreach (KTextEditor::SmartRange* range, m_externalHighlights)
    removeExternalHighlight(range);

  foreach (KTextEditor::SmartRange* range, m_internalHighlights)
    removeInternalHighlight(range);

  delete m_viewInternal;

  delete m_renderer;

  delete m_config;
  KateGlobal::self()->deregisterView (this);
}

void KateView::setupConnections()
{
  connect( m_doc, SIGNAL(undoChanged()),
           this, SLOT(slotNewUndo()) );
  connect( m_doc, SIGNAL(highlightingChanged(KTextEditor::Document *)),
           this, SLOT(slotHlChanged()) );
  connect( m_doc, SIGNAL(canceled(const QString&)),
           this, SLOT(slotSaveCanceled(const QString&)) );
  connect( m_viewInternal, SIGNAL(dropEventPass(QDropEvent*)),
           this,           SIGNAL(dropEventPass(QDropEvent*)) );

  if ( m_doc->browserView() )
  {
    connect( this, SIGNAL(dropEventPass(QDropEvent*)),
             this, SLOT(slotDropEventPass(QDropEvent*)) );
  }
}

void KateView::setupActions()
{
  KActionCollection *ac = this->actionCollection ();
  QAction *a;

  m_toggleWriteLock = 0;

  m_cut = a = ac->addAction(KStandardAction::Cut, this, SLOT(cut()));
  a->setWhatsThis(i18n("Cut the selected text and move it to the clipboard"));

  m_paste = a = ac->addAction(KStandardAction::PasteText, this, SLOT(paste()));
  a->setWhatsThis(i18n("Paste previously copied or cut clipboard contents"));

  m_copy = a = ac->addAction(KStandardAction::Copy, this, SLOT(copy()));
  a->setWhatsThis(i18n( "Use this command to copy the currently selected text to the system clipboard."));

  m_copyHTML = a = ac->addAction("edit_copy_html");
  m_copyHTML->setIcon(KIcon("edit-copy"));
  m_copyHTML->setText(i18n("Copy as &HTML"));
  connect(a, SIGNAL(triggered(bool)), SLOT(copyHTML()));
  a->setWhatsThis(i18n( "Use this command to copy the currently selected text as HTML to the system clipboard."));

  if (!m_doc->readOnly())
  {
    a = ac->addAction(KStandardAction::Save, m_doc, SLOT(documentSave()));
    a->setWhatsThis(i18n("Save the current document"));

    a = m_editUndo = ac->addAction(KStandardAction::Undo, m_doc, SLOT(undo()));
    a->setWhatsThis(i18n("Revert the most recent editing actions"));

    a = m_editRedo = ac->addAction(KStandardAction::Redo, m_doc, SLOT(redo()));
    a->setWhatsThis(i18n("Revert the most recent undo operation"));

    a = ac->addAction("tools_apply_wordwrap");
    a->setText(i18n("&Word Wrap Document"));
    a->setWhatsThis(i18n("Use this command to wrap all lines of the current document which are longer than the width of the"
    " current view, to fit into this view.<br><br> This is a static word wrap, meaning it is not updated"
    " when the view is resized."));
    connect(a, SIGNAL(triggered(bool)), SLOT(applyWordWrap()));

    // setup Tools menu
    a = ac->addAction("tools_indent");
    a->setIcon(KIcon("format-indent-more"));
    a->setText(i18n("&Indent"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_I));
    a->setWhatsThis(i18n("Use this to indent a selected block of text.<br><br>"
        "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
    connect(a, SIGNAL(triggered(bool)), SLOT(indent()));

    a = ac->addAction("tools_unindent");
    a->setIcon(KIcon("format-indent-less"));
    a->setText(i18n("&Unindent"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_I));
    a->setWhatsThis(i18n("Use this to unindent a selected block of text."));
    connect(a, SIGNAL(triggered(bool)), SLOT(unIndent()));

    a = ac->addAction("tools_cleanIndent");
    a->setText(i18n("&Clean Indentation"));
    a->setWhatsThis(i18n("Use this to clean the indentation of a selected block of text (only tabs/only spaces)<br><br>"
        "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
    connect(a, SIGNAL(triggered(bool)), SLOT(cleanIndent()));


    a = ac->addAction("tools_align");
    a->setText(i18n("&Align"));
    a->setWhatsThis(i18n("Use this to align the current line or block of text to its proper indent level."));
    connect(a, SIGNAL(triggered(bool)), SLOT(align()));

    a = ac->addAction("tools_comment");
    a->setText(i18n("C&omment"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_D));
    a->setWhatsThis(i18n("This command comments out the current line or a selected block of text.<BR><BR>"
        "The characters for single/multiple line comments are defined within the language's highlighting."));
    connect(a, SIGNAL(triggered(bool)), SLOT(comment()));

    a = ac->addAction("tools_uncomment");
    a->setText(i18n("Unco&mment"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_D));
    a->setWhatsThis(i18n("This command removes comments from the current line or a selected block of text.<BR><BR>"
    "The characters for single/multiple line comments are defined within the language's highlighting."));
    connect(a, SIGNAL(triggered(bool)), SLOT(uncomment()));

    a = m_toggleWriteLock = new KToggleAction(i18n("&Read Only Mode"), this);
    a->setWhatsThis( i18n("Lock/unlock the document for writing") );
    connect(a, SIGNAL(triggered(bool)), SLOT( toggleWriteLock() ));
    ac->addAction("tools_toggle_write_lock", a);

    a = ac->addAction("tools_uppercase");
    a->setText(i18n("Uppercase"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    a->setWhatsThis( i18n("Convert the selection to uppercase, or the character to the "
      "right of the cursor if no text is selected.") );
    connect(a, SIGNAL(triggered(bool)), SLOT(uppercase()));

    a = ac->addAction( "tools_lowercase" );
    a->setText( i18n("Lowercase") );
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_U));
    a->setWhatsThis( i18n("Convert the selection to lowercase, or the character to the "
      "right of the cursor if no text is selected.") );
    connect(a, SIGNAL(triggered(bool)), SLOT(lowercase()));

    a = ac->addAction( "tools_capitalize" );
    a->setText( i18n("Capitalize") );
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_U));
    a->setWhatsThis( i18n("Capitalize the selection, or the word under the "
      "cursor if no text is selected.") );
    connect(a, SIGNAL(triggered(bool)), SLOT(capitalize()));

    a = ac->addAction( "tools_join_lines" );
    a->setText( i18n("Join Lines") );
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
    connect(a, SIGNAL(triggered(bool)), SLOT( joinLines() ));

    a = ac->addAction( "tools_invoke_code_completion" );
    a->setText( i18n("Invoke Code Completion") );
    a->setWhatsThis(i18n("Manually invoke command completion, usually by using a shortcut bound to this action."));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Space));
    connect(a, SIGNAL(triggered(bool)), SLOT(userInvokedCompletion()));
  }
  else
  {
    m_cut->setEnabled (false);
    m_paste->setEnabled (false);
    m_editUndo = 0;
    m_editRedo = 0;
  }

  a = ac->addAction( KStandardAction::Print, m_doc, SLOT(print()) );
  a->setWhatsThis(i18n("Print the current document."));

  a = ac->addAction( "file_reload" );
  a->setIcon(KIcon("view-refresh"));
  a->setText(i18n("Reloa&d"));
  a->setShortcuts(KStandardShortcut::reload());
  a->setWhatsThis(i18n("Reload the current document from disk."));
  connect(a, SIGNAL(triggered(bool)), SLOT(reloadFile()));

  a = ac->addAction( KStandardAction::SaveAs, m_doc, SLOT(documentSaveAs()) );
  a->setWhatsThis(i18n("Save the current document to disk, with a name of your choice."));

  a = ac->addAction( KStandardAction::GotoLine, this, SLOT(gotoLine()) );
  a->setWhatsThis(i18n("This command opens a dialog and lets you choose a line that you want the cursor to move to."));

  a = ac->addAction("set_confdlg");
  a->setText(i18n("&Configure Editor..."));
  a->setWhatsThis(i18n("Configure various aspects of this editor."));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotConfigDialog()));

  KateViewHighlightAction *menu = new KateViewHighlightAction (i18n("&Highlighting"), this);
  ac->addAction("set_highlight", menu);
  menu->setWhatsThis(i18n("Here you can choose how the current document should be highlighted."));
  menu->updateMenu (m_doc);

  KateViewFileTypeAction *ftm = new KateViewFileTypeAction (i18n("&Filetype"), this);
  ac->addAction("set_filetype", ftm);
  ftm->updateMenu (m_doc);

  KateViewSchemaAction *schemaMenu = new KateViewSchemaAction (i18n("&Schema"), this);
  ac->addAction("view_schemas", schemaMenu);
  schemaMenu->updateMenu (this);

  // indentation menu
  KateViewIndentationAction *indentMenu = new KateViewIndentationAction(m_doc, i18n("&Indentation"), this);
  ac->addAction("tools_indentation", indentMenu);

  // html export
  a = ac->addAction("file_export_html");
  a->setText(i18n("E&xport as HTML..."));
  a->setWhatsThis(i18n("This command allows you to export the current document"
                      " with all highlighting information into a HTML document."));
  connect(a, SIGNAL(triggered(bool)), SLOT(exportAsHTML()));

  m_selectAll = a= ac->addAction( KStandardAction::SelectAll, this, SLOT(selectAll()) );
  a->setWhatsThis(i18n("Select the entire text of the current document."));

  m_deSelect = a= ac->addAction( KStandardAction::Deselect, this, SLOT(clearSelection()) );
  a->setWhatsThis(i18n("If you have selected something within the current document, this will no longer be selected."));

  a = ac->addAction("incFontSizes");
  a->setIcon(KIcon("zoom-in"));
  a->setText(i18n("Enlarge Font"));
  a->setWhatsThis(i18n("This increases the display font size."));
  connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotIncFontSizes()));

  a = ac->addAction("decFontSizes");
  a->setIcon(KIcon("zoom-out"));
  a->setText(i18n("Shrink Font"));
  a->setWhatsThis(i18n("This decreases the display font size."));
  connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotDecFontSizes()));

  a = m_toggleBlockSelection = new KToggleAction(i18n("Bl&ock Selection Mode"), this);
  ac->addAction("set_verticalSelect", a);
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
  a->setWhatsThis(i18n("This command allows switching between the normal (line based) selection mode and the block selection mode."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleBlockSelectionMode()));

  a = m_toggleInsert = new KToggleAction(i18n("Overwr&ite Mode"), this);
  ac->addAction("set_insert", a);
  a->setShortcut(QKeySequence(Qt::Key_Insert));
  a->setWhatsThis(i18n("Choose whether you want the text you type to be inserted or to overwrite existing text."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleInsert()));

  KToggleAction *toggleAction;
  a = m_toggleDynWrap = toggleAction = new KToggleAction(i18n("&Dynamic Word Wrap"), this);
  ac->addAction("view_dynamic_word_wrap", a);
  a->setShortcut(QKeySequence(Qt::Key_F10));
  a->setWhatsThis(i18n("If this option is checked, the text lines will be wrapped at the view border on the screen."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleDynWordWrap()));

  a = m_setDynWrapIndicators = new KSelectAction(i18n("Dynamic Word Wrap Indicators"), this);
  ac->addAction("dynamic_word_wrap_indicators", a);
  a->setWhatsThis(i18n("Choose when the Dynamic Word Wrap Indicators should be displayed"));

  connect(m_setDynWrapIndicators, SIGNAL(triggered(int)), this, SLOT(setDynWrapIndicators(int)));
  QStringList list2;
  list2.append(i18n("&Off"));
  list2.append(i18n("Follow &Line Numbers"));
  list2.append(i18n("&Always On"));
  m_setDynWrapIndicators->setItems(list2);

  a = toggleAction = m_toggleFoldingMarkers = new KToggleAction(i18n("Show Folding &Markers"), this);
  ac->addAction("view_folding_markers", a);
  a->setShortcut(QKeySequence(Qt::Key_F9));
  a->setWhatsThis(i18n("You can choose if the codefolding marks should be shown, if codefolding is possible."));
  toggleAction->setCheckedState(KGuiItem(i18n("Hide Folding &Markers")));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleFoldingMarkers()));

  a = m_toggleIconBar = toggleAction = new KToggleAction(i18n("Show &Icon Border"), this);
  ac->addAction("view_border", a);
  a->setShortcut(QKeySequence(Qt::Key_F6));
  a->setWhatsThis(i18n("Show/hide the icon border.<BR><BR> The icon border shows bookmark symbols, for instance."));
  toggleAction->setCheckedState(KGuiItem(i18n("Hide &Icon Border")));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleIconBorder()));

  a = toggleAction = m_toggleLineNumbers = new KToggleAction(i18n("Show &Line Numbers"), this);
  ac->addAction("view_line_numbers", a);
  a->setShortcut(QKeySequence(Qt::Key_F11));
  a->setWhatsThis(i18n("Show/hide the line numbers on the left hand side of the view."));
  toggleAction->setCheckedState(KGuiItem(i18n("Hide &Line Numbers")));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleLineNumbersOn()));

  a = m_toggleScrollBarMarks = toggleAction = new KToggleAction(i18n("Show Scroll&bar Marks"), this);
  ac->addAction("view_scrollbar_marks", a);
  a->setWhatsThis(i18n("Show/hide the marks on the vertical scrollbar.<BR><BR>The marks, for instance, show bookmarks."));
  toggleAction->setCheckedState(KGuiItem(i18n("Hide Scroll&bar Marks")));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMarks()));

  a = toggleAction = m_toggleWWMarker = new KToggleAction(i18n("Show Static &Word Wrap Marker"), this);
  ac->addAction("view_word_wrap_marker", a);
  a->setWhatsThis( i18n(
        "Show/hide the Word Wrap Marker, a vertical line drawn at the word "
        "wrap column as defined in the editing properties" ));
  toggleAction->setCheckedState(KGuiItem(i18n("Hide Static &Word Wrap Marker")));
  connect(a, SIGNAL(triggered(bool)), SLOT( toggleWWMarker() ));

  a = m_switchCmdLine = ac->addAction("switch_to_cmd_line");
  a->setText(i18n("Switch to Command Line"));
  a->setShortcut(QKeySequence(Qt::Key_F7));
  a->setWhatsThis(i18n("Show/hide the command line on the bottom of the view."));
  connect(a, SIGNAL(triggered(bool)), SLOT(switchToCmdLine()));

  a = m_setEndOfLine = new KSelectAction(i18n("&End of Line"), this);
  ac->addAction("set_eol", a);
  a->setWhatsThis(i18n("Choose which line endings should be used, when you save the document"));
  QStringList list;
  list.append("&UNIX");
  list.append("&Windows/DOS");
  list.append("&Macintosh");
  m_setEndOfLine->setItems(list);
  m_setEndOfLine->setCurrentItem (m_doc->config()->eol());
  connect(m_setEndOfLine, SIGNAL(triggered(int)), this, SLOT(setEol(int)));

  // encoding menu
  KateViewEncodingAction *encodingAction = new KateViewEncodingAction(m_doc, this, i18n("E&ncoding"), this);
  ac->addAction("set_encoding", encodingAction);

  a = ac->addAction( KStandardAction::Find, this, SLOT(find()) );
  a->setWhatsThis(i18n("Look up the first occurrence of a piece of text or regular expression."));
  addAction(a);

  a = ac->addAction( KStandardAction::FindNext, this, SLOT(findNext()) );
  a->setWhatsThis(i18n("Look up the next occurrence of the search phrase."));
  addAction(a);

  a = ac->addAction( KStandardAction::FindPrev, "edit_find_prev", this, SLOT(findPrevious()) );
  a->setWhatsThis(i18n("Look up the previous occurrence of the search phrase."));
  addAction(a);

  // TODO: something about "replace" (kling)
  a = ac->addAction( KStandardAction::Replace, m_search, SLOT(replace()) );
  a->setWhatsThis(i18n("Look up a piece of text or regular expression and replace the result with some given text."));

  m_spell->createActions( ac );
  m_bookmarks->createActions( ac );

  slotSelectionChanged ();

  connect (this, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(slotSelectionChanged()));
}

void KateView::slotConfigDialog ()
{
  KateGlobal::self ()->configDialog (this);
}

void KateView::setupEditActions()
{
  m_editActions = new KActionCollection( static_cast<QObject*>(this) );
  m_editActions->setObjectName( "edit_actions" );
  m_editActions->setAssociatedWidget(m_viewInternal);
  KActionCollection* ac = m_editActions;

  QAction* a = ac->addAction("word_left");
  a->setText(i18n("Move Word Left"));
  a->setShortcuts(KStandardShortcut::backwardWord());
  connect(a, SIGNAL(triggered(bool)),  SLOT(wordLeft()));

  a = ac->addAction("select_char_left");
  a->setText(i18n("Select Character Left"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Left));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorLeft()));

  a = ac->addAction("select_word_left");
  a->setText(i18n("Select Word Left"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Left));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordLeft()));


  a = ac->addAction("word_right");
  a->setText(i18n("Move Word Right"));
  a->setShortcuts(KStandardShortcut::forwardWord());
  connect(a, SIGNAL(triggered(bool)), SLOT(wordRight()));

  a = ac->addAction("select_char_right");
  a->setText(i18n("Select Character Right"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorRight()));

  a = ac->addAction("select_word_right");
  a->setText(i18n("Select Word Right"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Right));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordRight()));


  a = ac->addAction("beginning_of_line");
  a->setText(i18n("Move to Beginning of Line"));
  a->setShortcut(QKeySequence(Qt::Key_Home));
  connect(a, SIGNAL(triggered(bool)), SLOT(home()));

  a = ac->addAction("beginning_of_document");
  a->setText(i18n("Move to Beginning of Document"));
  a->setShortcuts(KStandardShortcut::home());
  connect(a, SIGNAL(triggered(bool)), SLOT(top()));

  a = ac->addAction("select_beginning_of_line");
  a->setText(i18n("Select to Beginning of Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Home));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftHome()));

  a = ac->addAction("select_beginning_of_document");
  a->setText(i18n("Select to Beginning of Document"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Home));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftTop()));


  a = ac->addAction("end_of_line");
  a->setText(i18n("Move to End of Line"));
  a->setShortcut(QKeySequence(Qt::Key_End));
  connect(a, SIGNAL(triggered(bool)), SLOT(end()));

  a = ac->addAction("end_of_document");
  a->setText(i18n("Move to End of Document"));
  a->setShortcuts(KStandardShortcut::end());
  connect(a, SIGNAL(triggered(bool)), SLOT(bottom()));

  a = ac->addAction("select_end_of_line");
  a->setText(i18n("Select to End of Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_End));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftEnd()));

  a = ac->addAction("select_end_of_document");
  a->setText(i18n("Select to End of Document"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_End));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottom()));


  a = ac->addAction("select_line_up");
  a->setText(i18n("Select to Previous Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Up));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftUp()));

  a = ac->addAction("scroll_line_up");
  a->setText(i18n("Scroll Line Up"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
  connect(a, SIGNAL(triggered(bool)), SLOT(scrollUp()));


  a = ac->addAction("move_line_down");
  a->setText(i18n("Move to Next Line"));
  a->setShortcut(QKeySequence(Qt::Key_Down));
  connect(a, SIGNAL(triggered(bool)), SLOT(down()));


  a = ac->addAction("move_line_up");
  a->setText(i18n("Move to Previous Line"));
  a->setShortcut(QKeySequence(Qt::Key_Up));
  connect(a, SIGNAL(triggered(bool)), SLOT(up()));


  a = ac->addAction("move_cursor_right");
  a->setText(i18n("Move Character Right"));
  a->setShortcut(QKeySequence(Qt::Key_Right));
  connect(a, SIGNAL(triggered(bool)), SLOT(cursorRight()));


  a = ac->addAction("move_cusor_left");
  a->setText(i18n("Move Character Left"));
  a->setShortcut(QKeySequence(Qt::Key_Left));
  connect(a, SIGNAL(triggered(bool)), SLOT(cursorLeft()));


  a = ac->addAction("select_line_down");
  a->setText(i18n("Select to Next Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Down));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftDown()));

  a = ac->addAction("scroll_line_down");
  a->setText(i18n("Scroll Line Down"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
  connect(a, SIGNAL(triggered(bool)), SLOT(scrollDown()));


  a = ac->addAction("scroll_page_up");
  a->setText(i18n("Scroll Page Up"));
  a->setShortcuts(KStandardShortcut::prior());
  connect(a, SIGNAL(triggered(bool)), SLOT(pageUp()));

  a = ac->addAction("select_page_up");
  a->setText(i18n("Select Page Up"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_PageUp));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageUp()));

  a = ac->addAction("move_top_of_view");
  a->setText(i18n("Move to Top of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
  connect(a, SIGNAL(triggered(bool)), SLOT(topOfView()));

  a = ac->addAction("select_top_of_view");
  a->setText(i18n("Select to Top of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT +  Qt::Key_PageUp));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftTopOfView()));


  a = ac->addAction("scroll_page_down");
  a->setText(i18n("Scroll Page Down"));
  a->setShortcuts(KStandardShortcut::next());
  connect(a, SIGNAL(triggered(bool)), SLOT(pageDown()));

  a = ac->addAction("select_page_down");
  a->setText(i18n("Select Page Down"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_PageDown));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageDown()));

  a = ac->addAction("move_bottom_of_view");
  a->setText(i18n("Move to Bottom of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
  connect(a, SIGNAL(triggered(bool)), SLOT(bottomOfView()));

  a = ac->addAction("select_bottom_of_view");
  a->setText(i18n("Select to Bottom of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_PageDown));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottomOfView()));

  a = ac->addAction("to_matching_bracket");
  a->setText(i18n("Move to Matching Bracket"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_6));
  connect(a, SIGNAL(triggered(bool)), SLOT(toMatchingBracket()));

  a = ac->addAction("select_matching_bracket");
  a->setText(i18n("Select to Matching Bracket"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_6));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftToMatchingBracket()));


  // anders: shortcuts doing any changes should not be created in browserextension
  if ( !m_doc->readOnly() )
  {
    a = ac->addAction("transpose_char");
    a->setText(i18n("Transpose Characters"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(a, SIGNAL(triggered(bool)), SLOT(transpose()));

    a = ac->addAction("delete_line");
    a->setText(i18n("Delete Line"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K));
    connect(a, SIGNAL(triggered(bool)), SLOT(killLine()));

    a = ac->addAction("delete_word_left");
    a->setText(i18n("Delete Word Left"));
    a->setShortcuts(KStandardShortcut::deleteWordBack());
    connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordLeft()));

    a = ac->addAction("delete_word_right");
    a->setText(i18n("Delete Word Right"));
    a->setShortcuts(KStandardShortcut::deleteWordForward());
    connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordRight()));

    a = ac->addAction("delete_next_character");
    a->setText(i18n("Delete Next Character"));
    a->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(a, SIGNAL(triggered(bool)), SLOT(keyDelete()));

    a = ac->addAction("backspace");
    a->setText(i18n("Backspace"));
    a->setShortcut(QKeySequence(Qt::Key_Backspace));
    connect(a, SIGNAL(triggered(bool)), SLOT(backspace()));

#ifdef __GNUC__
#warning REMOVE THIS IN THE RELEASE
#endif

//     a = ac->addAction("debug_template_code");
//     a->setText(i18n("Debug TemplateCode"));
//     a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_1));
//     connect(a, SIGNAL(triggered(bool)), m_doc,SLOT(testTemplateCode()));

  }

  m_editActions->setConfigGroup("Katepart Shortcuts");
  m_editActions->readSettings();

  if( hasFocus() )
    slotGotFocus();
  else
    slotLostFocus();
}

void KateView::setupCodeFolding()
{
  KActionCollection *ac=this->actionCollection();

  QAction* a = ac->addAction("folding_toplevel");
  a->setText(i18n("Collapse Toplevel"));
  a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Minus));
  connect(a, SIGNAL(triggered(bool)), m_doc->foldingTree(), SLOT(collapseToplevelNodes()));

  a = ac->addAction("folding_expandtoplevel");
  a->setText(i18n("Expand Toplevel"));
  a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Plus));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotExpandToplevel()));

  a = ac->addAction("folding_collapselocal");
  a->setText(i18n("Collapse One Local Level"));
  a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Minus));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotCollapseLocal()));

  a = ac->addAction("folding_expandlocal");
  a->setText(i18n("Expand One Local Level"));
  a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_Plus));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotExpandLocal()));
}

void KateView::slotExpandToplevel()
{
  m_doc->foldingTree()->expandToplevelNodes(m_doc->lines());
}

void KateView::slotCollapseLocal()
{
  int realLine = m_doc->foldingTree()->collapseOne(cursorPosition().line());
  if (realLine != -1)
    // TODO rodda: fix this to only set line and allow internal view to chose column
    // Explicitly call internal because we want this to be registered as an internal call
    setCursorPositionInternal(KTextEditor::Cursor(realLine, virtualCursorColumn()), m_doc->config()->tabWidth());
}

void KateView::slotExpandLocal()
{
  m_doc->foldingTree()->expandOne(cursorPosition().line(), m_doc->lines());
}

QString KateView::viewMode () const
{
  if (!m_doc->isReadWrite())
    return i18n ("R/O");

  return isOverwriteMode() ? i18n("OVR") : i18n ("NORM");
}

void KateView::slotGotFocus()
{
  editActionCollection()->setEnabled(true);
  emit focusIn ( this );
}

void KateView::slotLostFocus()
{
  editActionCollection()->setEnabled(false);
  emit focusOut ( this );
}

void KateView::setDynWrapIndicators(int mode)
{
  config()->setDynWordWrapIndicators (mode);
}

bool KateView::isOverwriteMode() const
{
  return m_doc->config()->configFlags() & KateDocumentConfig::cfOvr;
}

void KateView::reloadFile()
{
  // save cursor position
  KTextEditor::Cursor backupCursor(cursorPositionVirtual());

  // save bookmarks
  m_doc->documentReload();

  if (m_doc->lines() >= backupCursor.line())
    // Explicitly call internal function because we want this to be registered as a non-external call
    setCursorPositionInternal( backupCursor, m_doc->config()->tabWidth(), false );
}

void KateView::slotUpdate()
{
  slotNewUndo();
}

void KateView::slotReadWriteChanged ()
{
  if ( m_toggleWriteLock )
    m_toggleWriteLock->setChecked( ! m_doc->isReadWrite() );

  m_cut->setEnabled (m_doc->isReadWrite());
  m_paste->setEnabled (m_doc->isReadWrite());

  QStringList l;

  l << "edit_replace" << "set_insert" << "tools_spelling" << "tools_indent"
      << "tools_unindent" << "tools_cleanIndent" << "tools_align"  << "tools_comment"
      << "tools_uncomment" << "tools_uppercase" << "tools_lowercase"
      << "tools_capitalize" << "tools_join_lines" << "tools_apply_wordwrap"
      << "edit_undo" << "edit_redo" << "tools_spelling_from_cursor"
      << "tools_spelling_selection";

  QAction *a = 0;
  for (int z = 0; z < l.size(); z++)
    if ((a = actionCollection()->action( l[z].toAscii().constData() )))
      a->setEnabled (m_doc->isReadWrite());
}

void KateView::slotNewUndo()
{
  if (m_doc->readOnly())
    return;

  if ((m_doc->undoCount() > 0) != m_editUndo->isEnabled())
    m_editUndo->setEnabled(m_doc->undoCount() > 0);

  if ((m_doc->redoCount() > 0) != m_editRedo->isEnabled())
    m_editRedo->setEnabled(m_doc->redoCount() > 0);
}

void KateView::slotDropEventPass( QDropEvent * ev )
{
  const KUrl::List lstDragURLs=KUrl::List::fromMimeData(ev->mimeData());
  bool ok = !lstDragURLs.isEmpty();

  KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( doc() );
  if ( ok && ext )
    emit ext->openUrlRequest( lstDragURLs.first() );
}

void KateView::contextMenuEvent( QContextMenuEvent *ev )
{
  if ( !m_doc || !m_doc->browserExtension()  )
    return;
  emit m_doc->browserExtension()->popupMenu( ev->globalPos(), m_doc->url(),
                                        QLatin1String( "text/plain" ) );
  ev->accept();
}

bool KateView::setCursorPositionInternal( const KTextEditor::Cursor& position, uint tabwidth, bool calledExternally )
{
  KateTextLine::Ptr l = m_doc->kateTextLine( position.line() );

  if (!l)
    return false;

  QString line_str = m_doc->line( position.line() );

  int x = 0;
  for (int z = 0; z < line_str.length() && z < position.column(); z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  m_viewInternal->updateCursor( position, false, true, calledExternally );

  return true;
}

void KateView::toggleInsert()
{
  m_doc->config()->setConfigFlags(m_doc->config()->configFlags() ^ KateDocumentConfig::cfOvr);
  m_toggleInsert->setChecked (isOverwriteMode ());

  emit viewModeChanged(this);
  emit viewEditModeChanged(this,viewEditMode());
}

void KateView::slotSaveCanceled( const QString& error )
{
  if ( !error.isEmpty() ) // happens when canceling a job
    KMessageBox::error( this, error );
}

void KateView::gotoLine()
{
  // no around, create one...
  if (!m_gotoBar)
    m_gotoBar = new KateGotoBar (m_viewBar);

  // show it
  m_gotoBar->showBar ();
}

void KateView::joinLines()
{
  int first = selectionRange().start().line();
  int last = selectionRange().end().line();
  //int left = m_doc->line( last ).length() - m_doc->selEndCol();
  if ( first == last )
  {
    first = cursorPosition().line();
    last = first + 1;
  }
  m_doc->joinLines( first, last );
}

void KateView::readSessionConfig(const KConfigGroup& config)
{
  setCursorPositionInternal(KTextEditor::Cursor(config.readEntry("CursorLine",0), config.readEntry("CursorColumn",0)));
}

void KateView::writeSessionConfig(KConfigGroup& config)
{
  config.writeEntry("CursorLine",m_viewInternal->m_cursor.line());
  config.writeEntry("CursorColumn",m_viewInternal->m_cursor.column());
}

int KateView::getEol() const
{
  return m_doc->config()->eol();
}

void KateView::setEol(int eol)
{
  if (!doc()->isReadWrite())
    return;

  if (m_updatingDocumentConfig)
    return;

  m_doc->config()->setEol (eol);
}

void KateView::setIconBorder( bool enable )
{
  config()->setIconBar (enable);
}

void KateView::toggleIconBorder()
{
  config()->setIconBar (!config()->iconBar());
}

void KateView::setLineNumbersOn( bool enable )
{
  config()->setLineNumbers (enable);
}

void KateView::toggleLineNumbersOn()
{
  config()->setLineNumbers (!config()->lineNumbers());
}

void KateView::setScrollBarMarks( bool enable )
{
  config()->setScrollBarMarks (enable);
}

void KateView::toggleScrollBarMarks()
{
  config()->setScrollBarMarks (!config()->scrollBarMarks());
}

void KateView::toggleDynWordWrap()
{
  config()->setDynWordWrap( !config()->dynWordWrap() );
}

void KateView::toggleWWMarker()
{
  m_renderer->config()->setWordWrapMarker (!m_renderer->config()->wordWrapMarker());
}

void KateView::setFoldingMarkersOn( bool enable )
{
  config()->setFoldingBar ( enable );
}

void KateView::toggleFoldingMarkers()
{
  config()->setFoldingBar ( !config()->foldingBar() );
}

bool KateView::iconBorder() {
  return m_viewInternal->m_leftBorder->iconBorderOn();
}

bool KateView::lineNumbersOn() {
  return m_viewInternal->m_leftBorder->lineNumbersOn();
}

bool KateView::scrollBarMarks() {
  return m_viewInternal->m_lineScroll->showMarks();
}

int KateView::dynWrapIndicators() {
  return m_viewInternal->m_leftBorder->dynWrapIndicators();
}

bool KateView::foldingMarkersOn() {
  return m_viewInternal->m_leftBorder->foldingMarkersOn();
}

void KateView::showCmdLine ( bool enabled )
{
  if (enabled == m_cmdLineOn)
    return;

  if (enabled)
  {
    if (!m_cmdLine)
    {
      m_cmdLine = new KateCmdLine (this);
      m_vBox->addWidget (m_cmdLine);
    }

    m_cmdLine->show ();
    m_cmdLine->setFocus();
  }
  else {
    m_cmdLine->hide ();
    //m_toggleCmdLine->setChecked(false);
  }

  m_cmdLineOn = enabled;
}

void KateView::toggleCmdLine ()
{
  m_config->setCmdLine (!m_config->cmdLine ());
}

void KateView::toggleWriteLock()
{
  m_doc->setReadWrite( ! m_doc->isReadWrite() );
}

void KateView::enableTextHints(int timeout)
{
  m_viewInternal->enableTextHints(timeout);
}

void KateView::disableTextHints()
{
  m_viewInternal->disableTextHints();
}

void KateView::slotNeedTextHint(int line, int col, QString &text)
{
  text=QString("test %1 %2").arg(line).arg(col);
}

void KateView::find()
{
  m_searchBar->showBar();
  m_searchBar->setFocus();
}

void KateView::find( const QString& pattern, long flags, bool add )
{
  // TODO: something like the old thing (kling)
  //m_search->find( pattern, flags, add );
}

void KateView::replace()
{
  m_search->replace();
}

void KateView::replace( const QString &pattern, const QString &replacement, long flags )
{
  m_search->replace( pattern, replacement, flags );
}

void KateView::findNext()
{
  m_searchBar->findNext();
}

void KateView::findPrevious()
{
  m_searchBar->findPrevious();
}

void KateView::slotSelectionChanged ()
{
  m_copy->setEnabled (selection());
  m_copyHTML->setEnabled (selection());
  m_deSelect->setEnabled (selection());

  if (m_doc->readOnly())
    return;

  m_cut->setEnabled (selection());

  m_spell->updateActions ();
}

void KateView::switchToCmdLine ()
{
  if (!m_cmdLineOn)
    m_config->setCmdLine (true);
  else {
        if (m_cmdLine->hasFocus()) {
                this->setFocus();
                return;
        }
  }
  m_cmdLine->setFocus ();
}

KateRenderer *KateView::renderer ()
{
  return m_renderer;
}

void KateView::updateConfig ()
{
  if (m_startingUp)
    return;

  m_editActions->readSettings();

  // dyn. word wrap & markers
  if (m_hasWrap != config()->dynWordWrap()) {
    m_viewInternal->prepareForDynWrapChange();

    m_hasWrap = config()->dynWordWrap();

    m_viewInternal->dynWrapChanged();

    m_setDynWrapIndicators->setEnabled(config()->dynWordWrap());
    m_toggleDynWrap->setChecked( config()->dynWordWrap() );
  }

  m_viewInternal->m_leftBorder->setDynWrapIndicators( config()->dynWordWrapIndicators() );
  m_setDynWrapIndicators->setCurrentItem( config()->dynWordWrapIndicators() );

  // line numbers
  m_viewInternal->m_leftBorder->setLineNumbersOn( config()->lineNumbers() );
  m_toggleLineNumbers->setChecked( config()->lineNumbers() );

  // icon bar
  m_viewInternal->m_leftBorder->setIconBorderOn( config()->iconBar() );
  m_toggleIconBar->setChecked( config()->iconBar() );

  // scrollbar marks
  m_viewInternal->m_lineScroll->setShowMarks( config()->scrollBarMarks() );
  m_toggleScrollBarMarks->setChecked( config()->scrollBarMarks() );

  // cmd line
  showCmdLine (config()->cmdLine());
  //m_toggleCmdLine->setChecked( config()->cmdLine() );

  // misc edit
  m_toggleBlockSelection->setChecked( blockSelectionMode() );
  m_toggleInsert->setChecked( isOverwriteMode() );

  updateFoldingConfig ();

  // bookmark
  m_bookmarks->setSorting( (KateBookmarks::Sorting) config()->bookmarkSort() );

  m_viewInternal->setAutoCenterLines(config()->autoCenterLines ());
}

void KateView::updateDocumentConfig()
{
  if (m_startingUp)
    return;

  m_updatingDocumentConfig = true;

  m_setEndOfLine->setCurrentItem (m_doc->config()->eol());

  m_updatingDocumentConfig = false;

  m_viewInternal->updateView (true);

  m_renderer->setTabWidth (m_doc->config()->tabWidth());
  m_renderer->setIndentWidth (m_doc->config()->indentationWidth());
}

void KateView::updateRendererConfig()
{
  if (m_startingUp)
    return;

  m_toggleWWMarker->setChecked( m_renderer->config()->wordWrapMarker()  );

  // update the text area
  m_viewInternal->updateView (true);
  m_viewInternal->repaint ();

  // update the left border right, for example linenumbers
  m_viewInternal->m_leftBorder->updateFont();
  m_viewInternal->m_leftBorder->repaint ();

// @@ showIndentLines is not cached anymore.
//  m_renderer->setShowIndentLines (m_renderer->config()->showIndentationLines());
}

void KateView::updateFoldingConfig ()
{
  // folding bar
  bool doit = config()->foldingBar() && m_doc->highlight() && m_doc->highlight()->allowsFolding();
  m_viewInternal->m_leftBorder->setFoldingMarkersOn(doit);
  m_toggleFoldingMarkers->setChecked( doit );
  m_toggleFoldingMarkers->setEnabled( m_doc->highlight() && m_doc->highlight()->allowsFolding() );

  QStringList l;

  l << "folding_toplevel" << "folding_expandtoplevel"
    << "folding_collapselocal" << "folding_expandlocal";

  QAction *a = 0;
  for (int z = 0; z < l.size(); z++)
    if ((a = actionCollection()->action( l[z].toAscii().constData() )))
      a->setEnabled (m_doc->highlight() && m_doc->highlight()->allowsFolding());
}

//BEGIN EDIT STUFF
void KateView::editStart ()
{
  m_viewInternal->editStart ();
}

void KateView::editEnd (int editTagLineStart, int editTagLineEnd, bool tagFrom)
{
  m_viewInternal->editEnd (editTagLineStart, editTagLineEnd, tagFrom);
}

void KateView::editSetCursor (const KTextEditor::Cursor &cursor)
{
  m_viewInternal->editSetCursor (cursor);
}
//END

//BEGIN TAG & CLEAR
bool KateView::tagLine (const KTextEditor::Cursor& virtualCursor)
{
  return m_viewInternal->tagLine (virtualCursor);
}

bool KateView::tagRange(const KTextEditor::Range& range, bool realLines)
{
  return m_viewInternal->tagRange(range, realLines);
}

bool KateView::tagLines (int start, int end, bool realLines)
{
  return m_viewInternal->tagLines (start, end, realLines);
}

bool KateView::tagLines (KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors)
{
  return m_viewInternal->tagLines (start, end, realCursors);
}

void KateView::tagAll ()
{
  m_viewInternal->tagAll ();
}

void KateView::relayoutRange( const KTextEditor::Range & range, bool realLines )
{
  return m_viewInternal->relayoutRange(range, realLines);
}

void KateView::clear ()
{
  m_viewInternal->clear ();
}

void KateView::repaintText (bool paintOnlyDirty)
{
  if (paintOnlyDirty)
    m_viewInternal->updateDirty();
  else
    m_viewInternal->update();
}

void KateView::updateView (bool changed)
{
  m_viewInternal->updateView (changed);
  m_viewInternal->m_leftBorder->update();
}

//END

void KateView::slotHlChanged()
{
  KateHighlighting *hl = m_doc->highlight();
  bool ok ( !hl->getCommentStart(0).isEmpty() || !hl->getCommentSingleLineStart(0).isEmpty() );

  if (actionCollection()->action("tools_comment"))
    actionCollection()->action("tools_comment")->setEnabled( ok );

  if (actionCollection()->action("tools_uncomment"))
    actionCollection()->action("tools_uncomment")->setEnabled( ok );

  // show folding bar if "view defaults" says so, otherwise enable/disable only the menu entry
  updateFoldingConfig ();
}

int KateView::virtualCursorColumn() const
{
  int r = m_doc->toVirtualColumn(m_viewInternal->getCursor());
  if ( !( m_doc->config()->configFlags() & KateDocumentConfig::cfWrapCursor ) &&
       m_viewInternal->getCursor().column() > m_doc->line( m_viewInternal->getCursor().line() ).length()  )
    r += m_viewInternal->getCursor().column() - m_doc->line( m_viewInternal->getCursor().line() ).length();

  return r;
}

void KateView::notifyMousePositionChanged(const KTextEditor::Cursor& newPosition)
{
  emit mousePositionChanged(this, newPosition);
}

//BEGIN KTextEditor::SelectionInterface stuff

bool KateView::setSelection( const KTextEditor::Range &selection )
{
  KTextEditor::Range oldSelection = *m_selection;
  *m_selection = selection;

  tagSelection(oldSelection);

  repaintText(true);

  emit selectionChanged (this);

  return true;
}

bool KateView::clearSelection()
{
  return clearSelection(true);
}

bool KateView::clearSelection(bool redraw, bool finishedChangingSelection)
{
  if( !selection() )
    return false;

  KTextEditor::Range oldSelection = *m_selection;

  *m_selection = KTextEditor::Range::invalid();

  tagSelection(oldSelection);

  oldSelection = *m_selection;

  if (redraw)
    repaintText(true);

  if (finishedChangingSelection)
    emit selectionChanged(this);

  return true;
}

bool KateView::selection() const
{
  return m_selection->isValid ();
}

QString KateView::selectionText() const
{
  KTextEditor::Range range = *m_selection;

  if ( blockSelect )
    blockFix(range);

  return m_doc->text(range, blockSelect);
}

bool KateView::removeSelectedText()
{
  if (!selection())
    return false;

  m_doc->editStart (true, Kate::CutCopyPasteEdit);

  KTextEditor::Range range = *m_selection;

  if ( blockSelect )
    blockFix(range);

  m_doc->removeText(range, blockSelect);

  // don't redraw the cleared selection - that's done in editEnd().
  clearSelection(false);

  m_doc->editEnd ();

  return true;
}

bool KateView::selectAll()
{
  setBlockSelectionMode (false);

  return setSelection(KTextEditor::Range(KTextEditor::Cursor(), m_doc->documentEnd()));
}

bool KateView::cursorSelected(const KTextEditor::Cursor& cursor)
{
  KTextEditor::Cursor ret = cursor;
  if ( (!blockSelect) && (ret.column() < 0) )
    ret.setColumn(0);

  if (blockSelect)
    return cursor.line() >= m_selection->start().line() && ret.line() <= m_selection->end().line() && ret.column() >= m_selection->start().column() && ret.column() < m_selection->end().column();
  else
    return m_selection->contains(cursor);
}

bool KateView::lineSelected (int line)
{
  return !blockSelect && m_selection->containsLine(line);
}

bool KateView::lineEndSelected (const KTextEditor::Cursor& lineEndPos)
{
  return (!blockSelect)
    && (lineEndPos.line() > m_selection->start().line() || (lineEndPos.line() == m_selection->start().line() && (m_selection->start().column() < lineEndPos.column() || lineEndPos.column() == -1)))
    && (lineEndPos.line() < m_selection->end().line() || (lineEndPos.line() == m_selection->end().line() && (lineEndPos.column() <= m_selection->end().column() && lineEndPos.column() != -1)));
}

bool KateView::lineHasSelected (int line)
{
  return selection() && m_selection->containsLine(line);
}

bool KateView::lineIsSelection (int line)
{
  return (line == m_selection->start().line() && line == m_selection->end().line());
}

void KateView::tagSelection(const KTextEditor::Range &oldSelection)
{
  if (selection()) {
    if (oldSelection.start().line() == -1) {
      // We have to tag the whole lot if
      // 1) we have a selection, and:
      //  a) it's new; or
      tagLines(*m_selection, true);

    } else if (blockSelectionMode() && (oldSelection.start().column() != m_selection->start().column() || oldSelection.end().column() != m_selection->end().column())) {
      //  b) we're in block selection mode and the columns have changed
      tagLines(*m_selection, true);
      tagLines(oldSelection, true);

    } else {
      if (oldSelection.start() != m_selection->start()) {
        if (oldSelection.start() < m_selection->start())
          tagLines(oldSelection.start(), m_selection->start(), true);
        else
          tagLines(m_selection->start(), oldSelection.start(), true);
      }

      if (oldSelection.end() != m_selection->end()) {
        if (oldSelection.end() < m_selection->end())
          tagLines(oldSelection.end(), m_selection->end(), true);
        else
          tagLines(m_selection->end(), oldSelection.end(), true);
      }
    }

  } else {
    // No more selection, clean up
    tagLines(oldSelection, true);
  }
}

void KateView::selectWord( const KTextEditor::Cursor& cursor )
{
  int start, end, len;

  KateTextLine::Ptr textLine = m_doc->plainKateTextLine(cursor.line());

  if (!textLine)
    return;

  len = textLine->length();
  start = end = cursor.column();
  while (start > 0 && m_doc->highlight()->isInWord(textLine->at(start - 1), textLine->attribute(start - 1))) start--;
  while (end < len && m_doc->highlight()->isInWord(textLine->at(end), textLine->attribute(start - 1))) end++;
  if (end <= start) return;

  setSelection (KTextEditor::Range(cursor.line(), start, cursor.line(), end));
}

void KateView::selectLine( const KTextEditor::Cursor& cursor )
{
  setSelection (KTextEditor::Range(cursor.line(), 0, cursor.line()+1, 0));
}

void KateView::cut()
{
  if (!selection())
    return;

  copy();
  removeSelectedText();
}

void KateView::copy() const
{
  if (!selection())
    return;

  QApplication::clipboard()->setText(selectionText ());
}

void KateView::applyWordWrap ()
{
  if (selection())
    m_doc->wrapText (selectionRange().start().line(), selectionRange().end().line());
  else
    m_doc->wrapText (0, m_doc->lastLine());
}

void KateView::copyHTML()
{
  if (!selection())
    return;

  QMimeData *data = new QMimeData();
  data->setText(selectionText());
  data->setHtml(selectionAsHtml());
  QApplication::clipboard()->setMimeData(data);
}

QString KateView::selectionAsHtml()
{
  return textAsHtml(*m_selection, blockSelect);
}

QString KateView::textAsHtml ( KTextEditor::Range range, bool blockwise)
{
  kDebug(13020) << "textAsHtml" << endl;
  if (blockwise)
    blockFix(range);

  QString s;
  QTextStream ts( &s, QIODevice::WriteOnly );
  //ts.setEncoding(QTextStream::UnicodeUTF8);
  ts.setCodec(QTextCodec::codecForName("UTF-8"));
  ts << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
  ts << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
  ts << "<head>" << endl;
  ts << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />" << endl;
  ts << "<meta name=\"Generator\" content=\"Kate, the KDE Advanced Text Editor\" />" << endl;
  ts << "</head>" << endl;

  ts << "<body>" << endl;
  textAsHtmlStream(range, blockwise, &ts);

  ts << "</body>" << endl;
  ts << "</html>" << endl;
  kDebug(13020) << "html is: " << s << endl;
  return s;
}

void KateView::textAsHtmlStream ( const KTextEditor::Range& range, bool blockwise, QTextStream *ts)
{
  if ( (blockwise || range.onSingleLine()) && (range.start().column() > range.end().column() ) )
    return;

  if (range.onSingleLine())
  {
    KateTextLine::Ptr textLine = m_doc->kateTextLine(range.start().line());
    if ( !textLine )
      return;

    (*ts) << "<pre>" << endl;

    lineAsHTML(textLine, range.start().column(), range.columnWidth(), ts);
  }
  else
  {
    (*ts) << "<pre>" << endl;

    for (int i = range.start().line(); (i <= range.end().line()) && (i < m_doc->lines()); ++i)
    {
      KateTextLine::Ptr textLine = m_doc->kateTextLine(i);

      if ( !blockwise )
      {
        if (i == range.start().line())
          lineAsHTML(textLine, range.start().column(), textLine->length() - range.start().column(), ts);
        else if (i == range.end().line())
          lineAsHTML(textLine, 0, range.end().column(), ts);
        else
          lineAsHTML(textLine, 0, textLine->length(), ts);
      }
      else
      {
        lineAsHTML( textLine, range.start().column(), range.columnWidth(), ts);
      }

      if ( i < range.end().line() )
        (*ts) << "\n";    //we are inside a <pre>, so a \n is a new line
    }
  }
  (*ts) << "</pre>";
}

void KateView::lineAsHTML (KateTextLine::Ptr line, int startCol, int length, QTextStream *outputStream)
{
  if(length == 0) return;
  // some variables :
  bool previousCharacterWasBold = false;
  bool previousCharacterWasItalic = false;
  // when entering a new color, we'll close all the <b> & <i> tags,
  // for HTML compliancy. that means right after that font tag, we'll
  // need to reinitialize the <b> and <i> tags.
  bool needToReinitializeTags = false;
  QColor previousCharacterColor(0,0,0); // default color of HTML characters is black
  QColor blackColor(0,0,0);
//  (*outputStream) << "<span style='color: #000000'>";


  // for each character of the line : (curPos is the position in the line)
  for (int curPos=startCol;curPos<(length+startCol);curPos++)
    {
      KTextEditor::Attribute::Ptr charAttributes = m_renderer->attribute(line->attribute(curPos));

      //ASSERT(charAttributes != NULL);
      // let's give the color for that character :
      if ( (charAttributes->foreground() != previousCharacterColor))
      {  // the new character has a different color :
        // if we were in a bold or italic section, close it
        if (previousCharacterWasBold)
          (*outputStream) << "</b>";
        if (previousCharacterWasItalic)
          (*outputStream) << "</i>";

        // close the previous font tag :
  if(previousCharacterColor != blackColor)
          (*outputStream) << "</span>";
        // let's read that color :
        int red, green, blue;
        // getting the red, green, blue values of the color :
        charAttributes->foreground().color().getRgb(&red, &green, &blue);
  if(!(red == 0 && green == 0 && blue == 0)) {
          (*outputStream) << "<span style='color: #"
              << ( (red < 0x10)?"0":"")  // need to put 0f, NOT f for instance. don't touch 1f.
              << QString::number(red, 16) // html wants the hex value here (hence the 16)
              << ( (green < 0x10)?"0":"")
              << QString::number(green, 16)
              << ( (blue < 0x10)?"0":"")
              << QString::number(blue, 16)
              << "'>";
  }
        // we need to reinitialize the bold/italic status, since we closed all the tags
        needToReinitializeTags = true;
      }
      // bold status :
      if ( (needToReinitializeTags && charAttributes->fontBold()) ||
          (!previousCharacterWasBold && charAttributes->fontBold()) )
        // we enter a bold section
        (*outputStream) << "<b>";
      if ( !needToReinitializeTags && (previousCharacterWasBold && !charAttributes->fontBold()) )
        // we leave a bold section
        (*outputStream) << "</b>";

      // italic status :
      if ( (needToReinitializeTags && charAttributes->fontItalic()) ||
           (!previousCharacterWasItalic && charAttributes->fontItalic()) )
        // we enter an italic section
        (*outputStream) << "<i>";
      if ( !needToReinitializeTags && (previousCharacterWasItalic && !charAttributes->fontItalic()) )
        // we leave an italic section
        (*outputStream) << "</i>";

      // write the actual character :
      (*outputStream) << Qt::escape(QString(line->at(curPos)));

      // save status for the next character :
      previousCharacterWasItalic = charAttributes->fontItalic();
      previousCharacterWasBold = charAttributes->fontBold();
      previousCharacterColor = charAttributes->foreground().color();
      needToReinitializeTags = false;
    }
  // Be good citizens and close our tags
  if (previousCharacterWasBold)
    (*outputStream) << "</b>";
  if (previousCharacterWasItalic)
    (*outputStream) << "</i>";

  if(previousCharacterColor != blackColor)
    (*outputStream) << "</span>";
}

void KateView::exportAsHTML ()
{
  KUrl url = KFileDialog::getSaveUrl(m_doc->documentName(), "text/html",
                                     0, i18n("Export File as HTML"));

  if ( url.isEmpty() )
    return;

  QString filename;

  if ( url.isLocalFile() )
    filename = url.path();
  else {
    KTemporaryFile tmp; // ### only used for network export
    tmp.setAutoRemove(false);
    tmp.open();
    filename = tmp.fileName();
  }

  KSaveFile savefile(filename);
  if (savefile.open())
  {
    QTextStream outputStream ( &savefile );

    //outputStream.setEncoding(QTextStream::UnicodeUTF8);
    outputStream.setCodec(QTextCodec::codecForName("UTF-8"));
    // let's write the HTML header :
    outputStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    outputStream << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
    outputStream << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
    outputStream << "<head>" << endl;
    outputStream << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />" << endl;
    outputStream << "<meta name=\"Generator\" content=\"Kate, the KDE Advanced Text Editor\" />" << endl;
    // for the title, we write the name of the file (/usr/local/emmanuel/myfile.cpp -> myfile.cpp)
    outputStream << "<title>" << m_doc->documentName () << "</title>" << endl;
    outputStream << "</head>" << endl;
    outputStream << "<body>" << endl;

    textAsHtmlStream(m_doc->documentRange(), false, &outputStream);

    outputStream << "</body>" << endl;
    outputStream << "</html>" << endl;
    outputStream.flush();

    savefile.finalize(); //check error?
  }
//     else
//       {/*ERROR*/}

  if ( url.isLocalFile() )
      return;

  KIO::NetAccess::upload( filename, url, 0 );
}
//END

//BEGIN KTextEditor::BlockSelectionInterface stuff

bool KateView::blockSelectionMode () const
{
  return blockSelect;
}

bool KateView::setBlockSelectionMode (bool on)
{
  if (on != blockSelect)
  {
    blockSelect = on;

    KTextEditor::Range oldSelection = *m_selection;

    clearSelection(false, false);

    setSelection(oldSelection);

    m_toggleBlockSelection->setChecked( blockSelectionMode() );
  }

  return true;
}

bool KateView::toggleBlockSelectionMode ()
{
  m_toggleBlockSelection->setChecked (!blockSelect);
  return setBlockSelectionMode (!blockSelect);
}

bool KateView::wrapCursor ()
{
  return !blockSelectionMode() && (m_doc->config()->configFlags() & KateDocumentConfig::cfWrapCursor);
}

//END

//BEGIN IM INPUT STUFF
void KateView::setIMSelectionValue( const KTextEditor::Range& imRange, const KTextEditor::Range& imSelection, bool imComposeEvent )
{
  m_imRange = imRange;
  m_imSelection = imSelection;
  m_imComposeEvent = imComposeEvent;
}

const KTextEditor::Range& KateView::imSelection() const
{
  return m_imSelection;
}

const KTextEditor::Range& KateView::imEdit() const
{
  return m_imRange;
}
//END IM INPUT STUFF

void KateView::slotTextInserted ( KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text)
{
  emit textInserted ( view, position, text);
}

bool KateView::insertTemplateTextImplementation ( const KTextEditor::Cursor& c, const QString &templateString, const QMap<QString,QString> &initialValues) {
  return m_doc->insertTemplateTextImplementation(c,templateString,initialValues,this);
}

bool KateView::tagLines( KTextEditor::Range range, bool realRange )
{
  return tagLines(range.start(), range.end(), realRange);
}

void KateView::addInternalHighlight( KTextEditor::SmartRange * topRange )
{
  m_internalHighlights.append(topRange);

  addHighlightRange(topRange);
}

void KateView::removeInternalHighlight( KTextEditor::SmartRange * topRange )
{
  m_internalHighlights.removeAll(topRange);

  removeHighlightRange(topRange);
}

const QList< KTextEditor::SmartRange * > & KateView::internalHighlights( ) const
{
  return m_internalHighlights;
}

void KateView::rangeDeleted( KTextEditor::SmartRange * range )
{
  removeExternalHighlight(range);
  removeActions(range);
}

void KateView::addExternalHighlight( KTextEditor::SmartRange * topRange, bool supportDynamic )
{
  if (m_externalHighlights.contains(topRange))
    return;

  m_externalHighlights.append(topRange);

  // Deal with the range being deleted externally
  topRange->addWatcher(this);

  if (supportDynamic) {
    m_externalHighlightsDynamic.append(topRange);
    emit dynamicHighlightAdded(static_cast<KateSmartRange*>(topRange));
  }

  addHighlightRange(topRange);
}

void KateView::removeExternalHighlight( KTextEditor::SmartRange * topRange )
{
  if (!m_externalHighlights.contains(topRange))
    return;

  m_externalHighlights.removeAll(topRange);

  if (!m_actions.contains(topRange))
    topRange->removeWatcher(this);

  if (m_externalHighlightsDynamic.contains(topRange)) {
    m_externalHighlightsDynamic.removeAll(topRange);
    emit dynamicHighlightRemoved(static_cast<KateSmartRange*>(topRange));
  }

  removeHighlightRange(topRange);
}

const QList< KTextEditor::SmartRange * > & KateView::externalHighlights( ) const
{
  return m_externalHighlights;
}

void KateView::addHighlightRange( KTextEditor::SmartRange * range )
{
  m_viewInternal->addHighlightRange(range);
}

void KateView::removeHighlightRange( KTextEditor::SmartRange * range )
{
  m_viewInternal->removeHighlightRange(range);
}

void KateView::addActions( KTextEditor::SmartRange * topRange )
{
  if (m_actions.contains(topRange))
    return;

  m_actions.append(topRange);

  // Deal with the range being deleted externally
  topRange->addWatcher(this);
}

void KateView::removeActions( KTextEditor::SmartRange * topRange )
{
  if (!m_actions.contains(topRange))
    return;

  m_actions.removeAll(topRange);

  if (!m_externalHighlights.contains(topRange))
    topRange->removeWatcher(this);
}

const QList< KTextEditor::SmartRange * > & KateView::actions( ) const
{
  return m_actions;
}

void KateView::clearExternalHighlights( )
{
  m_externalHighlights.clear();
}

void KateView::clearActions( )
{
  m_actions.clear();
}

bool KateView::mouseTrackingEnabled( ) const
{
  // FIXME support
  return true;
}

bool KateView::setMouseTrackingEnabled( bool )
{
  // FIXME support
  return true;
}

bool KateView::isCompletionActive( ) const
{
  return completionWidget()->isCompletionActive();
}

KateCompletionWidget* KateView::completionWidget() const
{
  if (!m_completionWidget)
    m_completionWidget = new KateCompletionWidget(const_cast<KateView*>(this));

  return m_completionWidget;
}

void KateView::startCompletion( const KTextEditor::Range & word, KTextEditor::CodeCompletionModel * model )
{
  completionWidget()->startCompletion(word, model);
}

void KateView::abortCompletion( )
{
  completionWidget()->abortCompletion();
}

void KateView::forceCompletion( )
{
  completionWidget()->execute();
}

void KateView::registerCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  completionWidget()->registerCompletionModel(model);
}

void KateView::unregisterCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  completionWidget()->unregisterCompletionModel(model);
}

bool KateView::isAutomaticInvocationEnabled() const
{
  return completionWidget()->isAutomaticInvocationEnabled();
}

void KateView::setAutomaticInvocationEnabled(bool enabled)
{
  completionWidget()->setAutomaticInvocationEnabled(enabled);
}

void KateView::sendCompletionExecuted(const KTextEditor::Cursor& position, KTextEditor::CodeCompletionModel* model, int row)
{
  emit completionExecuted(this, position, model, row);
}

void KateView::sendCompletionAborted()
{
  emit completionAborted(this);
}

void KateView::paste( )
{
  m_doc->paste( this );
  m_viewInternal->repaint();
}

bool KateView::setCursorPosition( KTextEditor::Cursor position )
{
  return setCursorPositionInternal( position, 1, true );
}

KTextEditor::Cursor KateView::cursorPosition( ) const
{
  return m_viewInternal->getCursor();
}

KTextEditor::Cursor KateView::cursorPositionVirtual( ) const
{
  return KTextEditor::Cursor (m_viewInternal->getCursor().line(), virtualCursorColumn());
}

QPoint KateView::cursorToCoordinate( const KTextEditor::Cursor & cursor ) const
{
  return m_viewInternal->cursorToCoordinate(cursor);
}

QPoint KateView::cursorPositionCoordinates( ) const
{
  return m_viewInternal->cursorCoordinates();
}

bool KateView::setCursorPositionVisual( const KTextEditor::Cursor & position )
{
  return setCursorPositionInternal( position, m_doc->config()->tabWidth(), true );
}

QString KateView::currentTextLine( )
{
  return m_doc->line( cursorPosition().line() );
}

QString KateView::currentWord( )
{
  return m_doc->getWord( cursorPosition() );
}

void KateView::indent( )
{
  m_doc->indent( this, cursorPosition().line(), 1 );
}

void KateView::unIndent( )
{
  m_doc->indent( this, cursorPosition().line(), -1 );
}

void KateView::cleanIndent( )
{
  m_doc->indent( this, cursorPosition().line(), 0 );
}

void KateView::align( )
{
  m_doc->align( this, cursorPosition().line() );
}

void KateView::comment( )
{
  m_doc->comment( this, cursorPosition().line(), cursorPosition().column(), 1 );
}

void KateView::uncomment( )
{
  m_doc->comment( this, cursorPosition().line(), cursorPosition().column(),-1 );
}

void KateView::uppercase( )
{
  m_doc->transform( this, m_viewInternal->m_cursor, KateDocument::Uppercase );
}

void KateView::killLine( )
{
  m_doc->removeLine( cursorPosition().line() );
}

void KateView::lowercase( )
{
  m_doc->transform( this, m_viewInternal->m_cursor, KateDocument::Lowercase );
}

void KateView::capitalize( )
{
  m_doc->transform( this, m_viewInternal->m_cursor, KateDocument::Capitalize );
}

void KateView::keyReturn( )
{
  m_viewInternal->doReturn();
}

void KateView::backspace( )
{
  m_viewInternal->doBackspace();
}

void KateView::deleteWordLeft( )
{
  m_viewInternal->doDeleteWordLeft();
}

void KateView::keyDelete( )
{
  m_viewInternal->doDelete();
}

void KateView::deleteWordRight( )
{
  m_viewInternal->doDeleteWordRight();
}

void KateView::transpose( )
{
  m_viewInternal->doTranspose();
}

void KateView::cursorLeft( )
{
  m_viewInternal->cursorLeft();
}

void KateView::shiftCursorLeft( )
{
  m_viewInternal->cursorLeft(true);
}

void KateView::cursorRight( )
{
  m_viewInternal->cursorRight();
}

void KateView::shiftCursorRight( )
{
  m_viewInternal->cursorRight(true);
}

void KateView::wordLeft( )
{
  m_viewInternal->wordLeft();
}

void KateView::shiftWordLeft( )
{
  m_viewInternal->wordLeft(true);
}

void KateView::wordRight( )
{
  m_viewInternal->wordRight();
}

void KateView::shiftWordRight( )
{
  m_viewInternal->wordRight(true);
}

void KateView::home( )
{
  m_viewInternal->home();
}

void KateView::shiftHome( )
{
  m_viewInternal->home(true);
}

void KateView::end( )
{
  m_viewInternal->end();
}

void KateView::shiftEnd( )
{
  m_viewInternal->end(true);
}

void KateView::up( )
{
  m_viewInternal->cursorUp();
}

void KateView::shiftUp( )
{
  m_viewInternal->cursorUp(true);
}

void KateView::down( )
{
  m_viewInternal->cursorDown();
}

void KateView::shiftDown( )
{
  m_viewInternal->cursorDown(true);
}

void KateView::scrollUp( )
{
  m_viewInternal->scrollUp();
}

void KateView::scrollDown( )
{
  m_viewInternal->scrollDown();
}

void KateView::topOfView( )
{
  m_viewInternal->topOfView();
}

void KateView::shiftTopOfView( )
{
  m_viewInternal->topOfView(true);
}

void KateView::bottomOfView( )
{
  m_viewInternal->bottomOfView();
}

void KateView::shiftBottomOfView( )
{
  m_viewInternal->bottomOfView(true);
}

void KateView::pageUp( )
{
  m_viewInternal->pageUp();
}

void KateView::shiftPageUp( )
{
  m_viewInternal->pageUp(true);
}

void KateView::pageDown( )
{
  m_viewInternal->pageDown();
}

void KateView::shiftPageDown( )
{
  m_viewInternal->pageDown(true);
}

void KateView::top( )
{
  m_viewInternal->top_home();
}

void KateView::shiftTop( )
{
  m_viewInternal->top_home(true);
}

void KateView::bottom( )
{
  m_viewInternal->bottom_end();
}

void KateView::shiftBottom( )
{
  m_viewInternal->bottom_end(true);
}

void KateView::toMatchingBracket( )
{
  m_viewInternal->cursorToMatchingBracket();
}

void KateView::shiftToMatchingBracket( )
{
  m_viewInternal->cursorToMatchingBracket(true);
}

const KTextEditor::Range & KateView::selectionRange( ) const
{
  return *m_selection;
}

KTextEditor::Document * KateView::document( ) const
{
  return m_doc;
}

void KateView::setContextMenu( QMenu * menu )
{
  if (m_contextMenu)
    disconnect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));

  m_contextMenu = menu;

  if (m_contextMenu)
    connect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
}

QMenu * KateView::contextMenu( ) const
{
  return m_contextMenu;
}

QMenu * KateView::defaultContextMenu(QMenu* menu) const
{
  if (!menu)
    menu = new KMenu(const_cast<KateView*>(this));

  // Find top client
  KXMLGUIClient* client = const_cast<KateView*>(this);
  while (client->parentClient())
    client = client->parentClient();

  QWidget* popupwidget = 0;
  if (client && client->factory() && (popupwidget = client->factory()->container("ktexteditor_popup", client))) {
    menu->addActions(popupwidget->actions());

  } else {
    kDebug() << k_funcinfo << "no ktexteditor_popup container found; populating manually" << endl;
    menu->addAction(m_editUndo);
    menu->addAction(m_editRedo);
    menu->addSeparator();
    menu->addAction(m_cut);
    menu->addAction(m_copy);
    menu->addAction(m_paste);
    menu->addSeparator();
    menu->addAction(m_selectAll);
    menu->addAction(m_deSelect);
    if (QAction* bookmark = actionCollection()->action("bookmarks")) {
      menu->addSeparator();
      menu->addAction(bookmark);
    }
  }

  return menu;
}

void KateView::aboutToShowContextMenu( )
{
  QMenu* menu = qobject_cast<QMenu*>(sender());
  if (menu)
    emit contextMenuAboutToShow(this, menu);
}

//END Code completion new

// BEGIN ConfigInterface stuff
QStringList KateView::configKeys() const
{
  return QStringList() << "icon-bar" << "line-numbers" << "dynamic-word-wrap";
}

QVariant KateView::configValue(const QString &key)
{
  if (key == "icon-bar")
    return config()->iconBar();
  else if (key == "line-numbers")
    return config()->lineNumbers();
  else if (key == "dynamic-word-wrap")
    return config()->dynWordWrap();

  // return invalid variant
  return QVariant();
}

void KateView::setConfigValue(const QString &key, const QVariant &value)
{
  // We can only get away with this right now because there are no
  // non-bool functions here.. change this later if you are adding
  // a config option which uses variables other than bools..
  bool toggle = value.toBool();

  if (key == "icon-bar")
    config()->setIconBar(toggle);
  else if (key == "line-numbers")
    config()->setLineNumbers(toggle);
  else if (key == "dynamic-word-wrap")
    config()->setDynWordWrap(toggle);
}

// END ConfigInterface

// kate: space-indent on; indent-width 2; replace-tabs on;

void KateView::userInvokedCompletion()
{
  completionWidget()->userInvokedCompletion();
}
