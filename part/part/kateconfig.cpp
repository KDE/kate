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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kateconfig.h"

#include "katerenderer.h"
#include "kateview.h"
#include "katedocument.h"
#include "katefont.h"
#include "katefactory.h"
#include "kateschema.h"

#include <kconfig.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kcharsets.h>
#include <klocale.h>
#include <kfinddialog.h>
#include <kreplacedialog.h>
#include <kinstance.h>
#include <kstaticdeleter.h>

#include <qcolor.h>
#include <qtextcodec.h>

// $Id$

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
 : m_configFlags (0),
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
   m_backupFlagsSet (true),
   m_backupSuffixSet (false),
   m_doc (0)
{
  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfig *config = KateFactory::self()->instance()->config();
  config->setGroup("Kate Document Defaults");
  readConfig (config);
}

KateDocumentConfig::KateDocumentConfig (KateDocument *doc)
 : m_configFlags (0),
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
   m_backupFlagsSet (false),
   m_backupSuffixSet (false),
   m_doc (doc)
{
}

KateDocumentConfig::~KateDocumentConfig ()
{
}

static KStaticDeleter<KateDocumentConfig> sdDocConf; 

KateDocumentConfig *KateDocumentConfig::global ()
{
  if (!s_global)
    sdDocConf.setObject(s_global, new KateDocumentConfig ());

  return s_global;
}

