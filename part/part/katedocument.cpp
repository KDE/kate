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


//BEGIN includes
#include "katedocument.h"
#include "katedocument.moc"

#include "katefactory.h"
#include "kateviewdialog.h"
#include "katedialogs.h"
#include "katebuffer.h"
#include "katetextline.h"
#include "katecmd.h"
#include "kateglobal.h"
#include "kateexportaction.h"
#include "katecodefoldinghelpers.h"

#include <qfileinfo.h>
#include <qfile.h>
#include <qfocusdata.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qpaintdevicemetrics.h>
#include <qiodevice.h>
#include <qclipboard.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qobject.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qptrstack.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kurldrag.h>
#include <kprinter.h>
#include <kapp.h>
#include <kpopupmenu.h>
#include <klineeditdlg.h>
#include <kconfig.h>
#include <ksconfig.h>
#include <kcursor.h>
#include <kcharsets.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kparts/event.h>
#include <kiconloader.h>
#include <kxmlguifactory.h>
#include <dcopclient.h>
#include <kwin.h>
#include <kdialogbase.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kglobalsettings.h>
#include <ksavefile.h>

#include "kateviewhighlightaction.h"

//END  includes

class KateUndo
{
  friend class KateUndoGroup;

  public:
    KateUndo (KateDocument *doc, uint type, uint line, uint col, uint len,  QString text);
    ~KateUndo ();

  private:
    void undo ();
    void redo ();

  public:
    enum types
    {
      editInsertText,
      editRemoveText,
      editWrapLine,
      editUnWrapLine,
      editInsertLine,
      editRemoveLine
    };

  private:
    KateDocument *myDoc;
    uint type;
    uint line;
    uint col;
    uint len;
    QString text;
};

class KateUndoGroup
{
  public:
    KateUndoGroup (KateDocument *doc);
    ~KateUndoGroup ();

    void undo ();
    void redo ();

    void addItem (KateUndo *undo);

  private:
    KateDocument *myDoc;
    QPtrList<KateUndo> items;
};

QStringList KateDocument::searchForList = QStringList();
QStringList KateDocument::replaceWithList = QStringList();

KateUndo::KateUndo (KateDocument *doc, uint type, uint line, uint col, uint len, QString text)
{
  this->myDoc = doc;
  this->type = type;
  this->line = line;
  this->col = col;
  this->len = len;
  this->text = text;
}

KateUndo::~KateUndo ()
{
}

void KateUndo::undo ()
{
  if (type == KateUndo::editInsertText)
  {
    myDoc->editRemoveText (line, col, len);
  }
  else if (type == KateUndo::editRemoveText)
  {
    myDoc->editInsertText (line, col, text);
  }
  else if (type == KateUndo::editWrapLine)
  {
    myDoc->editUnWrapLine (line, col);
  }
  else if (type == KateUndo::editUnWrapLine)
  {
    myDoc->editWrapLine (line, col);
  }
  else if (type == KateUndo::editInsertLine)
  {
    myDoc->editRemoveLine (line);
  }
  else if (type == KateUndo::editRemoveLine)
  {
    myDoc->editInsertLine (line, text);
  }
}

void KateUndo::redo ()
{
  if (type == KateUndo::editRemoveText)
  {
    myDoc->editRemoveText (line, col, len);
  }
  else if (type == KateUndo::editInsertText)
  {
    myDoc->editInsertText (line, col, text);
  }
  else if (type == KateUndo::editUnWrapLine)
  {
    myDoc->editUnWrapLine (line, col);
  }
  else if (type == KateUndo::editWrapLine)
  {
    myDoc->editWrapLine (line, col);
  }
  else if (type == KateUndo::editRemoveLine)
  {
    myDoc->editRemoveLine (line);
  }
  else if (type == KateUndo::editInsertLine)
  {
    myDoc->editInsertLine (line, text);
  }
}

KateUndoGroup::KateUndoGroup (KateDocument *doc)
{
  myDoc = doc;
}

KateUndoGroup::~KateUndoGroup ()
{
}

void KateUndoGroup::undo ()
{
  if (items.count() == 0)
    return;

  myDoc->editStart (false);

  for (int pos=(int)items.count()-1; pos >= 0; pos--)
  {
    items.at(pos)->undo();

    if (myDoc->myActiveView != 0L)
    {
      myDoc->myActiveView->myViewInternal->cursorCache.line = items.at(pos)->line;
      myDoc->myActiveView->myViewInternal->cursorCache.col = items.at(pos)->col;
      myDoc->myActiveView->myViewInternal->cursorCacheChanged = true;
    }
  }

  myDoc->editEnd ();
}

void KateUndoGroup::redo ()
{
  if (items.count() == 0)
    return;

  myDoc->editStart (false);

  for (uint pos=0; pos < items.count(); pos++)
  {
    items.at(pos)->redo();
    
    if (myDoc->myActiveView != 0L)
    {
      myDoc->myActiveView->myViewInternal->cursorCache.line = items.at(pos)->line;
      myDoc->myActiveView->myViewInternal->cursorCache.col = items.at(pos)->col;
      myDoc->myActiveView->myViewInternal->cursorCacheChanged = true;
    }
  }

  myDoc->editEnd ();
}

void KateUndoGroup::addItem (KateUndo *undo)
{
  items.append (undo);
}

//
// KateDocument Constructor
//
KateDocument::KateDocument(bool bSingleViewMode, bool bBrowserView, bool bReadOnly,
                                           QWidget *parentWidget, const char *widgetName,
                                           QObject *, const char *)
  : Kate::Document (), viewFont(), printFont(),hlManager(HlManager::self ())
{
  regionTree=new KateCodeFoldingTree(this);
  myActiveView = 0L;

  hlSetByUser = false;
  setInstance( KateFactory::instance() );

  editSessionNumber = 0;
  editIsRunning = false;
  noViewUpdates = false;
  editCurrentUndo = 0L;
  editWithUndo = false;

  pseudoModal = 0L;
  blockSelect = false;
  restoreMarks = false;

  m_bSingleViewMode = bSingleViewMode;
  m_bBrowserView = bBrowserView;
  m_bReadOnly = bReadOnly;

  myMarks.setAutoDelete (true);

  selectStart.line = -1;
  selectStart.col = -1;
  selectEnd.line = -1;
  selectEnd.col = -1;
  selectAnchor.line = -1;
  selectAnchor.col = -1;

  newDocGeometry = false;
  readOnly = false;
  newDoc = false;

  modified = false;
  m_highlightedEnd = 0;

  // some defaults
  _configFlags = KateDocument::cfAutoIndent | KateDocument::cfTabIndents | KateDocument::cfKeepIndentProfile
    | KateDocument::cfRemoveSpaces
    | KateDocument::cfDelOnInput | KateDocument::cfWrapCursor
    | KateDocument::cfShowTabs | KateDocument::cfSmartHome;

  _searchFlags = 0;

  //KSpell initial values
  kspell.kspell = 0;
  kspell.ksc = new KSpellConfig; //default KSpellConfig to start
  kspell.kspellon = false;

  myEncoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());

  setFont (ViewFont,KGlobalSettings::fixedFont());
  setFont (PrintFont,KGlobalSettings::fixedFont());

  myDocName = QString ("");
  fileInfo = new QFileInfo ();

  myCmd = new KateCmd (this);

  connect(this,SIGNAL(modifiedChanged ()),this,SLOT(slotModChanged ()));

  buffer = new KateBuffer (this);
  connect(buffer, SIGNAL(linesChanged(int)), this, SLOT(slotBufferChanged()));

  connect(buffer, SIGNAL(tagLines(int,int)), this, SLOT(tagLines(int,int)));
  connect(buffer, SIGNAL(pleaseHighlight(uint,uint)),this,SLOT(slotBufferUpdateHighlight(uint,uint)));

  connect(buffer,SIGNAL(foldingUpdate(unsigned int , QMemArray<signed char>*,bool*,bool)),regionTree,SLOT(updateLine(unsigned int, QMemArray<signed char>*,bool *,bool)));
  connect(regionTree,SIGNAL(setLineVisible(unsigned int, bool)), buffer,SLOT(setLineVisible(unsigned int,bool)));
  connect(buffer,SIGNAL(codeFoldingUpdated()),this,SIGNAL(codeFoldingUpdated()));
  m_highlightTimer = new QTimer(this);
  connect(m_highlightTimer, SIGNAL(timeout()), this, SLOT(slotBufferUpdateHighlight()));


  colors[0] = KGlobalSettings::baseColor();
  colors[1] = KGlobalSettings::highlightColor();

  m_highlight = 0L;
  tabChars = 8;

  clear();

  // if the user changes the highlight with the dialog, notify the doc
  connect(hlManager,SIGNAL(changed()),SLOT(internalHlChanged()));

  readConfig();

  if ( m_bSingleViewMode )
  {
    KTextEditor::View *view = createView( parentWidget, widgetName );
    view->show();
    setWidget( view );
  }
}

//
// KateDocument Destructor
//
KateDocument::~KateDocument()
{
  if ( !m_bSingleViewMode )
  {
    myViews.setAutoDelete( true );
    myViews.clear();
    myViews.setAutoDelete( false );
  }

  if (kspell.kspell)
  {
    kspell.kspell->setAutoDelete(true);
    kspell.kspell->cleanUp(); // need a way to wait for this to complete
  }

  if (kspell.ksc)
    delete kspell.ksc;

  m_highlight->release();
  myMarks.clear ();
}

//
// KTextEditor::Document stuff
//

KTextEditor::View *KateDocument::createView( QWidget *parent, const char *name )
{
  return new KateView( this, parent, name);
}

QPtrList<KTextEditor::View> KateDocument::views () const
{
  return _views;
};

//
// KTextEditor::EditInterface stuff
//

QString KateDocument::text() const
{
  QString s;

  for (uint i=0; i < buffer->count(); i++)
  {
    TextLine::Ptr textLine = buffer->line(i);
    s.append (textLine->getString());
    if ( (i < (buffer->count()-1)) )
      s.append('\n');
  }

  return s;
}

QString KateDocument::text ( uint startLine, uint startCol, uint endLine, uint endCol ) const
{
  QString s;

  for (uint i=startLine; (i <= endLine) && (i < buffer->count()); i++)
  {
    TextLine::Ptr textLine = buffer->line(i);

    if (i == startLine)
      s.append(textLine->getString().mid (startCol, textLine->length()-startCol));
    else if (i == endLine)
      s.append(textLine->getString().mid (0, endCol));
    else
      s.append(textLine->getString());

    if ( i < endLine )
      s.append('\n');
  }

  return s;
}

QString KateDocument::textLine( uint line ) const
{
  return buffer->plainLine(line);
}

bool KateDocument::setText(const QString &s)
{
  clear();
  return insertText (0, 0, s);
}

bool KateDocument::clear()
{
  KateTextCursor cursor;
  KateView *view;

  setPseudoModal(0L);

  cursor.col = cursor.line = 0;
  for (view = myViews.first(); view != 0L; view = myViews.next() ) {
    view->myViewInternal->clear();//updateCursor(cursor);
    view->myViewInternal->tagAll();
  }

  eolMode = KateDocument::eolUnix;

  buffer->clear();
  clearMarks ();

  clearUndo();
  clearRedo();

  setModified(false);

  internalSetHlMode(0); //calls updateFontData()

  return true;
}

bool KateDocument::insertText( uint line, uint col, const QString &s )
{
  if (s.isEmpty())
    return true;

  uint insertPos = col;
  uint len = s.length();
  QChar ch;
  QString buf;
  uint endLine, endCol, startLine, startCol;

  startLine = line;
  startCol = col;
  endLine = line;
  endCol = col;

  editStart ();

  for (uint pos = 0; pos < len; pos++)
  {
    ch = s[pos];

    if (ch == '\n')
    {
      editInsertText (line, insertPos, buf);
      editWrapLine (line, insertPos + buf.length());

      line++;
      endLine++;
      insertPos = 0;
      buf.truncate(0);
    }
    else
      buf += ch; // append char to buffer
  }

  editInsertText (line, insertPos, buf);

  editEnd ();

  return true;
}

bool KateDocument::removeText ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  TextLine::Ptr l, tl;
  uint deletePos = 0;
  uint endPos = 0;
  uint line = 0;

  l = getTextLine(startLine);

  if (!l)
    return false;

  editStart ();

  if (startLine == endLine)
  {
    editRemoveText (startLine, startCol, endCol-startCol);
  }
  else if ((startLine+1) == endLine)
  {
    editRemoveText (startLine, startCol, l->length()-startCol);
    editRemoveText (startLine+1, 0, endCol);
    editUnWrapLine (startLine, startCol);
  }
  else
  {
    for (line = startLine; line <= endLine; line++)
    {
      if ((line > startLine) && (line < endLine))
      {
        deletePos = 0;

        editRemoveText (startLine, deletePos, l->length()-startCol);
        editUnWrapLine (startLine, deletePos);
      }
      else
      {
        if (line == startLine)
        {
          deletePos = startCol;
          endPos = l->length();
        }
         else
        {
          deletePos = 0;
          endPos = endCol;
        }

        l->replace (deletePos, endPos-deletePos, 0, 0);
        editRemoveText (startLine, deletePos, endPos-deletePos);
      }
    }
  }

  editEnd ();

  return true;
}

bool KateDocument::insertLine( uint l, const QString &str )
{
  if (l > buffer->count())
    return false;

  editStart ();

  editInsertLine (l, str);

  editEnd ();

  return true;
}

bool KateDocument::removeLine( uint line )
{
  editStart ();
  
  bool end = editRemoveLine (line);

  editEnd ();

  return end;
}

uint KateDocument::length() const
{
  return text().length();
}

