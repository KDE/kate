/* This file is part of the KDE libraries
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

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
   Boston, MA 02110-1301, USA.
*/

#include "kateconfig.h"

#include "kateglobal.h"
#include "katerenderer.h"
#include "kateview.h"
#include "katedocument.h"
#include "katefont.h"
#include "kateschema.h"

#include <math.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobalsettings.h>
#include <kcharsets.h>
#include <klocale.h>
#include <kfinddialog.h>
#include <kreplacedialog.h>
#include <kinstance.h>
#include <kstaticdeleter.h>
#include <kfind.h>
#include <kdebug.h>

#include <QTextCodec>

//BEGIN KateConfig
KateConfig::KateConfig ()
 : configSessionNumber (0), configIsRunning (false)
{
}

KateConfig::~KateConfig ()
{
}

void KateConfig::configStart ()
{
  configSessionNumber++;

  if (configSessionNumber > 1)
    return;

  configIsRunning = true;
}

void KateConfig::configEnd ()
{
  if (configSessionNumber == 0)
    return;

  configSessionNumber--;

  if (configSessionNumber > 0)
    return;

  configIsRunning = false;

  updateConfig ();
}
//END

//BEGIN KateDocumentConfig
KateDocumentConfig *KateDocumentConfig::s_global = 0;
KateViewConfig *KateViewConfig::s_global = 0;
KateRendererConfig *KateRendererConfig::s_global = 0;

KateDocumentConfig::KateDocumentConfig ()
 : m_tabWidth (8),
   m_indentationWidth (2),
   m_wordWrapAt (80),
   m_configFlags (0),
   m_plugins (KateGlobal::self()->plugins().count()),
   m_tabWidthSet (true),
   m_indentationWidthSet (true),
   m_indentationModeSet (true),
   m_wordWrapSet (true),
   m_wordWrapAtSet (true),
   m_pageUpDownMovesCursorSet (true),
   m_undoStepsSet (true),
   m_configFlagsSet (0xFFFF),
   m_encodingSet (true),
   m_eolSet (true),
   m_allowEolDetectionSet (true),
   m_backupFlagsSet (true),
   m_searchDirConfigDepthSet (true),
   m_backupPrefixSet (true),
   m_backupSuffixSet (true),
   m_pluginsSet (m_plugins.size()),
   m_doc (0)
{
  s_global = this;

  // init plugin array
  m_plugins.fill (false);
  m_pluginsSet.fill (true);

  // init with defaults from config or really hardcoded ones
  KConfig *config = kapp->config();
  config->setGroup("Kate Document Defaults");
  readConfig (config);
}

KateDocumentConfig::KateDocumentConfig (KateDocument *doc)
 : m_configFlags (0),
   m_plugins (KateGlobal::self()->plugins().count()),
   m_tabWidthSet (false),
   m_indentationWidthSet (false),
   m_indentationModeSet (false),
   m_wordWrapSet (false),
   m_wordWrapAtSet (false),
   m_pageUpDownMovesCursorSet (false),
   m_undoStepsSet (false),
   m_configFlagsSet (0),
   m_encodingSet (false),
   m_eolSet (false),
   m_allowEolDetectionSet (false),
   m_backupFlagsSet (false),
   m_searchDirConfigDepthSet (false),
   m_backupPrefixSet (false),
   m_backupSuffixSet (false),
   m_pluginsSet (m_plugins.size()),
   m_doc (doc)
{
  // init plugin array
  m_plugins.fill (false);
  m_pluginsSet.fill (false);
}

KateDocumentConfig::~KateDocumentConfig ()
{
}

void KateDocumentConfig::readConfig (KConfig *config)
{
  configStart ();

  setTabWidth (config->readNumEntry("Tab Width", 8));

  setIndentationWidth (config->readNumEntry("Indentation Width", 2));

  setIndentationMode (config->readNumEntry("Indentation Mode", KateDocumentConfig::imNone));

  setWordWrap (config->readBoolEntry("Word Wrap", false));
  setWordWrapAt (config->readNumEntry("Word Wrap Column", 80));
  setPageUpDownMovesCursor (config->readNumEntry("PageUp/PageDown Moves Cursor", false));
  setUndoSteps(config->readNumEntry("Undo Steps", 0));

  setConfigFlags (config->readNumEntry("Basic Config Flags", KateDocumentConfig::cfTabIndents
    | KateDocumentConfig::cfKeepIndentProfile
    | KateDocumentConfig::cfWrapCursor
    | KateDocumentConfig::cfShowTabs
    | KateDocumentConfig::cfSmartHome));

  setEncoding (config->readEntry("Encoding", ""));

  setEol (config->readNumEntry("End of Line", 0));
  setAllowEolDetection (config->readNumEntry("Allow End of Line Detection", true));

  setBackupFlags (config->readNumEntry("Backup Config Flags", 1));

  setSearchDirConfigDepth (config->readNumEntry("Search Dir Config Depth", -1));

  setBackupPrefix (config->readEntry("Backup Prefix", QString ("")));

  setBackupSuffix (config->readEntry("Backup Suffix", QString ("~")));

  // plugins
  for (int i=0; i<KateGlobal::self()->plugins().count(); i++)
    setPlugin (i, config->readBoolEntry("KTextEditor Plugin " + (KateGlobal::self()->plugins())[i]->library(), false));

  configEnd ();
}

