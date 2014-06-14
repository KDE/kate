/* This file is part of the KDE libraries
   Copyright (C) 2009 Michel Ludwig <michel.ludwig@kdemail.net>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001-2010 Joseph Wenninger <jowenn@kde.org>
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
#include "kateundomanager.h"
#include "katedocumenthelpers.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katehighlight.h"
#include "katehighlightmenu.h"
#include "katedialogs.h"
#include "katetextline.h"
#include "kateschema.h"
#include "katebookmarks.h"
#include "kateconfig.h"
#include "katemodemenu.h"
#include "kateautoindent.h"
#include "katecompletionwidget.h"
#include "katesearchbar.h"
#include "kateviemulatedcommandbar.h"
#include "katepartpluginmanager.h"
#include "katewordcompletion.h"
#include "katekeywordcompletion.h"
#include "katelayoutcache.h"
#include "spellcheck/spellcheck.h"
#include "spellcheck/spellcheckdialog.h"
#include "spellcheck/spellingmenu.h"
#include "katebuffer.h"
#include "script/katescriptmanager.h"
#include "script/katescriptaction.h"
#include "snippet/katesnippetglobal.h"
#include "snippet/snippetcompletionmodel.h"
#include "katemessagewidget.h"
#include "katetemplatehandler.h"

#include <ktexteditor/messageinterface.h>

#include <kparts/event.h>

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
#include <kencodingfiledialog.h>
#include <kstandardshortcut.h>
#include <kmenu.h>
#include <ktoggleaction.h>
#include <kselectaction.h>
#include <kactioncollection.h>
#include <kdeversion.h>

#include <QtGui/QFont>
#include <QtCore/QFileInfo>
#include <QtGui/QStyle>
#include <QtGui/QKeyEvent>
#include <QtGui/QLayout>
#include <QtCore/QMimeData>

//#define VIEW_RANGE_DEBUG

//END includes

namespace {

bool hasCommentInFirstLine(KateDocument* doc)
{
  const Kate::TextLine& line = doc->kateTextLine(0);
  Q_ASSERT(line);
  return doc->isComment(0, line->firstChar());
}

}

void KateView::blockFix(KTextEditor::Range& range)
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
    , m_completionWidget(0)
    , m_annotationModel(0)
    , m_hasWrap( false )
    , m_doc( doc )
    , m_textFolding (doc->buffer())
    , m_config( new KateViewConfig( this ) )
    , m_renderer( new KateRenderer( doc, m_textFolding, this ) )
    , m_viewInternal( new KateViewInternal( this ) )
    , m_spell( new KateSpellCheckDialog( this ) )
    , m_bookmarks( new KateBookmarks( this ) )
    , m_startingUp (true)
    , m_updatingDocumentConfig (false)
    , m_selection (m_doc->buffer(), KTextEditor::Range::invalid(), Kate::TextRange::ExpandLeft, Kate::TextRange::AllowEmpty)
    , blockSelect (false)
    , m_bottomViewBar (0)
    , m_topViewBar (0)
    , m_cmdLine (0)
    , m_searchBar (0)
    , m_viModeEmulatedCommandBar(0)
    , m_gotoBar (0)
    , m_dictionaryBar(NULL)
    , m_spellingMenu( new KateSpellingMenu( this ) )
    , m_userContextMenuSet( false )
    , m_delayedUpdateTriggered (false)
    , m_lineToUpdateMin (-1)
    , m_lineToUpdateMax (-1)
    , m_floatTopMessageWidget (0)
    , m_floatBottomMessageWidget (0)
{
  // queued connect to collapse view updates for range changes, INIT THIS EARLY ENOUGH!
  connect(this, SIGNAL(delayedUpdateOfView()), this, SLOT(slotDelayedUpdateOfView()), Qt::QueuedConnection);

  setComponentData ( KateGlobal::self()->componentData () );

  // selection if for this view only and will invalidate if becoming empty
  m_selection.setView (this);

  // use z depth defined in moving ranges interface
  m_selection.setZDepth (-100000.0);

  KateGlobal::self()->registerView( this );

  KTextEditor::ViewBarContainer *viewBarContainer=qobject_cast<KTextEditor::ViewBarContainer*>( KateGlobal::self()->container() );
  QWidget *bottomBarParent=viewBarContainer?viewBarContainer->getViewBarParent(this,KTextEditor::ViewBarContainer::BottomBar):0;
  QWidget *topBarParent=viewBarContainer?viewBarContainer->getViewBarParent(this,KTextEditor::ViewBarContainer::TopBar):0;

  m_bottomViewBar=new KateViewBar (bottomBarParent!=0,KTextEditor::ViewBarContainer::BottomBar,bottomBarParent?bottomBarParent:this,this);
  m_topViewBar=new KateViewBar (topBarParent!=0,KTextEditor::ViewBarContainer::TopBar,topBarParent?topBarParent:this,this);

  // ugly workaround:
  // Force the layout to be left-to-right even on RTL deskstop, as discussed
  // on the mailing list. This will cause the lines and icons panel to be on
  // the left, even for Arabic/Hebrew/Farsi/whatever users.
  setLayoutDirection ( Qt::LeftToRight );

  // layouting ;)
  m_vBox = new QVBoxLayout (this);
  m_vBox->setMargin (0);
  m_vBox->setSpacing (0);

  // add top viewbar...
  if (topBarParent)
    viewBarContainer->addViewBarToLayout(this,m_topViewBar,KTextEditor::ViewBarContainer::TopBar);
  else
    m_vBox->addWidget(m_topViewBar);

  m_bottomViewBar->installEventFilter(m_viewInternal);

  // add KateMessageWidget for KTE::MessageInterface immediately above view
  m_topMessageWidget = new KateMessageWidget(this);
  m_vBox->addWidget(m_topMessageWidget);
  m_topMessageWidget->hide();

  // add hbox: KateIconBorder | KateViewInternal | KateScrollBar
  QHBoxLayout *hbox = new QHBoxLayout ();
  m_vBox->addLayout (hbox, 100);
  hbox->setMargin (0);
  hbox->setSpacing (0);

  QStyleOption option;
  option.initFrom(this);

  if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &option, this)) {
      QHBoxLayout *extrahbox = new QHBoxLayout ();
      QFrame * frame = new QFrame(this);
      extrahbox->setMargin (0);
      extrahbox->setSpacing (0);
      extrahbox->addWidget (m_viewInternal->m_leftBorder);
      extrahbox->addWidget (m_viewInternal);
      frame->setLayout (extrahbox);
      hbox->addWidget (frame);
      hbox->addSpacing (style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &option, this));
      frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  }
  else {
    hbox->addWidget (m_viewInternal->m_leftBorder);
    hbox->addWidget (m_viewInternal);
  }
  hbox->addWidget (m_viewInternal->m_lineScroll);

  if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &option, this)) {
    m_vBox->addSpacing (style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &option, this));
  }

  // add hbox: ColumnsScrollBar | Dummy
  hbox = new QHBoxLayout ();
  m_vBox->addLayout (hbox);
  hbox->setMargin (0);
  hbox->setSpacing (0);

  hbox->addWidget (m_viewInternal->m_columnScroll);
  hbox->addWidget (m_viewInternal->m_dummy);

  // add KateMessageWidget for KTE::MessageInterface immediately above view
  m_bottomMessageWidget = new KateMessageWidget(this);
  m_vBox->addWidget(m_bottomMessageWidget);
  m_bottomMessageWidget->hide();

  // add bottom viewbar...
  if (bottomBarParent)
    viewBarContainer->addViewBarToLayout(this,m_bottomViewBar,KTextEditor::ViewBarContainer::BottomBar);
  else
    m_vBox->addWidget(m_bottomViewBar);

  // add layout for floating message widgets to KateViewInternal
  m_notificationLayout = new QVBoxLayout(m_viewInternal);
  m_notificationLayout->setContentsMargins(20, 20, 20, 20);
  m_viewInternal->setLayout(m_notificationLayout);

  // this really is needed :)
  m_viewInternal->updateView ();

  doc->addView( this );

  setFocusProxy( m_viewInternal );
  setFocusPolicy( Qt::StrongFocus );

  // default ui file with all features
  QString uifile = "katepartui.rc";

  // simple mode
  if (doc->simpleMode ())
    uifile = "katepartsimpleui.rc";

  setXMLFile( uifile );

  setupConnections();
  setupActions();

  // auto word completion
  new KateWordCompletionView (this, actionCollection ());

  // enable the plugins of this view
  KatePartPluginManager::self()->addView(this);

  // update the enabled state of the undo/redo actions...
  slotUpdateUndo();

  m_startingUp = false;
  updateConfig ();

  slotHlChanged();
  KCursor::setAutoHideCursor( m_viewInternal, true );

  if ( viInputMode() /* && FIXME: HAEHH? !config()->viInputModeHideStatusBar() */ ) {
    deactivateEditActions();
  }

  // user interaction (scrollling) starts notification auto-hide timer
  connect(this, SIGNAL(displayRangeChanged(KateView*)), m_topMessageWidget, SLOT(startAutoHideTimer()));
  connect(this, SIGNAL(displayRangeChanged(KateView*)), m_bottomMessageWidget, SLOT(startAutoHideTimer()));

  // user interaction (cursor navigation) starts notification auto-hide timer
  connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), m_topMessageWidget, SLOT(startAutoHideTimer()));
  connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), m_bottomMessageWidget, SLOT(startAutoHideTimer()));

  // folding restoration on reload
  connect(m_doc, SIGNAL(aboutToReload(KTextEditor::Document*)), SLOT(saveFoldingState()));
  connect(m_doc, SIGNAL(reloaded(KTextEditor::Document*)), SLOT(applyFoldingState()));
}

KateView::~KateView()
{
  // invalidate update signal
  m_delayedUpdateTriggered = false;

  // remove from xmlgui factory, to be safe
  if (factory())
    factory()->removeClient (this);

    KTextEditor::ViewBarContainer *viewBarContainer=qobject_cast<KTextEditor::ViewBarContainer*>( KateGlobal::self()->container() );
    if (viewBarContainer) {
     viewBarContainer->deleteViewBarForView(this,KTextEditor::ViewBarContainer::BottomBar);
      m_bottomViewBar=0;
      viewBarContainer->deleteViewBarForView(this,KTextEditor::ViewBarContainer::TopBar);
      m_topViewBar=0;
    }

  KatePartPluginManager::self()->removeView(this);

  m_doc->removeView( this );

  delete m_viewInternal;

  delete m_renderer;

  delete m_config;

  KateGlobal::self()->deregisterView (this);
}

void KateView::setupConnections()
{
  connect( m_doc, SIGNAL(undoChanged()),
           this, SLOT(slotUpdateUndo()) );
  connect( m_doc, SIGNAL(highlightingModeChanged(KTextEditor::Document*)),
           this, SLOT(slotHlChanged()) );
  connect( m_doc, SIGNAL(canceled(QString)),
           this, SLOT(slotSaveCanceled(QString)) );
  connect( m_viewInternal, SIGNAL(dropEventPass(QDropEvent*)),
           this,           SIGNAL(dropEventPass(QDropEvent*)) );

  connect( m_doc, SIGNAL(annotationModelChanged(KTextEditor::AnnotationModel*,KTextEditor::AnnotationModel*)),
           m_viewInternal->m_leftBorder, SLOT(annotationModelChanged(KTextEditor::AnnotationModel*,KTextEditor::AnnotationModel*)) );

  if ( m_doc->browserView() )
  {
    connect( this, SIGNAL(dropEventPass(QDropEvent*)),
             this, SLOT(slotDropEventPass(QDropEvent*)) );
  }
}

