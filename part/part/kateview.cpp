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

#define DEBUGACCELS

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
#include "katecodecompletion.h"
#include "katesearch.h"
#include "kateschema.h"
#include "katebookmarks.h"
#include "katesearch.h"
#include "kateconfig.h"
#include "katefiletype.h"
#include "kateautoindent.h"
#include "katearbitraryhighlight.h"
#include "katespell.h"

#include <ktexteditor/plugin.h>

#include <kparts/event.h>

#include <kio/netaccess.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kcursor.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kxmlguifactory.h>
#include <kaccel.h>
#include <klibloader.h>
#include <kencodingfiledialog.h>
#include <ktempfile.h>
#include <ksavefile.h>

#include <qfont.h>
#include <qfileinfo.h>
#include <qstyle.h>
#include <qevent.h>
#include <q3popupmenu.h>
#include <qlayout.h>
#include <qclipboard.h>
#include <qtextdocument.h>
#include <qtextstream.h>
#include <qmimedata.h>
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
    , m_customComplete(false)
    , m_cc_cleanup(false)
    , m_delayed_cc_type(KTextEditor::CompletionNone)
    , m_delayed_cc_provider(0)
    , m_editActions (0)
    , m_doc( doc )
    , m_search( new KateSearch( this ) )
    , m_spell( new KateSpell( this ) )
    , m_bookmarks( new KateBookmarks( this ) )
    , m_cmdLine (0)
    , m_cmdLineOn (false)
    , m_hasWrap( false )
    , m_startingUp (true)
    , m_updatingDocumentConfig (false)
    , m_selection(m_doc)
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
  m_vBox->addLayout (hbox);
  hbox->setMargin (0);
  hbox->setSpacing (0);

  hbox->addWidget (m_viewInternal->m_leftBorder);

  QVBoxLayout *vbox = new QVBoxLayout ();
  hbox->addLayout (vbox);
  vbox->setMargin (0);
  vbox->setSpacing (0);

  vbox->addWidget (m_viewInternal);
  vbox->addWidget (m_viewInternal->m_columnScroll);

  vbox = new QVBoxLayout ();
  hbox->addLayout (vbox);
  vbox->setMargin (0);
  vbox->setSpacing (0);

  vbox->addWidget (m_viewInternal->m_lineScroll);
  vbox->addWidget (m_viewInternal->m_dummy);

  // this really is needed :)
  m_viewInternal->updateView ();

  // FIXME design + update to new api
  //connect(&m_viewInternal->m_cursor, SIGNAL(positionChanged()), SLOT(slotCaretPositionChanged()));
  //connect(&m_viewInternal->m_mouse, SIGNAL(positionChanged()), SLOT(slotMousePositionChanged()));

  setInstance( KateGlobal::self()->instance() );
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
  setupCodeCompletion();

  // enable the plugins of this view
  m_doc->enableAllPluginsGUI (this);

  // update the enabled state of the undo/redo actions...
  slotNewUndo();

  m_startingUp = false;
  updateConfig ();

  slotHlChanged();
  /*test texthint
  connect(this,SIGNAL(needTextHint(int, int, QString &)),
  this,SLOT(slotNeedTextHint(int, int, QString &)));
  enableTextHints(1000);
  test texthint*/
}

KateView::~KateView()
{
  m_destructing=true;
  if (!m_doc->singleViewMode())
    m_doc->disableAllPluginsGUI (this);

  m_doc->removeView( this );

  delete m_viewInternal;
  delete m_codeCompletion;

  delete m_renderer;

  delete m_config;
  KateGlobal::self()->deregisterView (this);
}

