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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

//BEGIN includes
#include "kateview.h"
#include "kateview.moc"

#include "kateviewinternal.h"
#include "kateviewhelpers.h"
#include "katerenderer.h"
#include "katedocument.h"
#include "katedocumenthelpers.h"
#include "katefactory.h"
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

#include <ktexteditor/plugin.h>

#include <kparts/event.h>

#include <kconfig.h>
#include <kurldrag.h>
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
#include <kmultipledrag.h>

#include <qfont.h>
#include <qfileinfo.h>
#include <qstyle.h>
#include <qevent.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qclipboard.h>
//END includes

KateView::KateView( KateDocument *doc, QWidget *parent, const char * name )
    : Kate::View( doc, parent, name )
    , m_doc( doc )
    , m_search( new KateSearch( this ) )
    , m_bookmarks( new KateBookmarks( this ) )
    , m_cmdLine (0)
    , m_cmdLineOn (false)
    , m_active( false )
    , m_hasWrap( false )
    , m_startingUp (true)
    , m_updatingDocumentConfig (false)
    , selectStart (m_doc, true)
    , selectEnd (m_doc, true)
    , blockSelect (false)
    , m_imStartLine( 0 )
    , m_imStart( 0 )
    , m_imEnd( 0 )
    , m_imSelStart( 0 )
    , m_imSelEnd( 0 )
    , m_imComposeEvent( false )
{
  KateFactory::self()->registerView( this );
  m_config = new KateViewConfig (this);

  m_renderer = new KateRenderer(doc, this);

  m_grid = new QGridLayout (this, 3, 3);

  m_grid->setRowStretch ( 0, 10 );
  m_grid->setRowStretch ( 1, 0 );
  m_grid->setColStretch ( 0, 0 );
  m_grid->setColStretch ( 1, 10 );
  m_grid->setColStretch ( 2, 0 );

  m_viewInternal = new KateViewInternal( this, doc );
  m_grid->addWidget (m_viewInternal, 0, 1);

  setClipboardInterfaceDCOPSuffix (viewDCOPSuffix());
  setCodeCompletionInterfaceDCOPSuffix (viewDCOPSuffix());
  setDynWordWrapInterfaceDCOPSuffix (viewDCOPSuffix());
  setPopupMenuInterfaceDCOPSuffix (viewDCOPSuffix());
  setSessionConfigInterfaceDCOPSuffix (viewDCOPSuffix());
  setViewCursorInterfaceDCOPSuffix (viewDCOPSuffix());
  setViewStatusMsgInterfaceDCOPSuffix (viewDCOPSuffix());

  setInstance( KateFactory::self()->instance() );
  doc->addView( this );

  setFocusProxy( m_viewInternal );
  setFocusPolicy( StrongFocus );

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

  m_viewInternal->show ();
  slotHlChanged();
  /*test texthint
  connect(this,SIGNAL(needTextHint(int, int, QString &)),
  this,SLOT(slotNeedTextHint(int, int, QString &)));
  enableTextHints(1000);
  test texthint*/
}