void KateView::setupActions()
{
  KActionCollection *ac = actionCollection ();
  KAction *a;

  m_toggleWriteLock = 0;

  m_cut = a = ac->addAction(KStandardAction::Cut, this, SLOT(cut()));
  a->setWhatsThis(i18n("Cut the selected text and move it to the clipboard"));

  m_paste = a = ac->addAction(KStandardAction::PasteText, this, SLOT(paste()));
  a->setWhatsThis(i18n("Paste previously copied or cut clipboard contents"));

  m_copy = a = ac->addAction(KStandardAction::Copy, this, SLOT(copy()));
  a->setWhatsThis(i18n( "Use this command to copy the currently selected text to the system clipboard."));

  m_pasteMenu = ac->addAction("edit_paste_menu", new KatePasteMenu (i18n("Clipboard &History"), this));
  connect (KateGlobal::self(), SIGNAL(clipboardHistoryChanged()), this, SLOT(slotClipboardHistoryChanged()));

  if (!m_doc->readOnly())
  {
    a = ac->addAction(KStandardAction::Save, m_doc, SLOT(documentSave()));
    a->setWhatsThis(i18n("Save the current document"));

    a = m_editUndo = ac->addAction(KStandardAction::Undo, m_doc, SLOT(undo()));
    a->setWhatsThis(i18n("Revert the most recent editing actions"));

    a = m_editRedo = ac->addAction(KStandardAction::Redo, m_doc, SLOT(redo()));
    a->setWhatsThis(i18n("Revert the most recent undo operation"));

    // Tools > Scripts
    KateScriptActionMenu* scriptActionMenu = new KateScriptActionMenu(this, i18n("&Scripts"));
    ac->addAction("tools_scripts", scriptActionMenu);

    a = ac->addAction("tools_apply_wordwrap");
    a->setText(i18n("Apply &Word Wrap"));
    a->setWhatsThis(i18n("Use this command to wrap all lines of the current document which are longer than the width of the"
    " current view, to fit into this view.<br /><br /> This is a static word wrap, meaning it is not updated"
    " when the view is resized."));
    connect(a, SIGNAL(triggered(bool)), SLOT(applyWordWrap()));

    a = ac->addAction("tools_cleanIndent");
    a->setText(i18n("&Clean Indentation"));
    a->setWhatsThis(i18n("Use this to clean the indentation of a selected block of text (only tabs/only spaces).<br /><br />"
        "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
    connect(a, SIGNAL(triggered(bool)), SLOT(cleanIndent()));

    a = ac->addAction("tools_align");
    a->setText(i18n("&Align"));
    a->setWhatsThis(i18n("Use this to align the current line or block of text to its proper indent level."));
    connect(a, SIGNAL(triggered(bool)), SLOT(align()));

    a = ac->addAction("tools_comment");
    a->setText(i18n("C&omment"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_D));
    a->setWhatsThis(i18n("This command comments out the current line or a selected block of text.<br /><br />"
        "The characters for single/multiple line comments are defined within the language's highlighting."));
    connect(a, SIGNAL(triggered(bool)), SLOT(comment()));

    a = ac->addAction("tools_uncomment");
    a->setText(i18n("Unco&mment"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_D));
    a->setWhatsThis(i18n("This command removes comments from the current line or a selected block of text.<br /><br />"
    "The characters for single/multiple line comments are defined within the language's highlighting."));
    connect(a, SIGNAL(triggered(bool)), SLOT(uncomment()));

    a = ac->addAction("tools_toggle_comment");
    a->setText(i18n("Toggle Comment"));
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleComment()));

    a = m_toggleWriteLock = new KToggleAction(i18n("&Read Only Mode"), this);
    a->setWhatsThis( i18n("Lock/unlock the document for writing") );
    a->setChecked( !m_doc->isReadWrite() );
    connect(a, SIGNAL(triggered(bool)), SLOT(toggleWriteLock()));
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
    connect(a, SIGNAL(triggered(bool)), SLOT(joinLines()));

    a = ac->addAction( "tools_invoke_code_completion" );
    a->setText( i18n("Invoke Code Completion") );
    a->setWhatsThis(i18n("Manually invoke command completion, usually by using a shortcut bound to this action."));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Space));
    connect(a, SIGNAL(triggered(bool)), SLOT(userInvokedCompletion()));

    a = ac->addAction( "tools_create_snippet" );
    a->setIcon (KIcon("document-new"));
    a->setText( i18n("Create Snippet") );
    connect(a, SIGNAL(triggered(bool)), SLOT(createSnippet()));

    a = ac->addAction( "tools_snippets" );
    a->setText( i18n("Snippets...") );
    connect(a, SIGNAL(triggered(bool)), SLOT(showSnippetsDialog()));
  }
  else
  {
    m_cut->setEnabled (false);
    m_paste->setEnabled (false);
    m_pasteMenu->setEnabled (false);
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

  a = ac->addAction(QLatin1String("modified_line_up"));
  a->setText(i18n("Move to Previous Modified Line"));
  a->setWhatsThis(i18n("Move upwards to the previous modified line."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toPrevModifiedLine()));

  a = ac->addAction(QLatin1String("modified_line_down"));
  a->setText(i18n("Move to Next Modified Line"));
  a->setWhatsThis(i18n("Move downwards to the next modified line."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toNextModifiedLine()));

  a = ac->addAction("set_confdlg");
  a->setText(i18n("&Configure Editor..."));
  a->setWhatsThis(i18n("Configure various aspects of this editor."));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotConfigDialog()));

  KateModeMenu *ftm = new KateModeMenu (i18n("&Mode"), this);
  ac->addAction("tools_mode", ftm);
  ftm->setWhatsThis(i18n("Here you can choose which mode should be used for the current document. This will influence the highlighting and folding being used, for example."));
  ftm->updateMenu (m_doc);

  KateHighlightingMenu *menu = new KateHighlightingMenu (i18n("&Highlighting"), this);
  ac->addAction("tools_highlighting", menu);
  menu->setWhatsThis(i18n("Here you can choose how the current document should be highlighted."));
  menu->updateMenu (m_doc);

  KateViewSchemaAction *schemaMenu = new KateViewSchemaAction (i18n("&Schema"), this);
  ac->addAction("view_schemas", schemaMenu);
  schemaMenu->updateMenu (this);

  // indentation menu
  KateViewIndentationAction *indentMenu = new KateViewIndentationAction(m_doc, i18n("&Indentation"), this);
  ac->addAction("tools_indentation", indentMenu);

  m_selectAll = a= ac->addAction( KStandardAction::SelectAll, this, SLOT(selectAll()) );
  a->setWhatsThis(i18n("Select the entire text of the current document."));

  m_deSelect = a= ac->addAction( KStandardAction::Deselect, this, SLOT(clearSelection()) );
  a->setWhatsThis(i18n("If you have selected something within the current document, this will no longer be selected."));

  a = ac->addAction("view_inc_font_sizes");
  a->setIcon(KIcon("zoom-in"));
  a->setText(i18n("Enlarge Font"));
  a->setShortcut(KStandardShortcut::zoomIn());
  a->setWhatsThis(i18n("This increases the display font size."));
  connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotIncFontSizes()));

  a = ac->addAction("view_dec_font_sizes");
  a->setIcon(KIcon("zoom-out"));
  a->setText(i18n("Shrink Font"));
  a->setShortcut(KStandardShortcut::zoomOut());
  a->setWhatsThis(i18n("This decreases the display font size."));
  connect(a, SIGNAL(triggered(bool)), m_viewInternal, SLOT(slotDecFontSizes()));

  a = m_toggleBlockSelection = new KToggleAction(i18n("Bl&ock Selection Mode"), this);
  ac->addAction("set_verticalSelect", a);
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
  a->setWhatsThis(i18n("This command allows switching between the normal (line based) selection mode and the block selection mode."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleBlockSelection()));

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
  m_setDynWrapIndicators->setEnabled(m_toggleDynWrap->isChecked()); // only synced on real change, later

  a = toggleAction = m_toggleFoldingMarkers = new KToggleAction(i18n("Show Folding &Markers"), this);
  ac->addAction("view_folding_markers", a);
  a->setShortcut(QKeySequence(Qt::Key_F9));
  a->setWhatsThis(i18n("You can choose if the codefolding marks should be shown, if codefolding is possible."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleFoldingMarkers()));

  a = m_toggleIconBar = toggleAction = new KToggleAction(i18n("Show &Icon Border"), this);
  ac->addAction("view_border", a);
  a->setShortcut(QKeySequence(Qt::Key_F6));
  a->setWhatsThis(i18n("Show/hide the icon border.<br /><br />The icon border shows bookmark symbols, for instance."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleIconBorder()));

  a = toggleAction = m_toggleLineNumbers = new KToggleAction(i18n("Show &Line Numbers"), this);
  ac->addAction("view_line_numbers", a);
  a->setShortcut(QKeySequence(Qt::Key_F11));
  a->setWhatsThis(i18n("Show/hide the line numbers on the left hand side of the view."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleLineNumbersOn()));

  a = m_toggleScrollBarMarks = toggleAction = new KToggleAction(i18n("Show Scroll&bar Marks"), this);
  ac->addAction("view_scrollbar_marks", a);
  a->setWhatsThis(i18n("Show/hide the marks on the vertical scrollbar.<br /><br />The marks show bookmarks, for instance."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMarks()));

  a = m_toggleScrollBarMiniMap = toggleAction = new KToggleAction(i18n("Show Scrollbar Mini-Map"), this);
  ac->addAction("view_scrollbar_minimap", a);
  a->setWhatsThis(i18n("Show/hide the mini-map on the vertical scrollbar.<br /><br />The mini-map shows an overview of the whole document."));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMiniMap()));

