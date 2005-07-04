/* This file is part of the KDE libraries
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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02111-13020, USA.
*/

//BEGIN includes
#include "katedocument.h"
#include "katedocument.moc"
#include "katekeyinterceptorfunctor.h"
#include "katefactory.h"
#include "katedialogs.h"
#include "katehighlight.h"
#include "kateview.h"
#include "katesearch.h"
#include "kateautoindent.h"
#include "katetextline.h"
#include "katedocumenthelpers.h"
#include "kateprinter.h"
#include "katelinerange.h"
#include "katesupercursor.h"
#include "katearbitraryhighlight.h"
#include "katerenderer.h"
#include "kateattribute.h"
#include "kateconfig.h"
#include "katefiletype.h"
#include "kateschema.h"
#include "katetemplatehandler.h"
#include <ktexteditor/plugin.h>

#include <kio/job.h>
#include <kio/netaccess.h>
#include <kio/kfileitem.h>


#include <kparts/event.h>

#include <klocale.h>
#include <kglobal.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <kiconloader.h>
#include <kxmlguifactory.h>
#include <kdialogbase.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <klibloader.h>
#include <kdirwatch.h>
#include <kwin.h>
#include <kencodingfiledialog.h>
#include <ktempfile.h>
#include <kmdcodec.h>

#include <qtimer.h>
#include <qfile.h>
#include <qclipboard.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qmap.h>
//END  includes

//BEGIN PRIVATE CLASSES
class KatePartPluginItem
{
  public:
    KTextEditor::Plugin *plugin;
};
//END PRIVATE CLASSES

//BEGIN d'tor, c'tor
//
// KateDocument Constructor
//
KateDocument::KateDocument ( bool bSingleViewMode, bool bBrowserView,
                             bool bReadOnly, QWidget *parentWidget,
                             const char *widgetName, QObject *parent, const char *name)
: Kate::Document(parent, name),
  m_plugins (KateFactory::self()->plugins().count()),
  m_undoDontMerge(false),
  m_undoIgnoreCancel(false),
  lastUndoGroupWhenSaved( 0 ),
  docWasSavedWhenUndoWasEmpty( true ),
  m_modOnHd (false),
  m_modOnHdReason (0),
  m_job (0),
  m_tempFile (0),
  m_tabInterceptor(0)
{
  m_undoComplexMerge=false;
  // my dcop object
  setObjId ("KateDocument#"+documentDCOPSuffix());

  // ktexteditor interfaces
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

  // init local plugin array
  m_plugins.fill (0);

  // register doc at factory
  KateFactory::self()->registerDocument (this);

  m_reloading = false;

  m_buffer = new KateBuffer (this);

  // init the config object, be careful not to use it
  // until the initial readConfig() call is done
  m_config = new KateDocumentConfig (this);

  // init some more vars !
  m_activeView = 0L;

  hlSetByUser = false;
  m_fileType = -1;
  m_fileTypeSetByUser = false;
  setInstance( KateFactory::self()->instance() );

  editSessionNumber = 0;
  editIsRunning = false;
  m_editCurrentUndo = 0L;
  editWithUndo = false;

  m_docNameNumber = 0;

  m_bSingleViewMode = bSingleViewMode;
  m_bBrowserView = bBrowserView;
  m_bReadOnly = bReadOnly;

  m_marks.setAutoDelete( true );
  m_markPixmaps.setAutoDelete( true );
  m_markDescriptions.setAutoDelete( true );
  setMarksUserChangable( markType01 );

  m_undoMergeTimer = new QTimer(this);
  connect(m_undoMergeTimer, SIGNAL(timeout()), SLOT(undoCancel()));

  clearMarks ();
  clearUndo ();
  clearRedo ();
  setModified (false);
  docWasSavedWhenUndoWasEmpty = true;

  // normal hl
  m_buffer->setHighlight (0);

  m_extension = new KateBrowserExtension( this );
  m_arbitraryHL = new KateArbitraryHighlight();
  m_indenter = KateAutoIndent::createIndenter ( this, 0 );

  m_indenter->updateConfig ();

  // some nice signals from the buffer
  connect(m_buffer, SIGNAL(tagLines(int,int)), this, SLOT(tagLines(int,int)));
  connect(m_buffer, SIGNAL(codeFoldingUpdated()),this,SIGNAL(codeFoldingUpdated()));

  // if the user changes the highlight with the dialog, notify the doc
  connect(KateHlManager::self(),SIGNAL(changed()),SLOT(internalHlChanged()));

  // signal for the arbitrary HL
  connect(m_arbitraryHL, SIGNAL(tagLines(KateView*, KateSuperRange*)), SLOT(tagArbitraryLines(KateView*, KateSuperRange*)));

  // signals for mod on hd
  connect( KateFactory::self()->dirWatch(), SIGNAL(dirty (const QString &)),
           this, SLOT(slotModOnHdDirty (const QString &)) );

  connect( KateFactory::self()->dirWatch(), SIGNAL(created (const QString &)),
           this, SLOT(slotModOnHdCreated (const QString &)) );

  connect( KateFactory::self()->dirWatch(), SIGNAL(deleted (const QString &)),
           this, SLOT(slotModOnHdDeleted (const QString &)) );

  // update doc name
  setDocName ("");

  // if single view mode, like in the konqui embedding, create a default view ;)
  if ( m_bSingleViewMode )
  {
    KTextEditor::View *view = createView( parentWidget, widgetName );
    insertChildClient( view );
    view->show();
    setWidget( view );
  }

  connect(this,SIGNAL(sigQueryClose(bool *, bool*)),this,SLOT(slotQueryClose_save(bool *, bool*)));

  m_isasking = 0;

  // plugins
  for (uint i=0; i<KateFactory::self()->plugins().count(); i++)
  {
    if (config()->plugin (i))
      loadPlugin (i);
  }
}

//
// KateDocument Destructor
//
KateDocument::~KateDocument()
{
  // remove file from dirwatch
  deactivateDirWatch ();

  if (!singleViewMode())
  {
    // clean up remaining views
    m_views.setAutoDelete( true );
    m_views.clear();
  }

  delete m_editCurrentUndo;

  delete m_arbitraryHL;

  // cleanup the undo items, very important, truee :/
  undoItems.setAutoDelete(true);
  undoItems.clear();

  // clean up plugins
  unloadAllPlugins ();

  delete m_config;
  delete m_indenter;
  KateFactory::self()->deregisterDocument (this);
}
//END

//BEGIN Plugins
void KateDocument::unloadAllPlugins ()
{
  for (uint i=0; i<m_plugins.count(); i++)
    unloadPlugin (i);
}

void KateDocument::enableAllPluginsGUI (KateView *view)
{
  for (uint i=0; i<m_plugins.count(); i++)
    enablePluginGUI (m_plugins[i], view);
}

void KateDocument::disableAllPluginsGUI (KateView *view)
{
  for (uint i=0; i<m_plugins.count(); i++)
    disablePluginGUI (m_plugins[i], view);
}

void KateDocument::loadPlugin (uint pluginIndex)
{
  if (m_plugins[pluginIndex]) return;

  m_plugins[pluginIndex] = KTextEditor::createPlugin (QFile::encodeName((KateFactory::self()->plugins())[pluginIndex]->library()), this);

  enablePluginGUI (m_plugins[pluginIndex]);
}

void KateDocument::unloadPlugin (uint pluginIndex)
{
  if (!m_plugins[pluginIndex]) return;

  disablePluginGUI (m_plugins[pluginIndex]);

  delete m_plugins[pluginIndex];
  m_plugins[pluginIndex] = 0L;
}

void KateDocument::enablePluginGUI (KTextEditor::Plugin *plugin, KateView *view)
{
  if (!plugin) return;
  if (!KTextEditor::pluginViewInterface(plugin)) return;

  KXMLGUIFactory *factory = view->factory();
  if ( factory )
    factory->removeClient( view );

  KTextEditor::pluginViewInterface(plugin)->addView(view);

  if ( factory )
    factory->addClient( view );
}

void KateDocument::enablePluginGUI (KTextEditor::Plugin *plugin)
{
  if (!plugin) return;
  if (!KTextEditor::pluginViewInterface(plugin)) return;

  for (uint i=0; i< m_views.count(); i++)
    enablePluginGUI (plugin, m_views.at(i));
}

void KateDocument::disablePluginGUI (KTextEditor::Plugin *plugin, KateView *view)
{
  if (!plugin) return;
  if (!KTextEditor::pluginViewInterface(plugin)) return;

  KXMLGUIFactory *factory = view->factory();
  if ( factory )
    factory->removeClient( view );

  KTextEditor::pluginViewInterface( plugin )->removeView( view );

  if ( factory )
    factory->addClient( view );
}

void KateDocument::disablePluginGUI (KTextEditor::Plugin *plugin)
{
  if (!plugin) return;
  if (!KTextEditor::pluginViewInterface(plugin)) return;

  for (uint i=0; i< m_views.count(); i++)
    disablePluginGUI (plugin, m_views.at(i));
}
//END

//BEGIN KTextEditor::Document stuff

KTextEditor::View *KateDocument::createView( QWidget *parent, const char *name )
{
  KateView* newView = new KateView( this, parent, name);
  connect(newView, SIGNAL(cursorPositionChanged()), SLOT(undoCancel()));
  if ( s_fileChangedDialogsActivated )
    connect( newView, SIGNAL(gotFocus( Kate::View * )), this, SLOT(slotModifiedOnDisk()) );
  return newView;
}

QPtrList<KTextEditor::View> KateDocument::views () const
{
  return m_textEditViews;
}

void KateDocument::setActiveView( KateView *view )
{
  if ( m_activeView == view ) return;

  m_activeView = view;
}
//END

//BEGIN KTextEditor::ConfigInterfaceExtension stuff

uint KateDocument::configPages () const
{
  return 10;
}

KTextEditor::ConfigPage *KateDocument::configPage (uint number, QWidget *parent, const char * )
{
  switch( number )
  {
    case 0:
      return new KateViewDefaultsConfig (parent);

    case 1:
      return new KateSchemaConfigPage (parent, this);

    case 2:
      return new KateSelectConfigTab (parent);

    case 3:
      return new KateEditConfigTab (parent);

    case 4:
      return new KateIndentConfigTab (parent);

    case 5:
      return new KateSaveConfigTab (parent);

    case 6:
      return new KateHlConfigPage (parent);

    case 7:
      return new KateFileTypeConfigTab (parent);

    case 8:
      return new KateEditKeyConfiguration (parent, this);

    case 9:
      return new KatePartPluginConfigPage (parent);

    default:
      return 0;
  }

  return 0;
}

QString KateDocument::configPageName (uint number) const
{
  switch( number )
  {
    case 0:
      return i18n ("Appearance");

    case 1:
      return i18n ("Fonts & Colors");

    case 2:
      return i18n ("Cursor & Selection");

    case 3:
      return i18n ("Editing");

    case 4:
      return i18n ("Indentation");

    case 5:
      return i18n("Open/Save");

    case 6:
      return i18n ("Highlighting");

    case 7:
      return i18n("Filetypes");

    case 8:
      return i18n ("Shortcuts");

    case 9:
      return i18n ("Plugins");

    default:
      return QString ("");
  }

  return QString ("");
}

QString KateDocument::configPageFullName (uint number) const
{
  switch( number )
  {
    case 0:
      return i18n("Appearance");

    case 1:
      return i18n ("Font & Color Schemas");

    case 2:
      return i18n ("Cursor & Selection Behavior");

    case 3:
      return i18n ("Editing Options");

    case 4:
      return i18n ("Indentation Rules");

    case 5:
      return i18n("File Opening & Saving");

    case 6:
      return i18n ("Highlighting Rules");

    case 7:
      return i18n("Filetype Specific Settings");

    case 8:
      return i18n ("Shortcuts Configuration");

    case 9:
      return i18n ("Plugin Manager");

    default:
      return QString ("");
  }

  return QString ("");
}

QPixmap KateDocument::configPagePixmap (uint number, int size) const
{
  switch( number )
  {
    case 0:
      return BarIcon("view_text",size);

    case 1:
      return BarIcon("colorize", size);

    case 2:
        return BarIcon("frame_edit", size);

    case 3:
      return BarIcon("edit", size);

    case 4:
      return BarIcon("rightjust", size);

    case 5:
      return BarIcon("filesave", size);

    case 6:
      return BarIcon("source", size);

    case 7:
      return BarIcon("edit", size);

    case 8:
      return BarIcon("key_enter", size);

    case 9:
      return BarIcon("connect_established", size);

    default:
      return BarIcon("edit", size);
  }

  return BarIcon("edit", size);
}
//END

//BEGIN KTextEditor::EditInterface stuff

QString KateDocument::text() const
{
  QString s;

  for (uint i = 0; i < m_buffer->count(); i++)
  {
    KateTextLine::Ptr textLine = m_buffer->plainLine(i);

    if (textLine)
    {
      s.append (textLine->string());

      if ((i+1) < m_buffer->count())
        s.append('\n');
    }
  }

  return s;
}

QString KateDocument::text ( uint startLine, uint startCol, uint endLine, uint endCol ) const
{
  return text(startLine, startCol, endLine, endCol, false);
}

QString KateDocument::text ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise) const
{
  if ( blockwise && (startCol > endCol) )
    return QString ();

  QString s;

  if (startLine == endLine)
  {
    if (startCol > endCol)
      return QString ();

    KateTextLine::Ptr textLine = m_buffer->plainLine(startLine);

    if ( !textLine )
      return QString ();

    return textLine->string(startCol, endCol-startCol);
  }
  else
  {

    for (uint i = startLine; (i <= endLine) && (i < m_buffer->count()); i++)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(i);

      if ( !blockwise )
      {
        if (i == startLine)
          s.append (textLine->string(startCol, textLine->length()-startCol));
        else if (i == endLine)
          s.append (textLine->string(0, endCol));
        else
          s.append (textLine->string());
      }
      else
      {
        s.append( textLine->string( startCol, endCol-startCol));
      }

      if ( i < endLine )
        s.append('\n');
    }
  }

  return s;
}

