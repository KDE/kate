/* This file is part of the KDE libraries
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
#include "kateiconborder.h"
#include "kateexportaction.h"
#include "katecodefoldinghelpers.h"
#include "kateviewhighlightaction.h"
#include "katecodecompletion_iface_impl.h"
#include "katesearch.h"
#include "katebookmarks.h"
#include "katebrowserextension.h"

#include <kurldrag.h>
#include <qfocusdata.h>
#include <kdebug.h>
#include <kapp.h>
#include <kiconloader.h>
#include <qscrollbar.h>
#include <qiodevice.h>
#include <qclipboard.h>
#include <qpopupmenu.h>
#include <kpopupmenu.h>
#include <qkeycode.h>
#include <qintdict.h>
#include <klineeditdlg.h>
#include <qdropsite.h>
#include <qdragobject.h>
#include <kconfig.h>
#include <ksconfig.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qevent.h>
#include <qdir.h>
#include <qvbox.h>
#include <qpaintdevicemetrics.h>
#include <qstyle.h>

#include <kcursor.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kparts/event.h>
#include <kxmlguifactory.h>
#include <dcopclient.h>
#include <kaccel.h>

KateView::KateView( KateDocument *doc, QWidget *parent, const char * name )
    : Kate::View( doc, parent, name )
    , myDoc( doc )
    , myViewInternal( new KateViewInternal( this, doc ) )
    , m_editAccels( createEditKeys() )
    , m_search( new KateSearch( this ) )
    , m_bookmarks( new KateBookmarks( this ) )
    , m_extension( 0 )
    , m_rmbMenu( 0 )
    , m_active( false )
    , m_iconBorderStatus( KateIconBorder::None )
    , m_hasWrap( false )
{
  setInstance( KateFactory::instance() );

  initCodeCompletionImplementation();

  myViewInternal->leftBorder = new KateIconBorder(this, myViewInternal);
  myViewInternal->leftBorder->setGeometry(0, 0, myViewInternal->leftBorder->width(), myViewInternal->iconBorderHeight);
  myViewInternal->leftBorder->hide();
  myViewInternal->leftBorder->installEventFilter( this );
  connect( myViewInternal->leftBorder, SIGNAL(toggleRegionVisibility(unsigned int)),
           doc->regionTree, SLOT(toggleRegionVisibility(unsigned int)));
  connect( doc->regionTree, SIGNAL(regionVisibilityChangedAt(unsigned int)),
           this, SLOT(slotRegionVisibilityChangedAt(unsigned int)));
//  connect( doc->regionTree, SIGNAL(regionBeginEndAddedRemoved(unsigned int)),
//           this, SLOT(slotRegionBeginEndAddedRemoved(unsigned int)) );
  connect( doc, SIGNAL(codeFoldingUpdated()),
           this, SLOT(slotCodeFoldingChanged()) );

  doc->addView( this );

  connect(myViewInternal,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(dropEventPassEmited(QDropEvent *)));

  setFocusProxy( myViewInternal );
  myViewInternal->setFocus();
  resize(parent->width() -4, parent->height() -4);

  myViewInternal->installEventFilter( this );

  if (!doc->m_bSingleViewMode)
  {
    setXMLFile( "katepartui.rc" );
  }
  else
  {
    if (doc->m_bReadOnly)
      myDoc->setXMLFile( "katepartreadonlyui.rc" );
    else
      myDoc->setXMLFile( "katepartui.rc" );

    if (doc->m_bBrowserView)
    {
      m_extension = new KateBrowserExtension( this );
    }
  }

  setupActions();

  connect( this, SIGNAL( newStatus() ), this, SLOT( slotUpdate() ) );
  connect( doc, SIGNAL( undoChanged() ), this, SLOT( slotNewUndo() ) );

  if ( doc->m_bBrowserView )
  {
    connect( this, SIGNAL( dropEventPass(QDropEvent*) ), this, SLOT( slotDropEventPass(QDropEvent*) ) );
  }

  slotUpdate();

  KAccel *m_debugAccels=new KAccel(this,this);
  m_debugAccels->insert("KATE_DUMP_REGION_TREE",i18n("Show the code folding region tree"),"","Ctrl+Shift+Alt+D",myDoc,SLOT(dumpRegionTree()));
  m_debugAccels->setEnabled(true);
  if (doc->highlight()==0)
  	setFoldingMarkersOn(false);
  else
  	setFoldingMarkersOn(doc->highlight()->allowsFolding());
  myViewInternal->updateView (KateViewInternal::ufDocGeometry);
  
}

KateView::~KateView()
{
  if (myDoc && !myDoc->m_bSingleViewMode)
    myDoc->removeView( this );

  delete myViewInternal;
  delete myCC_impl;
}

void KateView::slotRegionVisibilityChangedAt(unsigned int)
{
  kdDebug()<<"void KateView::slotRegionVisibilityChangedAt(unsigned int)"<<endl;
  myViewInternal->updateView(KateViewInternal::ufFoldingChanged);
}

void KateView::slotCodeFoldingChanged()
{
  myViewInternal->leftBorder->repaint();
}

void KateView::slotRegionBeginEndAddedRemoved(unsigned int line)
{
  kdDebug()<<"void KateView::slotRegionBeginEndAddedRemoved(unsigned int)"<<endl;
//  myViewInternal->repaint();
  // FIXME: performance problem
  if (myDoc->getVirtualLine(line)<=myViewInternal->endLine)
    myViewInternal->leftBorder->repaint();
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

QPoint KateView::cursorCoordinates()
{
  return myViewInternal->cursorCoordinates();
}

void KateView::copy () const
{
  myDoc->copy(myDoc->_configFlags);
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
  
  accel->insert("KATE TRANSPOSE", i18n("Transpose two adjacent characters"),"",CTRL+Key_T,this,SLOT(transpose()));
  
  KConfig config("kateeditkeysrc");
  accel->readSettings(&config);

  if (!(myViewInternal->hasFocus())) accel->setEnabled(false);
  
  return accel;
}

void KateView::setupActions()
{
  KActionCollection *ac = this->actionCollection ();

  if (myDoc->m_bSingleViewMode)
  {
    ac = myDoc->actionCollection ();
  }

  if (!myDoc->m_bReadOnly)
  {
    KStdAction::save(this, SLOT(save()), ac);
    m_editUndo = KStdAction::undo(myDoc, SLOT(undo()), ac);
    m_editRedo = KStdAction::redo(myDoc, SLOT(redo()), ac);
    KStdAction::cut(this, SLOT(cut()), ac);
    KStdAction::paste(this, SLOT(paste()), ac);
    new KAction(i18n("Apply Word Wrap"), "", 0, myDoc, SLOT(applyWordWrap()), ac, "tools_apply_wordwrap");
    KStdAction::replace(this, SLOT(replace()), ac);
    new KAction(i18n("Editing Co&mmand"), Qt::CTRL+Qt::Key_M, this, SLOT(slotEditCommand()), ac, "tools_cmd");

    // setup Tools menu
    KStdAction::spelling(myDoc, SLOT(spellcheck()), ac);
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

  KStdAction::print(this, SLOT(print()), ac);
  
  new KAction(i18n("Reloa&d"), "reload", Key_F5, this, SLOT(reloadFile()), ac, "file_reload");
  
  KStdAction::saveAs(this, SLOT(saveAs()), ac);
  KStdAction::gotoLine(this, SLOT(gotoLine()), ac);
  new KAction(i18n("&Configure Editor..."), 0, myDoc, SLOT(configDialog()),ac, "set_confdlg");
  m_setHighlight = myDoc->hlActionMenu (i18n("&Highlight Mode"),ac,"set_highlight");
  myDoc->exportActionMenu (i18n("&Export"),ac,"file_export");
  KStdAction::selectAll(myDoc, SLOT(selectAll()), ac);
  KStdAction::deselect(myDoc, SLOT(clearSelection()), ac);
  new KAction(i18n("Increase Font Sizes"), "viewmag+", 0, this, SLOT(slotIncFontSizes()), ac, "incFontSizes");
  new KAction(i18n("Decrease Font Sizes"), "viewmag-", 0, this, SLOT(slotDecFontSizes()), ac, "decFontSizes");
  new KAction(i18n("&Toggle Block Selection"), Key_F4, myDoc, SLOT(toggleBlockSelectionMode()), ac, "set_verticalSelect");
  new KToggleAction(i18n("Show &Icon Border"), Key_F6, this, SLOT(toggleIconBorder()), ac, "view_border");
  KToggleAction* toggleAction = new KToggleAction(
     i18n("Show &Line Numbers"), Key_F11,
     0, 0,
     ac, "view_line_numbers" );
  connect( toggleAction, SIGNAL(toggled(bool)),
           this,SLOT(setLineNumbersOn(bool)) );

  QStringList list;
  m_setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, ac, "set_eol");
  connect(m_setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));
  list.clear();
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
  if (!canDiscard())
    return;
    
  // save cursor position
  uint cl = cursorLine();
  uint cc = cursorColumn();
  
  // save bookmarks
  myDoc->reloadFile();
  
  if (myDoc->numLines() >= cl)
    setCursorPosition( cl, cc );
}

void KateView::slotUpdate()
{
  slotNewUndo();
}

void KateView::slotNewUndo()
{
  if (myDoc->m_bReadOnly)
    return;

   if ((myDoc->undoCount() > 0) != m_editUndo->isEnabled())
    m_editUndo->setEnabled(myDoc->undoCount() > 0);

   if ((myDoc->redoCount() > 0) != m_editRedo->isEnabled())
    m_editRedo->setEnabled(myDoc->redoCount() > 0);
}

void KateView::slotDropEventPass( QDropEvent * ev )
{
    KURL::List lstDragURLs;
    bool ok = KURLDrag::decode( ev, lstDragURLs );

    KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( doc() );
    if ( ok && ext )
        emit ext->openURLRequest( lstDragURLs.first() );
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
    doEditCommand(KateView::cmReturn);
    break;
  case Key_Delete:
    keyDelete();
    break;
  case CTRL+Key_Delete:
    shiftWordRight();
    myDoc->removeSelectedText();
    myViewInternal->update();
    break;
  case Key_Backspace:
  case SHIFT+Key_Backspace:
    backspace();
    break;
  case CTRL+Key_Backspace:
    shiftWordLeft();
    myDoc->removeSelectedText();
    myViewInternal->update();
    break;
  case Key_Insert:
    toggleInsert();
    break;
  case CTRL+Key_K:
    killLine();
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
    if ( !m_extension || !myDoc )
        return;
    
    emit m_extension->popupMenu( ev->globalPos(), myDoc->url(),
                               QString::fromLatin1( "text/plain" ) );
    ev->accept();
}

bool KateView::setCursorPosition( uint line, uint col )
{
  setCursorPositionInternal( line, col, tabWidth() );
  return true;
}

bool KateView::setCursorPositionReal( uint line, uint col)
{
  setCursorPositionInternal( line, col, 1 );
  return true;
}

void KateView::cursorPosition( uint *line, uint *col )
{
  if ( line )
    *line = cursorLine();

  if ( col )
    *col = cursorColumn();
}

void KateView::cursorPositionReal( uint *line, uint *col )
{
  if ( line )
    *line = cursorLine();

  if ( col )
    *col = cursorColumnReal();
}

uint KateView::cursorLine() {
  return myViewInternal->getCursor().line;
}

uint KateView::cursorColumn() {
  return myDoc->currentColumn(myViewInternal->getCursor());
}

uint KateView::cursorColumnReal() {
  return myViewInternal->getCursor().col;
}

void KateView::setCursorPositionInternal(int line, int col, int tabwidth)
{
  if ((uint)line > myDoc->lastLine())
    return;

  KateTextCursor cursor;

  TextLine::Ptr textLine = myDoc->kateTextLine(line);
  QString line_str = QString(textLine->getText(), textLine->length());

  int z;
  int x = 0;
  for (z = 0; z < (int)line_str.length() && z < col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  cursor.col = x;
  cursor.line = line;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView();
}

int KateView::tabWidth() {
  return myDoc->tabChars;
}

void KateView::setTabWidth(int w) {
  myDoc->setTabWidth(w);
  myDoc->updateViews();
}

void KateView::setEncoding (QString e) {
  myDoc->setEncoding (e);
}

bool KateView::isLastView() {
  return myDoc->isLastView(1);
}

bool KateView::isOverwriteMode() const
{
  return ( myDoc->_configFlags & KateDocument::cfOvr );
}

void KateView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    myDoc->setConfigFlags( myDoc->_configFlags ^ KateDocument::cfOvr );
  else
    myDoc->setConfigFlags( myDoc->_configFlags | KateDocument::cfOvr );
}

void KateView::setDynWordWrap( bool b )
{
  if( m_hasWrap == b ) return;
  
  m_hasWrap = b;
  myViewInternal->updateView(KateViewInternal::ufDocGeometry);
}

bool KateView::dynWordWrap () const
{
  return m_hasWrap;
}

void KateView::toggleInsert() {
  myDoc->setConfigFlags(myDoc->_configFlags ^ KateDocument::cfOvr);
  emit newStatus();
}

QString KateView::currentTextLine() {
  TextLine::Ptr textLine = myDoc->kateTextLine(myViewInternal->getCursor().line);
  return QString(textLine->getText(), textLine->length());
}

QString KateView::currentWord() {
  return myDoc->getWord(myViewInternal->getCursor());
}

void KateView::insertText(const QString &s)
{
  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->insertText(c.cursor.line, c.cursor.col, s);

  // TODO: Better way to do this?
  for( uint i=0; i < s.length(); i++ ) {
    cursorRight();
  }
//  updateCursor(c.cursor);
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
  if (canDiscard()) myDoc->flush();
}

KateView::saveResult KateView::save() {
  int query = KMessageBox::Yes;
  if (doc()->isModified()) {
    if (!myDoc->url().fileName().isEmpty() && doc()->isReadWrite()) {
      // If document is new but has a name, check if saving it would
      // overwrite a file that has been created since the new doc
      // was created:
      if( myDoc->isNewDoc() )
      {
        query = checkOverwrite( myDoc->url() );
        if( query == KMessageBox::Cancel )
          return SAVE_CANCEL;
      }
      if( query == KMessageBox::Yes )
      {
         if( !myDoc->saveAs(myDoc->url()) ) {
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

    dialog = new KateFileDialog (myDoc->url().url(),doc()->encoding(), this, i18n ("Save File"), KateFileDialog::saveDialog);
    data = dialog->exec ();
    delete dialog;
    if (data.url.isEmpty())
      return SAVE_CANCEL;
    query = checkOverwrite( data.url );
  }
  while ((query != KMessageBox::Cancel) && (query != KMessageBox::Yes));

  if( query == KMessageBox::Cancel )
    return SAVE_CANCEL;

  myDoc->setEncoding (data.encoding);
  if( !myDoc->saveAs(data.url) ) {
    KMessageBox::sorry(this,
      i18n("The file could not be saved. Please check if you have write permission."));
    return SAVE_ERROR;
  }

  return SAVE_OK;
}

void KateView::cursorLeft()
{
  myViewInternal->cursorLeft();
}

void KateView::shiftCursorLeft()
{
  myViewInternal->cursorLeft(true);
}

void KateView::cursorRight()
{
  myViewInternal->cursorRight();
}

void KateView::shiftCursorRight()
{
  myViewInternal->cursorRight(true);
}

void KateView::wordLeft()
{
  myViewInternal->wordLeft();
}

void KateView::shiftWordLeft() 
{
  myViewInternal->wordLeft(true);
}

void KateView::wordRight()
{
  myViewInternal->wordRight();
}

void KateView::shiftWordRight()
{
  myViewInternal->wordRight(true);
}

void KateView::home()
{
  myViewInternal->home();
}

void KateView::shiftHome()
{
  myViewInternal->home(true);
}

void KateView::end()
{
  myViewInternal->end();
}

void KateView::shiftEnd()
{
  myViewInternal->end(true);
}

void KateView::up()
{
  myViewInternal->cursorUp();
}

void KateView::shiftUp()
{
  myViewInternal->cursorUp(true);
}

void KateView::down()
{
  myViewInternal->cursorDown();
}

void KateView::shiftDown()
{
  myViewInternal->cursorDown(true);
}

void KateView::scrollUp()
{
  myViewInternal->scrollUp();
}

void KateView::scrollDown()
{
  myViewInternal->scrollDown();
}

void KateView::topOfView()
{
  myViewInternal->topOfView();
}

void KateView::bottomOfView()
{
  myViewInternal->bottomOfView();
}

void KateView::pageUp()
{
  myViewInternal->pageUp();
}

void KateView::shiftPageUp()
{
  myViewInternal->pageUp(true);
}

void KateView::pageDown()
{
  myViewInternal->pageDown();
}

void KateView::shiftPageDown()
{
  myViewInternal->pageDown(true);
}

void KateView::top()
{
  myViewInternal->top_home();
}

void KateView::shiftTop()
{
  myViewInternal->top_home(true);
}

void KateView::bottom()
{
  myViewInternal->bottom_end();
}

void KateView::shiftBottom()
{
  myViewInternal->bottom_end(true);
}

void KateView::doEditCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  myViewInternal->doEditCommand(c, cmdNum);
}

void KateView::gotoLine()
{
  GotoLineDialog *dlg;

  dlg = new GotoLineDialog(this, myViewInternal->getCursor().line + 1, myDoc->numLines());

  if (dlg->exec() == QDialog::Accepted)
    gotoLineNumber( dlg->getLine() - 1 );

  delete dlg;
}

void KateView::gotoLineNumber( int linenumber )
{
  KateTextCursor cursor;

  cursor.col = 0;
  cursor.line = linenumber;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView();
 }

void KateView::installPopup(QPopupMenu *rmb_Menu)
{
  m_rmbMenu = rmb_Menu;
}

void KateView::readSessionConfig(KConfig *config)
{
  KateTextCursor cursor;

/*FIXME 
  myViewInternal->xPos = config->readNumEntry("XPos");
  myViewInternal->yPos = config->readNumEntry("YPos");
*/
  cursor.col = config->readNumEntry("CursorX");
  cursor.line = config->readNumEntry("CursorY");
  myViewInternal->updateCursor(cursor);
  m_iconBorderStatus = config->readNumEntry("IconBorderStatus");
  setIconBorder( m_iconBorderStatus & KateIconBorder::Icons );
  setLineNumbersOn( m_iconBorderStatus & KateIconBorder::LineNumbers );
}