//   a = m_toggleScrollBarMiniMapAll = toggleAction = new KToggleAction(i18n("Show the whole document in the Mini-Map"), this);
//   ac->addAction("view_scrollbar_minimap_all", a);
//   a->setWhatsThis(i18n("Display the whole document in the mini-map.<br /><br />With this option set the whole document will be visible in the mini-map."));
//   connect(a, SIGNAL(triggered(bool)), SLOT(toggleScrollBarMiniMapAll()));
//   connect(m_toggleScrollBarMiniMap, SIGNAL(triggered(bool)), m_toggleScrollBarMiniMapAll, SLOT(setEnabled(bool)));

  a = toggleAction = m_toggleWWMarker = new KToggleAction(i18n("Show Static &Word Wrap Marker"), this);
  ac->addAction("view_word_wrap_marker", a);
  a->setWhatsThis( i18n(
        "Show/hide the Word Wrap Marker, a vertical line drawn at the word "
        "wrap column as defined in the editing properties" ));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleWWMarker()));

  a = m_switchCmdLine = ac->addAction("switch_to_cmd_line");
  a->setText(i18n("Switch to Command Line"));
  a->setShortcut(QKeySequence(Qt::Key_F7));
  a->setWhatsThis(i18n("Show/hide the command line on the bottom of the view."));
  connect(a, SIGNAL(triggered(bool)), SLOT(switchToCmdLine()));

  a = m_viInputModeAction = new KToggleAction(i18n("&VI Input Mode"), this);
  ac->addAction("view_vi_input_mode", a);
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::META + Qt::Key_V));
  a->setWhatsThis( i18n("Activate/deactivate VI input mode" ));
  connect(a, SIGNAL(triggered(bool)), SLOT(toggleViInputMode()));

  a = m_setEndOfLine = new KSelectAction(i18n("&End of Line"), this);
  ac->addAction("set_eol", a);
  a->setWhatsThis(i18n("Choose which line endings should be used, when you save the document"));
  QStringList list;
  list.append(i18nc("@item:inmenu End of Line", "&UNIX"));
  list.append(i18nc("@item:inmenu End of Line", "&Windows/DOS"));
  list.append(i18nc("@item:inmenu End of Line", "&Macintosh"));
  m_setEndOfLine->setItems(list);
  m_setEndOfLine->setCurrentItem (m_doc->config()->eol());
  connect(m_setEndOfLine, SIGNAL(triggered(int)), this, SLOT(setEol(int)));

  a=m_addBom=new KToggleAction(i18n("Add &Byte Order Mark (BOM)"),this);
  m_addBom->setChecked(m_doc->config()->bom());
  ac->addAction("add_bom",a);
  a->setWhatsThis(i18n("Enable/disable adding of byte order markers for UTF-8/UTF-16 encoded files while saving"));
  connect(m_addBom,SIGNAL(triggered(bool)),this,SLOT(setAddBom(bool)));
  // encoding menu
  KateViewEncodingAction *encodingAction = new KateViewEncodingAction(m_doc, this, i18n("E&ncoding"), this);
  ac->addAction("set_encoding", encodingAction);

  a = ac->addAction( KStandardAction::Find, this, SLOT(find()) );
  a->setWhatsThis(i18n("Look up the first occurrence of a piece of text or regular expression."));
  addAction(a);

  a = ac->addAction("edit_find_selected");
  a->setText(i18n("Find Selected"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
  a->setWhatsThis(i18n("Finds next occurrence of selected text."));
  connect(a, SIGNAL(triggered(bool)), SLOT(findSelectedForwards()));

  a = ac->addAction("edit_find_selected_backwards");
  a->setText(i18n("Find Selected Backwards"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H));
  a->setWhatsThis(i18n("Finds previous occurrence of selected text."));
  connect(a, SIGNAL(triggered(bool)), SLOT(findSelectedBackwards()));

  a = ac->addAction( KStandardAction::FindNext, this, SLOT(findNext()) );
  a->setWhatsThis(i18n("Look up the next occurrence of the search phrase."));
  addAction(a);

  a = ac->addAction( KStandardAction::FindPrev, "edit_find_prev", this, SLOT(findPrevious()) );
  a->setWhatsThis(i18n("Look up the previous occurrence of the search phrase."));
  addAction(a);

  a = ac->addAction( KStandardAction::Replace, this, SLOT(replace()) );
  a->setWhatsThis(i18n("Look up a piece of text or regular expression and replace the result with some given text."));

  m_spell->createActions( ac );
  m_toggleOnTheFlySpellCheck = new KToggleAction(i18n("Automatic Spell Checking"), this);
  m_toggleOnTheFlySpellCheck->setWhatsThis(i18n("Enable/disable automatic spell checking"));
  m_toggleOnTheFlySpellCheck->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
  connect(m_toggleOnTheFlySpellCheck, SIGNAL(triggered(bool)), SLOT(toggleOnTheFlySpellCheck(bool)));
  ac->addAction("tools_toggle_automatic_spell_checking", m_toggleOnTheFlySpellCheck);

  a = ac->addAction("tools_change_dictionary");
  a->setText(i18n("Change Dictionary..."));
  a->setWhatsThis(i18n("Change the dictionary that is used for spell checking."));
  connect(a, SIGNAL(triggered()), SLOT(changeDictionary()));

  a = ac->addAction("tools_clear_dictionary_ranges");
  a->setText(i18n("Clear Dictionary Ranges"));
  a->setVisible(false);
  a->setWhatsThis(i18n("Remove all the separate dictionary ranges that were set for spell checking."));
  connect(a, SIGNAL(triggered()), m_doc, SLOT(clearDictionaryRanges()));
  connect(m_doc, SIGNAL(dictionaryRangesPresent(bool)), a, SLOT(setVisible(bool)));

  m_spellingMenu->createActions( ac );

  if (!m_doc->simpleMode ())
    m_bookmarks->createActions( ac );

  slotSelectionChanged ();

  //Now setup the editing actions before adding the associated
  //widget and setting the shortcut context
  setupEditActions();
  setupCodeFolding();
  slotClipboardHistoryChanged ();

  ac->addAssociatedWidget(m_viewInternal);

  foreach (QAction* action, ac->actions())
      action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

  connect (this, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(slotSelectionChanged()));
}

void KateView::createSnippet ()
{
  KateGlobal::self()->snippetGlobal()->createSnippet (this);
}

void KateView::showSnippetsDialog ()
{
  KateGlobal::self()->snippetGlobal()->showDialog (this);
}

void KateView::slotConfigDialog ()
{
  KateGlobal::self ()->configDialog (this);

  // write config to global settings, else simple programs don't get config saved ever
  // like Konqueror, Dolphin, ...
  KateGlobal::self ()->writeConfig (KGlobal::config().data());
}

void KateView::setupEditActions()
{
  //If you add an editing action to this
  //function make sure to include the line
  //m_editActions << a after creating the action
  KActionCollection* ac = actionCollection();

  KAction* a = ac->addAction("word_left");
  a->setText(i18n("Move Word Left"));
  a->setShortcuts(KStandardShortcut::backwardWord());
  connect(a, SIGNAL(triggered(bool)),  SLOT(wordLeft()));
  m_editActions << a;

  a = ac->addAction("select_char_left");
  a->setText(i18n("Select Character Left"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Left));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorLeft()));
  m_editActions << a;

  a = ac->addAction("select_word_left");
  a->setText(i18n("Select Word Left"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Left));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordLeft()));
  m_editActions << a;

  a = ac->addAction("word_right");
  a->setText(i18n("Move Word Right"));
  a->setShortcuts(KStandardShortcut::forwardWord());
  connect(a, SIGNAL(triggered(bool)), SLOT(wordRight()));
  m_editActions << a;

  a = ac->addAction("select_char_right");
  a->setText(i18n("Select Character Right"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftCursorRight()));
  m_editActions << a;

  a = ac->addAction("select_word_right");
  a->setText(i18n("Select Word Right"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Right));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftWordRight()));
  m_editActions << a;

  a = ac->addAction("beginning_of_line");
  a->setText(i18n("Move to Beginning of Line"));
  a->setShortcuts(KStandardShortcut::beginningOfLine());
  connect(a, SIGNAL(triggered(bool)), SLOT(home()));
  m_editActions << a;

  a = ac->addAction("beginning_of_document");
  a->setText(i18n("Move to Beginning of Document"));
  a->setShortcuts(KStandardShortcut::begin());
  connect(a, SIGNAL(triggered(bool)), SLOT(top()));
  m_editActions << a;

  a = ac->addAction("select_beginning_of_line");
  a->setText(i18n("Select to Beginning of Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Home));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftHome()));
  m_editActions << a;

  a = ac->addAction("select_beginning_of_document");
  a->setText(i18n("Select to Beginning of Document"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Home));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftTop()));
  m_editActions << a;


  a = ac->addAction("end_of_line");
  a->setText(i18n("Move to End of Line"));
  a->setShortcuts(KStandardShortcut::endOfLine());
  connect(a, SIGNAL(triggered(bool)), SLOT(end()));
  m_editActions << a;

  a = ac->addAction("end_of_document");
  a->setText(i18n("Move to End of Document"));
  a->setShortcuts(KStandardShortcut::end());
  connect(a, SIGNAL(triggered(bool)), SLOT(bottom()));
  m_editActions << a;

  a = ac->addAction("select_end_of_line");
  a->setText(i18n("Select to End of Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_End));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftEnd()));
  m_editActions << a;

  a = ac->addAction("select_end_of_document");
  a->setText(i18n("Select to End of Document"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_End));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottom()));
  m_editActions << a;


  a = ac->addAction("select_line_up");
  a->setText(i18n("Select to Previous Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Up));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftUp()));
  m_editActions << a;

  a = ac->addAction("scroll_line_up");
  a->setText(i18n("Scroll Line Up"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
  connect(a, SIGNAL(triggered(bool)), SLOT(scrollUp()));
  m_editActions << a;


  a = ac->addAction("move_line_down");
  a->setText(i18n("Move to Next Line"));
  a->setShortcut(QKeySequence(Qt::Key_Down));
  connect(a, SIGNAL(triggered(bool)), SLOT(down()));
  m_editActions << a;


  a = ac->addAction("move_line_up");
  a->setText(i18n("Move to Previous Line"));
  a->setShortcut(QKeySequence(Qt::Key_Up));
  connect(a, SIGNAL(triggered(bool)), SLOT(up()));
  m_editActions << a;


  a = ac->addAction("move_cursor_right");
  a->setText(i18n("Move Cursor Right"));
  a->setShortcut(QKeySequence(Qt::Key_Right));
  connect(a, SIGNAL(triggered(bool)), SLOT(cursorRight()));
  m_editActions << a;


  a = ac->addAction("move_cusor_left");
  a->setText(i18n("Move Cursor Left"));
  a->setShortcut(QKeySequence(Qt::Key_Left));
  connect(a, SIGNAL(triggered(bool)), SLOT(cursorLeft()));
  m_editActions << a;


  a = ac->addAction("select_line_down");
  a->setText(i18n("Select to Next Line"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Down));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftDown()));
  m_editActions << a;

  a = ac->addAction("scroll_line_down");
  a->setText(i18n("Scroll Line Down"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
  connect(a, SIGNAL(triggered(bool)), SLOT(scrollDown()));
  m_editActions << a;


  a = ac->addAction("scroll_page_up");
  a->setText(i18n("Scroll Page Up"));
  a->setShortcuts(KStandardShortcut::prior());
  connect(a, SIGNAL(triggered(bool)), SLOT(pageUp()));
  m_editActions << a;

  a = ac->addAction("select_page_up");
  a->setText(i18n("Select Page Up"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_PageUp));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageUp()));
  m_editActions << a;

  a = ac->addAction("move_top_of_view");
  a->setText(i18n("Move to Top of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
  connect(a, SIGNAL(triggered(bool)), SLOT(topOfView()));
  m_editActions << a;

  a = ac->addAction("select_top_of_view");
  a->setText(i18n("Select to Top of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT +  Qt::Key_PageUp));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftTopOfView()));
  m_editActions << a;


  a = ac->addAction("scroll_page_down");
  a->setText(i18n("Scroll Page Down"));
  a->setShortcuts(KStandardShortcut::next());
  connect(a, SIGNAL(triggered(bool)), SLOT(pageDown()));
  m_editActions << a;

  a = ac->addAction("select_page_down");
  a->setText(i18n("Select Page Down"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_PageDown));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftPageDown()));
  m_editActions << a;

  a = ac->addAction("move_bottom_of_view");
  a->setText(i18n("Move to Bottom of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
  connect(a, SIGNAL(triggered(bool)), SLOT(bottomOfView()));
  m_editActions << a;

  a = ac->addAction("select_bottom_of_view");
  a->setText(i18n("Select to Bottom of View"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_PageDown));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftBottomOfView()));
  m_editActions << a;

  a = ac->addAction("to_matching_bracket");
  a->setText(i18n("Move to Matching Bracket"));
  a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_6));
  connect(a, SIGNAL(triggered(bool)), SLOT(toMatchingBracket()));
  m_editActions << a;

  a = ac->addAction("select_matching_bracket");
  a->setText(i18n("Select to Matching Bracket"));
  a->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_6));
  connect(a, SIGNAL(triggered(bool)), SLOT(shiftToMatchingBracket()));
  m_editActions << a;


  // anders: shortcuts doing any changes should not be created in browserextension
  if ( !m_doc->readOnly() )
  {
    a = ac->addAction("transpose_char");
    a->setText(i18n("Transpose Characters"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(a, SIGNAL(triggered(bool)), SLOT(transpose()));
    m_editActions << a;

    a = ac->addAction("delete_line");
    a->setText(i18n("Delete Line"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K));
    connect(a, SIGNAL(triggered(bool)), SLOT(killLine()));
    m_editActions << a;

    a = ac->addAction("delete_word_left");
    a->setText(i18n("Delete Word Left"));
    a->setShortcuts(KStandardShortcut::deleteWordBack());
    connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordLeft()));
    m_editActions << a;

    a = ac->addAction("delete_word_right");
    a->setText(i18n("Delete Word Right"));
    a->setShortcuts(KStandardShortcut::deleteWordForward());
    connect(a, SIGNAL(triggered(bool)), SLOT(deleteWordRight()));
    m_editActions << a;

    a = ac->addAction("delete_next_character");
    a->setText(i18n("Delete Next Character"));
    a->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(a, SIGNAL(triggered(bool)), SLOT(keyDelete()));
    m_editActions << a;

    a = ac->addAction("backspace");
    a->setText(i18n("Backspace"));
    QList<QKeySequence> scuts;
    scuts << QKeySequence(Qt::Key_Backspace)
          << QKeySequence(Qt::SHIFT + Qt::Key_Backspace);
    a->setShortcuts(scuts);
    connect(a, SIGNAL(triggered(bool)), SLOT(backspace()));
    m_editActions << a;

    a = ac->addAction("insert_tabulator");
    a->setText(i18n("Insert Tab"));
    connect(a, SIGNAL(triggered(bool)), SLOT(insertTab()));
    m_editActions << a;

    a = ac->addAction("smart_newline");
    a->setText(i18n("Insert Smart Newline"));
    a->setWhatsThis(i18n("Insert newline including leading characters of the current line which are not letters or numbers."));
    scuts.clear();
    scuts << QKeySequence(Qt::SHIFT + Qt::Key_Return)
          << QKeySequence(Qt::SHIFT + Qt::Key_Enter);
    a->setShortcuts(scuts);
    connect(a, SIGNAL(triggered(bool)), SLOT(smartNewline()));
    m_editActions << a;

    a = ac->addAction("tools_indent");
    a->setIcon(KIcon("format-indent-more"));
    a->setText(i18n("&Indent"));
    a->setWhatsThis(i18n("Use this to indent a selected block of text.<br /><br />"
        "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_I));
    connect(a, SIGNAL(triggered(bool)), SLOT(indent()));

    a = ac->addAction("tools_unindent");
    a->setIcon(KIcon("format-indent-less"));
    a->setText(i18n("&Unindent"));
    a->setWhatsThis(i18n("Use this to unindent a selected block of text."));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_I));
    connect(a, SIGNAL(triggered(bool)), SLOT(unIndent()));
  }

  if( hasFocus() )
    slotGotFocus();
  else
    slotLostFocus();
}