void KateView::setupConnections()
{
  connect( m_doc, SIGNAL(undoChanged()),
           this, SLOT(slotNewUndo()) );
  connect( m_doc, SIGNAL(hlChanged()),
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
  KAction *a;

  m_toggleWriteLock = 0;

  m_cut = a=KStdAction::cut(this, SLOT(cut()), ac);
  a->setWhatsThis(i18n("Cut the selected text and move it to the clipboard"));

  m_paste = a=KStdAction::pasteText(this, SLOT(paste()), ac);
  a->setWhatsThis(i18n("Paste previously copied or cut clipboard contents"));

  m_copy = a=KStdAction::copy(this, SLOT(copy()), ac);
  a->setWhatsThis(i18n( "Use this command to copy the currently selected text to the system clipboard."));

  m_copyHTML = a = new KAction(i18n("Copy as &HTML"), "editcopy", 0, this, SLOT(copyHTML()), ac, "edit_copy_html");
  a->setWhatsThis(i18n( "Use this command to copy the currently selected text as HTML to the system clipboard."));

  if (!m_doc->readOnly())
  {
    a=KStdAction::save(m_doc, SLOT(documentSave()), ac);
    a->setWhatsThis(i18n("Save the current document"));

    a=m_editUndo = KStdAction::undo(m_doc, SLOT(undo()), ac);
    a->setWhatsThis(i18n("Revert the most recent editing actions"));

    a=m_editRedo = KStdAction::redo(m_doc, SLOT(redo()), ac);
    a->setWhatsThis(i18n("Revert the most recent undo operation"));

    (new KAction(i18n("&Word Wrap Document"), "", 0, m_doc, SLOT(applyWordWrap()), ac, "tools_apply_wordwrap"))->setWhatsThis(
  i18n("Use this command to wrap all lines of the current document which are longer than the width of the"
    " current view, to fit into this view.<br><br> This is a static word wrap, meaning it is not updated"
    " when the view is resized."));

    // setup Tools menu
    a=new KAction(i18n("&Indent"), "indent", Qt::CTRL+Qt::Key_I, this, SLOT(indent()), ac, "tools_indent");
    a->setWhatsThis(i18n("Use this to indent a selected block of text.<br><br>"
        "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
    a=new KAction(i18n("&Unindent"), "unindent", Qt::CTRL+Qt::SHIFT+Qt::Key_I, this, SLOT(unIndent()), ac, "tools_unindent");
    a->setWhatsThis(i18n("Use this to unindent a selected block of text."));

    a=new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()), ac, "tools_cleanIndent");
    a->setWhatsThis(i18n("Use this to clean the indentation of a selected block of text (only tabs/only spaces)<br><br>"
        "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));

    a=new KAction(i18n("&Align"), 0, this, SLOT(align()), ac, "tools_align");
    a->setWhatsThis(i18n("Use this to align the current line or block of text to its proper indent level."));

    a=new KAction(i18n("C&omment"), Qt::CTRL+Qt::Key_D, this, SLOT(comment()),
        ac, "tools_comment");
    a->setWhatsThis(i18n("This command comments out the current line or a selected block of text.<BR><BR>"
        "The characters for single/multiple line comments are defined within the language's highlighting."));

    a=new KAction(i18n("Unco&mment"), Qt::CTRL+Qt::SHIFT+Qt::Key_D, this, SLOT(uncomment()),
                                 ac, "tools_uncomment");
    a->setWhatsThis(i18n("This command removes comments from the current line or a selected block of text.<BR><BR>"
    "The characters for single/multiple line comments are defined within the language's highlighting."));
    a = m_toggleWriteLock = new KToggleAction(
                i18n("&Read Only Mode"), 0, 0,
                this, SLOT( toggleWriteLock() ),
                ac, "tools_toggle_write_lock" );
    a->setWhatsThis( i18n("Lock/unlock the document for writing") );

    a = new KAction( i18n("Uppercase"), Qt::CTRL + Qt::Key_U, this,
      SLOT(uppercase()), ac, "tools_uppercase" );
    a->setWhatsThis( i18n("Convert the selection to uppercase, or the character to the "
      "right of the cursor if no text is selected.") );

    a = new KAction( i18n("Lowercase"), Qt::CTRL + Qt::SHIFT + Qt::Key_U, this,
      SLOT(lowercase()), ac, "tools_lowercase" );
    a->setWhatsThis( i18n("Convert the selection to lowercase, or the character to the "
      "right of the cursor if no text is selected.") );

    a = new KAction( i18n("Capitalize"), Qt::CTRL + Qt::ALT + Qt::Key_U, this,
      SLOT(capitalize()), ac, "tools_capitalize" );
    a->setWhatsThis( i18n("Capitalize the selection, or the word under the "
      "cursor if no text is selected.") );

    a = new KAction( i18n("Join Lines"), Qt::CTRL + Qt::Key_J, this,
      SLOT( joinLines() ), ac, "tools_join_lines" );
  }
  else
  {
    m_cut->setEnabled (false);
    m_paste->setEnabled (false);
    m_editUndo = 0;
    m_editRedo = 0;
  }

  a=KStdAction::print( m_doc, SLOT(print()), ac );
  a->setWhatsThis(i18n("Print the current document."));

  a=new KAction(i18n("Reloa&d"), "reload", KStdAccel::reload(), this, SLOT(reloadFile()), ac, "file_reload");
  a->setWhatsThis(i18n("Reload the current document from disk."));

  a=KStdAction::saveAs(m_doc, SLOT(documentSaveAs()), ac);
  a->setWhatsThis(i18n("Save the current document to disk, with a name of your choice."));

  a=KStdAction::gotoLine(this, SLOT(gotoLine()), ac);
  a->setWhatsThis(i18n("This command opens a dialog and lets you choose a line that you want the cursor to move to."));

  a=new KAction(i18n("&Configure Editor..."), 0, this, SLOT(slotConfigDialog()),ac, "set_confdlg");
  a->setWhatsThis(i18n("Configure various aspects of this editor."));

  KateViewHighlightAction *menu = new KateViewHighlightAction (i18n("&Highlighting"), ac, "set_highlight");
  menu->setWhatsThis(i18n("Here you can choose how the current document should be highlighted."));
  menu->updateMenu (m_doc);

  KateViewFileTypeAction *ftm = new KateViewFileTypeAction (i18n("&Filetype"),ac,"set_filetype");
  ftm->updateMenu (m_doc);

  KateViewSchemaAction *schemaMenu = new KateViewSchemaAction (i18n("&Schema"),ac,"view_schemas");
  schemaMenu->updateMenu (this);

  // indentation menu
  new KateViewIndentationAction (m_doc, i18n("&Indentation"),ac,"tools_indentation");

  // html export
  a = new KAction(i18n("E&xport as HTML..."), 0, 0, this, SLOT(exportAsHTML()), ac, "file_export_html");
  a->setWhatsThis(i18n("This command allows you to export the current document"
                      " with all highlighting information into a HTML document."));

  m_selectAll = a=KStdAction::selectAll(this, SLOT(selectAll()), ac);
  a->setWhatsThis(i18n("Select the entire text of the current document."));

  m_deSelect = a=KStdAction::deselect(this, SLOT(clearSelection()), ac);
  a->setWhatsThis(i18n("If you have selected something within the current document, this will no longer be selected."));

  a=new KAction(i18n("Enlarge Font"), "viewmag+", 0, m_viewInternal, SLOT(slotIncFontSizes()), ac, "incFontSizes");
  a->setWhatsThis(i18n("This increases the display font size."));

  a=new KAction(i18n("Shrink Font"), "viewmag-", 0, m_viewInternal, SLOT(slotDecFontSizes()), ac, "decFontSizes");
  a->setWhatsThis(i18n("This decreases the display font size."));

  a= m_toggleBlockSelection = new KToggleAction(
    i18n("Bl&ock Selection Mode"), Qt::CTRL + Qt::SHIFT + Qt::Key_B,
    this, SLOT(toggleBlockSelectionMode()),
    ac, "set_verticalSelect");
  a->setWhatsThis(i18n("This command allows switching between the normal (line based) selection mode and the block selection mode."));

  a= m_toggleInsert = new KToggleAction(
    i18n("Overwr&ite Mode"), Qt::Key_Insert,
    this, SLOT(toggleInsert()),
    ac, "set_insert" );
  a->setWhatsThis(i18n("Choose whether you want the text you type to be inserted or to overwrite existing text."));

  KToggleAction *toggleAction;
   a= m_toggleDynWrap = toggleAction = new KToggleAction(
    i18n("&Dynamic Word Wrap"), Qt::Key_F10,
    this, SLOT(toggleDynWordWrap()),
    ac, "view_dynamic_word_wrap" );
  a->setWhatsThis(i18n("If this option is checked, the text lines will be wrapped at the view border on the screen."));

  a= m_setDynWrapIndicators = new KSelectAction(i18n("Dynamic Word Wrap Indicators"), 0, ac, "dynamic_word_wrap_indicators");
  a->setWhatsThis(i18n("Choose when the Dynamic Word Wrap Indicators should be displayed"));

  connect(m_setDynWrapIndicators, SIGNAL(activated(int)), this, SLOT(setDynWrapIndicators(int)));
  QStringList list2;
  list2.append(i18n("&Off"));
  list2.append(i18n("Follow &Line Numbers"));
  list2.append(i18n("&Always On"));
  m_setDynWrapIndicators->setItems(list2);

  a= toggleAction=m_toggleFoldingMarkers = new KToggleAction(
    i18n("Show Folding &Markers"), Qt::Key_F9,
    this, SLOT(toggleFoldingMarkers()),
    ac, "view_folding_markers" );
  a->setWhatsThis(i18n("You can choose if the codefolding marks should be shown, if codefolding is possible."));
  toggleAction->setCheckedState(i18n("Hide Folding &Markers"));

   a= m_toggleIconBar = toggleAction = new KToggleAction(
    i18n("Show &Icon Border"), Qt::Key_F6,
    this, SLOT(toggleIconBorder()),
    ac, "view_border");
  a=toggleAction;
  a->setWhatsThis(i18n("Show/hide the icon border.<BR><BR> The icon border shows bookmark symbols, for instance."));
  toggleAction->setCheckedState(i18n("Hide &Icon Border"));

  a= toggleAction=m_toggleLineNumbers = new KToggleAction(
     i18n("Show &Line Numbers"), Qt::Key_F11,
     this, SLOT(toggleLineNumbersOn()),
     ac, "view_line_numbers" );
  a->setWhatsThis(i18n("Show/hide the line numbers on the left hand side of the view."));
  toggleAction->setCheckedState(i18n("Hide &Line Numbers"));

  a= m_toggleScrollBarMarks = toggleAction = new KToggleAction(
     i18n("Show Scroll&bar Marks"), 0,
     this, SLOT(toggleScrollBarMarks()),
     ac, "view_scrollbar_marks");
  a->setWhatsThis(i18n("Show/hide the marks on the vertical scrollbar.<BR><BR>The marks, for instance, show bookmarks."));
  toggleAction->setCheckedState(i18n("Hide Scroll&bar Marks"));

  a = toggleAction = m_toggleWWMarker = new KToggleAction(
        i18n("Show Static &Word Wrap Marker"), 0,
        this, SLOT( toggleWWMarker() ),
        ac, "view_word_wrap_marker" );
  a->setWhatsThis( i18n(
        "Show/hide the Word Wrap Marker, a vertical line drawn at the word "
        "wrap column as defined in the editing properties" ));
  toggleAction->setCheckedState(i18n("Hide Static &Word Wrap Marker"));

  a= m_switchCmdLine = new KAction(
     i18n("Switch to Command Line"), Qt::Key_F7,
     this, SLOT(switchToCmdLine()),
     ac, "switch_to_cmd_line" );
  a->setWhatsThis(i18n("Show/hide the command line on the bottom of the view."));

  a=m_setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, ac, "set_eol");
  a->setWhatsThis(i18n("Choose which line endings should be used, when you save the document"));
  QStringList list;
  list.append("&UNIX");
  list.append("&Windows/DOS");
  list.append("&Macintosh");
  m_setEndOfLine->setItems(list);
  m_setEndOfLine->setCurrentItem (m_doc->config()->eol());
  connect(m_setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));

  // encoding menu
  new KateViewEncodingAction (m_doc, this, i18n("E&ncoding"), ac, "set_encoding");

  m_search->createActions( ac );
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
  m_editActions = new KActionCollection( m_viewInternal, this );
  m_editActions->setObjectName( "edit_actions" );
  KActionCollection* ac = m_editActions;

  new KAction(
    i18n("Move Word Left"),                         Qt::CTRL + Qt::Key_Left,
    this,SLOT(wordLeft()),
    ac, "word_left" );
  new KAction(
    i18n("Select Character Left"),          Qt::SHIFT +        Qt::Key_Left,
    this,SLOT(shiftCursorLeft()),
    ac, "select_char_left" );
  new KAction(
    i18n("Select Word Left"),               Qt::SHIFT + Qt::CTRL + Qt::Key_Left,
    this, SLOT(shiftWordLeft()),
    ac, "select_word_left" );

  new KAction(
    i18n("Move Word Right"),                        Qt::CTRL + Qt::Key_Right,
    this, SLOT(wordRight()),
    ac, "word_right" );
  new KAction(
    i18n("Select Character Right"),         Qt::SHIFT        + Qt::Key_Right,
    this, SLOT(shiftCursorRight()),
    ac, "select_char_right" );
  new KAction(
    i18n("Select Word Right"),              Qt::SHIFT + Qt::CTRL + Qt::Key_Right,
    this,SLOT(shiftWordRight()),
    ac, "select_word_right" );

  new KAction(
    i18n("Move to Beginning of Line"),                      Qt::Key_Home,
    this, SLOT(home()),
    ac, "beginning_of_line" );
  new KAction(
    i18n("Move to Beginning of Document"),           KStdAccel::home(),
    this, SLOT(top()),
    ac, "beginning_of_document" );
  new KAction(
    i18n("Select to Beginning of Line"),     Qt::SHIFT +        Qt::Key_Home,
    this, SLOT(shiftHome()),
    ac, "select_beginning_of_line" );
  new KAction(
    i18n("Select to Beginning of Document"), Qt::SHIFT + Qt::CTRL + Qt::Key_Home,
    this, SLOT(shiftTop()),
    ac, "select_beginning_of_document" );

  new KAction(
    i18n("Move to End of Line"),                            Qt::Key_End,
    this, SLOT(end()),
    ac, "end_of_line" );
  new KAction(
    i18n("Move to End of Document"),                 KStdAccel::end(),
    this, SLOT(bottom()),
    ac, "end_of_document" );
  new KAction(
    i18n("Select to End of Line"),           Qt::SHIFT +        Qt::Key_End,
    this, SLOT(shiftEnd()),
    ac, "select_end_of_line" );
  new KAction(
    i18n("Select to End of Document"),       Qt::SHIFT + Qt::CTRL + Qt::Key_End,
    this, SLOT(shiftBottom()),
    ac, "select_end_of_document" );

  new KAction(
    i18n("Select to Previous Line"),                Qt::SHIFT + Qt::Key_Up,
    this, SLOT(shiftUp()),
    ac, "select_line_up" );
  new KAction(
    i18n("Scroll Line Up"),"",              Qt::CTRL +          Qt::Key_Up,
    this, SLOT(scrollUp()),
    ac, "scroll_line_up" );

  new KAction(i18n("Move to Next Line"), Qt::Key_Down, this, SLOT(down()),
	      ac, "move_line_down");

  new KAction(i18n("Move to Previous Line"), Qt::Key_Up, this, SLOT(up()),
	      ac, "move_line_up");

  new KAction(i18n("Move Character Right"), Qt::Key_Right, this,
	      SLOT(cursorRight()), ac, "move_cursor_right");

  new KAction(i18n("Move Character Left"), Qt::Key_Left, this, SLOT(cursorLeft()),
	      ac, "move_cusor_left");

  new KAction(
    i18n("Select to Next Line"),                    Qt::SHIFT + Qt::Key_Down,
    this, SLOT(shiftDown()),
    ac, "select_line_down" );
  new KAction(
    i18n("Scroll Line Down"),               Qt::CTRL +          Qt::Key_Down,
    this, SLOT(scrollDown()),
    ac, "scroll_line_down" );

  new KAction(
    i18n("Scroll Page Up"),                         KStdAccel::prior(),
    this, SLOT(pageUp()),
    ac, "scroll_page_up" );
  new KAction(
    i18n("Select Page Up"),                         Qt::SHIFT + Qt::Key_PageUp,
    this, SLOT(shiftPageUp()),
    ac, "select_page_up" );
  new KAction(
    i18n("Move to Top of View"),             Qt::CTRL +         Qt::Key_PageUp,
    this, SLOT(topOfView()),
    ac, "move_top_of_view" );
  new KAction(
    i18n("Select to Top of View"),             Qt::CTRL + Qt::SHIFT +  Qt::Key_PageUp,
    this, SLOT(shiftTopOfView()),
    ac, "select_top_of_view" );

  new KAction(
    i18n("Scroll Page Down"),                          KStdAccel::next(),
    this, SLOT(pageDown()),
    ac, "scroll_page_down" );
  new KAction(
    i18n("Select Page Down"),                       Qt::SHIFT + Qt::Key_PageDown,
    this, SLOT(shiftPageDown()),
    ac, "select_page_down" );
  new KAction(
    i18n("Move to Bottom of View"),          Qt::CTRL +         Qt::Key_PageDown,
    this, SLOT(bottomOfView()),
    ac, "move_bottom_of_view" );
  new KAction(
    i18n("Select to Bottom of View"),         Qt::CTRL + Qt::SHIFT + Qt::Key_PageDown,
    this, SLOT(shiftBottomOfView()),
    ac, "select_bottom_of_view" );
  new KAction(
    i18n("Move to Matching Bracket"),               Qt::CTRL + Qt::Key_6,
    this, SLOT(toMatchingBracket()),
    ac, "to_matching_bracket" );
  new KAction(
    i18n("Select to Matching Bracket"),      Qt::SHIFT + Qt::CTRL + Qt::Key_6,
    this, SLOT(shiftToMatchingBracket()),
    ac, "select_matching_bracket" );

  // anders: shortcuts doing any changes should not be created in browserextension
  if ( !m_doc->readOnly() )
  {
    new KAction(
      i18n("Transpose Characters"),           Qt::CTRL + Qt::Key_T,
      this, SLOT(transpose()),
      ac, "transpose_char" );

    new KAction(
      i18n("Delete Line"),                    Qt::CTRL + Qt::Key_K,
      this, SLOT(killLine()),
      ac, "delete_line" );

    new KAction(
      i18n("Delete Word Left"),               KStdAccel::deleteWordBack(),
      this, SLOT(deleteWordLeft()),
      ac, "delete_word_left" );

    new KAction(
      i18n("Delete Word Right"),              KStdAccel::deleteWordForward(),
      this, SLOT(deleteWordRight()),
      ac, "delete_word_right" );

    new KAction(i18n("Delete Next Character"), Qt::Key_Delete,
                this, SLOT(keyDelete()),
                ac, "delete_next_character");

    new KAction(i18n("Backspace"), Qt::Key_Backspace,
                this, SLOT(backspace()),
                ac, "backspace");
  }

  m_editActions->readShortcutSettings( "Katepart Shortcuts" );

  if( hasFocus() )
    slotGotFocus();
  else
    slotLostFocus();
}

void KateView::setupCodeFolding()
{
  KActionCollection *ac=this->actionCollection();
  new KAction( i18n("Collapse Toplevel"), Qt::CTRL+Qt::SHIFT+Qt::Key_Minus,
       m_doc->foldingTree(),SLOT(collapseToplevelNodes()),ac,"folding_toplevel");
  new KAction( i18n("Expand Toplevel"), Qt::CTRL+Qt::SHIFT+Qt::Key_Plus,
       this,SLOT(slotExpandToplevel()),ac,"folding_expandtoplevel");
  new KAction( i18n("Collapse One Local Level"), Qt::CTRL+Qt::Key_Minus,
       this,SLOT(slotCollapseLocal()),ac,"folding_collapselocal");
  new KAction( i18n("Expand One Local Level"), Qt::CTRL+Qt::Key_Plus,
       this,SLOT(slotExpandLocal()),ac,"folding_expandlocal");

#ifdef DEBUGACCELS
  KAccel* debugAccels = new KAccel(this,this);
  debugAccels->insert("KATE_DUMP_REGION_TREE",i18n("Show the code folding region tree"),"","Ctrl+Shift+Alt+D",m_doc,SLOT(dumpRegionTree()));
  debugAccels->insert("KATE_TEMPLATE_TEST",i18n("Basic template code test"),"","Ctrl+Shift+Alt+T",m_doc,SLOT(testTemplateCode()));
  debugAccels->setEnabled(true);
#endif
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
    setCursorPositionInternal(realLine, cursorColumn(), m_doc->config()->tabWidth(), false);
}

void KateView::slotExpandLocal()
{
  m_doc->foldingTree()->expandOne(cursorPosition().line(), m_doc->lines());
}

void KateView::setupCodeCompletion()
{
  m_codeCompletion = new KateCodeCompletion(this);
  connect( m_codeCompletion, SIGNAL(completionAborted()),
           this,             SIGNAL(completionAborted()));
  connect( m_codeCompletion, SIGNAL(completionDone()),
           this,             SIGNAL(completionDone()));
  connect( m_codeCompletion, SIGNAL(argHintHidden()),
           this,             SIGNAL(argHintHidden()));
  connect( m_codeCompletion, SIGNAL(completionDone(KTextEditor::CompletionEntry)),
           this,             SIGNAL(completionDone(KTextEditor::CompletionEntry)));
  connect( m_codeCompletion, SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString*)),
           this,             SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString*)));
}