QString KateDocument::textLine( uint line ) const
{
  KateTextLine::Ptr l = m_buffer->plainLine(line);

  if (!l)
    return QString();

  return l->string();
}

bool KateDocument::setText(const QString &s)
{
  if (!isReadWrite())
    return false;

  QPtrList<KTextEditor::Mark> m = marks ();
  QValueList<KTextEditor::Mark> msave;

  for (uint i=0; i < m.count(); i++)
    msave.append (*m.at(i));

  editStart ();

  // delete the text
  clear();

  // insert the new text
  insertText (0, 0, s);

  editEnd ();

  for (uint i=0; i < msave.count(); i++)
    setMark (msave[i].line, msave[i].type);

  return true;
}

bool KateDocument::clear()
{
  if (!isReadWrite())
    return false;

  for (KateView * view = m_views.first(); view != 0L; view = m_views.next() ) {
    view->clear();
    view->tagAll();
    view->update();
  }

  clearMarks ();

  return removeText (0,0,lastLine()+1, 0);
}

bool KateDocument::insertText( uint line, uint col, const QString &s)
{
  return insertText (line, col, s, false);
}

bool KateDocument::insertText( uint line, uint col, const QString &s, bool blockwise )
{
  if (!isReadWrite())
    return false;

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

  bool replacetabs = ( config()->configFlags() & KateDocumentConfig::cfReplaceTabsDyn );
  uint tw = config()->tabWidth();

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
    {
      if ( replacetabs && ch == '\t' )
      {
        uint tr = tw - ( ((blockwise?col:insertPos)+buf.length())%tw ); //###
        for ( uint i=0; i < tr; i++ )
          buf += ' ';
      }
      else
        buf += ch; // append char to buffer
    }
  }

  if ( !blockwise )
    editInsertText (line, insertPos, buf);
  else
    editInsertText (line, col, buf);

  editEnd ();
  emit textInserted(line,insertPos);
  return true;
}

bool KateDocument::removeText ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  return removeText (startLine, startCol, endLine, endCol, false);
}

bool KateDocument::removeText ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise)
{
  if (!isReadWrite())
    return false;

  if ( blockwise && (startCol > endCol) )
    return false;

  if ( startLine > endLine )
    return false;

  if ( startLine > lastLine() )
    return false;

  if (!blockwise) {
    emit aboutToRemoveText(KateTextRange(startLine,startCol,endLine,endCol));
  }
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
      if ( (m_buffer->plainLine(startLine)->length()-startCol) > 0 )
        editRemoveText (startLine, startCol, m_buffer->plainLine(startLine)->length()-startCol);

      editRemoveText (startLine+1, 0, endCol);
      editUnWrapLine (startLine);
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
            if ( (m_buffer->plainLine(line)->length()-startCol) > 0 )
              editRemoveText (line, startCol, m_buffer->plainLine(line)->length()-startCol);

            editUnWrapLine (startLine);
          }
        }

        if ( line == 0 )
          break;
      }
    }
  } // if ( ! blockwise )
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
  emit textRemoved();
  return true;
}

bool KateDocument::insertLine( uint l, const QString &str )
{
  if (!isReadWrite())
    return false;

  if (l > numLines())
    return false;

  return editInsertLine (l, str);
}

bool KateDocument::removeLine( uint line )
{
  if (!isReadWrite())
    return false;

  if (line > lastLine())
    return false;

  return editRemoveLine (line);
}

uint KateDocument::length() const
{
  uint l = 0;

  for (uint i = 0; i < m_buffer->count(); i++)
  {
    KateTextLine::Ptr line = m_buffer->plainLine(i);

    if (line)
      l += line->length();
  }

  return l;
}

uint KateDocument::numLines() const
{
  return m_buffer->count();
}

uint KateDocument::numVisLines() const
{
  return m_buffer->countVisible ();
}

int KateDocument::lineLength ( uint line ) const
{
  KateTextLine::Ptr l = m_buffer->plainLine(line);

  if (!l)
    return -1;

  return l->length();
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

  editIsRunning = true;
  editWithUndo = withUndo;

  if (editWithUndo)
    undoStart();
  else
    undoCancel();

  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->editStart ();
  }

  m_buffer->editStart ();
}

void KateDocument::undoStart()
{
  if (m_editCurrentUndo || (m_activeView && m_activeView->imComposeEvent())) return;

  // Make sure the buffer doesn't get bigger than requested
  if ((config()->undoSteps() > 0) && (undoItems.count() > config()->undoSteps()))
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
  if (m_activeView && m_activeView->imComposeEvent())
    return;

  if (m_editCurrentUndo)
  {
    bool changedUndo = false;

    if (m_editCurrentUndo->isEmpty())
      delete m_editCurrentUndo;
    else if (!m_undoDontMerge && undoItems.last() && undoItems.last()->merge(m_editCurrentUndo,m_undoComplexMerge))
      delete m_editCurrentUndo;
    else
    {
      undoItems.append(m_editCurrentUndo);
      changedUndo = true;
    }

    m_undoDontMerge = false;
    m_undoIgnoreCancel = true;

    m_editCurrentUndo = 0L;

    // (Re)Start the single-shot timer to cancel the undo merge
    // the user has 5 seconds to input more data, or undo merging gets canceled for the current undo item.
    m_undoMergeTimer->start(5000, true);

    if (changedUndo)
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

void KateDocument::undoSafePoint() {
  Q_ASSERT(m_editCurrentUndo);
  if (!m_editCurrentUndo) return;
  m_editCurrentUndo->safePoint();
}

//
// End edit session and update Views
//
void KateDocument::editEnd ()
{
  if (editSessionNumber == 0)
    return;

  // wrap the new/changed text, if something really changed!
  if (m_buffer->editChanged() && (editSessionNumber == 1))
    if (editWithUndo && config()->wordWrap())
      wrapText (m_buffer->editTagStart(), m_buffer->editTagEnd());

  editSessionNumber--;

  if (editSessionNumber > 0)
    return;

  // end buffer edit, will trigger hl update
  // this will cause some possible adjustment of tagline start/end
  m_buffer->editEnd ();

  if (editWithUndo)
    undoEnd();

  // edit end for all views !!!!!!!!!
  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->editEnd (m_buffer->editTagStart(), m_buffer->editTagEnd(), m_buffer->editTagFrom());

  if (m_buffer->editChanged())
  {
    setModified(true);
    emit textChanged ();
  }

  editIsRunning = false;
}

bool KateDocument::wrapText (uint startLine, uint endLine)
{
  uint col = config()->wordWrapAt();

  if (col == 0)
    return false;

  editStart ();

  for (uint line = startLine; (line <= endLine) && (line < numLines()); line++)
  {
    KateTextLine::Ptr l = m_buffer->line(line);

    if (!l)
      return false;

    kdDebug (13020) << "try wrap line: " << line << endl;

    if (l->lengthWithTabs(m_buffer->tabWidth()) > col)
    {
      KateTextLine::Ptr nextl = m_buffer->line(line+1);

      kdDebug (13020) << "do wrap line: " << line << endl;

      const QChar *text = l->text();
      uint eolPosition = l->length()-1;

      // take tabs into account here, too
      uint x = 0;
      const QString & t = l->string();
      uint z2 = 0;
      for ( ; z2 < l->length(); z2++)
      {
        if (t[z2] == QChar('\t'))
          x += m_buffer->tabWidth() - (x % m_buffer->tabWidth());
        else
          x++;

        if (x > col)
          break;
      }

      uint searchStart = KMIN (z2, l->length()-1);

      // If where we are wrapping is an end of line and is a space we don't
      // want to wrap there
      if (searchStart == eolPosition && text[searchStart].isSpace())
        searchStart--;

      // Scan backwards looking for a place to break the line
      // We are not interested in breaking at the first char
      // of the line (if it is a space), but we are at the second
      // anders: if we can't find a space, try breaking on a word
      // boundry, using KateHighlight::canBreakAt().
      // This could be a priority (setting) in the hl/filetype/document
      int z = 0;
      uint nw = 0; // alternative position, a non word character
      for (z=searchStart; z > 0; z--)
      {
        if (text[z].isSpace()) break;
        if ( ! nw && highlight()->canBreakAt( text[z] , l->attribute(z) ) )
        nw = z;
      }

      if (z > 0)
      {
        // cu space
        editRemoveText (line, z, 1);
      }
      else
      {
        // There was no space to break at so break at a nonword character if
        // found, or at the wrapcolumn ( that needs be configurable )
        // Don't try and add any white space for the break
        if ( nw && nw < col ) nw++; // break on the right side of the character
        z = nw ? nw : col;
      }

      if (nextl && !nextl->isAutoWrapped())
      {
        editWrapLine (line, z, true);
        editMarkLineAutoWrapped (line+1, true);

        endLine++;
      }
      else
      {
        if (nextl && (nextl->length() > 0) && !nextl->getChar(0).isSpace() && ((l->length() < 1) || !l->getChar(l->length()-1).isSpace()))
          editInsertText (line+1, 0, QString (" "));

        bool newLineAdded = false;
        editWrapLine (line, z, false, &newLineAdded);

        editMarkLineAutoWrapped (line+1, true);

        endLine++;
      }
    }
  }

  editEnd ();

  return true;
}

void KateDocument::editAddUndo (KateUndoGroup::UndoType type, uint line, uint col, uint len, const QString &text)
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

bool KateDocument::editInsertText ( uint line, uint col, const QString &str )
{
  if (!isReadWrite())
    return false;

  QString s = str;

  KateTextLine::Ptr l = m_buffer->line(line);

  if (!l)
    return false;

    if ( config()->configFlags() & KateDocumentConfig::cfReplaceTabsDyn )
    {
      uint tw = config()->tabWidth();
      int pos = 0;
      uint l = 0;
      while ( (pos = s.find('\t')) > -1 )
      {
        l = tw - ( (col + pos)%tw );
        s.replace( pos, 1, QString().fill( ' ', l ) );
      }
    }

  editStart ();

  editAddUndo (KateUndoGroup::editInsertText, line, col, s.length(), s);

  l->insertText (col, s.length(), s.unicode());
//   removeTrailingSpace(line); // ### nessecary?

  m_buffer->changeLine(line);

  for( QPtrListIterator<KateSuperCursor> it (m_superCursors); it.current(); ++it )
    it.current()->editTextInserted (line, col, s.length());

  editEnd ();

  return true;
}

bool KateDocument::editRemoveText ( uint line, uint col, uint len )
{
  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editRemoveText, line, col, len, l->string().mid(col, len));

  l->removeText (col, len);
  removeTrailingSpace( line );

  m_buffer->changeLine(line);

  for( QPtrListIterator<KateSuperCursor> it (m_superCursors); it.current(); ++it )
    it.current()->editTextRemoved (line, col, len);

  editEnd ();

  return true;
}

bool KateDocument::editMarkLineAutoWrapped ( uint line, bool autowrapped )
{
  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editMarkLineAutoWrapped, line, autowrapped ? 1 : 0, 0, QString::null);

  l->setAutoWrapped (autowrapped);

  m_buffer->changeLine(line);

  editEnd ();

  return true;
}

bool KateDocument::editWrapLine ( uint line, uint col, bool newLine, bool *newLineAdded)
{
  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);

  if (!l)
    return false;

  editStart ();

  KateTextLine::Ptr nextLine = m_buffer->line(line+1);

  int pos = l->length() - col;

  if (pos < 0)
    pos = 0;

  editAddUndo (KateUndoGroup::editWrapLine, line, col, pos, (!nextLine || newLine) ? "1" : "0");

  if (!nextLine || newLine)
  {
    KateTextLine::Ptr textLine = new KateTextLine();

    textLine->insertText (0, pos, l->text()+col, l->attributes()+col);
    l->truncate(col);

    m_buffer->insertLine (line+1, textLine);
    m_buffer->changeLine(line);

    QPtrList<KTextEditor::Mark> list;
    for( QIntDictIterator<KTextEditor::Mark> it( m_marks ); it.current(); ++it )
    {
      if( it.current()->line >= line )
      {
        if ((col == 0) || (it.current()->line > line))
          list.append( it.current() );
      }
    }

    for( QPtrListIterator<KTextEditor::Mark> it( list ); it.current(); ++it )
    {
      KTextEditor::Mark* mark = m_marks.take( it.current()->line );
      mark->line++;
      m_marks.insert( mark->line, mark );
    }

    if( !list.isEmpty() )
      emit marksChanged();

    // yes, we added a new line !
    if (newLineAdded)
      (*newLineAdded) = true;
  }
  else
  {
    nextLine->insertText (0, pos, l->text()+col, l->attributes()+col);
    l->truncate(col);

    m_buffer->changeLine(line);
    m_buffer->changeLine(line+1);

    // no, no new line added !
    if (newLineAdded)
      (*newLineAdded) = false;
  }

  for( QPtrListIterator<KateSuperCursor> it (m_superCursors); it.current(); ++it )
    it.current()->editLineWrapped (line, col, !nextLine || newLine);

  editEnd ();

  return true;
}

