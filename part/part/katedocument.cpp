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
#include "katehighlight.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "katesearch.h"
#include "katetextline.h"
#include "kateexportaction.h"
#include "katebuffer.h"
#include "katecodefoldinghelpers.h"
#include "kateundo.h"
#include "kateprintsettings.h"
#include "katelinerange.h"
#include "katesupercursor.h"
#include "katearbitraryhighlight.h"
#include "katerenderer.h"
#include "kateviewhighlightaction.h"
#include "katebrowserextension.h"
#include "kateattribute.h"

#include <ktexteditor/plugin.h>

#include <kio/netaccess.h>

#include <kparts/event.h>

#include <klocale.h>
#include <kglobal.h>
#include <kprinter.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <kconfig.h>
#include <kcharsets.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kspell.h>
#include <kstdaction.h>
#include <kiconloader.h>
#include <kxmlguifactory.h>
#include <kdialogbase.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <ksavefile.h>
#include <klibloader.h>

#include <qfileinfo.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qtimer.h>
#include <qfile.h>
#include <qclipboard.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qmap.h>
//END  includes

//BEGIN variables
using namespace Kate;

bool KateDocument::s_configLoaded = false;

int KateDocument::tabChars = 8;
int KateDocument::indentationChars = 2;

uint KateDocument::_configFlags
    = KateDocument::cfAutoIndent
    | KateDocument::cfTabIndents
    | KateDocument::cfKeepIndentProfile
    | KateDocument::cfRemoveSpaces
    | KateDocument::cfDelOnInput
    | KateDocument::cfWrapCursor
    | KateDocument::cfShowTabs
    | KateDocument::cfSmartHome;

QColor KateDocument::colors[6];

uint KateDocument::myUndoSteps = 0;

uint KateDocument::myBackupConfig = 1;
QString KateDocument::myBackupSuffix ("~");

bool KateDocument::myWordWrap = false;
uint KateDocument::myWordWrapAt = 80;

bool KateDocument::m_dynWordWrap = true;
int KateDocument::m_dynWrapIndicators = 1;  // follow line numbers
bool KateDocument::m_lineNumbers = false;
bool KateDocument::m_collapseTopLevelOnLoad = false;
bool KateDocument::m_iconBar = false;
bool KateDocument::m_foldingBar = true;
int KateDocument::m_bookmarkSort = 0;
bool KateDocument::m_wordWrapMarker = true;
int KateDocument::m_autoCenterLines = 0;
int KateDocument::m_getSearchTextFrom = KateDocument::SelectionOnly;

Kate::PluginList KateDocument::s_plugins;
//END variables

// BEGIN d'tor, c'tor
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
  m_undoDontMerge(false),
  m_undoIgnoreCancel(false),
  lastUndoGroupWhenSaved( 0 ),
  docWasSavedWhenUndoWasEmpty( true ),
  hlManager(HlManager::self ())
{
  KateFactory::registerDocument (this);

  setBlockSelectionInterfaceDCOPSuffix (documentDCOPSuffix());
  setConfigInterfaceDCOPSuffix (documentDCOPSuffix());
  setConfigInterfaceExtensionDCOPSuffix (documentDCOPSuffix());
  setCursorInterfaceDCOPSuffix (documentDCOPSuffix());
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

  // init global plugin list
  if (!s_configLoaded)
  {
    s_plugins.setAutoDelete (true);

    KTrader::OfferList::Iterator it(KateFactory::plugins()->begin());
    for( ; it != KateFactory::plugins()->end(); ++it)
    {
      KService::Ptr ptr = (*it);

      PluginInfo *info=new PluginInfo;

      info->load = false;
      info->service = ptr;
      info->plugin = 0L;

      s_plugins.append(info);
    }
  }

  // init local plugin list
  m_plugins.setAutoDelete (true);

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

  m_activeView = 0L;

  hlSetByUser = false;
  setInstance( KateFactory::instance() );

  editSessionNumber = 0;
  editIsRunning = false;
  noViewUpdates = false;
  m_editCurrentUndo = 0L;
  editWithUndo = false;

  //BEGIN spelling stuff
  m_kspell = 0;
  m_mispellCount = 0;
  m_replaceCount =  0;
  //END

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

  myEncoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());

  m_docName = QString ("");
  fileInfo = new QFileInfo ();

  m_highlight = 0L;

  m_undoMergeTimer = new QTimer(this);
  connect(m_undoMergeTimer, SIGNAL(timeout()), SLOT(undoCancel()));

  buffer = new KateBuffer (this);
  clear();
  docWasSavedWhenUndoWasEmpty = true;

  m_extension = new KateBrowserExtension( this );
  m_arbitraryHL = new KateArbitraryHighlight();

  // read the config THE FIRST TIME ONLY, we store everything in static vars
  // to ensure each document has the same config the whole time
  if (!s_configLoaded)
  {
    // some sane defaults for the first try
    KateRenderer::setFont(KateRenderer::ViewFont, KGlobalSettings::fixedFont());
    KateRenderer::setFont(KateRenderer::PrintFont, KGlobalSettings::fixedFont());
    KateRenderer::setTabWidth(8);

    colors[0] = KGlobalSettings::baseColor();
    colors[1] = KGlobalSettings::highlightColor();
    colors[2] = KGlobalSettings::alternateBackgroundColor();
    colors[3] = QColor( "#FFFF99" );
    colors[4] = colors[2].dark();
    colors[5] = QColor( "#EAE9E8" );

    // read the standard config to get some defaults
    readConfig();

    // now we have our stuff loaded, no more need to parse config on each
    // katedocument creation and to construct the masses of stuff
    s_configLoaded = true;
  }

  // load all plugins
  loadAllEnabledPlugins ();

  // uh my, we got modified ;)
  connect(this,SIGNAL(modifiedChanged ()),this,SLOT(slotModChanged ()));

  // some nice signals from the buffer
  connect(buffer, SIGNAL(loadingFinished()), this, SLOT(slotLoadingFinished()));
  connect(buffer, SIGNAL(linesChanged(int)), this, SLOT(slotBufferChanged()));
  connect(buffer, SIGNAL(tagLines(int,int)), this, SLOT(tagLines(int,int)));
  connect(buffer, SIGNAL(codeFoldingUpdated()),this,SIGNAL(codeFoldingUpdated()));

  // if the user changes the highlight with the dialog, notify the doc
  connect(hlManager,SIGNAL(changed()),SLOT(internalHlChanged()));

  // signal for the arbitrary HL
  connect(m_arbitraryHL, SIGNAL(tagLines(KateView*, KateSuperRange*)), SLOT(tagArbitraryLines(KateView*, KateSuperRange*)));

  // if single view mode, like in the konqui embedding, create a default view ;)
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
  //BEGIN spellcheck stuff
  if( m_kspell )
  {
    m_kspell->setAutoDelete(true);
    m_kspell->cleanUp(); // need a way to wait for this to complete
    delete m_kspell;
  }
  //END

  //
  // other stuff
  //
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
//END

//BEGIN Plugins
void KateDocument::loadAllEnabledPlugins ()
{
  for (uint i=0; i<s_plugins.count(); i++)
  {
    if (s_plugins.at(i)->load)
      loadPlugin (m_plugins.at(i));
    else
      unloadPlugin (m_plugins.at(i));
  }
}

void KateDocument::enableAllPluginsGUI (KateView *view)
{
  for (uint i=0; i<m_plugins.count(); i++)
  {
    if  (m_plugins.at(i)->plugin)
      enablePluginGUI (m_plugins.at(i), view);
  }
}

void KateDocument::loadPlugin (PluginInfo *item)
{
  if (item->plugin) return;

  item->plugin = KTextEditor::createPlugin (QFile::encodeName(item->service->library()), this);

  enablePluginGUI (item);
}