void KateDocumentConfig::writeConfig (KConfig *config)
{
  config->writeEntry("Tab Width", tabWidth());

  config->writeEntry("Indentation Width", indentationWidth());
  config->writeEntry("Indentation Mode", indentationMode());

  config->writeEntry("Word Wrap", wordWrap());
  config->writeEntry("Word Wrap Column", wordWrapAt());

  config->writeEntry("PageUp/PageDown Moves Cursor", pageUpDownMovesCursor());

  config->writeEntry("Undo Steps", undoSteps());

  config->writeEntry("Basic Config Flags", configFlags());

  config->writeEntry("Encoding", encoding());

  config->writeEntry("End of Line", eol());
  config->writeEntry("Allow End of Line Detection", allowEolDetection());

  config->writeEntry("Backup Config Flags", backupFlags());

  config->writeEntry("Search Dir Config Depth", searchDirConfigDepth());

  config->writeEntry("Backup Prefix", backupPrefix());

  config->writeEntry("Backup Suffix", backupSuffix());

  // plugins
  for (int i=0; i<KateGlobal::self()->plugins().count(); i++)
    config->writeEntry("KTextEditor Plugin " + (KateGlobal::self()->plugins())[i]->library(), plugin(i));
}

void KateDocumentConfig::updateConfig ()
{
  if (m_doc)
  {
    m_doc->updateConfig ();
    return;
  }

  if (isGlobal())
  {
    for (int z=0; z < KateGlobal::self()->kateDocuments().size(); ++z)
      (KateGlobal::self()->kateDocuments())[z]->updateConfig ();
  }
}

int KateDocumentConfig::tabWidth () const
{
  if (m_tabWidthSet || isGlobal())
    return m_tabWidth;

  return s_global->tabWidth();
}

void KateDocumentConfig::setTabWidth (int tabWidth)
{
  if (tabWidth < 1)
    return;

  configStart ();

  m_tabWidthSet = true;
  m_tabWidth = tabWidth;

  configEnd ();
}

int KateDocumentConfig::indentationWidth () const
{
  if (m_indentationWidthSet || isGlobal())
    return m_indentationWidth;

  return s_global->indentationWidth();
}

void KateDocumentConfig::setIndentationWidth (int indentationWidth)
{
  if (indentationWidth < 1)
    return;

  configStart ();

  m_indentationWidthSet = true;
  m_indentationWidth = indentationWidth;

  configEnd ();
}

uint KateDocumentConfig::indentationMode () const
{
  if (m_indentationModeSet || isGlobal())
    return m_indentationMode;

  return s_global->indentationMode();
}

void KateDocumentConfig::setIndentationMode (uint indentationMode)
{
  configStart ();

  m_indentationModeSet = true;
  m_indentationMode = indentationMode;

  configEnd ();
}

bool KateDocumentConfig::wordWrap () const
{
  if (m_wordWrapSet || isGlobal())
    return m_wordWrap;

  return s_global->wordWrap();
}

void KateDocumentConfig::setWordWrap (bool on)
{
  configStart ();

  m_wordWrapSet = true;
  m_wordWrap = on;

  configEnd ();
}

unsigned int KateDocumentConfig::wordWrapAt () const
{
  if (m_wordWrapAtSet || isGlobal())
    return m_wordWrapAt;

  return s_global->wordWrapAt();
}

void KateDocumentConfig::setWordWrapAt (unsigned int col)
{
  if (col < 1)
    return;

  configStart ();

  m_wordWrapAtSet = true;
  m_wordWrapAt = col;

  configEnd ();
}