void KateView::setupCodeFolding()
{
  //FIXME: FOLDING
  KActionCollection *ac=this->actionCollection();

  KAction* a;

  a = ac->addAction("folding_toplevel");
  a->setText(i18n("Fold Toplevel Nodes"));
  a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Minus));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotFoldToplevelNodes()));

  /*a = ac->addAction("folding_expandtoplevel");
  a->setText(i18n("Unfold Toplevel Nodes"));
  a->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_Plus));
  connect(a, SIGNAL(triggered(bool)), m_doc->foldingTree(), SLOT(expandToplevelNodes()));

  a = ac->addAction("folding_expandall");
  a->setText(i18n("Unfold All Nodes"));
  connect(a, SIGNAL(triggered(bool)), m_doc->foldingTree(), SLOT(expandAll()));

  a = ac->addAction("folding_collapse_dsComment");
  a->setText(i18n("Fold Multiline Comments"));
  connect(a, SIGNAL(triggered(bool)), m_doc->foldingTree(), SLOT(collapseAll_dsComments()));
*/
  a = ac->addAction("folding_collapselocal");
  a->setText(i18n("Fold Current Node"));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotCollapseLocal()));

  a = ac->addAction("folding_expandlocal");
  a->setText(i18n("Unfold Current Node"));
  connect(a, SIGNAL(triggered(bool)), SLOT(slotExpandLocal()));
}

void KateView::slotFoldToplevelNodes()
{
  // FIXME: more performant implementation
  for (int line = 0; line < doc()->lines(); ++line) {
    if (textFolding().isLineVisible(line)) {
      foldLine(line);
    }
  }
}

void KateView::slotCollapseLocal()
{
  foldLine (cursorPosition().line());
}

void KateView::slotExpandLocal()
{
  unfoldLine (cursorPosition().line());
}

void KateView::slotCollapseLevel()
{
  //FIXME: FOLDING
#if 0
  if (!sender()) return;
  QAction *action = qobject_cast<QAction*>(sender());
  if (!action) return;

  const int level = action->data().toInt();
  Q_ASSERT(level > 0);
  m_doc->foldingTree()->collapseLevel(level);
#endif
}

void KateView::slotExpandLevel()
{
  //FIXME: FOLDING
#if 0
  if (!sender()) return;
  QAction *action = qobject_cast<QAction*>(sender());
  if (!action) return;

  const int level = action->data().toInt();
  Q_ASSERT(level > 0);
  m_doc->foldingTree()->expandLevel(level);
#endif
}

void KateView::foldLine (int startLine)
{
  // only for valid lines
  if (startLine < 0)
    return;

  // try to fold all known ranges
  QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags> > startingRanges = textFolding().foldingRangesStartingOnLine (startLine);
  for (int i = 0; i < startingRanges.size(); ++i)
    textFolding().foldRange (startingRanges[i].first);

  // try if the highlighting can help us and create a fold
  textFolding().newFoldingRange (doc()->buffer().computeFoldingRangeForStartLine (startLine), Kate::TextFolding::Folded);
}

void KateView::unfoldLine (int startLine)
{
  // only for valid lines
  if (startLine < 0)
    return;

  // try to unfold all known ranges
  QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags> > startingRanges = textFolding().foldingRangesStartingOnLine (startLine);
  for (int i = 0; i < startingRanges.size(); ++i)
    textFolding().unfoldRange (startingRanges[i].first);
}

KTextEditor::View::EditMode KateView::viewEditMode() const
{
  if (viInputMode())
    return EditViMode;
  
  return isOverwriteMode() ? EditOverwrite : EditInsert;
}

QString KateView::viewMode () const
{
  /**
   * normal two modes
   */
  QString currentMode = isOverwriteMode() ? i18n("OVR") : i18n ("INS");
  
  /**
   * if we are in vi mode, this will be overwritten by current vi mode
   */
  if (viInputMode()) {
    currentMode = KateViInputModeManager::modeToString (getCurrentViMode());

    if (m_viewInternal->getViInputModeManager()->isRecordingMacro())
    {
      currentMode += " (" + i18n("recording") + ") ";
    }

    /**
     * perhaps append the current keys of a command not finalized
     */
    QString cmd = m_viewInternal->getViInputModeManager()->getVerbatimKeys();
    if (!cmd.isEmpty())
      currentMode.append (QString (" <em>%1</em>").arg (cmd));
    
    /**
     * make it bold
     */
    currentMode = QString ("<b>%1</b>").arg (currentMode);
  }
  
  /**
   * append read-only if needed
   */
  if (!m_doc->isReadWrite())
    currentMode = i18n ("%1 (R/O)", currentMode);
  
  /**
   * return full mode
   */
  return currentMode;
}

void KateView::slotGotFocus()
{
  //kDebug(13020) << "KateView::slotGotFocus";

  if ( !viInputMode() ) {
    activateEditActions();
  }
  emit focusIn ( this );
}

void KateView::slotLostFocus()
{
  //kDebug(13020) << "KateView::slotLostFocus";

  if ( !viInputMode() ) {
    deactivateEditActions();
  }

  emit focusOut ( this );
}

void KateView::setDynWrapIndicators(int mode)
{
  config()->setDynWordWrapIndicators (mode);
}

bool KateView::isOverwriteMode() const
{
  return m_doc->config()->ovr();
}

void KateView::reloadFile()
{
  // bookmarks and cursor positions are temporarily saved by the document
  m_doc->documentReload();
}

void KateView::slotReadWriteChanged ()
{
  if ( m_toggleWriteLock )
    m_toggleWriteLock->setChecked( ! m_doc->isReadWrite() );

  m_cut->setEnabled (m_doc->isReadWrite() && (selection() || m_config->smartCopyCut()));
  m_paste->setEnabled (m_doc->isReadWrite());
  m_pasteMenu->setEnabled (m_doc->isReadWrite() && !KateGlobal::self()->clipboardHistory().isEmpty());
  m_setEndOfLine->setEnabled (m_doc->isReadWrite());

  QStringList l;

  l << "edit_replace" << "tools_spelling" << "tools_indent"
      << "tools_unindent" << "tools_cleanIndent" << "tools_align"  << "tools_comment"
      << "tools_uncomment" << "tools_toggle_comment" << "tools_uppercase" << "tools_lowercase"
      << "tools_capitalize" << "tools_join_lines" << "tools_apply_wordwrap"
      << "tools_spelling_from_cursor"
      << "tools_spelling_selection" << "tools_create_snippet";

  QAction *a = 0;
  for (int z = 0; z < l.size(); z++)
    if ((a = actionCollection()->action( l[z].toAscii().constData() )))
      a->setEnabled (m_doc->isReadWrite());
  slotUpdateUndo();

  // inform search bar
  if (m_searchBar)
    m_searchBar->slotReadWriteChanged ();
    
  // => view mode changed
  emit viewModeChanged(this);
  emit viewEditModeChanged(this,viewEditMode());
}

void KateView::slotClipboardHistoryChanged ()
{
  m_pasteMenu->setEnabled (m_doc->isReadWrite() && !KateGlobal::self()->clipboardHistory().isEmpty());
}