bool KateDocument::editUnWrapLine ( uint line, bool removeLine, uint length )
{
  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);
  KateTextLine::Ptr nextLine = m_buffer->line(line+1);

  if (!l || !nextLine)
    return false;

  editStart ();

  uint col = l->length ();

  editAddUndo (KateUndoGroup::editUnWrapLine, line, col, length, removeLine ? "1" : "0");

  if (removeLine)
  {
    l->insertText (col, nextLine->length(), nextLine->text(), nextLine->attributes());

    m_buffer->changeLine(line);
    m_buffer->removeLine(line+1);
  }
  else
  {
    l->insertText (col, (nextLine->length() < length) ? nextLine->length() : length,
      nextLine->text(), nextLine->attributes());
    nextLine->removeText (0, (nextLine->length() < length) ? nextLine->length() : length);

    m_buffer->changeLine(line);
    m_buffer->changeLine(line+1);
  }

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

  for( QPtrListIterator<KateSuperCursor> it (m_superCursors); it.current(); ++it )
    it.current()->editLineUnWrapped (line, col, removeLine, length);

  editEnd ();

  return true;
}

bool KateDocument::editInsertLine ( uint line, const QString &s )
{
  if (!isReadWrite())
    return false;

  if ( line > numLines() )
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editInsertLine, line, 0, s.length(), s);

  removeTrailingSpace( line ); // old line

  KateTextLine::Ptr tl = new KateTextLine();
  tl->insertText (0, s.length(), s.unicode(), 0);
  m_buffer->insertLine(line, tl);
  m_buffer->changeLine(line);

  removeTrailingSpace( line ); // new line

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

  for( QPtrListIterator<KateSuperCursor> it (m_superCursors); it.current(); ++it )
    it.current()->editLineInserted (line);

  editEnd ();

  return true;
}

bool KateDocument::editRemoveLine ( uint line )
{
  if (!isReadWrite())
    return false;

  if ( line > lastLine() )
    return false;

  if ( numLines() == 1 )
    return editRemoveText (0, 0, m_buffer->line(0)->length());

  editStart ();

  editAddUndo (KateUndoGroup::editRemoveLine, line, 0, lineLength(line), textLine(line));

  m_buffer->removeLine(line);

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

  for( QPtrListIterator<KateSuperCursor> it (m_superCursors); it.current(); ++it )
    it.current()->editLineRemoved (line);

  editEnd();

  return true;
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
  return m_config->undoSteps();
}

void KateDocument::setUndoSteps(uint steps)
{
  m_config->setUndoSteps (steps);
}