uint KateDocumentConfig::undoSteps () const
{
  if (m_undoStepsSet || isGlobal())
    return m_undoSteps;

  return s_global->undoSteps();
}

void KateDocumentConfig::setUndoSteps (uint undoSteps)
{
  configStart ();

  m_undoStepsSet = true;
  m_undoSteps = undoSteps;

  configEnd ();
}

bool KateDocumentConfig::pageUpDownMovesCursor () const
{
  if (m_pageUpDownMovesCursorSet || isGlobal())
    return m_pageUpDownMovesCursor;

  return s_global->pageUpDownMovesCursor();
}

void KateDocumentConfig::setPageUpDownMovesCursor (bool on)
{
  configStart ();

  m_pageUpDownMovesCursorSet = true;
  m_pageUpDownMovesCursor = on;

  configEnd ();
}

uint KateDocumentConfig::configFlags () const
{
  if (isGlobal())
    return m_configFlags;

  return ((s_global->configFlags() & ~ m_configFlagsSet) | m_configFlags);
}

void KateDocumentConfig::setConfigFlags (KateDocumentConfig::ConfigFlags flag, bool enable)
{
  configStart ();

  m_configFlagsSet |= flag;

  if (enable)
    m_configFlags = m_configFlags | flag;
  else
    m_configFlags = m_configFlags & ~ flag;

  configEnd ();
}

void KateDocumentConfig::setConfigFlags (uint fullFlags)
{
  configStart ();

  m_configFlagsSet = 0xFFFF;
  m_configFlags = fullFlags;

  configEnd ();
}

const QString &KateDocumentConfig::encoding () const
{
  if (m_encodingSet || isGlobal())
    return m_encoding;

  return s_global->encoding();
}

QTextCodec *KateDocumentConfig::codec () const
{
  if (m_encodingSet || isGlobal())
  {
    if (m_encoding.isEmpty() && isGlobal())
      return KGlobal::locale()->codecForEncoding();
    else if (m_encoding.isEmpty())
      return s_global->codec ();
    else
      return KGlobal::charsets()->codecForName (m_encoding);
  }

  return s_global->codec ();
}

bool KateDocumentConfig::setEncoding (const QString &encoding)
{
  QString enc = encoding;

  if (!enc.isEmpty())
  {
    bool found = false;
    QTextCodec *codec = KGlobal::charsets()->codecForName (encoding, found);

    if (!found || !codec)
      return false;

    enc = codec->name();
  }

  configStart ();

  m_encodingSet = true;
  m_encoding = enc;

  configEnd ();

  return true;
}

bool KateDocumentConfig::isSetEncoding () const
{
  return m_encodingSet;
}

int KateDocumentConfig::eol () const
{
  if (m_eolSet || isGlobal())
    return m_eol;

  return s_global->eol();
}

QString KateDocumentConfig::eolString ()
{
  if (eol() == KateDocumentConfig::eolUnix)
    return QString ("\n");
  else if (eol() == KateDocumentConfig::eolDos)
    return QString ("\r\n");
  else if (eol() == KateDocumentConfig::eolMac)
    return QString ("\r");

  return QString ("\n");
}

void KateDocumentConfig::setEol (int mode)
{
  configStart ();

  m_eolSet = true;
  m_eol = mode;

  configEnd ();
}

bool KateDocumentConfig::allowEolDetection () const
{
  if (m_allowEolDetectionSet || isGlobal())
    return m_allowEolDetection;

  return s_global->allowEolDetection();
}

void KateDocumentConfig::setAllowEolDetection (bool on)
{
  configStart ();

  m_allowEolDetectionSet = true;
  m_allowEolDetection = on;

  configEnd ();
}

uint KateDocumentConfig::backupFlags () const
{
  if (m_backupFlagsSet || isGlobal())
    return m_backupFlags;

  return s_global->backupFlags();
}

void KateDocumentConfig::setBackupFlags (uint flags)
 {
  configStart ();

  m_backupFlagsSet = true;
  m_backupFlags = flags;

  configEnd ();
}

const QString &KateDocumentConfig::backupPrefix () const
{
  if (m_backupPrefixSet || isGlobal())
    return m_backupPrefix;

  return s_global->backupPrefix();
}

const QString &KateDocumentConfig::backupSuffix () const
{
  if (m_backupSuffixSet || isGlobal())
    return m_backupSuffix;

  return s_global->backupSuffix();
}

void KateDocumentConfig::setBackupPrefix (const QString &prefix)
{
  configStart ();

  m_backupPrefixSet = true;
  m_backupPrefix = prefix;

  configEnd ();
}