void KateDocumentConfig::readConfig (KConfig *config)
{
  configStart ();

  setTabWidth (config->readNumEntry("Tab Width", 8));

  setIndentationWidth (config->readNumEntry("Indentation Width", 2));

  setIndentationMode (config->readNumEntry("Indentation Mode", 0));

  setWordWrap (config->readBoolEntry("Word Wrap", false));
  setWordWrapAt (config->readNumEntry("Word Wrap Column", 80));
  setPageUpDownMovesCursor (config->readNumEntry("PageUp/PageDown Moves Cursor", false));
  setUndoSteps(config->readNumEntry("Undo Steps", 0));

  setConfigFlags (config->readNumEntry("Basic Config Flags", KateDocumentConfig::cfAutoIndent
    | KateDocumentConfig::cfTabIndents
    | KateDocumentConfig::cfKeepIndentProfile
    | KateDocumentConfig::cfWrapCursor
    | KateDocumentConfig::cfShowTabs
    | KateDocumentConfig::cfSmartHome));

  setEncoding (config->readEntry("Encoding", QString::fromLatin1(KGlobal::locale()->encoding())));

  setEol (config->readNumEntry("End of Line", 0));

  setBackupFlags (config->readNumEntry("Backup Config Flags", 1));

  setBackupSuffix (config->readEntry("Backup Suffix", QString ("~")));

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

  config->writeEntry("Backup Config Flags", backupFlags());

  config->writeEntry("Backup Suffix", backupSuffix());
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
    for (uint z=0; z < KateFactory::self()->documents()->count(); z++)
    {
      KateFactory::self()->documents()->at(z)->updateConfig ();
    }
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

QTextCodec *KateDocumentConfig::codec ()
{
  if (m_encodingSet || isGlobal())
    return KGlobal::charsets()->codecForName (m_encoding);

  return s_global->codec ();
}

void KateDocumentConfig::setEncoding (const QString &encoding)
{
  bool found = false;
  QTextCodec *codec = KGlobal::charsets()->codecForName (encoding, found);

  if (!found)
    return;

  configStart ();
  
  if (isGlobal())
    KateDocument::setDefaultEncoding (codec->name());

  m_encodingSet = true;
  m_encoding = codec->name();

  configEnd ();
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

const QString &KateDocumentConfig::backupSuffix () const
{
  if (m_backupSuffixSet || isGlobal())
    return m_backupSuffix;

  return s_global->backupSuffix();
}

void KateDocumentConfig::setBackupSuffix (const QString &suffix)
 {
  configStart ();

  m_backupSuffixSet = true;
  m_backupSuffix = suffix;

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
   m_iconBarSet (true),
   m_foldingBarSet (true),
   m_bookmarkSortSet (true),
   m_autoCenterLinesSet (true),
   m_searchFlagsSet (true),
   m_cmdLineSet (true),
   m_view (0)
{
  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfig *config = KateFactory::self()->instance()->config();
  config->setGroup("Kate View Defaults");
  readConfig (config);
}

KateViewConfig::KateViewConfig (KateView *view)
 :
   m_dynWordWrapSet (false),
   m_dynWordWrapIndicatorsSet (false),
   m_dynWordWrapAlignIndentSet (false),
   m_lineNumbersSet (false),
   m_iconBarSet (false),
   m_foldingBarSet (false),
   m_bookmarkSortSet (false),
   m_autoCenterLinesSet (false),
   m_searchFlagsSet (false),
   m_cmdLineSet (false),
   m_view (view)
{
}

KateViewConfig::~KateViewConfig ()
{
}

static KStaticDeleter<KateViewConfig> sdViewConf; 

KateViewConfig *KateViewConfig::global ()
{
  if (!s_global)
    sdViewConf.setObject(s_global, new KateViewConfig ());

  return s_global;
}

void KateViewConfig::readConfig (KConfig *config)
{
  configStart ();

  setDynWordWrap (config->readBoolEntry( "Dynamic Word Wrap", true ));
  setDynWordWrapIndicators (config->readNumEntry( "Dynamic Word Wrap Indicators", 1 ));
  setDynWordWrapAlignIndent (config->readNumEntry( "Dynamic Word Wrap Align Indent", 80 ));

  setLineNumbers (config->readBoolEntry( "Line Numbers",  false));

  setIconBar (config->readBoolEntry( "Icon Bar", false ));

  setFoldingBar (config->readBoolEntry( "Folding Bar", true));

  setBookmarkSort (config->readNumEntry( "Bookmark Menu Sorting", 0 ));

  setAutoCenterLines (config->readNumEntry( "Auto Center Lines", 0 ));

  setSearchFlags (config->readNumEntry("Search Config Flags", KFindDialog::FromCursor | KFindDialog::CaseSensitive | KReplaceDialog::PromptOnReplace));

  setCmdLine (config->readBoolEntry( "Command Line", false));

  configEnd ();
}

void KateViewConfig::writeConfig (KConfig *config)
{
  config->writeEntry( "Dynamic Word Wrap", dynWordWrap() );
  config->writeEntry( "Dynamic Word Wrap Indicators", dynWordWrapIndicators() );
  config->writeEntry( "Dynamic Word Wrap Align Indent", dynWordWrapAlignIndent() );

  config->writeEntry( "Line Numbers", lineNumbers() );

  config->writeEntry( "Icon Bar", iconBar() );

  config->writeEntry( "Folding Bar", foldingBar() );

  config->writeEntry( "Bookmark Menu Sorting", bookmarkSort() );

  config->writeEntry( "Auto Center Lines", autoCenterLines() );

  config->writeEntry("Search Config Flags", searchFlags());

  config->writeEntry("Command Line", cmdLine());
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
    for (uint z=0; z < KateFactory::self()->views()->count(); z++)
    {
      KateFactory::self()->views()->at(z)->updateConfig ();
    }
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
//END

//BEGIN KateRendererConfig
KateRendererConfig::KateRendererConfig ()
 :
   m_font (new FontStruct ()),
   m_backgroundColor (0),
   m_selectionColor (0),
   m_highlightedLineColor (0),
   m_highlightedBracketColor (0),
   m_wordWrapMarkerColor (0),
   m_tabMarkerColor (0),
   m_iconBarColor (0),
   m_schemaSet (true),
   m_fontSet (true),
   m_wordWrapMarkerSet (true),
   m_backgroundColorSet (true),
   m_selectionColorSet (true),
   m_highlightedLineColorSet (true),
   m_highlightedBracketColorSet (true),
   m_wordWrapMarkerColorSet (true),
   m_tabMarkerColorSet(true),
   m_iconBarColorSet (true),
   m_renderer (0)
{
  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfig *config = KateFactory::self()->instance()->config();
  config->setGroup("Kate Renderer Defaults");
  readConfig (config);
}

KateRendererConfig::KateRendererConfig (KateRenderer *renderer)
 : m_font (0),
   m_backgroundColor (0),
   m_selectionColor (0),
   m_highlightedLineColor (0),
   m_highlightedBracketColor (0),
   m_wordWrapMarkerColor (0),
   m_tabMarkerColor (0),
   m_iconBarColor (0),
   m_schemaSet (false),
   m_fontSet (false),
   m_wordWrapMarkerSet (false),
   m_backgroundColorSet (false),
   m_selectionColorSet (false),
   m_highlightedLineColorSet (false),
   m_highlightedBracketColorSet (false),
   m_wordWrapMarkerColorSet (false),
   m_tabMarkerColorSet(false),
   m_iconBarColorSet (false),
   m_renderer (renderer)
{
}

KateRendererConfig::~KateRendererConfig ()
{
  delete m_font;

  delete m_backgroundColor;
  delete m_selectionColor;
  delete m_highlightedLineColor;
  delete m_highlightedBracketColor;
  delete m_wordWrapMarkerColor;
  delete m_tabMarkerColor;
  delete m_iconBarColor;
}

static KStaticDeleter<KateRendererConfig> sdRendererConf;

KateRendererConfig *KateRendererConfig::global ()
{
  if (!s_global)
    sdRendererConf.setObject(s_global, new KateRendererConfig ());

  return s_global;
}

void KateRendererConfig::readConfig (KConfig *config)
{
  configStart ();

  setSchema (KateFactory::self()->schemaManager()->number (config->readEntry("Schema", "Kate Normal Schema")));

  setWordWrapMarker (config->readBoolEntry("Word Wrap Marker", false ));

  configEnd ();
}

void KateRendererConfig::writeConfig (KConfig *config)
{
  config->writeEntry ("Schema", KateFactory::self()->schemaManager()->name(schema()));

  config->writeEntry( "Word Wrap Marker", wordWrapMarker() );
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
    for (uint z=0; z < KateFactory::self()->renderers()->count(); z++)
    {
      KateFactory::self()->renderers()->at(z)->updateConfig ();
    }
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

  KConfig *config (KateFactory::self()->schemaManager()->schema(schema));

  QColor tmp0 (KGlobalSettings::baseColor());
  QColor tmp1 (KGlobalSettings::highlightColor());
  QColor tmp2 (KGlobalSettings::alternateBackgroundColor());
  QColor tmp3 ( "#FFFF99" );
  QColor tmp4 (tmp2.dark());
  QColor tmp5 ( KGlobalSettings::textColor() );
  QColor tmp6 ( "#EAE9E8" );

  setBackgroundColor (config->readColorEntry("Color Background", &tmp0));
  setSelectionColor (config->readColorEntry("Color Selection", &tmp1));
  setHighlightedLineColor (config->readColorEntry("Color Highlighted Line", &tmp2));
  setHighlightedBracketColor (config->readColorEntry("Color Highlighted Bracket", &tmp3));
  setWordWrapMarkerColor (config->readColorEntry("Color Word Wrap Marker", &tmp4));
  setTabMarkerColor (config->readColorEntry("Color Tab Marker", &tmp5));
  setIconBarColor (config->readColorEntry("Color Icon Bar", &tmp6));

  QFont f (KGlobalSettings::fixedFont());

  setFont(config->readFontEntry("Font", &f));

  configEnd ();
}

FontStruct *KateRendererConfig::fontStruct ()
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
    m_font = new FontStruct ();
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

const QColor *KateRendererConfig::backgroundColor() const
{
  if (m_backgroundColorSet || isGlobal())
    return m_backgroundColor;

  return s_global->backgroundColor();
}

void KateRendererConfig::setBackgroundColor (const QColor &col)
{
  configStart ();

  m_backgroundColorSet = true;
  delete m_backgroundColor;
  m_backgroundColor = new QColor (col);

  configEnd ();
}

const QColor *KateRendererConfig::selectionColor() const
{
  if (m_selectionColorSet || isGlobal())
    return m_selectionColor;

  return s_global->selectionColor();
}

void KateRendererConfig::setSelectionColor (const QColor &col)
{
  configStart ();

  m_selectionColorSet = true;
  delete m_selectionColor;
  m_selectionColor = new QColor (col);

  configEnd ();
}

const QColor *KateRendererConfig::highlightedLineColor() const
{
  if (m_highlightedLineColorSet || isGlobal())
    return m_highlightedLineColor;

  return s_global->highlightedLineColor();
}

void KateRendererConfig::setHighlightedLineColor (const QColor &col)
{
  configStart ();

  m_highlightedLineColorSet = true;
  delete m_highlightedLineColor;
  m_highlightedLineColor = new QColor (col);

  configEnd ();
}

const QColor *KateRendererConfig::highlightedBracketColor() const
{
  if (m_highlightedBracketColorSet || isGlobal())
    return m_highlightedBracketColor;

  return s_global->highlightedBracketColor();
}

void KateRendererConfig::setHighlightedBracketColor (const QColor &col)
{
  configStart ();

  m_highlightedBracketColorSet = true;
  delete m_highlightedBracketColor;
  m_highlightedBracketColor = new QColor (col);

  configEnd ();
}

const QColor *KateRendererConfig::wordWrapMarkerColor() const
{
  if (m_wordWrapMarkerColorSet || isGlobal())
    return m_wordWrapMarkerColor;

  return s_global->wordWrapMarkerColor();
}

void KateRendererConfig::setWordWrapMarkerColor (const QColor &col)
{
  configStart ();

  m_wordWrapMarkerColorSet = true;
  delete m_wordWrapMarkerColor;
  m_wordWrapMarkerColor = new QColor (col);

  configEnd ();
}

const QColor *KateRendererConfig::tabMarkerColor() const
{
  if (m_tabMarkerColorSet || isGlobal())
    return m_tabMarkerColor;

  return s_global->tabMarkerColor();
}

void KateRendererConfig::setTabMarkerColor (const QColor &col)
{
  configStart ();

  m_tabMarkerColorSet = true;
  delete m_tabMarkerColor;
  m_tabMarkerColor = new QColor (col);

  configEnd ();
}

const QColor *KateRendererConfig::iconBarColor() const
{
  if (m_iconBarColorSet || isGlobal())
    return m_iconBarColor;

  return s_global->iconBarColor();
}

void KateRendererConfig::setIconBarColor (const QColor &col)
{
  configStart ();

  m_iconBarColorSet = true;
  delete m_iconBarColor;
  m_iconBarColor = new QColor (col);

  configEnd ();
}

//END
// kate: space-indent on; indent-width 2; replace-tabs on;