uint KateDocument::numLines() const
{
  return buffer->count();
}

uint KateDocument::numVisLines() const
{
  return buffer->count()-regionTree->getHiddenLinesCount();
}

int KateDocument::lineLength ( uint line ) const
{
  return textLength(line);
}

//
// KTextEditor::EditInterface internal stuff
//

//
// Starts an edit session with (or without) undo, update of view disabled during session
//
void KateDocument::editStart (bool withUndo)
{
  editSessionNumber++;

  if (editSessionNumber > 1)
    return;

  editIsRunning = true;
  noViewUpdates = true;
  editWithUndo = withUndo;
  buffer->noHlUpdate = true;

  editTagLineStart = 0xffffff;
  editTagLineEnd = 0;

  if (editWithUndo)
  {
    if (undoItems.count () > myUndoSteps)
      undoItems.removeFirst ();
    editCurrentUndo = new KateUndoGroup (this);
  }
  else
    editCurrentUndo = 0L;

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);
    v->myViewInternal->cursorCacheChanged = false;
    v->myViewInternal->tagLinesFrom = -1;
    v->myViewInternal->cursorCache = v->myViewInternal->getCursor();
  }
}

//
// End edit session and update Views
//
void KateDocument::editEnd ()
{
  if (editSessionNumber == 0)
    return;

  // wrap the new/changed text
  if (editSessionNumber == 1)
    if (myWordWrap)
      wrapText (editTagLineStart, editTagLineEnd, myWordWrapAt);

  editSessionNumber--;

  if (editSessionNumber > 0)
    return;

  buffer->noHlUpdate = false;

  if (editTagLineStart <= editTagLineEnd)
    updateLines(editTagLineStart, editTagLineEnd);

  if (editWithUndo && editCurrentUndo)
  {
    undoItems.append (editCurrentUndo);
    editCurrentUndo = 0L;
    emit undoChanged ();
  }

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);

    v->myViewInternal->updateLineRanges ();

    if (v->myViewInternal->tagLinesFrom > -1)
    {
      int startTagging = editTagLineStart;
      int endTagging = v->myViewInternal->endLineReal;
      
      if (v->myViewInternal->tagLinesFrom < editTagLineStart)
        startTagging = v->myViewInternal->tagLinesFrom;

      v->myViewInternal->tagRealLines (startTagging, endTagging, 0, 0xffff);
    }
    else
      v->myViewInternal->tagRealLines (editTagLineStart, editTagLineEnd, 0, 0xffff);

    if (v->myViewInternal->cursorCacheChanged)
      v->myViewInternal->updateCursor (v->myViewInternal->cursorCache);

    v->myViewInternal->tagLinesFrom = -1;
    v->myViewInternal->cursorCacheChanged = false;
  }

  setModified(true);
  emit textChanged ();

  noViewUpdates = false;
  updateViews();
  editIsRunning = false;
}

bool KateDocument::wrapText (uint startLine, uint endLine, uint col)
{
  if (endLine < startLine)
    return false;

  if (col == 0)
    return false;

  editStart ();

  uint line = startLine;
  int z = 0;

  while(line <= endLine)
  {
    TextLine::Ptr l = getTextLine(line);

    if (l->length() > col)
    {
      const QChar *text = l->getText();

      for (z=col; z>0; z--)
      {
        if (z < 1) break;
        if (text[z].isSpace()) break;
      }

      if (!(z < 1))
      {
        z++; // (anders: avoid the space at the beginning of the line)
        editWrapLine (line, z);
        endLine++;
      }
    }

    line++;

    if (line >= numLines()) break;
  };

  editEnd ();

  return true;
}

void KateDocument::editAddUndo (KateUndo *undo)
{
  if (!undo)
    return;

  if (editIsRunning && editWithUndo && editCurrentUndo)
    editCurrentUndo->addItem (undo);
  else
    delete undo;
}

void KateDocument::editTagLine (uint line)
{
  if (line < editTagLineStart)
    editTagLineStart = line;

  if (line > editTagLineEnd)
    editTagLineEnd = line;
}

void KateDocument::editInsertTagLine (uint line)
{
  if (line <= editTagLineStart)
    editTagLineStart++;

  if (line <= editTagLineEnd)
    editTagLineEnd++;
}

void KateDocument::editRemoveTagLine (uint line)
{
  if ((line < editTagLineStart) && (editTagLineStart > 0))
    editTagLineStart--;

  if ((line < editTagLineEnd) && (editTagLineEnd > 0))
    editTagLineEnd--;
}

bool KateDocument::editInsertText ( uint line, uint col, const QString &s )
{
  TextLine::Ptr l;

  l = getTextLine(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editInsertText, line, col, s.length(), s));

  newDocGeometry = true;

  l->replace(col, 0, s.unicode(), s.length());

  buffer->changeLine(line);
  editTagLine (line);

  editEnd ();

  return true;
}

bool KateDocument::editRemoveText ( uint line, uint col, uint len )
{
  TextLine::Ptr l;
  uint cLine, cCol;

  l = getTextLine(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editRemoveText, line, col, len, l->getString().mid(col, len)));

  l->replace(col, len, 0L, 0);

  buffer->changeLine(line);

  editTagLine(line);

  newDocGeometry = true;

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);

    cLine = v->myViewInternal->cursorCache.line;
    cCol = v->myViewInternal->cursorCache.col;

    if ( (cLine == line) && (cCol > col) )
    {
      if ((cCol - len) >= col)
      {
        if ((cCol - len) > 0)
          cCol = cCol-len;
        else
          cCol = 0;
      }
      else
        cCol = col;

      v->myViewInternal->cursorCache.line = line;
      v->myViewInternal->cursorCache.col = cCol;
      v->myViewInternal->cursorCacheChanged = true;
    }
  }

  editEnd ();

  return true;
}

bool KateDocument::editWrapLine ( uint line, uint col )
{
  TextLine::Ptr l, tl;
  KateView *view;

  l = getTextLine(line);
  tl = new TextLine();

  if (!l || !tl)
    return false;

  editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editWrapLine, line, col, 0, 0));

  l->wrap (tl, col);

  buffer->insertLine (line+1, tl);
  buffer->changeLine(line);

  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
      if (myMarks.at(z)->line > line+1)
      {
        myMarks.at(z)->line = myMarks.at(z)->line+1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  editInsertTagLine (line);
  editTagLine(line);
  editTagLine(line+1);

  regionTree->lineHasBeenInserted(line); //test line or line +1

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);

    if (line >= view->myViewInternal->newStartLineReal)
    {
      if ((view->myViewInternal->tagLinesFrom > line) || (view->myViewInternal->tagLinesFrom == -1))
        view->myViewInternal->tagLinesFrom = line;
    }
    else
      view->myViewInternal->newStartLineReal++;

    // correct cursor position
    if (view->myViewInternal->cursorCache.line > (int)line)
    {
      view->myViewInternal->cursorCache.line++;
      view->myViewInternal->cursorCacheChanged = true;
    }
    else if ( view->myViewInternal->cursorCache.line == (int)line
              && view->myViewInternal->cursorCache.col >= (int)col )
    {
      view->myViewInternal->cursorCache.col = tl->length();
      view->myViewInternal->cursorCache.line++;
      view->myViewInternal->cursorCacheChanged = true;
    }

  }
  editEnd ();
  return true;
}

bool KateDocument::editUnWrapLine ( uint line, uint col )
{
  TextLine::Ptr l, tl;
  KateView *view;
  uint cLine, cCol;

  l = getTextLine(line);
  tl = getTextLine(line+1);

  if (!l || !tl)
    return false;

  editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editUnWrapLine, line, col, 0, 0));

  l->unWrap (col, tl, tl->length());
  l->setContext (tl->getContext(), tl->getContextLength());

  buffer->changeLine(line);
  buffer->removeLine(line+1);

  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
      if (myMarks.at(z)->line > line)
      {
        if (myMarks.at(z)->line == line+1)
          myMarks.remove(z);
        else
          myMarks.at(z)->line = myMarks.at(z)->line-1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  editRemoveTagLine (line);
  editTagLine(line);
  editTagLine(line+1);

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);
    
    if (line >= view->myViewInternal->newStartLineReal)
    {
      if ((view->myViewInternal->tagLinesFrom > line) || (view->myViewInternal->tagLinesFrom == -1))
        view->myViewInternal->tagLinesFrom = line;
    }
    else
      view->myViewInternal->newStartLineReal--;

    cLine = view->myViewInternal->cursorCache.line;
    cCol = view->myViewInternal->cursorCache.col;

    if ( (cLine == (line+1)) || ((cLine == line) && (cCol >= col)) )
    {
      cCol = col;

      view->myViewInternal->cursorCache.line = line;
      view->myViewInternal->cursorCache.col = cCol;
      view->myViewInternal->cursorCacheChanged = true;
    }
  }

  editEnd ();

  return true;
}

bool KateDocument::editInsertLine ( uint line, const QString &s )
{
  KateView *view;

  editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editInsertLine, line, 0, s.length(), s));

  TextLine::Ptr TL=new TextLine();
  TL->append(s.unicode(),s.length());
  buffer->insertLine(line,TL);
  buffer->changeLine(line);

  editInsertTagLine (line);
  editTagLine(line);


  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
       if (myMarks.at(z)->line >= line)
      {
        myMarks.at(z)->line = myMarks.at(z)->line+1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  regionTree->lineHasBeenInserted(line);

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);
    
    if (line >= view->myViewInternal->newStartLineReal)
    {
      if ((view->myViewInternal->tagLinesFrom > line) || (view->myViewInternal->tagLinesFrom == -1))
        view->myViewInternal->tagLinesFrom = line;
    }
    else
      view->myViewInternal->newStartLineReal++;
  }

  editEnd ();

  return true;
}

bool KateDocument::editRemoveLine ( uint line )
{
  KateView *view;
  uint cLine, cCol;

//  regionTree->lineHasBeenRemoved(line);// is this the right place ?

  if (numLines() == 1)
    return false;

  editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editRemoveLine, line, 0, textLength(line), textLine(line) ));

  buffer->removeLine(line);

  editRemoveTagLine (line);

  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
      if (myMarks.at(z)->line >= line)
      {
        if (myMarks.at(z)->line == line)
          myMarks.remove(z);
        else
          myMarks.at(z)->line = myMarks.at(z)->line-1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  regionTree->lineHasBeenRemoved(line);

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);

    if (line >= view->myViewInternal->newStartLineReal)
    {
      if ((view->myViewInternal->tagLinesFrom > line) || (view->myViewInternal->tagLinesFrom == -1))
        view->myViewInternal->tagLinesFrom = line;
    }
    else
      view->myViewInternal->newStartLineReal--;

    cLine = view->myViewInternal->cursorCache.line;
    cCol = view->myViewInternal->cursorCache.col;

    if ( (cLine == line) )
    {
      if (line < lastLine())
        view->myViewInternal->cursorCache.line = line;
      else
        view->myViewInternal->cursorCache.line = line-1;

      cCol = 0;
      view->myViewInternal->cursorCache.col = cCol;
      view->myViewInternal->cursorCacheChanged = true;
    }
  }

  editEnd();

  return true;
}

//
// KTextEditor::SelectionInterface stuff
//

bool KateDocument::setSelection ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  int oldStartL, oldEndL;

  oldStartL = selectStart.line;
  oldEndL = selectEnd.line;

  if (startLine < endLine)
  {
    selectStart.line = startLine;
    selectStart.col = startCol;
    selectEnd.line = endLine;
    selectEnd.col = endCol;
  }
  else if (startLine > endLine)
  {
    selectStart.line = endLine;
    selectStart.col = endCol;
    selectEnd.line = startLine;
    selectEnd.col = startCol;
  }
  else if (startCol < endCol)
  {
    selectStart.line = startLine;
    selectStart.col = startCol;
    selectEnd.line = endLine;
    selectEnd.col = endCol;
  }
  else if (startCol >= endCol)
  {
    selectStart.line = endLine;
    selectStart.col = endCol;
    selectEnd.line = startLine;
    selectEnd.col = startCol;
  }

  int endL, startL;
  if (oldEndL > selectEnd.line)
    endL = oldEndL;
  else
    endL = selectEnd.line;

  if (oldStartL < selectStart.line)
    startL = oldStartL;
  else
    startL = selectStart.line;

  tagLines (startL, endL);
  updateViews ();

  emit selectionChanged ();

  return true;
}

bool KateDocument::clearSelection ()
{
  tagLines(selectStart.line,selectEnd.line);

  selectStart.line = -1;
  selectStart.col = -1;
  selectEnd.line = -1;
  selectEnd.col = -1;
  selectAnchor.line = -1;
  selectAnchor.col = -1;

  updateViews ();

  emit selectionChanged();

  return true;
}

bool KateDocument::hasSelection() const
{
  return ((selectStart.col != selectEnd.col) || (selectEnd.line != selectStart.line));
}

QString KateDocument::selection() const
{
  QString s;

  for (int z=selectStart.line; z <= selectEnd.line; z++)
  {
      QString line = textLine(z);

      if (!blockSelect)
      {
        if ((z > selectStart.line) && (z < selectEnd.line))
          s.append (line);
        else
        {
          if ((z == selectStart.line) && (z == selectEnd.line))
            s.append (line.mid(selectStart.col, selectEnd.col-selectStart.col));
          else if ((z == selectStart.line))
            s.append (line.mid(selectStart.col, line.length()-selectStart.col));
          else if ((z == selectEnd.line))
            s.append (line.mid(0, selectEnd.col));
        }
      }
      else
      {
        s.append (line.mid(selectStart.col, selectEnd.col-selectStart.col));
      }

      if (z < selectEnd.line)
        s.append (QChar('\n'));
    }

  return s;
}