void KateDocument::undo()
{
  if ((undoItems.count() > 0) && undoItems.last())
  {
    clearSelection ();

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
    clearSelection ();

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
       || ( undoItems.isEmpty() && docWasSavedWhenUndoWasEmpty ) )
  {
    setModified( false );
    kdDebug(13020) << k_funcinfo << "setting modified to false!" << endl;
  };
}

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
      KateTextLine::Ptr textLine = m_buffer->plainLine(line);

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
      KateTextLine::Ptr textLine = m_buffer->plainLine(line);

      if (!textLine)
        return false;

      uint foundAt, myMatchLen;
      bool found = textLine->searchText (col, text, &foundAt, &myMatchLen, casesensitive, true);

      if (found)
      {
       /* if ((uint) line == startLine && foundAt + myMatchLen >= (uint) col
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
      }*/

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
  kdDebug(13020)<<"KateDocument::searchText( "<<startLine<<", "<<startCol<<", "<<regexp.pattern()<<", "<<backwards<<" )"<<endl;
  if (regexp.isEmpty() || !regexp.isValid())
    return false;

  int line = startLine;
  int col = startCol;

  if (!backwards)
  {
    int searchEnd = lastLine();

    while (line <= searchEnd)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(line);

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
      KateTextLine::Ptr textLine = m_buffer->plainLine(line);

      if (!textLine)
        return false;

      uint foundAt, myMatchLen;
      bool found = textLine->searchText (col, regexp, &foundAt, &myMatchLen, true);

      if (found)
      {
        /*if ((uint) line == startLine && foundAt + myMatchLen >= (uint) col
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
      }*/

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
  return KateHlManager::self()->findHl(highlight());
}

bool KateDocument::setHlMode (uint mode)
{
  m_buffer->setHighlight (mode);

  if (true)
  {
    setDontChangeHlOnSave();
    return true;
  }

  return false;
}

void KateDocument::bufferHlChanged ()
{
  // update all views
  makeAttribs(false);

  emit hlChanged();
}

uint KateDocument::hlModeCount ()
{
  return KateHlManager::self()->highlights();
}

QString KateDocument::hlModeName (uint mode)
{
  return KateHlManager::self()->hlName (mode);
}

QString KateDocument::hlModeSectionName (uint mode)
{
  return KateHlManager::self()->hlSection (mode);
}

void KateDocument::setDontChangeHlOnSave()
{
  hlSetByUser = true;
}
//END

//BEGIN KTextEditor::ConfigInterface stuff
void KateDocument::readConfig(KConfig *config)
{
  config->setGroup("Kate Document Defaults");

  // read max loadable blocks, more blocks will be swapped out
  KateBuffer::setMaxLoadedBlocks (config->readNumEntry("Maximal Loaded Blocks", KateBuffer::maxLoadedBlocks()));

  KateDocumentConfig::global()->readConfig (config);

  config->setGroup("Kate View Defaults");
  KateViewConfig::global()->readConfig (config);

  config->setGroup("Kate Renderer Defaults");
  KateRendererConfig::global()->readConfig (config);
}

void KateDocument::writeConfig(KConfig *config)
{
  config->setGroup("Kate Document Defaults");

  // write max loadable blocks, more blocks will be swapped out
  config->writeEntry("Maximal Loaded Blocks", KateBuffer::maxLoadedBlocks());

  KateDocumentConfig::global()->writeConfig (config);

  config->setGroup("Kate View Defaults");
  KateViewConfig::global()->writeConfig (config);

  config->setGroup("Kate Renderer Defaults");
  KateRendererConfig::global()->writeConfig (config);
}

void KateDocument::readConfig()
{
  KConfig *config = kapp->config();
  readConfig (config);
}

void KateDocument::writeConfig()
{
  KConfig *config = kapp->config();
  writeConfig (config);
  config->sync();
}

void KateDocument::readSessionConfig(KConfig *kconfig)
{
  // restore the url
  KURL url (kconfig->readEntry("URL"));

  // get the encoding
  QString tmpenc=kconfig->readEntry("Encoding");
  if (!tmpenc.isEmpty() && (tmpenc != encoding()))
    setEncoding(tmpenc);

  // open the file if url valid
  if (!url.isEmpty() && url.isValid())
    openURL (url);

  // restore the hl stuff
  m_buffer->setHighlight(KateHlManager::self()->nameFind(kconfig->readEntry("Highlighting")));

  if (hlMode() > 0)
    hlSetByUser = true;

  // indent mode
  config()->setIndentationMode( (uint)kconfig->readNumEntry("Indentation Mode", config()->indentationMode() ) );

  // Restore Bookmarks
  QValueList<int> marks = kconfig->readIntListEntry("Bookmarks");
  for( uint i = 0; i < marks.count(); i++ )
    addMark( marks[i], KateDocument::markType01 );
}

void KateDocument::writeSessionConfig(KConfig *kconfig)
{
  // save url
  kconfig->writeEntry("URL", m_url.prettyURL() );

  // save encoding
  kconfig->writeEntry("Encoding",encoding());

  // save hl
  kconfig->writeEntry("Highlighting", highlight()->name());

  kconfig->writeEntry("Indentation Mode", config()->indentationMode() );

  // Save Bookmarks
  QValueList<int> marks;
  for( QIntDictIterator<KTextEditor::Mark> it( m_marks );
       it.current() && it.current()->type & KTextEditor::MarkInterface::markType01;
       ++it )
     marks << it.current()->line;

  kconfig->writeEntry( "Bookmarks", marks );
}

void KateDocument::configDialog()
{
  KDialogBase *kd = new KDialogBase ( KDialogBase::IconList,
                                      i18n("Configure"),
                                      KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Help,
                                      KDialogBase::Ok,
                                      kapp->mainWidget() );

#ifndef Q_WS_WIN //TODO: reenable
  KWin::setIcons( kd->winId(), kapp->icon(), kapp->miniIcon() );
#endif

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
    KateDocumentConfig::global()->configStart ();
    KateViewConfig::global()->configStart ();
    KateRendererConfig::global()->configStart ();

    for (uint i=0; i<editorPages.count(); i++)
    {
      editorPages.at(i)->apply();
    }

    KateDocumentConfig::global()->configEnd ();
    KateViewConfig::global()->configEnd ();
    KateRendererConfig::global()->configEnd ();

    writeConfig ();
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
  if( line > lastLine() )
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
  if( line > lastLine())
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
  uint reserved = (0x1 << KTextEditor::MarkInterface::reservedMarkersCount()) - 1;
  if ((uint)type >= (uint)markType01 && (uint)type <= reserved) {
    return KateRendererConfig::global()->lineMarkerColor(type);
  } else {
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
  return KatePrinter::print (this);
}

bool KateDocument::print ()
{
  return KatePrinter::print (this);
}
//END

//BEGIN KTextEditor::DocumentInfoInterface (### unfinished)
QString KateDocument::mimeType()
{
  KMimeType::Ptr result = KMimeType::defaultMimeTypePtr();

  // if the document has a URL, try KMimeType::findByURL
  if ( ! m_url.isEmpty() )
    result = KMimeType::findByURL( m_url );

  else if ( m_url.isEmpty() || ! m_url.isLocalFile() )
    result = mimeTypeForContent();

  return result->name();
}

// TODO implement this -- how to calculate?
long KateDocument::fileSize()
{
  return 0;
}

// TODO implement this
QString KateDocument::niceFileSize()
{
  return "UNKNOWN";
}

KMimeType::Ptr KateDocument::mimeTypeForContent()
{
  QByteArray buf (1024);
  uint bufpos = 0;

  for (uint i=0; i < numLines(); i++)
  {
    QString line = textLine( i );
    uint len = line.length() + 1;

    if (bufpos + len > 1024)
      len = 1024 - bufpos;

    memcpy(&buf[bufpos], (line + "\n").latin1(), len);

    bufpos += len;

    if (bufpos >= 1024)
      break;
  }
  buf.resize( bufpos );

  int accuracy = 0;
  return KMimeType::findByContent( buf, &accuracy );
}
//END KTextEditor::DocumentInfoInterface


//BEGIN KParts::ReadWrite stuff

bool KateDocument::openURL( const KURL &url )
{
//   kdDebug(13020)<<"KateDocument::openURL( "<<url.prettyURL()<<")"<<endl;
  // no valid URL
  if ( !url.isValid() )
    return false;

  // could not close old one
  if ( !closeURL() )
    return false;

  // set my url
  m_url = url;

  if ( m_url.isLocalFile() )
  {
    // local mode, just like in kpart

    m_file = m_url.path();

    emit started( 0 );

    if (openFile())
    {
      emit completed();
      emit setWindowCaption( m_url.prettyURL() );

      return true;
    }

    return false;
  }
  else
  {
    // remote mode

    m_bTemp = true;

    m_tempFile = new KTempFile ();
    m_file = m_tempFile->name();

    m_job = KIO::get ( url, false, isProgressInfoEnabled() );

    // connect to slots
    connect( m_job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
           SLOT( slotDataKate( KIO::Job*, const QByteArray& ) ) );

    connect( m_job, SIGNAL( result( KIO::Job* ) ),
           SLOT( slotFinishedKate( KIO::Job* ) ) );

    QWidget *w = widget ();
    if (!w && !m_views.isEmpty ())
      w = m_views.first();

    if (w)
      m_job->setWindow (w->topLevelWidget());

    emit started( m_job );

    return true;
  }
}

void KateDocument::slotDataKate ( KIO::Job *, const QByteArray &data )
{
//   kdDebug(13020) << "KateDocument::slotData" << endl;

  if (!m_tempFile || !m_tempFile->file())
    return;

  m_tempFile->file()->writeBlock (data);
}

void KateDocument::slotFinishedKate ( KIO::Job * job )
{
//   kdDebug(13020) << "KateDocument::slotJobFinished" << endl;

  if (!m_tempFile)
    return;

  delete m_tempFile;
  m_tempFile = 0;
  m_job = 0;

  if (job->error())
    emit canceled( job->errorString() );
  else
  {
    if ( openFile(job) )
      emit setWindowCaption( m_url.prettyURL() );
    emit completed();
  }
}

void KateDocument::abortLoadKate()
{
  if ( m_job )
  {
    kdDebug(13020) << "Aborting job " << m_job << endl;
    m_job->kill();
    m_job = 0;
  }

  delete m_tempFile;
  m_tempFile = 0;
}

bool KateDocument::openFile()
{
  return openFile (0);
}

bool KateDocument::openFile(KIO::Job * job)
{
  // add new m_file to dirwatch
  activateDirWatch ();

  //
  // use metadata
  //
  if (job)
  {
    QString metaDataCharset = job->queryMetaData("charset");

    // only overwrite config if nothing set
    if (!metaDataCharset.isEmpty () && (!m_config->isSetEncoding() || m_config->encoding().isEmpty()))
      setEncoding (metaDataCharset);
  }

  //
  // service type magic to get encoding right
  //
  QString serviceType = m_extension->urlArgs().serviceType.simplifyWhiteSpace();
  int pos = serviceType.find(';');
  if (pos != -1)
    setEncoding (serviceType.mid(pos+1));

  // do we have success ?
  bool success = m_buffer->openFile (m_file);
  //
  // yeah, success
  //
  if (success)
  {
    /*if (highlight() && !m_url.isLocalFile()) {
      // The buffer's highlighting gets nuked by KateBuffer::clear()
      m_buffer->setHighlight(m_highlight);
  }*/

    // update our hl type if needed
    if (!hlSetByUser)
    {
      int hl (KateHlManager::self()->detectHighlighting (this));

      if (hl >= 0)
        m_buffer->setHighlight(hl);
    }

    // update file type
    updateFileType (KateFactory::self()->fileTypeManager()->fileType (this));

    // read dir config (if possible and wanted)
    readDirConfig ();

    // read vars
    readVariables();

    // update the md5 digest
    createDigest( m_digest );
  }

  //
  // update views
  //
  for (KateView * view = m_views.first(); view != 0L; view = m_views.next() )
  {
    view->updateView(true);
  }

  //
  // emit the signal we need for example for kate app
  //
  emit fileNameChanged ();

  //
  // set doc name, dummy value as arg, don't need it
  //
  setDocName  (QString::null);

  //
  // to houston, we are not modified
  //
  if (m_modOnHd)
  {
    m_modOnHd = false;
    m_modOnHdReason = 0;
    emit modifiedOnDisc (this, m_modOnHd, 0);
  }

  //
  // display errors
  //
  if (s_openErrorDialogsActivated)
  {
    if (!success && m_buffer->loadingBorked())
      KMessageBox::error (widget(), i18n ("The file %1 could not be loaded completely, as there is not enough temporary disk storage for it.").arg(m_url.url()));
    else if (!success)
      KMessageBox::error (widget(), i18n ("The file %1 could not be loaded, as it was not possible to read from it.\n\nCheck if you have read access to this file.").arg(m_url.url()));
  }

  // warn -> opened binary file!!!!!!!
  if (m_buffer->binary())
  {
    // this file can't be saved again without killing it
    setReadWrite( false );

    KMessageBox::information (widget()
      , i18n ("The file %1 is a binary, saving it will result in a corrupt file.").arg(m_url.url())
      , i18n ("Binary File Opened")
      , "Binary File Opened Warning");
  }

  //
  // return the success
  //
  return success;
}

bool KateDocument::save()
{
  bool l ( url().isLocalFile() );

  if ( ( l && config()->backupFlags() & KateDocumentConfig::LocalFiles )
       || ( ! l && config()->backupFlags() & KateDocumentConfig::RemoteFiles ) )
  {
    KURL u( url() );
    u.setFileName( config()->backupPrefix() + url().fileName() + config()->backupSuffix() );

    kdDebug () << "backup src file name: " << url() << endl;
    kdDebug () << "backup dst file name: " << u << endl;

    // get the right permissions, start with safe default
    mode_t  perms = 0600;
    KIO::UDSEntry fentry;
    if (KIO::NetAccess::stat (url(), fentry, kapp->mainWidget()))
    {
      kdDebug () << "stating succesfull: " << url() << endl;
      KFileItem item (fentry, url());
      perms = item.permissions();
    }

    // first del existing file if any, than copy over the file we have
    // failure if a: the existing file could not be deleted, b: the file could not be copied
    if ( (!KIO::NetAccess::exists( u, false, kapp->mainWidget() ) || KIO::NetAccess::del( u, kapp->mainWidget() ))
          && KIO::NetAccess::file_copy( url(), u, perms, true, false, kapp->mainWidget() ) )
    {
      kdDebug(13020)<<"backing up successfull ("<<url().prettyURL()<<" -> "<<u.prettyURL()<<")"<<endl;
    }
    else
    {
      kdDebug(13020)<<"backing up failed ("<<url().prettyURL()<<" -> "<<u.prettyURL()<<")"<<endl;
      // FIXME: notify user for real ;)
    }
  }

  return KParts::ReadWritePart::save();
}

bool KateDocument::saveFile()
{
  //
  // we really want to save this file ?
  //
  if (m_buffer->loadingBorked() && (KMessageBox::warningYesNo(widget(),
      i18n("This file could not be loaded correctly due to lack of temporary disk space. Saving it could cause data loss.\n\nDo you really want to save it?")) != KMessageBox::Yes))
    return false;

  //
  // warn -> try to save binary file!!!!!!!
  //
  if (m_buffer->binary() && (KMessageBox::warningYesNo (widget()
        , i18n ("The file %1 is a binary, saving it will result in a corrupt file.").arg(m_url.url())
        , i18n ("Try To Save Binary File")
        , KStdGuiItem::yes(), KStdGuiItem::no(), "Binary File Save Warning") != KMessageBox::Yes))
    return false;

  if ( !url().isEmpty() )
  {
    if (s_fileChangedDialogsActivated && m_modOnHd)
    {
      QString str = reasonedMOHString() + "\n\n";

      if (!isModified())
      {
        if (KMessageBox::warningYesNo(0,
               str + i18n("Do you really want to save this unmodified file? You could overwrite changed data in the file on disk.")) != KMessageBox::Yes)
          return false;
      }
      else
      {
        if (KMessageBox::warningYesNo(0,
               str + i18n("Do you really want to save this file? Both your open file and the file on disk were changed. There could be some data lost.")) != KMessageBox::Yes)
          return false;
      }
    }
  }

  //
  // can we encode it if we want to save it ?
  //
  if (!m_buffer->canEncode ()
       && (KMessageBox::warningYesNo(0,
           i18n("The selected encoding cannot encode every unicode character in this document. Do you really want to save it? There could be some data lost.")) != KMessageBox::Yes))
  {
    return false;
  }

  // remove file from dirwatch
  deactivateDirWatch ();

  //
  // try to save
  //
  bool success = m_buffer->saveFile (m_file);

  // update the md5 digest
  createDigest( m_digest );

  // add m_file again to dirwatch
  activateDirWatch ();

  //
  // hurray, we had success, do stuff we need
  //
  if (success)
  {
    // update our hl type if needed
    if (!hlSetByUser)
    {
      int hl (KateHlManager::self()->detectHighlighting (this));

      if (hl >= 0)
        m_buffer->setHighlight(hl);
    }

    // read our vars
    readVariables();
  }

  //
  // we are not modified
  //
  if (success && m_modOnHd)
  {
    m_modOnHd = false;
    m_modOnHdReason = 0;
    emit modifiedOnDisc (this, m_modOnHd, 0);
  }

  //
  // display errors
  //
  if (!success)
    KMessageBox::error (widget(), i18n ("The document could not be saved, as it was not possible to write to %1.\n\nCheck that you have write access to this file or that enough disk space is available.").arg(m_url.url()));

  //
  // return success
  //
  return success;
}

bool KateDocument::saveAs( const KURL &u )
{
  QString oldDir = url().directory();

  if ( KParts::ReadWritePart::saveAs( u ) )
  {
    // null means base on filename
    setDocName( QString::null );

    if ( u.directory() != oldDir )
      readDirConfig();

    emit fileNameChanged();
    return true;
  }

  return false;
}

void KateDocument::readDirConfig ()
{
  int depth = config()->searchDirConfigDepth ();

  if (m_url.isLocalFile() && (depth > -1))
  {
    QString currentDir = QFileInfo (m_file).dirPath();

    // only search as deep as specified or not at all ;)
    while (depth > -1)
    {
      kdDebug (13020) << "search for config file in path: " << currentDir << endl;

      // try to open config file in this dir
      QFile f (currentDir + "/.kateconfig");

      if (f.open (IO_ReadOnly))
      {
        QTextStream stream (&f);

        uint linesRead = 0;
        QString line = stream.readLine();
        while ((linesRead < 32) && !line.isNull())
        {
          readVariableLine( line );

          line = stream.readLine();

          linesRead++;
        }

        break;
      }

      QString newDir = QFileInfo (currentDir).dirPath();

      // bail out on looping (for example reached /)
      if (currentDir == newDir)
        break;

      currentDir = newDir;
      --depth;
    }
  }
}

void KateDocument::activateDirWatch ()
{
  // same file as we are monitoring, return
  if (m_file == m_dirWatchFile)
    return;

  // remove the old watched file
  deactivateDirWatch ();

  // add new file if needed
  if (m_url.isLocalFile() && !m_file.isEmpty())
  {
    KateFactory::self()->dirWatch ()->addFile (m_file);
    m_dirWatchFile = m_file;
  }
}

void KateDocument::deactivateDirWatch ()
{
  if (!m_dirWatchFile.isEmpty())
    KateFactory::self()->dirWatch ()->removeFile (m_dirWatchFile);

  m_dirWatchFile = QString::null;
}

bool KateDocument::closeURL()
{
  abortLoadKate();

  //
  // file mod on hd
  //
  if ( !m_reloading && !url().isEmpty() )
  {
    if (s_fileChangedDialogsActivated && m_modOnHd)
    {
      if (!(KMessageBox::warningYesNo(
            widget(),
            reasonedMOHString() + "\n\n" + i18n("Do you really want to continue to close this file? Data loss may occur."),
            "", KStdGuiItem::yes(), KStdGuiItem::no(),
            QString("kate_close_modonhd_%1").arg( m_modOnHdReason ) ) == KMessageBox::Yes))
        return false;
    }
  }

  //
  // first call the normal kparts implementation
  //
  if (!KParts::ReadWritePart::closeURL ())
    return false;

  // remove file from dirwatch
  deactivateDirWatch ();

  //
  // empty url + filename
  //
  m_url = KURL ();
  m_file = QString::null;

  // we are not modified
  if (m_modOnHd)
  {
    m_modOnHd = false;
    m_modOnHdReason = 0;
    emit modifiedOnDisc (this, m_modOnHd, 0);
  }

  // clear the buffer
  m_buffer->clear();

  // remove all marks
  clearMarks ();

  // clear undo/redo history
  clearUndo();
  clearRedo();

  // no, we are no longer modified
  setModified(false);

  // we have no longer any hl
  m_buffer->setHighlight(0);

  // update all our views
  for (KateView * view = m_views.first(); view != 0L; view = m_views.next() )
  {
    // Explicitly call the internal version because we don't want this to look like
    // an external request (and thus have the view not QWidget::scroll()ed.
    view->setCursorPositionInternal(0, 0, 1, false);
    view->updateView(true);
  }

  // uh, filename changed
  emit fileNameChanged ();

  // update doc name
  setDocName (QString::null);

  // success
  return true;
}

void KateDocument::setReadWrite( bool rw )
{
  if (isReadWrite() != rw)
  {
    KParts::ReadWritePart::setReadWrite (rw);

    for( KateView* view = m_views.first(); view != 0L; view = m_views.next() )
    {
      view->slotUpdate();
      view->slotReadWriteChanged ();
    }
  }
}

void KateDocument::setModified(bool m) {

  if (isModified() != m) {
    KParts::ReadWritePart::setModified (m);

    for( KateView* view = m_views.first(); view != 0L; view = m_views.next() )
    {
      view->slotUpdate();
    }

    emit modifiedChanged ();
    emit modStateChanged ((Kate::Document *)this);
  }
  if ( m == false && ! undoItems.isEmpty() )
  {
    lastUndoGroupWhenSaved = undoItems.last();
  }

  if ( m == false ) docWasSavedWhenUndoWasEmpty = undoItems.isEmpty();
}
//END

//BEGIN Kate specific stuff ;)

void KateDocument::makeAttribs(bool needInvalidate)
{
  highlight()->clearAttributeArrays ();

  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->renderer()->updateAttributes ();

  if (needInvalidate)
    m_buffer->invalidateHighlighting();

  tagAll ();
}

// the attributes of a hl have changed, update
void KateDocument::internalHlChanged()
{
  makeAttribs();
}

void KateDocument::addView(KTextEditor::View *view) {
  if (!view)
    return;

  m_views.append( (KateView *) view  );
  m_textEditViews.append( view );

  // apply the view & renderer vars from the file type
  const KateFileType *t = 0;
  if ((m_fileType > -1) && (t = KateFactory::self()->fileTypeManager()->fileType(m_fileType)))
    readVariableLine (t->varLine, true);

  // apply the view & renderer vars from the file
  readVariables (true);

  m_activeView = (KateView *) view;
}

void KateDocument::removeView(KTextEditor::View *view) {
  if (!view)
    return;

  if (m_activeView == view)
    m_activeView = 0L;

  m_views.removeRef( (KateView *) view );
  m_textEditViews.removeRef( view  );
}

void KateDocument::addSuperCursor(KateSuperCursor *cursor, bool privateC) {
  if (!cursor)
    return;

  m_superCursors.append( cursor );

  if (!privateC)
    myCursors.append( cursor );
}

void KateDocument::removeSuperCursor(KateSuperCursor *cursor, bool privateC) {
  if (!cursor)
    return;

  if (!privateC)
    myCursors.removeRef( cursor  );

  m_superCursors.removeRef( cursor  );
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
  KateTextLine::Ptr textLine = m_buffer->plainLine(cursor.line());

  if (textLine)
    return textLine->cursorX(cursor.col(), config()->tabWidth());
  else
    return 0;
}

bool KateDocument::typeChars ( KateView *view, const QString &chars )
{
  KateTextLine::Ptr textLine = m_buffer->plainLine(view->cursorLine ());

  if (!textLine)
    return false;

  bool bracketInserted = false;
  QString buf;
  QChar c;
  for( uint z = 0; z < chars.length(); z++ )
  {
    QChar ch = c = chars[z];

    if (ch.isPrint() || ch == '\t')
    {
      buf.append (ch);

      if (!bracketInserted && (config()->configFlags() & KateDocument::cfAutoBrackets))
      {
        if (ch == '(') { bracketInserted = true; buf.append (')'); }
        if (ch == '[') { bracketInserted = true; buf.append (']'); }
        if (ch == '{') { bracketInserted = true; buf.append ('}'); }
      }
    }
  }

  if (buf.isEmpty())
    return false;

  editStart ();

  if (!view->config()->persistentSelection() && view->hasSelection() )
    view->removeSelectedText();

  int oldLine = view->cursorLine ();
  int oldCol = view->cursorColumnReal ();


  if (config()->configFlags()  & KateDocument::cfOvr)
    removeText (view->cursorLine(), view->cursorColumnReal(), view->cursorLine(), QMIN( view->cursorColumnReal()+buf.length(), textLine->length() ) );

  insertText (view->cursorLine(), view->cursorColumnReal(), buf);
  m_indenter->processChar(c);

  editEnd ();

  if (bracketInserted)
    view->setCursorPositionInternal (view->cursorLine(), view->cursorColumnReal()-1);

  emit charactersInteractivelyInserted (oldLine, oldCol, chars);

  return true;
}

void KateDocument::newLine( KateTextCursor& c, KateViewInternal *v )
{
  editStart();

  if( !v->view()->config()->persistentSelection() && v->view()->hasSelection() )
    v->view()->removeSelectedText();

  // temporary hack to get the cursor pos right !!!!!!!!!
  c = v->getCursor ();

  if (c.line() > (int)lastLine())
   c.setLine(lastLine());

  if ( c.line() < 0 )
    c.setLine( 0 );

  uint ln = c.line();

  KateTextLine::Ptr textLine = kateTextLine(c.line());

  if (c.col() > (int)textLine->length())
    c.setCol(textLine->length());

  if (m_indenter->canProcessNewLine ())
  {
    int pos = textLine->firstChar();

    // length should do the job better
    if (pos < 0)
      pos = textLine->length();

    if (c.col() < pos)
      c.setCol(pos); // place cursor on first char if before

    editWrapLine (c.line(), c.col());

    KateDocCursor cursor (c.line() + 1, pos, this);
    m_indenter->processNewline(cursor, true);

    c.setPos(cursor);
  }
  else
  {
    editWrapLine (c.line(), c.col());
    c.setPos(c.line() + 1, 0);
  }

  removeTrailingSpace( ln );

  editEnd();
}

void KateDocument::transpose( const KateTextCursor& cursor)
{
  KateTextLine::Ptr textLine = m_buffer->plainLine(cursor.line());

  if (!textLine || (textLine->length() < 2))
    return;

  uint col = cursor.col();

  if (col > 0)
    col--;

  if ((textLine->length() - col) < 2)
    return;

  uint line = cursor.line();
  QString s;

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

void KateDocument::backspace( KateView *view, const KateTextCursor& c )
{
  if ( !view->config()->persistentSelection() && view->hasSelection() ) {
    view->removeSelectedText();
    return;
  }

  uint col = QMAX( c.col(), 0 );
  uint line = QMAX( c.line(), 0 );

  if ((col == 0) && (line == 0))
    return;

  if (col > 0)
  {
    if (!(config()->configFlags() & KateDocument::cfBackspaceIndents))
    {
      // ordinary backspace
      //c.cursor.col--;
      removeText(line, col-1, line, col);
    }
    else
    {
      // backspace indents: erase to next indent position
      KateTextLine::Ptr textLine = m_buffer->plainLine(line);

      // don't forget this check!!!! really!!!!
      if (!textLine)
        return;

      int colX = textLine->cursorX(col, config()->tabWidth());
      int pos = textLine->firstChar();
      if (pos > 0)
        pos = textLine->cursorX(pos, config()->tabWidth());

      if (pos < 0 || pos >= (int)colX)
      {
        // only spaces on left side of cursor
        // search a line with less spaces
        int y = line;
        while (--y >= 0)
        {
          // this is save, y <= line, and line was already success
          textLine = m_buffer->plainLine(y);

          pos = textLine->firstChar();

          if (pos >= 0)
          {
            pos = textLine->cursorX(pos, config()->tabWidth());
            if (pos < (int)colX)
            {
              replaceWithOptimizedSpace(line, col, pos, config()->configFlags());
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
      KateTextLine::Ptr textLine = m_buffer->plainLine(line-1);

      // don't forget this check!!!! really!!!!
      if (!textLine)
        return;

      if (config()->wordWrap() && textLine->endingWith(QString::fromLatin1(" ")))
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

void KateDocument::del( KateView *view, const KateTextCursor& c )
{
  if ( !view->config()->persistentSelection() && view->hasSelection() ) {
    view->removeSelectedText();
    return;
  }

  if( c.col() < (int) m_buffer->plainLine(c.line())->length())
  {
    removeText(c.line(), c.col(), c.line(), c.col()+1);
  }
  else if ( (uint)c.line() < lastLine() )
  {
    removeText(c.line(), c.col(), c.line()+1, 0);
  }
}

void KateDocument::paste ( KateView* view )
{
  QString s = QApplication::clipboard()->text();

  if (s.isEmpty())
    return;

  uint lines = s.contains (QChar ('\n'));

  m_undoDontMerge = true;

  editStart ();

  if (!view->config()->persistentSelection() && view->hasSelection() )
    view->removeSelectedText();

  uint line = view->cursorLine ();
  uint column = view->cursorColumnReal ();

  insertText ( line, column, s, view->blockSelectionMode() );

  editEnd();

  // move cursor right for block select, as the user is moved right internal
  // even in that case, but user expects other behavior in block selection
  // mode !
  if (view->blockSelectionMode())
    view->setCursorPositionInternal (line+lines, column);

  if (m_indenter->canProcessLine())
  {
    editStart();

    KateDocCursor begin(line, 0, this);
    KateDocCursor end(line + lines, 0, this);

    m_indenter->processSection (begin, end);

    editEnd();
  }

  if (!view->blockSelectionMode()) emit charactersSemiInteractivelyInserted (line, column, s);
  m_undoDontMerge = true;
}

void KateDocument::insertIndentChars ( KateView *view )
{
  editStart ();

  QString s;
  if (config()->configFlags() & KateDocument::cfSpaceIndent)
  {
    int width = config()->indentationWidth();
    s.fill (' ', width - (view->cursorColumnReal() % width));
  }
  else
    s.append ('\t');

  insertText (view->cursorLine(), view->cursorColumnReal(), s);

  editEnd ();
}

void KateDocument::indent ( KateView *v, uint line, int change)
{
  editStart ();

  if (!hasSelection())
  {
    // single line
    optimizeLeadingSpace(line, config()->configFlags(), change);
  }
  else
  {
    int sl = v->selStartLine();
    int el = v->selEndLine();
    int ec = v->selEndCol();

    if ((ec == 0) && ((el-1) >= 0))
    {
      el--; /* */
    }

    if (config()->configFlags() & KateDocument::cfKeepIndentProfile && change < 0) {
      // unindent so that the existing indent profile doesn't get screwed
      // if any line we may unindent is already full left, don't do anything
      int adjustedChange = -change;

      for (line = sl; (int) line <= el && adjustedChange > 0; line++) {
        KateTextLine::Ptr textLine = m_buffer->plainLine(line);
        int firstChar = textLine->firstChar();
        if (firstChar >= 0 && (v->lineSelected(line) || v->lineHasSelected(line))) {
          int maxUnindent = textLine->cursorX(firstChar, config()->tabWidth()) / config()->indentationWidth();
          if (maxUnindent < adjustedChange)
            adjustedChange = maxUnindent;
        }
      }

      change = -adjustedChange;
    }

    for (line = sl; (int) line <= el; line++) {
      if (v->lineSelected(line) || v->lineHasSelected(line)) {
        optimizeLeadingSpace(line, config()->configFlags(), change);
      }
    }
  }

  editEnd ();
}

void KateDocument::align(KateView *view, uint line)
{
  if (m_indenter->canProcessLine())
  {
    editStart ();

    if (!view->hasSelection())
    {
      KateDocCursor curLine(line, 0, this);
      m_indenter->processLine (curLine);
      editEnd ();
      activeView()->setCursorPosition (line, curLine.col());
    }
    else
    {
      m_indenter->processSection (view->selStart(), view->selEnd());
      editEnd ();
    }
  }
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
  KateTextLine::Ptr textline = m_buffer->plainLine(line);

  int first_char = textline->firstChar();

  int w = 0;
  if (flags & KateDocument::cfSpaceIndent)
    w = config()->indentationWidth();
  else
    w = config()->tabWidth();

  if (first_char < 0)
    first_char = textline->length();

  int space =  textline->cursorX(first_char, config()->tabWidth()) + change * w;
  if (space < 0)
    space = 0;

  if (!(flags & KateDocument::cfKeepExtraSpaces))
  {
    uint extra = space % w;

    space -= extra;
    if (extra && change < 0) {
      // otherwise it unindents too much (e.g. 12 chars when indentation is 8 chars wide)
      space += w;
    }
  }

  //kdDebug(13020)  << "replace With Op: " << line << " " << first_char << " " << space << endl;
  replaceWithOptimizedSpace(line, first_char, space, flags);
}

void KateDocument::replaceWithOptimizedSpace(uint line, uint upto_column, uint space, int flags)
{
  uint length;
  QString new_space;

  if (flags & KateDocument::cfSpaceIndent && ! (flags & KateDocumentConfig::cfMixedIndent) ) {
    length = space;
    new_space.fill(' ', length);
  }
  else {
    length = space / config()->tabWidth();
    new_space.fill('\t', length);

    QString extra_space;
    extra_space.fill(' ', space % config()->tabWidth());
    length += space % config()->tabWidth();
    new_space += extra_space;
  }

  KateTextLine::Ptr textline = m_buffer->plainLine(line);
  uint change_from;
  for (change_from = 0; change_from < upto_column && change_from < length; change_from++) {
    if (textline->getChar(change_from) != new_space[change_from])
      break;
  }

  editStart();

  if (change_from < upto_column)
    removeText(line, change_from, line, upto_column);

  if (change_from < length)
    insertText(line, change_from, new_space.right(length - change_from));

  editEnd();
}

/*
  Remove a given string at the begining
  of the current line.
*/
bool KateDocument::removeStringFromBegining(int line, QString &str)
{
  KateTextLine::Ptr textline = m_buffer->plainLine(line);

  int index = 0;
  bool there = false;

  if (textline->startingWith(str))
    there = true;
  else
  {
    index = textline->firstChar ();

    if ((index >= 0) && (textline->length() >= (index + str.length())) && (textline->string(index, str.length()) == str))
      there = true;
  }

  if (there)
  {
    // Remove some chars
    removeText (line, index, line, index+str.length());
  }

  return there;
}

/*
  Remove a given string at the end
  of the current line.
*/
bool KateDocument::removeStringFromEnd(int line, QString &str)
{
  KateTextLine::Ptr textline = m_buffer->plainLine(line);

  int index = 0;
  bool there = false;

  if(textline->endingWith(str))
  {
    index = textline->length() - str.length();
    there = true;
  }
  else
  {
    index = textline->lastChar ()-str.length()+1;

    if ((index >= 0) && (textline->length() >= (index + str.length())) && (textline->string(index, str.length()) == str))
      there = true;
  }

  if (there)
  {
    // Remove some chars
    removeText (line, index, line, index+str.length());
  }

  return there;
}

/*
  Add to the current line a comment line mark at
  the begining.
*/
void KateDocument::addStartLineCommentToSingleLine( int line, int attrib )
{
  if (highlight()->getCommentSingleLinePosition(attrib)==KateHighlighting::CSLPosColumn0)
  {
    QString commentLineMark = highlight()->getCommentSingleLineStart( attrib ) + " ";
    insertText (line, 0, commentLineMark);
  }
  else
  {
    QString commentLineMark=highlight()->getCommentSingleLineStart(attrib);
    KateTextLine::Ptr l = m_buffer->line(line);
    int pos=l->firstChar();
    if (pos >=0)
      insertText(line,pos,commentLineMark);
  }
}

/*
  Remove from the current line a comment line mark at
  the begining if there is one.
*/
bool KateDocument::removeStartLineCommentFromSingleLine( int line, int attrib )
{
  QString shortCommentMark = highlight()->getCommentSingleLineStart( attrib );
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
void KateDocument::addStartStopCommentToSingleLine( int line, int attrib )
{
  QString startCommentMark = highlight()->getCommentStart( attrib ) + " ";
  QString stopCommentMark = " " + highlight()->getCommentEnd( attrib );

  editStart();

  // Add the start comment mark
  insertText (line, 0, startCommentMark);

  // Go to the end of the line
  int col = m_buffer->plainLine(line)->length();

  // Add the stop comment mark
  insertText (line, col, stopCommentMark);

  editEnd();
}

/*
  Remove from the current line a start comment mark at
  the begining and a stop comment mark at the end.
*/
bool KateDocument::removeStartStopCommentFromSingleLine( int line, int attrib )
{
  QString shortStartCommentMark = highlight()->getCommentStart( attrib );
  QString longStartCommentMark = shortStartCommentMark + " ";
  QString shortStopCommentMark = highlight()->getCommentEnd( attrib );
  QString longStopCommentMark = " " + shortStopCommentMark;

  editStart();

#ifdef __GNUC__
#warning "that's a bad idea, can lead to stray endings, FIXME"
#endif
  // Try to remove the long start comment mark first
  bool removedStart = (removeStringFromBegining(line, longStartCommentMark)
                       || removeStringFromBegining(line, shortStartCommentMark));

  bool removedStop = false;
  if (removedStart)
  {
    // Try to remove the long stop comment mark first
    removedStop = (removeStringFromEnd(line, longStopCommentMark)
                      || removeStringFromEnd(line, shortStopCommentMark));
  }

  editEnd();

  return (removedStart || removedStop);
}

/*
  Add to the current selection a start comment
 mark at the begining and a stop comment mark
 at the end.
*/
void KateDocument::addStartStopCommentToSelection( KateView *view, int attrib )
{
  QString startComment = highlight()->getCommentStart( attrib );
  QString endComment = highlight()->getCommentEnd( attrib );

  int sl = view->selStartLine();
  int el = view->selEndLine();
  int sc = view->selStartCol();
  int ec = view->selEndCol();

  if ((ec == 0) && ((el-1) >= 0))
  {
    el--;
    ec = m_buffer->plainLine (el)->length();
  }

  editStart();

  insertText (el, ec, endComment);
  insertText (sl, sc, startComment);

  editEnd ();

  // Set the new selection
  ec += endComment.length() + ( (el == sl) ? startComment.length() : 0 );
  view->setSelection(sl, sc, el, ec);
}

/*
  Add to the current selection a comment line
 mark at the begining of each line.
*/
void KateDocument::addStartLineCommentToSelection( KateView *view, int attrib )
{
  QString commentLineMark = highlight()->getCommentSingleLineStart( attrib ) + " ";

  int sl = view->selStartLine();
  int el = view->selEndLine();

  if ((view->selEndCol() == 0) && ((el-1) >= 0))
  {
    el--;
  }

  editStart();

  // For each line of the selection
  for (int z = el; z >= sl; z--) {
    //insertText (z, 0, commentLineMark);
    addStartLineCommentToSingleLine(z, attrib );
  }

  editEnd ();

  // Set the new selection

  KateDocCursor end (view->selEnd());
  end.setCol(view->selEndCol() + ((el == view->selEndLine()) ? commentLineMark.length() : 0) );

  view->setSelection(view->selStartLine(), 0, end.line(), end.col());
}

bool KateDocument::nextNonSpaceCharPos(int &line, int &col)
{
  for(; line < (int)m_buffer->count(); line++) {
    KateTextLine::Ptr textLine = m_buffer->plainLine(line);

    if (!textLine)
      break;

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

bool KateDocument::previousNonSpaceCharPos(int &line, int &col)
{
  while(true)
  {
    KateTextLine::Ptr textLine = m_buffer->plainLine(line);

    if (!textLine)
      break;

    col = textLine->previousNonSpaceChar(col);
    if(col != -1) return true;
    if(line == 0) return false;
    --line;
    col = textLine->length();
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
bool KateDocument::removeStartStopCommentFromSelection( KateView *view, int attrib )
{
  QString startComment = highlight()->getCommentStart( attrib );
  QString endComment = highlight()->getCommentEnd( attrib );

  int sl = kMax<int> (0, view->selStartLine());
  int el = kMin<int>  (view->selEndLine(), lastLine());
  int sc = view->selStartCol();
  int ec = view->selEndCol();

  // The selection ends on the char before selectEnd
  if (ec != 0) {
    ec--;
  } else {
    if (el > 0) {
      el--;
      ec = m_buffer->plainLine(el)->length() - 1;
    }
  }

  int startCommentLen = startComment.length();
  int endCommentLen = endComment.length();

  // had this been perl or sed: s/^\s*$startComment(.+?)$endComment\s*/$1/

  bool remove = nextNonSpaceCharPos(sl, sc)
      && m_buffer->plainLine(sl)->stringAtPos(sc, startComment)
      && previousNonSpaceCharPos(el, ec)
      && ( (ec - endCommentLen + 1) >= 0 )
      && m_buffer->plainLine(el)->stringAtPos(ec - endCommentLen + 1, endComment);

  if (remove) {
    editStart();

    removeText (el, ec - endCommentLen + 1, el, ec + 1);
    removeText (sl, sc, sl, sc + startCommentLen);

    editEnd ();

    // Set the new selection
    ec -= endCommentLen + ( (el == sl) ? startCommentLen : 0 );
    view->setSelection(sl, sc, el, ec + 1);
  }

  return remove;
}

bool KateDocument::removeStartStopCommentFromRegion(const KateTextCursor &start,const KateTextCursor &end,int attrib)
{
  QString startComment = highlight()->getCommentStart( attrib );
  QString endComment = highlight()->getCommentEnd( attrib );
  int startCommentLen = startComment.length();
  int endCommentLen = endComment.length();

    bool remove = m_buffer->plainLine(start.line())->stringAtPos(start.col(), startComment)
      && ( (end.col() - endCommentLen ) >= 0 )
      && m_buffer->plainLine(end.line())->stringAtPos(end.col() - endCommentLen , endComment);
      if (remove)  {
        editStart();
          removeText(end.line(),end.col()-endCommentLen,end.line(),end.col());
          removeText(start.line(),start.col(),start.line(),start.col()+startCommentLen);
        editEnd();
      }
      return remove;
}

/*
  Remove from the begining of each line of the
  selection a start comment line mark.
*/
bool KateDocument::removeStartLineCommentFromSelection( KateView *view, int attrib )
{
  QString shortCommentMark = highlight()->getCommentSingleLineStart( attrib );
  QString longCommentMark = shortCommentMark + " ";

  int sl = view->selStartLine();
  int el = view->selEndLine();

  if ((view->selEndCol() == 0) && ((el-1) >= 0))
  {
    el--;
  }

  // Find out how many char will be removed from the last line
  int removeLength = 0;
  if (m_buffer->plainLine(el)->startingWith(longCommentMark))
    removeLength = longCommentMark.length();
  else if (m_buffer->plainLine(el)->startingWith(shortCommentMark))
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

  if (removed)
  {
    // Set the new selection
    KateDocCursor end (view->selEnd());
    end.setCol(view->selEndCol() - ((el == view->selEndLine()) ? removeLength : 0) );

    setSelection(view->selStartLine(), view->selStartCol(), end.line(), end.col());
  }

  return removed;
}

/*
  Comment or uncomment the selection or the current
  line if there is no selection.
*/
void KateDocument::comment( KateView *v, uint line,uint column, int change)
{
  // We need to check that we can sanely comment the selectino or region.
  // It is if the attribute of the first and last character of the range to
  // comment belongs to the same language definition.
  // for lines with no text, we need the attribute for the lines context.
  bool hassel = v->hasSelection();
  int startAttrib, endAttrib;
  if ( hassel )
  {
    KateTextLine::Ptr ln = kateTextLine( v->selStartLine() );
    int l = v->selStartLine(), c = v->selStartCol();
    startAttrib = nextNonSpaceCharPos( l, c ) ? kateTextLine( l )->attribute( c ) : 0;

    ln = kateTextLine( v->selEndLine() );
    l = v->selEndLine(), c = v->selEndCol();
    endAttrib = previousNonSpaceCharPos( l, c ) ? kateTextLine( l )->attribute( c ) : 0;
  }
  else
  {
    KateTextLine::Ptr ln = kateTextLine( line );
    if ( ln->length() )
    {
      startAttrib = ln->attribute( ln->firstChar() );
      endAttrib = ln->attribute( ln->lastChar() );
    }
    else
    {
      int l = line, c = 0;
      if ( nextNonSpaceCharPos( l, c )  || previousNonSpaceCharPos( l, c ) )
        startAttrib = endAttrib = kateTextLine( l )->attribute( c );
      else
        startAttrib = endAttrib = 0;
    }
  }

  if ( ! highlight()->canComment( startAttrib, endAttrib ) )
  {
    kdDebug(13020)<<"canComment( "<<startAttrib<<", "<<endAttrib<<" ) returned false!"<<endl;
    return;
  }

  bool hasStartLineCommentMark = !(highlight()->getCommentSingleLineStart( startAttrib ).isEmpty());
  bool hasStartStopCommentMark = ( !(highlight()->getCommentStart( startAttrib ).isEmpty())
      && !(highlight()->getCommentEnd( endAttrib ).isEmpty()) );

  bool removed = false;

  if (change > 0) // comment
  {
    if ( !hassel )
    {
      if ( hasStartLineCommentMark )
        addStartLineCommentToSingleLine( line, startAttrib );
      else if ( hasStartStopCommentMark )
        addStartStopCommentToSingleLine( line, startAttrib );
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
           ( v->selStartCol() > m_buffer->plainLine( v->selStartLine() )->firstChar() ) ||
           ( v->selEndCol() < ((int)m_buffer->plainLine( v->selEndLine() )->length()) )
         ) ) )
        addStartStopCommentToSelection( v, startAttrib );
      else if ( hasStartLineCommentMark )
        addStartLineCommentToSelection( v, startAttrib );
    }
  }
  else // uncomment
  {
    if ( !hassel )
    {
      removed = ( hasStartLineCommentMark
                  && removeStartLineCommentFromSingleLine( line, startAttrib ) )
        || ( hasStartStopCommentMark
             && removeStartStopCommentFromSingleLine( line, startAttrib ) );
      if ((!removed) && foldingTree()) {
        kdDebug(13020)<<"easy approach for uncommenting did not work, trying harder (folding tree)"<<endl;
        int commentRegion=(highlight()->commentRegion(startAttrib));
        if (commentRegion){
           KateCodeFoldingNode *n=foldingTree()->findNodeForPosition(line,column);
           if (n) {
            KateTextCursor start,end;
             if ((n->nodeType()==commentRegion) && n->getBegin(foldingTree(), &start) && n->getEnd(foldingTree(), &end)) {
                kdDebug(13020)<<"Enclosing region found:"<<start.col()<<"/"<<start.line()<<"-"<<end.col()<<"/"<<end.line()<<endl;
                removeStartStopCommentFromRegion(start,end,startAttrib);
             } else {
                  kdDebug(13020)<<"Enclosing region found, but not valid"<<endl;
                  kdDebug(13020)<<"Region found: "<<n->nodeType()<<" region needed: "<<commentRegion<<endl;
             }
            //perhaps nested regions should be hadled here too...
          } else kdDebug(13020)<<"No enclosing region found"<<endl;
        } else kdDebug(13020)<<"No comment region specified for current hl"<<endl;
      }
    }
    else
    {
      // anders: this seems like it will work with above changes :)
      removed = ( hasStartLineCommentMark
          && removeStartLineCommentFromSelection( v, startAttrib ) )
        || ( hasStartStopCommentMark
          && removeStartStopCommentFromSelection( v, startAttrib ) );
    }
  }
}

void KateDocument::transform( KateView *v, const KateTextCursor &c,
                            KateDocument::TextTransform t )
{
  editStart();
  uint cl( c.line() ), cc( c.col() );

  if ( hasSelection() )
  {
    // cache the selection and cursor, so we can be sure to restore.
    KateTextCursor s = v->selStart();
    KateTextCursor e = v->selEnd();

    int ln = v->selStartLine();
    while ( ln <= v->selEndLine() )
    {
      uint start, end;
      start = (ln == v->selStartLine() || v->blockSelectionMode()) ?
          v->selStartCol() : 0;
      end = (ln == v->selEndLine() || v->blockSelectionMode()) ?
          v->selEndCol() : lineLength( ln );
      QString s = text( ln, start, ln, end );

      if ( t == Uppercase )
        s = s.upper();
      else if ( t == Lowercase )
        s = s.lower();
      else // Capitalize
      {
        KateTextLine::Ptr l = m_buffer->plainLine( ln );
        uint p ( 0 );
        while( p < s.length() )
        {
          // If bol or the character before is not in a word, up this one:
          // 1. if both start and p is 0, upper char.
          // 2. if blockselect or first line, and p == 0 and start-1 is not in a word, upper
          // 3. if p-1 is not in a word, upper.
          if ( ( ! start && ! p ) ||
                   ( ( ln == selStartLine() || v->blockSelectionMode() ) &&
                   ! p && ! highlight()->isInWord( l->getChar( start - 1 )) ) ||
                   ( p && ! highlight()->isInWord( s.at( p-1 ) ) )
             )
            s[p] = s.at(p).upper();
          p++;
        }
      }

      removeText( ln, start, ln, end );
      insertText( ln, start, s );

      ln++;
    }

    // restore selection
    v->setSelection( s, e );

  } else {  // no selection
    QString s;
    int n ( cc );
    switch ( t ) {
      case Uppercase:
      s = text( cl, cc, cl, cc + 1 ).upper();
      break;
      case Lowercase:
      s = text( cl, cc, cl, cc + 1 ).lower();
      break;
      case Capitalize:
      {
        KateTextLine::Ptr l = m_buffer->plainLine( cl );
        while ( n > 0 && highlight()->isInWord( l->getChar( n-1 ), l->attribute( n-1 ) ) )
          n--;
        s = text( cl, n, cl, n + 1 ).upper();
      }
      break;
      default:
      break;
    }
    removeText( cl, n, cl, n+1 );
    insertText( cl, n, s );
  }

  editEnd();

  v->setCursorPosition( cl, cc );
}

void KateDocument::joinLines( uint first, uint last )
{
//   if ( first == last ) last += 1;
  editStart();
  int line( first );
  while ( first < last )
  {
    // Normalize the whitespace in the joined lines by making sure there's
    // always exactly one space between the joined lines
    // This cannot be done in editUnwrapLine, because we do NOT want this
    // behaviour when deleting from the start of a line, just when explicitly
    // calling the join command
    KateTextLine::Ptr l = m_buffer->line( line );
    KateTextLine::Ptr tl = m_buffer->line( line + 1 );

    if ( !l || !tl )
    {
      editEnd();
      return;
    }

    int pos = tl->firstChar();
    if ( pos >= 0 )
    {
      if (pos != 0)
        editRemoveText( line + 1, 0, pos );
      if ( !( l->length() == 0 || l->getChar( l->length() - 1 ).isSpace() ) )
        editInsertText( line + 1, 0, " " );
    }
    else
    {
      // Just remove the whitespace and let Kate handle the rest
      editRemoveText( line + 1, 0, tl->length() );
    }

    editUnWrapLine( line );
    first++;
  }
  editEnd();
}

QString KateDocument::getWord( const KateTextCursor& cursor ) {
  int start, end, len;

  KateTextLine::Ptr textLine = m_buffer->plainLine(cursor.line());
  len = textLine->length();
  start = end = cursor.col();
  if (start > len)        // Probably because of non-wrapping cursor mode.
    return QString("");

  while (start > 0 && highlight()->isInWord(textLine->getChar(start - 1), textLine->attribute(start - 1))) start--;
  while (end < len && highlight()->isInWord(textLine->getChar(end), textLine->attribute(end))) end++;
  len = end - start;
  return QString(&textLine->text()[start], len);
}

void KateDocument::tagLines(int start, int end)
{
  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->tagLines (start, end, true);
}

void KateDocument::tagLines(KateTextCursor start, KateTextCursor end)
{
  // May need to switch start/end cols if in block selection mode
  if (blockSelectionMode() && start.col() > end.col()) {
    int sc = start.col();
    start.setCol(end.col());
    end.setCol(sc);
  }

  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->tagLines(start, end, true);
}

void KateDocument::repaintViews(bool paintOnlyDirty)
{
  for (uint z = 0; z < m_views.count(); z++)
    m_views.at(z)->repaintText(paintOnlyDirty);
}

void KateDocument::tagAll()
{
  for (uint z = 0; z < m_views.count(); z++)
  {
    m_views.at(z)->tagAll();
    m_views.at(z)->updateView (true);
  }
}

uint KateDocument::configFlags ()
{
  return config()->configFlags();
}

void KateDocument::setConfigFlags (uint flags)
{
  config()->setConfigFlags(flags);
}

inline bool isStartBracket( const QChar& c ) { return c == '{' || c == '[' || c == '('; }
inline bool isEndBracket  ( const QChar& c ) { return c == '}' || c == ']' || c == ')'; }
inline bool isBracket     ( const QChar& c ) { return isStartBracket( c ) || isEndBracket( c ); }

/*
   Bracket matching uses the following algorithm:
   If in overwrite mode, match the bracket currently underneath the cursor.
   Otherwise, if the character to the right of the cursor is an starting bracket,
   match it. Otherwise if the character to the left of the cursor is a
   ending bracket, match it. Otherwise, if the the character to the left
   of the cursor is an starting bracket, match it. Otherwise, if the character
   to the right of the cursor is an ending bracket, match it. Otherwise, don't
   match anything.
*/
void KateDocument::newBracketMark( const KateTextCursor& cursor, KateBracketRange& bm, int maxLines )
{
  bm.setValid(false);

  bm.start() = cursor;

  if( !findMatchingBracket( bm.start(), bm.end(), maxLines ) )
    return;

  bm.setValid(true);

  const int tw = config()->tabWidth();
  const int indentStart = m_buffer->plainLine(bm.start().line())->indentDepth(tw);
  const int indentEnd = m_buffer->plainLine(bm.end().line())->indentDepth(tw);
  bm.setIndentMin(QMIN(indentStart, indentEnd));
}

bool KateDocument::findMatchingBracket( KateTextCursor& start, KateTextCursor& end, int maxLines )
{
  KateTextLine::Ptr textLine = m_buffer->plainLine( start.line() );
  if( !textLine )
    return false;

  QChar right = textLine->getChar( start.col() );
  QChar left  = textLine->getChar( start.col() - 1 );
  QChar bracket;

  if ( config()->configFlags() & cfOvr ) {
    if( isBracket( right ) ) {
      bracket = right;
    } else {
      return false;
    }
  } else if ( isStartBracket( right ) ) {
    bracket = right;
  } else if ( isEndBracket( left ) ) {
    start.setCol(start.col() - 1);
    bracket = left;
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
  int lines = 0;
  end = start;

  while( true ) {
    /* Increment or decrement, check base cases */
    if( forward ) {
      end.setCol(end.col() + 1);
      if( end.col() >= lineLength( end.line() ) ) {
        if( end.line() >= (int)lastLine() )
          return false;
        end.setPos(end.line() + 1, 0);
        textLine = m_buffer->plainLine( end.line() );
        lines++;
      }
    } else {
      end.setCol(end.col() - 1);
      if( end.col() < 0 ) {
        if( end.line() <= 0 )
          return false;
        end.setLine(end.line() - 1);
        end.setCol(lineLength( end.line() ) - 1);
        textLine = m_buffer->plainLine( end.line() );
        lines++;
      }
    }

    if ((maxLines != -1) && (lines > maxLines))
      return false;

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

void KateDocument::setDocName (QString name )
{
  if ( name == m_docName )
    return;

  if ( !name.isEmpty() )
  {
    // TODO check for similarly named documents
    m_docName = name;
    updateFileType (KateFactory::self()->fileTypeManager()->fileType (this));
    emit nameChanged((Kate::Document *) this);
    return;
  }

  // if the name is set, and starts with FILENAME, it should not be changed!
  if ( ! url().isEmpty() && m_docName.startsWith( url().filename() ) ) return;

  int count = -1;

  for (uint z=0; z < KateFactory::self()->documents()->count(); z++)
  {
    if ( (KateFactory::self()->documents()->at(z) != this) && (KateFactory::self()->documents()->at(z)->url().filename() == url().filename()) )
      if ( KateFactory::self()->documents()->at(z)->m_docNameNumber > count )
        count = KateFactory::self()->documents()->at(z)->m_docNameNumber;
  }

  m_docNameNumber = count + 1;

  m_docName = url().filename();

  if (m_docName.isEmpty())
    m_docName = i18n ("Untitled");

  if (m_docNameNumber > 0)
    m_docName = QString(m_docName + " (%1)").arg(m_docNameNumber+1);

  updateFileType (KateFactory::self()->fileTypeManager()->fileType (this));
  emit nameChanged ((Kate::Document *) this);
}

void KateDocument::slotModifiedOnDisk( Kate::View * /*v*/ )
{
  if ( m_isasking < 0 )
  {
    m_isasking = 0;
    return;
  }

  if ( !s_fileChangedDialogsActivated || m_isasking )
    return;

  if (m_modOnHd && !url().isEmpty())
  {
    m_isasking = 1;

    KateModOnHdPrompt p( this, m_modOnHdReason, reasonedMOHString(), widget() );
    switch ( p.exec() )
    {
      case KateModOnHdPrompt::Save:
      {
        m_modOnHd = false;
        KEncodingFileDialog::Result res=KEncodingFileDialog::getSaveURLAndEncoding(config()->encoding(),
            url().url(),QString::null,widget(),i18n("Save File"));

        kdDebug(13020)<<"got "<<res.URLs.count()<<" URLs"<<endl;
        if( ! res.URLs.isEmpty() && ! res.URLs.first().isEmpty() && checkOverwrite( res.URLs.first() ) )
        {
          setEncoding( res.encoding );

          if( ! saveAs( res.URLs.first() ) )
          {
            KMessageBox::error( widget(), i18n("Save failed") );
            m_modOnHd = true;
          }
          else
            emit modifiedOnDisc( this, false, 0 );
        }
        else // the save as dialog was cancelled, we are still modified on disk
        {
          m_modOnHd = true;
        }

        m_isasking = 0;
        break;
      }

      case KateModOnHdPrompt::Reload:
        m_modOnHd = false;
        emit modifiedOnDisc( this, false, 0 );
        reloadFile();
        m_isasking = 0;
        break;

      case KateModOnHdPrompt::Ignore:
        m_modOnHd = false;
        emit modifiedOnDisc( this, false, 0 );
        m_isasking = 0;
        break;

      case KateModOnHdPrompt::Overwrite:
        m_modOnHd = false;
        emit modifiedOnDisc( this, false, 0 );
        m_isasking = 0;
        save();
        break;

      default:               // cancel: ignore next focus event
        m_isasking = -1;
    }
  }
}

void KateDocument::setModifiedOnDisk( int reason )
{
  m_modOnHdReason = reason;
  m_modOnHd = (reason > 0);
  emit modifiedOnDisc( this, (reason > 0), reason );
}

class KateDocumentTmpMark
{
  public:
    QString line;
    KTextEditor::Mark mark;
};

void KateDocument::reloadFile()
{
  if ( !url().isEmpty() )
  {
    if (m_modOnHd && s_fileChangedDialogsActivated)
    {
      int i = KMessageBox::warningYesNoCancel
                (0, reasonedMOHString() + "\n\n" + i18n("What do you want to do?"),
                i18n("File Was Changed on Disk"), i18n("&Reload File"), i18n("&Ignore Changes"));

      if ( i != KMessageBox::Yes)
      {
        if (i == KMessageBox::No)
        {
          m_modOnHd = false;
          m_modOnHdReason = 0;
          emit modifiedOnDisc (this, m_modOnHd, 0);
        }

        return;
      }
    }

    QValueList<KateDocumentTmpMark> tmp;

    for( QIntDictIterator<KTextEditor::Mark> it( m_marks ); it.current(); ++it )
    {
      KateDocumentTmpMark m;

      m.line = textLine (it.current()->line);
      m.mark = *it.current();

      tmp.append (m);
    }

    uint mode = hlMode ();
    bool byUser = hlSetByUser;

    m_storedVariables.clear();

    m_reloading = true;
    KateDocument::openURL( url() );
    m_reloading = false;

    for (uint z=0; z < tmp.size(); z++)
    {
      if (z < numLines())
      {
        if (textLine(tmp[z].mark.line) == tmp[z].line)
          setMark (tmp[z].mark.line, tmp[z].mark.type);
      }
    }

    if (byUser)
      setHlMode (mode);
  }
}

void KateDocument::flush ()
{
  closeURL ();
}

void KateDocument::setWordWrap (bool on)
{
  config()->setWordWrap (on);
}

bool KateDocument::wordWrap ()
{
  return config()->wordWrap ();
}

void KateDocument::setWordWrapAt (uint col)
{
  config()->setWordWrapAt (col);
}

unsigned int KateDocument::wordWrapAt ()
{
  return config()->wordWrapAt ();
}

void KateDocument::applyWordWrap ()
{
  /*if (hasSelection())
    wrapText (selectStart.line(), selectEnd.line());
  else
  wrapText (0, lastLine()); */
}

void KateDocument::setPageUpDownMovesCursor (bool on)
{
  config()->setPageUpDownMovesCursor (on);
}

bool KateDocument::pageUpDownMovesCursor ()
{
  return config()->pageUpDownMovesCursor ();
}

void KateDocument::dumpRegionTree()
{
  m_buffer->foldingTree()->debugDump();
}
//END

//BEGIN KTextEditor::CursorInterface stuff

KTextEditor::Cursor *KateDocument::createCursor ( )
{
  return new KateSuperCursor (this, false, 0, 0, this);
}

void KateDocument::tagArbitraryLines(KateView* view, KateSuperRange* range)
{
  if (view)
    view->tagLines(range->start(), range->end());
  else
    tagLines(range->start(), range->end());
}

void KateDocument::lineInfo (KateLineInfo *info, unsigned int line)
{
  m_buffer->lineInfo(info,line);
}

KateCodeFoldingTree *KateDocument::foldingTree ()
{
  return m_buffer->foldingTree();
}

void KateDocument::setEncoding (const QString &e)
{
  m_config->setEncoding(e);
}

QString KateDocument::encoding() const
{
  return m_config->encoding();
}

void KateDocument::updateConfig ()
{
  emit undoChanged ();
  tagAll();

  for (KateView * view = m_views.first(); view != 0L; view = m_views.next() )
  {
    view->updateDocumentConfig ();
  }

  // switch indenter if needed
  if (m_indenter->modeNumber() != m_config->indentationMode())
  {
    delete m_indenter;
    m_indenter = KateAutoIndent::createIndenter ( this, m_config->indentationMode() );
  }

  m_indenter->updateConfig();

  m_buffer->setTabWidth (config()->tabWidth());

  // plugins
  for (uint i=0; i<KateFactory::self()->plugins().count(); i++)
  {
    if (config()->plugin (i))
      loadPlugin (i);
    else
      unloadPlugin (i);
  }
}

//BEGIN Variable reader
// "local variable" feature by anders, 2003
/* TODO
      add config options (how many lines to read, on/off)
      add interface for plugins/apps to set/get variables
      add view stuff
*/
QRegExp KateDocument::kvLine = QRegExp("kate:(.*)");
QRegExp KateDocument::kvVar = QRegExp("([\\w\\-]+)\\s+([^;]+)");

void KateDocument::readVariables(bool onlyViewAndRenderer)
{
  if (!onlyViewAndRenderer)
    m_config->configStart();

  // views!
  KateView *v;
  for (v = m_views.first(); v != 0L; v= m_views.next() )
  {
    v->config()->configStart();
    v->renderer()->config()->configStart();
  }
  // read a number of lines in the top/bottom of the document
  for (uint i=0; i < QMIN( 9, numLines() ); ++i )
  {
    readVariableLine( textLine( i ), onlyViewAndRenderer );
  }
  if ( numLines() > 10 )
  {
    for ( uint i = QMAX(10, numLines() - 10); i < numLines(); ++i )
    {
      readVariableLine( textLine( i ), onlyViewAndRenderer );
    }
  }

  if (!onlyViewAndRenderer)
    m_config->configEnd();

  for (v = m_views.first(); v != 0L; v= m_views.next() )
  {
    v->config()->configEnd();
    v->renderer()->config()->configEnd();
  }
}

void KateDocument::readVariableLine( QString t, bool onlyViewAndRenderer )
{
  if ( kvLine.search( t ) > -1 )
  {
    QStringList vvl; // view variable names
    vvl << "dynamic-word-wrap" << "dynamic-word-wrap-indicators"
        << "line-numbers" << "icon-border" << "folding-markers"
        << "bookmark-sorting" << "auto-center-lines"
        << "icon-bar-color"
        // renderer
        << "background-color" << "selection-color"
        << "current-line-color" << "bracket-highlight-color"
        << "word-wrap-marker-color"
        << "font" << "font-size" << "scheme";
    int p( 0 );
    QString s = kvLine.cap(1);
    QString  var, val;
    while ( (p = kvVar.search( s, p )) > -1 )
    {
      p += kvVar.matchedLength();
      var = kvVar.cap( 1 );
      val = kvVar.cap( 2 ).stripWhiteSpace();
      bool state; // store booleans here
      int n; // store ints here

      // only apply view & renderer config stuff
      if (onlyViewAndRenderer)
      {
        if ( vvl.contains( var ) ) // FIXME define above
          setViewVariable( var, val );
      }
      else
      {
        // BOOL  SETTINGS
        if ( var == "word-wrap" && checkBoolValue( val, &state ) )
          setWordWrap( state ); // ??? FIXME CHECK
        else if ( var == "block-selection"  && checkBoolValue( val, &state ) )
          setBlockSelectionMode( state );
        // KateConfig::configFlags
        // FIXME should this be optimized to only a few calls? how?
        else if ( var == "backspace-indents" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfBackspaceIndents, state );
        else if ( var == "replace-tabs" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfReplaceTabsDyn, state );
        else if ( var == "remove-trailing-space" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfRemoveTrailingDyn, state );
        else if ( var == "wrap-cursor" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfWrapCursor, state );
        else if ( var == "auto-brackets" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfAutoBrackets, state );
        else if ( var == "overwrite-mode" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfOvr, state );
        else if ( var == "keep-indent-profile" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfKeepIndentProfile, state );
        else if ( var == "keep-extra-spaces" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfKeepExtraSpaces, state );
        else if ( var == "tab-indents" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfTabIndents, state );
        else if ( var == "show-tabs" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfShowTabs, state );
        else if ( var == "space-indent" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfSpaceIndent, state );
        else if ( var == "smart-home" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfSmartHome, state );
        else if ( var == "replace-trailing-space-save" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfRemoveSpaces, state );
        else if ( var == "auto-insert-doxygen" && checkBoolValue( val, &state) )
          m_config->setConfigFlags( KateDocumentConfig::cfDoxygenAutoTyping, state);
        else if ( var == "mixed-indent" && checkBoolValue( val, &state ) )
          m_config->setConfigFlags( KateDocumentConfig::cfMixedIndent, state );

        // INTEGER SETTINGS
        else if ( var == "tab-width" && checkIntValue( val, &n ) )
          m_config->setTabWidth( n );
        else if ( var == "indent-width"  && checkIntValue( val, &n ) )
          m_config->setIndentationWidth( n );
        else if ( var == "indent-mode" )
        {
          if ( checkIntValue( val, &n ) )
            m_config->setIndentationMode( n );
          else
            m_config->setIndentationMode( KateAutoIndent::modeNumber( val) );
        }
        else if ( var == "word-wrap-column" && n > 0  && checkIntValue( val, &n ) ) // uint, but hard word wrap at 0 will be no fun ;)
          m_config->setWordWrapAt( n );
        else if ( var == "undo-steps"  && n >= 0  && checkIntValue( val, &n ) )
          setUndoSteps( n );

        // STRING SETTINGS
        else if ( var == "eol" || var == "end-of-line" )
        {
          QStringList l;
          l << "unix" << "dos" << "mac";
          if ( (n = l.findIndex( val.lower() )) != -1 )
            m_config->setEol( n );
        }
        else if ( var == "encoding" )
          m_config->setEncoding( val );
        else if ( var == "syntax" || var == "hl" )
        {
          for ( uint i=0; i < hlModeCount(); i++ )
          {
            if ( hlModeName( i ).lower() == val.lower() )
            {
              setHlMode( i );
              break;
            }
          }
        }

        // VIEW SETTINGS
        else if ( vvl.contains( var ) )
          setViewVariable( var, val );
        else
        {
          m_storedVariables.insert( var, val );
          emit variableChanged( var, val );
        }
      }
    }
  }
}

void KateDocument::setViewVariable( QString var, QString val )
{
  KateView *v;
  bool state;
  int n;
  QColor c;
  for (v = m_views.first(); v != 0L; v= m_views.next() )
  {
    if ( var == "dynamic-word-wrap" && checkBoolValue( val, &state ) )
      v->config()->setDynWordWrap( state );
    else if ( var == "persistent-selection" && checkBoolValue( val, &state ) )
      v->config()->setPersistentSelection( state );
    //else if ( var = "dynamic-word-wrap-indicators" )
    else if ( var == "line-numbers" && checkBoolValue( val, &state ) )
      v->config()->setLineNumbers( state );
    else if (var == "icon-border" && checkBoolValue( val, &state ) )
      v->config()->setIconBar( state );
    else if (var == "folding-markers" && checkBoolValue( val, &state ) )
      v->config()->setFoldingBar( state );
    else if ( var == "auto-center-lines" && checkIntValue( val, &n ) )
      v->config()->setAutoCenterLines( n ); // FIXME uint, > N ??
    else if ( var == "icon-bar-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setIconBarColor( c );
    // RENDERER
    else if ( var == "background-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setBackgroundColor( c );
    else if ( var == "selection-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setSelectionColor( c );
    else if ( var == "current-line-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setHighlightedLineColor( c );
    else if ( var == "bracket-highlight-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setHighlightedBracketColor( c );
    else if ( var == "word-wrap-marker-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setWordWrapMarkerColor( c );
    else if ( var == "font" || ( var == "font-size" && checkIntValue( val, &n ) ) )
    {
      QFont _f( *v->renderer()->config()->font(  ) );

      if ( var == "font" )
      {
        _f.setFamily( val );
        _f.setFixedPitch( QFont( val ).fixedPitch() );
      }
      else
        _f.setPointSize( n );

      v->renderer()->config()->setFont( _f );
    }
    else if ( var == "scheme" )
    {
      v->renderer()->config()->setSchema( KateFactory::self()->schemaManager()->number( val ) );
    }
  }
}

bool KateDocument::checkBoolValue( QString val, bool *result )
{
  val = val.stripWhiteSpace().lower();
  QStringList l;
  l << "1" << "on" << "true";
  if ( l.contains( val ) )
  {
    *result = true;
    return true;
  }
  l.clear();
  l << "0" << "off" << "false";
  if ( l.contains( val ) )
  {
    *result = false;
    return true;
  }
  return false;
}

bool KateDocument::checkIntValue( QString val, int *result )
{
  bool ret( false );
  *result = val.toInt( &ret );
  return ret;
}

bool KateDocument::checkColorValue( QString val, QColor &c )
{
  c.setNamedColor( val );
  return c.isValid();
}

// KTextEditor::variable
QString KateDocument::variable( const QString &name ) const
{
  if ( m_storedVariables.contains( name ) )
    return m_storedVariables[ name ];

  return "";
}

//END

void KateDocument::slotModOnHdDirty (const QString &path)
{
  if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != 1))
  {
    // compare md5 with the one we have (if we have one)
    if ( ! m_digest.isEmpty() )
    {
      QCString tmp;
      if ( createDigest( tmp ) && tmp == m_digest )
        return;
    }

    m_modOnHd = true;
    m_modOnHdReason = 1;

    // reenable dialog if not running atm
    if (m_isasking == -1)
      m_isasking = false;

    emit modifiedOnDisc (this, m_modOnHd, m_modOnHdReason);
  }
}

void KateDocument::slotModOnHdCreated (const QString &path)
{
  if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != 2))
  {
    m_modOnHd = true;
    m_modOnHdReason = 2;

    // reenable dialog if not running atm
    if (m_isasking == -1)
      m_isasking = false;

    emit modifiedOnDisc (this, m_modOnHd, m_modOnHdReason);
  }
}

void KateDocument::slotModOnHdDeleted (const QString &path)
{
  if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != 3))
  {
    m_modOnHd = true;
    m_modOnHdReason = 3;

    // reenable dialog if not running atm
    if (m_isasking == -1)
      m_isasking = false;

    emit modifiedOnDisc (this, m_modOnHd, m_modOnHdReason);
  }
}

bool KateDocument::createDigest( QCString &result )
{
  bool ret = false;
  result = "";
  if ( url().isLocalFile() )
  {
    QFile f ( url().path() );
    if ( f.open( IO_ReadOnly) )
    {
      KMD5 md5;
      ret = md5.update( f );
      md5.hexDigest( result );
      f.close();
    }
  }
  return ret;
}

QString KateDocument::reasonedMOHString() const
{
  switch( m_modOnHdReason )
  {
    case 1:
      return i18n("The file '%1' was modified by another program.").arg( url().prettyURL() );
      break;
    case 2:
      return i18n("The file '%1' was created by another program.").arg( url().prettyURL() );
      break;
    case 3:
      return i18n("The file '%1' was deleted by another program.").arg( url().prettyURL() );
      break;
    default:
      return QString();
  }
}

void KateDocument::removeTrailingSpace( uint line )
{
  // remove trailing spaces from left line if required
  if ( config()->configFlags() & KateDocumentConfig::cfRemoveTrailingDyn )
  {
    KateTextLine::Ptr ln = kateTextLine( line );

    if ( ! ln ) return;

    if ( line == activeView()->cursorLine()
         && activeView()->cursorColumnReal() >= (uint)QMAX(0,ln->lastChar()) )
      return;

    if ( ln->length() )
    {
      uint p = ln->lastChar() + 1;
      uint l = ln->length() - p;
      if ( l )
        editRemoveText( line, p, l);
    }
  }
}

void KateDocument::updateFileType (int newType, bool user)
{
  if (user || !m_fileTypeSetByUser)
  {
    const KateFileType *t = 0;
    if ((newType == -1) || (t = KateFactory::self()->fileTypeManager()->fileType (newType)))
    {
      m_fileType = newType;

      if (t)
      {
        m_config->configStart();
        // views!
        KateView *v;
        for (v = m_views.first(); v != 0L; v= m_views.next() )
        {
          v->config()->configStart();
          v->renderer()->config()->configStart();
        }

        readVariableLine( t->varLine );

        m_config->configEnd();
        for (v = m_views.first(); v != 0L; v= m_views.next() )
        {
          v->config()->configEnd();
          v->renderer()->config()->configEnd();
        }
      }
    }
  }
}

uint KateDocument::documentNumber () const
{
  return KTextEditor::Document::documentNumber ();
}




void KateDocument::slotQueryClose_save(bool *handled, bool* abortClosing) {
      *handled=true;
      *abortClosing=true;
      if (m_url.isEmpty())
      {
        KEncodingFileDialog::Result res=KEncodingFileDialog::getSaveURLAndEncoding(config()->encoding(),
                QString::null,QString::null,0,i18n("Save File"));

        if( res.URLs.isEmpty() || !checkOverwrite( res.URLs.first() ) ) {
                *abortClosing=true;
                return;
        }
        setEncoding( res.encoding );
          saveAs( res.URLs.first() );
        *abortClosing=false;
      }
      else
      {
          save();
          *abortClosing=false;
      }

}

bool KateDocument::checkOverwrite( KURL u )
{
  if( !u.isLocalFile() )
    return true;

  QFileInfo info( u.path() );
  if( !info.exists() )
    return true;

  return KMessageBox::Cancel != KMessageBox::warningContinueCancel( 0,
    i18n( "A file named \"%1\" already exists. "
          "Are you sure you want to overwrite it?" ).arg( info.fileName() ),
    i18n( "Overwrite File?" ),
    i18n( "&Overwrite" ) );
}

void KateDocument::setDefaultEncoding (const QString &encoding)
{
  s_defaultEncoding = encoding;
}

//BEGIN KTextEditor::TemplateInterface
bool KateDocument::insertTemplateTextImplementation ( uint line, uint column, const QString &templateString, const QMap<QString,QString> &initialValues, QWidget *) {
      return (new KateTemplateHandler(this,line,column,templateString,initialValues))->initOk();
}

void KateDocument::testTemplateCode() {
  int col=activeView()->cursorColumn();
  int line=activeView()->cursorLine();
  insertTemplateText(line,col,"for ${index} \\${NOPLACEHOLDER} ${index} ${blah} ${fullname} \\$${Placeholder} \\${${PLACEHOLDER2}}\n next line:${ANOTHERPLACEHOLDER} $${DOLLARBEFOREPLACEHOLDER} {NOTHING} {\n${cursor}\n}",QMap<QString,QString>());
}

bool KateDocument::invokeTabInterceptor(KKey key) {
  if (m_tabInterceptor) return (*m_tabInterceptor)(key);
  return false;
}

bool KateDocument::setTabInterceptor(KateKeyInterceptorFunctor *interceptor) {
  if (m_tabInterceptor) return false;
  m_tabInterceptor=interceptor;
  return true;
}

bool KateDocument::removeTabInterceptor(KateKeyInterceptorFunctor *interceptor) {
  if (m_tabInterceptor!=interceptor) return false;
  m_tabInterceptor=0;
  return true;
}
//END KTextEditor::TemplateInterface

//BEGIN DEPRECATED STUFF
 bool KateDocument::setSelection ( uint startLine, uint startCol, uint endLine, uint endCol )
{ if (m_activeView) return m_activeView->setSelection (startLine, startCol, endLine, endCol); return false; }

 bool KateDocument::clearSelection ()
 { if (m_activeView) return m_activeView->clearSelection(); return false; }

 bool KateDocument::hasSelection () const
 { if (m_activeView) return m_activeView->hasSelection (); return false; }

 QString KateDocument::selection () const
 { if (m_activeView) return m_activeView->selection (); return QString(""); }

 bool KateDocument::removeSelectedText ()
 { if (m_activeView) return m_activeView->removeSelectedText (); return false; }

 bool KateDocument::selectAll()
 { if (m_activeView) return m_activeView->selectAll (); return false; }

 int KateDocument::selStartLine()
 { if (m_activeView) return m_activeView->selStartLine (); return 0; }

 int KateDocument::selStartCol()
 { if (m_activeView) return m_activeView->selStartCol (); return 0; }

 int KateDocument::selEndLine()
 { if (m_activeView) return m_activeView->selEndLine (); return 0; }

 int KateDocument::selEndCol()
 { if (m_activeView) return m_activeView->selEndCol (); return 0; }

 bool KateDocument::blockSelectionMode ()
    { if (m_activeView) return m_activeView->blockSelectionMode (); return false; }

bool KateDocument::setBlockSelectionMode (bool on)
    { if (m_activeView) return m_activeView->setBlockSelectionMode (on); return false; }

bool KateDocument::toggleBlockSelectionMode ()
    { if (m_activeView) return m_activeView->toggleBlockSelectionMode (); return false; }
//END DEPRECATED

//END DEPRECATED STUFF

// kate: space-indent on; indent-width 2; replace-tabs on;