void KateView::writeSessionConfig(KConfig *config)
{
/*FIXME
  config->writeEntry("XPos",myViewInternal->xPos);
  config->writeEntry("YPos",myViewInternal->yPos);
  config->writeEntry("CursorX",myViewInternal->cursor.col);
  config->writeEntry("CursorY",myViewInternal->cursor.line);
*/

  config->writeEntry("IconBorderStatus", m_iconBorderStatus );
}

int KateView::getEol() {
  return myDoc->eolMode;
}

void KateView::setEol(int eol) {
  if (!doc()->isReadWrite())
    return;

  myDoc->eolMode = eol;
  myDoc->setModified(true);
}

void KateView::slotSetEncoding(const QString& descriptiveName) {
  setEncoding(KGlobal::charsets()->encodingForName(descriptiveName));

  // myDoc->setModified(true);
  if (!doc()->isReadWrite()) {
      myDoc->reloadFile();
      // that's the only way I've found to make Kate redraw everything
      // without optimizations
      myViewInternal->tagAll();
      myViewInternal->updateView(KateViewInternal::ufFoldingChanged);
  }
}

void KateView::resizeEvent(QResizeEvent *)
{
  myViewInternal->updateView(KateViewInternal::ufRepaint);
}

void KateView::dropEventPassEmited (QDropEvent* e)
{
  emit dropEventPass(e);
}

