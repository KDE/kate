/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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

// $Id$

#include "kateview.h"
#include "kateview.moc"

#include <ktexteditor/plugin.h>

#include "kateviewinternal.h"
#include "katedocument.h"
#include "katecmd.h"
#include "katefactory.h"
#include "katehighlight.h"
#include "kateviewdialog.h"
#include "kateiconborder.h"
#include "katedialogs.h"
#include "katefiledialog.h"
#include "katetextline.h"
#include "kateexportaction.h"
#include "katecodefoldinghelpers.h"
#include "kateviewhighlightaction.h"
#include "katesearch.h"
#include "katebookmarks.h"
#include "katebrowserextension.h"

#include <qfont.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qevent.h>

#include <kconfig.h>
#include <klineeditdlg.h>
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
#include <kparts/event.h>
#include <kxmlguifactory.h>
#include <kaccel.h>
#include <klibloader.h>

KateView::KateView( KateDocument *doc, QWidget *parent, const char * name )
    : Kate::View( doc, parent, name )
    , m_doc( doc )
    , m_viewInternal( new KateViewInternal( this, doc ) )
    , m_editAccels( createEditKeys() )
    , m_search( new KateSearch( this ) )
    , m_bookmarks( new KateBookmarks( this ) )
    , m_rmbMenu( 0 )
    , m_active( false )
    , m_hasWrap( false )
{
  KateFactory::registerView (this);
  setInstance( KateFactory::instance() );

  initCodeCompletionImplementation();

  doc->addView( this );

  connect( m_viewInternal, SIGNAL(dropEventPass(QDropEvent*)),
           this,           SIGNAL(dropEventPass(QDropEvent*)) );

  setFocusProxy( m_viewInternal );
  m_viewInternal->setFocus();
  m_viewInternal->installEventFilter( this );

  if (!doc->m_bSingleViewMode)
  {
    setXMLFile( "katepartui.rc" );
  }
  else
  {
    if (doc->m_bReadOnly)
      setXMLFile( "katepartreadonlyui.rc" );
    else
      setXMLFile( "katepartui.rc" );

  }

  setupActions();

  connect( this, SIGNAL( newStatus() ), this, SLOT( slotUpdate() ) );
  connect( doc, SIGNAL( undoChanged() ), this, SLOT( slotNewUndo() ) );

  if ( doc->m_bBrowserView )
  {
    connect( this, SIGNAL(dropEventPass(QDropEvent*)),
             this, SLOT(slotDropEventPass(QDropEvent*)) );
  }

  slotUpdate();

  KAccel* debugAccels = new KAccel(this,this);
  debugAccels->insert("KATE_DUMP_REGION_TREE",i18n("Show the code folding region tree"),"","Ctrl+Shift+Alt+D",m_doc,SLOT(dumpRegionTree()));
  debugAccels->setEnabled(true);

  setFoldingMarkersOn( m_doc->highlight() && m_doc->highlight()->allowsFolding() );

  KTrader::OfferList::Iterator it(KateFactory::viewPlugins()->begin());
  for( ; it != KateFactory::viewPlugins()->end(); ++it)
  {
    KService::Ptr ptr = (*it);

    KLibFactory *factory = KLibLoader::self()->factory( QFile::encodeName(ptr->library()) );
    if (factory)
    {
      KTextEditor::ViewPlugin *plugin = static_cast<KTextEditor::ViewPlugin *>(factory->create(this, ptr->name().latin1(), "KTextEditor::ViewPlugin"));
      insertChildClient (plugin);
      plugin->setView (this);
    }
  }     
  
  if (doc->m_bSingleViewMode)
    doc->insertChildClient (this);
}

KateView::~KateView()
{
  if (m_doc && !m_doc->m_bSingleViewMode)
    m_doc->removeView( this );

  delete m_viewInternal;
  delete myCC_impl;
  
  KateFactory::deregisterView (this);
}