bool KateDocument::removeSelectedText ()
{
  TextLine::Ptr textLine = 0L;
  int delLen, delStart, delLine;

  if (!hasSelection())
    return false;

  editStart ();

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);
    if ((selectStart.line <= v->myViewInternal->cursorCache.line) && (v->myViewInternal->cursorCache.line<= selectEnd.line))
    {
      v->myViewInternal->cursorCache.line = selectStart.line;
      v->myViewInternal->cursorCache.col = selectStart.col;
      v->myViewInternal->cursorCacheChanged = true;
    }
  }

  int sl = selectStart.line;
  int el = selectEnd.line;
  int sc = selectStart.col;
  int ec = selectEnd.col;

  for (int z=el; z >= sl; z--)
  {
    textLine = getTextLine(z);
    if (!textLine)
      break;

    delLine = 0;
    delStart = 0;
    delLen = 0;

    if (!blockSelect)
    {
      if ((z > sl) && (z < el))
        delLine = 1;
      else
      {
        if ((z == sl) && (z == el))
        {
          delStart = sc;
          delLen = ec-sc;
        }
        else if ((z == sl))
        {
          delStart = sc;
          delLen = textLine->length()-sc;

          if (sl < el)
            delLen++;
        }
        else if ((z == el))
        {
          delStart = 0;
          delLen = ec;
        }
      }
    }
    else
    {
      delStart = sc;
      delLen = ec-sc;

      if (delStart >= (int)textLine->length())
      {
        delStart = 0;
        delLen = 0;
      }
      else if (delLen+delStart > (int)textLine->length())
        delLen = textLine->length()-delStart;
    }

    if (delLine == 1)
      editRemoveLine (z);
    else if (delStart+delLen > (int)textLine->length())
    {
      editRemoveText (z, delStart, textLine->length()-delStart);
      editUnWrapLine (z, delStart);
    }
    else
      editRemoveText (z, delStart, delLen);
  }

  clearSelection();

  editEnd ();

  return true;
}

bool KateDocument::selectAll()
{
  return setSelection (0, 0, lastLine(), textLength(lastLine()));
}

//
// KTextEditor::BlockSelectionInterface stuff
//

bool KateDocument::blockSelectionMode ()
{
  return blockSelect;
}

bool KateDocument::setBlockSelectionMode (bool on)
{
  if (on != blockSelect)
  {
    blockSelect = on;
    setSelection (selectStart.line, selectStart.col, selectEnd.line, selectEnd.col);
    KTextEditor::View *view;
    for (view = myViews.first(); view != 0L; view = myViews.next() ) {
      emit static_cast<KateView *>( view )->newStatus();
    }
  }

  return true;
}

bool KateDocument::toggleBlockSelectionMode ()
{
  setBlockSelectionMode (!blockSelect);
  return true;
}

//
// KTextEditor::UndoInterface stuff
//

uint KateDocument::undoCount () const
{
  return undoItems.count ();
}

uint KateDocument::redoCount () const
{
  return redoItems.count ();
}

uint KateDocument::undoSteps () const
{
  return myUndoSteps;
}

void KateDocument::setUndoSteps(uint steps)
{
  myUndoSteps = steps;

  emit undoChanged ();
}

void KateDocument::undo()
{
  if ((undoItems.count() >0) && undoItems.last())
  {
    undoItems.last()->undo();
    redoItems.append (undoItems.last());
    undoItems.removeLast ();

    emit undoChanged ();
  }
}

void KateDocument::redo()
{
  if ((redoItems.count() >0) && redoItems.last())
  {
    redoItems.last()->redo();
    undoItems.append (redoItems.last());
    redoItems.removeLast ();

    emit undoChanged ();
  }
}

void KateDocument::clearUndo()
{
  undoItems.setAutoDelete (true);
  undoItems.clear ();
  undoItems.setAutoDelete (false);

  emit undoChanged ();
}

void KateDocument::clearRedo()
{
  redoItems.setAutoDelete (true);
  redoItems.clear ();
  redoItems.setAutoDelete (false);

  emit undoChanged ();
}

//
// KTextEditor::CursorInterface stuff
//

KTextEditor::Cursor *KateDocument::createCursor ( )
{
  return new KateCursor (this);
}

QPtrList<KTextEditor::Cursor> KateDocument::cursors () const
{
  return myCursors;
}

//
// KTextEditor::SearchInterface stuff
//

bool KateDocument::searchText (unsigned int startLine, unsigned int startCol, const QString &text, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive, bool backwards)
{
  int line, col;
  int searchEnd;
  TextLine::Ptr textLine;
  uint foundAt, myMatchLen;
  bool found;

  if (text.isEmpty())
    return false;

  line = startLine;
  col = startCol;

  if (!backwards)
  {
    searchEnd = lastLine();

    while (line <= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, text, &foundAt, &myMatchLen, casesensitive, false);

      if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      col = 0;
      line++;
    }
  }
  else
  {
    // backward search
    searchEnd = 0;

    while (line >= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, text, &foundAt, &myMatchLen, casesensitive, true);

        if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      if (line >= 1)
        col = textLength(line-1);

      line--;
    }
  }

  return false;
}

bool KateDocument::searchText (unsigned int startLine, unsigned int startCol, const QRegExp &regexp, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool backwards)
{
  int line, col;
  int searchEnd;
  TextLine::Ptr textLine;
  uint foundAt, myMatchLen;
  bool found;

  if (regexp.isEmpty() || !regexp.isValid())
    return false;

  line = startLine;
  col = startCol;

  if (!backwards)
  {
    searchEnd = lastLine();

    while (line <= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, regexp, &foundAt, &myMatchLen, false);

      if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      col = 0;
      line++;
    }
  }
  else
  {
    // backward search
    searchEnd = 0;

    while (line >= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, regexp, &foundAt, &myMatchLen, true);

        if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      if (line >= 1)
        col = textLength(line-1);

      line--;
    }
  }

  return false;
}

//
// KTextEditor::HighlightingInterface stuff
//

uint KateDocument::hlMode ()
{
  return hlManager->findHl(m_highlight);
}

bool KateDocument::setHlMode (uint mode)
{
  if (internalSetHlMode (mode))
  {
    setDontChangeHlOnSave();
    updateViews();
    return true;
  }

  return false;
}

bool KateDocument::internalSetHlMode (uint mode)
{
  Highlight *h;

  h = hlManager->getHl(mode);
  if (h == m_highlight) {
    updateLines();
  } else {
    if (m_highlight != 0L) m_highlight->release();
    h->use();
    m_highlight = h;
    buffer->setHighlight(m_highlight);
    makeAttribs();
  }

  emit(hlChanged());

  return true;
}

uint KateDocument::hlModeCount ()
{
  return HlManager::self()->highlights();
}

QString KateDocument::hlModeName (uint mode)
{
  return HlManager::self()->hlName (mode);
}

QString KateDocument::hlModeSectionName (uint mode)
{
  return HlManager::self()->hlSection (mode);
}

void KateDocument::setDontChangeHlOnSave()
{
  hlSetByUser = true;
}

//
// KTextEditor::ConfigInterface stuff
//

void KateDocument::readConfig(KConfig *config)
{
  _searchFlags = config->readNumEntry("SearchFlags", KateDocument::sfPrompt);
  _configFlags = config->readNumEntry("ConfigFlags", _configFlags) & ~KateDocument::cfMark;

  myWordWrap = config->readBoolEntry("Word Wrap On", false);
  myWordWrapAt = config->readNumEntry("Word Wrap At", 80);

  setTabWidth(config->readNumEntry("TabWidth", 8));
  setUndoSteps(config->readNumEntry("UndoSteps", 256));
  setFont (ViewFont,config->readFontEntry("Font", &viewFont.myFont));
  setFont (PrintFont,config->readFontEntry("PrintFont", &printFont.myFont));

  colors[0] = config->readColorEntry("Color Background", &colors[0]);
  colors[1] = config->readColorEntry("Color Selected", &colors[1]);

  if (myWordWrap)
  {
    editStart (false);
    wrapText (myWordWrapAt);
    editEnd ();
    setModified(false);
    emit textChanged ();
  }

  tagAll();
  updateEditAccels();
  updateViews();
}

void KateDocument::writeConfig(KConfig *config)
{
  config->writeEntry("SearchFlags",_searchFlags);
  config->writeEntry("ConfigFlags",_configFlags);

  config->writeEntry("Word Wrap On", myWordWrap);
  config->writeEntry("Word Wrap At", myWordWrapAt);
  config->writeEntry("UndoSteps", myUndoSteps);
  config->writeEntry("TabWidth", tabChars);
  config->writeEntry("Font", viewFont.myFont);
  config->writeEntry("PrintFont", printFont.myFont);
  config->writeEntry("Color Background", colors[0]);
  config->writeEntry("Color Selected", colors[1]);
}

void KateDocument::readConfig()
{
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate Document");
  readConfig (config);
  config->sync();
}

void KateDocument::writeConfig()
{
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate Document");
  writeConfig (config);
  config->sync();
}

void KateDocument::readSessionConfig(KConfig *config)
{
  // enable the setMark function to set marks for lines > lastLine !!!
  restoreMarks = true;

  m_url = config->readEntry("URL"); // ### doesn't this break the encoding? (Simon)
  internalSetHlMode(hlManager->nameFind(config->readEntry("Highlight")));

  // restore bookmarks
  QValueList<int> l = config->readIntListEntry("Bookmarks");
  if ( l.count() )
  {
    for (uint i=0; i < l.count(); i++)
      setMark( l[i], KateDocument::markType01 );
  }

  restoreMarks = false;
}

void KateDocument::writeSessionConfig(KConfig *config)
{
  config->writeEntry("URL", m_url.url() ); // ### encoding?? (Simon)
  config->writeEntry("Highlight", m_highlight->name());
  // anders: save bookmarks
  QValueList<int> ml;
  for (uint i=0; i < myMarks.count(); i++) {
    if ( myMarks.at(i)->type == 1) // only save bookmarks
     ml << myMarks.at(i)->line;
  }
  if (ml.count() )
    config->writeEntry("Bookmarks", ml);
}

void KateDocument::configDialog()
{
  KWin kwin;

  KDialogBase *kd = new KDialogBase(KDialogBase::IconList,
                                    i18n("Configure"),
                                    KDialogBase::Ok | KDialogBase::Cancel |
                                    KDialogBase::Help ,
                                    KDialogBase::Ok, kapp->mainWidget());

  // color options
  QVBox *page=kd->addVBoxPage(i18n("Colors"), i18n("Colors"),
                              BarIcon("colorize", KIcon::SizeMedium) );
  Kate::ConfigPage *mcolorConfigPage = colorConfigPage(page);

  page = kd->addVBoxPage(i18n("Fonts"), i18n("Fonts Settings"),
                              BarIcon("fonts", KIcon::SizeMedium) );
  Kate::ConfigPage *mfontConfigPage = fontConfigPage(page);

  // indent options
  page=kd->addVBoxPage(i18n("Indent"), i18n("Indent Options"),
                       BarIcon("rightjust", KIcon::SizeMedium) );
  Kate::ConfigPage *mindentConfigPage = indentConfigPage(page);

  // select options
  page=kd->addVBoxPage(i18n("Select"), i18n("Selection Behavior"),
                       BarIcon("misc") );
  Kate::ConfigPage *mselectConfigPage = selectConfigPage(page);

  // edit options
  page=kd->addVBoxPage(i18n("Edit"), i18n("Editing Options"),
                       BarIcon("edit", KIcon::SizeMedium ) );
  Kate::ConfigPage *meditConfigPage = editConfigPage (page);

  // Cursor key options
  page=kd->addVBoxPage(i18n("Keyboard"), i18n("Keyboard Configuration"),
                       BarIcon("edit", KIcon::SizeMedium ) );
  Kate::ConfigPage *mkeysConfigPage = keysConfigPage (page);

  // spell checker
  page = kd->addVBoxPage( i18n("Spelling"), i18n("Spell Checker Behavior"),
                          BarIcon("spellcheck", KIcon::SizeMedium) );
  Kate::ConfigPage *mkSpellConfigPage = kSpellConfigPage (page);

  kwin.setIcons(kd->winId(), kapp->icon(), kapp->miniIcon());

  page=kd->addVBoxPage(i18n("Highlighting"),i18n("Highlighting Configuration"),
                        BarIcon("edit",KIcon::SizeMedium));
  Kate::ConfigPage *mhlConfigPage = hlConfigPage (page);

  if (kd->exec())
  {
    mcolorConfigPage->apply();
    mfontConfigPage->apply();
    mindentConfigPage->apply();
    mselectConfigPage->apply();
    meditConfigPage->apply();
    mkeysConfigPage->apply();
    mkSpellConfigPage->apply();
    mhlConfigPage->apply();

    // save the config, reload it to update doc + all views
    writeConfig();
    readConfig();
  }

  delete kd;
}

uint KateDocument::mark (uint line)
{
  if (myMarks.isEmpty())
    return 0;

  if (line > lastLine())
    return 0;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
      return myMarks.at(z)->type;

  return 0;
}

void KateDocument::setMark (uint line, uint markType)
{
  if ((!restoreMarks) && (line > lastLine()))
    return;

  bool b = false;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.at(z)->type=markType;
      b = true;
    }

  if (!b)
  {
    KTextEditor::Mark *mark = new KTextEditor::Mark;
    mark->line = line;
    mark->type = markType;
    myMarks.append (mark);
  }

  emit marksChanged ();

  tagLines (line,line);
  updateViews ();
}