void KateView::setFocus ()
{
  QWidget::setFocus ();

  emit gotFocus ((Kate::View *) this);
}

bool KateView::eventFilter (QObject *object, QEvent *event)
{
  if ( object == myViewInternal )
    KCursor::autoHideEventFilter( object, event );

  if ( (event->type() == QEvent::FocusIn) )
  {
    m_editAccels->setEnabled(true);
    emit gotFocus (this);
  }

  if ( (event->type() == QEvent::FocusOut) )
  {
  	m_editAccels->setEnabled(false);
  }

  if ( (event->type() == QEvent::KeyPress) )
    {
        QKeyEvent * ke=(QKeyEvent *)event;

        if ((ke->key()==Qt::Key_Tab) || (ke->key()==Qt::Key_BackTab))
          {
            myViewInternal->keyPressEvent(ke);
            return true;
          }
    }
  if (object == myViewInternal->leftBorder && event->type() == QEvent::Resize)
    updateIconBorder();
  return QWidget::eventFilter (object, event);
}

void KateView::slotEditCommand ()
{
  bool ok;
  QString cmd = KLineEditDlg::getText(i18n("Editing Command"), "", &ok, this);

  if (ok)
    myDoc->cmd()->execCmd (cmd, this);
}

void KateView::setIconBorder (bool enable)
{
  if ( enable == m_iconBorderStatus & KateIconBorder::Icons )
    return; // no change
  if ( enable )
    m_iconBorderStatus |= KateIconBorder::Icons;
  else
    m_iconBorderStatus &= ~KateIconBorder::Icons;

  updateIconBorder();
}

