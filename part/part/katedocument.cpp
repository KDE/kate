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

#include <ktexteditor/plugin.h>

#include "katefactory.h"
#include "kateviewdialog.h"
#include "katedialogs.h"
#include "katehighlight.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "katetextline.h"
#include "katecmd.h"
#include "kateexportaction.h"
#include "kateundo.h"
#include "kateprintsettings.h"

#include <qfileinfo.h>
#include <qfocusdata.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qpaintdevicemetrics.h>
#include <qiodevice.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qobject.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qptrstack.h>
#include <qdatetime.h>
#include <qmap.h>

#include <klocale.h>
#include <kglobal.h>
#include <kurldrag.h>
#include <kprinter.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <klineeditdlg.h>
#include <kconfig.h>
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
#include <klibloader.h>

#include "kateviewhighlightaction.h"

//END  includes

using namespace Kate;

LineRange::LineRange()
  : line(-1)
  , visibleLine(-1)
  , startCol(-1)
  , endCol(-1)
  , startX(-1)
  , endX(-1)
  , dirty(false)
  , viewLine(-1)
  , wrap(false)
{
}

void LineRange::clear()
{
  line = -1;
  visibleLine = -1;
  startCol = -1;
  endCol = -1;
  startX = -1;
  endX = -1;
  viewLine = -1;
  wrap = false;
}

//
// KateDocument Constructor
//
KateDocument::KateDocument ( bool bSingleViewMode, bool bBrowserView,
                                                    bool bReadOnly, QWidget *parentWidget,
                                                    const char *widgetName, QObject *, const char *)
: Kate::Document (),
  selectStart(-1, -1),
  selectEnd(-1, -1),
  oldSelectStart(-1, -1),
  oldSelectEnd(-1, -1),
  selectAnchor(-1, -1),
  lastUndoGroupWhenSaved( 0 ),
  docWasSavedWhenUndoWasEmpty( true ),
  viewFont(),
  printFont(),
  hlManager(HlManager::self ())
{
  KateFactory::registerDocument (this);

  setBlockSelectionInterfaceDCOPSuffix (documentDCOPSuffix());
  setConfigInterfaceDCOPSuffix (documentDCOPSuffix());
  setConfigInterfaceExtensionDCOPSuffix (documentDCOPSuffix());
  setCursorInterfaceDCOPSuffix (documentDCOPSuffix());
  //setDocumentInfoInterfaceDCOPSuffix (documentDCOPSuffix());
  setEditInterfaceDCOPSuffix (documentDCOPSuffix());
  setEncodingInterfaceDCOPSuffix (documentDCOPSuffix());
  setHighlightingInterfaceDCOPSuffix (documentDCOPSuffix());
  setMarkInterfaceDCOPSuffix (documentDCOPSuffix());
  setMarkInterfaceExtensionDCOPSuffix (documentDCOPSuffix());
  setPrintInterfaceDCOPSuffix (documentDCOPSuffix());
  setSearchInterfaceDCOPSuffix (documentDCOPSuffix());
  setSelectionInterfaceDCOPSuffix (documentDCOPSuffix());
  setSelectionInterfaceExtDCOPSuffix (documentDCOPSuffix());
  setSessionConfigInterfaceDCOPSuffix (documentDCOPSuffix());
  setUndoInterfaceDCOPSuffix (documentDCOPSuffix());
  setWordWrapInterfaceDCOPSuffix (documentDCOPSuffix());

  m_activeView = 0L;

  hlSetByUser = false;
  setInstance( KateFactory::instance() );

  editSessionNumber = 0;
  editIsRunning = false;
  noViewUpdates = false;
  editCurrentUndo = 0L;
  editWithUndo = false;

  blockSelect = false;

  m_bSingleViewMode = bSingleViewMode;
  m_bBrowserView = bBrowserView;
  m_bReadOnly = bReadOnly;

  m_marks.setAutoDelete( true );
  m_markPixmaps.setAutoDelete( true );
  m_markDescriptions.setAutoDelete( true );
  restoreMarks = false;
  setMarksUserChangable( markType01 );

  readOnly = false;
  newDoc = false;

  modified = false;

  // some defaults
  _configFlags = KateDocument::cfAutoIndent | KateDocument::cfTabIndents | KateDocument::cfKeepIndentProfile
    | KateDocument::cfRemoveSpaces
    | KateDocument::cfDelOnInput | KateDocument::cfWrapCursor
    | KateDocument::cfShowTabs | KateDocument::cfSmartHome;

  myEncoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());

  setFont (ViewFont,KGlobalSettings::fixedFont());
  setFont (PrintFont,KGlobalSettings::fixedFont());

  m_docName = QString ("");
  fileInfo = new QFileInfo ();

  myCmd = new KateCmd (this);

  connect(this,SIGNAL(modifiedChanged ()),this,SLOT(slotModChanged ()));

  buffer = new KateBuffer (this);

  connect(buffer, SIGNAL(loadingFinished()), this, SLOT(slotLoadingFinished()));
  connect(buffer, SIGNAL(linesChanged(int)), this, SLOT(slotBufferChanged()));
  connect(buffer, SIGNAL(tagLines(int,int)), this, SLOT(tagLines(int,int)));
  connect(buffer,SIGNAL(codeFoldingUpdated()),this,SIGNAL(codeFoldingUpdated()));

  colors[0] = KGlobalSettings::baseColor();
  colors[1] = KGlobalSettings::highlightColor();
  colors[2] = KGlobalSettings::alternateBackgroundColor();
  colors[3] = QColor( "#FFFF99" );

  m_highlight = 0L;
  tabChars = 8;

  KTrader::OfferList::Iterator it(KateFactory::plugins()->begin());
  for( ; it != KateFactory::plugins()->end(); ++it)
  {
    KService::Ptr ptr = (*it);

    PluginInfo *info=new PluginInfo;

    info->load = false;
    info->service = ptr;
    info->plugin = 0L;

    m_plugins.append(info);
  }

  clear();
  docWasSavedWhenUndoWasEmpty = true;

  // if the user changes the highlight with the dialog, notify the doc
  connect(hlManager,SIGNAL(changed()),SLOT(internalHlChanged()));

  readConfig();
  loadAllEnabledPlugins ();

  m_extension = new KateBrowserExtension( this );

  if ( m_bSingleViewMode )
  {
    KTextEditor::View *view = createView( parentWidget, widgetName );
    insertChildClient( view );
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
    m_views.setAutoDelete( true );
    m_views.clear();
    m_views.setAutoDelete( false );
  }

  m_highlight->release();

  KateFactory::deregisterDocument (this);
  delete fileInfo;
}

void KateDocument::loadAllEnabledPlugins ()
{
  for (uint i=0; i<m_plugins.count(); i++)
  {
    if  (m_plugins.at(i)->load)
      loadPlugin (m_plugins.at(i));
  }
}

void KateDocument::enableAllPluginsGUI (KateView *view)
{
  for (uint i=0; i<m_plugins.count(); i++)
  {
    if  (m_plugins.at(i)->load)
      enablePluginGUI (m_plugins.at(i), view);
  }
}

void KateDocument::loadPlugin (PluginInfo *item)
{
  item->load = (item->plugin = KTextEditor::createPlugin (QFile::encodeName(item->service->library()), this));
}

void KateDocument::unloadPlugin (PluginInfo *item)
{
  item->load = false;

  if (!item->plugin) return;

  disablePluginGUI (item);

  delete item->plugin;
  item->plugin = 0L;
}

void KateDocument::enablePluginGUI (PluginInfo *item, KateView *view)
{
  if (!item->plugin) return;
  if (!KTextEditor::pluginViewInterface(item->plugin)) return;

  KTextEditor::pluginViewInterface(item->plugin)->addView(view);

  if (KXMLGUIFactory *factory = view->factory())
  {
    factory->removeClient (view);
    factory->addClient (view);
  }
}

void KateDocument::enablePluginGUI (PluginInfo *item)
{
  if (!item->plugin) return;
  if (!KTextEditor::pluginViewInterface(item->plugin)) return;

  for (uint i=0; i< m_views.count(); i++)
  {
    enablePluginGUI (item, m_views.at(i));
  }
}

void KateDocument::disablePluginGUI (PluginInfo *item)
{
  if (!item->plugin) return;
  if (!KTextEditor::pluginViewInterface(item->plugin)) return;

  for (uint i=0; i< m_views.count(); i++)
  {
      KXMLGUIFactory *factory = m_views.at( i )->factory();
      if ( factory )
	  factory->removeClient( m_views.at( i ) );

      KTextEditor::PluginViewInterface *viewIface = KTextEditor::pluginViewInterface( item->plugin );
      viewIface->removeView(m_views.at(i));

      if ( factory )
          factory->addClient( m_views.at( i ) );
   }
}

bool KateDocument::closeURL()
{
  if (!KParts::ReadWritePart::closeURL ())
    return false;

  m_url = KURL();
  fileInfo->setFile (QString());
  setMTime();

  clear();
  updateViews();

  emit fileNameChanged ();

  return true;
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
  return m_textEditViews;
};

//
// KTextEditor::ConfigInterfaceExtension stuff
//

uint KateDocument::configPages () const
{
  return 9;
}

KTextEditor::ConfigPage *KateDocument::configPage (uint number, QWidget *parent, const char * )
{
  switch( number )
  {
    case 0:
      return colorConfigPage(parent);

    case 1:
      return fontConfigPage(parent);

    case 2:
      return indentConfigPage(parent);

    case 3:
      return selectConfigPage(parent);

    case 4:
      return editConfigPage (parent);

    case 5:
      return keysConfigPage (parent);

    case 6:
      return hlConfigPage (parent);

    case 7:
      return viewDefaultsConfigPage(parent);

    case 8:
      return new PluginConfigPage (parent, this);

    default:
      return 0;
  }
}

QString KateDocument::configPageName (uint number) const
{
  switch( number )
  {
    case 0:
      return i18n ("Colors");

    case 1:
      return i18n ("Fonts");

    case 2:
      return i18n ("Indentation");

    case 3:
      return i18n ("Selection");

    case 4:
      return i18n ("Editing");

    case 5:
      return i18n ("Shortcuts");

    case 6:
      return i18n ("Highlighting");

    case 7:
      return i18n ("View defaults");

    case 8:
      return i18n ("Plugins");

    default:
      return 0;
  }
}

QString KateDocument::configPageFullName (uint number) const
{
  switch( number )
  {
    case 0:
      return i18n ("Color Settings");

    case 1:
      return i18n ("Font Settings");

    case 2:
      return i18n ("Indentation Rules");

    case 3:
      return i18n ("Selection Behavior");

    case 4:
      return i18n ("Editing Options");

    case 5:
      return i18n ("Shortcuts Configuration");

    case 6:
      return i18n ("Highlighting Rules");

    case 7:
      return i18n("View Defaults");

    case 8:
      return i18n ("Plugin Manager");

    default:
      return 0;
  }
}

QPixmap KateDocument::configPagePixmap (uint number, int size) const
{
  switch( number )
  {
    case 0:
      return BarIcon("colorize", size);

    case 1:
      return BarIcon("fonts", size);

    case 2:
      return BarIcon("rightjust", size);

    case 3:
      return BarIcon("misc", size);

    case 4:
      return BarIcon("edit", size);

    case 5:
      return BarIcon("misc", size);

    case 6:
      return BarIcon("misc", size);

    case 7:
      return BarIcon("misc",size);

    case 8:
      return BarIcon("misc", size);

    default:
      return 0;
  }
}

//
// KTextEditor::EditInterface stuff
//

QString KateDocument::text() const
{
  return buffer->text();
}

QString KateDocument::text ( uint startLine, uint startCol, uint endLine, uint endCol ) const
{
  return text(startLine, startCol, endLine, endCol, false);
}

QString KateDocument::text ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise) const
{
  return buffer->text(startLine, startCol, endLine, endCol, blockwise);
}

QString KateDocument::textLine( uint line ) const
{
  return buffer->textLine(line);
}

bool KateDocument::setText(const QString &s)
{
  clear();
  return insertText (0, 0, s);
}