void KateDocument::clearMark (uint line)
{
  if (myMarks.isEmpty())
    return;

  if (line > lastLine())
    return;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.remove(z);
      emit marksChanged ();

      tagLines (line,line);
      updateViews ();
    }
}

void KateDocument::addMark (uint line, uint markType)
{
  if (line > lastLine())
    return;

  bool b = false;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.at(z)->type=myMarks.at(z)->type | markType;
      b = true;
    }

  if (!b)
  {
    KTextEditor::Mark *mark = new KTextEditor::Mark;
    mark->line = line;
    mark->type = markType;
    myMarks.append (mark);
  }

  emit marksChanged ();

  tagLines (line,line);
  updateViews ();
}

void KateDocument::removeMark (uint line, uint markType)
{
  if (myMarks.isEmpty())
    return;

  if (line > lastLine())
    return;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.at(z)->type=myMarks.at(z)->type & ~markType;
      if (myMarks.at(z)->type == 0)
        myMarks.remove(z);

      emit marksChanged ();
    }

  tagLines (line,line);
  updateViews ();
}

QPtrList<KTextEditor::Mark> KateDocument::marks ()
{
  return myMarks;
}

void KateDocument::clearMarks ()
{
  if (myMarks.isEmpty())
    return;

  while (myMarks.count() > 0)
  {
    tagLines (myMarks.at (0)->line, myMarks.at (0)->line);
    myMarks.remove((uint)0);
  }

  emit marksChanged ();
  updateViews ();
}

//
// KTextEditor::PrintInterface stuff
//

bool KateDocument::printDialog ()
{
  KPrinter printer;
  if (!docName().isEmpty())
    printer.setDocName(docName());
  else if (!url().isEmpty())
    printer.setDocName(url().fileName());
  else
    printer.setDocName(i18n("Untitled"));

   if ( printer.setup( kapp->mainWidget() ) )
   {
     QPainter paint( &printer );
     QPaintDeviceMetrics pdm( &printer );

     uint y = 0;
     uint lineCount = 0;
     uint maxWidth = pdm.width();
     int startCol = 0;
     int endCol = 0;
     bool needWrap = true;

     while (  lineCount <= lastLine()  )
     {
       startCol = 0;
       endCol = 0;
       needWrap = true;

       while (needWrap)
       {
         if (y+printFont.fontHeight >= (uint)pdm.height() )
         {
           printer.newPage();
           y=0;
         }

         endCol = textWidth (getTextLine(lineCount), startCol, maxWidth, 0, PrintFont, &needWrap);
         paintTextLine ( paint, lineCount, startCol, endCol, y, 0, maxWidth, false,PrintFont );
         startCol = endCol;
         y += printFont.fontHeight;
       }

       lineCount++;
     }

     return true;
  }

  return false;
}

bool KateDocument::print ()
{
  return printDialog ();
}

//
// KParts::ReadWrite stuff
//

bool KateDocument::openFile()
{
  fileInfo->setFile (m_file);
  setMTime();

  if (!fileInfo->exists() || !fileInfo->isReadable())
    return false;

  clear();
  buffer->insertFile(0, m_file, KGlobal::charsets()->codecForName(myEncoding));

  setMTime();

  if (myWordWrap)
  {
    editStart (false);
    wrapText (myWordWrapAt);
    editEnd ();
    setModified(false);
    emit textChanged ();
  }

  int hl = hlManager->wildcardFind( m_file );

  if (hl == -1)
  {
    // fill the detection buffer with the contents of the text
    // anders: I fixed this to work :^)
    const int HOWMANY = 1024;
    QByteArray buf(HOWMANY);
    int bufpos = 0, len;
    for (uint i=0; i < buffer->count(); i++)
    {
      QString line = buffer->plainLine(i);
      len = line.length() + 1; // space for a newline - seemingly not required by kmimemagic, but nicer for debugging.
//kdDebug(13020)<<"openFile(): collecting a buffer for hlManager->mimeFind(): found "<<len<<" bytes in line "<<i<<endl;
      if (bufpos + len > HOWMANY) len = HOWMANY - bufpos;
//kdDebug(13020)<<"copying "<<len<<"bytes."<<endl;
      memcpy(&buf[bufpos], (line+"\n").latin1(), len);
      bufpos += len;
      if (bufpos >= HOWMANY) break;
    }
//kdDebug(13020)<<"openFile(): calling hlManager->mimeFind() with data:"<<endl<<buf.data()<<endl<<"--"<<endl;
    hl = hlManager->mimeFind( buf, m_file );
  }

  internalSetHlMode(hl);

  newDocGeometry = true;

  updateLines();
  updateViews();

  emit fileNameChanged();

  return true;
}

bool KateDocument::saveFile()
{
  QFile f( m_file );
  if ( !f.open( IO_WriteOnly ) )
    return false; // Error

  QTextStream stream(&f);

  stream.setEncoding(QTextStream::RawUnicode); // disable Unicode headers
  stream.setCodec(KGlobal::charsets()->codecForName(myEncoding)); // this line sets the mapper to the correct codec

  int maxLine = numLines();
  int line = 0;
  while(true)
  {
    stream << textLine(line);
    line++;
    if (line >= maxLine) break;

    if (eolMode == KateDocument::eolUnix) stream << "\n";
    else if (eolMode == KateDocument::eolDos) stream << "\r\n";
    else if (eolMode == KateDocument::eolMacintosh) stream << '\r';
  };
  f.close();

  fileInfo->setFile (m_file);
  setMTime();

  if (!hlSetByUser)
  {
  int hl = hlManager->wildcardFind( m_file );

  if (hl == -1)
  {
    // fill the detection buffer with the contents of the text
    const int HOWMANY = 1024;
    QByteArray buf(HOWMANY);
    int bufpos = 0, len;
    for (uint i=0; i < buffer->count(); i++)
    {
      TextLine::Ptr textLine = buffer->line(i);
      len = textLine->length();
      if (bufpos + len > HOWMANY) len = HOWMANY - bufpos;
      memcpy(&buf[bufpos], textLine->getText(), len);
      bufpos += len;
      if (bufpos >= HOWMANY) break;
    }

    hl = hlManager->mimeFind( buf, m_file );
  }

  internalSetHlMode(hl);
  }
  emit fileNameChanged ();

  return (f.status() == IO_Ok);
}

void KateDocument::setReadWrite( bool rw )
{
  KTextEditor::View *view;

  if (rw == readOnly)
  {
    readOnly = !rw;
    KParts::ReadWritePart::setReadWrite (rw);
    for (view = myViews.first(); view != 0L; view = myViews.next() ) {
      emit static_cast<KateView *>( view )->newStatus();
    }
  }
}

bool KateDocument::isReadWrite() const
{
  return !readOnly;
}

void KateDocument::setModified(bool m) {
  KTextEditor::View *view;

  if (m != modified) {
    modified = m;
    KParts::ReadWritePart::setModified (m);
    for (view = myViews.first(); view != 0L; view = myViews.next() ) {
      emit static_cast<KateView *>( view )->newStatus();
    }
    emit modifiedChanged ();
  }
}

bool KateDocument::isModified() const {
  return modified;
}

//
// Kate specific stuff ;)
//

void KateDocument::setFont (WhichFont wf,QFont font)
{
  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  fs->myFont = font;
  fs->myFontBold = QFont (font);
  fs->myFontBold.setBold (true);

  fs->myFontItalic = QFont (font);
  fs->myFontItalic.setItalic (true);

  fs->myFontBI = QFont (font);
  fs->myFontBI.setBold (true);
  fs->myFontBI.setItalic (true);

  fs->myFontMetrics = KateFontMetrics (fs->myFont);
  fs->myFontMetricsBold = KateFontMetrics (fs->myFontBold);
  fs->myFontMetricsItalic = KateFontMetrics (fs->myFontItalic);
  fs->myFontMetricsBI = KateFontMetrics (fs->myFontBI);

  fs->updateFontData(tabChars);
  if (wf==ViewFont)
  {
    updateFontData();
    updateViews(); //Quick & Dirty Hack (by JoWenn) //Remove in KDE 3.0
  }
}

void KateDocument::slotBufferUpdateHighlight(uint from, uint to)
{
  if (to > m_highlightedEnd)
     m_highlightedEnd = to;
  uint till = from + 100;
  if (till > m_highlightedEnd)
     till = m_highlightedEnd;
  buffer->updateHighlighting(from, till, false);
  m_highlightedTill = till;
  if (m_highlightedTill >= m_highlightedEnd)
  {
      m_highlightedTill = 0;
      m_highlightedEnd = 0;
      m_highlightTimer->stop();
  }
  else
  {
      m_highlightTimer->start(100, true);
  }
}

void KateDocument::slotBufferUpdateHighlight()
{
  uint till = m_highlightedTill + 1000;

  uint max = numLines();
  if (m_highlightedEnd > max)
    m_highlightedEnd = max;

  if (till > m_highlightedEnd)
     till = m_highlightedEnd;
  buffer->updateHighlighting(m_highlightedTill, till, false);
  m_highlightedTill = till;
  if (m_highlightedTill >= m_highlightedEnd)
  {
      m_highlightedTill = 0;
      m_highlightedEnd = 0;
      m_highlightTimer->stop();
  }
  else
  {
      m_highlightTimer->start(100, true);
  }
  updateViews();
}

TextLine::Ptr KateDocument::getTextLine(int line) const
{
  // This is a hack to get this stuff working.
  return buffer->line(line);
}

int KateDocument::textLength(int line) const {
  return buffer->plainLine(line).length();
}

void KateDocument::setTabWidth(int chars) {
  if (tabChars == chars) return;
  if (chars < 1) chars = 1;
  if (chars > 16) chars = 16;
  tabChars = chars;
  printFont.updateFontData(tabChars);
  viewFont.updateFontData(tabChars);
  updateFontData();
}

void KateDocument::setNewDoc( bool m )
{
  if ( m != newDoc )
  {
    newDoc = m;
  }
}

bool KateDocument::isNewDoc() const {
  return newDoc;
}

//  Spellchecking methods

void KateDocument::spellcheck()
{
  if (!isReadWrite())
    return;

  kspell.kspell= new KSpell (kapp->mainWidget(), "KateView: Spellcheck", this,
                      SLOT (spellcheck2 (KSpell *)));

  connect (kspell.kspell, SIGNAL(death()),
          this, SLOT(spellCleanDone()));

  connect (kspell.kspell, SIGNAL (progress (unsigned int)),
          this, SIGNAL (spellcheck_progress (unsigned int)) );
  connect (kspell.kspell, SIGNAL (misspelling (const QString &, const QStringList &, unsigned int)),
          this, SLOT (misspelling (const QString &, const QStringList &, unsigned int)));
  connect (kspell.kspell, SIGNAL (corrected (const QString&, const QString&, unsigned int)),
          this, SLOT (corrected (const QString&, const QString&, unsigned int)));
  connect (kspell.kspell, SIGNAL (done(const QString&)),
          this, SLOT (spellResult (const QString&)));
}

void KateDocument::spellcheck2(KSpell *)
{
  setReadWrite (false);

  // this is a hack, setPseudoModal() has been hacked to recognize 0x01
  // as special (never tries to delete it)
  // this should either get improved (with a #define or something),
  // or kspell should provide access to the spell widget.
  setPseudoModal((QWidget*)0x01);

  kspell.spell_tmptext = text();

  kspell.kspellon = TRUE;
  kspell.kspellMispellCount = 0;
  kspell.kspellReplaceCount = 0;
  kspell.kspellPristine = !isModified();

  kspell.kspell->setProgressResolution (1);

  kspell.kspell->check(kspell.spell_tmptext);
}

void KateDocument::misspelling (const QString &origword, const QStringList &, unsigned pos)
{
  uint line;
  uint cnt;

  // Find pos  -- CHANGEME: store the last found pos's cursor
  //   and do these searched relative to that to
  //   (significantly) increase the speed of the spellcheck

  for (cnt = 0, line = 0 ; line <= lastLine() && cnt <= pos ; line++)
    cnt += textLength(line)+1;

  // Highlight the mispelled word
  KateTextCursor cursor;
  line--;
  cursor.col = pos - (cnt - textLength(line)) + 1;
  cursor.line = line;
//  deselectAll(); // shouldn't the spell check be allowed within selected text?
  kspell.kspellMispellCount++;

  KateView *view;
  VConfig c;
  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->myViewInternal->updateCursor(cursor); //this does deselectAll() if no persistent selections
    view->myViewInternal->getVConfig(c);
  }

  selectLength(cursor,origword.length(),c.flags);
  updateViews();
}

void KateDocument::corrected (const QString & originalword, const QString & newword, unsigned)
{
  removeText(selectStart.line, selectStart.col, selectStart.line, selectStart.col + originalword.length());
  insertText(selectStart.line, selectStart.col, newword);
  kspell.kspellReplaceCount++;
}

void KateDocument::spellResult (const QString &)
{
  clearSelection ();

  // we know if the check was cancelled
  // we can safely use the undo mechanism to backout changes
  // in case of a cancel, because we force the entire spell check
  // into one group (record)
  if (kspell.kspell->dlgResult() == 0)
  {
  }

  setPseudoModal(0L);
  setReadWrite (true);

  updateViews();

  kspell.kspell->cleanUp();
}

void KateDocument::spellCleanDone ()
{
  KSpell::spellStatus status = kspell.kspell->status();
  kspell.spell_tmptext = "";
  delete kspell.kspell;

  kspell.kspell = 0;
  kspell.kspellon = FALSE;

  if (status == KSpell::Error)
  {
     KMessageBox::sorry(kapp->mainWidget(), i18n("ISpell could not be started.\n"
     "Please make sure you have ISpell properly configured and in your PATH."));
  }
  else if (status == KSpell::Crashed)
  {
     setPseudoModal(0L);
     setReadWrite (true);

     updateViews();
     KMessageBox::sorry(kapp->mainWidget(), i18n("ISpell seems to have crashed."));
  }
  else
  {
     emit spellcheck_done();
  }
}