void KateView::slotUpdateUndo()
{
  if (m_doc->readOnly())
    return;

  m_editUndo->setEnabled(m_doc->isReadWrite() && m_doc->undoCount() > 0);
  m_editRedo->setEnabled(m_doc->isReadWrite() && m_doc->redoCount() > 0);
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
  KParts::OpenUrlArguments args;
  args.setMimeType( QLatin1String("text/plain") );
  emit m_doc->browserExtension()->popupMenu( ev->globalPos(), m_doc->url(), S_IFREG, args );
  ev->accept();
}

bool KateView::setCursorPositionInternal( const KTextEditor::Cursor& position, uint tabwidth, bool calledExternally )
{
  Kate::TextLine l = m_doc->kateTextLine( position.line() );

  if (!l)
    return false;

  QString line_str = m_doc->line( position.line() );

  int x = 0;
  int z = 0;
  for (; z < line_str.length() && z < position.column(); z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  if (blockSelection())
    if (z < position.column())
      x += position.column() - z;

  m_viewInternal->updateCursor( KTextEditor::Cursor(position.line(), x), false, true, calledExternally );

  return true;
}

void KateView::toggleInsert()
{
  m_doc->config()->setOvr(!m_doc->config()->ovr());
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
  gotoBar()->updateData();
  bottomViewBar()->showBarWidget(gotoBar());
}

void KateView::changeDictionary()
{
  dictionaryBar()->updateData();
  bottomViewBar()->showBarWidget(dictionaryBar());
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
  // cursor position
  setCursorPositionInternal(KTextEditor::Cursor(config.readEntry("CursorLine",0), config.readEntry("CursorColumn",0)));

  // TODO: text folding state
//   m_savedFoldingState = config.readEntry("TextFolding", QVariantList());
//   applyFoldingState ();

  // save vi registers and jump list
  getViInputModeManager()->readSessionConfig( config );
}

void KateView::writeSessionConfig(KConfigGroup& config)
{
  // cursor position
  config.writeEntry("CursorLine",m_viewInternal->m_cursor.line());
  config.writeEntry("CursorColumn",m_viewInternal->m_cursor.column());

  // TODO: text folding state
//   saveFoldingState();
//   config.writeEntry("TextFolding", m_savedFoldingState);
//   m_savedFoldingState.clear ();

  // save vi registers and jump list
  getViInputModeManager()->writeSessionConfig( config );
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

  if (eol != m_doc->config()->eol()) {
    m_doc->setModified(true); // mark modified (bug #143120)
    m_doc->config()->setEol (eol);
  }
}

void KateView::setAddBom(bool enabled)
{
  if (!doc()->isReadWrite())
    return;

  if (m_updatingDocumentConfig)
   return;

  m_doc->config()->setBom (enabled);
  m_doc->bomSetByUser();
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

void KateView::setScrollBarMiniMap( bool enable )
{
  config()->setScrollBarMiniMap (enable);
}

void KateView::toggleScrollBarMiniMap()
{
  config()->setScrollBarMiniMap (!config()->scrollBarMiniMap());
}

void KateView::setScrollBarMiniMapAll( bool enable )
{
  config()->setScrollBarMiniMapAll (enable);
}

void KateView::toggleScrollBarMiniMapAll()
{
  config()->setScrollBarMiniMapAll (!config()->scrollBarMiniMapAll());
}

void KateView::setScrollBarMiniMapWidth( int width )
{
  config()->setScrollBarMiniMapWidth (width);
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

bool KateView::scrollBarMiniMap() {
  return m_viewInternal->m_lineScroll->showMiniMap();
}

int KateView::dynWrapIndicators() {
  return m_viewInternal->m_leftBorder->dynWrapIndicators();
}

bool KateView::foldingMarkersOn() {
  return m_viewInternal->m_leftBorder->foldingMarkersOn();
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

bool KateView::viInputMode() const
{
  return m_viewInternal->m_viInputMode;
}

bool KateView::viInputModeStealKeys() const
{
  return m_viewInternal->m_viInputModeStealKeys;
}

bool KateView::viRelativeLineNumbers() const
{
  return m_viewInternal->m_viRelLineNumbers;
}

void KateView::toggleViInputMode()
{
  config()->setViInputMode (!config()->viInputMode());

  if ( viInputMode() ) {
    m_viewInternal->getViInputModeManager()->viEnterNormalMode();
    deactivateEditActions();
  } else { // disabling the vi input mode
    activateEditActions();
  }

  emit viewModeChanged(this);
  emit viewEditModeChanged(this,viewEditMode());
}

void KateView::showViModeEmulatedCommandBar()
{
  if (viInputMode() && config()->viInputModeEmulateCommandBar()) {
    bottomViewBar()->addBarWidget(viModeEmulatedCommandBar());
    bottomViewBar()->showBarWidget(viModeEmulatedCommandBar());
  }
}

void KateView::updateViModeBarMode()
{  
  // view mode changed => status bar in container apps might change!
  emit viewModeChanged (this);
  emit viewEditModeChanged(this,viewEditMode());
}

void KateView::updateViModeBarCmd()
{
  // view mode changed => status bar in container apps might change!
  emit viewModeChanged (this);
  emit viewEditModeChanged(this,viewEditMode());
}

ViMode KateView::getCurrentViMode() const
{
  return m_viewInternal->getCurrentViMode();
}

KateViInputModeManager* KateView::getViInputModeManager()
{
  return m_viewInternal->getViInputModeManager();
}

KateViInputModeManager* KateView::resetViInputModeManager()
{
  return m_viewInternal->resetViInputModeManager();
}

void KateView::find()
{
  const bool INIT_HINT_AS_INCREMENTAL = false;
  KateSearchBar * const bar = searchBar(INIT_HINT_AS_INCREMENTAL);
  bar->enterIncrementalMode();
  bottomViewBar()->addBarWidget(bar);
  bottomViewBar()->showBarWidget(bar);
  bar->setFocus();
}

void KateView::findSelectedForwards()
{
  KateSearchBar::nextMatchForSelection(this, KateSearchBar::SearchForward);
}

void KateView::findSelectedBackwards()
{
  KateSearchBar::nextMatchForSelection(this, KateSearchBar::SearchBackward);
}

void KateView::replace()
{
  const bool INIT_HINT_AS_POWER = true;
  KateSearchBar * const bar = searchBar(INIT_HINT_AS_POWER);
  bar->enterPowerMode();
  bottomViewBar()->addBarWidget(bar);
  bottomViewBar()->showBarWidget(bar);
  bar->setFocus();
}

void KateView::findNext()
{
  searchBar()->findNext();
}

void KateView::findPrevious()
{
  searchBar()->findPrevious();
}

void KateView::slotSelectionChanged ()
{
  m_copy->setEnabled (selection() || m_config->smartCopyCut());
  m_deSelect->setEnabled (selection());

  if (m_doc->readOnly())
    return;

  m_cut->setEnabled (selection() || m_config->smartCopyCut() );

  actionCollection()->action ("tools_create_snippet")->setEnabled (selection());

  m_spell->updateActions ();
}

void KateView::switchToCmdLine ()
{
  // if the user has selected text, insert the selection's range (start line to end line) in the
  // command line when opened
  if (selectionRange().start().line() != -1 && selectionRange().end().line() != -1) {
    cmdLineBar()->setText(QString::number(selectionRange().start().line()+1)+','
        +QString::number(selectionRange().end().line()+1));
  }
  bottomViewBar()->showBarWidget(cmdLineBar());
  cmdLineBar()->setFocus ();
}

KateRenderer *KateView::renderer ()
{
  return m_renderer;
}

void KateView::updateConfig ()
{
  if (m_startingUp)
    return;

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

  // scrollbar mini-map
  m_viewInternal->m_lineScroll->setShowMiniMap( config()->scrollBarMiniMap() );
  m_toggleScrollBarMiniMap->setChecked( config()->scrollBarMiniMap() );

  // scrollbar mini-map - (whole document)
  m_viewInternal->m_lineScroll->setMiniMapAll( config()->scrollBarMiniMapAll() );
  //m_toggleScrollBarMiniMapAll->setChecked( config()->scrollBarMiniMapAll() );

  // scrollbar mini-map.width
  m_viewInternal->m_lineScroll->setMiniMapWidth( config()->scrollBarMiniMapWidth() );

  // misc edit
  m_toggleBlockSelection->setChecked( blockSelection() );
  m_toggleInsert->setChecked( isOverwriteMode() );

  // vi modes
  m_viInputModeAction->setChecked( config()->viInputMode() );

  updateFoldingConfig ();

  // bookmark
  m_bookmarks->setSorting( (KateBookmarks::Sorting) config()->bookmarkSort() );

  m_viewInternal->setAutoCenterLines(config()->autoCenterLines ());

  // vi input mode
  m_viewInternal->m_viInputMode = config()->viInputMode();

  // whether vi input mode should override actions or not
  m_viewInternal->m_viInputModeStealKeys = config()->viInputModeStealKeys();

  // whether relative line numbers should be used or not.
  m_viewInternal->m_leftBorder->setViRelLineNumbersOn(config()->viRelativeLineNumbers());

  reflectOnTheFlySpellCheckStatus(m_doc->isOnTheFlySpellCheckingEnabled());

  // register/unregister word completion...
  unregisterCompletionModel (KateGlobal::self()->wordCompletionModel());
  unregisterCompletionModel (KateGlobal::self()->keywordCompletionModel());
  if (config()->wordCompletion ())
    registerCompletionModel (KateGlobal::self()->wordCompletionModel());
  if (config()->keywordCompletion ())
    registerCompletionModel (KateGlobal::self()->keywordCompletionModel());

  // add snippet completion, later with option
  unregisterCompletionModel (KateGlobal::self()->snippetGlobal()->completionModel());
  if (1)
    registerCompletionModel (KateGlobal::self()->snippetGlobal()->completionModel());

  m_cut->setEnabled(m_doc->isReadWrite() && (selection() || m_config->smartCopyCut()));
  m_copy->setEnabled(selection() || m_config->smartCopyCut());

  // now redraw...
  m_viewInternal->cache()->clear();
  tagAll ();
  updateView (true);

  if (hasCommentInFirstLine(m_doc)) {
    if (config()->foldFirstLine()) {
      foldLine(0);
    } else {
      unfoldLine(0);
    }
  }

  emit configChanged();
}

void KateView::updateDocumentConfig()
{
  if (m_startingUp)
    return;

  m_updatingDocumentConfig = true;

  m_setEndOfLine->setCurrentItem (m_doc->config()->eol());

  m_addBom->setChecked(m_doc->config()->bom());

  m_updatingDocumentConfig = false;

  // maybe block selection or wrap-cursor mode changed
  ensureCursorColumnValid();

  // first change this
  m_renderer->setTabWidth (m_doc->config()->tabWidth());
  m_renderer->setIndentWidth (m_doc->config()->indentationWidth());

  // now redraw...
  m_viewInternal->cache()->clear();
  tagAll ();
  updateView (true);
}

void KateView::updateRendererConfig()
{
  if (m_startingUp)
    return;

  m_toggleWWMarker->setChecked( m_renderer->config()->wordWrapMarker()  );

  m_viewInternal->updateBracketMarkAttributes();
  m_viewInternal->updateBracketMarks();

  if (m_searchBar) {
    m_searchBar->updateHighlightColors();
  }

  // now redraw...
  m_viewInternal->cache()->clear();
  tagAll ();
  m_viewInternal->updateView (true);

  // update the left border right, for example linenumbers
  m_viewInternal->m_leftBorder->updateFont();
  m_viewInternal->m_leftBorder->repaint ();

  m_viewInternal->m_lineScroll->queuePixmapUpdate ();

// @@ showIndentLines is not cached anymore.
//  m_renderer->setShowIndentLines (m_renderer->config()->showIndentationLines());
  emit configChanged();
}

void KateView::updateFoldingConfig ()
{
  // folding bar
  m_viewInternal->m_leftBorder->setFoldingMarkersOn(config()->foldingBar());
  m_toggleFoldingMarkers->setChecked( config()->foldingBar() );

#if 0
  // FIXME: FOLDING
  QStringList l;

  l << "folding_toplevel" << "folding_expandtoplevel"
    << "folding_collapselocal" << "folding_expandlocal";

  QAction *a = 0;
  for (int z = 0; z < l.size(); z++)
    if ((a = actionCollection()->action( l[z].toAscii().constData() )))
      a->setEnabled (m_doc->highlight() && m_doc->highlight()->allowsFolding());
#endif
}

void KateView::ensureCursorColumnValid()
{
  KTextEditor::Cursor c = m_viewInternal->getCursor();

  // make sure the cursor is valid:
  // - in block selection mode or if wrap cursor is off, the column is arbitrary
  // - otherwise: it's bounded by the line length
  if (!blockSelection() && wrapCursor()
      && (!c.isValid() || c.column() > m_doc->lineLength(c.line())))
  {
    c.setColumn(m_doc->kateTextLine(cursorPosition().line())->length());
    setCursorPosition(c);
  }
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
  if (paintOnlyDirty)
    m_viewInternal->updateDirty();
  else
    m_viewInternal->update();
}

void KateView::updateView (bool changed)
{
  //kDebug(13020) << "KateView::updateView";

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

  if (actionCollection()->action("tools_toggle_comment"))
    actionCollection()->action("tools_toggle_comment")->setEnabled( ok );

  // show folding bar if "view defaults" says so, otherwise enable/disable only the menu entry
  updateFoldingConfig ();
}

int KateView::virtualCursorColumn() const
{
  return m_doc->toVirtualColumn(m_viewInternal->getCursor());
}

void KateView::notifyMousePositionChanged(const KTextEditor::Cursor& newPosition)
{
  emit mousePositionChanged(this, newPosition);
}

//BEGIN KTextEditor::SelectionInterface stuff

bool KateView::setSelection( const KTextEditor::Range &selection )
{
  /**
   * anything to do?
   */
  if (selection == m_selection)
    return true;

  /**
   * backup old range
   */
  KTextEditor::Range oldSelection = m_selection;

  /**
   * set new range
   */
  m_selection.setRange (selection.isEmpty() ? KTextEditor::Range::invalid() : selection);

  /**
   * trigger update of correct area
   */
  tagSelection(oldSelection);
  repaintText(true);

  /**
   * emit holy signal
   */
  emit selectionChanged (this);

  /**
   * be done
   */
  return true;
}

bool KateView::clearSelection()
{
  return clearSelection (true);
}

bool KateView::clearSelection(bool redraw, bool finishedChangingSelection)
{
  /**
   * no selection, nothing to do...
   */
  if( !selection() )
    return false;

  /**
   * backup old range
   */
  KTextEditor::Range oldSelection = m_selection;

  /**
   * invalidate current selection
   */
  m_selection.setRange (KTextEditor::Range::invalid());

  /**
   * trigger update of correct area
   */
  tagSelection(oldSelection);
  if (redraw)
    repaintText(true);

  /**
   * emit holy signal
   */
  if (finishedChangingSelection)
    emit selectionChanged (this);

  /**
   * be done
   */
  return true;
}

bool KateView::selection() const
{
  if (!wrapCursor())
    return m_selection != KTextEditor::Range::invalid();
  else
    return m_selection.toRange().isValid();
}

QString KateView::selectionText() const
{
  return m_doc->text(m_selection, blockSelect);
}

bool KateView::removeSelectedText()
{
  if (!selection())
    return false;

  m_doc->editStart ();

  // Optimization: clear selection before removing text
  KTextEditor::Range selection = m_selection;

  m_doc->removeText(selection, blockSelect);

  // don't redraw the cleared selection - that's done in editEnd().
  if (blockSelect) {
    int selectionColumn = qMin(m_doc->toVirtualColumn(selection.start()), m_doc->toVirtualColumn(selection.end()));
    KTextEditor::Range newSelection = selection;
    newSelection.start().setColumn(m_doc->fromVirtualColumn(newSelection.start().line(), selectionColumn));
    newSelection.end().setColumn(m_doc->fromVirtualColumn(newSelection.end().line(), selectionColumn));
    setSelection(newSelection);
    setCursorPositionInternal(newSelection.start());
  }
  else
    clearSelection(false);

  m_doc->editEnd ();

  return true;
}

bool KateView::selectAll()
{
  setBlockSelection (false);
  top();
  shiftBottom();
  return true;
}

bool KateView::cursorSelected(const KTextEditor::Cursor& cursor)
{
  KTextEditor::Cursor ret = cursor;
  if ( (!blockSelect) && (ret.column() < 0) )
    ret.setColumn(0);

  if (blockSelect)
    return cursor.line() >= m_selection.start().line() && ret.line() <= m_selection.end().line()
        && ret.column() >= m_selection.start().column() && ret.column() <= m_selection.end().column();
  else
    return m_selection.toRange().contains(cursor) || m_selection.end() == cursor;
}

bool KateView::lineSelected (int line)
{
  return !blockSelect && m_selection.toRange().containsLine(line);
}

bool KateView::lineEndSelected (const KTextEditor::Cursor& lineEndPos)
{
  return (!blockSelect)
    && (lineEndPos.line() > m_selection.start().line() || (lineEndPos.line() == m_selection.start().line() && (m_selection.start().column() < lineEndPos.column() || lineEndPos.column() == -1)))
    && (lineEndPos.line() < m_selection.end().line() || (lineEndPos.line() == m_selection.end().line() && (lineEndPos.column() <= m_selection.end().column() && lineEndPos.column() != -1)));
}

bool KateView::lineHasSelected (int line)
{
  return selection() && m_selection.toRange().containsLine(line);
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

    } else if (blockSelection() && (oldSelection.start().column() != m_selection.start().column() || oldSelection.end().column() != m_selection.end().column())) {
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
  // TODO: KDE5: reuse KateDocument::getWord() or rather KTextEditor::Document::wordRangeAt()
  //       avoid code duplication of selectWord() here, and KateDocument::getWord()
  int start, end, len;

  Kate::TextLine textLine = m_doc->plainKateTextLine(cursor.line());

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
  int line = cursor.line();
  if ( line+1 >= m_doc->lines() )
    setSelection (KTextEditor::Range(line, 0, line, m_doc->lineLength(line)));
  else
    setSelection (KTextEditor::Range(line, 0, line+1, 0));
}

void KateView::cut()
{
  if (!selection() && !m_config->smartCopyCut())
    return;

  copy();
  if (!selection())
    selectLine(m_viewInternal->m_cursor);
  removeSelectedText();
}

void KateView::copy() const
{
  QString text = selectionText();

  if (!selection()) {
    if (!m_config->smartCopyCut())
      return;
    text = m_doc->line(m_viewInternal->m_cursor.line()) + '\n';
    m_viewInternal->moveEdge(KateViewInternal::left, false);
  }

  // copy to clipboard and our history!
  KateGlobal::self()->copyToClipboard (text);
}

void KateView::applyWordWrap ()
{
  if (selection())
    m_doc->wrapText (selectionRange().start().line(), selectionRange().end().line());
  else
    m_doc->wrapText (0, m_doc->lastLine());
}

//END

//BEGIN KTextEditor::BlockSelectionInterface stuff

bool KateView::blockSelection () const
{
  return blockSelect;
}

bool KateView::setBlockSelection (bool on)
{
  if (on != blockSelect)
  {
    blockSelect = on;

    KTextEditor::Range oldSelection = m_selection;

    const bool hadSelection = clearSelection(false, false);

    setSelection(oldSelection);

    m_toggleBlockSelection->setChecked( blockSelection() );

    // when leaving block selection mode, if cursor is at an invalid position or past the end of the
    // line, move the cursor to the last column of the current line unless cursor wrapping is off
    ensureCursorColumnValid();

    if (!hadSelection) {
      // emit selectionChanged() according to the KTextEditor::View api
      // documentation also if there is no selection around. This is needed,
      // as e.g. the Kate App status bar uses this signal to update the state
      // of the selection mode (block selection, line based selection)
      emit selectionChanged(this);
    }
  }

  return true;
}

bool KateView::toggleBlockSelection ()
{
  m_toggleBlockSelection->setChecked (!blockSelect);
  return setBlockSelection (!blockSelect);
}

bool KateView::wrapCursor () const
{
  return !blockSelection();
}

//END


void KateView::slotTextInserted ( KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text)
{
  emit textInserted ( view, position, text);
}

bool KateView::insertTemplateTextImplementation ( const KTextEditor::Cursor& c,
                                                  const QString &templateString,
                                                  const QMap<QString,QString> &initialValues)
{
  return insertTemplateTextImplementation(c, templateString, initialValues, 0);
}

bool KateView::insertTemplateTextImplementation ( const KTextEditor::Cursor& c,
                                                  const QString &templateString,
                                                  const QMap<QString,QString> &initialValues,
                                                  KTextEditor::TemplateScript* templateScript)
{
  /**
   * no empty templates
   */
  if (templateString.isEmpty())
    return false;

  /**
   * not for read-only docs
   */
  if (!m_doc->isReadWrite())
    return false;

  /**
   * get script
   */
  KateTemplateScript* kateTemplateScript = KateGlobal::self()->scriptManager()->templateScript(templateScript);

  /**
   * the handler will delete itself when necessary
   */
  new KateTemplateHandler(this, c, templateString, initialValues, m_doc->undoManager(), kateTemplateScript);
  return true;
}


bool KateView::tagLines( KTextEditor::Range range, bool realRange )
{
  return tagLines(range.start(), range.end(), realRange);
}

void KateView::deactivateEditActions()
{
  foreach(QAction *action, m_editActions)
    action->setEnabled(false);
}

void KateView::activateEditActions()
{
  foreach(QAction *action, m_editActions)
    action->setEnabled(true);
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
  return m_config->automaticCompletionInvocation();
}

void KateView::setAutomaticInvocationEnabled(bool enabled)
{
  config()->setAutomaticCompletionInvocation(enabled);
}

void KateView::sendCompletionExecuted(const KTextEditor::Cursor& position, KTextEditor::CodeCompletionModel* model, const QModelIndex& index)
{
  emit completionExecuted(this, position, model, index);
}

void KateView::sendCompletionAborted()
{
  emit completionAborted(this);
}

void KateView::paste(const QString *textToPaste)
{
  const bool completionEnabled = isAutomaticInvocationEnabled();
  if (completionEnabled) {
    setAutomaticInvocationEnabled(false);
  }

  m_doc->paste( this, textToPaste ? *textToPaste : QApplication::clipboard()->text(QClipboard::Clipboard) );

  if (completionEnabled) {
    setAutomaticInvocationEnabled(true);
  }
}

void KateView::setCaretStyle( KateRenderer::caretStyles style, bool repaint )
{
  m_viewInternal->setCaretStyle( style, repaint );
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

KTextEditor::Cursor KateView::coordinatesToCursor(const QPoint& coords) const
{
  return m_viewInternal->coordinatesToCursor(coords);
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

QString KateView::searchPattern() const
{
    if (hasSearchBar()) {
      return m_searchBar->searchPattern();
    } else {
      return QString();
    }
}

QString KateView::replacementPattern() const
{
    if (hasSearchBar()) {
      return m_searchBar->replacementPattern();
    } else {
      return QString();
    }
}

void KateView::setSearchPattern(const QString &searchPattern)
{
  searchBar()->setSearchPattern(searchPattern);
}

void KateView::setReplacementPattern(const QString &replacementPattern)
{
  searchBar()->setReplacementPattern(replacementPattern);
}

void KateView::indent( )
{
  KTextEditor::Cursor c(cursorPosition().line(), 0);
  KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
  m_doc->indent( r, 1 );
}

void KateView::unIndent( )
{
  KTextEditor::Cursor c(cursorPosition().line(), 0);
  KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
  m_doc->indent( r, -1 );
}

void KateView::cleanIndent( )
{
  KTextEditor::Cursor c(cursorPosition().line(), 0);
  KTextEditor::Range r = selection() ? selectionRange() : KTextEditor::Range(c, c);
  m_doc->indent( r, 0 );
}

void KateView::align( )
{
  // no selection: align current line; selection: use selection range
  const int line = cursorPosition().line();
  KTextEditor::Range alignRange(KTextEditor::Cursor (line,0), KTextEditor::Cursor (line,0));
  if (selection()) {
    alignRange = selectionRange();
  }

  m_doc->align( this, alignRange );
}

void KateView::comment( )
{
  m_selection.setInsertBehaviors(Kate::TextRange::ExpandLeft | Kate::TextRange::ExpandRight);
  m_doc->comment( this, cursorPosition().line(), cursorPosition().column(), 1 );
  m_selection.setInsertBehaviors(Kate::TextRange::ExpandRight);
}

void KateView::uncomment( )
{
  m_doc->comment( this, cursorPosition().line(), cursorPosition().column(),-1 );
}

void KateView::toggleComment( )
{
  m_selection.setInsertBehaviors(Kate::TextRange::ExpandLeft | Kate::TextRange::ExpandRight);
  m_doc->comment( this, cursorPosition().line(), cursorPosition().column(), 0 );
  m_selection.setInsertBehaviors(Kate::TextRange::ExpandRight);
}

void KateView::uppercase( )
{
  m_doc->transform( this, m_viewInternal->m_cursor, KateDocument::Uppercase );
}

void KateView::killLine( )
{
  if (m_selection.isEmpty()) {
    m_doc->removeLine(cursorPosition().line());
  } else {
    m_doc->editStart();
    for (int line = m_selection.end().line(); line >= m_selection.start().line(); line--) {
      m_doc->removeLine(line);
    }
    m_doc->editEnd();
  }
}

void KateView::lowercase( )
{
  m_doc->transform( this, m_viewInternal->m_cursor, KateDocument::Lowercase );
}

void KateView::capitalize( )
{
  m_doc->editStart();
  m_doc->transform( this, m_viewInternal->m_cursor, KateDocument::Lowercase );
  m_doc->transform( this, m_viewInternal->m_cursor, KateDocument::Capitalize );
  m_doc->editEnd();
}

void KateView::keyReturn( )
{
  m_viewInternal->doReturn();
}

void KateView::smartNewline( )
{
  m_viewInternal->doSmartNewline();
}

void KateView::backspace( )
{
  m_viewInternal->doBackspace();
}

void KateView::insertTab( )
{
  m_viewInternal->doTabulator();
}

void KateView::deleteWordLeft( )
{
  m_viewInternal->doDeletePrevWord();
}

void KateView::keyDelete( )
{
  m_viewInternal->doDelete();
}

void KateView::deleteWordRight( )
{
  m_viewInternal->doDeleteNextWord();
}

void KateView::transpose( )
{
  m_viewInternal->doTranspose();
}

void KateView::cursorLeft( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->cursorNextChar();
  else
    m_viewInternal->cursorPrevChar();
}

void KateView::shiftCursorLeft( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->cursorNextChar(true);
  else
    m_viewInternal->cursorPrevChar(true);
}

void KateView::cursorRight( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->cursorPrevChar();
  else
   m_viewInternal->cursorNextChar();
}

void KateView::shiftCursorRight( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->cursorPrevChar(true);
  else
    m_viewInternal->cursorNextChar(true);
}

void KateView::wordLeft( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->wordNext();
  else
    m_viewInternal->wordPrev();
}

void KateView::shiftWordLeft( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->wordNext(true);
  else
    m_viewInternal->wordPrev(true);
}

void KateView::wordRight( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->wordPrev();
  else
    m_viewInternal->wordNext();
}

void KateView::shiftWordRight( )
{
  if (m_viewInternal->m_view->currentTextLine().isRightToLeft())
    m_viewInternal->wordPrev(true);
  else
    m_viewInternal->wordNext(true);
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

void KateView::toPrevModifiedLine()
{
  const int startLine = m_viewInternal->m_cursor.line() - 1;
  const int line = m_doc->findModifiedLine(startLine, false);
  if (line >= 0) {
    KTextEditor::Cursor c(line, 0);
    m_viewInternal->updateSelection(c, false);
    m_viewInternal->updateCursor(c);
  }
}

void KateView::toNextModifiedLine()
{
  const int startLine = m_viewInternal->m_cursor.line() + 1;
  const int line = m_doc->findModifiedLine(startLine, true);
  if (line >= 0) {
    KTextEditor::Cursor c(line, 0);
    m_viewInternal->updateSelection(c, false);
    m_viewInternal->updateCursor(c);
  }
}

const KTextEditor::Range & KateView::selectionRange( ) const
{
  // update the cache
  m_holdSelectionRangeForAPI = m_selection;

  // return cached value, has right type!
  return m_holdSelectionRangeForAPI;
}

KTextEditor::Document * KateView::document( ) const
{
  return m_doc;
}

void KateView::setContextMenu( QMenu * menu )
{
  if (m_contextMenu) {
    disconnect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
    disconnect(m_contextMenu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
  }
  m_contextMenu = menu;
  m_userContextMenuSet=true;

  if (m_contextMenu) {
    connect(m_contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
    connect(m_contextMenu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
  }
}

QMenu *KateView::contextMenu( ) const
{
  if (m_userContextMenuSet)
    return m_contextMenu;
  else
  {
    KXMLGUIClient* client = const_cast<KateView*>(this);
    while (client->parentClient())
      client = client->parentClient();

    //kDebug() << "looking up all menu containers";
    if (client->factory()){
      QList<QWidget*> conts = client->factory()->containers("menu");
      foreach (QWidget *w, conts)
      {
        if (w->objectName() == "ktexteditor_popup")
        {//perhaps optimize this block
              QMenu* menu=(QMenu*)w;
              disconnect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
              disconnect(menu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
              connect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
              connect(menu, SIGNAL(aboutToHide()), this, SLOT(aboutToHideContextMenu()));
              return menu;
        }
      }
    }
  }
  return 0;
}

QMenu * KateView::defaultContextMenu(QMenu* menu) const
{
    if (!menu)
      menu = new KMenu(const_cast<KateView*>(this));

    menu->addAction(m_editUndo);
    menu->addAction(m_editRedo);
    menu->addSeparator();
    menu->addAction(m_cut);
    menu->addAction(m_copy);
    menu->addAction(m_paste);
    menu->addSeparator();
    menu->addAction(m_selectAll);
    menu->addAction(m_deSelect);
    if (QAction *spellingSuggestions = actionCollection()->action("spelling_suggestions")) {
      menu->addSeparator();
      menu->addAction(spellingSuggestions);
    }
    if (QAction* bookmark = actionCollection()->action("bookmarks")) {
      menu->addSeparator();
      menu->addAction(bookmark);
    }
  return menu;
}

void KateView::aboutToShowContextMenu( )
{
  QMenu* menu = qobject_cast<QMenu*>(sender());

  if (menu) {
    emit contextMenuAboutToShow(this, menu);
  }
}

void KateView::aboutToHideContextMenu( )
{
  m_spellingMenu->setUseMouseForMisspelledRange(false);
}

// BEGIN ConfigInterface stff
QStringList KateView::configKeys() const
{
  return QStringList() << "icon-bar" << "line-numbers" << "dynamic-word-wrap"
                       << "background-color" << "selection-color"
                       << "search-highlight-color" << "replace-highlight-color"
                       << "folding-bar" << "icon-border-color" << "folding-marker-color"
                       << "line-number-color" << "modification-markers";
}

QVariant KateView::configValue(const QString &key)
{
  if (key == "icon-bar")
    return config()->iconBar();
  else if (key == "line-numbers")
    return config()->lineNumbers();
  else if (key == "dynamic-word-wrap")
    return config()->dynWordWrap();
  else if (key == "background-color")
    return renderer()->config()->backgroundColor();
  else if (key == "selection-color")
    return renderer()->config()->selectionColor();
  else if (key == "search-highlight-color")
    return renderer()->config()->searchHighlightColor();
  else if (key == "replace-highlight-color")
    return renderer()->config()->replaceHighlightColor();
  else if (key == "default-mark-type")
    return config()->defaultMarkType();
  else if (key == "allow-mark-menu")
    return config()->allowMarkMenu();
  else if (key == "folding-bar")
    return config()->foldingBar();
  else if (key == "icon-border-color")
    return renderer()->config()->iconBarColor();
  else if (key == "folding-marker-color")
    return renderer()->config()->foldingColor();
  else if (key == "line-number-color")
    return renderer()->config()->lineNumberColor();
  else if (key == "modification-markers")
    return config()->lineModification();

  // return invalid variant
  return QVariant();
}

void KateView::setConfigValue(const QString &key, const QVariant &value)
{
  if ( value.canConvert(QVariant::Color) ) {
     if (key == "background-color")
      renderer()->config()->setBackgroundColor(value.value<QColor>());
    else if (key == "selection-color")
      renderer()->config()->setSelectionColor(value.value<QColor>());
    else if (key == "search-highlight-color")
      renderer()->config()->setSearchHighlightColor(value.value<QColor>());
    else if (key == "replace-highlight-color")
      renderer()->config()->setReplaceHighlightColor(value.value<QColor>());
    else if (key == "icon-border-color")
      renderer()->config()->setIconBarColor(value.value<QColor>());
    else if (key == "folding-marker-color")
      renderer()->config()->setFoldingColor(value.value<QColor>());
    else if (key == "line-number-color")
      renderer()->config()->setLineNumberColor(value.value<QColor>());
  } else if ( value.type() == QVariant::Bool ) {
    // Note explicit type check above. If we used canConvert, then
    // values of type UInt will be trapped here.
    if (key == "icon-bar")
      config()->setIconBar(value.toBool());
    else if (key == "line-numbers")
      config()->setLineNumbers(value.toBool());
    else if (key == "dynamic-word-wrap")
      config()->setDynWordWrap(value.toBool());
    else if (key == "allow-mark-menu")
      config()->setAllowMarkMenu(value.toBool());
    else if (key == "folding-bar")
      config()->setFoldingBar(value.toBool());
    else if (key == "modification-markers")
      config()->setLineModification(value.toBool());
  } else if ( value.canConvert(QVariant::UInt) ) {
    if (key == "default-mark-type")
      config()->setDefaultMarkType(value.toUInt());
  }
}

// END ConfigInterface

void KateView::userInvokedCompletion()
{
  completionWidget()->userInvokedCompletion();
}

KateViewBar *KateView::topViewBar() const
{
  return m_topViewBar;
}

KateViewBar *KateView::bottomViewBar() const
{
  return m_bottomViewBar;
}

KateCommandLineBar *KateView::cmdLineBar ()
{
  if (!m_cmdLine) {
    m_cmdLine = new KateCommandLineBar (this, bottomViewBar());
    bottomViewBar()->addBarWidget(m_cmdLine);
  }

  return m_cmdLine;
}

KateSearchBar *KateView::searchBar (bool initHintAsPower)
{
  if (!m_searchBar) {
    m_searchBar = new KateSearchBar(initHintAsPower, this, KateViewConfig::global());
  }
  return m_searchBar;
}

KateGotoBar *KateView::gotoBar ()
{
  if (!m_gotoBar) {
    m_gotoBar = new KateGotoBar (this);
    bottomViewBar()->addBarWidget(m_gotoBar);
  }

  return m_gotoBar;
}

KateDictionaryBar *KateView::dictionaryBar ()
{
  if(!m_dictionaryBar) {
    m_dictionaryBar = new KateDictionaryBar(this);
    bottomViewBar()->addBarWidget(m_dictionaryBar);
  }

  return m_dictionaryBar;
}

KateViEmulatedCommandBar* KateView::viModeEmulatedCommandBar()
{
  if (!m_viModeEmulatedCommandBar) {
    m_viModeEmulatedCommandBar = new KateViEmulatedCommandBar(this, this);
    m_viModeEmulatedCommandBar->hide ();
  }

  return m_viModeEmulatedCommandBar;
}

void KateView::setAnnotationModel( KTextEditor::AnnotationModel* model )
{
  KTextEditor::AnnotationModel* oldmodel = m_annotationModel;
  m_annotationModel = model;
  m_viewInternal->m_leftBorder->annotationModelChanged(oldmodel, m_annotationModel);
}

KTextEditor::AnnotationModel* KateView::annotationModel() const
{
  return m_annotationModel;
}

void KateView::setAnnotationBorderVisible( bool visible )
{
  m_viewInternal->m_leftBorder->setAnnotationBorderOn( visible );
}

bool KateView::isAnnotationBorderVisible() const
{
  return m_viewInternal->m_leftBorder->annotationBorderOn();
}

KTextEditor::Range KateView::visibleRange()
{
  //ensure that the view is up-to-date, otherwise 'endPos()' might fail!
  m_viewInternal->updateView();
  return KTextEditor::Range(m_viewInternal->toRealCursor(m_viewInternal->startPos()),
                            m_viewInternal->toRealCursor(m_viewInternal->endPos()));
}

void KateView::toggleOnTheFlySpellCheck(bool b)
{
  m_doc->onTheFlySpellCheckingEnabled(b);
}

void KateView::reflectOnTheFlySpellCheckStatus(bool enabled)
{
  m_spellingMenu->setVisible(enabled);
  m_toggleOnTheFlySpellCheck->setChecked(enabled);
}

KateSpellingMenu* KateView::spellingMenu()
{
  return m_spellingMenu;
}

void KateView::notifyAboutRangeChange (int startLine, int endLine, bool rangeWithAttribute)
{
#ifdef VIEW_RANGE_DEBUG
  // output args
  kDebug() << "trigger attribute changed from" << startLine << "to" << endLine << "rangeWithAttribute" << rangeWithAttribute;
#endif

  // first call:
  if (!m_delayedUpdateTriggered) {
      m_delayedUpdateTriggered = true;
      m_lineToUpdateMin = -1;
      m_lineToUpdateMax = -1;

      // only set initial line range, if range with attribute!
      if (rangeWithAttribute) {
        m_lineToUpdateMin = startLine;
        m_lineToUpdateMax = endLine;
      }

      // emit queued signal and be done
      emit delayedUpdateOfView ();
      return;
  }

  // ignore lines if no attribute
  if (!rangeWithAttribute)
    return;

  // update line range
  if (startLine != -1 && (m_lineToUpdateMin == -1 || startLine < m_lineToUpdateMin))
    m_lineToUpdateMin = startLine;

  if (endLine != -1 && endLine > m_lineToUpdateMax)
    m_lineToUpdateMax = endLine;
}

void KateView::slotDelayedUpdateOfView ()
{
  if (!m_delayedUpdateTriggered)
    return;

#ifdef VIEW_RANGE_DEBUG
  // output args
  kDebug() << "delayed attribute changed from" << m_lineToUpdateMin << "to" << m_lineToUpdateMax;
#endif

  // update ranges in
  updateRangesIn (KTextEditor::Attribute::ActivateMouseIn);
  updateRangesIn (KTextEditor::Attribute::ActivateCaretIn);

  // update view, if valid line range, else only feedback update wanted anyway
  if (m_lineToUpdateMin != -1 && m_lineToUpdateMax != -1) {
    tagLines (m_lineToUpdateMin, m_lineToUpdateMax, true);
    updateView (true);
  }

  // reset flags
  m_delayedUpdateTriggered = false;
  m_lineToUpdateMin = -1;
  m_lineToUpdateMax = -1;
}

void KateView::updateRangesIn (KTextEditor::Attribute::ActivationType activationType)
{
  // new ranges with cursor in, default none
  QSet<Kate::TextRange *> newRangesIn;

  // on which range set we work?
  QSet<Kate::TextRange *> &oldSet = (activationType == KTextEditor::Attribute::ActivateMouseIn) ? m_rangesMouseIn : m_rangesCaretIn;

  // which cursor position to honor?
  KTextEditor::Cursor currentCursor =  (activationType == KTextEditor::Attribute::ActivateMouseIn) ? m_viewInternal->getMouse() : m_viewInternal->getCursor ();

  // first: validate the remembered ranges
  QSet<Kate::TextRange *> validRanges;
  foreach (Kate::TextRange *range, oldSet)
    if (m_doc->buffer().rangePointerValid(range))
      validRanges.insert (range);

  // cursor valid? else no new ranges can be found
  if (currentCursor.isValid () && currentCursor.line() < m_doc->buffer().lines()) {
    // now: get current ranges for the line of cursor with an attribute
    QList<Kate::TextRange *> rangesForCurrentCursor = m_doc->buffer().rangesForLine (currentCursor.line(), this, false);

    // match which ranges really fit the given cursor
    foreach (Kate::TextRange *range, rangesForCurrentCursor) {
      // range has no dynamic attribute of right type and no feedback object
      if ((!range->attribute() || !range->attribute()->dynamicAttribute (activationType)) && !range->feedback())
        continue;

      // range doesn't contain cursor, not interesting
      if ((range->start().insertBehavior() == KTextEditor::MovingCursor::StayOnInsert)
          ? (currentCursor < range->start().toCursor ()) : (currentCursor <= range->start().toCursor ()))
        continue;

      if ((range->end().insertBehavior() == KTextEditor::MovingCursor::StayOnInsert)
          ? (range->end().toCursor () <= currentCursor) : (range->end().toCursor () < currentCursor))
        continue;

      // range contains cursor, was it already in old set?
      if (validRanges.contains (range)) {
        // insert in new, remove from old, be done with it
        newRangesIn.insert (range);
        validRanges.remove (range);
        continue;
      }

      // oh, new range, trigger update and insert into new set
      newRangesIn.insert (range);

      if (range->attribute() && range->attribute()->dynamicAttribute (activationType))
        notifyAboutRangeChange (range->start().line(), range->end().line(), true);

      // feedback
      if (range->feedback ()) {
          if (activationType == KTextEditor::Attribute::ActivateMouseIn)
            range->feedback ()->mouseEnteredRange (range, this);
          else
            range->feedback ()->caretEnteredRange (range, this);
      }

#ifdef VIEW_RANGE_DEBUG
      // found new range for activation
      kDebug() << "activated new range" << range << "by" << activationType;
#endif
    }
  }

  // now: notify for left ranges!
  foreach (Kate::TextRange *range, validRanges) {
    // range valid + right dynamic attribute, trigger update
    if (range->toRange().isValid() && range->attribute() && range->attribute()->dynamicAttribute (activationType))
      notifyAboutRangeChange (range->start().line(), range->end().line(), true);

    // feedback
    if (range->feedback ()) {
        if (activationType == KTextEditor::Attribute::ActivateMouseIn)
          range->feedback ()->mouseExitedRange (range, this);
        else
          range->feedback ()->caretExitedRange (range, this);
    }
  }

  // set new ranges
  oldSet = newRangesIn;
}

void KateView::postMessage(KTextEditor::Message* message,
                           QList<QSharedPointer<QAction> > actions)
{
  // just forward to KateMessageWidget :-)
  if (message->position() == KTextEditor::Message::AboveView) {
    m_topMessageWidget->postMessage(message, actions);
  } else if (message->position() == KTextEditor::Message::BelowView) {
    m_bottomMessageWidget->postMessage(message, actions);
  } else if (message->position() == KTextEditor::Message::TopInView) {
    if (!m_floatTopMessageWidget) {
      m_floatTopMessageWidget = new KateMessageWidget(m_viewInternal, true);
      m_notificationLayout->insertWidget(0, m_floatTopMessageWidget, 0, Qt::AlignTop | Qt::AlignRight);
      connect(this, SIGNAL(displayRangeChanged(KateView*)), m_floatTopMessageWidget, SLOT(startAutoHideTimer()));
      connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), m_floatTopMessageWidget, SLOT(startAutoHideTimer()));
    }
    m_floatTopMessageWidget->postMessage(message, actions);
  } else if (message->position() == KTextEditor::Message::BottomInView) {
    if (!m_floatBottomMessageWidget) {
      m_floatBottomMessageWidget = new KateMessageWidget(m_viewInternal, true);
      m_notificationLayout->addWidget(m_floatBottomMessageWidget, 0, Qt::AlignBottom | Qt::AlignRight);
      connect(this, SIGNAL(displayRangeChanged(KateView*)), m_floatBottomMessageWidget, SLOT(startAutoHideTimer()));
      connect(this, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), m_floatBottomMessageWidget, SLOT(startAutoHideTimer()));
    }
    m_floatBottomMessageWidget->postMessage(message, actions);
  }
}

void KateView::saveFoldingState ()
{
  m_savedFoldingState = m_textFolding.exportFoldingRanges ();
}
    
void KateView::applyFoldingState ()
{
  m_textFolding.importFoldingRanges (m_savedFoldingState);
  m_savedFoldingState.clear ();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