void KateDocumentConfig::setBackupSuffix (const QString &suffix)
{
  configStart ();

  m_backupSuffixSet = true;
  m_backupSuffix = suffix;

  configEnd ();
}

bool KateDocumentConfig::plugin (int index) const
{
  if (index < 0 || index >= m_plugins.size())
    return false;

  if (m_pluginsSet.at(index) || isGlobal())
    return m_plugins.at(index);

  return s_global->plugin (index);
}

void KateDocumentConfig::setPlugin (int index, bool load)
{
  if (index < 0 || index >= m_plugins.size())
    return;

  configStart ();

  m_pluginsSet.setBit(index);
  m_plugins.setBit(index, load);

  configEnd ();
}

int KateDocumentConfig::searchDirConfigDepth () const
{
  if (m_searchDirConfigDepthSet || isGlobal())
    return m_searchDirConfigDepth;

  return s_global->searchDirConfigDepth ();
}

void KateDocumentConfig::setSearchDirConfigDepth (int depth)
{
  configStart ();

  m_searchDirConfigDepthSet = true;
  m_searchDirConfigDepth = depth;

  configEnd ();
}

//END

//BEGIN KateViewConfig
KateViewConfig::KateViewConfig ()
 :
   m_dynWordWrapSet (true),
   m_dynWordWrapIndicatorsSet (true),
   m_dynWordWrapAlignIndentSet (true),
   m_lineNumbersSet (true),
   m_scrollBarMarksSet (true),
   m_iconBarSet (true),
   m_foldingBarSet (true),
   m_bookmarkSortSet (true),
   m_autoCenterLinesSet (true),
   m_searchFlagsSet (true),
   m_cmdLineSet (true),
   m_defaultMarkTypeSet (true),
   m_persistentSelectionSet (true),
   m_textToSearchModeSet (true),
   m_view (0)
{
  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfig *config = kapp->config();
  config->setGroup("Kate View Defaults");
  readConfig (config);
}

KateViewConfig::KateViewConfig (KateView *view)
 :
   m_dynWordWrapSet (false),
   m_dynWordWrapIndicatorsSet (false),
   m_dynWordWrapAlignIndentSet (false),
   m_lineNumbersSet (false),
   m_scrollBarMarksSet (false),
   m_iconBarSet (false),
   m_foldingBarSet (false),
   m_bookmarkSortSet (false),
   m_autoCenterLinesSet (false),
   m_searchFlagsSet (false),
   m_cmdLineSet (false),
   m_defaultMarkTypeSet (false),
   m_persistentSelectionSet (false),
   m_textToSearchModeSet (false),
   m_view (view)
{
}

KateViewConfig::~KateViewConfig ()
{
}

void KateViewConfig::readConfig (KConfig *config)
{
  configStart ();

  setDynWordWrap (config->readBoolEntry( "Dynamic Word Wrap", true ));
  setDynWordWrapIndicators (config->readNumEntry( "Dynamic Word Wrap Indicators", 1 ));
  setDynWordWrapAlignIndent (config->readNumEntry( "Dynamic Word Wrap Align Indent", 80 ));

  setLineNumbers (config->readBoolEntry( "Line Numbers",  false));

  setScrollBarMarks (config->readBoolEntry( "Scroll Bar Marks",  false));

  setIconBar (config->readBoolEntry( "Icon Bar", false ));

  setFoldingBar (config->readBoolEntry( "Folding Bar", true));

  setBookmarkSort (config->readNumEntry( "Bookmark Menu Sorting", 0 ));

  setAutoCenterLines (config->readNumEntry( "Auto Center Lines", 0 ));

  setSearchFlags (config->readNumEntry("Search Config Flags", KFind::FromCursor | KFind::CaseSensitive | KReplaceDialog::PromptOnReplace));

  setCmdLine (config->readBoolEntry( "Command Line", false));

  setDefaultMarkType (config->readNumEntry( "Default Mark Type", KTextEditor::MarkInterface::markType01 ));

  setPersistentSelection (config->readNumEntry( "Persistent Selection", false ));

  setTextToSearchMode (config->readNumEntry( "Text To Search Mode", KateViewConfig::SelectionWord));

  configEnd ();
}