void KateDocument::makeAttribs()
{
  hlManager->makeAttribs(this, m_highlight);
  updateFontData();
  updateLines();
}

void KateDocument::updateFontData() {
  KateView *view;

  for (view = myViews.first(); view != 0L; view = myViews.next() ) {
    view->myViewInternal->resizeDrawBuffer(view->width(),viewFont.fontHeight);
    view->myViewInternal->tagAll();
    view->myViewInternal->updateCursor();
  }
}

void KateDocument::internalHlChanged() { //slot
  makeAttribs();
  updateViews();
}

void KateDocument::addView(KTextEditor::View *view) {
  myViews.append( (KateView *) view  );
  _views.append( view );
  myActiveView = (KateView *) view;
}

void KateDocument::removeView(KTextEditor::View *view) {
  if (myActiveView == view)
    myActiveView = 0L;

  myViews.removeRef( (KateView *) view );
  _views.removeRef( view  );
}

void KateDocument::addCursor(KTextEditor::Cursor *cursor) {
  myCursors.append( cursor );
}

void KateDocument::removeCursor(KTextEditor::Cursor *cursor) {
  myCursors.removeRef( cursor  );
}

bool KateDocument::ownedView(KateView *view) {
  // do we own the given view?
  return (myViews.containsRef(view) > 0);
}

bool KateDocument::isLastView(int numViews) {
  return ((int) myViews.count() == numViews);
}

int KateDocument::charWidth(const TextLine::Ptr &textLine, int cursorX,WhichFont wf) {
  QChar ch = textLine->getChar(cursorX);
  Attribute *a = attribute(textLine->getAttr(cursorX));
  int x;
  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  if (ch == '\t')
    x = fs->m_tabWidth;
  else if (a->bold && a->italic)
    x = fs->myFontMetricsBI.width(ch);
  else if (a->bold)
    x = fs->myFontMetricsBold.width(ch);
  else if (a->italic)
    x = fs->myFontMetricsItalic.width(ch);
  else
    x = fs->myFontMetrics.width(ch);

  return x;
}

int KateDocument::charWidth(KateTextCursor &cursor) {
  if (cursor.col < 0)
     cursor.col = 0;
  if (cursor.line < 0)
     cursor.line = 0;
  if (cursor.line >= (int) numLines())
     cursor.line = lastLine();
  return charWidth(getTextLine(cursor.line),cursor.col);
}

uint KateDocument::textWidth(const TextLine::Ptr &textLine, int cursorX,WhichFont wf)
{
  int x;
  int z;
  QChar ch;
  Attribute *a;
  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  x = 0;
  for (z = 0; z < cursorX; z++) {
    ch = textLine->getChar(z);
    a = attribute(textLine->getAttr(z));

    if (ch == '\t')
      x += fs->m_tabWidth;
    else if (a->bold && a->italic)
      x += fs->myFontMetricsBI.width(ch);
    else if (a->bold)
      x += fs->myFontMetricsBold.width(ch);
    else if (a->italic)
      x += fs->myFontMetricsItalic.width(ch);
    else
      x += fs->myFontMetrics.width(ch);
  }
  return x;
}

uint KateDocument::textWidth(const TextLine::Ptr &textLine, uint startcol, uint maxwidth, uint wrapsymwidth, WhichFont wf, bool *needWrap)
{
  QChar ch;
  Attribute *a;
  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;
  uint x = 0;
  uint endcol = 0;
  uint endcolwithsym = 0;

  *needWrap = false;

  for (uint z = startcol; z < textLine->length(); z++)
  {
    ch = textLine->getChar(z);
    a = attribute(textLine->getAttr(z));

    if (ch == '\t')
      x += fs->m_tabWidth;
    else if (a->bold && a->italic)
      x += fs->myFontMetricsBI.width(ch);
    else if (a->bold)
      x += fs->myFontMetricsBold.width(ch);
    else if (a->italic)
      x += fs->myFontMetricsItalic.width(ch);
    else
      x += fs->myFontMetrics.width(ch);

    if (x <= maxwidth-wrapsymwidth )
      endcolwithsym = z+1;

    if (x <= maxwidth)
      endcol = z+1;

    if (x >= maxwidth)
    {
      *needWrap = true;
      break;
    }
  }

  if (*needWrap)
    return endcolwithsym;
  else
    return endcol;
}

uint KateDocument::textWidth(KateTextCursor &cursor)
{
  if (cursor.col < 0)
     cursor.col = 0;
  if (cursor.line < 0)
     cursor.line = 0;
  if (cursor.line >= (int)numLines())
     cursor.line = lastLine();
  return textWidth(getTextLine(cursor.line),cursor.col);
}

uint KateDocument::textWidth(bool wrapCursor, KateTextCursor &cursor, int xPos,WhichFont wf)
{
  int len;
  int x, oldX;
  int z;
  QChar ch;
  Attribute *a;

  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  if (cursor.line < 0) cursor.line = 0;
  if (cursor.line > (int)lastLine()) cursor.line = lastLine();
  TextLine::Ptr textLine = getTextLine(cursor.line);
  len = textLine->length();

  x = oldX = z = 0;
  while (x < xPos && (!wrapCursor || z < len)) {
    oldX = x;
    ch = textLine->getChar(z);
    a = attribute(textLine->getAttr(z));

    if (ch == '\t')
      x += fs->m_tabWidth;
    else if (a->bold && a->italic)
      x += fs->myFontMetricsBI.width(ch);
    else if (a->bold)
      x += fs->myFontMetricsBold.width(ch);
    else if (a->italic)
      x += fs->myFontMetricsItalic.width(ch);
    else
      x += fs->myFontMetrics.width(ch);

    z++;
  }
  if (xPos - oldX < x - xPos && z > 0) {
    z--;
    x = oldX;
  }
  cursor.col = z;
  return x;
}

uint KateDocument::textPos(const TextLine::Ptr &textLine, int xPos,WhichFont wf) {
  int x, oldX;
  int z;
  QChar ch;
  Attribute *a;

  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  x = oldX = z = 0;
  while (x < xPos) { // && z < len) {
    oldX = x;
    ch = textLine->getChar(z);
    a = attribute(textLine->getAttr(z));

    if (ch == '\t')
      x += fs->m_tabWidth;
    else if (a->bold && a->italic)
      x += fs->myFontMetricsBI.width(ch);
    else if (a->bold)
      x += fs->myFontMetricsBold.width(ch);
    else if (a->italic)
      x += fs->myFontMetricsItalic.width(ch);
    else
      x += fs->myFontMetrics.width(ch);

    z++;
  }
  if (xPos - oldX < x - xPos && z > 0) {
    z--;
   // newXPos = oldX;
  }// else newXPos = x;
  return z;
}

uint KateDocument::textHeight(WhichFont wf) {
  return numLines()*((wf==ViewFont)?viewFont.fontHeight:printFont.fontHeight);
}

uint KateDocument::currentColumn(KateTextCursor &cursor)
{
  TextLine::Ptr t = getTextLine(cursor.line);

  if (t)
    return t->cursorX(cursor.col,tabChars);
  else
    return 0;
}

bool KateDocument::insertChars ( int line, int col, const QString &chars, KateView *view )
{
  int z, pos, l;
  bool onlySpaces;
  QChar ch;
  QString buf;

  int savedCol=col;
  int savedLine=line;
  QString savedChars(chars);

  TextLine::Ptr textLine = getTextLine(line);

  pos = 0;
  onlySpaces = true;
  for (z = 0; z < (int) chars.length(); z++) {
    ch = chars[z];
    if (ch == '\t' && _configFlags & KateDocument::cfReplaceTabs) {
      l = tabChars - (textLine->cursorX(col, tabChars) % tabChars);
      while (l > 0) {
        buf.insert(pos, ' ');
        pos++;
        l--;
      }
    } else if (ch.isPrint() || ch == '\t') {
      buf.insert(pos, ch);
      pos++;
      if (ch != ' ') onlySpaces = false;
      if (_configFlags & KateDocument::cfAutoBrackets) {
        if (ch == '(') buf.insert(pos, ')');
        if (ch == '[') buf.insert(pos, ']');
        if (ch == '{') buf.insert(pos, '}');
      }
    }
  }
  //pos = cursor increment

  //return false if nothing has to be inserted
  if (buf.isEmpty()) return false;

  editStart ();

  if (_configFlags & KateDocument::cfDelOnInput)
  {
    if (hasSelection())
    {
      removeSelectedText();
      line = view->myViewInternal->cursorCache.line;
      col = view->myViewInternal->cursorCache.col;
    }
  }

  if (_configFlags & KateDocument::cfOvr)
  {
    if ((col+buf.length()) <= textLine->length())
      removeText (line, col, line, col+buf.length());
    else
      removeText (line, col, line, textLine->length());
  }

  insertText (line, col, buf);
  col += pos;

  // editEnd will set the cursor from this cache right ;))
  view->myViewInternal->cursorCache.line = line;
  view->myViewInternal->cursorCache.col = col;
  view->myViewInternal->cursorCacheChanged = true;

  editEnd ();

  emit charactersInteractivelyInserted(savedLine,savedCol,savedChars);
  return true;
}

QString tabString(int pos, int tabChars)
{
  QString s;
  while (pos >= tabChars) {
    s += '\t';
    pos -= tabChars;
  }
  while (pos > 0) {
    s += ' ';
    pos--;
  }
  return s;
}

void KateDocument::newLine(VConfig &c)
{

  if (!(_configFlags & KateDocument::cfAutoIndent)) {
    insertText (c.cursor.line, c.cursor.col, "\n");
    c.cursor.line++;
//    c.displayCursor.line++;
//    c.displayCursor.col=0;
    c.cursor.col = 0;
  } else {
    TextLine::Ptr textLine = getTextLine(c.cursor.line);
    int pos = textLine->firstChar();
    if (c.cursor.col < pos) c.cursor.col = pos; // place cursor on first char if before


    int y = c.cursor.line;
    while ((y > 0) && (pos < 0)) { // search a not empty text line
      textLine = getTextLine(--y);
      pos = textLine->firstChar();
    }
    insertText (c.cursor.line, c.cursor.col, "\n");
    c.cursor.line++;
    c.cursor.col = 0;
    if (pos > 0) {
      pos = textLine->cursorX(pos, tabChars);
//      if (getTextLine(c.cursor.line)->length() > 0) {
        QString s = tabString(pos, (_configFlags & KateDocument::cfSpaceIndent) ? 0xffffff : tabChars);
        insertText (c.cursor.line, c.cursor.col, s);
        pos = s.length();
//      }
//      recordInsert(c.cursor, QString(textLine->getText(), pos));
      c.cursor.col = pos;
    }
  }
  // anders: document should _never_ care about the view
  //c.view->myViewInternal->updateCursor(c.cursor);
}

void KateDocument::killLine(VConfig &c)
{
  removeLine (c.cursor.line);
//  regionTree->lineHasBeenRemoved(c.cursor.line);// is this the right place ?

}

void KateDocument::backspace(uint line, uint col)
{
  if ((col == 0) && (line == 0))
    return;

  if (col > 0)
  {
    if (!(_configFlags & KateDocument::cfBackspaceIndents))
    {
      // ordinary backspace
      //c.cursor.col--;
      removeText(line, col-1, line, col);
    }
    else
    {
      // backspace indents: erase to next indent position
      int l = 1; // del one char

      TextLine::Ptr textLine = getTextLine(line);
      int pos = textLine->firstChar();
      if (pos < 0 || pos >= (int)col)
      {
        // only spaces on left side of cursor
        // search a line with less spaces
        uint y = line;
        while (y > 0)
        {
          textLine = getTextLine(--y);
          pos = textLine->firstChar();

          if (pos >= 0 && pos < (int)col)
          {
            l = col - pos; // del more chars
            break;
          }
        }
      }
      // break effectively jumps here
      //c.cursor.col -= l;
      removeText(line, col-l, line, col);
    }
  }
  else
  {
    // col == 0: wrap to previous line
    if (line >= 1)
    {
      regionTree->lineHasBeenRemoved(line);
      removeText (line-1, getTextLine(line-1)->length(), line, 0);
    }
  }
}

void KateDocument::del(VConfig &c)
{
  if (c.cursor.col < (int) getTextLine(c.cursor.line)->length())
  {
    removeText(c.cursor.line, c.cursor.col, c.cursor.line, c.cursor.col+1);
  }
  else
  {
    regionTree->lineHasBeenRemoved(c.cursor.line);
    removeText(c.cursor.line, c.cursor.col, c.cursor.line+1, 0);
  }
}

void KateDocument::cut(VConfig &)
{
  if (!hasSelection())
    return;

  copy(_configFlags);
  removeSelectedText();
}

void KateDocument::copy(int )
{
  if (!hasSelection())
    return;

  QApplication::clipboard()->setText(selection ());
}

void KateDocument::paste (VConfig &c)
{
  QString s = QApplication::clipboard()->text();

  if (!s.isEmpty())
  {
    editStart ();

    if (!blockSelect)
      insertText(c.cursor.line, c.cursor.col, s);
    else
    {
      insertText(c.cursor.line, c.cursor.col, s);
    }

    int line = c.cursor.line;
    int col = c.cursor.col;
      // anders: we want to be able to move the cursor to the
  // position at the end of the pasted text,
  // so we calculate that and applies it to c.cursor
  // This may not work, when wordwrap gets fixed :(
  TextLine *ln = getTextLine( line );
  int l = s.length();
  while ( l > 0 ) {
    if ( (uint)col < ln->length() ) {
      col++;
    }
    else {
      line++;
      ln = getTextLine( line );
      col = 0;
    }
    l--;
  }

// editEnd will set the cursor from this cache right ;))
  c.view->myViewInternal->cursorCache.line = line;
  c.view->myViewInternal->cursorCache.col = col;
  c.view->myViewInternal->cursorCacheChanged = true;

    editEnd ();
  }
}