void KateView::initCodeCompletionImplementation()
{
  myCC_impl=new CodeCompletion_Impl(this);
  connect(myCC_impl,SIGNAL(completionAborted()),this,SIGNAL(completionAborted()));
  connect(myCC_impl,SIGNAL(completionDone()),this,SIGNAL(completionDone()));
  connect(myCC_impl,SIGNAL(argHintHidden()),this,SIGNAL(argHintHidden()));
  connect(myCC_impl,SIGNAL(completionDone(KTextEditor::CompletionEntry)),this,SIGNAL(completionDone(KTextEditor::CompletionEntry)));
  connect(myCC_impl,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)),this,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)));
}

void KateView::setupEditKeys()
{
  delete m_editAccels;
  m_editAccels = createEditKeys();
}

KAccel* KateView::createEditKeys()
{
  KAccel* accel = new KAccel( this, this, "edit accels" );
  
  accel->insert("KATE_CURSOR_LEFT",i18n("Cursor left"),"",Key_Left,this,SLOT(cursorLeft()));
  accel->insert("KATE_WORD_LEFT",i18n("One word left"),"",CTRL+Key_Left,this,SLOT(wordLeft()));
  accel->insert("KATE_CURSOR_LEFT_SELECT",i18n("Cursor left + SELECT"),"",SHIFT+Key_Left,this,SLOT(shiftCursorLeft()));
  accel->insert("KATE_WORD_LEFT_SELECT",i18n("One word left + SELECT"),"",SHIFT+CTRL+Key_Left,this,SLOT(shiftWordLeft()));

  accel->insert("KATE_CURSOR_RIGHT",i18n("Cursor right"),"",Key_Right,this,SLOT(cursorRight()));
  accel->insert("KATE_WORD_RIGHT",i18n("One word right"),"",CTRL+Key_Right,this,SLOT(wordRight()));
  accel->insert("KATE_CURSOR_RIGHT_SELECT",i18n("Cursor right + SELECT"),"",SHIFT+Key_Right,this,SLOT(shiftCursorRight()));
  accel->insert("KATE_WORD_RIGHT_SELECT",i18n("One word right + SELECT"),"",SHIFT+CTRL+Key_Right,this,SLOT(shiftWordRight()));

  accel->insert("KATE_CURSOR_HOME",i18n("Home"),"",Key_Home,this,SLOT(home()));
  accel->insert("KATE_CURSOR_TOP",i18n("Top"),"",CTRL+Key_Home,this,SLOT(top()));
  accel->insert("KATE_CURSOR_HOME_SELECT",i18n("Home + SELECT"),"",SHIFT+Key_Home,this,SLOT(shiftHome()));
  accel->insert("KATE_CURSOR_TOP_SELECT",i18n("Top + SELECT"),"",SHIFT+CTRL+Key_Home,this,SLOT(shiftTop()));

  accel->insert("KATE_CURSOR_END",i18n("End"),"",Key_End,this,SLOT(end()));
  accel->insert("KATE_CURSOR_BOTTOM",i18n("Bottom"),"",CTRL+Key_End,this,SLOT(bottom()));
  accel->insert("KATE_CURSOR_END_SELECT",i18n("End + SELECT"),"",SHIFT+Key_End,this,SLOT(shiftEnd()));
  accel->insert("KATE_CURSOR_BOTTOM_SELECT",i18n("Bottom + SELECT"),"",SHIFT+CTRL+Key_End,this,SLOT(shiftBottom()));

  accel->insert("KATE_CURSOR_UP",i18n("Cursor up"),"",Key_Up,this,SLOT(up()));
  accel->insert("KATE_CURSOR_UP_SELECT",i18n("Cursor up + SELECT"),"",SHIFT+Key_Up,this,SLOT(shiftUp()));
  accel->insert("KATE_SCROLL_UP",i18n("Scroll one line up"),"",CTRL+Key_Up,this,SLOT(scrollUp()));

  accel->insert("KATE_CURSOR_DOWN",i18n("Cursor down"),"",Key_Down,this,SLOT(down()));
  accel->insert("KATE_CURSOR_DOWN_SELECT",i18n("Cursor down + SELECT"),"",SHIFT+Key_Down,this,SLOT(shiftDown()));
  accel->insert("KATE_SCROLL_DOWN",i18n("Scroll one line down"),"",CTRL+Key_Down,this,SLOT(scrollDown()));
  
  accel->insert("KATE_TRANSPOSE", i18n("Transpose characters"),"",CTRL+Key_T,this,SLOT(transpose()));
  accel->insert("KATE_KILL_LINE", i18n("Delete line"),"",CTRL+Key_K,this,SLOT(killLine()));
  
  KConfig config("kateeditkeysrc");
  accel->readSettings(&config);

  if (!(m_viewInternal->hasFocus())) accel->setEnabled(false);
  
  return accel;
}