bool KateDocument::clear()
{
  for (KateView * view = m_views.first(); view != 0L; view = m_views.next() ) {
    view->m_viewInternal->clear();
    view->m_viewInternal->tagAll();
    view->m_viewInternal->update();
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

bool KateDocument::insertText( uint line, uint col, const QString &s)
{
  return insertText (line, col, s, false);
}

bool KateDocument::insertText( uint line, uint col, const QString &s, bool blockwise )
{
  if (s.isEmpty())
    return true;

	if (line == numLines())
		editInsertLine(line,"");
	else if (line > lastLine())
    return false;

  editStart ();

  uint insertPos = col;
  uint len = s.length();
  QString buf;

  for (uint pos = 0; pos < len; pos++)
  {
    QChar ch = s[pos];

    if (ch == '\n')
    {
      if ( !blockwise )
      {
        editInsertText (line, insertPos, buf);
        editWrapLine (line, insertPos + buf.length());
      }
      else
      {
        editInsertText (line, col, buf);

        if ( line == lastLine() )
          editWrapLine (line, col + buf.length());
      }

      line++;
      insertPos = 0;
      buf.truncate(0);
    }
    else
      buf += ch; // append char to buffer
  }

  if ( !blockwise )
    editInsertText (line, insertPos, buf);
  else
    editInsertText (line, col, buf);

  editEnd ();

  return true;
}

bool KateDocument::removeText ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  return removeText (startLine, startCol, endLine, endCol, false);
}

bool KateDocument::removeText ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise )
{
  if ( blockwise && (startCol > endCol) )
    return false;

  if ( startLine > endLine )
    return false;

  if ( startLine > lastLine() )
    return false;

  editStart ();

  if ( !blockwise )
  {
    if ( endLine > lastLine() )
    {
      endLine = lastLine()+1;
      endCol = 0;
    }

    if (startLine == endLine)
    {
      editRemoveText (startLine, startCol, endCol-startCol);
    }
    else if ((startLine+1) == endLine)
    {
      if ( (buffer->plainLine(startLine)->length()-startCol) > 0 )
        editRemoveText (startLine, startCol, buffer->plainLine(startLine)->length()-startCol);

      editRemoveText (startLine+1, 0, endCol);
      editUnWrapLine (startLine, startCol);
    }
    else
    {
      for (uint line = endLine; line >= startLine; line--)
      {
        if ((line > startLine) && (line < endLine))
        {
          editRemoveLine (line);
        }
        else
        {
          if (line == endLine)
          {
            if ( endLine <= lastLine() )
              editRemoveText (line, 0, endCol);
          }
          else
          {
            if ( (buffer->plainLine(line)->length()-startCol) > 0 )
              editRemoveText (line, startCol, buffer->plainLine(line)->length()-startCol);

            editUnWrapLine (startLine, startCol);
          }
        }

        if ( line == 0 )
          break;
      }
    }
  }
  else
  {
    if ( endLine > lastLine() )
      endLine = lastLine ();

    for (uint line = endLine; line >= startLine; line--)
    {
      editRemoveText (line, startCol, endCol-startCol);

      if ( line == 0 )
        break;
    }
  }

  editEnd ();

  return true;
}

bool KateDocument::insertLine( uint l, const QString &str )
{
  if (l > numLines())
    return false;

  editStart ();

  editInsertLine (l, str);

  editEnd ();

  return true;
}

bool KateDocument::removeLine( uint line )
{
  if (line > lastLine())
    return false;

  editStart ();

  bool end = editRemoveLine (line);

  editEnd ();

  return end;
}

uint KateDocument::length() const
{
  return buffer->length();
}

uint KateDocument::numLines() const
{
  return buffer->count();
}

uint KateDocument::numVisLines() const
{
  return buffer->countVisible ();
}

int KateDocument::lineLength ( uint line ) const
{
  return buffer->lineLength(line);
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

  buffer->setHlUpdate (false);

  editIsRunning = true;
  noViewUpdates = true;
  editWithUndo = withUndo;

  editTagLineStart = 0xffffff;
  editTagLineEnd = 0;

  if (editWithUndo)
  {
    if ((myUndoSteps > 0) && (undoItems.count () > myUndoSteps))
    {
      undoItems.setAutoDelete (true);
      undoItems.removeFirst ();
      undoItems.setAutoDelete (false);
      docWasSavedWhenUndoWasEmpty = false;
    }

    editCurrentUndo = new KateUndoGroup (this);
  }
  else
    editCurrentUndo = 0L;

  for (uint z = 0; z < m_views.count(); z++)
  {
    (m_views.at(z))->m_viewInternal->editStart();
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

  buffer->setHlUpdate (true);

  if (editTagLineStart <= editTagLineEnd)
    updateLines(editTagLineStart, editTagLineEnd);

  if (editWithUndo && editCurrentUndo)
  {
    undoItems.append (editCurrentUndo);
    editCurrentUndo = 0L;
    emit undoChanged ();
  }

  for (uint z = 0; z < m_views.count(); z++)
  {
    (m_views.at(z))->m_viewInternal->editEnd(editTagLineStart, editTagLineEnd);
  }

  setModified(true);
  emit textChanged ();

  noViewUpdates = false;
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
    TextLine::Ptr l = buffer->plainLine(line);

    if (l->length() > col)
    {
      const QChar *text = l->text();

      for (z=col; z>0; z--)
      {
        if (z < 1) break;
        if (text[z].isSpace()) break;
      }

      if (!(z < 1))
      {
        z++; // (anders: avoid the space at the beginning of the line)
        editWrapLine (line, z, true);
        endLine++;
      }
      else
      {
        editWrapLine (line, col, true);
        endLine++;
      }
    }

    line++;

    if (line >= numLines()) break;
  };

  editEnd ();

  return true;
}

void KateDocument::editAddUndo (uint type, uint line, uint col, uint len, const QString &text)
{
  if (editIsRunning && editWithUndo && editCurrentUndo)
    editCurrentUndo->addItem (type, line, col, len, text);
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
  TextLine::Ptr l = buffer->plainLine(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editInsertText, line, col, s.length(), s);

  l->insertText (col, s.length(), s.unicode());

  buffer->changeLine(line);
  editTagLine (line);

  editEnd ();

  return true;
}

bool KateDocument::editRemoveText ( uint line, uint col, uint len )
{
  TextLine::Ptr l = buffer->plainLine(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editRemoveText, line, col, len, l->string().mid(col, len));

  l->removeText (col, len);

  buffer->changeLine(line);

  editTagLine(line);

  for (uint z = 0; z < m_views.count(); z++)
  {
    (m_views.at(z))->m_viewInternal->editRemoveText(line, col, len);
  }

  editEnd ();

  return true;
}

bool KateDocument::editWrapLine ( uint line, uint col, bool autowrap)
{
  TextLine::Ptr l = buffer->plainLine(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editWrapLine, line, col, 0, 0);

  TextLine::Ptr nl = buffer->plainLine(line+1);
  TextLine::Ptr tl = new TextLine();
  int llen = l->length(), nllen;

  if (!nl || !autowrap)
  {
    l->wrap (tl, col);

    buffer->insertLine (line+1, tl);
    buffer->changeLine(line);
    QPtrList<KTextEditor::Mark> list;
    for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
         it.current(); ++it ) {
      if( it.current()->line > line || ( col == 0 && it.current()->line == line ) )
        list.append( it.current() );
    }

    for( QPtrListIterator<KTextEditor::Mark> it( list );
         it.current(); ++it ) {
      KTextEditor::Mark* mark = m_marks.take( it.current()->line );
      mark->line++;
      m_marks.insert( mark->line, mark );
    }
    if( !list.isEmpty() )
      emit marksChanged();

     editInsertTagLine (line);
  }
  else
  {
    int nlsave = nl->length();
    l->wrap (nl, col);
    nllen = nl->length() - nlsave;

    buffer->changeLine(line);
    buffer->changeLine(line+1);
  }

  editTagLine(line);
  editTagLine(line+1);

  for (uint z = 0; z < m_views.count(); z++)
  {
    if(!autowrap)
      (m_views.at(z))->m_viewInternal->editWrapLine(line, col, tl->length());
    else
    {
      int offset = llen - (m_views.at(z))->m_viewInternal->cursorCache.col;
      offset = (nl ? nllen:tl->length()) - offset;
      if(offset < 0) offset = 0;
      (m_views.at(z))->m_viewInternal->editWrapLine(line, col, offset);
    }
  }
  editEnd ();
  return true;
}

bool KateDocument::editUnWrapLine ( uint line, uint col )
{
  TextLine::Ptr l = buffer->plainLine(line);
  TextLine::Ptr tl = buffer->plainLine(line+1);

  if (!l || !tl)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editUnWrapLine, line, col, 0, 0);

  l->unWrap (col, tl, tl->length());

  buffer->changeLine(line);
  buffer->removeLine(line+1);

  QPtrList<KTextEditor::Mark> list;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
       it.current(); ++it ) {
    if( it.current()->line > line )
      list.append( it.current() );
  }
  for( QPtrListIterator<KTextEditor::Mark> it( list );
       it.current(); ++it ) {
    KTextEditor::Mark* mark = m_marks.take( it.current()->line );
    mark->line--;
    m_marks.insert( mark->line, mark );
  }
  if( !list.isEmpty() )
    emit marksChanged();

  editRemoveTagLine(line);
  editTagLine(line);
  editTagLine(line+1);

  for (uint z = 0; z < m_views.count(); z++)
  {
    (m_views.at(z))->m_viewInternal->editUnWrapLine(line, col);
  }

  editEnd ();

  return true;
}

bool KateDocument::editInsertLine ( uint line, const QString &s )
{
  if ( line > numLines() )
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editInsertLine, line, 0, s.length(), s);

  TextLine::Ptr tl = new TextLine();
  tl->append(s.unicode(),s.length());
  buffer->insertLine(line, tl);
  buffer->changeLine(line);

  editInsertTagLine (line);
  editTagLine(line);

  QPtrList<KTextEditor::Mark> list;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
       it.current(); ++it ) {
    if( it.current()->line >= line )
      list.append( it.current() );
  }
  for( QPtrListIterator<KTextEditor::Mark> it( list );
       it.current(); ++it ) {
       kdDebug(13000)<<"editInsertLine: updating mark at line "<<it.current()->line<<endl;
    KTextEditor::Mark* mark = m_marks.take( it.current()->line );
    mark->line++;
    m_marks.insert( mark->line, mark );
  }
  if( !list.isEmpty() )
    emit marksChanged();

  for (uint z = 0; z < m_views.count(); z++)
  {
    (m_views.at(z))->m_viewInternal->setViewTagLinesFrom(line);
  }

  editEnd ();

  return true;
}

bool KateDocument::editRemoveLine ( uint line )
{
  if ( line > lastLine() )
    return false;

  if ( numLines() == 1 )
    return editRemoveText (0, 0, buffer->plainLine(0)->length());

  editStart ();

  editAddUndo (KateUndoGroup::editRemoveLine, line, 0, lineLength(line), textLine(line));

  buffer->removeLine(line);

  editRemoveTagLine (line);

  QPtrList<KTextEditor::Mark> list;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
       it.current(); ++it ) {
    if( it.current()->line >= line )
      list.append( it.current() );
  }
  for( QPtrListIterator<KTextEditor::Mark> it( list );
       it.current(); ++it ) {
    KTextEditor::Mark* mark = m_marks.take( it.current()->line );
    mark->line--;
    m_marks.insert( mark->line, mark );
  }
  if( !list.isEmpty() )
    emit marksChanged();

  for (uint z = 0; z < m_views.count(); z++)
  {
    (m_views.at(z))->m_viewInternal->editRemoveLine(line);
  }

  editEnd();

  return true;
}

//
// KTextEditor::SelectionInterface stuff
//

bool KateDocument::setSelection( const KateTextCursor& start, const KateTextCursor& end )
{
  oldSelectStart = selectStart;
  oldSelectEnd = selectEnd;

  if (start <= end) {
    selectStart.setPos(start);
    selectEnd.setPos(end);
  } else {
    selectStart.setPos(end);
    selectEnd.setPos(start);
  }

  if (hasSelection() || selectAnchor.line != -1)
    tagSelection();

  repaintViews();

  emit selectionChanged ();

  return true;
}

bool KateDocument::setSelection( uint startLine, uint startCol, uint endLine, uint endCol )
{
  if (hasSelection())
    clearSelection(false);

  selectAnchor.line = startLine;
  selectAnchor.col = startCol;

  return setSelection( KateTextCursor(startLine, startCol), KateTextCursor(endLine, endCol) );
}

bool KateDocument::clearSelection()
{
  return clearSelection(true);
}

bool KateDocument::clearSelection(bool redraw)
{
  if( !hasSelection() )
    return false;

  oldSelectStart = selectStart;
  oldSelectEnd = selectEnd;

  selectStart.setPos(-1, -1);
  selectEnd.setPos(-1, -1);
  selectAnchor.setPos(-1, -1);

  tagSelection();

  oldSelectStart = selectStart;
  oldSelectEnd = selectEnd;

  if (redraw)
    repaintViews();

  emit selectionChanged();

  return true;
}

bool KateDocument::hasSelection() const
{
  return selectStart != selectEnd;
}

QString KateDocument::selection() const
{
  int sc = selectStart.col;
  int ec = selectEnd.col;

  if ( blockSelect )
  {
    if (sc > ec)
    {
      uint tmp = sc;
      sc = ec;
      ec = tmp;
    }
  }

  return text (selectStart.line, sc, selectEnd.line, ec, blockSelect);
}