void KateDocument::selectTo(VConfig &c, KateTextCursor &cursor, int )
{
  if ( selectAnchor.line == -1 )
  {
    // anders: if we allready have a selection, we want to include all of that
    if ( hasSelection() &&
            ( cursor.line > selectEnd.line || cursor.line >= selectEnd.line && cursor.col >= selectEnd.col ) ) {
      selectAnchor.line = selectStart.line;
      selectAnchor.col = selectStart.col;
    }
    else {
      selectAnchor.line = c.cursor.line;
      selectAnchor.col = c.cursor.col;
    }
  }

  setSelection (selectAnchor.line, selectAnchor.col, cursor.line, cursor.col);
}

void KateDocument::selectWord(KateTextCursor &cursor, int flags) {
  int start, end, len;

  TextLine::Ptr textLine = getTextLine(cursor.line);
  len = textLine->length();
  start = end = cursor.col;
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;
  if (end <= start) return;
  if (!(flags & KateDocument::cfKeepSelection)) clearSelection ();

  setSelection (cursor.line, start, cursor.line, end);
}

void KateDocument::selectLine(KateTextCursor &cursor, int flags) {
  TextLine::Ptr textLine = getTextLine(cursor.line);
  if (!(flags & KateDocument::cfKeepSelection)) clearSelection ();
  setSelection (cursor.line, 0, cursor.line+1, 0);
}

void KateDocument::selectLength(KateTextCursor &cursor, int length, int flags) {
  int start, end;

  TextLine::Ptr textLine = getTextLine(cursor.line);
  start = cursor.col;
  end = start + length;
  if (end <= start) return;
  if (!(flags & KateDocument::cfKeepSelection)) clearSelection ();

  setSelection (cursor.line, start, cursor.line, end);
}

void KateDocument::doIndent(VConfig &c, int change)
{
  c.cursor.col = 0;

  editStart ();

  if (!hasSelection()) {
    // single line
    optimizeLeadingSpace(c.cursor.line, _configFlags, change);
  }
  else
  {
    int sl = selectStart.line;
    int el = selectEnd.line;
    int ec = selectEnd.col;

    if ((ec == 0) && ((el-1) >= 0))
    {
      el--;
    }

    // entire selection
    TextLine::Ptr textLine;
    int line, z;
    QChar ch;

    if (_configFlags & KateDocument::cfKeepIndentProfile && change < 0) {
      // unindent so that the existing indent profile doesnt get screwed
      // if any line we may unindent is already full left, don't do anything
      for (line = sl; line <= el; line++) {
        textLine = getTextLine(line);
        if (lineSelected(line) || lineHasSelected(line)) {
          for (z = 0; z < tabChars; z++) {
            ch = textLine->getChar(z);
            if (ch == '\t') break;
            if (ch != ' ') {
              change = 0;
              goto jumpOut;
            }
          }
        }
      }
      jumpOut:;
    }

    for (line = sl; line <= el; line++) {
      if (lineSelected(line) || lineHasSelected(line)) {
        optimizeLeadingSpace(line, _configFlags, change);
      }
    }
  }

  editEnd ();
}

/*
  Optimize the leading whitespace for a single line.
  If change is > 0, it adds indentation units (tabChars)
  if change is == 0, it only optimizes
  If change is < 0, it removes indentation units
  This will be used to indent, unindent, and optimal-fill a line.
  If excess space is removed depends on the flag cfKeepExtraSpaces
  which has to be set by the user
*/
void KateDocument::optimizeLeadingSpace(int line, int flags, int change) {
  int len;
  int chars, space, okLen;
  QChar ch;
  int extra;
  QString s;
  KateTextCursor cursor;

  TextLine::Ptr textLine = getTextLine(line);
  len = textLine->length();
  space = 0; // length of space at the beginning of the textline
  okLen = 0; // length of space which does not have to be replaced
  for (chars = 0; chars < len; chars++) {
    ch = textLine->getChar(chars);
    if (ch == ' ') {
      space++;
      if (flags & KateDocument::cfSpaceIndent && okLen == chars) okLen++;
    } else if (ch == '\t') {
      space += tabChars - space % tabChars;
      if (!(flags & KateDocument::cfSpaceIndent) && okLen == chars) okLen++;
    } else break;
  }

  space += change*tabChars; // modify space width
  // if line contains only spaces it will be cleared
  if (space < 0 || chars == len) space = 0;

  extra = space % tabChars; // extra spaces which dont fit the indentation pattern
  if (flags & KateDocument::cfKeepExtraSpaces) chars -= extra;

  if (flags & KateDocument::cfSpaceIndent) {
    space -= extra;
    ch = ' ';
  } else {
    space /= tabChars;
    ch = '\t';
  }

  // dont replace chars which are already ok
  cursor.col = QMIN(okLen, QMIN(chars, space));
  chars -= cursor.col;
  space -= cursor.col;
  if (chars == 0 && space == 0) return; //nothing to do

  s.fill(ch, space);

//printf("chars %d insert %d cursor.col %d\n", chars, insert, cursor.col);
  cursor.line = line;
  removeText (cursor.line, cursor.col, cursor.line, cursor.col+chars);
  insertText(cursor.line, cursor.col, s);
}

/*
  Remove a given string at the begining
  of the current line.
*/
bool KateDocument::removeStringFromBegining(int line, QString &str)
{
  TextLine* textline = getTextLine(line);

  if(textline->startingWith(str))
  {
    // Get string lenght
    int length = str.length();

    // Remove some chars
    removeText (line, 0, line, length);

    return true;
  }

  return false;
}

/*
  Remove a given string at the end
  of the current line.
*/
bool KateDocument::removeStringFromEnd(int line, QString &str)
{
  TextLine* textline = getTextLine(line);

  if(textline->endingWith(str))
  {
    // Get string lenght
    int length = str.length();

    // Remove some chars
    removeText (line, 0, line, length);

    return true;
  }

  return false;
}

/*
  Add to the current line a comment line mark at
  the begining.
*/
void KateDocument::addStartLineCommentToSingleLine(int line)
{
  QString commentLineMark = m_highlight->getCommentSingleLineStart() + " ";
  insertText (line, 0, commentLineMark);
}

/*
  Remove from the current line a comment line mark at
  the begining if there is one.
*/
bool KateDocument::removeStartLineCommentFromSingleLine(int line)
{
  QString shortCommentMark = m_highlight->getCommentSingleLineStart();
  QString longCommentMark = shortCommentMark + " ";

  // Try to remove the long comment mark first
  bool removed = (removeStringFromBegining(line, longCommentMark)
                  || removeStringFromBegining(line, shortCommentMark));

  return removed;
}

/*
  Add to the current line a start comment mark at the
 begining and a stop comment mark at the end.
*/
void KateDocument::addStartStopCommentToSingleLine(int line)
{
  QString startCommentMark = m_highlight->getCommentStart() + " ";
  QString stopCommentMark = " " + m_highlight->getCommentEnd();

  // Add the start comment mark
  insertText (line, 0, startCommentMark);

  // Go to the end of the line
  TextLine* textline = getTextLine(line);
  int col = textline->length();

  // Add the stop comment mark
  insertText (line, col, stopCommentMark);
}

/*
  Remove from the current line a start comment mark at
  the begining and a stop comment mark at the end.
*/
bool KateDocument::removeStartStopCommentFromSingleLine(int line)
{
  QString shortStartCommentMark = m_highlight->getCommentStart();
  QString longStartCommentMark = shortStartCommentMark + " ";
  QString shortStopCommentMark = m_highlight->getCommentEnd();
  QString longStopCommentMark = " " + shortStopCommentMark;

  // Try to remove the long start comment mark first
  bool removedStart = (removeStringFromBegining(line, longStartCommentMark)
                       || removeStringFromBegining(line, shortStartCommentMark));

  // Try to remove the long stop comment mark first
  bool removedStop = (removeStringFromEnd(line, longStopCommentMark)
                      || removeStringFromEnd(line, shortStopCommentMark));

  return (removedStart || removedStop);
}

/*
  Add to the current selection a start comment
 mark at the begining and a stop comment mark
 at the end.
*/
void KateDocument::addStartStopCommentToSelection()
{
  QString startComment = m_highlight->getCommentStart();
  QString endComment = m_highlight->getCommentEnd();

  int sl = selectStart.line;
  int el = selectEnd.line;
  int sc = selectStart.col;
  int ec = selectEnd.col;

  if ((ec == 0) && ((el-1) >= 0))
  {
    el--;
    ec = getTextLine (el)->length();
  }

  insertText (el, ec, endComment);
  insertText (sl, sc, startComment);
}

/*
  Add to the current selection a comment line
 mark at the begining of each line.
*/
void KateDocument::addStartLineCommentToSelection()
{
  QString commentLineMark = m_highlight->getCommentSingleLineStart() + " ";

  int sl = selectStart.line;
  int el = selectEnd.line;

  if ((selectEnd.col == 0) && ((el-1) >= 0))
  {
    el--;
  }

  // For each line of the selection
  for (int z = el; z >= sl; z--)
    insertText (z, 0, commentLineMark);
}

/*
  Remove from the selection a start comment mark at
  the begining and a stop comment mark at the end.
*/
bool KateDocument::removeStartStopCommentFromSelection()
{
  QString startComment = m_highlight->getCommentStart();
  QString endComment = m_highlight->getCommentEnd();

  int sl = selectStart.line;
  int el = selectEnd.line;
  int sc = selectStart.col;
  int ec = selectEnd.col;

   if ((ec == 0) && (el > 0))
  {
    el--;
    ec = getTextLine (el)->length();
  }

  int startCommentLen = startComment.length();
  int endCommentLen = endComment.length();

  // had this been perl or sed: s/^\s*$startComment(.+?)$endComment\s*/$1/
  // check if both ends matches, allowing whitespace on outer side; return if eithre fails
  TextLine::Ptr l;
  l = getTextLine(sl);
  // skip spaces/lines starting at selectStart
  while ( (sl <= el) && (sc < ec) && l->getChar(sc).isSpace() ) {
    if ( ((uint) sc) == l->length() ) {
      sl++;
      sc = 0;
      l = getTextLine( sl );
      if (!l) return false; // hopefully _VERY_ unlikely
    }
    sc++;
  }
  if ( l->getString().mid( sc, startCommentLen ) != startComment ) {
    kdDebug(13020)<<"removeBlaBla(): '"<<startComment<<"' not found after skipping space ("<<sl<<", "<<sc<<") - giving up :("<<endl;
    return false;
  }
  // repat kinda reversed for end.....
  l = getTextLine( el );
  ec--;
  while ( el >= sl /*&& ec > sc*/ && l->getChar(ec).isSpace() ) {
    if ( ec < 0 ) {
      kdDebug(13020)<<"removeBlaBla(): up a line = "<<el-1<<endl;
      el--;
      l = getTextLine( el );
      if (!l) return false; // hopefully _VERY_ unlikely
      ec = l->length();
    }
    ec--;
  }
  ec++; // we went one char too far to find a nonspace
  if ( ec - endCommentLen < 0 || l->getString().mid(ec-endCommentLen,endCommentLen) != endComment ) {
    kdDebug(13020)<<"removeBlaBla(): '"<<endComment<<"' not found after skipping space ("<<el<<", "<<ec-endCommentLen<<") - giving up :("<<endl;
    return false;
  }
  removeText (el, ec-endCommentLen, el, ec);
  removeText (sl, sc, sl, sc+startCommentLen);
  // TODO anders: redefine selection
  kdDebug(13020)<<"removeBlaBla(): I DID IT!! I'm DANCING AROUND"<<endl;
  return true;
}

/*
  Remove from the begining of each line of the
  selection a start comment line mark.
*/
bool KateDocument::removeStartLineCommentFromSelection()
{
  QString shortCommentMark = m_highlight->getCommentSingleLineStart();
  QString longCommentMark = shortCommentMark + " ";

  int sl = selectStart.line;
  int el = selectEnd.line;

   if ((selectEnd.col == 0) && ((el-1) >= 0))
  {
    el--;
  }

  bool removed = false;

  // For each line of the selection
  for (int z = el; z >= sl; z--)
  {
    // Try to remove the long comment mark first
    removed = (removeStringFromBegining(z, longCommentMark)
                 || removeStringFromBegining(z, shortCommentMark)
                 || removed);
  }

  return removed;
}