void KateView::setupActions()
{
  KActionCollection *ac = this->actionCollection ();

  if (!m_doc->m_bReadOnly)
  {
    KStdAction::save(this, SLOT(save()), ac);
    m_editUndo = KStdAction::undo(m_doc, SLOT(undo()), ac);
    m_editRedo = KStdAction::redo(m_doc, SLOT(redo()), ac);
    KStdAction::cut(this, SLOT(cut()), ac);
    KStdAction::paste(this, SLOT(paste()), ac);
    new KAction(i18n("Apply Word Wrap"), "", 0, m_doc, SLOT(applyWordWrap()), ac, "tools_apply_wordwrap");
    new KAction(i18n("Editing Co&mmand"), Qt::CTRL+Qt::Key_M, this, SLOT(slotEditCommand()), ac, "tools_cmd");

    // setup Tools menu
    new KAction(i18n("&Indent"), "indent", Qt::CTRL+Qt::Key_I, this, SLOT(indent()),
                              ac, "tools_indent");
    new KAction(i18n("&Unindent"), "unindent", Qt::CTRL+Qt::SHIFT+Qt::Key_I, this, SLOT(unIndent()),
                                ac, "tools_unindent");
    new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()),
                                   ac, "tools_cleanIndent");
    new KAction(i18n("C&omment"), CTRL+Qt::Key_NumberSign, this, SLOT(comment()),
                               ac, "tools_comment");
    new KAction(i18n("Unco&mment"), CTRL+SHIFT+Qt::Key_NumberSign, this, SLOT(uncomment()),
                                 ac, "tools_uncomment");
  }

  KStdAction::copy(this, SLOT(copy()), ac);

  KStdAction::print( m_doc, SLOT(print()), ac );
  
  new KAction(i18n("Reloa&d"), "reload", Key_F5, this, SLOT(reloadFile()), ac, "file_reload");
  
  KStdAction::saveAs(this, SLOT(saveAs()), ac);
  KStdAction::gotoLine(this, SLOT(gotoLine()), ac);
  new KAction(i18n("&Configure Editor..."), 0, m_doc, SLOT(configDialog()),ac, "set_confdlg");
  m_setHighlight = m_doc->hlActionMenu (i18n("&Highlight Mode"),ac,"set_highlight");
  m_doc->exportActionMenu (i18n("&Export"),ac,"file_export");
  KStdAction::selectAll(m_doc, SLOT(selectAll()), ac);
  KStdAction::deselect(m_doc, SLOT(clearSelection()), ac);
  new KAction(i18n("Increase Font Sizes"), "viewmag+", 0, this, SLOT(slotIncFontSizes()), ac, "incFontSizes");
  new KAction(i18n("Decrease Font Sizes"), "viewmag-", 0, this, SLOT(slotDecFontSizes()), ac, "decFontSizes");
  new KAction(i18n("&Toggle Block Selection"), Key_F4, m_doc, SLOT(toggleBlockSelectionMode()), ac, "set_verticalSelect");
  
  m_toggleFoldingMarkers = new KToggleAction(
    i18n("Show &Folding Markers"), Key_F9,
    this, SLOT(toggleFoldingMarkers()),
    ac, "view_folding_markers" );
  connect( m_doc, SIGNAL(hlChanged()),
           this, SLOT(updateFoldingMarkersAction()) );
  updateFoldingMarkersAction();
  KToggleAction* toggleAction = new KToggleAction(
    i18n("Show &Icon Border"), Key_F6,
    this, SLOT(toggleIconBorder()),
    ac, "view_border");
  toggleAction->setChecked( iconBorder() );
  toggleAction = new KToggleAction(
     i18n("Show &Line Numbers"), Key_F11,
     this, SLOT(toggleLineNumbersOn()),
     ac, "view_line_numbers" );
  toggleAction->setChecked( lineNumbersOn() );

  m_setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, ac, "set_eol");
  connect(m_setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));
  QStringList list;
  list.append("&Unix");
  list.append("&Windows/Dos");
  list.append("&Macintosh");
  m_setEndOfLine->setItems(list);

  m_setEncoding = new KSelectAction(i18n("Set &Encoding"), 0, ac, "set_encoding");
  connect(m_setEncoding, SIGNAL(activated(const QString&)), this, SLOT(slotSetEncoding(const QString&)));
  list = KGlobal::charsets()->descriptiveEncodingNames();
  list.prepend( i18n( "Auto" ) );
  m_setEncoding->setItems(list);
  
  m_search->createActions( ac );
  m_bookmarks->createActions( ac );
}