void KateDocument::unloadPlugin (PluginInfo *item)
{
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

  for (KateView * view = m_views.first(); view != 0L; view = m_views.next() )
  {
    KXMLGUIFactory *factory = view->factory();
    if ( factory )
      factory->removeClient( view );

    KTextEditor::PluginViewInterface *viewIface = KTextEditor::pluginViewInterface( item->plugin );
    viewIface->removeView( view );

    if ( factory )
      factory->addClient( view );
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
//END

//BEGIN KTextEditor::Document stuff

KTextEditor::View *KateDocument::createView( QWidget *parent, const char *name )
{
  KateView* newView = new KateView( this, parent, name);
  connect(newView, SIGNAL(cursorPositionChanged()), SLOT(undoCancel()));
  return newView;
}

QPtrList<KTextEditor::View> KateDocument::views () const
{
  return m_textEditViews;
};
//END

//BEGIN KTextEditor::ConfigInterfaceExtension stuff

uint KateDocument::configPages () const
{
  return 11;
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
      return editConfigPage (parent);

    case 3:
      return keysConfigPage (parent);

    case 4:
      return indentConfigPage(parent);

    case 5:
      return selectConfigPage(parent);

    case 6:
      return saveConfigPage( parent );

    case 7:
      return viewDefaultsConfigPage(parent);

    case 8:
      return hlConfigPage (parent);

    case 9:
      return new SpellConfigPage (parent);

    case 10:
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

    case 4:
      return i18n ("Indentation");

    case 5:
      return i18n ("Selection");

    case 2:
      return i18n ("Editing");

    case 3:
      return i18n ("Shortcuts");

    case 8:
      return i18n ("Highlighting");

    case 7:
      return i18n ("View Defaults");

    case 10:
      return i18n ("Plugins");

    case 6:
      return i18n("Saving");

    case 9:
      return i18n("Spelling");

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

    case 4:
      return i18n ("Indentation Rules");

    case 5:
      return i18n ("Selection Behavior");

    case 2:
      return i18n ("Editing Options");

    case 3:
      return i18n ("Shortcuts Configuration");

    case 8:
      return i18n ("Highlighting Rules");

    case 7:
      return i18n("View Defaults");

    case 10:
      return i18n ("Plugin Manager");

    case 6:
      return i18n("Saving & Backups");

    case 9:
      return i18n("Spell Checker Behavior");

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

    case 4:
      return BarIcon("rightjust", size);

    case 5:
      return BarIcon("frame_edit", size);

    case 2:
      return BarIcon("edit", size);

    case 3:
      return BarIcon("key_enter", size);

    case 8:
      return BarIcon("source", size);

    case 7:
      return BarIcon("view_text",size);

    case 10:
      return BarIcon("connect_established", size);

    case 6:
      return BarIcon("filesave", size);

    case 9:
      return BarIcon("spellcheck", size);

    default:
      return 0;
  }
}
//END

//BEGIN KTextEditor::EditInterface stuff

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
//END

//BEGIN KTextEditor::EditInterface internal stuff
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
    undoStart();
  else
    undoCancel();

  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->m_viewInternal->editStart();
  }
}

void KateDocument::undoStart()
{
  if (m_editCurrentUndo) return;

  // Make sure the buffer doesn't get bigger than requested
  if ((myUndoSteps > 0) && (undoItems.count() > myUndoSteps))
  {
    undoItems.setAutoDelete(true);
    undoItems.removeFirst();
    undoItems.setAutoDelete(false);
    docWasSavedWhenUndoWasEmpty = false;
  }

  // new current undo item
  m_editCurrentUndo = new KateUndoGroup(this);
}

void KateDocument::undoEnd()
{
  if (m_editCurrentUndo)
  {
    if (!m_undoDontMerge && undoItems.last() && undoItems.last()->merge(m_editCurrentUndo))
      delete m_editCurrentUndo;
    else
      undoItems.append(m_editCurrentUndo);

    m_undoDontMerge = false;
    m_undoIgnoreCancel = true;

    m_editCurrentUndo = 0L;

    // (Re)Start the single-shot timer to cancel the undo merge
    // the user has 5 seconds to input more data, or undo merging gets cancelled for the current undo item.
    m_undoMergeTimer->start(5000, true);

    emit undoChanged();
  }
}

void KateDocument::undoCancel()
{
  if (m_undoIgnoreCancel) {
    m_undoIgnoreCancel = false;
    return;
  }

  m_undoDontMerge = true;

  Q_ASSERT(!m_editCurrentUndo);

  // As you can see by the above assert, neither of these should really be required
  delete m_editCurrentUndo;
  m_editCurrentUndo = 0L;
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
    updateLines(editTagLineStart, editTagLineEnd+1);

  if (editWithUndo)
    undoEnd();

  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->m_viewInternal->editEnd(editTagLineStart, editTagLineEnd);
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
    TextLine::Ptr l = buffer->line(line);

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
  if (editIsRunning && editWithUndo && m_editCurrentUndo) {
    m_editCurrentUndo->addItem(type, line, col, len, text);

    // Clear redo buffer
    if (redoItems.count()) {
      redoItems.setAutoDelete(true);
      redoItems.clear();
      redoItems.setAutoDelete(false);
    }
  }
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
  TextLine::Ptr l = buffer->line(line);

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
  TextLine::Ptr l = buffer->line(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editRemoveText, line, col, len, l->string().mid(col, len));

  l->removeText (col, len);

  buffer->changeLine(line);

  editTagLine(line);

  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->m_viewInternal->editRemoveText(line, col, len);
  }

  editEnd ();

  return true;
}

bool KateDocument::editWrapLine ( uint line, uint col, bool autowrap)
{
  TextLine::Ptr l = buffer->line(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editWrapLine, line, col, 0, 0);

  TextLine::Ptr nl = buffer->line(line+1);
  TextLine::Ptr tl = new TextLine(buffer);
  int llen = l->length(), nllen = 0;

  if (!nl || !autowrap)
  {
    l->wrap (tl, col);

    buffer->insertLine (line+1, tl);
    buffer->changeLine(line);

    QPtrList<KTextEditor::Mark> list;
    for( QIntDictIterator<KTextEditor::Mark> it( m_marks ); it.current(); ++it )
    {
      if( it.current()->line > line )
        list.append( it.current() );
    }

    for( QPtrListIterator<KTextEditor::Mark> it( list ); it.current(); ++it )
    {
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
      m_views.at(z)->m_viewInternal->editWrapLine(line, col, tl->length());
    else
    {
      int offset = llen - m_views.at(z)->m_viewInternal->cursorCache.col();
      offset = (nl ? nllen:tl->length()) - offset;
      if(offset < 0) offset = 0;
      m_views.at(z)->m_viewInternal->editWrapLine(line, col, offset);
    }
  }
  editEnd ();
  return true;
}

bool KateDocument::editUnWrapLine ( uint line, uint col )
{
  TextLine::Ptr l = buffer->line(line);
  TextLine::Ptr tl = buffer->line(line+1);

  if (!l || !tl)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editUnWrapLine, line, col, 0, 0);

  l->unWrap (col, tl, tl->length());

  buffer->changeLine(line);
  buffer->removeLine(line+1);

  QPtrList<KTextEditor::Mark> list;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks ); it.current(); ++it )
  {
    if( it.current()->line >= line+1 )
      list.append( it.current() );

    if ( it.current()->line == line+1 )
    {
      KTextEditor::Mark* mark = m_marks.take( line );

      if (mark)
      {
        it.current()->type |= mark->type;
      }
    }
  }

  for( QPtrListIterator<KTextEditor::Mark> it( list ); it.current(); ++it )
  {
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
    m_views.at(z)->m_viewInternal->editUnWrapLine(line, col);
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

  TextLine::Ptr tl = new TextLine(buffer);
  tl->append(s.unicode(),s.length());
  buffer->insertLine(line, tl);
  buffer->changeLine(line);

  editInsertTagLine (line);
  editTagLine(line);

  QPtrList<KTextEditor::Mark> list;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks ); it.current(); ++it )
  {
    if( it.current()->line >= line )
      list.append( it.current() );
  }

  for( QPtrListIterator<KTextEditor::Mark> it( list ); it.current(); ++it )
  {
    KTextEditor::Mark* mark = m_marks.take( it.current()->line );
    mark->line++;
    m_marks.insert( mark->line, mark );
  }

  if( !list.isEmpty() )
    emit marksChanged();

  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->m_viewInternal->setViewTagLinesFrom(line);
  }

  editEnd ();

  return true;
}