/*
  Comment or uncomment the selection or the current
  line if there is no selection.
*/
void KateDocument::doComment(VConfig &c, int change)
{
  bool hasStartLineCommentMark = !(m_highlight->getCommentSingleLineStart().isEmpty());
  bool hasStartStopCommentMark = ( !(m_highlight->getCommentStart().isEmpty())
                                   && !(m_highlight->getCommentEnd().isEmpty()) );

  bool removed = false;

  if (change > 0)
  {
    if ( !hasSelection() )
    {
      if ( hasStartLineCommentMark )
        addStartLineCommentToSingleLine(c.cursor.line);
      else if ( hasStartStopCommentMark )
        addStartStopCommentToSingleLine(c.cursor.line);
    }
    else
    {
      // anders: prefer single line comment to avoid nesting probs
      // If the selection starts after first char in the first line
      // or ends before the last char of the last line, we may use
      // multiline comment markers.
      // TODO We should try to detect nesting.
      //    - if selection ends at col 0, most likely she wanted that line ignored
      if ( hasStartStopCommentMark &&
           ( !hasStartLineCommentMark || (
             ( selectStart.col > getTextLine( selectStart.line )->firstChar() ) ||
               ( selectEnd.col < ((int)getTextLine( selectEnd.line )->length()) )
         ) ) )
        addStartStopCommentToSelection();
      else if ( hasStartLineCommentMark )
        addStartLineCommentToSelection();
    }
  }
  else
  {
    if ( !hasSelection() )
    {
      removed = ( hasStartLineCommentMark
                  && removeStartLineCommentFromSingleLine(c.cursor.line) )
        || ( hasStartStopCommentMark
             && removeStartStopCommentFromSingleLine(c.cursor.line) );
    }
    else
    {
      // anders: this seems like it will work with above changes :)
      removed = ( hasStartLineCommentMark
                  && removeStartLineCommentFromSelection() )
        || ( hasStartStopCommentMark
             && removeStartStopCommentFromSelection() );
    }
  }
}

QString KateDocument::getWord(KateTextCursor &cursor) {
  int start, end, len;

  TextLine::Ptr textLine = getTextLine(cursor.line);
  len = textLine->length();
  start = end = cursor.col;
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;
  len = end - start;
  return QString(&textLine->getText()[start], len);
}

void KateDocument::tagLines(int start, int end)
{
  for (uint z = 0; z < myViews.count(); z++)
    myViews.at(z)->myViewInternal->tagRealLines(start, end, 0, 0xffffff);
}

void KateDocument::tagAll()
{
  for (uint z = 0; z < myViews.count(); z++)
    myViews.at(z)->myViewInternal->tagAll();
}

void KateDocument::updateLines()
{
  buffer->invalidateHighlighting();
}

void KateDocument::updateLines(int startLine, int endLine)
{
  buffer->updateHighlighting(startLine, endLine+1, true);
}

void KateDocument::slotBufferChanged()
{
  newDocGeometry = true;
  updateViews();
}


void KateDocument::updateViews()
{
  if (noViewUpdates)
    return;

  KateView *view;
  int flags;

  flags = (newDocGeometry) ? KateView::ufDocGeometry : 0;

  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->myViewInternal->updateView(flags);
  }

  newDocGeometry = false;
}

void KateDocument::updateEditAccels()
{
  KateView *view;

  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->setupEditKeys();
  }

}

QColor &KateDocument::backCol(int x, int y) {
  return (lineColSelected(x,y)) ? colors[1] : colors[0];
 }

QColor &KateDocument::cursorCol(int x, int y)
{
  TextLine::Ptr textLine = getTextLine(y);
  Attribute *a = attribute(textLine->getAttr(x));

  if (lineColSelected (y, x))
    return a->selCol;
  else
    return a->col;
}

bool KateDocument::lineColSelected (int line, int col)
{
  if (!blockSelect)
  {
    if ((line > selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectStart.line) && (col >= selectStart.col) && (line < selectEnd.line))
      return true;

    if ((line == selectEnd.line) && (col < selectEnd.col) && (line > selectStart.line))
      return true;

    if ((line == selectEnd.line) && (col < selectEnd.col) && (line == selectStart.line) && (col >= selectStart.col))
      return true;

    if ((line == selectStart.line) && (selectStart.col == 0) && (col < 0))
      return true;
  }
  else
  {
    if ((line >= selectStart.line) && (line <= selectEnd.line) && (col >= selectStart.col) && (col < selectEnd.col))
      return true;
  }

  return false;
}

bool KateDocument::lineSelected (int line)
{
  if (!blockSelect)
  {
    if ((line > selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectStart.line) && (line < selectEnd.line) && (selectStart.col == 0))
      return true;
  }

  return false;
}

bool KateDocument::lineEndSelected (int line)
{
  if (!blockSelect)
  {
    if ((line >= selectStart.line) && (line < selectEnd.line))
      return true;
  }

  return false;
}

bool KateDocument::lineHasSelected (int line)
{
  if (!blockSelect)
  {
    if ((line > selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectEnd.line) && (line > selectStart.line))
      return true;

    if ((line == selectEnd.line) && (line == selectStart.line) && (selectEnd.col > selectStart.col))
      return true;
  }
  else
  {
    if ((line <= selectEnd.line) && (line >= selectStart.line) && (selectEnd.col > selectStart.col))
      return true;
  }

  return false;
}

bool KateDocument::paintTextLine(QPainter &paint, uint line, int xStart, int xEnd, bool showTabs)
{
  return paintTextLine (paint, line, 0, -1, 0, xStart, xEnd, showTabs);
}

bool KateDocument::paintTextLine(QPainter &paint, uint line, int startcol, int endcol, int xStart, int xEnd, bool showTabs)
{
  return paintTextLine (paint, line, startcol, endcol, 0, xStart, xEnd, showTabs);
}

bool KateDocument::paintTextLine(QPainter &paint, uint line, int startcol, int endcol, int y, int xStart, int xEnd, bool showTabs,WhichFont wf)
{
  // font data
  FontStruct *fs = (wf==ViewFont)?&viewFont:&printFont;

  // text attribs font/style data
  Attribute *at = myAttribs.data();
  uint atLen = myAttribs.size();

  // textline
  TextLine::Ptr textLine;

  // length, chars + raw attribs
  uint len = 0;
  const QChar *s;
  const uchar *a;

  // was the selection background allready completly painted ?
  bool selectionPainted = false;

  if (!selectionPainted && lineSelected (line))
  {
    paint.fillRect(0, y, xEnd - xStart, fs->fontHeight, colors[1]);
    selectionPainted = true;
  }
  else
    paint.fillRect(0, y, xEnd - xStart, fs->fontHeight, colors[0]);


  if (line > lastLine())
    return false;

  textLine = getTextLine(line);

  if (!textLine)
    return false;

  if (!textLine->isVisible())
    return false;

  len = textLine->length();
  uint oldLen = len;

  if (startcol > len)
    startcol = len;

  if (startcol < 0)
    startcol = 0;

  if (endcol < 0)
    len = len - startcol;
  else
    len = endcol - startcol;

  // text + attrib data from line
  s = textLine->getText ();
  a = textLine->getAttribs ();

  // adjust to startcol ;)
  s = s + startcol;
  a = a + startcol;

  uint curCol = startcol;

  // or we will see no text ;)
  uint oldY = y;
  y += fs->fontAscent;

  // painting loop
  uint xPos = 0;
  uint xPosAfter = 0;
  uint width;

  Attribute *curAt = 0;
  Attribute *oldAt = 0;

  // selection startcol/endcol calc
  bool hasSel = false;
  uint startSel = 0;
  uint endSel = 0;

  if (!selectionPainted)
  {
    if (hasSelection())
    {
      if (!blockSelect)
      {
        if ((line == selectStart.line) && (line < selectEnd.line))
        {
          startSel = selectStart.col;
          endSel = oldLen;
          hasSel = true;
        }
        else if ((line == selectEnd.line) && (line > selectStart.line))
        {
          startSel = 0;
          endSel = selectEnd.col;
          hasSel = true;
        }
        else if ((line == selectEnd.line) && (line == selectStart.line))
        {
          startSel = selectStart.col;
          endSel = selectEnd.col;
          hasSel = true;
        }
      }
      else
      {
        if ((line >= selectStart.line) && (line <= selectEnd.line))
        {
          startSel = selectStart.col;
          endSel = selectEnd.col;
          hasSel = true;
        }
      }
    }
  }

  for (uint tmp = len; tmp > 0; tmp--)
  {
    if ((*s) == QChar('\t'))
      width = fs->m_tabWidth;
    else
    {
      if ((*a) >= atLen)
      {
        curAt = &at[0];
        width = fs->myFontMetrics.width(*s);

        if (curAt != oldAt)
          paint.setFont(fs->myFont);
      }
      else
      {
        curAt = &at[*a];

        if (curAt->bold && curAt->italic)
	{
          width = fs->myFontMetricsBI.width(*s);

          if (curAt != oldAt)
	    paint.setFont(fs->myFontBI);
        }
        else if (curAt->bold)
	{
          width = fs->myFontMetricsBold.width(*s);

          if (curAt != oldAt)
            paint.setFont(fs->myFontBold);
        }
        else if (curAt->italic)
	{
          width = fs->myFontMetricsItalic.width(*s);

          if (curAt != oldAt)
            paint.setFont(fs->myFontItalic);
	}
        else
	{
          width = fs->myFontMetrics.width(*s);

          if (curAt != oldAt)
            paint.setFont(fs->myFont);
	}
      }

      if (curAt != oldAt)
        paint.setPen(curAt->col);
    }

    xPosAfter += width;

    if (xPosAfter >= xStart)
    {
      if (!selectionPainted && hasSel && (curCol >= startSel) && (curCol < endSel))
        paint.fillRect(xPos - xStart, oldY, xPosAfter - xPos, fs->fontHeight, colors[1]);

      if ((*s) != QChar('\t'))
      {
        QConstString str((QChar *) s, 1);
        paint.drawText(xPos-xStart, y, str.string());
      }
      else if (showTabs)
      {
        paint.drawPoint(xPos - xStart, y);
        paint.drawPoint(xPos - xStart + 1, y);
        paint.drawPoint(xPos - xStart, y - 1);
      }
    }

    // increase xPos
    xPos = xPosAfter;

    // increase char + attribs pos
    s++;
    a++;

    // to only switch font/color if needed
    oldAt = curAt;

    // col move
    curCol++;
  }

  if (!selectionPainted && lineEndSelected (line))
  {
    paint.fillRect(xPos-xStart, oldY, xEnd - xStart, fs->fontHeight, colors[1]);
    selectionPainted = true;
  }

  return true;
}

// Applies the search context, and returns whether a match was found. If one is,
// the length of the string matched is also returned.
bool KateDocument::doSearch(SConfig &sc, const QString &searchFor) {
  int line, col;
  int searchEnd;
  TextLine::Ptr textLine;

  if (searchFor.isEmpty())
    return false;

  line = sc.cursor.line;
  col = sc.cursor.col;

  if (line < 0)
    return false;

  if (!(sc.flags & KateDocument::sfBackward)) // FORWARDS
  {
    if (sc.flags & KateDocument::sfSelected)
    {
      if (line < selectStart.line)
      {
        line = selectStart.line;
        col = selectStart.col;
      }
      searchEnd = selectEnd.line;
    }
    else
      searchEnd = lastLine();

    while (line <= searchEnd)
    {
      textLine = getTextLine(line);

      uint fCol = 0;
      uint mlen = 0;

      bool found = false;

      if (sc.flags & KateDocument::sfRegularExpression)
        found = textLine->searchText (col, sc.m_regExp, &fCol, &mlen, false);
      else
        found = textLine->searchText (col, sc.m_pattern, &fCol, &mlen, sc.flags & KateDocument::sfCaseSensitive, false);

      if (found && (sc.flags & KateDocument::sfSelected) && blockSelectionMode())
        if ((int)(fCol+mlen) > selectEnd.col)
          found = false;

      if (found && (sc.flags & KateDocument::sfSelected) && (line == selectEnd.line))
        if ((int)(fCol+mlen) > selectEnd.col)
          found = false;

      if (found && (sc.flags & KateDocument::sfWholeWords))
      {
        if ((fCol > 0) && m_highlight->isInWord (textLine->getChar(fCol-1)))
          found = false;

        if (((fCol+mlen) < textLine->length()) && m_highlight->isInWord (textLine->getChar(fCol+mlen)))
          found = false;
      }

      if (found)
      {
        sc.cursor.col = fCol;
        sc.cursor.line = line;
        sc.matchedLength = mlen;
        return true;
      }

      if ((sc.flags & KateDocument::sfSelected) && blockSelectionMode())
        col = selectStart.col;
      else
        col = 0;

      line++;
    }
  }
  else // BACKWARDS
  {
    if (sc.flags & KateDocument::sfSelected)
    {
      if (line > selectEnd.line)
      {
        line = selectEnd.line;
        col = selectEnd.col;
      }
      searchEnd = selectStart.line;
    }
    else
      searchEnd = 0;

    while (line >= searchEnd)
    {
      textLine = getTextLine(line);

      if ((col == -1) || (col > ((int) textLine->length())))
        col = textLine->length();

      uint fCol = 0;
      uint mlen = 0;
      bool found = false;

      if (sc.flags & KateDocument::sfRegularExpression)
        found = textLine->searchText (col, sc.m_regExp, &fCol, &mlen, true);
      else
        found = textLine->searchText (col, sc.m_pattern, &fCol, &mlen, sc.flags & KateDocument::sfCaseSensitive, true);

      if (found && (sc.flags & KateDocument::sfSelected) && blockSelectionMode())
        if (((int)fCol) < selectStart.col)
          found = false;

      if (found && (sc.flags & KateDocument::sfSelected) && (line == selectStart.line))
        if (((int)fCol) < selectStart.col)
          found = false;

      if (found && (sc.flags & KateDocument::sfWholeWords))
      {
        if ((fCol > 0) && m_highlight->isInWord (textLine->getChar(fCol-1)))
          found = false;

        if (((fCol+mlen) < textLine->length()) && m_highlight->isInWord (textLine->getChar(fCol+mlen)))
          found = false;
      }

      if (found)
      {
        sc.cursor.col = fCol;
        sc.cursor.line = line;
        sc.matchedLength = mlen;
        return true;
      }

      if ((sc.flags & KateDocument::sfSelected) && blockSelectionMode())
        col = selectEnd.col;
      else
        col = -1;

      line--;
    }
  }

  //    if (sc.flags & KateDocument::sfWholeWords) {

  sc.flags |= KateDocument::sfWrapped;

  return false;
}