void KateViewConfig::writeConfig (KConfig *config)
{
  config->writeEntry( "Dynamic Word Wrap", dynWordWrap() );
  config->writeEntry( "Dynamic Word Wrap Indicators", dynWordWrapIndicators() );
  config->writeEntry( "Dynamic Word Wrap Align Indent", dynWordWrapAlignIndent() );

  config->writeEntry( "Line Numbers", lineNumbers() );

  config->writeEntry( "Scroll Bar Marks", scrollBarMarks() );

  config->writeEntry( "Icon Bar", iconBar() );

  config->writeEntry( "Folding Bar", foldingBar() );

  config->writeEntry( "Bookmark Menu Sorting", bookmarkSort() );

  config->writeEntry( "Auto Center Lines", autoCenterLines() );

  config->writeEntry("Search Config Flags", searchFlags());

  config->writeEntry("Command Line", cmdLine());

  config->writeEntry("Default Mark Type", defaultMarkType());

  config->writeEntry("Persistent Selection", persistentSelection());

  config->writeEntry("Text To Search Mode", textToSearchMode());
}

void KateViewConfig::updateConfig ()
{
  if (m_view)
  {
    m_view->updateConfig ();
    return;
  }

  if (isGlobal())
  {
    for (int z=0; z < KateGlobal::self()->views().size(); ++z)
      (KateGlobal::self()->views())[z]->updateConfig ();
  }
}

bool KateViewConfig::dynWordWrap () const
{
  if (m_dynWordWrapSet || isGlobal())
    return m_dynWordWrap;

  return s_global->dynWordWrap();
}

void KateViewConfig::setDynWordWrap (bool wrap)
{
  configStart ();

  m_dynWordWrapSet = true;
  m_dynWordWrap = wrap;

  configEnd ();
}

int KateViewConfig::dynWordWrapIndicators () const
{
  if (m_dynWordWrapIndicatorsSet || isGlobal())
    return m_dynWordWrapIndicators;

  return s_global->dynWordWrapIndicators();
}

void KateViewConfig::setDynWordWrapIndicators (int mode)
{
  configStart ();

  m_dynWordWrapIndicatorsSet = true;
  m_dynWordWrapIndicators = QMIN(80, QMAX(0, mode));

  configEnd ();
}

int KateViewConfig::dynWordWrapAlignIndent () const
{
  if (m_dynWordWrapAlignIndentSet || isGlobal())
    return m_dynWordWrapAlignIndent;

  return s_global->dynWordWrapAlignIndent();
}

void KateViewConfig::setDynWordWrapAlignIndent (int indent)
{
  configStart ();

  m_dynWordWrapAlignIndentSet = true;
  m_dynWordWrapAlignIndent = indent;

  configEnd ();
}

bool KateViewConfig::lineNumbers () const
{
  if (m_lineNumbersSet || isGlobal())
    return m_lineNumbers;

  return s_global->lineNumbers();
}

void KateViewConfig::setLineNumbers (bool on)
{
  configStart ();

  m_lineNumbersSet = true;
  m_lineNumbers = on;

  configEnd ();
}

bool KateViewConfig::scrollBarMarks () const
{
  if (m_scrollBarMarksSet || isGlobal())
    return m_scrollBarMarks;

  return s_global->scrollBarMarks();
}

void KateViewConfig::setScrollBarMarks (bool on)
{
  configStart ();

  m_scrollBarMarksSet = true;
  m_scrollBarMarks = on;

  configEnd ();
}

bool KateViewConfig::iconBar () const
{
  if (m_iconBarSet || isGlobal())
    return m_iconBar;

  return s_global->iconBar();
}

void KateViewConfig::setIconBar (bool on)
{
  configStart ();

  m_iconBarSet = true;
  m_iconBar = on;

  configEnd ();
}

bool KateViewConfig::foldingBar () const
{
  if (m_foldingBarSet || isGlobal())
    return m_foldingBar;

  return s_global->foldingBar();
}

void KateViewConfig::setFoldingBar (bool on)
{
  configStart ();

  m_foldingBarSet = true;
  m_foldingBar = on;

  configEnd ();
}

int KateViewConfig::bookmarkSort () const
{
  if (m_bookmarkSortSet || isGlobal())
    return m_bookmarkSort;

  return s_global->bookmarkSort();
}

void KateViewConfig::setBookmarkSort (int mode)
{
  configStart ();

  m_bookmarkSortSet = true;
  m_bookmarkSort = mode;

  configEnd ();
}

int KateViewConfig::autoCenterLines () const
{
  if (m_autoCenterLinesSet || isGlobal())
    return m_autoCenterLines;

  return s_global->autoCenterLines();
}