QString KateView::viewMode () const
{
  if (!m_doc->isReadWrite())
    return i18n ("R/O");

  return isOverwriteMode() ? i18n("OVR") : i18n ("NORM");
}

void KateView::slotGotFocus()
{
  if (m_editActions)
    m_editActions->accel()->setEnabled( true );

  emit focusIn ( this );
}

void KateView::slotLostFocus()
{
  if (m_editActions)
    m_editActions->accel()->setEnabled( false );

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
  int cl = cursorPosition().line();
  int cc = cursorColumn();

  // save bookmarks
  m_doc->documentReload();

  if (m_doc->lines() >= cl)
    // Explicitly call internal function because we want this to be registered as a non-external call
    setCursorPositionInternal( cl, cc, m_doc->config()->tabWidth(), false );
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

  KAction *a = 0;
  for (int z = 0; z < l.size(); z++)
    if ((a = actionCollection()->action( l[z].ascii() )))
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
  const KURL::List lstDragURLs=KURL::List::fromMimeData(ev->mimeData());
  bool ok = !lstDragURLs.isEmpty();

  KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( doc() );
  if ( ok && ext )
    emit ext->openURLRequest( lstDragURLs.first() );
}

void KateView::contextMenuEvent( QContextMenuEvent *ev )
{
  if ( !m_doc || !m_doc->browserExtension()  )
    return;
  emit m_doc->browserExtension()->popupMenu( ev->globalPos(), m_doc->url(),
                                        QLatin1String( "text/plain" ) );
  ev->accept();
}