bool KateDocument::editRemoveLine ( uint line )
{
  if ( line > lastLine() )
    return false;

  if ( numLines() == 1 )
    return editRemoveText (0, 0, buffer->line(0)->length());

  editStart ();

  editAddUndo (KateUndoGroup::editRemoveLine, line, 0, lineLength(line), textLine(line));

  buffer->removeLine(line);

  editRemoveTagLine (line);

  QPtrList<KTextEditor::Mark> list;
  KTextEditor::Mark* rmark = 0;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks ); it.current(); ++it )
  {
    if ( (it.current()->line > line) )
      list.append( it.current() );
    else if ( (it.current()->line == line) )
      rmark = it.current();
  }

  if (rmark)
    delete (m_marks.take (rmark->line));

  for( QPtrListIterator<KTextEditor::Mark> it( list ); it.current(); ++it )
  {
    KTextEditor::Mark* mark = m_marks.take( it.current()->line );
    mark->line--;
    m_marks.insert( mark->line, mark );
  }

  if( !list.isEmpty() )
    emit marksChanged();

  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->m_viewInternal->editRemoveLine(line);
  }

  editEnd();

  return true;
}
//END

//BEGIN KTextEditor::SelectionInterface stuff

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

  if (hasSelection() || selectAnchor.line() != -1)
    tagSelection();

  repaintViews();

  emit selectionChanged ();

  return true;
}

bool KateDocument::setSelection( uint startLine, uint startCol, uint endLine, uint endCol )
{
  if (hasSelection())
    clearSelection(false);

  selectAnchor.setPos(startLine, startCol);

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

  return text (selectStart.line(), sc, selectEnd.line(), ec, blockSelect);
}

bool KateDocument::removeSelectedText ()
{
  if (!hasSelection())
    return false;

  editStart ();

  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->m_viewInternal->removeSelectedText(selectStart);
  }

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

  removeText (selectStart.line(), sc, selectEnd.line(), ec, blockSelect);

  // don't redraw the cleared selection - that's done in editEnd().
  clearSelection(false);

  editEnd ();

  return true;
}

bool KateDocument::selectAll()
{
  return setSelection (0, 0, lastLine(), lineLength(lastLine()));
}
//END

//BEGIN KTextEditor::BlockSelectionInterface stuff

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

    for (KateView * view = m_views.first(); view; view = m_views.next())
    {
      view->slotSelectionTypeChanged();
    }
  }

  return true;
}

bool KateDocument::toggleBlockSelectionMode ()
{
  return setBlockSelectionMode (!blockSelect);
}
//END

//BEGIN KTextEditor::UndoInterface stuff

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

  for (uint z=0; z < KateFactory::documents()->count(); z++)
    emit KateFactory::documents()->at(z)->undoChanged ();
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
//END

//BEGIN KTextEditor::SearchInterface stuff