void KateViewConfig::setAutoCenterLines (int lines)
{
  if (lines < 0)
    return;

  configStart ();

  m_autoCenterLinesSet = true;
  m_autoCenterLines = lines;

  configEnd ();
}

long KateViewConfig::searchFlags () const
{
  if (m_searchFlagsSet || isGlobal())
    return m_searchFlags;

  return s_global->searchFlags();
}

void KateViewConfig::setSearchFlags (long flags)
 {
  configStart ();

  m_searchFlagsSet = true;
  m_searchFlags = flags;

  configEnd ();
}

bool KateViewConfig::cmdLine () const
{
  if (m_cmdLineSet || isGlobal())
    return m_cmdLine;

  return s_global->cmdLine();
}

void KateViewConfig::setCmdLine (bool on)
{
  configStart ();

  m_cmdLineSet = true;
  m_cmdLine = on;

  configEnd ();
}

uint KateViewConfig::defaultMarkType () const
{
  if (m_defaultMarkTypeSet || isGlobal())
    return m_defaultMarkType;

  return s_global->defaultMarkType();
}

void KateViewConfig::setDefaultMarkType (uint type)
{
  configStart ();

  m_defaultMarkTypeSet = true;
  m_defaultMarkType = type;

  configEnd ();
}

bool KateViewConfig::persistentSelection () const
{
  if (m_persistentSelectionSet || isGlobal())
    return m_persistentSelection;

  return s_global->persistentSelection();
}

void KateViewConfig::setPersistentSelection (bool on)
{
  configStart ();

  m_persistentSelectionSet = true;
  m_persistentSelection = on;

  configEnd ();
}

int KateViewConfig::textToSearchMode () const
{
  if (m_textToSearchModeSet || isGlobal())
    return m_textToSearchMode;

  return s_global->textToSearchMode();
}

void KateViewConfig::setTextToSearchMode (int mode)
{
  configStart ();

  m_textToSearchModeSet = true;
  m_textToSearchMode = mode;

  configEnd ();
}

//END

//BEGIN KateRendererConfig
KateRendererConfig::KateRendererConfig ()
 :
   m_font (new KateFontStruct ()),
   m_lineMarkerColor (KTextEditor::MarkInterface::reservedMarkersCount()),
   m_schemaSet (true),
   m_fontSet (true),
   m_wordWrapMarkerSet (true),
   m_showIndentationLinesSet (true),
   m_backgroundColorSet (true),
   m_selectionColorSet (true),
   m_highlightedLineColorSet (true),
   m_highlightedBracketColorSet (true),
   m_wordWrapMarkerColorSet (true),
   m_tabMarkerColorSet(true),
   m_iconBarColorSet (true),
   m_lineNumberColorSet (true),
   m_lineMarkerColorSet (m_lineMarkerColor.size()),
   m_renderer (0)
{
  // init bitarray
  m_lineMarkerColorSet.fill (true);

  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfig *config = kapp->config();
  config->setGroup("Kate Renderer Defaults");
  readConfig (config);
}

KateRendererConfig::KateRendererConfig (KateRenderer *renderer)
 : m_font (0),
   m_lineMarkerColor (KTextEditor::MarkInterface::reservedMarkersCount()),
   m_schemaSet (false),
   m_fontSet (false),
   m_wordWrapMarkerSet (false),
   m_showIndentationLinesSet (false),
   m_backgroundColorSet (false),
   m_selectionColorSet (false),
   m_highlightedLineColorSet (false),
   m_highlightedBracketColorSet (false),
   m_wordWrapMarkerColorSet (false),
   m_tabMarkerColorSet(false),
   m_iconBarColorSet (false),
   m_lineNumberColorSet (false),
   m_lineMarkerColorSet (m_lineMarkerColor.size()),
   m_renderer (renderer)
{
  // init bitarray
  m_lineMarkerColorSet.fill (false);
}

KateRendererConfig::~KateRendererConfig ()
{
  delete m_font;
}

void KateRendererConfig::readConfig (KConfig *config)
{
  configStart ();

  setSchema (KateGlobal::self()->schemaManager()->number (config->readEntry("Schema", KateSchemaManager::normalSchema())));

  setWordWrapMarker (config->readBoolEntry("Word Wrap Marker", false ));

  setShowIndentationLines (config->readBoolEntry( "Show Indentation Lines", false));

  configEnd ();
}

void KateRendererConfig::writeConfig (KConfig *config)
{
  config->writeEntry ("Schema", KateGlobal::self()->schemaManager()->name(schema()));

  config->writeEntry("Word Wrap Marker", wordWrapMarker() );

  config->writeEntry("Show Indentation Lines", showIndentationLines());
}