bool KateDocument::removeSelectedText ()
{
  if (!hasSelection())
    return false;

  editStart ();

  for (uint z = 0; z < m_views.count(); z++)
  {
    KateViewInternal *v = (m_views.at(z))->m_viewInternal;
    if (lineHasSelected(v->cursorCache.line))
    {
      v->cursorCache = selectStart;
      v->cursorCacheChanged = true;
    }
  }

  int sc = selectStart.col;
  int ec = selectEnd.col;

  if ( blockSelect )
  {
    if (sc > ec)
    {
      uint tmp = sc;
      sc = ec;
      ec = tmp;
    }
  }

  removeText (selectStart.line, sc, selectEnd.line, ec, blockSelect);

  // don't redraw the cleared selection - that's done in editEnd().
  clearSelection(false);

  editEnd ();

  return true;
}

bool KateDocument::selectAll()
{
  return setSelection (0, 0, lastLine(), lineLength(lastLine()));
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
    oldSelectStart = selectStart;
    oldSelectEnd = selectEnd;
    clearSelection();
    setSelection(oldSelectStart, oldSelectEnd);
  }

  return true;
}

bool KateDocument::toggleBlockSelectionMode ()
{
  return setBlockSelectionMode (!blockSelect);
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
  if ((undoItems.count() > 0) && undoItems.last())
  {
    undoItems.last()->undo();
    redoItems.append (undoItems.last());
    undoItems.removeLast ();
    updateModified();

    emit undoChanged ();
  }
}

void KateDocument::redo()
{
  if ((redoItems.count() > 0) && redoItems.last())
  {
    redoItems.last()->redo();
    undoItems.append (redoItems.last());
    redoItems.removeLast ();
    updateModified();

    emit undoChanged ();
  }
}

void KateDocument::updateModified()
{
  if ( ( lastUndoGroupWhenSaved &&
         !undoItems.isEmpty() &&
         undoItems.last() == lastUndoGroupWhenSaved )
       || ( undoItems.isEmpty() && docWasSavedWhenUndoWasEmpty && myUndoSteps != 0 ) )
  {
    setModified( false );
    kdDebug() << k_funcinfo << "setting modified to false !" << endl;
  };
};

void KateDocument::clearUndo()
{
  undoItems.setAutoDelete (true);
  undoItems.clear ();
  undoItems.setAutoDelete (false);

  lastUndoGroupWhenSaved = 0;
  docWasSavedWhenUndoWasEmpty = false;

  emit undoChanged ();
}

void KateDocument::clearRedo()
{
  redoItems.setAutoDelete (true);
  redoItems.clear ();
  redoItems.setAutoDelete (false);

  emit undoChanged ();
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

  Q_ASSERT( startLine < numLines() );

  if (text.isEmpty())
    return false;

  line = startLine;
  col = startCol;

  if (!backwards)
  {
    searchEnd = lastLine();

    while (line <= searchEnd)
    {
      textLine = buffer->plainLine(line);

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
      textLine = buffer->plainLine(line);

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
        col = lineLength(line-1);

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
      textLine = buffer->plainLine(line);

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
      textLine = buffer->plainLine(line);

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
        col = lineLength(line-1);

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

  emit hlChanged();
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
  config->setGroup("Kate Document");
  _configFlags = config->readNumEntry("ConfigFlags", _configFlags);

  myWordWrap = config->readBoolEntry("Word Wrap On", false);
  myWordWrapAt = config->readNumEntry("Word Wrap At", 80);

  setTabWidth(config->readNumEntry("TabWidth", 8));
  setUndoSteps(config->readNumEntry("UndoSteps", 256));
  setFont (ViewFont,config->readFontEntry("Font", &viewFont.myFont));
  setFont (PrintFont,config->readFontEntry("PrintFont", &printFont.myFont));

  colors[0] = config->readColorEntry("Color Background", &colors[0]);
  colors[1] = config->readColorEntry("Color Selected", &colors[1]);
  colors[2] = config->readColorEntry("Color Current Line", &colors[2]);
  colors[3] = config->readColorEntry("Color Bracket Highlight", &colors[3]);

  config->setGroup("Kate Plugins");
  for (uint i=0; i<m_plugins.count(); i++)
    if  (config->readBoolEntry(m_plugins.at(i)->service->library(), false))
      m_plugins.at(i)->load = true;

  if (myWordWrap)
  {
    editStart (false);
    wrapText (myWordWrapAt);
    editEnd ();
    setModified(false);
    emit textChanged ();
  }

  config->setGroup("Kate View");
  m_dynWordWrap = config->readBoolEntry( "DynamicWordWrap", false );
  m_lineNumbers = config->readBoolEntry( "LineNumbers", false );
  m_iconBar = config->readBoolEntry( "Iconbar", false );
  m_foldingBar = config->readBoolEntry( "FoldingMarkers", true );
  m_bookmarkSort = config->readNumEntry( "Bookmark Menu Sorting", 0 );

  updateViewDefaults ();
  tagAll();
}

void KateDocument::updateViewDefaults ()
{
  for (uint i=0; i<m_views.count(); i++)
    m_views.at(i)->updateViewDefaults ();
}

void KateDocument::writeConfig(KConfig *config)
{
  config->setGroup("Kate Document");
  config->writeEntry("ConfigFlags",_configFlags);

  config->writeEntry("Word Wrap On", myWordWrap);
  config->writeEntry("Word Wrap At", myWordWrapAt);
  config->writeEntry("UndoSteps", myUndoSteps);
  config->writeEntry("TabWidth", tabChars);
  config->writeEntry("Font", viewFont.myFont);
  config->writeEntry("PrintFont", printFont.myFont);
  config->writeEntry("Color Background", colors[0]);
  config->writeEntry("Color Selected", colors[1]);
  config->writeEntry("Color Current Line", colors[2]);
  config->writeEntry("Color Bracket Highlight", colors[3]);

  config->setGroup("Kate Plugins");
  for (uint i=0; i<m_plugins.count(); i++)
    config->writeEntry(m_plugins.at(i)->service->library(), m_plugins.at(i)->load);

  config->setGroup("Kate View");
  config->writeEntry( "DynamicWordWrap", m_dynWordWrap );
  config->writeEntry( "LineNumbers", m_lineNumbers );
  config->writeEntry( "Iconbar", m_iconBar );
  config->writeEntry( "FoldingMarkers", m_foldingBar );
  config->writeEntry( "Bookmark Menu Sorting", m_bookmarkSort );
}

void KateDocument::readConfig()
{
  KConfig *config = KateFactory::instance()->config();
  readConfig (config);
  config->sync();
}

void KateDocument::writeConfig()
{
  KConfig *config = KateFactory::instance()->config();
  writeConfig (config);
  config->sync();
}

void KateDocument::readSessionConfig(KConfig *config)
{
  m_url = config->readEntry("URL"); // ### doesn't this break the encoding? (Simon)
  internalSetHlMode(hlManager->nameFind(config->readEntry("Highlight")));
  QString tmpenc=config->readEntry("Encoding");

  if (m_url.isValid() && (!tmpenc.isEmpty()) && (tmpenc!=myEncoding))
  {
	kdDebug()<<"Reloading document because of encoding change to "<<tmpenc<<endl;
	setEncoding(tmpenc);
	reloadFile();
  }

  // Restore Bookmarks
  restoreMarks = true; // Hack: make sure we can set marks for lines > lastLine
  QValueList<int> marks = config->readIntListEntry("Bookmarks");
  for( uint i = 0; i < marks.count(); i++ )
    addMark( marks[i], KateDocument::markType01 );
  restoreMarks = false;
}

void KateDocument::writeSessionConfig(KConfig *config)
{
  config->writeEntry("URL", m_url.url() ); // ### encoding?? (Simon)
  config->writeEntry("Highlight", m_highlight->name());
  config->writeEntry("Encoding",myEncoding);
  // Save Bookmarks
  QValueList<int> marks;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
       it.current() && it.current()->type & KTextEditor::MarkInterface::markType01; ++it ) {
     marks << it.current()->line;
  }
  if( !marks.isEmpty() )
    config->writeEntry( "Bookmarks", marks );
}

void KateDocument::configDialog()
{
  KWin kwin;

  KDialogBase *kd = new KDialogBase(KDialogBase::IconList,
                                    i18n("Configure"),
                                    KDialogBase::Ok | KDialogBase::Cancel |
                                    KDialogBase::Help ,
                                    KDialogBase::Ok, kapp->mainWidget());

  kwin.setIcons(kd->winId(), kapp->icon(), kapp->miniIcon());

  QPtrList<KTextEditor::ConfigPage> editorPages;

  for (uint i = 0; i < KTextEditor::configInterfaceExtension (this)->configPages (); i++)
  {
    QStringList path;
    path.clear();
    path << KTextEditor::configInterfaceExtension (this)->configPageName (i);
    QVBox *page = kd->addVBoxPage(path, KTextEditor::configInterfaceExtension (this)->configPageFullName (i),
                              KTextEditor::configInterfaceExtension (this)->configPagePixmap(i, KIcon::SizeMedium) );

    editorPages.append (KTextEditor::configInterfaceExtension (this)->configPage(i, page));
  }

  if (kd->exec())
  {
    for (uint i=0; i<editorPages.count(); i++)
    {
      editorPages.at(i)->apply();
    }

    // save the config, reload it to update doc + all views
    writeConfig();
    readConfig();
  }

  delete kd;
}

uint KateDocument::mark( uint line )
{
  if( !m_marks[line] )
    return 0;
  return m_marks[line]->type;
}

void KateDocument::setMark( uint line, uint markType )
{
  clearMark( line );
  addMark( line, markType );
}

void KateDocument::clearMark( uint line )
{
  if( !restoreMarks && line > lastLine() )
    return;
  if( !m_marks[line] )
    return;

  KTextEditor::Mark* mark = m_marks.take( line );
  emit markChanged( *mark, MarkRemoved );
  emit marksChanged();
  delete mark;
  tagLines( line, line );
}

void KateDocument::addMark( uint line, uint markType )
{
  if( !restoreMarks && line > lastLine())
    return;
  if( markType == 0 )
    return;

  if( m_marks[line] ) {
    KTextEditor::Mark* mark = m_marks[line];

    // Remove bits already set
    markType &= ~mark->type;

    if( markType == 0 )
      return;

    // Add bits
    mark->type |= markType;
  } else {
    KTextEditor::Mark *mark = new KTextEditor::Mark;
    mark->line = line;
    mark->type = markType;
    m_marks.insert( line, mark );
  }

  // Emit with a mark having only the types added.
  KTextEditor::Mark temp;
  temp.line = line;
  temp.type = markType;
  emit markChanged( temp, MarkAdded );

  emit marksChanged();
  tagLines( line, line );
}

void KateDocument::removeMark( uint line, uint markType )
{
  if( line > lastLine() )
    return;
  if( !m_marks[line] )
    return;

  KTextEditor::Mark* mark = m_marks[line];

  // Remove bits not set
  markType &= mark->type;

  if( markType == 0 )
    return;

  // Subtract bits
  mark->type &= ~markType;

  // Emit with a mark having only the types removed.
  KTextEditor::Mark temp;
  temp.line = line;
  temp.type = markType;
  emit markChanged( temp, MarkRemoved );

  if( mark->type == 0 )
    m_marks.remove( line );

  emit marksChanged();
  tagLines( line, line );
}

QPtrList<KTextEditor::Mark> KateDocument::marks()
{
  QPtrList<KTextEditor::Mark> list;

  for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
       it.current(); ++it ) {
    list.append( it.current() );
  }

  return list;
}

void KateDocument::clearMarks()
{
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
       it.current(); ++it ) {
    KTextEditor::Mark* mark = it.current();
    emit markChanged( *mark, MarkRemoved );
    tagLines( mark->line, mark->line );
  }

  m_marks.clear();

  emit marksChanged();
}

void KateDocument::setPixmap( MarkInterface::MarkTypes type, const QPixmap& pixmap )
{
  m_markPixmaps.replace( type, new QPixmap( pixmap ) );
}

void KateDocument::setDescription( MarkInterface::MarkTypes type, const QString& description )
{
  m_markDescriptions.replace( type, new QString( description ) );
}

QPixmap KateDocument::markPixmap( MarkInterface::MarkTypes type )
{
  if( m_markPixmaps[type] )
    return *m_markPixmaps[type];
  return QPixmap();
}

QString KateDocument::markDescription( MarkInterface::MarkTypes type )
{
  if( m_markDescriptions[type] )
    return *m_markDescriptions[type];
  return QString::null;
}

void KateDocument::setMarksUserChangable( uint markMask )
{
  m_editableMarks = markMask;
}