bool KateDocument::searchText (unsigned int startLine, unsigned int startCol, const QString &text, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive, bool backwards)
{
  if (text.isEmpty())
    return false;

  int line = startLine;
  int col = startCol;

  if (!backwards)
  {
    int searchEnd = lastLine();

    while (line <= searchEnd)
    {
      TextLine::Ptr textLine = buffer->plainLine(line);

      if (!textLine)
        return false;

      uint foundAt, myMatchLen;
      bool found = textLine->searchText (col, text, &foundAt, &myMatchLen, casesensitive, false);

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
    int searchEnd = 0;

    while (line >= searchEnd)
    {
      TextLine::Ptr textLine = buffer->plainLine(line);

      if (!textLine)
        return false;

      uint foundAt, myMatchLen;
      bool found = textLine->searchText (col, text, &foundAt, &myMatchLen, casesensitive, true);

      if (found)
      {
        if ((uint) line == startLine && foundAt + myMatchLen >= (uint) col
            && line == selectStart.line() && foundAt == (uint) selectStart.col()
            && line == selectEnd.line() && foundAt + myMatchLen == (uint) selectEnd.col())
        {
          // To avoid getting stuck at one match we skip a match if it is already
          // selected (most likely because it has just been found).
          if (foundAt > 0)
            col = foundAt - 1;
          else {
            if (--line >= 0)
              col = lineLength(line);
          }
          continue;
        }

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
  if (regexp.isEmpty() || !regexp.isValid())
    return false;

  int line = startLine;
  int col = startCol;

  if (!backwards)
  {
    int searchEnd = lastLine();

    while (line <= searchEnd)
    {
      TextLine::Ptr textLine = buffer->plainLine(line);

      if (!textLine)
        return false;

      uint foundAt, myMatchLen;
      bool found = textLine->searchText (col, regexp, &foundAt, &myMatchLen, false);

      if (found)
      {
        // A special case which can only occur when searching with a regular expression consisting
        // only of a lookahead (e.g. ^(?=\{) for a function beginning without selecting '{').
        if (myMatchLen == 0 && (uint) line == startLine && foundAt == (uint) col)
        {
          if (col < lineLength(line))
            col++;
          else {
            line++;
            col = 0;
          }
          continue;
        }

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
    int searchEnd = 0;

    while (line >= searchEnd)
    {
      TextLine::Ptr textLine = buffer->plainLine(line);

      if (!textLine)
        return false;

      uint foundAt, myMatchLen;
      bool found = textLine->searchText (col, regexp, &foundAt, &myMatchLen, true);

      if (found)
      {
        if ((uint) line == startLine && foundAt + myMatchLen >= (uint) col
            && line == selectStart.line() && foundAt == (uint) selectStart.col()
            && line == selectEnd.line() && foundAt + myMatchLen == (uint) selectEnd.col())
        {
          // To avoid getting stuck at one match we skip a match if it is already
          // selected (most likely because it has just been found).
          if (foundAt > 0)
            col = foundAt - 1;
          else {
            if (--line >= 0)
              col = lineLength(line);
          }
          continue;
        }

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
//END

//BEGIN KTextEditor::HighlightingInterface stuff

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
//END

//BEGIN KTextEditor::ConfigInterface stuff
//

void KateDocument::readConfig(KConfig *config)
{
  // set kate document section
  config->setGroup("Kate Document");

  // basic and search config flags
  _configFlags = config->readNumEntry("Basic Config Flags", _configFlags);
  KateSearch::s_options = config->readNumEntry("Search Config Flags", KateSearch::s_options);

  // does we use static word wrap and on which column
  myWordWrap = config->readBoolEntry("Word Wrap", myWordWrap);
  myWordWrapAt = config->readNumEntry("Word Wrap Column", myWordWrapAt);

  // tabs & indentations
  setTabWidth(config->readNumEntry("Tab Width", tabChars));
  setIndentationWidth(config->readNumEntry("Indentation Width", indentationChars));

  // undo steps
  setUndoSteps(config->readNumEntry("Undo Steps", myUndoSteps));

  KateRenderer::setFont(KateRenderer::ViewFont, config->readFontEntry("View Font", &KateRenderer::getFont(KateRenderer::ViewFont)));
  KateRenderer::setFont(KateRenderer::PrintFont, config->readFontEntry("Printer Font", &KateRenderer::getFont(KateRenderer::PrintFont)));

  colors[0] = config->readColorEntry("Color Background", &colors[0]);
  colors[1] = config->readColorEntry("Color Selected", &colors[1]);
  colors[2] = config->readColorEntry("Color Current Line", &colors[2]);
  colors[3] = config->readColorEntry("Color Bracket Highlight", &colors[3]);
  colors[4] = config->readColorEntry("Color WWMarker", &colors[4]);
  colors[5] = config->readColorEntry("Color Icon Border", &colors[5]);

  myBackupConfig = config->readNumEntry( "Backup Config Flags", myBackupConfig);
  myBackupSuffix = config->readEntry("Backup Files Suffix", myBackupSuffix);

  config->setGroup("Kate Plugins");
  for (uint i=0; i<s_plugins.count(); i++)
    if  (config->readBoolEntry(s_plugins.at(i)->service->library(), false))
      s_plugins.at(i)->load = true;

  config->setGroup("Kate View");
  m_dynWordWrap = config->readBoolEntry( "Dynamic Word Wrap", m_dynWordWrap );
  m_dynWrapIndicators = config->readNumEntry( "Dynamic Word Wrap Indicators", m_dynWrapIndicators );
  m_lineNumbers = config->readBoolEntry( "Line Numbers", m_lineNumbers );
  m_iconBar = config->readBoolEntry( "Icon Bar", m_iconBar );
  m_foldingBar = config->readBoolEntry( "Folding Markers", m_foldingBar );
  m_collapseTopLevelOnLoad = config->readBoolEntry("Collapse Top Level On Load", m_collapseTopLevelOnLoad);
  m_bookmarkSort = config->readNumEntry( "Bookmark Menu Sorting", m_bookmarkSort );
  m_wordWrapMarker = config->readBoolEntry("Word Wrap Marker", m_wordWrapMarker );
  m_autoCenterLines = config->readNumEntry( "Auto Center Lines", m_autoCenterLines );
  m_getSearchTextFrom = config->readNumEntry( "Get Search Text From", m_getSearchTextFrom );

  for (uint z=0; z < KateFactory::documents()->count(); z++)
    KateFactory::documents()->at(z)->loadAllEnabledPlugins ();

  // update view defaults
  for (uint z=0; z < KateFactory::views()->count(); z++)
  {
    KateFactory::views()->at(z)->updateViewDefaults ();;
  }

  // update the remaining document stuff
  for (uint z=0; z < KateFactory::documents()->count(); z++)
  {
    KateFactory::documents()->at(z)->tagAll();
    KateFactory::documents()->at(z)->updateViews();
  }
}

void KateDocument::updateViewDefaults ()
{
  for (uint i=0; i<m_views.count(); i++)
    m_views.at(i)->updateViewDefaults ();
}

void KateDocument::writeConfig(KConfig *config)
{
  config->setGroup("Kate Document");
  config->writeEntry("Basic Config Flags",_configFlags);
  config->writeEntry("Search Config Flags",KateSearch::s_options);

  config->writeEntry("Word Wrap", myWordWrap);
  config->writeEntry("Word Wrap Column", myWordWrapAt);
  config->writeEntry("Undo Steps", myUndoSteps);
  config->writeEntry("Tab Width", tabChars);
  config->writeEntry("Indentation Width", indentationChars);
  config->writeEntry("View Font", KateRenderer::getFont(KateRenderer::ViewFont));
  config->writeEntry("Printer Font", KateRenderer::getFont(KateRenderer::PrintFont));
  config->writeEntry("Color Background", colors[0]);
  config->writeEntry("Color Selected", colors[1]);
  config->writeEntry("Color Current Line", colors[2]);
  config->writeEntry("Color Bracket Highlight", colors[3]);
  config->writeEntry("Color WWMarker", colors[4] );
  config->writeEntry("Color Icon Border", colors[5] );

  config->writeEntry( "Backup Config Flags", myBackupConfig );
  config->writeEntry( "Backup Files Suffix", myBackupSuffix );

  config->setGroup("Kate Plugins");
  for (uint i=0; i<s_plugins.count(); i++)
    config->writeEntry(s_plugins.at(i)->service->library(), s_plugins.at(i)->load);

  config->setGroup("Kate View");
  config->writeEntry( "Dynamic Word Wrap", m_dynWordWrap );
  config->writeEntry( "Dynamic Word Wrap Indicators", m_dynWrapIndicators );
  config->writeEntry( "Line Numbers", m_lineNumbers );
  config->writeEntry( "Icon Bar", m_iconBar );
  config->writeEntry( "Folding Markers", m_foldingBar );
  config->writeEntry( "Collapse Top Level On Load", m_collapseTopLevelOnLoad );
  config->writeEntry( "Bookmark Menu Sorting", m_bookmarkSort );
  config->writeEntry( "Word Wrap Marker", m_wordWrapMarker );
  config->writeEntry( "Auto Center Lines", m_autoCenterLines );
  config->writeEntry( "Get Search Text From", m_getSearchTextFrom );

  config->sync();
}

void KateDocument::readConfig()
{
  KConfig *config = KateFactory::instance()->config();
  readConfig (config);
}

void KateDocument::writeConfig()
{
  KConfig *config = KateFactory::instance()->config();
  writeConfig (config);
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
  KDialogBase *kd = new KDialogBase(KDialogBase::IconList,
                                    i18n("Configure"),
                                    KDialogBase::Ok | KDialogBase::Cancel |
                                    KDialogBase::Help ,
                                    KDialogBase::Ok, kapp->mainWidget());

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
  repaintViews(true);
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
  repaintViews(true);
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
  repaintViews(true);
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
  repaintViews(true);
}

void KateDocument::setPixmap( MarkInterface::MarkTypes type, const QPixmap& pixmap )
{
  m_markPixmaps.replace( type, new QPixmap( pixmap ) );
}

void KateDocument::setDescription( MarkInterface::MarkTypes type, const QString& description )
{
  m_markDescriptions.replace( type, new QString( description ) );
}

QPixmap *KateDocument::markPixmap( MarkInterface::MarkTypes type )
{
  return m_markPixmaps[type];
}

QColor KateDocument::markColor( MarkInterface::MarkTypes type )
{
  switch (type) {
    // Bookmark
    case markType01:
      return Qt::blue;

    // Breakpoint
    case markType02:
      return Qt::red;

    // ActiveBreakpoint
    case markType03:
      return Qt::yellow;

    // ReachedBreakpoint
    case markType04:
      return Qt::magenta;

    // DisabledBreakpoint
    case markType05:
      return Qt::gray;

    // ExecutionPoint
    case markType06:
      return Qt::green;

    default:
      return QColor();
  }
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
//END

//BEGIN KTextEditor::PrintInterface stuff

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
     KateRenderer renderer(this);
     renderer.setPrinterFriendly(true);

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
         if ( selectStart.line() < selectEnd.line() )
         {
           firstline = selectStart.line();
           selStartCol = selectStart.col();
           lastline = selectEnd.line();
           selEndCol = selectEnd.col();
         }
         else // TODO check if this is possible!
         {
           kdDebug()<<"selectStart > selectEnd!"<<endl;
           firstline = selectEnd.line();
           selStartCol = selectEnd.col();
           lastline = selectStart.line();
           selEndCol = selectStart.col();
         }
         lineCount = firstline;
       }

       if ( printLineNumbers )
       {
         // figure out the horiizontal space required
         QString s( QString("%1 ").arg( numLines() ) );
         s.fill('5', -1); // some non-fixed fonts haven't equally wide numbers
                          // FIXME calculate which is actually the widest...
         lineNumberWidth = renderer.currentFontMetrics().width( s );
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
         tags["U"] =  url().prettyURL();
         if ( selectionOnly )
         {
           QString s( i18n("(Selection of) ") );
           tags["f"].prepend( s );
           tags["U"].prepend( s );
         }

         QRegExp reTags( "%([dDfUhuyY])" ); // TODO tjeck for "%%<TAG>"

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
         QString _title = i18n("Typographical Conventions for %1").arg(m_highlight->name());
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
                                _d->bold() ?
                                  _d->italic() ?
                                    KateRenderer::getFontStruct(KateRenderer::PrintFont).myFontMetricsBI :
                                    KateRenderer::getFontStruct(KateRenderer::PrintFont).myFontMetricsBold :
                                  _d->italic() ?
                                    KateRenderer::getFontStruct(KateRenderer::PrintFont).myFontMetricsItalic :
                                    KateRenderer::getFontStruct(KateRenderer::PrintFont).myFontMetrics
                                    ) ).width( _d->name ) );
           _items++;
           ++it;
         }
         guideCols = _w/( _widest + innerMargin );
         // add height for required number of lines needed given columns
         guideHeight += renderer.fontHeight() * ( _items/guideCols );
         if ( _items%guideCols )
           guideHeight += renderer.fontHeight();
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
         int _lpp = _ph / renderer.fontHeight();
         uint _lt = 0, _c=0;
         // add space for guide if required
         if ( useGuide )
           _lt += (guideHeight + (renderer.fontHeight() /2)) / renderer.fontHeight();
         long _lw;
         for ( uint i = firstline; i < lastline; i++ )
         {
           _lw = renderer.textWidth( kateTextLine( i ), -1 );
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

         // substitute both tag lists
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

       while (needWrap)
       {
         if ( y + renderer.fontHeight() >= (uint)(maxHeight) )
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
             paint.setFont( KateRenderer::getFontStruct(KateRenderer::PrintFont).myFontBold );
             QRect _r;
             paint.drawText( _marg, y, pdmWidth-(2*_marg), maxHeight - y,
                Qt::AlignTop|Qt::AlignHCenter,
                i18n("Typographical Conventions for %1").arg(m_highlight->name()), -1, &_r );
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

             while ( ( _d = _it.current() ) != 0 )
             {
               paint.setPen( attribute(_i)->textColor() );
               paint.setFont( attribute(_i)->font( renderer.currentFont() ) );
               paint.drawText(( _x + ((_i%guideCols)*_cw)), y, _cw, renderer.fontHeight(),
                        Qt::AlignVCenter|Qt::AlignLeft, _d->name, -1, &_r );
               _i++;
               if ( _i && ! ( _i%guideCols ) ) y += renderer.fontHeight();
               ++_it;
             }
             if ( _i%guideCols ) y += renderer.fontHeight();// last row not full
             y += ( useBox ? boxWidth : 1 ) + (innerMargin*2);
           }

           pageStarted = false;
         } // pageStarted; move on to contents:)

         if ( printLineNumbers && ! startCol ) // don't repeat!
         {
           paint.setFont( KateRenderer::getFontStruct(KateRenderer::PrintFont).font( false, false ) );
           paint.setPen( colors[1] ); // using "selected" color for now...
           paint.drawText( (( useBox || useBackground ) ? innerMargin : 0), y,
                        lineNumberWidth, renderer.fontHeight(),
                        Qt::AlignRight, QString("%1").arg( lineCount + 1 ) );
         }
         endCol = renderer.textWidth(buffer->line(lineCount), startCol, maxWidth, &needWrap);

         if ( endCol < startCol )
         {
           //kdDebug(13020)<<"--- Skipping garbage, line: "<<lineCount<<" start: "<<startCol<<" end: "<<endCol<<" real EndCol; "<< buffer->line(lineCount)->length()<< " !?"<<endl;
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
         // FIXME Convert this function + related functionality to a separate KatePrintView
         LineRange range;
         range.line = lineCount;
         range.startCol = startCol;
         range.endCol = endCol;
         range.wrap = needWrap;
         paint.translate(xstart, y);
         renderer.paintTextLine(paint, &range, 0, maxWidth);
         paint.resetXForm();
         if ( skip )
         {
           needWrap = false;
           startCol = 0;
         }
         else
         {
           startCol = endCol;
         }

         y += renderer.fontHeight();
         _count++;
       } // done while ( needWrap )

       lineCount++;
     } // done lineCount <= lastline
     return true;
  }

  return false;
}

bool KateDocument::print ()
{
  return printDialog ();
}
//END

//BEGIN KParts::ReadWrite stuff

bool KateDocument::openFile()
{
  fileInfo->setFile (m_file);
  setMTime();

  if (!fileInfo->exists() || !fileInfo->isReadable())
  {
    // m_file && m_url have changed e.g. with "kwrite filethatdoesntexist"
    emit fileNameChanged();
    return false;
  }

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

    int bufpos = 0;
    for (uint i=0; i < buffer->count(); i++)
    {
      QString line = textLine(i);
      int len = line.length() + 1; // space for a newline - seemingly not required by kmimemagic, but nicer for debugging.

      if (bufpos + len > HOWMANY)
        len = HOWMANY - bufpos;

      memcpy(&buf[bufpos], (line+"\n").latin1(), len);
      bufpos += len;

      if (bufpos >= HOWMANY)
        break;
    }

    hl = hlManager->mimeFind( buf, m_file );
  }

  internalSetHlMode(hl);

  updateLines();
  updateViews();

  // FIXME clean up this feature
  if ( m_collapseTopLevelOnLoad ) {
kdDebug()<<"calling collapseToplevelNodes()"<<endl;
    foldingTree()->collapseToplevelNodes();
  }

  emit fileNameChanged();

  return success;
}

bool KateDocument::save()
{
  // FIXME reorder for efficiency, prompt user in case of failure
  bool l ( url().isLocalFile() );
  if ( ( ( l && myBackupConfig & LocalFiles ) ||
         ( ! l && myBackupConfig & RemoteFiles ) )
       && modified ) {
    KURL u( url().path() + myBackupSuffix );
    if ( ! KIO::NetAccess::upload( url().path(), u ) )
      kdDebug(13020)<<"backing up failed ("<<url().prettyURL()<<" -> "<<u.prettyURL()<<")"<<endl;
  }
  return KParts::ReadWritePart::save();
}

bool KateDocument::saveFile()
{
  QString eol ("\n");

  if (eolMode == KateDocument::eolDos) eol = QString("\r\n");
  else if (eolMode == KateDocument::eolMacintosh) eol = QString ("\r");

  if (!buffer->canEncode (KGlobal::charsets()->codecForName(myEncoding)))
    KMessageBox::error (0, i18n ("The document has been saved, but the selected encoding cannot encode every unicode character in it. "
    "If you don't save it again with another encoding, some characters will be lost after closing this document."));

  bool success = buffer->saveFile (m_file, KGlobal::charsets()->codecForName(myEncoding), eol);

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

    uint mode = hl;
    if (mode != hlMode ())
      internalSetHlMode(hl);
  }

  emit fileNameChanged ();

  setDocName  (url().fileName());

  // last stuff to do
  fileInfo->setFile (m_file);
  setMTime();

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
  }

  if ( m == false ) docWasSavedWhenUndoWasEmpty = undoItems.isEmpty();
}

bool KateDocument::isModified() const {
  return modified;
}
//END

//BEGIN Kate specific stuff ;)

void KateDocument::setTabWidth(int chars)
{
  if (tabChars == chars) return;
  if (chars < 1) chars = 1;
  if (chars > 16) chars = 16;

  tabChars = chars;

  KateRenderer::setTabWidth(tabChars);

  for (uint z=0; z < KateFactory::documents()->count(); z++)
  {
    KateFactory::documents()->at(z)->updateFontData();
    KateFactory::documents()->at(z)->updateViews();
  }
}

void KateDocument::setIndentationWidth(int chars)
{
  if (indentationChars == chars)
    return;
  if (chars < 1)
    chars = 1;
  if (chars > 16)
    chars = 16;

  indentationChars = chars;
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

uint KateDocument::currentColumn( const KateTextCursor& cursor )
{
  TextLine::Ptr textLine = buffer->plainLine(cursor.line());

  if (textLine)
    return textLine->cursorX(cursor.col(), tabChars);
  else
    return 0;
}

bool KateDocument::insertChars ( int line, int col, const QString &chars, KateView *view )
{
  QString buf;

  int savedCol = col;
  int savedLine = line;
  QString savedChars(chars);

  TextLine::Ptr textLine = buffer->plainLine(line);

  uint pos = 0;
  bool onlySpaces = true;
  for( uint z = 0; z < chars.length(); z++ )
  {
    QChar ch = chars[z];
    if (ch.isPrint() || ch == '\t') {
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
    line = view->m_viewInternal->cursorCache.line();
    col = view->m_viewInternal->cursorCache.col();
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

static QString tabString(int pos, int tabChars)
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

  if (c.line() > (int)lastLine())
   c.setLine(lastLine());

  TextLine::Ptr textLine = kateTextLine(c.line());
  if (c.col() > (int)textLine->length())
    c.setCol(textLine->length());

  if (!(_configFlags & KateDocument::cfAutoIndent)) {
    insertText( c.line(), c.col(), "\n" );
    c.setPos(c.line() + 1, 0);
  } else {
    int pos = textLine->firstChar();
    if (c.col() < pos) c.setCol(pos); // place cursor on first char if before

    int y = c.line();
    while ((y > 0) && (pos < 0)) { // search a not empty text line
      textLine = buffer->plainLine(--y);
      pos = textLine->firstChar();
    }
    insertText (c.line(), c.col(), "\n");
    c.setPos(c.line() + 1, 0);

    if (pos > 0) {
      pos = textLine->cursorX(pos, tabChars);
      QString s = tabString(pos, (_configFlags & KateDocument::cfSpaceIndent) ? 0xffffff : tabChars);
      insertText (c.line(), c.col(), s);
      pos = s.length();
      c.setCol(pos);
    }
  }

  editEnd();
}

void KateDocument::transpose( const KateTextCursor& cursor)
{
  TextLine::Ptr textLine = buffer->plainLine(cursor.line());
  uint line = cursor.line();
  uint col = cursor.col();

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

  uint col = QMAX( c.col(), 0 );
  uint line = QMAX( c.line(), 0 );

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

      TextLine::Ptr textLine = buffer->plainLine(line);
      int colX = textLine->cursorX(col, tabChars);
      int pos = textLine->firstChar();
      if (pos > 0)
        pos = textLine->cursorX(pos, tabChars);

      if (pos < 0 || pos >= (int)colX)
      {
        // only spaces on left side of cursor
        // search a line with less spaces
        int y = line;
        while (--y >= 0)
        {
          textLine = buffer->plainLine(y);
          pos = textLine->firstChar();

          if (pos >= 0)
          {
            pos = textLine->cursorX(pos, tabChars);
            if (pos < (int)colX)
            {
              replaceWithOptimizedSpace(line, col, pos, _configFlags);
              break;
            }
          }
        }
        if (y < 0) {
          // FIXME: what shoud we do in this case?
          removeText(line, 0, line, col);
        }
      }
      else
        removeText(line, col-1, line, col);
    }
  }
  else
  {
    // col == 0: wrap to previous line
    if (line >= 1)
    {
      TextLine::Ptr textLine = buffer->plainLine(line-1);
      if (myWordWrap && textLine->endingWith(QString::fromLatin1(" ")))
      {
        // gg: in hard wordwrap mode, backspace must also eat the trailing space
        removeText (line-1, textLine->length()-1, line, 0);
      }
      else
        removeText (line-1, textLine->length(), line, 0);
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

  if( c.col() < (int) buffer->plainLine(c.line())->length())
  {
    removeText(c.line(), c.col(), c.line(), c.col()+1);
  }
  else
  {
    removeText(c.line(), c.col(), c.line()+1, 0);
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
  m_undoDontMerge = true;

  QString s = QApplication::clipboard()->text();

  if (s.isEmpty())
    return;

  editStart ();

  uint line = cursor.line();
  uint col = cursor.col();

  if (_configFlags & KateDocument::cfDelOnInput && hasSelection() )
  {
    removeSelectedText();
    line = view->m_viewInternal->cursorCache.line();
    col = view->m_viewInternal->cursorCache.col();
  }

  insertText( line, col, s, blockSelect );

  // anders: we want to be able to move the cursor to the
  // position at the end of the pasted text,
  // so we calculate that and applies it to c.cursor
  // This may not work, when wordwrap gets fixed :(
  TextLine::Ptr ln = buffer->plainLine( line );
  int l = s.length();
  while ( l > 0 ) {
    if ( col < ln->length() ) {
      col++;
    } else {
      line++;
      ln = buffer->plainLine( line );
      col = 0;
    }
    l--;
  }

  // editEnd will set the cursor from this cache right ;))
  // Totally breaking the whole idea of the doc view model here...
  view->m_viewInternal->cursorCache.setPos(line, col);
  view->m_viewInternal->cursorCacheChanged = true;

  editEnd();

  m_undoDontMerge = true;
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

  TextLine::Ptr textLine = buffer->plainLine(cursor.line());
  len = textLine->length();
  start = end = cursor.col();
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;
  if (end <= start) return;

  if (!(_configFlags & KateDocument::cfKeepSelection))
    clearSelection ();
  setSelection (cursor.line(), start, cursor.line(), end);
}

void KateDocument::selectLine( const KateTextCursor& cursor ) {
  if (!(_configFlags & KateDocument::cfKeepSelection))
    clearSelection ();
  setSelection (cursor.line(), 0, cursor.line()/*+1, 0*/, buffer->plainLine(cursor.line())->length() );
}

void KateDocument::selectLength( const KateTextCursor& cursor, int length ) {
  int start, end;

  TextLine::Ptr textLine = buffer->plainLine(cursor.line());
  start = cursor.col();
  end = start + length;
  if (end <= start) return;

  if (!(_configFlags & KateDocument::cfKeepSelection))
    clearSelection ();
  setSelection (cursor.line(), start, cursor.line(), end);
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
    int sl = selectStart.line();
    int el = selectEnd.line();
    int ec = selectEnd.col();

    if ((ec == 0) && ((el-1) >= 0))
    {
      el--;
    }

    if (_configFlags & KateDocument::cfKeepIndentProfile && change < 0) {
      // unindent so that the existing indent profile doesnt get screwed
      // if any line we may unindent is already full left, don't do anything
      int adjustedChange = -change;

      for (line = sl; (int) line <= el && adjustedChange > 0; line++) {
        TextLine::Ptr textLine = buffer->plainLine(line);
        int firstChar = textLine->firstChar();
        if (firstChar >= 0 && (lineSelected(line) || lineHasSelected(line))) {
          int maxUnindent = textLine->cursorX(firstChar, tabChars) / indentationChars;
          if (maxUnindent < adjustedChange)
            adjustedChange = maxUnindent;
        }
      }

      change = -adjustedChange;
    }

    for (line = sl; (int) line <= el; line++) {
      if (lineSelected(line) || lineHasSelected(line)) {
        optimizeLeadingSpace(line, _configFlags, change);
      }
    }
  }

  editEnd ();
}

/*
  Optimize the leading whitespace for a single line.
  If change is > 0, it adds indentation units (indentationChars)
  if change is == 0, it only optimizes
  If change is < 0, it removes indentation units
  This will be used to indent, unindent, and optimal-fill a line.
  If excess space is removed depends on the flag cfKeepExtraSpaces
  which has to be set by the user
*/
void KateDocument::optimizeLeadingSpace(uint line, int flags, int change)
{
  TextLine::Ptr textline = buffer->plainLine(line);

  int first_char = textline->firstChar();

  if (first_char < 0)
    first_char = ((int) textline->length()) - 1;

  if (first_char < 0)
    first_char = 0;

  int space = textline->cursorX(first_char, tabChars) + change * indentationChars;
  if (space < 0)
    space = 0;

  if (!(flags & KateDocument::cfKeepExtraSpaces))
  {
    uint extra = space % indentationChars;
    space -= extra;
    if (extra && change < 0) {
      // otherwise it unindents too much (e.g. 12 chars when indentation is 8 chars wide)
      space += indentationChars;
    }
  }

  replaceWithOptimizedSpace(line, first_char, space, flags);
}

void KateDocument::replaceWithOptimizedSpace(uint line, uint upto_column, uint space, int flags)
{
  uint length;
  QString new_space;

  if (flags & KateDocument::cfSpaceIndent) {
    length = space;
    new_space.fill(' ', length);
  }
  else {
    length = space / tabChars;
    new_space.fill('\t', length);

    QString extra_space;
    extra_space.fill(' ', space % tabChars);
    length += space % tabChars;
    new_space += extra_space;
  }

  TextLine::Ptr textline = buffer->plainLine(line);
  uint change_from;
  for (change_from = 0; change_from < upto_column && change_from < length; change_from++) {
    if (textline->getChar(change_from) != new_space[change_from])
      break;
  }

  editStart();

  if (change_from < upto_column)
    removeText(line, change_from, line, upto_column);
  if (change_from < length) {
    insertText(line, change_from, new_space.right(length - change_from));
    // FIXME: this doesn't work as intended. Is m_activeView updated at all?
    if (m_activeView) {
      m_activeView->m_viewInternal->cursorCache.setPos(line, length);
      m_activeView->m_viewInternal->cursorCacheChanged = true;
    }
  }

  editEnd();
}

/*
  Remove a given string at the begining
  of the current line.
*/
bool KateDocument::removeStringFromBegining(int line, QString &str)
{
  TextLine::Ptr textline = buffer->plainLine(line);

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
  TextLine::Ptr textline = buffer->plainLine(line);

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
  int col = buffer->plainLine(line)->length();

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

  int sl = selectStart.line();
  int el = selectEnd.line();
  int sc = selectStart.col();
  int ec = selectEnd.col();

  if ((ec == 0) && ((el-1) >= 0))
  {
    el--;
    ec = buffer->plainLine (el)->length();
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

  int sl = selectStart.line();
  int el = selectEnd.line();

  if ((selectEnd.col() == 0) && ((el-1) >= 0))
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
  selectEnd.setCol(selectEnd.col() + ((el == selectEnd.line()) ? commentLineMark.length() : 0) );
  setSelection(selectStart.line(), 0, selectEnd.line(), selectEnd.col());
}

/*
  Find the position (line and col) of the next char
  that is not a space. Return true if found.
*/
bool KateDocument::nextNonSpaceCharPos(int &line, int &col)
{
  for(; line < (int)buffer->count(); line++) {
    col = buffer->plainLine(line)->nextNonSpaceChar(col);
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
    col = buffer->plainLine(line)->previousNonSpaceChar(col);
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

  int sl = selectStart.line();
  int el = selectEnd.line();
  int sc = selectStart.col();
  int ec = selectEnd.col();

  // The selection ends on the char before selectEnd
  if (ec != 0) {
    ec--;
  } else {
    if (el > 0) {
      el--;
      ec = buffer->plainLine(el)->length() - 1;
    }
  }

  int startCommentLen = startComment.length();
  int endCommentLen = endComment.length();

  // had this been perl or sed: s/^\s*$startComment(.+?)$endComment\s*/$1/

  bool remove = nextNonSpaceCharPos(sl, sc)
      && buffer->plainLine(sl)->stringAtPos(sc, startComment)
      && previousNonSpaceCharPos(el, ec)
      && ( (ec - endCommentLen + 1) >= 0 )
      && buffer->plainLine(el)->stringAtPos(ec - endCommentLen + 1, endComment);

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

  int sl = selectStart.line();
  int el = selectEnd.line();

  if ((selectEnd.col() == 0) && ((el-1) >= 0))
  {
    el--;
  }

  // Find out how many char will be removed from the last line
  int removeLength = 0;
  if (buffer->plainLine(el)->startingWith(longCommentMark))
    removeLength = longCommentMark.length();
  else if (buffer->plainLine(el)->startingWith(shortCommentMark))
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
    selectEnd.setCol(selectEnd.col() - ((el == selectEnd.line()) ? removeLength : 0) );
    setSelection(selectStart.line(), selectStart.col(), selectEnd.line(), selectEnd.col());
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
             ( selectStart.col() > buffer->plainLine( selectStart.line() )->firstChar() ) ||
               ( selectEnd.col() < ((int)buffer->plainLine( selectEnd.line() )->length()) )
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

  TextLine::Ptr textLine = buffer->plainLine(cursor.line());
  len = textLine->length();
  start = end = cursor.col();
  if (start > len)        // Probably because of non-wrapping cursor mode.
    return QString("");

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
    if ((oldSelectStart.line() == -1 || (blockSelectionMode() && (oldSelectStart.col() != selectStart.col() || oldSelectEnd.col() != selectEnd.col())))) {
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
    && (line < selectEnd.line());
}

bool KateDocument::lineEndSelected (int line, int endCol)
{
  return (!blockSelect)
    && (line > selectStart.line() || (line == selectStart.line() && (selectStart.col() < endCol || endCol == -1)))
    && (line < selectEnd.line() || (line == selectEnd.line() && (endCol <= selectEnd.col() && endCol != -1)));
}

bool KateDocument::lineHasSelected (int line)
{
  return (selectStart < selectEnd)
    && (line >= selectStart.line())
    && (line <= selectEnd.line());
}

bool KateDocument::lineIsSelection (int line)
{
  return (line == selectStart.line() && line == selectEnd.line());
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
void KateDocument::newBracketMark( const KateTextCursor& cursor, KateTextRange& bm )
{
  bm.setValid(false);

  bm.start() = cursor;

  if( !findMatchingBracket( bm.start(), bm.end() ) )
    return;

  bm.setValid(true);

//   TextLine::Ptr textLine;
//   KateAttribute* a;

  /* Calculate starting geometry */
  /*textLine = buffer->plainLine( bm.start().line() );
  a = attribute( textLine->attribute( bm.start().col() ) );
  bm.startCol = start.col();
  bm.startLine = start.line();*/
//   bm.startX = textWidth( textLine, start.col() );
//   bm.startW = a->width( KateRenderer::getFontStruct(KateRenderer::ViewFont, textLine->getChar( start.col() ) );

  /* Calculate ending geometry */
  /*textLine = buffer->plainLine( end.line() );
  a = attribute( textLine->attribute( end.col() ) );
  bm.endCol = end.col();
  bm.endLine = end.line();*/
/*  bm.endX = textWidth( textLine, end.col() );
  bm.endW = a->width( viewFont, textLine->getChar( end.col() ) );*/
}

bool KateDocument::findMatchingBracket( KateTextCursor& start, KateTextCursor& end )
{
  TextLine::Ptr textLine = buffer->plainLine( start.line() );
  if( !textLine )
    return false;

  QChar right = textLine->getChar( start.col() );
  QChar left  = textLine->getChar( start.col() - 1 );
  QChar bracket;

  if ( _configFlags & cfOvr ) {
    if( isBracket( right ) ) {
      bracket = right;
    } else {
      return false;
    }
  } else if ( isEndBracket( left ) ) {
    start.setCol(start.col() - 1);
    bracket = left;
  } else if ( isStartBracket( right ) ) {
    bracket = right;
  } else if ( isBracket( left ) ) {
    start.setCol(start.col() - 1);
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
  int startAttr = textLine->attribute( start.col() );
  uint count = 0;
  end = start;

  while( true ) {
    /* Increment or decrement, check base cases */
    if( forward ) {
      end.setCol(end.col() + 1);
      if( end.col() >= lineLength( end.line() ) ) {
        if( end.line() >= (int)lastLine() )
          return false;
        end.setPos(end.line() + 1, 0);
        textLine = buffer->plainLine( end.line() );
      }
    } else {
      end.setCol(end.col() - 1);
      if( end.col() < 0 ) {
        if( end.line() <= 0 )
          return false;
        end.setLine(end.line() - 1);
        end.setCol(lineLength( end.line() ) - 1);
        textLine = buffer->plainLine( end.line() );
      }
    }

    /* Easy way to skip comments */
    if( textLine->attribute( end.col() ) != startAttr )
      continue;

    /* Check for match */
    QChar c = textLine->getChar( end.col() );
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
  if (fileInfo && !fileInfo->fileName().isEmpty())
  {
    fileInfo->refresh();

    if (fileInfo->lastModified() != mTime)
    {
      setMTime();

      if ( forceReload ||
           (KMessageBox::warningYesNo(0,
               (i18n("The file %1 has changed on disk.\nDo you want to reload the modified file?\n\nIf you choose Discard and subsequently save the file, you will lose those modifications.")).arg(url().fileName()),
               i18n("File has Changed on Disk"),
               i18n("Reload"),
               KStdGuiItem::discard() ) == KMessageBox::Yes)
          )
        reloadFile();
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

KateAttribute* KateDocument::attribute(uint pos)
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
    wrapText (selectStart.line(), selectEnd.line(), myWordWrapAt);
  else
    wrapText (myWordWrapAt);
}

void KateDocument::setAutoCenterLines(int viewLines)
{
  if (m_autoCenterLines != viewLines) {
    m_autoCenterLines = viewLines;
    for (uint z = 0; z < m_views.count(); z++) {
      m_views.at(z)->setAutoCenterLines(m_autoCenterLines);
    }
  }
}

int KateDocument::autoCenterLines() const
{
  return m_autoCenterLines;
}

void KateDocument::setGetSearchTextFrom (int where)
{
  m_getSearchTextFrom = where;
}

int KateDocument::getSearchTextFrom() const
{
  return m_getSearchTextFrom;
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
    for (uint z=0; z < KateFactory::documents()->count(); z++)
      KateFactory::documents()->at(z)->updateViews();
}

void KateDocument::exportAs(const QString& filter)
{
  if (filter=="kate_html_export")
  {
    QString filename=KFileDialog::getSaveFileName(QString::null,"text/html",0,i18n("Export File As"));
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
  (*outputStream) << "<span style='color: #000000'>";

  for (uint curLine=0;curLine<numLines();curLine++)
  { // html-export that line :
    TextLine::Ptr textLine = buffer->plainLine(curLine);
    //ASSERT(textLine != NULL);
    // for each character of the line : (curPos is the position in the line)
    for (uint curPos=0;curPos<textLine->length();curPos++)
    {
      KateAttribute* charAttributes = attribute(textLine->attribute(curPos));
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
        (*outputStream) << "</span>";
        // let's read that color :
        int red, green, blue;
        // getting the red, green, blue values of the color :
        charAttributes->textColor().rgb(&red, &green, &blue);
        (*outputStream) << "<span style='color: #"
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
      (*outputStream) << HTMLEncode(textLine->getChar(curPos));

      // save status for the next character :
      previousCharacterWasItalic = charAttributes->italic();
      previousCharacterWasBold = charAttributes->bold();
      previousCharacterColor = charAttributes->textColor();
      needToReinitializeTags = false;
    }
    // finish the line :
    (*outputStream) << endl;
  }

  // Be good citizens and close our tags
  if (previousCharacterWasBold)
    (*outputStream) << "</b>";
  if (previousCharacterWasItalic)
    (*outputStream) << "</i>";

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

Kate::ConfigPage *KateDocument::saveConfigPage(QWidget *p)
{
  return (Kate::ConfigPage*) new SaveConfigTab(p, this);
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

TextLine::Ptr KateDocument::kateTextLine(uint i)
{
  return buffer->line (i);
}
//END

//BEGIN KTextEditor::CursorInterface stuff

KTextEditor::Cursor *KateDocument::createCursor ( )
{
  return new KateCursor (this);
}

void KateDocument::tagArbitraryLines(KateView* view, KateSuperRange* range)
{
  if (view)
    view->m_viewInternal->tagLines(range->start(), range->end());
  else
    tagLines(range->start(), range->end());
}

//
// Spellchecking IN again
//
void KateDocument::spellcheck()
{
  if( !isReadWrite() )
    return;

  m_kspell = new KSpell( 0, i18n("Spellcheck"),
                         this, SLOT(ready()) );

  connect( m_kspell, SIGNAL(death()),
           this, SLOT(spellCleanDone()) );

  connect( m_kspell, SIGNAL(misspelling(const QString&, const QStringList&, unsigned int)),
           this, SLOT(misspelling(const QString&, const QStringList&, unsigned int)) );
  connect( m_kspell, SIGNAL(corrected(const QString&, const QString&, unsigned int)),
           this, SLOT(corrected(const QString&, const QString&, unsigned int)) );
  connect( m_kspell, SIGNAL(done(const QString&)),
           this, SLOT(spellResult(const QString&)) );
}

void KateDocument::ready()
{
  setReadWrite( false );

  m_mispellCount = 0;
  m_replaceCount = 0;

  m_kspell->setProgressResolution( 1 );

  m_kspell->check( text() );
}

void KateDocument::locatePosition( uint pos, uint& line, uint& col )
{
  uint cnt = 0;

  line = col = 0;

  // Find pos  -- CHANGEME: store the last found pos's cursor
  //   and do these searched relative to that to
  //   (significantly) increase the speed of the spellcheck
  for( ; line < numLines() && cnt <= pos; line++ )
    cnt += lineLength(line) + 1;

  line--;
  col = pos - (cnt - lineLength(line)) + 1;
}

void KateDocument::misspelling( const QString& origword, const QStringList&, uint pos )
{
  m_mispellCount++;

  uint line, col;

  locatePosition( pos, line, col );

  if (activeView())
    activeView()->m_viewInternal->updateCursor( KateTextCursor( line, col ) );

  setSelection( line, col, line, col + origword.length() );
}

void KateDocument::corrected( const QString& originalword, const QString& newword, uint pos )
{
  m_replaceCount++;

  uint line, col;

  locatePosition( pos, line, col );

  removeText( line, col, line, col + originalword.length() );
  insertText( line, col, newword );
}

void KateDocument::spellResult( const QString& )
{
  clearSelection();
  setReadWrite( true );
  m_kspell->cleanUp();
}

void KateDocument::spellCleanDone()
{
  KSpell::spellStatus status = m_kspell->status();

  if( status == KSpell::Error ) {
    KMessageBox::sorry( 0,
      i18n("ISpell could not be started. "
           "Please make sure you have ISpell "
           "properly configured and in your PATH."));
  } else if( status == KSpell::Crashed ) {
    setReadWrite( true );
    KMessageBox::sorry( 0,
      i18n("ISpell seems to have crashed."));
  }

  delete m_kspell;
  m_kspell = 0;
}
//END

void KateDocument::lineInfo (KateLineInfo *info, unsigned int line)
{
  buffer->lineInfo(info,line);
}

KateCodeFoldingTree *KateDocument::foldingTree ()
{
  return buffer->foldingTree();
}