void KateRendererConfig::updateConfig ()
{
  if (m_renderer)
  {
    m_renderer->updateConfig ();
    return;
  }

  if (isGlobal())
  {
    for (int z=0; z < KateGlobal::self()->views().size(); ++z)
      (KateGlobal::self()->views())[z]->renderer()->updateConfig ();
  }
}

uint KateRendererConfig::schema () const
{
  if (m_schemaSet || isGlobal())
    return m_schema;

  return s_global->schema();
}

void KateRendererConfig::setSchema (uint schema)
{
  configStart ();
  m_schemaSet = true;
  m_schema = schema;
  setSchemaInternal( schema );
  configEnd ();
}

void KateRendererConfig::reloadSchema()
{
  if ( isGlobal() )
    for ( int z=0; z < KateGlobal::self()->views().size(); ++z )
      (KateGlobal::self()->views())[z]->renderer()->config()->reloadSchema();

  else if ( m_renderer && m_schemaSet )
    setSchemaInternal( m_schema );
}

void KateRendererConfig::setSchemaInternal( int schema )
{
  m_schemaSet = true;
  m_schema = schema;

  KConfig *config (KateGlobal::self()->schemaManager()->schema(schema));

  QColor tmp0 (KGlobalSettings::baseColor());
  QColor tmp1 (KGlobalSettings::highlightColor());
  QColor tmp2 (KGlobalSettings::alternateBackgroundColor());
  QColor tmp3 ( "#FFFF99" );
  QColor tmp4 (tmp2.dark());
  QColor tmp5 ( KGlobalSettings::textColor() );
  QColor tmp6 ( "#EAE9E8" );
  QColor tmp7 ( "#000000" );

  m_backgroundColor = config->readColorEntry("Color Background", &tmp0);
  m_backgroundColorSet = true;
  m_selectionColor = config->readColorEntry("Color Selection", &tmp1);
  m_selectionColorSet = true;
  m_highlightedLineColor  = config->readColorEntry("Color Highlighted Line", &tmp2);
  m_highlightedLineColorSet = true;
  m_highlightedBracketColor = config->readColorEntry("Color Highlighted Bracket", &tmp3);
  m_highlightedBracketColorSet = true;
  m_wordWrapMarkerColor = config->readColorEntry("Color Word Wrap Marker", &tmp4);
  m_wordWrapMarkerColorSet = true;
  m_tabMarkerColor = config->readColorEntry("Color Tab Marker", &tmp5);
  m_tabMarkerColorSet = true;
  m_iconBarColor  = config->readColorEntry("Color Icon Bar", &tmp6);
  m_iconBarColorSet = true;
  m_lineNumberColor = config->readColorEntry("Color Line Number", &tmp7);
  m_lineNumberColorSet = true;

    // same std colors like in KateDocument::markColor
  QColor mark[7];
  mark[0] = Qt::blue;
  mark[1] = Qt::red;
  mark[2] = Qt::yellow;
  mark[3] = Qt::magenta;
  mark[4] = Qt::gray;
  mark[5] = Qt::green;
  mark[6] = Qt::red;

  for (int i = 1; i <= KTextEditor::MarkInterface::reservedMarkersCount(); i++) {
    QColor col = config->readColorEntry(QString("Color MarkType%1").arg(i), &mark[i - 1]);
    int index = i-1;
    m_lineMarkerColorSet[index] = true;
    m_lineMarkerColor[index] = col;
  }

  QFont f (KGlobalSettings::fixedFont());

  if (!m_fontSet)
  {
    m_fontSet = true;
    m_font = new KateFontStruct ();
  }

  m_font->setFont(config->readFontEntry("Font", &f));
}

KateFontStruct *KateRendererConfig::fontStruct ()
{
  if (m_fontSet || isGlobal())
    return m_font;

  return s_global->fontStruct ();
}

QFont *KateRendererConfig::font()
{
  return &(fontStruct ()->myFont);
}

KateFontMetrics *KateRendererConfig::fontMetrics()
{
  return &(fontStruct ()->myFontMetrics);
}

void KateRendererConfig::setFont(const QFont &font)
{
  configStart ();

  if (!m_fontSet)
  {
    m_fontSet = true;
    m_font = new KateFontStruct ();
  }

  m_font->setFont(font);

  configEnd ();
}