void KateDocument::newBracketMark(KateTextCursor &cursor, BracketMark &bm)
{
  TextLine::Ptr textLine;
  int x, line, count, attr;
  QChar bracket, opposite, ch;
  Attribute *a;

  bm.eXPos = -1; //mark bracked mark as invalid
  x = cursor.col -1; // -1 to look at left side of cursor
  if (x < 0) return;
  line = cursor.line; //current line
  count = 0; //bracket counter for nested brackets

  textLine = getTextLine(line);
  if (!textLine) return;

  bracket = textLine->getChar(x);
  attr = textLine->getAttr(x);

  if (bracket == '(' || bracket == '[' || bracket == '{')
  {
    //get opposite bracket
    opposite = ')';
    if (bracket == '[') opposite = ']';
    if (bracket == '{') opposite = '}';
    //get attribute of bracket (opposite bracket must have the same attribute)
    x++;
    while (line - cursor.line < 40) {
      //go to next line on end of line
      while (x >= (int) textLine->length()) {
        line++;
        if (line > (int) lastLine()) return;
        textLine = getTextLine(line);
        x = 0;
      }
      if (textLine->getAttr(x) == attr) {
        //try to find opposite bracked
        ch = textLine->getChar(x);
        if (ch == bracket) count++; //same bracket : increase counter
        if (ch == opposite) {
          count--;
          if (count < 0) goto found;
        }
      }
      x++;
    }
  }
  else if (bracket == ')' || bracket == ']' || bracket == '}')
  {
    opposite = '(';
    if (bracket == ']') opposite = '[';
    if (bracket == '}') opposite = '{';
    x--;
    while (cursor.line - line < 20) {

      while (x < 0) {
        line--;
        if (line < 0) return;
        textLine = getTextLine(line);
        x = textLine->length() -1;
      }
      if (textLine->getAttr(x) == attr) {
        ch = textLine->getChar(x);
        if (ch == bracket) count++;
        if (ch == opposite) {
          count--;
          if (count < 0) goto found;
        }
      }
      x--;
    }
  }
  return;

found:
  //cursor position of opposite bracket
  bm.cursor.col = x;
  bm.cursor.line = line;
  //x position (start and end) of related bracket
  bm.sXPos = textWidth(textLine, x);
  a = attribute(attr);

   if (a->bold && a->italic)
      bm.eXPos = bm.sXPos + viewFont.myFontMetricsBI.width(bracket);
    else if (a->bold)
      bm.eXPos = bm.sXPos + viewFont.myFontMetricsBold.width(bracket);
    else if (a->italic)
      bm.eXPos = bm.sXPos + viewFont.myFontMetricsItalic.width(bracket);
    else
      bm.eXPos = bm.sXPos + viewFont.myFontMetrics.width(bracket);
}

void KateDocument::guiActivateEvent( KParts::GUIActivateEvent *ev )
{
  KParts::ReadWritePart::guiActivateEvent( ev );
  if ( ev->activated() )
    emit selectionChanged();
}

void KateDocument::setDocName (QString docName)
{
  myDocName = docName;
  emit nameChanged ((Kate::Document *) this);
}

void KateDocument::setMTime()
{
    if (fileInfo && !fileInfo->fileName().isEmpty()) {
      fileInfo->refresh();
      mTime = fileInfo->lastModified();
    }
}

void KateDocument::isModOnHD(bool forceReload)
{
  if (fileInfo && !fileInfo->fileName().isEmpty()) {
    fileInfo->refresh();
    if (fileInfo->lastModified() > mTime) {
      if ( forceReload ||
           (KMessageBox::warningContinueCancel(0,
               (i18n("The file %1 has changed on disk.\nDo you want to reload the modified file?\n\nIf you choose Cancel and subsequently save the file, you will lose those modifications.")).arg(url().filename()),
               i18n("File has changed on disk"),
               i18n("Yes") ) == KMessageBox::Continue)
          )
        reloadFile();
      else
        setMTime();
    }
  }
}

void KateDocument::reloadFile()
{
  if (fileInfo && !fileInfo->fileName().isEmpty())
  {
    uint mode = hlMode ();
    bool byUser = hlSetByUser;

    KateDocument::openFile();
    setMTime();
    
    if (byUser)
      setHlMode (mode);
  }
}

void KateDocument::slotModChanged()
{
  emit modStateChanged ((Kate::Document *)this);
}

void KateDocument::flush ()
{
  if (!isReadWrite())
    return;

  m_url = KURL();
  fileInfo->setFile (QString());
  setMTime();

  clear();
  updateViews();

  emit fileNameChanged ();
}

void KateDocument::open (const QString &name)
{
  openURL (KURL (name));
}

Attribute *KateDocument::attribute (uint pos)
{
  if (pos < myAttribs.size())
    return &myAttribs[pos];

  return &myAttribs[0];
}

void KateDocument::wrapText (uint col)
{
  wrapText (0, lastLine(), col);
}

void KateDocument::setPseudoModal(QWidget *w) {
//  QWidget *old = pseudoModal;

  // (glenebob)
  // this is a temporary hack to make the spell checker work a little
  // better - as kspell progresses, this sort of thing should become
  // obsolete or worked around more cleanly
  // this is relied upon *only* by the spell-check code
  if (pseudoModal && pseudoModal != (QWidget*)1L)
    delete pseudoModal;

//  pseudoModal = 0L;
//  if (old || w) recordReset();

  pseudoModal = w;
}

void KateDocument::setWordWrap (bool on)
{
  if (on != myWordWrap && on)
    wrapText (myWordWrapAt);

  myWordWrap = on;
}

void KateDocument::setWordWrapAt (uint col)
{
  if (myWordWrapAt != col && myWordWrap)
    wrapText (myWordWrapAt);

  myWordWrapAt = col;
}

void KateDocument::applyWordWrap ()
{
  wrapText (myWordWrapAt);
}

uint KateDocument::configFlags ()
{
  return _configFlags;
}

void KateDocument::setConfigFlags (uint flags)
{
  bool updateView;

  if (flags != _configFlags)
  {
    // update the view if visibility of tabs has changed
    updateView = (flags ^ _configFlags) & KateDocument::cfShowTabs;
    _configFlags = flags;
    //emit newStatus();
    if (updateView) updateViews ();
  }
}



void KateDocument::exportAs(const QString& filter)
{
	if (filter=="kate_html_export")
	{
		QString filename=KFileDialog::getSaveFileName(QString::null,QString::null,0,i18n("Export File As"));
		if (filename.isEmpty())
			{
				return;
			}
		KSaveFile *savefile=new KSaveFile(filename);
		if (!savefile->status())
		{
			if (exportDocumentToHTML(savefile->textStream(),filename)) savefile->close(); 
				else savefile->abort();
			//if (!savefile->status()) --> Error
		} else {/*ERROR*/}
		delete savefile;
	}
}

/* For now, this should become an plugin */
bool KateDocument::exportDocumentToHTML(QTextStream *outputStream,const QString &name)
{
	outputStream->setEncoding(QTextStream::UnicodeUTF8);
	// let's write the HTML header :
	(*outputStream) << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	(*outputStream) << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
	(*outputStream) << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
	(*outputStream) << "<head>" << endl;
	(*outputStream) << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />" << endl;
	(*outputStream) << "<meta name=\"Generator\" content=\"Kate, the KDE Advanced Text Editor\" />" << endl;
	// for the title, we write the name of the file (/usr/local/emmanuel/myfile.cpp -> myfile.cpp)
	(*outputStream) << "<title>" << name.right(name.length() - name.findRev('/') -1) << "</title>" << endl;
	(*outputStream) << "</head>" << endl;

	(*outputStream) << "<body><pre>" << endl;
	// for each line :

	// some variables :
	bool previousCharacterWasBold = false;
	bool previousCharacterWasItalic = false;
	// when entering a new color, we'll close all the <b> & <i> tags,
	// for HTML compliancy. that means right after that font tag, we'll
	// need to reinitialize the <b> and <i> tags.
	bool needToReinitializeTags = false;
	QColor previousCharacterColor(0,0,0); // default color of HTML characters is black
	(*outputStream) << "<span style='color=#000000'>";

	for (uint curLine=0;curLine<numLines();curLine++)
	{ // html-export that line :
		TextLine::Ptr textLine = getTextLine(curLine);
		//ASSERT(textLine != NULL);
		// for each character of the line : (curPos is the position in the line)
		for (uint curPos=0;curPos<textLine->length();curPos++)
		{
			Attribute *charAttributes = attribute(textLine->getAttr(curPos));
			//ASSERT(charAttributes != NULL);
			// let's give the color for that character :
			if ( (charAttributes->col != previousCharacterColor))
			{	// the new character has a different color :
				// if we were in a bold or italic section, close it
				if (previousCharacterWasBold)
					(*outputStream) << "</b>";
				if (previousCharacterWasItalic)
					(*outputStream) << "</i>";

				// close the previous font tag :
				(*outputStream) << "</span>";
				// let's read that color :
				int red, green, blue;
				// getting the red, green, blue values of the color :
				charAttributes->col.rgb(&red, &green, &blue);
				(*outputStream) << "<span style='color:#"
							<< ( (red < 0x10)?"0":"")  // need to put 0f, NOT f for instance. don't touch 1f.
							<< QString::number(red, 16) // html wants the hex value here (hence the 16)
							<< ( (green < 0x10)?"0":"")
							<< QString::number(green, 16)
							<< ( (blue < 0x10)?"0":"")
							<< QString::number(blue, 16)
							<< "'>";
				// we need to reinitialize the bold/italic status, since we closed all the tags
				needToReinitializeTags = true;
			}
			// bold status :
			if ( (needToReinitializeTags && charAttributes->bold) ||
			    (!previousCharacterWasBold && charAttributes->bold) )
				// we enter a bold section
				(*outputStream) << "<b>";
			if ( !needToReinitializeTags && (previousCharacterWasBold && !charAttributes->bold) )
				// we leave a bold section
				(*outputStream) << "</b>";

			// italic status :
			if ( (needToReinitializeTags && charAttributes->italic) ||
			     (!previousCharacterWasItalic && charAttributes->italic) )
				// we enter an italic section
				(*outputStream) << "<i>";
			if ( !needToReinitializeTags && (previousCharacterWasItalic && !charAttributes->italic) )
				// we leave an italic section
				(*outputStream) << "</i>";

			// write the actual character :
			(*outputStream) << HTMLEncode(textLine->getChar(curPos));

			// save status for the next character :
			previousCharacterWasItalic = charAttributes->italic;
			previousCharacterWasBold = charAttributes->bold;
			previousCharacterColor = charAttributes->col;
			needToReinitializeTags = false;
		}
		// finish the line :
		(*outputStream) << endl;
	}
	// HTML document end :
	(*outputStream) << "</span>";  // i'm guaranteed a span is started (i started one at the beginning of the output).
	(*outputStream) << "</pre></body>";
	(*outputStream) << "</html>";
	// close the file :
	return true;
}

QString KateDocument::HTMLEncode(QChar theChar)
{
	switch (theChar.latin1())
	{
	case '>':
		return QString("&gt;");
	case '<':
		return QString("&lt;");
	case '&':
		return QString("&amp;");
	};
	return theChar;
}

Kate::ConfigPage *KateDocument::colorConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new ColorConfig(p, "", this);
}

Kate::ConfigPage *KateDocument::fontConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new FontConfig(p, "", this);
}

Kate::ConfigPage *KateDocument::indentConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new IndentConfigTab(p, this);
}

Kate::ConfigPage *KateDocument::selectConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new SelectConfigTab(p, this);
}

Kate::ConfigPage *KateDocument::editConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new EditConfigTab(p, this);
}

Kate::ConfigPage *KateDocument::keysConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new EditKeyConfiguration(p);
}

Kate::ConfigPage *KateDocument::kSpellConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new KSpellConfigPage (p, this);
}

Kate::ConfigPage *KateDocument::hlConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new HlConfigPage (p, this);
}

Kate::ActionMenu *KateDocument::hlActionMenu (const QString& text, QObject* parent, const char* name)
{
  KateViewHighlightAction *menu = new KateViewHighlightAction (text, parent, name);
  menu->updateMenu (this);

  return (Kate::ActionMenu *)menu;
}

Kate::ActionMenu *KateDocument::exportActionMenu (const QString& text, QObject* parent, const char* name)
{
  KateExportAction *menu = new KateExportAction (text, parent, name);
  menu->updateMenu (this);

  return (Kate::ActionMenu *)menu;
}


void KateDocument::dumpRegionTree()
{
	regionTree->debugDump();
}

unsigned int KateDocument::getRealLine(unsigned int virtualLine)
{
	return regionTree->getRealLine(virtualLine);
}

unsigned int KateDocument::getVirtualLine(unsigned int realLine)
{
	return regionTree->getVirtualLine(realLine);
}

unsigned int KateDocument::visibleLines ()
{
	return numLines() - regionTree->getHiddenLinesCount();
}

KateCursor::KateCursor ( KateDocument *doc)
{
  myDoc = doc;
  myDoc->addCursor (this);
}

KateCursor::~KateCursor ( )
{
  myDoc->removeCursor (this);
}

void KateCursor::position ( uint *, uint * ) const
{

}

bool KateCursor::setPosition ( uint , uint  )
{
  return false;
}

bool KateCursor::insertText ( const QString&  )
{
  return false;
}

bool KateCursor::removeText ( uint  )
{
  return false;
}

QChar KateCursor::currentChar () const
{
  return QChar();
}