void KateView::reloadFile()
{
  // save cursor position
  uint cl = cursorLine();
  uint cc = cursorColumn();
  
  // save bookmarks
  m_doc->reloadFile();
  
  if (m_doc->numLines() >= cl)
    setCursorPosition( cl, cc );
}

void KateView::slotUpdate()
{
  slotNewUndo();
}

void KateView::slotNewUndo()
{
  if (m_doc->m_bReadOnly)
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

void KateView::updateFoldingMarkersAction()
{
  m_toggleFoldingMarkers->setChecked( foldingMarkersOn() );
  m_toggleFoldingMarkers->setEnabled( m_doc->highlight() && m_doc->highlight()->allowsFolding() );
}

void KateView::keyPressEvent( QKeyEvent *ev )
{
  switch( KKey(ev).keyCodeQt() ) {
  case Key_PageUp:
    pageUp();
    break;
  case SHIFT+Key_PageUp:
    shiftPageUp();
    break;
  case CTRL+Key_PageUp:
    topOfView();
    break;
  case Key_PageDown:
    pageDown();
    break;
  case SHIFT+Key_PageDown:
    shiftPageDown();
    break;
  case CTRL+Key_PageDown:
    bottomOfView();
    break;
  case Key_Return:
  case Key_Enter:
    keyReturn();
    break;
  case Key_Delete:
    keyDelete();
    break;
  case CTRL+Key_Delete:
    shiftWordRight();
    m_doc->removeSelectedText();
    m_viewInternal->update();
    break;
  case Key_Backspace:
  case SHIFT+Key_Backspace:
    backspace();
    break;
  case CTRL+Key_Backspace:
    shiftWordLeft();
    m_doc->removeSelectedText();
    m_viewInternal->update();
    break;
  case Key_Insert:
    toggleInsert();
    break;
  default:
    KTextEditor::View::keyPressEvent( ev );
    return;
  }
  ev->accept();
}

void KateView::customEvent( QCustomEvent *ev )
{
    if ( KParts::GUIActivateEvent::test( ev ) && static_cast<KParts::GUIActivateEvent *>( ev )->activated() )
    {
        installPopup(static_cast<QPopupMenu *>(factory()->container("katepart_popup", this) ) );
        return;
    }

    KTextEditor::View::customEvent( ev );
}

void KateView::contextMenuEvent( QContextMenuEvent *ev )
{
    if ( !m_doc || !m_doc->m_extension  )
        return;
    
    emit m_doc->m_extension->popupMenu( ev->globalPos(), m_doc->url(),
                                        QString::fromLatin1( "text/plain" ) );
    ev->accept();
}

bool KateView::setCursorPositionInternal( uint line, uint col, uint tabwidth )
{
  if( line > m_doc->lastLine() )
    return false;

  QString line_str = m_doc->textLine( line );

  uint z;
  uint x = 0;
  for (z = 0; z < line_str.length() && z < col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  m_viewInternal->updateCursor( KateTextCursor( line, x ) );
  m_viewInternal->centerCursor();
  
  return true;
}

void KateView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    m_doc->setConfigFlags( m_doc->_configFlags ^ KateDocument::cfOvr );
  else
    m_doc->setConfigFlags( m_doc->_configFlags | KateDocument::cfOvr );
}

void KateView::toggleInsert() {
  m_doc->setConfigFlags(m_doc->_configFlags ^ KateDocument::cfOvr);
  emit newStatus();
}

bool KateView::canDiscard() {
  int query;

  if (doc()->isModified()) {
    query = KMessageBox::warningYesNoCancel(this,
      i18n("The current Document has been modified.\nWould you like to save it?"));
    switch (query) {
      case KMessageBox::Yes: //yes
        if (save() == SAVE_CANCEL) return false;
        if (doc()->isModified()) {
            query = KMessageBox::warningContinueCancel(this,
               i18n("Could not save the document.\nDiscard it and continue?"),
           QString::null, i18n("&Discard"));
          if (query == KMessageBox::Cancel) return false;
        }
        break;
      case KMessageBox::Cancel: //cancel
        return false;
    }
  }
  return true;
}

void KateView::flush()
{
  if (canDiscard()) m_doc->flush();
}

KateView::saveResult KateView::save() {
  int query = KMessageBox::Yes;
  if (doc()->isModified()) {
    if (!m_doc->url().fileName().isEmpty() && doc()->isReadWrite()) {
      // If document is new but has a name, check if saving it would
      // overwrite a file that has been created since the new doc
      // was created:
      if( m_doc->isNewDoc() )
      {
        query = checkOverwrite( m_doc->url() );
        if( query == KMessageBox::Cancel )
          return SAVE_CANCEL;
      }
      if( query == KMessageBox::Yes )
      {
         if( !m_doc->saveAs(m_doc->url()) ) {
       KMessageBox::sorry(this,
        i18n("The file could not be saved. Please check if you have write permission."));
      return SAVE_ERROR;
  }
      }
      else  // Do not overwrite already existing document:
        return saveAs();
    } // New, unnamed document:
    else
      return saveAs();
  }
  return SAVE_OK;
}

/*
 * Check if the given URL already exists. Currently used by both save() and saveAs()
 *
 * Asks the user for permission and returns the message box result and defaults to
 * KMessageBox::Yes in case of doubt
 */
int KateView::checkOverwrite( KURL u )
{
  int query = KMessageBox::Yes;

  if( u.isLocalFile() )
  {
    QFileInfo info;
    QString name( u.path() );
    info.setFile( name );
    if( info.exists() )
      query = KMessageBox::warningYesNoCancel( this,
        i18n( "A Document with this Name already exists.\nDo you want to overwrite it?" ) );
  }
  return query;
}

KateView::saveResult KateView::saveAs() {
  int query;
  KateFileDialog *dialog;
  KateFileDialogData data;

  do {
    query = KMessageBox::Yes;

    dialog = new KateFileDialog (m_doc->url().url(),doc()->encoding(), this, i18n ("Save File"), KateFileDialog::saveDialog);
    data = dialog->exec ();
    delete dialog;
    if (data.url.isEmpty())
      return SAVE_CANCEL;
    query = checkOverwrite( data.url );
  }
  while ((query != KMessageBox::Cancel) && (query != KMessageBox::Yes));

  if( query == KMessageBox::Cancel )
    return SAVE_CANCEL;

  m_doc->setEncoding (data.encoding);
  if( !m_doc->saveAs(data.url) ) {
    KMessageBox::sorry(this,
      i18n("The file could not be saved. Please check if you have write permission."));
    return SAVE_ERROR;
  }

  return SAVE_OK;
}

void KateView::gotoLine()
{
  GotoLineDialog *dlg;

  dlg = new GotoLineDialog(this, m_viewInternal->getCursor().line + 1, m_doc->numLines());

  if (dlg->exec() == QDialog::Accepted)
    gotoLineNumber( dlg->getLine() - 1 );

  delete dlg;
}

void KateView::gotoLineNumber( int line )
{
  m_viewInternal->updateCursor( KateTextCursor( line, 0 ) );
  m_viewInternal->centerCursor();
}

void KateView::readSessionConfig(KConfig *config)
{
  KateTextCursor cursor;

/*FIXME 
  m_viewInternal->xPos = config->readNumEntry("XPos");
  m_viewInternal->yPos = config->readNumEntry("YPos");
*/
  cursor.col = config->readNumEntry("CursorX");
  cursor.line = config->readNumEntry("CursorY");
  m_viewInternal->updateCursor(cursor);
/*  m_viewInternal->m_iconBorderStatus = config->readNumEntry("IconBorderStatus");
  setIconBorder( m_viewInternal->m_iconBorderStatus & KateIconBorder::Icons );
  setLineNumbersOn( m_viewInternal->m_iconBorderStatus & KateIconBorder::LineNumbers );*/
}

void KateView::writeSessionConfig(KConfig */*config*/)
{
/*FIXME
  config->writeEntry("XPos",m_viewInternal->xPos);
  config->writeEntry("YPos",m_viewInternal->yPos);
  config->writeEntry("CursorX",m_viewInternal->cursor.col);
  config->writeEntry("CursorY",m_viewInternal->cursor.line);
*/

//  config->writeEntry("IconBorderStatus", m_viewInternal->m_iconBorderStatus );
}

void KateView::setEol(int eol) {
  if (!doc()->isReadWrite())
    return;

  m_doc->eolMode = eol;
  m_doc->setModified(true);
}

void KateView::slotSetEncoding( const QString& descriptiveName )
{
  setEncoding( KGlobal::charsets()->encodingForName( descriptiveName ) );

  if( !doc()->isReadWrite() ) {
      m_doc->reloadFile();
      m_viewInternal->tagAll();
  }
}

void KateView::resizeEvent(QResizeEvent *)
{
  // resize viewinternal
  m_viewInternal->resize(width(),height());
}

void KateView::setFocus ()
{
  QWidget::setFocus ();

  emit gotFocus( this );
}

bool KateView::eventFilter (QObject *object, QEvent *event)
{
  if ( object == m_viewInternal )
    KCursor::autoHideEventFilter( object, event );

  if ( event->type() == QEvent::FocusIn ) {
    m_editAccels->setEnabled(true);
    emit gotFocus( this );
  }

  if ( event->type() == QEvent::FocusOut ) {
    m_editAccels->setEnabled(false);
  }

  if ( event->type() == QEvent::KeyPress ) {
    QKeyEvent* ke = (QKeyEvent*)event;
    if ((ke->key()==Qt::Key_Tab) || (ke->key()==Qt::Key_BackTab)) {
      m_viewInternal->keyPressEvent(ke);
      return true;
    }
  }

  return QWidget::eventFilter (object, event);
}

void KateView::slotEditCommand ()
{
  bool ok;
  QString cmd = KLineEditDlg::getText(i18n("Editing Command"), "", &ok, this);

  if (ok)
    m_doc->cmd()->execCmd (cmd, this);
}

void KateView::setIconBorder( bool enable )
{
  m_viewInternal->leftBorder->setIconBorderOn( enable );
}

void KateView::toggleIconBorder()
{
  m_viewInternal->leftBorder->toggleIconBorder();
}

void KateView::setLineNumbersOn( bool enable )
{
  m_viewInternal->leftBorder->setLineNumbersOn( enable );
}

void KateView::toggleLineNumbersOn()
{
  m_viewInternal->leftBorder->toggleLineNumbers();
}

void KateView::setFoldingMarkersOn( bool enable )
{
  m_viewInternal->leftBorder->setFoldingMarkersOn( enable );
}

void KateView::toggleFoldingMarkers()
{
  m_viewInternal->leftBorder->toggleFoldingMarkers();
}

bool KateView::iconBorder() {
  return m_viewInternal->leftBorder->iconBorderOn();
}

bool KateView::lineNumbersOn() {
  return m_viewInternal->leftBorder->lineNumbersOn();
}

bool KateView::foldingMarkersOn() {
  return m_viewInternal->leftBorder->foldingMarkersOn();
}

void KateView::slotIncFontSizes ()
{
  QFont font = m_doc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()+1);
  m_doc->setFont (KateDocument::ViewFont,font);
}

void KateView::slotDecFontSizes ()
{
  QFont font = m_doc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()-1);
  m_doc->setFont (KateDocument::ViewFont,font);
}