bool KateView::setCursorPositionInternal( uint line, uint col, uint tabwidth, bool calledExternally )
{
  KateTextLine::Ptr l = m_doc->kateTextLine( line );

  if (!l)
    return false;

  QString line_str = m_doc->line( line );

  int x = 0;
  for (int z = 0; z < line_str.length() && (uint)z < col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  m_viewInternal->updateCursor( KTextEditor::Cursor( line, x ), false, true, calledExternally );

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
  if ( !error.isEmpty() ) // happens when cancelling a job
    KMessageBox::error( this, error );
}

void KateView::gotoLine()
{
  KateGotoLineDialog *dlg = new KateGotoLineDialog (this, m_viewInternal->getCursor().line() + 1, m_doc->lines());

  if (dlg->exec() == QDialog::Accepted)
    setCursorPositionInternal( dlg->getLine() - 1, 0 );

  delete dlg;
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

void KateView::readSessionConfig(KConfig *config)
{
  setCursorPositionInternal (config->readNumEntry("CursorLine"), config->readNumEntry("CursorColumn"), 1);
}

void KateView::writeSessionConfig(KConfig *config)
{
  config->writeEntry("CursorLine",m_viewInternal->m_cursor.line());
  config->writeEntry("CursorColumn",m_viewInternal->m_cursor.column());
}

int KateView::getEol()
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
  m_search->find();
}

void KateView::find( const QString& pattern, long flags, bool add )
{
  m_search->find( pattern, flags, add );
}

void KateView::replace()
{
  m_search->replace();
}

void KateView::replace( const QString &pattern, const QString &replacement, long flags )
{
  m_search->replace( pattern, replacement, flags );
}

void KateView::findAgain( bool back )
{
  m_search->findAgain( back );
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


#if 0
void KateView::showArgHint( QStringList arg1, const QString& arg2, const QString& arg3 )
{
  m_codeCompletion->showArgHint( arg1, arg2, arg3 );
}

void KateView::showCompletionBox( Q3ValueList<KTextEditor::CompletionEntry> arg1, int offset, bool cs )
{
  emit aboutToShowCompletionBox();
  m_codeCompletion->showCompletionBox( arg1, offset, cs );
}
#endif

KateRenderer *KateView::renderer ()
{
  return m_renderer;
}

void KateView::updateConfig ()
{
  if (m_startingUp)
    return;

  m_editActions->readShortcutSettings( "Katepart Shortcuts" );

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

  KAction *a = 0;
  for (int z = 0; z < l.size(); z++)
    if ((a = actionCollection()->action( l[z].ascii() )))
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

void KateView::clear ()
{
  m_viewInternal->clear ();
}

void KateView::repaintText (bool paintOnlyDirty)
{
  m_viewInternal->update ();
  //m_viewInternal->paintText(0,0,m_viewInternal->width(),m_viewInternal->height(), paintOnlyDirty);
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

int KateView::cursorColumn() const
{
  uint r = m_doc->currentColumn(m_viewInternal->getCursor());
  if ( !( m_doc->config()->configFlags() & KateDocumentConfig::cfWrapCursor ) &&
       m_viewInternal->getCursor().column() > m_doc->line( m_viewInternal->getCursor().line() ).length()  )
    r += m_viewInternal->getCursor().column() - m_doc->line( m_viewInternal->getCursor().line() ).length();

  return r;
}

void KateView::slotMousePositionChanged( )
{
  /* FIXME design + update to new api
  Q_ASSERT(sender() && sender()->inherits("KateSmartCursor"));
  KateSmartCursor* mousePosition = static_cast<KateSmartCursor*>(const_cast<QObject*>(sender()));
  emit mousePositionChanged(*mousePosition);*/
}

void KateView::slotCaretPositionChanged( )
{
  /* FIXME design + update to new api
  Q_ASSERT(sender() && sender()->inherits("KateSmartCursor"));
  KateSmartCursor* caretPosition = static_cast<KateSmartCursor*>(const_cast<QObject*>(sender()));
  emit caretPositionChanged(*caretPosition);*/
}

//BEGIN KTextEditor::SelectionInterface stuff

bool KateView::setSelection( const KTextEditor::Range &selection )
{
  KTextEditor::Range oldSelection = m_selection;
  m_selection = selection;

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

  KTextEditor::Range oldSelection = m_selection;

  m_selection = KTextEditor::Range::invalid();

  tagSelection(oldSelection);

  oldSelection = m_selection;

  if (redraw)
    repaintText(true);

  if (finishedChangingSelection)
    emit selectionChanged(this);

  return true;
}

bool KateView::selection() const
{
  return m_selection.start() != m_selection.end();
}

QString KateView::selectionText() const
{
  KTextEditor::Range range = m_selection;

  if ( blockSelect )
    blockFix(range);

  return m_doc->text(range, blockSelect);
}

bool KateView::removeSelectedText()
{
  if (!selection())
    return false;

  m_doc->editStart ();

  KTextEditor::Range range = m_selection;

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

  return setSelection(KTextEditor::Range(KTextEditor::Cursor(), m_doc->end()));
}

bool KateView::cursorSelected(const KTextEditor::Cursor& cursor)
{
  KTextEditor::Cursor ret = cursor;
  if ( (!blockSelect) && (ret.column() < 0) )
    ret.setColumn(0);

  if (blockSelect)
    return cursor.line() >= m_selection.start().line() && ret.line() <= m_selection.end().line() && ret.column() >= m_selection.start().column() && ret.column() < m_selection.end().column();
  else
    return m_selection.contains(cursor);
}

bool KateView::lineSelected (int line)
{
  return !blockSelect && m_selection.containsLine(line);
}

bool KateView::lineEndSelected (const KTextEditor::Cursor& lineEndPos)
{
  return (!blockSelect)
    && (lineEndPos.line() > m_selection.start().line() || (lineEndPos.line() == m_selection.start().line() && (m_selection.start().column() < lineEndPos.column() || lineEndPos.column() == -1)))
    && (lineEndPos.line() < m_selection.end().line() || (lineEndPos.line() == m_selection.end().line() && (lineEndPos.column() <= m_selection.end().column() && lineEndPos.column() != -1)));
}

bool KateView::lineHasSelected (int line)
{
  return selection() && m_selection.containsLine(line);
}

bool KateView::lineIsSelection (int line)
{
  return (line == m_selection.start().line() && line == m_selection.end().line());
}

void KateView::tagSelection(const KTextEditor::Range &oldSelection)
{
  if (selection()) {
    if (oldSelection.start().line() == -1) {
      // We have to tag the whole lot if
      // 1) we have a selection, and:
      //  a) it's new; or
      tagLines(m_selection, true);

    } else if (blockSelectionMode() && (oldSelection.start().column() != m_selection.start().column() || oldSelection.end().column() != m_selection.end().column())) {
      //  b) we're in block selection mode and the columns have changed
      tagLines(m_selection, true);
      tagLines(oldSelection, true);

    } else {
      if (oldSelection.start() != m_selection.start()) {
        if (oldSelection.start() < m_selection.start())
          tagLines(oldSelection.start(), m_selection.start(), true);
        else
          tagLines(m_selection.start(), oldSelection.start(), true);
      }

      if (oldSelection.end() != m_selection.end()) {
        if (oldSelection.end() < m_selection.end())
          tagLines(oldSelection.end(), m_selection.end(), true);
        else
          tagLines(m_selection.end(), oldSelection.end(), true);
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
  while (start > 0 && m_doc->highlight()->isInWord(textLine->getChar(start - 1), textLine->attribute(start - 1))) start--;
  while (end < len && m_doc->highlight()->isInWord(textLine->getChar(end), textLine->attribute(start - 1))) end++;
  if (end <= start) return;

  setSelection (KTextEditor::Cursor(cursor.line(), start), end-start);
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

void KateView::copyHTML()
{
  if (!selection())
    return;

  QMimeData *data=new QMimeData();
  data->setText(selectionText());
  data->setHtml(selectionAsHtml());
  QApplication::clipboard()->setMimeData(data);
}

QString KateView::selectionAsHtml()
{
  return textAsHtml(m_selection, blockSelect);
}

QString KateView::textAsHtml ( KTextEditor::Range range, bool blockwise)
{
  kdDebug(13020) << "textAsHtml" << endl;
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
  kdDebug(13020) << "html is: " << s << endl;
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
      KTextEditor::Attribute* charAttributes = 0;

      charAttributes = m_renderer->attribute(line->attribute(curPos));

      //ASSERT(charAttributes != NULL);
      // let's give the color for that character :
      if ( (charAttributes->textColor() != previousCharacterColor))
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
        charAttributes->textColor().getRgb(&red, &green, &blue);
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
      if ( (needToReinitializeTags && charAttributes->bold()) ||
          (!previousCharacterWasBold && charAttributes->bold()) )
        // we enter a bold section
        (*outputStream) << "<b>";
      if ( !needToReinitializeTags && (previousCharacterWasBold && !charAttributes->bold()) )
        // we leave a bold section
        (*outputStream) << "</b>";

      // italic status :
      if ( (needToReinitializeTags && charAttributes->italic()) ||
           (!previousCharacterWasItalic && charAttributes->italic()) )
        // we enter an italic section
        (*outputStream) << "<i>";
      if ( !needToReinitializeTags && (previousCharacterWasItalic && !charAttributes->italic()) )
        // we leave an italic section
        (*outputStream) << "</i>";

      // write the actual character :
      (*outputStream) << Qt::escape(QString(line->getChar(curPos)));

      // save status for the next character :
      previousCharacterWasItalic = charAttributes->italic();
      previousCharacterWasBold = charAttributes->bold();
      previousCharacterColor = charAttributes->textColor();
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
  KURL url = KFileDialog::getSaveURL(QString::null,"text/html",0,i18n("Export File as HTML"));

  if ( url.isEmpty() )
    return;

  QString filename;
  KTempFile tmp; // ### only used for network export

  if ( url.isLocalFile() )
    filename = url.path();
  else
    filename = tmp.name();

  KSaveFile *savefile=new KSaveFile(filename);
  if (!savefile->status())
  {
    QTextStream *outputStream = savefile->textStream();

    //outputStream->setEncoding(QTextStream::UnicodeUTF8);
    outputStream->setCodec(QTextCodec::codecForName("UTF-8"));
    // let's write the HTML header :
    (*outputStream) << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    (*outputStream) << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
    (*outputStream) << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
    (*outputStream) << "<head>" << endl;
    (*outputStream) << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />" << endl;
    (*outputStream) << "<meta name=\"Generator\" content=\"Kate, the KDE Advanced Text Editor\" />" << endl;
    // for the title, we write the name of the file (/usr/local/emmanuel/myfile.cpp -> myfile.cpp)
    (*outputStream) << "<title>" << m_doc->documentName () << "</title>" << endl;
    (*outputStream) << "</head>" << endl;
    (*outputStream) << "<body>" << endl;

    textAsHtmlStream(m_doc->all(), false, outputStream);

    (*outputStream) << "</body>" << endl;
    (*outputStream) << "</html>" << endl;


    savefile->close();
    //if (!savefile->status()) --> Error
  }
//     else
//       {/*ERROR*/}
  delete savefile;

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

    KTextEditor::Range oldSelection = m_selection;

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

bool KateView::isIMSelection( const KTextEditor::Cursor& pos )
{
  return m_imSelection.contains(pos);
}

bool KateView::isIMEdit( const KTextEditor::Cursor& pos )
{
  return m_imRange.contains(pos);
}

void KateView::getIMSelectionValue( KTextEditor::Range* imRange, KTextEditor::Range* imSelection )
{
  *imRange = m_imRange;
  *imSelection = m_imSelection;
}
//END IM INPUT STUFF

// merge the following two functions
void KateView::slotTextInserted ( KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text)
{
  emit textInserted ( view, position, text);
  if (m_customComplete) return;
  kdDebug(13030)<<"Checking if cc provider list is empty"<<endl;
  if (m_completionProviders.isEmpty()) return;
  QLinkedList<KTextEditor::CompletionData> newdata;

  KTextEditor::Cursor c=cursorPosition();
  QString lineText=m_doc->line(c.line());
  kdDebug(13030)<<"Checking state for all providers"<<endl;
  const KTextEditor::CompletionData nulldata=KTextEditor::CompletionData::Null();
  foreach (KTextEditor::CompletionProvider *provider, m_completionProviders)
  {
    const KTextEditor::CompletionData &nd=provider->completionData(view,KTextEditor::CompletionAsYouType,position,text,c,lineText);
    if (nd.isValid()) newdata.append(nd);
  }
  m_codeCompletion->showCompletion(position,newdata);
}


void KateView::invokeCompletion(enum KTextEditor::CompletionType type) {
  kdDebug(13020)<<"KateView::invokeCompletion"<<endl;
  if ((type==KTextEditor::CompletionAsYouType) || (type==KTextEditor::CompletionAsYouTypeBackspace))
  {
    kdDebug(13020)<<"KateView::invokeCompletion: ignoring invalid call"<<endl;
    return;
  }
  kdDebug(13020)<<"Before delay check"<<endl;
  if (m_cc_cleanup) {m_delayed_cc_type=type; m_delayed_cc_provider=0; return;}
  kdDebug(13020)<<"Before custom complete check"<<endl;
  if (m_customComplete) return;
  if (m_completionProviders.isEmpty()) return;
  kdDebug(13020)<<"About to iterate over provider list"<<endl;
  QLinkedList<KTextEditor::CompletionData> newdata;
  KTextEditor::Cursor c=cursorPosition();
  QString lineText=m_doc->line(c.line());
  foreach (KTextEditor::CompletionProvider *provider, m_completionProviders)
  {
    const KTextEditor::CompletionData& nd=provider->completionData(this,type,KTextEditor::Cursor(),"",c,lineText);
    if (nd.isValid()) newdata.append(nd);
  }
  m_codeCompletion->showCompletion(c,newdata);
  if (newdata.size()!=0)
  if (type>KTextEditor::CompletionReinvokeAsYouType) m_customComplete=true;
}

void KateView::invokeCompletion(KTextEditor::CompletionProvider* provider,enum KTextEditor::CompletionType type) {
  kdDebug(13020)<<"KateView::invokeCompletion"<<endl;
  if ((type==KTextEditor::CompletionAsYouType) || (type==KTextEditor::CompletionAsYouTypeBackspace))
  {
    kdDebug(13020)<<"KateView::invokeCompletion: ignoring invalid call"<<endl;
    return;
  }
  kdDebug(13020)<<"Before delay check"<<endl;
  if (m_cc_cleanup) {m_delayed_cc_type=type; m_delayed_cc_provider=provider; return;}
  kdDebug(13020)<<"Before custom complete check"<<endl;
  if (m_customComplete) return;
  if (m_completionProviders.isEmpty()) return;
  if (!m_completionProviders.contains(provider)) return;

  QLinkedList<KTextEditor::CompletionData> newdata;
  KTextEditor::Cursor c=cursorPosition();
  QString lineText=m_doc->line(c.line());

  const KTextEditor::CompletionData& nd=provider->completionData(this,type,KTextEditor::Cursor(),"",c,lineText);
  if (nd.isValid()) newdata.append(nd);
  m_codeCompletion->showCompletion(c,newdata);
  if (newdata.size()!=0)
    m_customComplete=true;
}


void KateView::completionDone(){
  kdDebug(13030)<<"KateView::completionDone"<<endl;
  m_customComplete=false;
  m_cc_cleanup=true;
  foreach (KTextEditor::CompletionProvider *provider, m_completionProviders)
    provider->completionDone(this);
  m_cc_cleanup=false;
  if (m_delayed_cc_type!=KTextEditor::CompletionNone) {
    kdDebug(13030)<<"delayed completion call"<<endl;
    enum KTextEditor::CompletionType t=m_delayed_cc_type;
    m_delayed_cc_type=KTextEditor::CompletionNone;
    if (m_delayed_cc_provider)
      invokeCompletion(m_delayed_cc_provider,t);
    else
      invokeCompletion(t);
    m_delayed_cc_provider=0;
  }
}
void KateView::completionAborted(){
  kdDebug(13030)<<"KateView::completionAborted"<<endl;
  m_customComplete=false;
  m_cc_cleanup=true;
  foreach (KTextEditor::CompletionProvider *provider, m_completionProviders)
    provider->completionAborted(this);
  m_cc_cleanup=false;
  if (m_delayed_cc_type!=KTextEditor::CompletionNone) {
    enum KTextEditor::CompletionType t=m_delayed_cc_type;
    m_delayed_cc_type=KTextEditor::CompletionNone;
    if (m_delayed_cc_provider)
      invokeCompletion(m_delayed_cc_provider,t);
    else
      invokeCompletion(t);
    m_delayed_cc_provider=0;
  }
}

bool KateView::insertTemplateTextImplementation ( const KTextEditor::Cursor& c, const QString &templateString, const QMap<QString,QString> &initialValues) {
  return m_doc->insertTemplateTextImplementation(c,templateString,initialValues,this);
}


//BEGIN Code completion new
bool KateView::registerCompletionProvider(KTextEditor::CompletionProvider* provider)
{
  kdDebug(13030)<<"Registering completion provider:"<<provider<<endl;
  if (!provider) return false;
  if (m_completionProviders.contains(provider)) return false;
  m_completionProviders.append(provider);
  return true;
}

bool KateView::unregisterCompletionProvider(KTextEditor::CompletionProvider* provider)
{
  kdDebug(13030)<<"Unregistering completion provider:"<<provider<<endl;
  if (!provider) return false;
  m_completionProviderData.remove(provider);
  return m_completionProviders.removeAll(provider);
}

bool KateView::tagLines( KTextEditor::Range range, bool realRange )
{
  return tagLines(range.start(), range.end(), realRange);
}


//END Code completion new

// kate: space-indent on; indent-width 2; replace-tabs on;