uint KateDocument::editableMarks()
{
  return m_editableMarks;
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

  KatePrintTextSettings *kpts = new KatePrintTextSettings(&printer, NULL);
  kpts->enableSelection( hasSelection() );
  printer.addDialogPage( kpts );
  printer.addDialogPage( new KatePrintHeaderFooter(&printer, NULL) );
  printer.addDialogPage( new KatePrintLayout(&printer, NULL) );

   if ( printer.setup( kapp->mainWidget() ) )
   {
     QPainter paint( &printer );
     QPaintDeviceMetrics pdm( &printer );
     /*
        We work in tree cycles:
        1) initialize variables and retrieve print settings
        2) prepare data according to those settings
        3) draw to the printer
     */
     uint pdmWidth = pdm.width();
     uint y = 0;
     uint xstart = 0; // beginning point for painting lines
     uint lineCount = 0;
     uint maxWidth = pdmWidth;
     uint headerWidth = pdmWidth;
     int startCol = 0;
     int endCol = 0;
     bool needWrap = true;
     bool pageStarted = true;

//     kdDebug(13020)<<"pdm width: "<<pdmWidth<<endl;

     // Text Settings Page
     bool selectionOnly = ( hasSelection() &&
                           ( printer.option("app-kate-printselection") == "true" ) );
     int selStartCol = 0;
     int selEndCol = 0;

     bool useGuide = ( printer.option("app-kate-printguide") == "true" );
     int guideHeight = 0;
     int guideCols = 0;

     bool printLineNumbers = ( printer.option("app-kate-printlinenumbers") == "true" );
     uint lineNumberWidth( 0 );

     // Header/Footer Page
     QFont headerFont; // used for header/footer
     headerFont.fromString( printer.option("app-kate-hffont") );

     bool useHeader = (printer.option("app-kate-useheader") == "true");
     QColor headerBgColor(printer.option("app-kate-headerbg"));
     QColor headerFgColor(printer.option("app-kate-headerfg"));
     uint headerHeight( 0 ); // further init only if needed
     QStringList headerTagList; // do
     bool headerDrawBg = false; // do

     bool useFooter = (printer.option("app-kate-usefooter") == "true");
     QColor footerBgColor(printer.option("app-kate-footerbg"));
     QColor footerFgColor(printer.option("app-kate-footerfg"));
     uint footerHeight( 0 ); // further init only if needed
     QStringList footerTagList = 0; // do
     bool footerDrawBg = 0; // do

     // Layout Page
     bool useBackground = ( printer.option("app-kate-usebackground") == "true" );
     bool useBox = (printer.option("app-kate-usebox") == "true");
     int boxWidth(printer.option("app-kate-boxwidth").toInt());
     QColor boxColor(printer.option("app-kate-boxcolor"));
     int innerMargin = useBox ? printer.option("app-kate-boxmargin").toInt() : 6;

     // Post initialization
     uint maxHeight = (useBox ? pdm.height()-innerMargin : pdm.height());
     uint currentPage( 1 );
     uint lastline = lastLine(); // nessecary to print selection only
     uint firstline( 0 );

     /*
        Now on for preparations...
        during preparations, variable names starting with a "_" means
        those variables are local to the enclosing block.
     */
     {
       if ( selectionOnly )
       {
         // set a line range from the first selected line to the last
         if ( selectStart.line < selectEnd.line )
         {
           firstline = selectStart.line;
           selStartCol = selectStart.col;
           lastline = selectEnd.line;
           selEndCol = selectEnd.col;
         }
         else // TODO check if this is possible!
         {
           kdDebug()<<"selectStart > selectEnd!"<<endl;
           firstline = selectEnd.line;
           selStartCol = selectEnd.col;
           lastline = selectStart.line;
           selEndCol = selectStart.col;
         }
         lineCount = firstline;
       }

       if ( printLineNumbers )
       {
         // figure out the horiizontal space required
         QString s( QString("%1 ").arg( numLines() ) );
         s.fill('5', -1); // some non-fixed fonts haven't equally wide numbers
                          // FIXME calculate which is actually the widest...
         lineNumberWidth = ((QFontMetrics)printFont.myFontMetrics).width( s );
         //lineNumberWidth = printFont.myFontMetrics.maxWidth() * s.length(); // BAD!
         // adjust available width and set horizontal start point for data
         maxWidth -= lineNumberWidth;
         xstart += lineNumberWidth;
       }

       if ( useHeader || useFooter )
       {
         // Set up a tag map
         // This retrieves all tags, ued or not, but
         // none of theese operations should be expensive,
         // and searcing each tag in the format strings is avoided.
         QDateTime dt = QDateTime::currentDateTime();
         QMap<QString,QString> tags;
         tags["u"] = getenv("USER");
         tags["d"] = KGlobal::locale()->formatDateTime(dt, true, false);
         tags["D"] =  KGlobal::locale()->formatDateTime(dt, false, false);
         tags["h"] =  KGlobal::locale()->formatTime(dt.time(), false);
         tags["y"] =  KGlobal::locale()->formatDate(dt.date(), true);
         tags["Y"] =  KGlobal::locale()->formatDate(dt.date(), false);
         tags["f"] =  url().fileName();
         tags["F"] =  url().prettyURL();
         if ( selectionOnly )
         {
           QString s( i18n("(Selection of) ") );
           tags["f"].prepend( s );
           tags["F"].prepend( s );
         }

         QRegExp reTags( "%([dDfFhuyY])" ); // TODO tjeck for "%%<TAG>"

         if (useHeader)
         {
           headerDrawBg = ( printer.option("app-kate-headerusebg") == "true" );
           headerHeight = QFontMetrics( headerFont ).height();
           if ( useBox || headerDrawBg )
             headerHeight += innerMargin * 2;
           else
             headerHeight += 1 + QFontMetrics( headerFont ).leading();

           QString headerTags = printer.option("app-kate-headerformat");
           int pos = reTags.search( headerTags );
           QString rep;
           while ( pos > -1 )
           {
             rep = tags[reTags.cap( 1 )];
             headerTags.replace( (uint)pos, 2, rep );
             pos += rep.length();
             pos = reTags.search( headerTags, pos );
           }
           headerTagList = QStringList::split('|', headerTags, true);

           if (!headerBgColor.isValid())
             headerBgColor = Qt::lightGray;
           if (!headerFgColor.isValid())
             headerFgColor = Qt::black;
         }

         if (useFooter)
         {
           footerDrawBg = ( printer.option("app-kate-footerusebg") == "true" );
           footerHeight = QFontMetrics( headerFont ).height();
           if ( useBox || footerDrawBg )
             footerHeight += 2*innerMargin;
           else
             footerHeight += 1; // line only

           QString footerTags = printer.option("app-kate-footerformat");
           int pos = reTags.search( footerTags );
           QString rep;
           while ( pos > 0 )
           {
             rep = tags[reTags.cap( 1 )];
             footerTags.replace( (uint)pos, 2, rep );
             pos += rep.length();
             pos = reTags.search( footerTags, pos );
           }

           footerTagList = QStringList::split('|', footerTags, true);
           if (!footerBgColor.isValid())
             footerBgColor = Qt::lightGray;
           if (!footerFgColor.isValid())
             footerFgColor = Qt::black;
           // adjust maxheight, so we can know when/where to print footer
           maxHeight -= footerHeight;
         }
       } // if ( useHeader || useFooter )

       if ( useBackground )
       {
         if ( ! useBox )
         {
           xstart += innerMargin;
           maxWidth -= innerMargin * 2;
         }
       }

       if ( useBox )
       {
         if (!boxColor.isValid())
           boxColor = Qt::black;
         if (boxWidth < 1) // shouldn't be pssible no more!
           boxWidth = 1;
         // set maxwidth to something sensible
         maxWidth -= ( ( boxWidth + innerMargin )  * 2 );
         xstart += boxWidth + innerMargin;
         // maxheight too..
         maxHeight -= boxWidth;
       }
       else
         boxWidth = 0;

       if ( useGuide )
       {
         // calculate the height required
         // the number of columns is a side effect, saved for drawing time
         // first width is needed
         int _w = pdmWidth - innerMargin * 2;
         if ( useBox )
           _w -= boxWidth * 2;
         else
         {
           if ( useBackground )
             _w -= ( innerMargin * 2 );
           _w -= 2; // 1 px line on each side
         }

         // base of height: margins top/bottom, above and below tetle sep line
         guideHeight = ( innerMargin * 4 ) + 1;
         // get a title and add the height required to draw it
         QString _title = i18n("Typographical Conventions for ") + m_highlight->name();
         guideHeight += paint.boundingRect( 0, 0, _w, 1000, Qt::AlignTop|Qt::AlignHCenter, _title ).height();
         // see how many columns we can fit in
         int _widest( 0 );
         QPtrListIterator<ItemData> it( m_highlight->getData()->itemDataList );
         ItemData *_d;
         /////KateFontMetrics _kfm = KateFontMetrics(QFont()); // eew
         int _items ( 0 );
         while ( ( _d = it.current()) != 0 )
         {
           _widest = QMAX( _widest, ((QFontMetrics)(
                                _d->bold ?
                                  _d->italic ?
                                    printFont.myFontMetricsBI :
                                    printFont.myFontMetricsBold :
                                  _d->italic ?
                                    printFont.myFontMetricsItalic :
                                    printFont.myFontMetrics
                                    ) ).width( _d->name ) );
           _items++;
           ++it;
         }
         guideCols = _w/( _widest + innerMargin );
         // add height for required number of lines needed given columns
         guideHeight += printFont.fontHeight * ( _items/guideCols );
         if ( _items%guideCols )
           guideHeight += printFont.fontHeight;
       }

       // now that we know the vertical amount of space needed,
       // it is possible to calculate the total number of pages
       // if needed, that is if any header/footer tag contains "%P".
       if ( headerTagList.grep("%P").count() || footerTagList.grep("%P").count() )
       {
         kdDebug(13020)<<"'%P' found! calculating number of pages..."<<endl;
         uint _pages = 0;
         uint _ph = maxHeight;
         if ( useHeader )
           _ph -= ( headerHeight + innerMargin );
         if ( useFooter )
           _ph -= innerMargin;
         int _lpp = _ph/printFont.fontHeight;
         kdDebug(13020)<<"... Printer Pixel Hunt: "<<
                "\n- printer heignt:    "<<pdm.height()<<
                "\n- max height:        "<<maxHeight<<
                "\n- header Height:     "<<headerHeight<<
                "\n- footer height:     "<<footerHeight<<
                "\n- inner margin:      "<<innerMargin<<
                "\n- contents height:   "<<_ph<<
                "\n- print font height: "<<printFont.fontHeight<<endl;
         kdDebug(13020)<<"...Lines pr page is "<<_lpp<<endl;
         uint _lt = 0, _c=0;
         // add space for guide if required
         if ( useGuide )
           _lt += (guideHeight + (printFont.fontHeight/2))/printFont.fontHeight;
         long _lw;
         for ( uint i = firstline; i < lastline; i++ )
         {
           _lw = textWidth( kateTextLine( i ), -1, PrintFont );
           while ( _lw >= 0 )
           {
             _c++;
             _lt++;
             if ( (int)_lt  == _lpp )
             {
               _pages++;
               _lt = 0;
             }
             _lw -= maxWidth;
             if ( ! _lw ) _lw--; // skip lines matching exactly!
           }
         }
         if ( _lt ) _pages++; // last page
//         kdDebug(13020)<<"... Number of pages to print: "<<_pages<<endl;
         // substitute both tag lists
//         kdDebug(13020)<<"... lines total: "<<_c<<endl;
         QRegExp re("%P");
         QStringList::Iterator it;
         for ( it=headerTagList.begin(); it!=headerTagList.end(); ++it )
           (*it).replace( re, QString( "%1" ).arg( _pages ) );
         for ( it=footerTagList.begin(); it!=footerTagList.end(); ++it )
           (*it).replace( re, QString( "%1" ).arg( _pages ) );
       }
     } // end prepare block

     /*
        On to draw something :-)
     */
uint _count = 0;
     while (  lineCount <= lastline  )
     {
       startCol = 0;
       endCol = 0;
       needWrap = true;

//       kdDebug(13020)<<"Starting real new line "<<lineCount<<endl;

       while (needWrap)
       {
         if ( y+printFont.fontHeight >= (uint)(maxHeight) )
         {
kdDebug(13020)<<"Starting new page, "<<_count<<" lines up to now."<<endl;
           printer.newPage();
           currentPage++;
           pageStarted = true;
           y=0;
         }

         if ( pageStarted )
         {

           if ( useHeader )
           {
             paint.setPen(headerFgColor);
             paint.setFont(headerFont);
             if ( headerDrawBg )
                paint.fillRect(0, 0, headerWidth, headerHeight, headerBgColor);
             if (headerTagList.count() == 3)
             {
               int valign = ( (useBox||headerDrawBg||useBackground) ?
                              Qt::AlignVCenter : Qt::AlignTop );
               int align = valign|Qt::AlignLeft;
               int marg = ( useBox || headerDrawBg ) ? innerMargin : 0;
               if ( useBox ) marg += boxWidth;
               QString s;
               for (int i=0; i<3; i++)
               {
                 s = headerTagList[i];
                 if (s.find("%p") != -1) s.replace(QRegExp("%p"), QString::number(currentPage));
                 paint.drawText(marg, 0, headerWidth-(marg*2), headerHeight, align, s);
                 align = valign|(i == 0 ? Qt::AlignHCenter : Qt::AlignRight);
               }
             }
             if ( ! ( headerDrawBg || useBox || useBackground ) ) // draw a 1 px (!?) line to separate header from contents
             {
               paint.drawLine( 0, headerHeight-1, headerWidth, headerHeight-1 );
               //y += 1; now included in headerHeight
             }
             y += headerHeight + innerMargin;
           }

           if ( useFooter )
           {
             if ( ! ( footerDrawBg || useBox || useBackground ) ) // draw a 1 px (!?) line to separate footer from contents
               paint.drawLine( 0, maxHeight + innerMargin - 1, headerWidth, maxHeight + innerMargin - 1 );
             if ( footerDrawBg )
                paint.fillRect(0, maxHeight+innerMargin+boxWidth, headerWidth, footerHeight, footerBgColor);
             if (footerTagList.count() == 3)
             {
               int align = Qt::AlignVCenter|Qt::AlignLeft;
               int marg = ( useBox || footerDrawBg ) ? innerMargin : 0;
               if ( useBox ) marg += boxWidth;
               QString s;
               for (int i=0; i<3; i++)
               {
                 s = footerTagList[i];
                 if (s.find("%p") != -1) s.replace(QRegExp("%p"), QString::number(currentPage));
                 paint.drawText(marg, maxHeight+innerMargin, headerWidth-(marg*2), footerHeight, align, s);
                 align = Qt::AlignVCenter|(i == 0 ? Qt::AlignHCenter : Qt::AlignRight);
               }
             }
           } // done footer

           if ( useBackground )
           {
             // If we have a box, or the header/footer has backgrounds, we want to paint
             // to the border of those. Otherwise just the contents area.
             int _y = y, _h = maxHeight - y;
             if ( useBox )
             {
               _y -= innerMargin;
               _h += 2 * innerMargin;
             }
             else
             {
               if ( headerDrawBg )
               {
                 _y -= innerMargin;
                 _h += innerMargin;
               }
               if ( footerDrawBg )
               {
                 _h += innerMargin;
               }
             }
             paint.fillRect( 0, _y, pdmWidth, _h, colors[0] );
           }

           if ( useBox )
           {
             paint.setPen(QPen(boxColor, boxWidth));
             paint.drawRect(0, 0, pdmWidth, pdm.height());
             if (useHeader)
               paint.drawLine(0, headerHeight, headerWidth, headerHeight);
             else
               y += innerMargin;

             if ( useFooter ) // drawline is not trustable, grr.
               paint.fillRect( 0, maxHeight+innerMargin, headerWidth, boxWidth, boxColor );
           }

           if ( useGuide && currentPage == 1 )
           {  // FIXME - this may span more pages...
             // draw a box unless we have boxes, in which case we end with a box line
             paint.setPen( colors[1] );
             int _marg = 0; // this could be available globally!??
             if ( useBox )
             {
               _marg += (2*boxWidth) + (2*innerMargin);
               paint.fillRect( 0, y+guideHeight-innerMargin-boxWidth, headerWidth, boxWidth, boxColor );
             }
             else
             {
               if ( useBackground )
                 _marg += 2*innerMargin;
               paint.drawRect( _marg, y, pdmWidth-(2*_marg), guideHeight );
               _marg += 1;
               y += 1 + innerMargin;
             }
             // draw a title string
             paint.setFont( printFont.myFontBold );
             QRect _r;
             paint.drawText( _marg, y, pdmWidth-(2*_marg), maxHeight - y,
                Qt::AlignTop|Qt::AlignHCenter,
                i18n("Typographical Conventions for ") + m_highlight->name(), -1, &_r );
             int _w = pdmWidth - (_marg*2) - (innerMargin*2);
             int _x = _marg + innerMargin;
             y += _r.height() + innerMargin;
             paint.drawLine( _x, y, _x + _w, y );
             y += 1 + innerMargin;
             // draw attrib names using their styles
             QPtrListIterator<ItemData> _it( m_highlight->getData()->itemDataList );
             ItemData *_d;
             int _cw = _w/guideCols;
             int _i(0);
//      kdDebug()<<"guide cols: "<<guideCols<<endl;
             while ( ( _d = _it.current() ) != 0 )
             {
//      kdDebug()<<"style: "<<_d->name<<", color:"<<attribute(_i)->col.name()<<endl;
               paint.setPen( attribute(_i)->col );
               paint.setFont( attribute(_i)->font( printFont ) );
//      kdDebug()<<"x: "<<( _x + ((_i%guideCols)*_cw))<<", y: "<<y<<", w: "<<_cw<<endl;
               paint.drawText(( _x + ((_i%guideCols)*_cw)), y, _cw, printFont.fontHeight,
                        Qt::AlignVCenter|Qt::AlignLeft, _d->name, -1, &_r );
//      kdDebug()<<"painted in rect: "<<_r.x()<<", "<<_r.y()<<", "<<_r.width()<<", "<<_r.height()<<endl;
               _i++;
               if ( _i && ! ( _i%guideCols ) ) y += printFont.fontHeight;
               ++_it;
             }
             if ( _i%guideCols ) y += printFont.fontHeight;// last row not full
             y += ( useBox ? boxWidth : 1 ) + (innerMargin*2);
//        kdDebug(13020)<<"DONE HL GUIDE! Starting to print from line "<<lineCount<<endl;
//        kdDebug(13020)<<"max width for lines: "<<maxWidth<<endl;
           }

           pageStarted = false;
         } // pageStarted; move on to contents:)

         if ( printLineNumbers && ! startCol ) // don't repeat!
         {
           paint.setFont( printFont.font( false, false ) );
           paint.setPen( colors[1] ); // using "selected" color for now...
           paint.drawText( ( useBox || useBackground ) ? innerMargin : 0, y,
                        lineNumberWidth, printFont.fontHeight,
                        Qt::AlignRight, QString("%1 ").arg( lineCount + 1 ) );
         }
//        kdDebug(13020)<<"Calling textWidth( startCol="<<startCol<<", maxWidth="<<maxWidth<<", needWrap="<<needWrap<<")"<<endl;
         endCol = textWidth (buffer->line(lineCount), startCol, maxWidth, 0, PrintFont, &needWrap);
//         kdDebug(13020)<<"REAL WIDTH: " << pdmWidth << " WIDTH: " << maxWidth <<" line: "<<lineCount<<" start: "<<startCol<<" end: "<<endCol<<" line length: "<< buffer->line(lineCount)->length()<< "; need Wrap: " << needWrap <<" !?"<<endl;

         if ( endCol < startCol )
         {
           kdDebug(13020)<<"--- Skipping garbage, line: "<<lineCount<<" start: "<<startCol<<" end: "<<endCol<<" real EndCol; "<< buffer->line(lineCount)->length()<< " !?"<<endl;
           lineCount++;
           continue; // strange case...
                     // Happens if the line fits exactly.
                     // When it happens, a line of garbage would be printed.
                     // FIXME Most likely this is an error in textWidth(),
                     // failing to correctly set needWrap to false in this case?
         }
         // if we print only selection:
         // print only selected range of chars.
         bool skip = false;
         if ( selectionOnly )
         {
           bool inBlockSelection = ( blockSelect && lineCount >= firstline && lineCount <= lastline );
           if ( lineCount == firstline || inBlockSelection )
           {
             if ( startCol < selStartCol )
               startCol = selStartCol;
           }
           if ( lineCount == lastline  || inBlockSelection )
           {
             if ( endCol > selEndCol )
             {
               endCol = selEndCol;
               skip = true;
             }
           }
         }
         // HA! this is where we print [part of] a line ;]]
         // FIXME HACK
         LineRange range;
         range.line = lineCount;
         range.startCol = startCol;
         range.endCol = endCol;
         range.wrap = needWrap;
         paintTextLine ( paint, range, xstart, y, 0, maxWidth, -1, false, 0, false, false, PrintFont, false, true );
         if ( skip )
         {
           needWrap = false;
           startCol = 0;
         }
         else
         {
//kdDebug()<<"Line: "<<lineCount<<", Start Col: "<<startCol<<", End Col: "<<endCol<<", Need Wrap: "<<needWrap<<", maxWidth: "<<maxWidth<<endl;
           startCol = endCol;
         }

         y += printFont.fontHeight;
_count++;
       } // done while ( needWrap )

       lineCount++;
     } // done lineCount <= lastline
//kdDebug(13020)<<"lines printed in total: "<<_count<<endl;
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

  QString serviceType = m_extension->urlArgs().serviceType.simplifyWhiteSpace();
  kdDebug(13020) << "servicetype: " << serviceType << endl;
  int pos = serviceType.find(';');
  if (pos != -1)
    myEncoding = serviceType.mid(pos+1);
  kdDebug(13020) << "myEncoding: " << myEncoding << endl;

  bool success = buffer->openFile (m_file, KGlobal::charsets()->codecForName(myEncoding));

  setMTime();

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
      QString line = textLine(i);
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

  updateLines();
  updateViews();

  emit fileNameChanged();

  return success;
}

bool KateDocument::saveFile()
{
  QString eol ("\n");

  if (eolMode == KateDocument::eolDos) eol = QString("\r\n");
  else if (eolMode == KateDocument::eolMacintosh) eol = QString ("\r");

  bool success = buffer->saveFile (m_file, KGlobal::charsets()->codecForName(myEncoding), eol);

  fileInfo->setFile (m_file);
  setMTime();

  if (!hlSetByUser)
  {
  int hl = hlManager->wildcardFind( m_file );

  if (hl == -1)
  {
    // fill the detection buffer with the contents of the text
    // anders: fixed to work. I thought I allready did :(
    const int HOWMANY = 1024;
    QByteArray buf(HOWMANY);
    int bufpos = 0, len;
    for (uint i=0; i < buffer->count(); i++)
    {
      QString line = textLine( i );
      len = line.length() + 1;
      if (bufpos + len > HOWMANY) len = HOWMANY - bufpos;
      memcpy(&buf[bufpos], (line + "\n").latin1(), len);
      bufpos += len;
      if (bufpos >= HOWMANY) break;
    }

    hl = hlManager->mimeFind( buf, m_file );
  }

    internalSetHlMode(hl);
  }

  emit fileNameChanged ();

  setDocName  (url().fileName());

  return success;
}

void KateDocument::setReadWrite( bool rw )
{
  if (rw == readOnly)
  {
    readOnly = !rw;
    KParts::ReadWritePart::setReadWrite (rw);
    for( KateView* view = m_views.first(); view != 0L; view = m_views.next() ) {
      view->slotUpdate();
    }
  }
}

bool KateDocument::isReadWrite() const
{
  return !readOnly;
}

void KateDocument::setModified(bool m) {

  if (m != modified) {
    modified = m;
    KParts::ReadWritePart::setModified (m);
    for( KateView* view = m_views.first(); view != 0L; view = m_views.next() ) {
      view->slotUpdate();
    }
    emit modifiedChanged ();
  }
  if ( m == false && ! undoItems.isEmpty() )
  {
    lastUndoGroupWhenSaved = undoItems.last();
  };
  if ( m == false ) docWasSavedWhenUndoWasEmpty = undoItems.isEmpty();

}

bool KateDocument::isModified() const {
  return modified;
}

//
// Kate specific stuff ;)
//

const FontStruct& KateDocument::getFontStruct( WhichFont wf )
{
  return (wf == ViewFont) ? viewFont : printFont;
}

void KateDocument::setFont (WhichFont wf, QFont font)
{
  FontStruct& fs = (wf == ViewFont) ? viewFont : printFont;

  fs.setFont(font);
  fs.updateFontData(tabChars);

  if (wf == ViewFont)
  {
    updateFontData();
    updateViews();
  }
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

void KateDocument::makeAttribs()
{
  hlManager->makeAttribs(this, m_highlight);
  updateFontData();
  updateLines();
}

void KateDocument::updateFontData() {
  tagAll();
}

void KateDocument::internalHlChanged() { //slot
  makeAttribs();
}

void KateDocument::addView(KTextEditor::View *view) {
  m_views.append( (KateView *) view  );
  m_textEditViews.append( view );
  m_activeView = (KateView *) view;
}

void KateDocument::removeView(KTextEditor::View *view) {
  if (m_activeView == view)
    m_activeView = 0L;

  m_views.removeRef( (KateView *) view );
  m_textEditViews.removeRef( view  );
}

void KateDocument::addCursor(KTextEditor::Cursor *cursor) {
  myCursors.append( cursor );
}

void KateDocument::removeCursor(KTextEditor::Cursor *cursor) {
  myCursors.removeRef( cursor  );
}

bool KateDocument::ownedView(KateView *view) {
  // do we own the given view?
  return (m_views.containsRef(view) > 0);
}

bool KateDocument::isLastView(int numViews) {
  return ((int) m_views.count() == numViews);
}

uint KateDocument::textWidth(const TextLine::Ptr &textLine,
			     int cursorX, WhichFont wf)
{
  if (!textLine)
    return 0;

  if (cursorX < 0)
    cursorX = textLine->length();

  const FontStruct & fs = getFontStruct(wf);

  int x = 0;
  for (int z = 0; z < cursorX; z++) {
    Attribute *a = attribute(textLine->attribute(z));
    int width = a->width(fs, textLine->getChar(z));
    x += width;

    if (textLine->getChar(z) == QChar('\t'))
      x -= x % width;
  }

  return x;
}

uint KateDocument::textWidth(const TextLine::Ptr &textLine, uint startcol, uint maxwidth, uint wrapsymwidth, WhichFont wf, bool *needWrap, int *endX)
{
  const FontStruct & fs = getFontStruct(wf);
  uint x = 0;
  uint endcol = 0;
  uint endcolwithsym = 0;
  int endX2 = 0;
  int endXWithSym = 0;
  int lastWhiteSpace = -1;
  int lastWhiteSpaceX = -1;

  *needWrap = false;

  uint z = startcol;
  for (; z < textLine->length(); z++)
  {
    Attribute *a = attribute(textLine->attribute(z));
    int width = a->width(fs, textLine->getChar(z));
    x += width;

    if (textLine->getChar(z).isSpace ())
    {
      lastWhiteSpace = z+1;
      lastWhiteSpaceX = x;
    }

    // How should tabs be treated when they word-wrap on a print-out?
    // if startcol != 0, this messes up (then again, word wrapping messes up anyway)
    if (textLine->getChar(z) == QChar('\t'))
      x -= x % width;

    if (x <= maxwidth)
    {
      if (lastWhiteSpace > -1)
      {
        endcol = lastWhiteSpace;
        endX2 = lastWhiteSpaceX;
      }
      else
      {
        endcol = z+1;
        endX2 = x;
      }

      if (x <= maxwidth-wrapsymwidth )
      {
        if (lastWhiteSpace > -1)
        {
          endcolwithsym = lastWhiteSpace;
          endXWithSym = lastWhiteSpaceX;
        }
        else
        {
          endcolwithsym = z+1;
          endXWithSym = x;
        }
      }
    }

    if (x >= maxwidth)
    {
      *needWrap = true;
      break;
    }
  }

  if (*needWrap)
  {
    if (endX)
      *endX = endX2;
    return endcolwithsym;
  }
  else
  {
    if (endX)
      *endX = x;

    return z+1;
  }
}

uint KateDocument::textWidth(KateTextCursor &cursor)
{
  if (cursor.col < 0)
     cursor.col = 0;
  if (cursor.line < 0)
     cursor.line = 0;
  if (cursor.line >= (int)numLines())
     cursor.line = lastLine();
  return textWidth(buffer->line(cursor.line),cursor.col);
}

uint KateDocument::textWidth( KateTextCursor &cursor, int xPos,WhichFont wf, uint startCol)
{
  bool wrapCursor = configFlags() & KateDocument::cfWrapCursor;
  int len;
  int x, oldX;

  const FontStruct & fs = getFontStruct(wf);

  if (cursor.line < 0) cursor.line = 0;
  if (cursor.line > (int)lastLine()) cursor.line = lastLine();
  TextLine::Ptr textLine = buffer->line(cursor.line);
  len = textLine->length();

  x = oldX = 0;
  int z = startCol;
  while (x < xPos && (!wrapCursor || z < len)) {
    oldX = x;

    Attribute *a = attribute(textLine->attribute(z));


    int width = 0;

    if (z < len)
      width = a->width(fs, textLine->getChar(z));
    else
      width = a->width(fs, QChar (' '));

    x += width;

    if (textLine->getChar(z) == QChar('\t'))
      x -= x % width;

    z++;
  }
  if (xPos - oldX < x - xPos && z > 0) {
    z--;
    x = oldX;
  }
  cursor.col = z;
  return x;
}

uint KateDocument::textPos(uint line, int xPos, WhichFont wf, uint startCol)
{
  return textPos(buffer->line(line), xPos, wf, startCol);
}

uint KateDocument::textPos(const TextLine::Ptr &textLine, int xPos,WhichFont wf, uint startCol) {
  const FontStruct & fs = getFontStruct(wf);

  int x, oldX;
  x = oldX = 0;

  uint z = startCol;
  uint len= textLine->length();
  while ( (x < xPos)  && (z < len)) {
    oldX = x;

    Attribute *a = attribute(textLine->attribute(z));
    x += a->width(fs, textLine->getChar(z));

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

uint KateDocument::currentColumn( const KateTextCursor& cursor )
{
  TextLine::Ptr t = buffer->line(cursor.line);

  if (t)
    return t->cursorX(cursor.col,tabChars);
  else
    return 0;
}

bool KateDocument::insertChars ( int line, int col, const QString &chars, KateView *view )
{
  QString buf;

  int savedCol = col;
  int savedLine = line;
  QString savedChars(chars);

  TextLine::Ptr textLine = buffer->line(line);

  uint pos = 0;
  int l;
  bool onlySpaces = true;
  for( uint z = 0; z < chars.length(); z++ ) {
    QChar ch = chars[z];
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

  if (_configFlags & KateDocument::cfDelOnInput && hasSelection() )
  {
    removeSelectedText();
    line = view->m_viewInternal->cursorCache.line;
    col = view->m_viewInternal->cursorCache.col;
  }

  if (_configFlags & KateDocument::cfOvr)
  {
    removeText (line, col, line, QMIN( col+buf.length(), textLine->length() ) );
  }

  insertText (line, col, buf);
  col += pos;

  // editEnd will set the cursor from this cache right ;))
  view->m_viewInternal->cursorCache.setPos(line, col);
  view->m_viewInternal->cursorCacheChanged = true;

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

void KateDocument::newLine( KateTextCursor& c, KateViewInternal *v )
{
  editStart();

  if( configFlags() & cfDelOnInput && hasSelection() )
    removeSelectedText();

  // temporary hack to get the cursor pos right !!!!!!!!!
  c = v->cursorCache;

  if (c.line > (int)lastLine())
   c.line = lastLine();

  if (c.col > (int)kateTextLine(c.line)->length())
    c.col = kateTextLine(c.line)->length();

  if (!(_configFlags & KateDocument::cfAutoIndent)) {
    insertText( c.line, c.col, "\n" );
    c.line++;
    c.col = 0;
  } else {
    TextLine::Ptr textLine = buffer->line(c.line);
    int pos = textLine->firstChar();
    if (c.col < pos) c.col = pos; // place cursor on first char if before


    int y = c.line;
    while ((y > 0) && (pos < 0)) { // search a not empty text line
      textLine = buffer->line(--y);
      pos = textLine->firstChar();
    }
    insertText (c.line, c.col, "\n");
    c.line++;
    c.col = 0;
    if (pos > 0) {
      pos = textLine->cursorX(pos, tabChars);
      QString s = tabString(pos, (_configFlags & KateDocument::cfSpaceIndent) ? 0xffffff : tabChars);
      insertText (c.line, c.col, s);
      pos = s.length();
      c.col = pos;
    }
  }

  editEnd();
}

void KateDocument::transpose( const KateTextCursor& cursor)
{
  TextLine::Ptr textLine = buffer->line(cursor.line);
  uint line = cursor.line;
  uint col = cursor.col;

  if (!textLine)
    return;

  QString s;
  if (col != 0)  //there is a special case for this one
    --col;

  //clever swap code if first character on the line swap right&left
  //otherwise left & right
  s.append (textLine->getChar(col+1));
  s.append (textLine->getChar(col));
  //do the swap

  // do it right, never ever manipulate a textline
  editStart ();
  editRemoveText (line, col, 2);
  editInsertText (line, col, s);

  editEnd ();
}

void KateDocument::backspace( const KateTextCursor& c )
{
  if( configFlags() & cfDelOnInput && hasSelection() ) {
    removeSelectedText();
    return;
  }

  uint col = QMAX( c.col, 0 );
  uint line = QMAX( c.line, 0 );

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

      TextLine::Ptr textLine = buffer->line(line);
      int pos = textLine->firstChar();
      if (pos < 0 || pos >= (int)col)
      {
        // only spaces on left side of cursor
        // search a line with less spaces
        uint y = line;
        while (y > 0)
        {
          textLine = buffer->line(--y);
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
      if (myWordWrap && buffer->line(line-1)->endingWith(QString::fromLatin1(" ")))
      {
        // gg: in hard wordwrap mode, backspace must also eat the trailing space
        removeText (line-1, buffer->line(line-1)->length()-1, line, 0);
      }
      else
        removeText (line-1, buffer->line(line-1)->length(), line, 0);
    }
  }
  emit backspacePressed();
}

void KateDocument::del( const KateTextCursor& c )
{
  if ( configFlags() & cfDelOnInput && hasSelection() ) {
    removeSelectedText();
    return;
  }

  if( c.col < (int) buffer->line(c.line)->length())
  {
    removeText(c.line, c.col, c.line, c.col+1);
  }
  else
  {
    removeText(c.line, c.col, c.line+1, 0);
  }
}

void KateDocument::cut()
{
  if (!hasSelection())
    return;

  copy();
  removeSelectedText();
}

void KateDocument::copy()
{
  if (!hasSelection())
    return;

  QApplication::clipboard()->setText(selection ());
}

void KateDocument::paste( const KateTextCursor& cursor, KateView* view )
{
  QString s = QApplication::clipboard()->text();

  if (s.isEmpty())
    return;

  editStart ();

  uint line = cursor.line;
  uint col = cursor.col;

  if (_configFlags & KateDocument::cfDelOnInput && hasSelection() )
  {
    removeSelectedText();
    line = view->m_viewInternal->cursorCache.line;
    col = view->m_viewInternal->cursorCache.col;
  }

  insertText( line, col, s, blockSelect );

  // anders: we want to be able to move the cursor to the
  // position at the end of the pasted text,
  // so we calculate that and applies it to c.cursor
  // This may not work, when wordwrap gets fixed :(
  TextLine *ln = buffer->line( line );
  int l = s.length();
  while ( l > 0 ) {
    if ( col < ln->length() ) {
      col++;
    } else {
      line++;
      ln = buffer->line( line );
      col = 0;
    }
    l--;
  }

  // editEnd will set the cursor from this cache right ;))
  // Totally breaking the whole idea of the doc view model here...
  view->m_viewInternal->cursorCache.setPos(line, col);
  view->m_viewInternal->cursorCacheChanged = true;

  editEnd();
}

void KateDocument::selectTo( const KateTextCursor& from, const KateTextCursor& to )
{
  if (!hasSelection()) {
    selectAnchor.setPos(from);
  }

  setSelection(selectAnchor, to);
}

void KateDocument::selectWord( const KateTextCursor& cursor ) {
  int start, end, len;

  TextLine::Ptr textLine = buffer->line(cursor.line);
  len = textLine->length();
  start = end = cursor.col;
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;
  if (end <= start) return;

  if (!(_configFlags & KateDocument::cfKeepSelection))
    clearSelection ();
  setSelection (cursor.line, start, cursor.line, end);
}

void KateDocument::selectLine( const KateTextCursor& cursor ) {
  if (!(_configFlags & KateDocument::cfKeepSelection))
    clearSelection ();
  setSelection (cursor.line, 0, cursor.line/*+1, 0*/, buffer->line(cursor.line)->length() );
}

void KateDocument::selectLength( const KateTextCursor& cursor, int length ) {
  int start, end;

  TextLine::Ptr textLine = buffer->line(cursor.line);
  start = cursor.col;
  end = start + length;
  if (end <= start) return;

  if (!(_configFlags & KateDocument::cfKeepSelection))
    clearSelection ();
  setSelection (cursor.line, start, cursor.line, end);
}

void KateDocument::doIndent( uint line, int change)
{
  editStart ();

  if (!hasSelection()) {
    // single line
    optimizeLeadingSpace(line, _configFlags, change);
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
        textLine = buffer->line(line);
        if ((textLine->length() > 0) && (lineSelected(line) || lineHasSelected(line))) {
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
void KateDocument::optimizeLeadingSpace(uint line, int flags, int change) {
  int len;
  int chars, space, okLen;
  QChar ch;
  int extra;
  QString s;
  KateTextCursor cursor;

  TextLine::Ptr textLine = buffer->line(line);
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
  TextLine* textline = buffer->line(line);

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
  TextLine* textline = buffer->line(line);

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

  editStart();

  // Try to remove the long comment mark first
  bool removed = (removeStringFromBegining(line, longCommentMark)
                  || removeStringFromBegining(line, shortCommentMark));

  editEnd();

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

  editStart();

  // Add the start comment mark
  insertText (line, 0, startCommentMark);

  // Go to the end of the line
  TextLine* textline = buffer->line(line);
  int col = textline->length();

  // Add the stop comment mark
  insertText (line, col, stopCommentMark);

  editEnd();
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

  editStart();

  // Try to remove the long start comment mark first
  bool removedStart = (removeStringFromBegining(line, longStartCommentMark)
                       || removeStringFromBegining(line, shortStartCommentMark));

  // Try to remove the long stop comment mark first
  bool removedStop = (removeStringFromEnd(line, longStopCommentMark)
                      || removeStringFromEnd(line, shortStopCommentMark));

  editEnd();

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
    ec = buffer->line (el)->length();
  }

  editStart();

  insertText (el, ec, endComment);
  insertText (sl, sc, startComment);

  editEnd ();

  // Set the new selection
  ec += endComment.length() + ( (el == sl) ? startComment.length() : 0 );
  setSelection(sl, sc, el, ec);
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

  editStart();

  // For each line of the selection
  for (int z = el; z >= sl; z--) {
    insertText (z, 0, commentLineMark);
  }

  editEnd ();

  // Set the new selection
  selectEnd.col += ( (el == selectEnd.line) ? commentLineMark.length() : 0 );
  setSelection(selectStart.line, 0,
	       selectEnd.line, selectEnd.col);
}

/*
  Find the position (line and col) of the next char
  that is not a space. Return true if found.
*/
bool KateDocument::nextNonSpaceCharPos(int &line, int &col)
{
  for(; line < (int)buffer->count(); line++) {
    TextLine::Ptr textLine = buffer->line(line);
    col = textLine->nextNonSpaceChar(col);
    if(col != -1)
      return true; // Next non-space char found
    col = 0;
  }
  // No non-space char found
  line = -1;
  col = -1;
  return false;
}

/*
  Find the position (line and col) of the previous char
  that is not a space. Return true if found.
*/
bool KateDocument::previousNonSpaceCharPos(int &line, int &col)
{
  for(; line >= 0; line--) {
    TextLine::Ptr textLine = buffer->line(line);
    col = textLine->previousNonSpaceChar(col);
    if(col != -1)
      return true; // Previous non-space char found
    col = 0;
  }
  // No non-space char found
  line = -1;
  col = -1;
  return false;
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

  // The selection ends on the char before selectEnd
  if (ec != 0) {
    ec--;
  } else {
    if (el > 0) {
      el--;
      ec = buffer->line(el)->length() - 1;
    }
  }

  int startCommentLen = startComment.length();
  int endCommentLen = endComment.length();

  // had this been perl or sed: s/^\s*$startComment(.+?)$endComment\s*/$1/

  bool remove = nextNonSpaceCharPos(sl, sc)
      && buffer->line(sl)->stringAtPos(sc, startComment)
      && previousNonSpaceCharPos(el, ec)
      && ( (ec - endCommentLen + 1) >= 0 )
      && buffer->line(el)->stringAtPos(ec - endCommentLen + 1, endComment);

  if (remove) {
    editStart();

    removeText (el, ec - endCommentLen + 1, el, ec + 1);
    removeText (sl, sc, sl, sc + startCommentLen);

    editEnd ();

    // Set the new selection
    ec -= endCommentLen + ( (el == sl) ? startCommentLen : 0 );
    setSelection(sl, sc, el, ec + 1);
  }

  return remove;
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

  // Find out how many char will be removed from the last line
  int removeLength = 0;
  if (buffer->line(el)->startingWith(longCommentMark))
    removeLength = longCommentMark.length();
  else if (buffer->line(el)->startingWith(shortCommentMark))
    removeLength = shortCommentMark.length();

  bool removed = false;

  editStart();

  // For each line of the selection
  for (int z = el; z >= sl; z--)
  {
    // Try to remove the long comment mark first
    removed = (removeStringFromBegining(z, longCommentMark)
                 || removeStringFromBegining(z, shortCommentMark)
                 || removed);
  }

  editEnd();

  if(removed) {
    // Set the new selection
    selectEnd.col -= ( (el == selectEnd.line) ? removeLength : 0 );
    setSelection(selectStart.line, selectStart.col,
	       selectEnd.line, selectEnd.col);
  }

  return removed;
}

/*
  Comment or uncomment the selection or the current
  line if there is no selection.
*/
void KateDocument::doComment( uint line, int change)
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
        addStartLineCommentToSingleLine(line);
      else if ( hasStartStopCommentMark )
        addStartStopCommentToSingleLine(line);
    }
    else
    {
      // anders: prefer single line comment to avoid nesting probs
      // If the selection starts after first char in the first line
      // or ends before the last char of the last line, we may use
      // multiline comment markers.
      // TODO We should try to detect nesting.
      //    - if selection ends at col 0, most likely she wanted that
      // line ignored
      if ( hasStartStopCommentMark &&
           ( !hasStartLineCommentMark || (
             ( selectStart.col > buffer->line( selectStart.line )->firstChar() ) ||
               ( selectEnd.col < ((int)buffer->line( selectEnd.line )->length()) )
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
                  && removeStartLineCommentFromSingleLine(line) )
        || ( hasStartStopCommentMark
             && removeStartStopCommentFromSingleLine(line) );
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

QString KateDocument::getWord( const KateTextCursor& cursor ) {
  int start, end, len;

  TextLine::Ptr textLine = buffer->line(cursor.line);
  len = textLine->length();
  start = end = cursor.col;
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;
  len = end - start;
  return QString(&textLine->text()[start], len);
}

void KateDocument::tagLines(int start, int end)
{
  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->m_viewInternal->tagLines(start, end, true);
}

void KateDocument::tagLines(KateTextCursor start, KateTextCursor end)
{
  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->m_viewInternal->tagLines(start, end, true);
}

void KateDocument::tagSelection()
{
  if (hasSelection()) {
    if ((oldSelectStart.line == -1 || (blockSelectionMode() && (oldSelectStart.col != selectStart.col || oldSelectEnd.col != selectEnd.col)))) {
      // We have to tag the whole lot if
      // 1) we have a selection, and:
      //  a) it's new; or
      //  b) we're in block selection mode and the columns have changed
      tagLines(selectStart, selectEnd);

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

void KateDocument::repaintViews(bool paintOnlyDirty)
{
  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->m_viewInternal->paintText(0,0,m_views.at(z)->m_viewInternal->width(),m_views.at(z)->m_viewInternal->height(), paintOnlyDirty);
}

void KateDocument::tagAll()
{
  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->m_viewInternal->tagAll();
    m_views.at(z)->m_viewInternal->updateView (true);
  }
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
  updateViews();
}

void KateDocument::updateViews()
{
  if (noViewUpdates)
    return;

  for (KateView * view = m_views.first(); view != 0L; view = m_views.next() )
  {
    view->m_viewInternal->updateView();
  }
}

QColor &KateDocument::backCol(int x, int y)
{
  return (lineColSelected(x,y)) ? colors[1] : colors[0];
}

QColor &KateDocument::cursorCol(int x, int y)
{
  TextLine::Ptr textLine = buffer->line(y);
  Attribute *a = attribute(textLine->attribute(x));

  if (lineColSelected (y, x))
    return a->selCol;
  else
    return a->col;
}

bool KateDocument::lineColSelected (int line, int col)
{
  if ( (!blockSelect) && (col < 0) )
    col = 0;

  KateTextCursor cursor(line, col);

  return (cursor >= selectStart) && (cursor < selectEnd);
}

bool KateDocument::lineSelected (int line)
{
  return (!blockSelect)
    && (selectStart <= KateTextCursor(line, 0))
    && (line < selectEnd.line);
}

bool KateDocument::lineEndSelected (int line, int endCol)
{
  return (!blockSelect)
    && (line > selectStart.line || (line == selectStart.line && (selectStart.col < endCol || endCol == -1)))
    && (line < selectEnd.line || (line == selectEnd.line && (endCol <= selectEnd.col && endCol != -1)));
}

bool KateDocument::lineHasSelected (int line)
{
  return (selectStart < selectEnd)
    && (line >= selectStart.line)
    && (line <= selectEnd.line);
}

bool KateDocument::lineIsSelection (int line)
{
  return (line == selectStart.line && line == selectEnd.line);
}

bool KateDocument::selectBounds(uint line, uint &start, uint &end, uint lineLength)
{
  bool hasSel = false;

  if (hasSelection() && !blockSelect)
  {
    if (lineIsSelection(line))
    {
      start = selectStart.col;
      end = selectEnd.col;
      hasSel = true;
    }
    else if ((int)line == selectStart.line)
    {
      start = selectStart.col;
      end = lineLength;
      hasSel = true;
    }
    else if ((int)line == selectEnd.line)
    {
      start = 0;
      end = selectEnd.col;
      hasSel = true;
    }
  }
  else if (lineHasSelected(line))
  {
    start = selectStart.col;
    end = selectEnd.col;
    hasSel = true;
  }

  if (start > end) {
    int temp = end;
    end = start;
    start = temp;
  }

  return hasSel;
}

bool KateDocument::paintTextLine(QPainter &paint, const LineRange& range,
				 int xPos2, int y, int xStart, int xEnd,
				 int showCursor, bool replaceCursor,
				 int cursorXPos2, bool showSelections,
				 bool showTabs, WhichFont wf,
				 bool currentLine, bool printerfriendly,
				 const BracketMark& bm, int startXCol )
{
  uint line = range.line;
  int startcol = range.startCol;
  int endcol = range.wrap ? range.endCol : -1;

  // font data
  const FontStruct & fs = getFontStruct(wf);

  // text attribs font/style data
  Attribute *at = myAttribs.data();
  uint atLen = myAttribs.size();

  // textline
  TextLine::Ptr textLine = buffer->line(line);

  if (!textLine)
    return false;

  // length, chars + raw attribs
  uint len = textLine->length();
  uint oldLen = len;
  const QChar *s;
  const uchar *a;

   // selection startcol/endcol calc
  bool hasSel = false;
  uint startSel = 0;
  uint endSel = 0;

  // was the selection background allready completly painted ?
  bool selectionPainted = false;

  // should the cursor be painted (if it is in the current xstart - xend range)
  bool cursorVisible = false;
  int cursorXPos = 0;
  int cursorMaxWidth = 0;
  QColor *cursorColor = 0;

  if (!printerfriendly && showSelections && lineSelected (line))
  {
    paint.fillRect(xPos2, y, xEnd - xStart, fs.fontHeight, colors[1]);
    selectionPainted = true;
    hasSel = true;
    startSel = 0;
    endSel = len + 1;
  }
  else if (!printerfriendly && currentLine)
    paint.fillRect(xPos2, y, xEnd - xStart, fs.fontHeight, colors[2]);
  else if (!printerfriendly)
    paint.fillRect(xPos2, y, xEnd - xStart, fs.fontHeight, colors[0]);

  if( !printerfriendly && bm.valid && (bm.startLine == line) && ((int)bm.startCol >= startcol) && ((endcol == -1) || ((int)bm.startCol < endcol)) )
    paint.fillRect( bm.startX - startXCol, y, bm.startW, fs.fontHeight, colors[3] );
  if( !printerfriendly && bm.valid && (bm.endLine == line) && ((int)bm.endCol >= startcol) && ((endcol == -1) || ((int)bm.endCol < endcol)) )
    paint.fillRect( bm.endX - startXCol, y, bm.endW, fs.fontHeight, colors[3] );

  if (startcol > (int)len)
    startcol = len;

  if (startcol < 0)
    startcol = 0;

  if (endcol < 0)
    len = len - startcol;
  else
    len = endcol - startcol;

  // text + attrib data from line
  s = textLine->text ();
  a = textLine->attributes ();

  // adjust to startcol ;)
  s = s + startcol;
  a = a + startcol;

  uint curCol = startcol;

  // or we will see no text ;)
  uint oldY = y;
  y += fs.fontAscent;

  // painting loop
  uint xPos = 0;
  uint xPosAfter = 0;

  Attribute *oldAt = 0;

  QColor *curColor = 0;
  QColor *oldColor = 0;

  if (showSelections && !selectionPainted)
  {
    hasSel = selectBounds(line, startSel, endSel, oldLen);
  }

  uint oldCol = startcol;
  uint oldXPos = xPos;
  const QChar *oldS = s;

  bool isSel = false;

  //kdDebug(13020)<<"paint 1"<<endl;

  if (len < 1)
  {
    //  kdDebug(13020)<<"paint 2"<<endl;

    if ((showCursor > -1) && (showCursor == (int)curCol))
    {
      cursorVisible = true;
      cursorXPos = xPos;
      cursorMaxWidth = xPosAfter - xPos;
      cursorColor = &at[0].col;
    }
    //  kdDebug(13020)<<"paint 3"<<endl;

  }
  else
    {
  for (uint tmp = len; (tmp > 0); tmp--)
  {
    bool isTab = ((*s) == QChar('\t'));

    Attribute * curAt = ((*a) >= atLen) ? &at[0] : &at[*a];

    if (curAt != oldAt)
      paint.setFont(curAt->font(fs));

    xPosAfter += curAt->width(fs, *s);

    if (isTab)
      xPosAfter -= (xPosAfter % curAt->width(fs, *s));

    //  kdDebug(13020)<<"paint 5"<<endl;

    if ((int)xPosAfter >= xStart)
    {
      isSel = (showSelections && hasSel && (curCol >= startSel) && (curCol < endSel));

      curColor = isSel ? &(curAt->selCol) : &(curAt->col);

      if (curColor != oldColor)
        paint.setPen(*curColor);

      // make sure we redraw the right character groups on attrib/selection changes
      if (isTab)
      {
        if (!printerfriendly && isSel && !selectionPainted)
          paint.fillRect(xPos2 + oldXPos - xStart, oldY, xPosAfter - oldXPos, fs.fontHeight, colors[1]);

        if (showTabs)
        {
          paint.drawPoint(xPos2 + xPos - xStart, y);
          paint.drawPoint(xPos2 + xPos - xStart + 1, y);
          paint.drawPoint(xPos2 + xPos - xStart, y - 1);
        }

        oldCol = curCol+1;
        oldXPos = xPosAfter;
        oldS = s+1;
      }
      else if (
           (tmp < 2) || ((int)xPos > xEnd) || (curAt != &at[*(a+1)]) ||
           (isSel != (hasSel && ((curCol+1) >= startSel) && ((curCol+1) < endSel))) ||
           (((*(s+1)) == QChar('\t')) && !isTab)
         )
      {
        if (!printerfriendly && isSel && !selectionPainted)
          paint.fillRect(xPos2 + oldXPos - xStart, oldY, xPosAfter - oldXPos, fs.fontHeight, colors[1]);

        QConstString str((QChar *) oldS, curCol+1-oldCol);
        paint.drawText(xPos2 + oldXPos-xStart, y, str.string(), curCol+1-oldCol);

        if ((int)xPos > xEnd)
          break;

        oldCol = curCol+1;
        oldXPos = xPosAfter;
        oldS = s+1;
      }


      if ((showCursor > -1) && (showCursor == (int)curCol))
      {
        cursorVisible = true;
        cursorXPos = xPos;
        cursorMaxWidth = xPosAfter - xPos;
        cursorColor = &curAt->col;
      }
    }
    else
    {
      oldCol = curCol+1;
      oldXPos = xPosAfter;
      oldS = s+1;
    }

    //   kdDebug(13020)<<"paint 6"<<endl;

    // increase xPos
    xPos = xPosAfter;

    // increase char + attribs pos
    s++;
    a++;

    // to only switch font/color if needed
    oldAt = curAt;
    oldColor = curColor;

    // col move
    curCol++;
  }
  // kdDebug(13020)<<"paint 7"<<endl;
  if ((showCursor > -1) && (showCursor == (int)curCol))
  {
    cursorVisible = true;
    cursorXPos = xPos;
    cursorMaxWidth = xPosAfter - xPos;
    cursorColor = &oldAt->col;
  }
}
  //kdDebug(13020)<<"paint 8"<<endl;
  if (!printerfriendly && showSelections && !selectionPainted && lineEndSelected (line, endcol))
  {
    paint.fillRect(xPos2 + xPos-xStart, oldY, xEnd - xStart, fs.fontHeight, colors[1]);
    selectionPainted = true;
  }

  if (cursorVisible)
  {
    if (replaceCursor && (cursorMaxWidth > 2))
      paint.fillRect(xPos2 + cursorXPos-xStart, oldY, cursorMaxWidth, fs.fontHeight, *cursorColor);
    else
      paint.fillRect(xPos2 + cursorXPos-xStart, oldY, 2, fs.fontHeight, *cursorColor);
  }
  else if (showCursor > -1)
  {
    if ((cursorXPos2>=xStart) && (cursorXPos2<=xEnd))
    {
      cursorMaxWidth = fs.myFontMetrics.width(QChar (' '));

      if (replaceCursor && (cursorMaxWidth > 2))
        paint.fillRect(xPos2 + cursorXPos2-xStart, oldY, cursorMaxWidth, fs.fontHeight, myAttribs[0].col);
      else
        paint.fillRect(xPos2 + cursorXPos2-xStart, oldY, 2, fs.fontHeight, myAttribs[0].col);
    }
  }

  return true;
}

inline bool isStartBracket( const QChar& c ) { return c == '{' || c == '[' || c == '('; }
inline bool isEndBracket  ( const QChar& c ) { return c == '}' || c == ']' || c == ')'; }
inline bool isBracket     ( const QChar& c ) { return isStartBracket( c ) || isEndBracket( c ); }

/*
   Bracket matching uses the following algorithm:
   If in overwrite mode, match the bracket currently underneath the cursor.
   Otherwise, if the character to the left of the cursor is an ending bracket,
   match it. Otherwise if the character to the right of the cursor is a
   starting bracket, match it. Otherwise, if the the character to the left
   of the cursor is an starting bracket, match it. Otherwise, if the character
   to the right of the cursor is an ending bracket, match it. Otherwise, don't
   match anything.
*/
void KateDocument::newBracketMark( const KateTextCursor& cursor, BracketMark& bm )
{
  bm.valid = false;

  KateTextCursor start( cursor ), end;

  if( !findMatchingBracket( start, end ) )
    return;

  bm.valid = true;

  TextLine::Ptr textLine;
  Attribute* a;

  /* Calculate starting geometry */
  textLine = buffer->line( start.line );
  a = attribute( textLine->attribute( start.col ) );
  bm.startCol = start.col;
  bm.startLine = start.line;
  bm.startX = textWidth( textLine, start.col );
  bm.startW = a->width( viewFont, textLine->getChar( start.col ) );

  /* Calculate ending geometry */
  textLine = buffer->line( end.line );
  a = attribute( textLine->attribute( end.col ) );
  bm.endCol = end.col;
  bm.endLine = end.line;
  bm.endX = textWidth( textLine, end.col );
  bm.endW = a->width( viewFont, textLine->getChar( end.col ) );
}

bool KateDocument::findMatchingBracket( KateTextCursor& start, KateTextCursor& end )
{
  TextLine::Ptr textLine = buffer->line( start.line );
  if( !textLine )
    return false;

  QChar right = textLine->getChar( start.col );
  QChar left  = textLine->getChar( start.col - 1 );
  QChar bracket;

  if ( _configFlags & cfOvr ) {
    if( isBracket( right ) ) {
      bracket = right;
    } else {
      return false;
    }
  } else if ( isEndBracket( left ) ) {
    start.col--;
    bracket = left;
  } else if ( isStartBracket( right ) ) {
    bracket = right;
  } else if ( isBracket( left ) ) {
    start.col--;
    bracket = left;
  } else if ( isBracket( right ) ) {
    bracket = right;
  } else {
    return false;
  }

  QChar opposite;

  switch( bracket ) {
  case '{': opposite = '}'; break;
  case '}': opposite = '{'; break;
  case '[': opposite = ']'; break;
  case ']': opposite = '['; break;
  case '(': opposite = ')'; break;
  case ')': opposite = '('; break;
  default: return false;
  }

  bool forward = isStartBracket( bracket );
  int startAttr = textLine->attribute( start.col );
  uint count = 0;
  end = start;

  while( true ) {

    /* Increment or decrement, check base cases */
    if( forward ) {
      end.col++;
      if( end.col >= lineLength( end.line ) ) {
        if( end.line >= (int)lastLine() )
          return false;
        end.line++;
        end.col = 0;
        textLine = buffer->line( end.line );
      }
    } else {
      end.col--;
      if( end.col < 0 ) {
        if( end.line <= 0 )
          return false;
        end.line--;
        end.col = lineLength( end.line ) - 1;
        textLine = buffer->line( end.line );
      }
    }

    /* Easy way to skip comments */
    if( textLine->attribute( end.col ) != startAttr )
      continue;

    /* Check for match */
    QChar c = textLine->getChar( end.col );
    if( c == bracket ) {
      count++;
    } else if( c == opposite ) {
      if( count == 0 )
        return true;
      count--;
    }

  }
}

void KateDocument::guiActivateEvent( KParts::GUIActivateEvent *ev )
{
  KParts::ReadWritePart::guiActivateEvent( ev );
  if ( ev->activated() )
    emit selectionChanged();
}

void KateDocument::setDocName (QString docName)
{
  m_docName = docName;
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
    if (fileInfo->lastModified() != mTime) {
      if ( forceReload ||
           (KMessageBox::warningContinueCancel(0,
               (i18n("The file %1 has changed on disk.\nDo you want to reload the modified file?\n\nIf you choose Cancel and subsequently save the file, you will lose those modifications.")).arg(url().fileName()),
               i18n("File has Changed on Disk"),
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
    KateDocument::openURL( url() );
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
  closeURL ();
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

void KateDocument::setWordWrap (bool on)
{
  myWordWrap = on;
}

void KateDocument::setWordWrapAt (uint col)
{
  myWordWrapAt = col;
}

void KateDocument::applyWordWrap ()
{
  if (hasSelection())
    wrapText (selectStart.line, selectEnd.line, myWordWrapAt);
  else
    wrapText (myWordWrapAt);
}

uint KateDocument::configFlags ()
{
  return _configFlags;
}

void KateDocument::setConfigFlags( uint flags )
{
  if( flags == _configFlags )
    return;

  // update the view if visibility of tabs has changed
  bool updateView = (flags ^ _configFlags) & KateDocument::cfShowTabs;
  _configFlags = flags;
  if( updateView )
    updateViews();
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
    TextLine::Ptr textLine = buffer->line(curLine);
    //ASSERT(textLine != NULL);
    // for each character of the line : (curPos is the position in the line)
    for (uint curPos=0;curPos<textLine->length();curPos++)
    {
      Attribute *charAttributes = attribute(textLine->attribute(curPos));
      //ASSERT(charAttributes != NULL);
      // let's give the color for that character :
      if ( (charAttributes->col != previousCharacterColor))
      {  // the new character has a different color :
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

Kate::ConfigPage *KateDocument::viewDefaultsConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new ViewDefaultsConfig(p, "", this);
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
  return (Kate::ConfigPage*) new EditKeyConfiguration(p, this);
}

Kate::ConfigPage *KateDocument::hlConfigPage (QWidget *p)
{
  return (Kate::ConfigPage*) new HlConfigPage (p, this);
}

Kate::ActionMenu *KateDocument::hlActionMenu (const QString& text, QObject* parent, const char* name)
{
  KateViewHighlightAction *menu = new KateViewHighlightAction (text, parent, name);
  menu->setWhatsThis(i18n("Here you can choose how the current document should be highlighted."));
  menu->updateMenu (this);

  return (Kate::ActionMenu *)menu;
}

Kate::ActionMenu *KateDocument::exportActionMenu (const QString& text, QObject* parent, const char* name)
{
  KateExportAction *menu = new KateExportAction (text, parent, name);
  menu->updateMenu (this);
  menu->setWhatsThis(i18n("This command allows you to export the current document"
		" with all highlighting information into a markup document, e.g. HTML."));
  return (Kate::ActionMenu *)menu;
}


void KateDocument::dumpRegionTree()
{
  buffer->dumpRegionTree();
}

unsigned int KateDocument::getRealLine(unsigned int virtualLine)
{
  return buffer->lineNumber (virtualLine);
}

unsigned int KateDocument::getVirtualLine(unsigned int realLine)
{
  return buffer->lineVisibleNumber (realLine);
}

unsigned int KateDocument::visibleLines ()
{
  return buffer->countVisible ();
}

void KateDocument::slotLoadingFinished()
{
  tagAll();
}

const QFont& KateDocument::getFont( WhichFont wf )
{
  if( wf == ViewFont)
    return viewFont.myFont;
  else
    return printFont.myFont;
}

const KateFontMetrics& KateDocument::getFontMetrics( WhichFont wf )
{
  if( wf == ViewFont )
    return viewFont.myFontMetrics;
  else
    return printFont.myFontMetrics;
}

TextLine::Ptr KateDocument::kateTextLine(uint i)
{
  return buffer->line (i);
}

//
// KTextEditor::CursorInterface stuff
//
KTextEditor::Cursor *KateDocument::createCursor ( )
{
  return new KateCursor (this);
}