KateView::~KateView()
{
  if (!m_doc->singleViewMode())
    m_doc->disableAllPluginsGUI (this);

  m_doc->removeView( this );

  delete m_viewInternal;
  delete m_codeCompletion;

  delete m_renderer;

  delete m_config;
  KateFactory::self()->deregisterView (this);
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
  connect(this,SIGNAL(cursorPositionChanged()),this,SLOT(slotStatusMsg()));
  connect(this,SIGNAL(newStatus()),this,SLOT(slotStatusMsg()));
  connect(m_doc, SIGNAL(undoChanged()), this, SLOT(slotStatusMsg()));

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


  if (!m_doc->readOnly())
  {
    KStdAction::spelling( m_doc, SLOT(spellcheck()), ac );
    a = new KAction( i18n("Spelling (from cursor)..."), "spellcheck", 0, this, SLOT(spellcheckFromCursor()), ac, "tools_spelling_from_cursor" );
    a->setWhatsThis(i18n("Check the document's spelling from the cursor and forward"));

    m_spellcheckSelection = new KAction( i18n("Spellcheck Selection..."), "spellcheck", 0, this, SLOT(spellcheckSelection()), ac, "tools_spelling_selection" );
    m_spellcheckSelection->setWhatsThis(i18n("Check spelling of the selected text"));

    a=KStdAction::save(this, SLOT(save()), ac);
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

    a=new KAction(i18n("C&omment"), CTRL+Qt::Key_D, this, SLOT(comment()),
        ac, "tools_comment");
    a->setWhatsThis(i18n("This command comments out the current line or a selected block of text.<BR><BR>"
        "The characters for single/multiple line comments are defined within the language's highlighting."));

    a=new KAction(i18n("Unco&mment"), CTRL+SHIFT+Qt::Key_D, this, SLOT(uncomment()),
                                 ac, "tools_uncomment");
    a->setWhatsThis(i18n("This command removes comments from the current line or a selected block of text.<BR><BR>"
    "The characters for single/multiple line comments are defined within the language's highlighting."));
    a = m_toggleWriteLock = new KToggleAction(
                i18n("&Read Only Mode"), 0, 0,
                this, SLOT( toggleWriteLock() ),
                ac, "tools_toggle_write_lock" );
    a->setWhatsThis( i18n("Lock/unlock the document for writing") );

    a = new KAction( i18n("Uppercase"), CTRL + Qt::Key_U, this,
      SLOT(uppercase()), ac, "tools_uppercase" );
    a->setWhatsThis( i18n("Convert the selection to uppercase, or the character to the "
      "right of the cursor if no text is selected.") );

    a = new KAction( i18n("Lowercase"), CTRL + SHIFT + Qt::Key_U, this,
      SLOT(lowercase()), ac, "tools_lowercase" );
    a->setWhatsThis( i18n("Convert the selection to lowercase, or the character to the "
      "right of the cursor if no text is selected.") );

    a = new KAction( i18n("Capitalize"), CTRL + ALT + Qt::Key_U, this,
      SLOT(capitalize()), ac, "tools_capitalize" );
    a->setWhatsThis( i18n("Capitalize the selection, or the word under the "
      "cursor if no text is selected.") );

    a = new KAction( i18n("Join Lines"), CTRL + Qt::Key_J, this,
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

  a=KStdAction::saveAs(this, SLOT(saveAs()), ac);
  a->setWhatsThis(i18n("Save the current document to disk, with a name of your choice."));

  a=KStdAction::gotoLine(this, SLOT(gotoLine()), ac);
  a->setWhatsThis(i18n("This command opens a dialog and lets you choose a line that you want the cursor to move to."));

  a=new KAction(i18n("&Configure Editor..."), 0, m_doc, SLOT(configDialog()),ac, "set_confdlg");
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

  KateExportAction *emenu = new KateExportAction (i18n("E&xport"),ac,"file_export");
  emenu->updateMenu (m_doc);
  emenu->setWhatsThis(i18n("This command allows you to export the current document"
                      " with all highlighting information into a markup document, e.g. HTML."));

  m_selectAll = a=KStdAction::selectAll(this, SLOT(selectAll()), ac);
  a->setWhatsThis(i18n("Select the entire text of the current document."));

  m_deSelect = a=KStdAction::deselect(this, SLOT(clearSelection()), ac);
  a->setWhatsThis(i18n("If you have selected something within the current document, this will no longer be selected."));

  a=new KAction(i18n("Enlarge Font"), "viewmag+", 0, m_viewInternal, SLOT(slotIncFontSizes()), ac, "incFontSizes");
  a->setWhatsThis(i18n("This increases the display font size."));

  a=new KAction(i18n("Shrink Font"), "viewmag-", 0, m_viewInternal, SLOT(slotDecFontSizes()), ac, "decFontSizes");
  a->setWhatsThis(i18n("This decreases the display font size."));

  a= m_toggleBlockSelection = new KToggleAction(
    i18n("Bl&ock Selection Mode"), CTRL + SHIFT + Key_B,
    this, SLOT(toggleBlockSelectionMode()),
    ac, "set_verticalSelect");
  a->setWhatsThis(i18n("This command allows switching between the normal (line based) selection mode and the block selection mode."));

  a= m_toggleInsert = new KToggleAction(
    i18n("Overwr&ite Mode"), Key_Insert,
    this, SLOT(toggleInsert()),
    ac, "set_insert" );
  a->setWhatsThis(i18n("Choose whether you want the text you type to be inserted or to overwrite existing text."));

  KToggleAction *toggleAction;
   a= m_toggleDynWrap = toggleAction = new KToggleAction(
    i18n("&Dynamic Word Wrap"), Key_F10,
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
    i18n("Show Folding &Markers"), Key_F9,
    this, SLOT(toggleFoldingMarkers()),
    ac, "view_folding_markers" );
  a->setWhatsThis(i18n("You can choose if the codefolding marks should be shown, if codefolding is possible."));
  toggleAction->setCheckedState(i18n("Hide Folding &Markers"));

   a= m_toggleIconBar = toggleAction = new KToggleAction(
    i18n("Show &Icon Border"), Key_F6,
    this, SLOT(toggleIconBorder()),
    ac, "view_border");
  a=toggleAction;
  a->setWhatsThis(i18n("Show/hide the icon border.<BR><BR> The icon border shows bookmark symbols, for instance."));
  toggleAction->setCheckedState(i18n("Hide &Icon Border"));

  a= toggleAction=m_toggleLineNumbers = new KToggleAction(
     i18n("Show &Line Numbers"), Key_F11,
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
     i18n("Switch to Command Line"), Key_F7,
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
  m_bookmarks->createActions( ac );

  slotSelectionChanged ();

  connect (this, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));
}

void KateView::setupEditActions()
{
  m_editActions = new KActionCollection( m_viewInternal, this, "edit_actions" );
  KActionCollection* ac = m_editActions;

  new KAction(
    i18n("Move Word Left"),                         CTRL + Key_Left,
    this,SLOT(wordLeft()),
    ac, "word_left" );
  new KAction(
    i18n("Select Character Left"),          SHIFT +        Key_Left,
    this,SLOT(shiftCursorLeft()),
    ac, "select_char_left" );
  new KAction(
    i18n("Select Word Left"),               SHIFT + CTRL + Key_Left,
    this, SLOT(shiftWordLeft()),
    ac, "select_word_left" );

  new KAction(
    i18n("Move Word Right"),                        CTRL + Key_Right,
    this, SLOT(wordRight()),
    ac, "word_right" );
  new KAction(
    i18n("Select Character Right"),         SHIFT        + Key_Right,
    this, SLOT(shiftCursorRight()),
    ac, "select_char_right" );
  new KAction(
    i18n("Select Word Right"),              SHIFT + CTRL + Key_Right,
    this,SLOT(shiftWordRight()),
    ac, "select_word_right" );

  new KAction(
    i18n("Move to Beginning of Line"),                      Key_Home,
    this, SLOT(home()),
    ac, "beginning_of_line" );
  new KAction(
    i18n("Move to Beginning of Document"),           KStdAccel::home(),
    this, SLOT(top()),
    ac, "beginning_of_document" );
  new KAction(
    i18n("Select to Beginning of Line"),     SHIFT +        Key_Home,
    this, SLOT(shiftHome()),
    ac, "select_beginning_of_line" );
  new KAction(
    i18n("Select to Beginning of Document"), SHIFT + CTRL + Key_Home,
    this, SLOT(shiftTop()),
    ac, "select_beginning_of_document" );

  new KAction(
    i18n("Move to End of Line"),                            Key_End,
    this, SLOT(end()),
    ac, "end_of_line" );
  new KAction(
    i18n("Move to End of Document"),                 KStdAccel::end(),
    this, SLOT(bottom()),
    ac, "end_of_document" );
  new KAction(
    i18n("Select to End of Line"),           SHIFT +        Key_End,
    this, SLOT(shiftEnd()),
    ac, "select_end_of_line" );
  new KAction(
    i18n("Select to End of Document"),       SHIFT + CTRL + Key_End,
    this, SLOT(shiftBottom()),
    ac, "select_end_of_document" );

  new KAction(
    i18n("Select to Previous Line"),                SHIFT + Key_Up,
    this, SLOT(shiftUp()),
    ac, "select_line_up" );
  new KAction(
    i18n("Scroll Line Up"),"",              CTRL +          Key_Up,
    this, SLOT(scrollUp()),
    ac, "scroll_line_up" );

  new KAction(i18n("Move to Next Line"), Key_Down, this, SLOT(down()),
	      ac, "move_line_down");

  new KAction(i18n("Move to Previous Line"), Key_Up, this, SLOT(up()),
	      ac, "move_line_up");

  new KAction(i18n("Move Character Right"), Key_Right, this,
	      SLOT(cursorRight()), ac, "move_cursor_right");

  new KAction(i18n("Move Character Left"), Key_Left, this, SLOT(cursorLeft()),
	      ac, "move_cusor_left");

  new KAction(
    i18n("Select to Next Line"),                    SHIFT + Key_Down,
    this, SLOT(shiftDown()),
    ac, "select_line_down" );
  new KAction(
    i18n("Scroll Line Down"),               CTRL +          Key_Down,
    this, SLOT(scrollDown()),
    ac, "scroll_line_down" );

  new KAction(
    i18n("Scroll Page Up"),                         KStdAccel::prior(),
    this, SLOT(pageUp()),
    ac, "scroll_page_up" );
  new KAction(
    i18n("Select Page Up"),                         SHIFT + Key_PageUp,
    this, SLOT(shiftPageUp()),
    ac, "select_page_up" );
  new KAction(
    i18n("Move to Top of View"),             CTRL +         Key_PageUp,
    this, SLOT(topOfView()),
    ac, "move_top_of_view" );
  new KAction(
    i18n("Select to Top of View"),             CTRL + SHIFT +  Key_PageUp,
    this, SLOT(shiftTopOfView()),
    ac, "select_top_of_view" );

  new KAction(
    i18n("Scroll Page Down"),                          KStdAccel::next(),
    this, SLOT(pageDown()),
    ac, "scroll_page_down" );
  new KAction(
    i18n("Select Page Down"),                       SHIFT + Key_PageDown,
    this, SLOT(shiftPageDown()),
    ac, "select_page_down" );
  new KAction(
    i18n("Move to Bottom of View"),          CTRL +         Key_PageDown,
    this, SLOT(bottomOfView()),
    ac, "move_bottom_of_view" );
  new KAction(
    i18n("Select to Bottom of View"),         CTRL + SHIFT + Key_PageDown,
    this, SLOT(shiftBottomOfView()),
    ac, "select_bottom_of_view" );
  new KAction(
    i18n("Move to Matching Bracket"),               CTRL + Key_6,
    this, SLOT(toMatchingBracket()),
    ac, "to_matching_bracket" );
  new KAction(
    i18n("Select to Matching Bracket"),      SHIFT + CTRL + Key_6,
    this, SLOT(shiftToMatchingBracket()),
    ac, "select_matching_bracket" );

/*
  new KAction(
    i18n("Switch to Command Line"),          Qt::Key_F7,
    this, SLOT(switchToCmdLine()),
    ac, "switch_to_cmd_line" );*/

  // anders: shortcuts doing any changes should not be created in browserextension
  if ( !m_doc->readOnly() )
  {
    new KAction(
      i18n("Transpose Characters"),           CTRL + Key_T,
      this, SLOT(transpose()),
      ac, "transpose_char" );

    new KAction(
      i18n("Delete Line"),                    CTRL + Key_K,
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

    new KAction(i18n("Delete Next Character"), Key_Delete,
                this, SLOT(keyDelete()),
                ac, "delete_next_character");

    new KAction(i18n("Backspace"), Key_Backspace,
                this, SLOT(backspace()),
                ac, "backspace");
  }

  connect( this, SIGNAL(gotFocus(Kate::View*)),
           this, SLOT(slotGotFocus()) );
  connect( this, SIGNAL(lostFocus(Kate::View*)),
           this, SLOT(slotLostFocus()) );

  m_editActions->readShortcutSettings( "Katepart Shortcuts" );

  if( hasFocus() )
    slotGotFocus();
  else
    slotLostFocus();


}

void KateView::setupCodeFolding()
{
  KActionCollection *ac=this->actionCollection();
  new KAction( i18n("Collapse Toplevel"), CTRL+SHIFT+Key_Minus,
       m_doc->foldingTree(),SLOT(collapseToplevelNodes()),ac,"folding_toplevel");
  new KAction( i18n("Expand Toplevel"), CTRL+SHIFT+Key_Plus,
       this,SLOT(slotExpandToplevel()),ac,"folding_expandtoplevel");
  new KAction( i18n("Collapse One Local Level"), CTRL+Key_Minus,
       this,SLOT(slotCollapseLocal()),ac,"folding_collapselocal");
  new KAction( i18n("Expand One Local Level"), CTRL+Key_Plus,
       this,SLOT(slotExpandLocal()),ac,"folding_expandlocal");
#if 0
  KAccel* debugAccels = new KAccel(this,this);
  debugAccels->insert("KATE_DUMP_REGION_TREE",i18n("Show the code folding region tree"),"","Ctrl+Shift+Alt+D",m_doc,SLOT(dumpRegionTree()));
  debugAccels->insert("KATE_TEMPLATE_TEST",i18n("Basic template code test"),"","Ctrl+Shift+Alt+T",m_doc,SLOT(testTemplateCode()));
  debugAccels->setEnabled(true);
#endif
}

void KateView::slotExpandToplevel()
{
  m_doc->foldingTree()->expandToplevelNodes(m_doc->numLines());
}

void KateView::slotCollapseLocal()
{
  int realLine = m_doc->foldingTree()->collapseOne(cursorLine());
  if (realLine != -1)
    // TODO rodda: fix this to only set line and allow internal view to chose column
    // Explicitly call internal because we want this to be registered as an internal call
    setCursorPositionInternal(realLine, cursorColumn(), tabWidth(), false);
}

void KateView::slotExpandLocal()
{
  m_doc->foldingTree()->expandOne(cursorLine(), m_doc->numLines());
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

void KateView::slotGotFocus()
{
  m_editActions->accel()->setEnabled( true );

  slotStatusMsg ();
}

void KateView::slotLostFocus()
{
  m_editActions->accel()->setEnabled( false );
}

void KateView::setDynWrapIndicators(int mode)
{
  config()->setDynWordWrapIndicators (mode);
}

void KateView::slotStatusMsg ()
{
  QString ovrstr;
  if (m_doc->isReadWrite())
  {
    if (m_doc->config()->configFlags() & KateDocument::cfOvr)
      ovrstr = i18n(" OVR ");
    else
      ovrstr = i18n(" INS ");
  }
  else
    ovrstr = i18n(" R/O ");

  uint r = cursorLine() + 1;
  uint c = cursorColumn() + 1;

  QString s1 = i18n(" Line: %1").arg(KGlobal::locale()->formatNumber(r, 0));
  QString s2 = i18n(" Col: %1").arg(KGlobal::locale()->formatNumber(c, 0));

  QString modstr = m_doc->isModified() ? QString (" * ") : QString ("   ");
  QString blockstr = blockSelectionMode() ? i18n(" BLK ") : i18n(" NORM ");

  emit viewStatusMsg (s1 + s2 + " " + ovrstr + blockstr + modstr);
}

void KateView::slotSelectionTypeChanged()
{
  m_toggleBlockSelection->setChecked( blockSelectionMode() );

  emit newStatus();
}

bool KateView::isOverwriteMode() const
{
  return m_doc->config()->configFlags() & KateDocument::cfOvr;
}

void KateView::reloadFile()
{
  // save cursor position
  uint cl = cursorLine();
  uint cc = cursorColumn();

  // save bookmarks
  m_doc->reloadFile();

  if (m_doc->numLines() >= cl)
    // Explicitly call internal function because we want this to be registered as a non-external call
    setCursorPositionInternal( cl, cc, tabWidth(), false );

  emit newStatus();
}

void KateView::slotUpdate()
{
  emit newStatus();

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
  for (uint z = 0; z < l.size(); z++)
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
  KURL::List lstDragURLs;
  bool ok = KURLDrag::decode( ev, lstDragURLs );

  KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( doc() );
  if ( ok && ext )
    emit ext->openURLRequest( lstDragURLs.first() );
}

void KateView::contextMenuEvent( QContextMenuEvent *ev )
{
  if ( !m_doc || !m_doc->browserExtension()  )
    return;
  emit m_doc->browserExtension()->popupMenu( /*this, */ev->globalPos(), m_doc->url(),
                                        QString::fromLatin1( "text/plain" ) );
  ev->accept();
}

bool KateView::setCursorPositionInternal( uint line, uint col, uint tabwidth, bool calledExternally )
{
  KateTextLine::Ptr l = m_doc->kateTextLine( line );

  if (!l)
    return false;

  QString line_str = m_doc->textLine( line );

  uint z;
  uint x = 0;
  for (z = 0; z < line_str.length() && z < col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  m_viewInternal->updateCursor( KateTextCursor( line, x ), false, true, calledExternally );

  return true;
}

void KateView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    m_doc->setConfigFlags( m_doc->config()->configFlags() ^ KateDocument::cfOvr );
  else
    m_doc->setConfigFlags( m_doc->config()->configFlags() | KateDocument::cfOvr );

  m_toggleInsert->setChecked (isOverwriteMode ());
}

void KateView::toggleInsert()
{
  m_doc->setConfigFlags(m_doc->config()->configFlags() ^ KateDocument::cfOvr);
  m_toggleInsert->setChecked (isOverwriteMode ());

  emit newStatus();
}

bool KateView::canDiscard()
{
  return m_doc->closeURL();
}

void KateView::flush()
{
  m_doc->closeURL();
}

KateView::saveResult KateView::save()
{
  if( !m_doc->url().isValid() || !doc()->isReadWrite() )
    return saveAs();

  if( m_doc->save() )
    return SAVE_OK;

  return SAVE_ERROR;
}

KateView::saveResult KateView::saveAs()
{

  KEncodingFileDialog::Result res=KEncodingFileDialog::getSaveURLAndEncoding(doc()->config()->encoding(),
                m_doc->url().url(),QString::null,this,i18n("Save File"));

//   kdDebug()<<"urllist is emtpy?"<<res.URLs.isEmpty()<<endl;
//   kdDebug()<<"url is:"<<res.URLs.first()<<endl;
  if( res.URLs.isEmpty() || !checkOverwrite( res.URLs.first() ) )
    return SAVE_CANCEL;

  m_doc->setEncoding( res.encoding );

  if( m_doc->saveAs( res.URLs.first() ) )
    return SAVE_OK;

  return SAVE_ERROR;
}

bool KateView::checkOverwrite( KURL u )
{
  if( !u.isLocalFile() )
    return true;

  QFileInfo info( u.path() );
  if( !info.exists() )
    return true;

  return KMessageBox::Yes
         == KMessageBox::warningYesNo
              ( this,
                i18n( "A file named \"%1\" already exists. Are you sure you want to overwrite it?" ).arg( info.fileName() ),
                i18n( "Overwrite File?" ),
                KGuiItem( i18n( "&Overwrite" ), "filesave", i18n( "Overwrite the file" ) ),
                KStdGuiItem::cancel()
              );
}

void KateView::slotSaveCanceled( const QString& error )
{
  if ( !error.isEmpty() ) // happens when cancelling a job
    KMessageBox::error( this, error );
}

void KateView::gotoLine()
{
  KateGotoLineDialog *dlg = new KateGotoLineDialog (this, m_viewInternal->getCursor().line() + 1, m_doc->numLines());

  if (dlg->exec() == QDialog::Accepted)
    gotoLineNumber( dlg->getLine() - 1 );

  delete dlg;
}

void KateView::gotoLineNumber( int line )
{
  // clear selection, unless we are in persistent selection mode
  if ( !config()->persistentSelection() )
    clearSelection();
  setCursorPositionInternal ( line, 0, 1 );
}

void KateView::joinLines()
{
  int first = selStartLine();
  int last = selEndLine();
  //int left = m_doc->textLine( last ).length() - m_doc->selEndCol();
  if ( first == last )
  {
    first = cursorLine();
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
  config->writeEntry("CursorLine",m_viewInternal->cursor.line());
  config->writeEntry("CursorColumn",m_viewInternal->cursor.col());
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

void KateView::setDynWordWrap( bool b )
{
  config()->setDynWordWrap( b );
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
  return m_viewInternal->leftBorder->iconBorderOn();
}

bool KateView::lineNumbersOn() {
  return m_viewInternal->leftBorder->lineNumbersOn();
}

bool KateView::scrollBarMarks() {
  return m_viewInternal->m_lineScroll->showMarks();
}

int KateView::dynWrapIndicators() {
  return m_viewInternal->leftBorder->dynWrapIndicators();
}

bool KateView::foldingMarkersOn() {
  return m_viewInternal->leftBorder->foldingMarkersOn();
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
      m_grid->addMultiCellWidget (m_cmdLine, 2, 2, 0, 2);
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
  if (hasSelection())
  {
    m_copy->setEnabled (true);
    m_deSelect->setEnabled (true);
  }
  else
  {
    m_copy->setEnabled (false);
    m_deSelect->setEnabled (false);
  }

  if (m_doc->readOnly())
    return;

  bool b = hasSelection();
  m_cut->setEnabled (b);
  m_spellcheckSelection->setEnabled(b);
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

void KateView::showArgHint( QStringList arg1, const QString& arg2, const QString& arg3 )
{
  m_codeCompletion->showArgHint( arg1, arg2, arg3 );
}

void KateView::showCompletionBox( QValueList<KTextEditor::CompletionEntry> arg1, int offset, bool cs )
{
  emit aboutToShowCompletionBox();
  m_codeCompletion->showCompletionBox( arg1, offset, cs );
}

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

  m_viewInternal->leftBorder->setDynWrapIndicators( config()->dynWordWrapIndicators() );
  m_setDynWrapIndicators->setCurrentItem( config()->dynWordWrapIndicators() );

  // line numbers
  m_viewInternal->leftBorder->setLineNumbersOn( config()->lineNumbers() );
  m_toggleLineNumbers->setChecked( config()->lineNumbers() );

  // icon bar
  m_viewInternal->leftBorder->setIconBorderOn( config()->iconBar() );
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
  m_viewInternal->leftBorder->updateFont();
  m_viewInternal->leftBorder->repaint ();

  m_renderer->setShowIndentLines (config()->showIndentationLines());
}

void KateView::updateFoldingConfig ()
{
  // folding bar
  bool doit = config()->foldingBar() && m_doc->highlight() && m_doc->highlight()->allowsFolding();
  m_viewInternal->leftBorder->setFoldingMarkersOn(doit);
  m_toggleFoldingMarkers->setChecked( doit );
  m_toggleFoldingMarkers->setEnabled( m_doc->highlight() && m_doc->highlight()->allowsFolding() );

  QStringList l;

  l << "folding_toplevel" << "folding_expandtoplevel"
    << "folding_collapselocal" << "folding_expandlocal";

  KAction *a = 0;
  for (uint z = 0; z < l.size(); z++)
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

void KateView::editSetCursor (const KateTextCursor &cursor)
{
  m_viewInternal->editSetCursor (cursor);
}
//END

//BEGIN TAG & CLEAR
bool KateView::tagLine (const KateTextCursor& virtualCursor)
{
  return m_viewInternal->tagLine (virtualCursor);
}

bool KateView::tagLines (int start, int end, bool realLines)
{
  return m_viewInternal->tagLines (start, end, realLines);
}

bool KateView::tagLines (KateTextCursor start, KateTextCursor end, bool realCursors)
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
  m_viewInternal->paintText(0,0,m_viewInternal->width(),m_viewInternal->height(), paintOnlyDirty);
}

void KateView::updateView (bool changed)
{
  m_viewInternal->updateView (changed);
  m_viewInternal->leftBorder->update();
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

uint KateView::cursorColumn()
{
  uint r = m_doc->currentColumn(m_viewInternal->getCursor());
  if ( !( m_doc->config()->configFlags() & KateDocumentConfig::cfWrapCursor ) &&
       (uint)m_viewInternal->getCursor().col() > m_doc->textLine( m_viewInternal->getCursor().line() ).length()  )
    r += m_viewInternal->getCursor().col() - m_doc->textLine( m_viewInternal->getCursor().line() ).length();

  return r;
}

void KateView::spellcheckFromCursor()
{
  m_doc->spellcheck( m_viewInternal->getCursor() );
}

void KateView::spellcheckSelection()
{
  KateTextCursor from( selStartLine(), selStartCol() );
  KateTextCursor to( selEndLine(), selEndCol() );
  m_doc->spellcheck( from, to );
}

//BEGIN KTextEditor::SelectionInterface stuff

bool KateView::setSelection( const KateTextCursor& start, const KateTextCursor& end )
{
  KateTextCursor oldSelectStart = selectStart;
  KateTextCursor oldSelectEnd = selectEnd;

  if (start <= end) {
    selectStart.setPos(start);
    selectEnd.setPos(end);
  } else {
    selectStart.setPos(end);
    selectEnd.setPos(start);
  }

  tagSelection(oldSelectStart, oldSelectEnd);

  repaintText(true);

  emit selectionChanged ();
  emit m_doc->selectionChanged ();

  return true;
}

bool KateView::setSelection( uint startLine, uint startCol, uint endLine, uint endCol )
{
  if (hasSelection())
    clearSelection(false, false);

  return setSelection( KateTextCursor(startLine, startCol), KateTextCursor(endLine, endCol) );
}

bool KateView::clearSelection()
{
  return clearSelection(true);
}

bool KateView::clearSelection(bool redraw, bool finishedChangingSelection)
{
  if( !hasSelection() )
    return false;

  KateTextCursor oldSelectStart = selectStart;
  KateTextCursor oldSelectEnd = selectEnd;

  selectStart.setPos(-1, -1);
  selectEnd.setPos(-1, -1);

  tagSelection(oldSelectStart, oldSelectEnd);

  oldSelectStart = selectStart;
  oldSelectEnd = selectEnd;

  if (redraw)
    repaintText(true);

  if (finishedChangingSelection)
  {
    emit selectionChanged();
    emit m_doc->selectionChanged ();
  }

  return true;
}

bool KateView::hasSelection() const
{
  return selectStart != selectEnd;
}

QString KateView::selectionAsHtml() const
{
  int sc = selectStart.col();
  int ec = selectEnd.col();

  if ( blockSelect )
  {
    if (sc > ec)
    {
      uint tmp = sc;
      sc = ec;
      ec = tmp;
    }
  }
  return m_doc->textAsHtml (selectStart.line(), sc, selectEnd.line(), ec, blockSelect);
}

QString KateView::selection() const
{
  int sc = selectStart.col();
  int ec = selectEnd.col();

  if ( blockSelect )
  {
    if (sc > ec)
    {
      uint tmp = sc;
      sc = ec;
      ec = tmp;
    }
  }
  return m_doc->text (selectStart.line(), sc, selectEnd.line(), ec, blockSelect);
}

bool KateView::removeSelectedText ()
{
  if (!hasSelection())
    return false;

  m_doc->editStart ();

  int sc = selectStart.col();
  int ec = selectEnd.col();

  if ( blockSelect )
  {
    if (sc > ec)
    {
      uint tmp = sc;
      sc = ec;
      ec = tmp;
    }
  }

  m_doc->removeText (selectStart.line(), sc, selectEnd.line(), ec, blockSelect);

  // don't redraw the cleared selection - that's done in editEnd().
  clearSelection(false);

  m_doc->editEnd ();

  return true;
}

bool KateView::selectAll()
{
  setBlockSelectionMode (false);

  return setSelection (0, 0, m_doc->lastLine(), m_doc->lineLength(m_doc->lastLine()));
}

bool KateView::lineColSelected (int line, int col)
{
  if ( (!blockSelect) && (col < 0) )
    col = 0;

  KateTextCursor cursor(line, col);

  if (blockSelect)
    return cursor.line() >= selectStart.line() && cursor.line() <= selectEnd.line() && cursor.col() >= selectStart.col() && cursor.col() < selectEnd.col();
  else
    return (cursor >= selectStart) && (cursor < selectEnd);
}

bool KateView::lineSelected (int line)
{
  return (!blockSelect)
    && (selectStart <= KateTextCursor(line, 0))
    && (line < selectEnd.line());
}

bool KateView::lineEndSelected (int line, int endCol)
{
  return (!blockSelect)
    && (line > selectStart.line() || (line == selectStart.line() && (selectStart.col() < endCol || endCol == -1)))
    && (line < selectEnd.line() || (line == selectEnd.line() && (endCol <= selectEnd.col() && endCol != -1)));
}

bool KateView::lineHasSelected (int line)
{
  return (selectStart < selectEnd)
    && (line >= selectStart.line())
    && (line <= selectEnd.line());
}

bool KateView::lineIsSelection (int line)
{
  return (line == selectStart.line() && line == selectEnd.line());
}

void KateView::tagSelection(const KateTextCursor &oldSelectStart, const KateTextCursor &oldSelectEnd)
{
  if (hasSelection()) {
    if (oldSelectStart.line() == -1) {
      // We have to tag the whole lot if
      // 1) we have a selection, and:
      //  a) it's new; or
      tagLines(selectStart, selectEnd);

    } else if (blockSelectionMode() && (oldSelectStart.col() != selectStart.col() || oldSelectEnd.col() != selectEnd.col())) {
      //  b) we're in block selection mode and the columns have changed
      tagLines(selectStart, selectEnd);
      tagLines(oldSelectStart, oldSelectEnd);

    } else {
      if (oldSelectStart != selectStart) {
        if (oldSelectStart < selectStart)
          tagLines(oldSelectStart, selectStart);
        else
          tagLines(selectStart, oldSelectStart);
      }

      if (oldSelectEnd != selectEnd) {
        if (oldSelectEnd < selectEnd)
          tagLines(oldSelectEnd, selectEnd);
        else
          tagLines(selectEnd, oldSelectEnd);
      }
    }

  } else {
    // No more selection, clean up
    tagLines(oldSelectStart, oldSelectEnd);
  }
}

void KateView::selectWord( const KateTextCursor& cursor )
{
  int start, end, len;

  KateTextLine::Ptr textLine = m_doc->plainKateTextLine(cursor.line());

  if (!textLine)
    return;

  len = textLine->length();
  start = end = cursor.col();
  while (start > 0 && m_doc->highlight()->isInWord(textLine->getChar(start - 1), textLine->attribute(start - 1))) start--;
  while (end < len && m_doc->highlight()->isInWord(textLine->getChar(end), textLine->attribute(start - 1))) end++;
  if (end <= start) return;

  setSelection (cursor.line(), start, cursor.line(), end);
}

void KateView::selectLine( const KateTextCursor& cursor )
{
  setSelection (cursor.line(), 0, cursor.line()+1, 0);
}

void KateView::selectLength( const KateTextCursor& cursor, int length )
{
  int start, end;

  KateTextLine::Ptr textLine = m_doc->plainKateTextLine(cursor.line());

  if (!textLine)
    return;

  start = cursor.col();
  end = start + length;
  if (end <= start) return;

  setSelection (cursor.line(), start, cursor.line(), end);
}

void KateView::cut()
{
  if (!hasSelection())
    return;

  copy();
  removeSelectedText();

  m_viewInternal->repaint();
}

void KateView::copy() const
{
  kdDebug(13020) << "in katedocument::copy()" << endl;
  if (!hasSelection())
    return;
#ifndef QT_NO_MIMECLIPBOARD
  QClipboard *cb = QApplication::clipboard();

  KMultipleDrag *drag = new KMultipleDrag();
  QString htmltext;
  if(!cb->selectionModeEnabled())
    htmltext = selectionAsHtml();

  if(!htmltext.isEmpty()) {
    QTextDrag *htmltextdrag = new QTextDrag(htmltext) ;
    htmltextdrag->setSubtype("html");

    drag->addDragObject( htmltextdrag);
  }
  drag->addDragObject( new QTextDrag( selection()));

  QApplication::clipboard()->setData(drag);
#else
  QApplication::clipboard()->setText(selection ());
#endif

  m_viewInternal->repaint();
}

//END

//BEGIN KTextEditor::BlockSelectionInterface stuff

bool KateView::blockSelectionMode ()
{
  return blockSelect;
}

bool KateView::setBlockSelectionMode (bool on)
{
  if (on != blockSelect)
  {
    blockSelect = on;

    KateTextCursor oldSelectStart = selectStart;
    KateTextCursor oldSelectEnd = selectEnd;

    clearSelection(false, false);

    setSelection(oldSelectStart, oldSelectEnd);

    slotSelectionTypeChanged();
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
  return !blockSelectionMode() && (m_doc->configFlags() & KateDocument::cfWrapCursor);
}

//END

//BEGIN IM INPUT STUFF
void KateView::setIMSelectionValue( uint imStartLine, uint imStart, uint imEnd,
                                        uint imSelStart, uint imSelEnd, bool imComposeEvent )
{
  m_imStartLine = imStartLine;
  m_imStart = imStart;
  m_imEnd = imEnd;
  m_imSelStart = imSelStart;
  m_imSelEnd = imSelEnd;
  m_imComposeEvent = imComposeEvent;
}

bool KateView::isIMSelection( int _line, int _column )
{
  return ( ( int( m_imStartLine ) == _line ) && ( m_imSelStart < m_imSelEnd ) && ( _column >= int( m_imSelStart ) ) &&
    ( _column < int( m_imSelEnd ) ) );
}

bool KateView::isIMEdit( int _line, int _column )
{
  return ( ( int( m_imStartLine ) == _line ) && ( m_imStart < m_imEnd ) && ( _column >= int( m_imStart ) ) &&
    ( _column < int( m_imEnd ) ) );
}

void KateView::getIMSelectionValue( uint *imStartLine, uint *imStart, uint *imEnd,
                                        uint *imSelStart, uint *imSelEnd )
{
  *imStartLine = m_imStartLine;
  *imStart = m_imStart;
  *imEnd = m_imEnd;
  *imSelStart = m_imSelStart;
  *imSelEnd = m_imSelEnd;
}
//END IM INPUT STUFF

// kate: space-indent on; indent-width 2; replace-tabs on;