void KateView::toggleIconBorder ()
{
  setIconBorder ( ! (m_iconBorderStatus & KateIconBorder::Icons) );
}

void KateView::setLineNumbersOn(bool enable)
{
  if (enable == m_iconBorderStatus & KateIconBorder::LineNumbers)
    return; // no change

  if (enable)
    m_iconBorderStatus |= KateIconBorder::LineNumbers;
  else
    m_iconBorderStatus &= ~KateIconBorder::LineNumbers;

  updateIconBorder();
}

void KateView::setFoldingMarkersOn(bool enable)
{
	if (enable == bool(m_iconBorderStatus & KateIconBorder::FoldingMarkers))
		return;
	if (enable)
		m_iconBorderStatus|= KateIconBorder::FoldingMarkers;
	else
		m_iconBorderStatus&= ~KateIconBorder::FoldingMarkers;
	updateIconBorder();
}

void KateView::toggleLineNumbersOn()
{
  setLineNumbersOn( ! (m_iconBorderStatus & KateIconBorder::LineNumbers) );
}

// FIXME anders: move into KateIconBorder class
void KateView::updateIconBorder()
{
  if ( m_iconBorderStatus != KateIconBorder::None )
  {
    myViewInternal->leftBorder->show();
  }
  else
  {
    myViewInternal->leftBorder->hide();
  }
  myViewInternal->leftBorder->resize(myViewInternal->leftBorder->width(),myViewInternal->leftBorder->height());
  myViewInternal->resize(width()-myViewInternal->leftBorder->width(), myViewInternal->height());
  myViewInternal->move(myViewInternal->leftBorder->width(), 0);
  myViewInternal->updateView(0);
}

void KateView::slotIncFontSizes ()
{
  QFont font = myDoc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()+1);
  myDoc->setFont (KateDocument::ViewFont,font);
}

void KateView::slotDecFontSizes ()
{
  QFont font = myDoc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()-1);
  myDoc->setFont (KateDocument::ViewFont,font);
}

bool KateView::iconBorder() {
  return m_iconBorderStatus & KateIconBorder::Icons;
}

bool KateView::lineNumbersOn() {
  return m_iconBorderStatus & KateIconBorder::LineNumbers;
}

void KateView::showArgHint(QStringList arg1, const QString &arg2, const QString &arg3)
    	{ myCC_impl->showArgHint(arg1,arg2,arg3);}
void KateView::showCompletionBox(QValueList<KTextEditor::CompletionEntry> arg1, int arg2, bool arg3)
    	{ myCC_impl->showCompletionBox(arg1,arg2,arg3);}