bool KateRendererConfig::wordWrapMarker () const
{
  if (m_wordWrapMarkerSet || isGlobal())
    return m_wordWrapMarker;

  return s_global->wordWrapMarker();
}

void KateRendererConfig::setWordWrapMarker (bool on)
{
  configStart ();

  m_wordWrapMarkerSet = true;
  m_wordWrapMarker = on;

  configEnd ();
}

const QColor& KateRendererConfig::backgroundColor() const
{
  if (m_backgroundColorSet || isGlobal())
    return m_backgroundColor;

  return s_global->backgroundColor();
}

void KateRendererConfig::setBackgroundColor (const QColor &col)
{
  configStart ();

  m_backgroundColorSet = true;
  m_backgroundColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::selectionColor() const
{
  if (m_selectionColorSet || isGlobal())
    return m_selectionColor;

  return s_global->selectionColor();
}

void KateRendererConfig::setSelectionColor (const QColor &col)
{
  configStart ();

  m_selectionColorSet = true;
  m_selectionColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::highlightedLineColor() const
{
  if (m_highlightedLineColorSet || isGlobal())
    return m_highlightedLineColor;

  return s_global->highlightedLineColor();
}

void KateRendererConfig::setHighlightedLineColor (const QColor &col)
{
  configStart ();

  m_highlightedLineColorSet = true;
  m_highlightedLineColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::lineMarkerColor(KTextEditor::MarkInterface::MarkTypes type) const
{
  int index = 0;
  if (type > 0) { while((type >> index++) ^ 1) {} }
  index -= 1;

  if ( index < 0 || index >= KTextEditor::MarkInterface::reservedMarkersCount() )
  {
    static QColor dummy;
    return dummy;
  }

  if (m_lineMarkerColorSet[index] || isGlobal())
    return m_lineMarkerColor[index];

  return s_global->lineMarkerColor( type );
}

void KateRendererConfig::setLineMarkerColor (const QColor &col, KTextEditor::MarkInterface::MarkTypes type)
{
  int index = static_cast<int>( log(static_cast<double>(type)) / log(2.0) );
  Q_ASSERT( index >= 0 && index < KTextEditor::MarkInterface::reservedMarkersCount() );
  configStart ();

  m_lineMarkerColorSet[index] = true;
  m_lineMarkerColor[index] = col;

  configEnd ();
}

const QColor& KateRendererConfig::highlightedBracketColor() const
{
  if (m_highlightedBracketColorSet || isGlobal())
    return m_highlightedBracketColor;

  return s_global->highlightedBracketColor();
}

void KateRendererConfig::setHighlightedBracketColor (const QColor &col)
{
  configStart ();

  m_highlightedBracketColorSet = true;
  m_highlightedBracketColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::wordWrapMarkerColor() const
{
  if (m_wordWrapMarkerColorSet || isGlobal())
    return m_wordWrapMarkerColor;

  return s_global->wordWrapMarkerColor();
}

void KateRendererConfig::setWordWrapMarkerColor (const QColor &col)
{
  configStart ();

  m_wordWrapMarkerColorSet = true;
  m_wordWrapMarkerColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::tabMarkerColor() const
{
  if (m_tabMarkerColorSet || isGlobal())
    return m_tabMarkerColor;

  return s_global->tabMarkerColor();
}

void KateRendererConfig::setTabMarkerColor (const QColor &col)
{
  configStart ();

  m_tabMarkerColorSet = true;
  m_tabMarkerColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::iconBarColor() const
{
  if (m_iconBarColorSet || isGlobal())
    return m_iconBarColor;

  return s_global->iconBarColor();
}

void KateRendererConfig::setIconBarColor (const QColor &col)
{
  configStart ();

  m_iconBarColorSet = true;
  m_iconBarColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::lineNumberColor() const
{
  if (m_lineNumberColorSet || isGlobal())
    return m_lineNumberColor;

  return s_global->lineNumberColor();
}

void KateRendererConfig::setLineNumberColor (const QColor &col)
{
  configStart ();

  m_lineNumberColorSet = true;
  m_lineNumberColor = col;

  configEnd ();
}

bool KateRendererConfig::showIndentationLines () const
{
  if (m_showIndentationLinesSet || isGlobal())
    return m_showIndentationLines;

  return s_global->showIndentationLines();
}

void KateRendererConfig::setShowIndentationLines (bool on)
{
  configStart ();

  m_showIndentationLinesSet = true;
  m_showIndentationLines = on;

  configEnd ();
}

//END

// kate: space-indent on; indent-width 2; replace-tabs on;
