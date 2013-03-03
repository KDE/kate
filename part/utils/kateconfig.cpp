/* This file is part of the KDE libraries
   Copyright (C) 2007, 2008 Matthew Woehlke <mw_triad@users.sourceforge.net>
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateconfig.h"

#include "kateglobal.h"
#include "katerenderer.h"
#include "kateview.h"
#include "katedocument.h"
#include "kateschema.h"

#include <math.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobalsettings.h>
#include <kcolorscheme.h>
#include <kcolorutils.h>
#include <kcharsets.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kdebug.h>

#include <QtCore/QTextCodec>
#include <QStringListModel>

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
KateGlobalConfig *KateGlobalConfig::s_global = 0;
KateDocumentConfig *KateDocumentConfig::s_global = 0;
KateViewConfig *KateViewConfig::s_global = 0;
KateRendererConfig *KateRendererConfig::s_global = 0;

KateGlobalConfig::KateGlobalConfig ()
{
  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfigGroup cg( KGlobal::config(), "Kate Part Defaults");
  readConfig (cg);
}

KateGlobalConfig::~KateGlobalConfig ()
{
}

void KateGlobalConfig::readConfig (const KConfigGroup &config)
{
  configStart ();

  setProberType ((KEncodingProber::ProberType)config.readEntry("Encoding Prober Type", (int)KEncodingProber::Universal));
  setFallbackEncoding (config.readEntry("Fallback Encoding", ""));

  configEnd ();
}

void KateGlobalConfig::writeConfig (KConfigGroup &config)
{
  config.writeEntry("Encoding Prober Type", (int)proberType());
  config.writeEntry("Fallback Encoding", fallbackEncoding());
}

void KateGlobalConfig::updateConfig ()
{
}

void KateGlobalConfig::setProberType (KEncodingProber::ProberType proberType)
{
  configStart ();
  m_proberType = proberType;
  configEnd ();
}

const QString &KateGlobalConfig::fallbackEncoding () const
{
  return m_fallbackEncoding;
}

QTextCodec *KateGlobalConfig::fallbackCodec () const
{
  if (m_fallbackEncoding.isEmpty())
      return QTextCodec::codecForName("ISO 8859-15");

  return KGlobal::charsets()->codecForName (m_fallbackEncoding);
}

bool KateGlobalConfig::setFallbackEncoding (const QString &encoding)
{
  QTextCodec *codec;
  bool found = false;
  if (encoding.isEmpty())
  {
    codec = s_global->fallbackCodec();
    found = true;
  }
  else
    codec = KGlobal::charsets()->codecForName (encoding, found);

  if (!found || !codec)
    return false;

  configStart ();
  m_fallbackEncoding = codec->name();
  configEnd ();
  return true;
}

KateDocumentConfig::KateDocumentConfig ()
 : m_indentationWidth (2),
   m_tabWidth (8),
   m_tabHandling (tabSmart),
   m_configFlags (0),
   m_wordWrapAt (80),
   m_tabWidthSet (true),
   m_indentationWidthSet (true),
   m_indentationModeSet (true),
   m_wordWrapSet (true),
   m_wordWrapAtSet (true),
   m_pageUpDownMovesCursorSet (true),
   m_keepExtraSpacesSet (false),
   m_indentPastedTextSet (false),
   m_backspaceIndentsSet (false),
   m_smartHomeSet (false),
   m_showTabsSet (false),
   m_showSpacesSet (false),
   m_replaceTabsDynSet (false),
   m_removeSpacesSet (false),
   m_newLineAtEofSet (false),
   m_overwiteModeSet (false),
   m_tabIndentsSet (false),
   m_encodingSet (true),
   m_eolSet (true),
   m_bomSet (true),
   m_allowEolDetectionSet (false),
   m_allowSimpleModeSet (false),
   m_backupFlagsSet (true),
   m_searchDirConfigDepthSet (true),
   m_backupPrefixSet (true),
   m_backupSuffixSet (true),
   m_swapFileNoSyncSet (true),
   m_onTheFlySpellCheckSet (true),
   m_lineLengthLimitSet (true),
   m_doc (0)
{
  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfigGroup cg( KGlobal::config(), "Kate Document Defaults");
  readConfig (cg);
}

KateDocumentConfig::KateDocumentConfig (const KConfigGroup &cg)
 : m_indentationWidth (2),
   m_tabWidth (8),
   m_tabHandling (tabSmart),
   m_configFlags (0),
   m_wordWrapAt (80),
   m_tabWidthSet (true),
   m_indentationWidthSet (true),
   m_indentationModeSet (true),
   m_wordWrapSet (true),
   m_wordWrapAtSet (true),
   m_pageUpDownMovesCursorSet (true),
   m_keepExtraSpacesSet (false),
   m_indentPastedTextSet (false),
   m_backspaceIndentsSet (false),
   m_smartHomeSet (false),
   m_showTabsSet (false),
   m_showSpacesSet (false),
   m_replaceTabsDynSet (false),
   m_removeSpacesSet (false),
   m_newLineAtEofSet (false),
   m_overwiteModeSet (false),
   m_tabIndentsSet (false),
   m_encodingSet (true),
   m_eolSet (true),
   m_bomSet (true),
   m_allowEolDetectionSet (false),
   m_allowSimpleModeSet (false),
   m_backupFlagsSet (true),
   m_searchDirConfigDepthSet (true),
   m_backupPrefixSet (true),
   m_backupSuffixSet (true),
   m_swapFileNoSyncSet (true),
   m_onTheFlySpellCheckSet (true),
   m_lineLengthLimitSet (true),
   m_doc (0)
{
  // init with defaults from config or really hardcoded ones
  readConfig (cg);
}

KateDocumentConfig::KateDocumentConfig (KateDocument *doc)
 : m_tabHandling (tabSmart),
   m_configFlags (0),
   m_tabWidthSet (false),
   m_indentationWidthSet (false),
   m_indentationModeSet (false),
   m_wordWrapSet (false),
   m_wordWrapAtSet (false),
   m_pageUpDownMovesCursorSet (false),
   m_keepExtraSpacesSet (false),
   m_indentPastedTextSet (false),
   m_backspaceIndentsSet (false),
   m_smartHomeSet (false),
   m_showTabsSet (false),
   m_showSpacesSet (false),
   m_replaceTabsDynSet (false),
   m_removeSpacesSet (false),
   m_newLineAtEofSet (false),
   m_overwiteModeSet (false),
   m_tabIndentsSet (false),
   m_encodingSet (false),
   m_eolSet (false),
   m_bomSet (false),
   m_allowEolDetectionSet (false),
   m_allowSimpleModeSet (false),
   m_backupFlagsSet (false),
   m_searchDirConfigDepthSet (false),
   m_backupPrefixSet (false),
   m_backupSuffixSet (false),
   m_swapFileNoSyncSet (false),
   m_onTheFlySpellCheckSet (false),
   m_lineLengthLimitSet (false),
   m_doc (doc)
{
}

KateDocumentConfig::~KateDocumentConfig ()
{
}

void KateDocumentConfig::readConfig (const KConfigGroup &config)
{
  configStart ();

  setTabWidth (config.readEntry("Tab Width", 8));

  setIndentationWidth (config.readEntry("Indentation Width", 2));

  setIndentationMode (config.readEntry("Indentation Mode", "normal"));

  setTabHandling (config.readEntry("Tab Handling", int(KateDocumentConfig::tabSmart)));

  setWordWrap (config.readEntry("Word Wrap", false));
  setWordWrapAt (config.readEntry("Word Wrap Column", 80));
  setPageUpDownMovesCursor (config.readEntry("PageUp/PageDown Moves Cursor", false));

  setSmartHome (config.readEntry("Smart Home", true));
  setShowTabs (config.readEntry("Show Tabs", true));
  setTabIndents (config.readEntry("Indent On Tab", true));
  setKeepExtraSpaces (config.readEntry("Keep Extra Spaces", false));
  setIndentPastedText (config.readEntry("Indent On Text Paste", false));
  setBackspaceIndents (config.readEntry("Indent On Backspace", false));
  setShowSpaces (config.readEntry("Show Spaces", false));
  setReplaceTabsDyn (config.readEntry("ReplaceTabsDyn", false));
  setRemoveSpaces (config.readEntry("Remove Spaces", 0));
  setNewLineAtEof (config.readEntry("Newline At EOF", false));
  setOvr (config.readEntry("Overwrite Mode", false));

  setEncoding (config.readEntry("Encoding", ""));

  setEol (config.readEntry("End of Line", 0));
  setAllowEolDetection (config.readEntry("Allow End of Line Detection", true));

  setBom (config.readEntry("BOM",false));

  setAllowSimpleMode (config.readEntry("Allow Simple Mode", true));

  setBackupFlags (config.readEntry("Backup Flags", 0));

  setSearchDirConfigDepth (config.readEntry("Search Dir Config Depth", 9));

  setBackupPrefix (config.readEntry("Backup Prefix", QString ("")));

  setBackupSuffix (config.readEntry("Backup Suffix", QString ("~")));

  setSwapFileNoSync (config.readEntry("No sync", false));

  setOnTheFlySpellCheck(config.readEntry("On-The-Fly Spellcheck", false));

  setLineLengthLimit(config.readEntry("Line Length Limit", 4096));

  configEnd ();
}

void KateDocumentConfig::writeConfig (KConfigGroup &config)
{
  config.writeEntry("Tab Width", tabWidth());

  config.writeEntry("Indentation Width", indentationWidth());
  config.writeEntry("Indentation Mode", indentationMode());

  config.writeEntry("Tab Handling", tabHandling());

  config.writeEntry("Word Wrap", wordWrap());
  config.writeEntry("Word Wrap Column", wordWrapAt());

  config.writeEntry("PageUp/PageDown Moves Cursor", pageUpDownMovesCursor());

  config.writeEntry("Smart Home", smartHome());
  config.writeEntry("Show Tabs", showTabs());
  config.writeEntry("Indent On Tab", tabIndentsEnabled());
  config.writeEntry("Keep Extra Spaces", keepExtraSpaces());
  config.writeEntry("Indent On Text Paste", indentPastedText());
  config.writeEntry("Indent On Backspace", backspaceIndents());
  config.writeEntry("Show Spaces", showSpaces());
  config.writeEntry("ReplaceTabsDyn", replaceTabsDyn());
  config.writeEntry("Remove Spaces", removeSpaces());
  config.writeEntry("Newline At EOF", newLineAtEof());
  config.writeEntry("Overwrite Mode", ovr());

  config.writeEntry("Encoding", encoding());

  config.writeEntry("End of Line", eol());
  config.writeEntry("Allow End of Line Detection", allowEolDetection());

  config.writeEntry("BOM",bom());

  config.writeEntry("Allow Simple Mode", allowSimpleMode());

  config.writeEntry("Backup Flags", backupFlags());

  config.writeEntry("Search Dir Config Depth", searchDirConfigDepth());

  config.writeEntry("Backup Prefix", backupPrefix());

  config.writeEntry("Backup Suffix", backupSuffix());

  config.writeEntry("No sync", swapFileNoSync());

  config.writeEntry("On-The-Fly Spellcheck", onTheFlySpellCheck());

  config.writeEntry("Line Length Limit", lineLengthLimit());
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

const QString &KateDocumentConfig::indentationMode () const
{
  if (m_indentationModeSet || isGlobal())
    return m_indentationMode;

  return s_global->indentationMode();
}

void KateDocumentConfig::setIndentationMode (const QString &indentationMode)
{
  configStart ();

  m_indentationModeSet = true;
  m_indentationMode = indentationMode;

  configEnd ();
}

uint KateDocumentConfig::tabHandling () const
{
  // This setting is purly a user preference,
  // hence, there exists only the global setting.
  if (isGlobal())
    return m_tabHandling;

  return s_global->tabHandling();
}

void KateDocumentConfig::setTabHandling (uint tabHandling)
{
  configStart ();

  m_tabHandling = tabHandling;

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

void KateDocumentConfig::setKeepExtraSpaces(bool on)
{
  configStart ();

  m_keepExtraSpacesSet = true;
  m_keepExtraSpaces = on;

  configEnd ();
}

bool KateDocumentConfig::keepExtraSpaces() const
{
  if (m_keepExtraSpacesSet || isGlobal())
    return m_keepExtraSpaces;

  return s_global->keepExtraSpaces();
}

void KateDocumentConfig::setIndentPastedText(bool on)
{
  configStart ();

  m_indentPastedTextSet = true;
  m_indentPastedText = on;

  configEnd ();
}

bool KateDocumentConfig::indentPastedText() const
{
  if (m_indentPastedTextSet || isGlobal())
    return m_indentPastedText;

  return s_global->indentPastedText();
}

void KateDocumentConfig::setBackspaceIndents(bool on)
{
  configStart ();

  m_backspaceIndentsSet = true;
  m_backspaceIndents = on;

  configEnd ();
}

bool KateDocumentConfig::backspaceIndents() const
{
  if (m_backspaceIndentsSet || isGlobal())
    return m_backspaceIndents;

  return s_global->backspaceIndents();
}

void KateDocumentConfig::setSmartHome(bool on)
{
  configStart ();

  m_smartHomeSet = true;
  m_smartHome = on;

  configEnd ();
}

bool KateDocumentConfig::smartHome() const
{
  if (m_smartHomeSet || isGlobal())
    return m_smartHome;

  return s_global->smartHome();
}

void KateDocumentConfig::setShowTabs(bool on)
{
  configStart ();

  m_showTabsSet = true;
  m_showTabs = on;

  configEnd ();
}

bool KateDocumentConfig::showTabs() const
{
  if (m_showTabsSet || isGlobal())
    return m_showTabs;

  return s_global->showTabs();
}

void KateDocumentConfig::setShowSpaces(bool on)
{
  configStart ();

  m_showSpacesSet = true;
  m_showSpaces = on;

  configEnd ();
}

bool KateDocumentConfig::showSpaces() const
{
  if (m_showSpacesSet || isGlobal())
    return m_showSpaces;

  return s_global->showSpaces();
}

void KateDocumentConfig::setReplaceTabsDyn(bool on)
{
  configStart ();

  m_replaceTabsDynSet = true;
  m_replaceTabsDyn = on;

  configEnd ();
}

bool KateDocumentConfig::replaceTabsDyn() const
{
  if (m_replaceTabsDynSet || isGlobal())
    return m_replaceTabsDyn;

  return s_global->replaceTabsDyn();
}

void KateDocumentConfig::setRemoveSpaces(int triState)
{
  configStart ();

  m_removeSpacesSet = true;
  m_removeSpaces = triState;

  configEnd ();
}

int KateDocumentConfig::removeSpaces() const
{
  if (m_removeSpacesSet || isGlobal())
    return m_removeSpaces;

  return s_global->removeSpaces();
}

void KateDocumentConfig::setNewLineAtEof (bool on)
{
  configStart ();

  m_newLineAtEofSet = true;
  m_newLineAtEof = on;

  configEnd ();
}

bool KateDocumentConfig::newLineAtEof () const
{
  if (m_newLineAtEofSet || isGlobal())
    return m_newLineAtEof;

  return s_global->newLineAtEof();
}

void KateDocumentConfig::setOvr(bool on)
{
  configStart ();

  m_overwiteModeSet = true;
  m_overwiteMode = on;

  configEnd ();
}

bool KateDocumentConfig::ovr() const
{
  if (m_overwiteModeSet || isGlobal())
    return m_overwiteMode;

  return s_global->ovr();
}

void KateDocumentConfig::setTabIndents(bool on)
{
  configStart ();

  m_tabIndentsSet = true;
  m_tabIndents = on;

  configEnd ();
}

bool KateDocumentConfig::tabIndentsEnabled() const
{
  if (m_tabIndentsSet || isGlobal())
    return m_tabIndents;

  return s_global->tabIndentsEnabled();
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
  QTextCodec *codec;
  bool found = false;
  if (encoding.isEmpty())
  {
    codec = s_global->codec();
    found=true;
  }
  else
    codec = KGlobal::charsets()->codecForName (encoding, found);

  if (!found || !codec)
    return false;

  configStart ();
  m_encodingSet = true;
  m_encoding = codec->name();

  if (isGlobal())
    KateGlobal::self()->setDefaultEncoding (m_encoding);

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

void KateDocumentConfig::setBom (bool bom)
{
  configStart ();

  m_bomSet = true;
  m_bom = bom;

  configEnd ();
}

bool KateDocumentConfig::bom() const
{
    if (m_bomSet || isGlobal())
      return m_bom;
    return s_global->bom();
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


bool KateDocumentConfig::allowSimpleMode () const
{
  if (m_allowSimpleModeSet || isGlobal())
    return m_allowSimpleMode;

  return s_global->allowSimpleMode();
}

void KateDocumentConfig::setAllowSimpleMode (bool on)
{
  configStart ();

  m_allowSimpleModeSet = true;
  m_allowSimpleMode = on;

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

bool KateDocumentConfig::swapFileNoSync() const
{
  if (m_swapFileNoSyncSet || isGlobal())
    return m_swapFileNoSync;

  return s_global->swapFileNoSync();
}

void KateDocumentConfig::setSwapFileNoSync(bool on)
{
  configStart();

  m_swapFileNoSyncSet = true;
  m_swapFileNoSync = on;

  configEnd();
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

bool KateDocumentConfig::onTheFlySpellCheck() const
{
  if(isGlobal()) {
    // WARNING: this is slightly hackish, but it's currently the only way to
    //          do it, see also the KTextEdit class
    KConfigGroup configGroup(KGlobal::config(), "Spelling");
    return configGroup.readEntry("checkerEnabledByDefault", false);
  }
  if (m_onTheFlySpellCheckSet) {
    return m_onTheFlySpellCheck;
  }

  return s_global->onTheFlySpellCheck();
}

void KateDocumentConfig::setOnTheFlySpellCheck(bool on)
{
  configStart ();

  m_onTheFlySpellCheckSet = true;
  m_onTheFlySpellCheck = on;

  configEnd ();
}


int KateDocumentConfig::lineLengthLimit() const
{
  if (m_lineLengthLimitSet || isGlobal())
    return m_lineLengthLimit;

  return s_global->lineLengthLimit();
}

void KateDocumentConfig::setLineLengthLimit(int lineLengthLimit)
{
  configStart();

  m_lineLengthLimitSet = true;
  m_lineLengthLimit = lineLengthLimit;

  configEnd();
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
   m_scrollBarMiniMapSet (true),
   m_scrollBarMiniMapAllSet (true),
   m_scrollBarMiniMapWidthSet (true),
   m_iconBarSet (true),
   m_foldingBarSet (true),
   m_lineModificationSet (true),
   m_bookmarkSortSet (true),
   m_autoCenterLinesSet (true),
   m_searchFlagsSet (true),
   m_defaultMarkTypeSet (true),
   m_persistentSelectionSet (true),
   m_viInputModeSet (true),
   m_viInputModeStealKeysSet (true),
   m_automaticCompletionInvocationSet (true),
   m_wordCompletionSet (true),
   m_wordCompletionMinimalWordLengthSet (true),
   m_smartCopyCutSet (true),
   m_scrollPastEndSet (true),
   m_allowMarkMenu (true),
   m_wordCompletionRemoveTailSet (false),
   m_view (0)
{
  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfigGroup config( KGlobal::config(), "Kate View Defaults");
  readConfig (config);
}

KateViewConfig::KateViewConfig (KateView *view)
 :
   m_searchFlags (PowerModePlainText),
   m_maxHistorySize (100),
   m_dynWordWrapSet (false),
   m_dynWordWrapIndicatorsSet (false),
   m_dynWordWrapAlignIndentSet (false),
   m_lineNumbersSet (false),
   m_scrollBarMarksSet (false),
   m_scrollBarMiniMapSet (false),
   m_scrollBarMiniMapAllSet (false),
   m_scrollBarMiniMapWidthSet (false),
   m_iconBarSet (false),
   m_foldingBarSet (false),
   m_lineModificationSet (false),
   m_bookmarkSortSet (false),
   m_autoCenterLinesSet (false),
   m_searchFlagsSet (false),
   m_defaultMarkTypeSet (false),
   m_persistentSelectionSet (false),
   m_viInputModeSet (false),
   m_viInputModeStealKeysSet (false),
   m_automaticCompletionInvocationSet (false),
   m_wordCompletionSet (false),
   m_wordCompletionMinimalWordLengthSet (false),
   m_smartCopyCutSet (false),
   m_scrollPastEndSet (false),
   m_allowMarkMenu (true),
   m_wordCompletionRemoveTailSet (false),
   m_view (view)
{
}

KateViewConfig::~KateViewConfig ()
{
}


// TODO Extract more keys to constants for maintainability
static const char * const KEY_SEARCH_REPLACE_FLAGS = "Search/Replace Flags";
static const char * const KEY_PATTERN_HISTORY = "Search Pattern History";
static const char * const KEY_REPLACEMENT_HISTORY = "Replacement Text History";


void KateViewConfig::readConfig ( const KConfigGroup &config)
{
  configStart ();

  // default off again, until this is usable for large size documents
  setDynWordWrap (config.readEntry( "Dynamic Word Wrap", false ));
  setDynWordWrapIndicators (config.readEntry( "Dynamic Word Wrap Indicators", 1 ));
  setDynWordWrapAlignIndent (config.readEntry( "Dynamic Word Wrap Align Indent", 80 ));

  setLineNumbers (config.readEntry( "Line Numbers",  false));

  setScrollBarMarks (config.readEntry( "Scroll Bar Marks",  false));

  setScrollBarMiniMap (config.readEntry( "Scroll Bar Mini Map",  false));

  setScrollBarMiniMapAll (config.readEntry( "Scroll Bar Mini Map All",  false));

  setScrollBarMiniMapWidth (config.readEntry( "Scroll Bar Mini Map Width",  60));

  setIconBar (config.readEntry( "Icon Bar", false ));

  setFoldingBar (config.readEntry( "Folding Bar", true));

  setLineModification (config.readEntry( "Line Modification", false));

  setBookmarkSort (config.readEntry( "Bookmark Menu Sorting", 0 ));

  setAutoCenterLines (config.readEntry( "Auto Center Lines", 0 ));

  setSearchFlags(config.readEntry(KEY_SEARCH_REPLACE_FLAGS,
      IncFromCursor|PowerMatchCase|PowerModePlainText));

  m_maxHistorySize = config.readEntry("Maximum Search History Size", 100);

  setDefaultMarkType (config.readEntry( "Default Mark Type", int(KTextEditor::MarkInterface::markType01) ));
  setAllowMarkMenu (config.readEntry( "Allow Mark Menu", true ));

  setPersistentSelection (config.readEntry( "Persistent Selection", false ));

  setViInputMode (config.readEntry( "Vi Input Mode", false));
  setViInputModeStealKeys (config.readEntry( "Vi Input Mode Steal Keys", false));
  setViInputModeHideStatusBar (config.readEntry( "Vi Input Mode Hide Status Bar", false));

  setAutomaticCompletionInvocation (config.readEntry( "Auto Completion", true ));
  setWordCompletion (config.readEntry( "Word Completion", true ));
  setWordCompletionMinimalWordLength (config.readEntry( "Word Completion Minimal Word Length", 3 ));
  setWordCompletionRemoveTail (config.readEntry( "Word Completion Remove Tail", false ));
  setSmartCopyCut (config.readEntry( "Smart Copy Cut", false ));
  setScrollPastEnd (config.readEntry( "Scroll Past End", false ));

  if (isGlobal()) {
    // Read search pattern history
    QStringList patternHistory = config.readEntry(KEY_PATTERN_HISTORY, QStringList());
    m_patternHistoryModel.setStringList(patternHistory);

    // Read replacement text history
    QStringList replacementHistory = config.readEntry(KEY_REPLACEMENT_HISTORY, QStringList());
    m_replacementHistoryModel.setStringList(replacementHistory);
  }

  configEnd ();
}

void KateViewConfig::writeConfig (KConfigGroup &config)
{
  config.writeEntry( "Dynamic Word Wrap", dynWordWrap() );
  config.writeEntry( "Dynamic Word Wrap Indicators", dynWordWrapIndicators() );
  config.writeEntry( "Dynamic Word Wrap Align Indent", dynWordWrapAlignIndent() );

  config.writeEntry( "Line Numbers", lineNumbers() );

  config.writeEntry( "Scroll Bar Marks", scrollBarMarks() );

  config.writeEntry( "Scroll Bar Mini Map", scrollBarMiniMap() );

  config.writeEntry( "Scroll Bar Mini Map All", scrollBarMiniMapAll() );

  config.writeEntry( "Scroll Bar Mini Map Width", scrollBarMiniMapWidth() );

  config.writeEntry( "Icon Bar", iconBar() );

  config.writeEntry( "Folding Bar", foldingBar() );

  config.writeEntry( "Line Modification", lineModification() );

  config.writeEntry( "Bookmark Menu Sorting", bookmarkSort() );

  config.writeEntry( "Auto Center Lines", autoCenterLines() );

  config.writeEntry(KEY_SEARCH_REPLACE_FLAGS, int(searchFlags()));

  config.writeEntry("Maximum Search History Size", m_maxHistorySize);

  config.writeEntry("Default Mark Type", defaultMarkType());

  config.writeEntry("Allow Mark Menu", allowMarkMenu());

  config.writeEntry("Persistent Selection", persistentSelection());

  config.writeEntry( "Auto Completion", automaticCompletionInvocation());
  config.writeEntry( "Word Completion", wordCompletion());
  config.writeEntry( "Word Completion Minimal Word Length", wordCompletionMinimalWordLength());
  config.writeEntry( "Word Completion Remove Tail", wordCompletionRemoveTail());

  config.writeEntry( "Smart Copy Cut", smartCopyCut() );
  config.writeEntry( "Scroll Past End" , scrollPastEnd() );

  config.writeEntry( "Vi Input Mode", viInputMode());

  config.writeEntry( "Vi Input Mode Steal Keys", viInputModeStealKeys());

  config.writeEntry( "Vi Input Mode Hide Status Bar", viInputModeHideStatusBar());


  if (isGlobal()) {
    // Write search pattern history
    config.writeEntry(KEY_PATTERN_HISTORY, m_patternHistoryModel.stringList());

    // Write replacement text history
    config.writeEntry(KEY_REPLACEMENT_HISTORY, m_replacementHistoryModel.stringList());
  }
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
  m_dynWordWrapIndicators = qBound(0, mode, 80);

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

bool KateViewConfig::scrollBarMiniMap () const
{
  if (m_scrollBarMiniMapSet || isGlobal())
    return m_scrollBarMiniMap;

  return s_global->scrollBarMiniMap();
}

void KateViewConfig::setScrollBarMiniMap (bool on)
{
  configStart ();

  m_scrollBarMiniMapSet = true;
  m_scrollBarMiniMap = on;

  configEnd ();
}

bool KateViewConfig::scrollBarMiniMapAll () const
{
  if (m_scrollBarMiniMapAllSet || isGlobal())
    return m_scrollBarMiniMapAll;

  return s_global->scrollBarMiniMapAll();
}

void KateViewConfig::setScrollBarMiniMapAll (bool on)
{
  configStart ();

  m_scrollBarMiniMapAllSet = true;
  m_scrollBarMiniMapAll = on;

  configEnd ();
}

int KateViewConfig::scrollBarMiniMapWidth () const
{
  if (m_scrollBarMiniMapWidthSet || isGlobal())
    return m_scrollBarMiniMapWidth;

  return s_global->scrollBarMiniMapWidth();
}

void KateViewConfig::setScrollBarMiniMapWidth (int width)
{
  configStart ();

  m_scrollBarMiniMapWidthSet = true;
  m_scrollBarMiniMapWidth = width;

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

bool KateViewConfig::lineModification () const
{
  if (m_lineModificationSet || isGlobal())
    return m_lineModification;

  return s_global->lineModification();
}

void KateViewConfig::setLineModification (bool on)
{
  configStart ();

  m_lineModificationSet = true;
  m_lineModification = on;

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

QStringListModel *KateViewConfig::patternHistoryModel()
{
  // always return global history
  if (isGlobal())
    return &m_patternHistoryModel;

  return s_global->patternHistoryModel();
}

int KateViewConfig::maxHistorySize() const
{
  return m_maxHistorySize;
}

QStringListModel *KateViewConfig::replacementHistoryModel()
{
  // always return global history
  if (isGlobal())
    return &m_replacementHistoryModel;

  return s_global->replacementHistoryModel();
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

bool KateViewConfig::allowMarkMenu() const
{
  return m_allowMarkMenu;
}

void KateViewConfig::setAllowMarkMenu (bool allow)
{
  m_allowMarkMenu = allow;
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

bool KateViewConfig::viInputMode () const
{
  if (m_viInputModeSet || isGlobal())
    return m_viInputMode;

  return s_global->viInputMode();
}

void KateViewConfig::setViInputMode (bool on)
{
  configStart ();

  m_viInputModeSet = true;
  m_viInputMode = on;

  // update all views and show/hide the status bar
  foreach (KateView* view, KateGlobal::self()->views() ) {
    if (on && !m_viInputModeHideStatusBar) {
      view->showViModeBar();
    } else {
      view->hideViModeBar();
    }
  }

  // make sure to turn off edits mergin when leaving vi input mode
  if (!on && m_view) {
    m_view->doc()->setUndoMergeAllEdits(false);
  }

  configEnd ();
}

bool KateViewConfig::viInputModeStealKeys () const
{
  if (m_viInputModeStealKeysSet || isGlobal())
    return m_viInputModeStealKeys;

  return s_global->viInputModeStealKeys();
}

void KateViewConfig::setViInputModeStealKeys (bool on)
{
  configStart ();

  m_viInputModeStealKeysSet = true;
  m_viInputModeStealKeys = on;

  configEnd ();
}

bool KateViewConfig::viInputModeHideStatusBar () const
{
  if (m_viInputModeHideStatusBarSet || isGlobal())
    return m_viInputModeHideStatusBar;

  return s_global->viInputModeHideStatusBar();
}

void KateViewConfig::setViInputModeHideStatusBar (bool on)
{
  configStart ();

  m_viInputModeHideStatusBarSet = true;
  m_viInputModeHideStatusBar = on;

  // update all views and show/hide the status bar
  foreach (KateView* view, KateGlobal::self()->views() ) {
    if (on && m_viInputMode) {
      view->hideViModeBar();
    } else if (viInputMode()) {
      view->showViModeBar();
    }
  }

  configEnd ();
}


bool KateViewConfig::automaticCompletionInvocation () const
{
  if (m_automaticCompletionInvocationSet || isGlobal())
    return m_automaticCompletionInvocation;

  return s_global->automaticCompletionInvocation();
}

void KateViewConfig::setAutomaticCompletionInvocation (bool on)
{
  configStart ();

  m_automaticCompletionInvocationSet = true;
  m_automaticCompletionInvocation = on;

  configEnd ();
}

bool KateViewConfig::wordCompletion () const
{
  if (m_wordCompletionSet || isGlobal())
    return m_wordCompletion;

  return s_global->wordCompletion();
}

void KateViewConfig::setWordCompletion (bool on)
{
  configStart ();

  m_wordCompletionSet = true;
  m_wordCompletion = on;

  configEnd ();
}

int KateViewConfig::wordCompletionMinimalWordLength () const
{
  if (m_wordCompletionMinimalWordLengthSet || isGlobal())
    return m_wordCompletionMinimalWordLength;

  return s_global->wordCompletionMinimalWordLength();
}

void KateViewConfig::setWordCompletionMinimalWordLength (int length)
{
  configStart ();

  m_wordCompletionMinimalWordLengthSet = true;
  m_wordCompletionMinimalWordLength = length;

  configEnd ();
}

bool KateViewConfig::wordCompletionRemoveTail () const
{
  if (m_wordCompletionRemoveTailSet || isGlobal())
  {
    return m_wordCompletionRemoveTail;
    }
  return s_global->wordCompletionRemoveTail();
}

void KateViewConfig::setWordCompletionRemoveTail (bool on)
{
  configStart ();
  m_wordCompletionRemoveTailSet = true;
  m_wordCompletionRemoveTail = on;

  configEnd ();
}

bool KateViewConfig::smartCopyCut () const
{
  if (m_smartCopyCutSet || isGlobal())
     return m_smartCopyCut;

  return s_global->smartCopyCut();
}

void KateViewConfig::setSmartCopyCut (bool on)
{
  configStart ();

  m_smartCopyCutSet = true;
  m_smartCopyCut = on;

  configEnd ();
}

bool KateViewConfig::scrollPastEnd () const
{
  if (m_scrollPastEndSet || isGlobal())
    return m_scrollPastEnd;

  return s_global->scrollPastEnd();
}

void KateViewConfig::setScrollPastEnd (bool on)
{
  configStart ();

  m_scrollPastEndSet = true;
  m_scrollPastEnd = on;

  configEnd ();
}

//END

//BEGIN KateRendererConfig
KateRendererConfig::KateRendererConfig ()
 : m_fontMetrics(QFont()),
   m_lineMarkerColor (KTextEditor::MarkInterface::reservedMarkersCount()),
   m_schemaSet (true),
   m_fontSet (true),
   m_wordWrapMarkerSet (true),
   m_showIndentationLinesSet (true),
   m_showWholeBracketExpressionSet (true),
   m_backgroundColorSet (true),
   m_selectionColorSet (true),
   m_highlightedLineColorSet (true),
   m_highlightedBracketColorSet (true),
   m_wordWrapMarkerColorSet (true),
   m_tabMarkerColorSet(true),
   m_indentationLineColorSet(true),
   m_iconBarColorSet(true),
   m_foldingColorSet(true),
   m_lineNumberColorSet (true),
   m_separatorColorSet (true),
   m_spellingMistakeLineColorSet (true),
   m_templateColorsSet(true),
   m_modifiedLineColorSet(true),
   m_savedLineColorSet(true),
   m_searchHighlightColorSet(true),
   m_replaceHighlightColorSet(true),
   m_lineMarkerColorSet (m_lineMarkerColor.size()),
   m_renderer (0)
{
  // init bitarray
  m_lineMarkerColorSet.fill (true);

  s_global = this;

  // init with defaults from config or really hardcoded ones
  KConfigGroup config(KGlobal::config(), "Kate Renderer Defaults");
  readConfig (config);
}

KateRendererConfig::KateRendererConfig (KateRenderer *renderer)
 : m_fontMetrics(QFont()),
   m_lineMarkerColor (KTextEditor::MarkInterface::reservedMarkersCount()),
   m_schemaSet (false),
   m_fontSet (false),
   m_wordWrapMarkerSet (false),
   m_showIndentationLinesSet (false),
   m_showWholeBracketExpressionSet (false),
   m_backgroundColorSet (false),
   m_selectionColorSet (false),
   m_highlightedLineColorSet (false),
   m_highlightedBracketColorSet (false),
   m_wordWrapMarkerColorSet (false),
   m_tabMarkerColorSet (false),
   m_indentationLineColorSet (false),
   m_iconBarColorSet (false),
   m_foldingColorSet (false),
   m_lineNumberColorSet (false),
   m_separatorColorSet (false),
   m_spellingMistakeLineColorSet (false),
   m_templateColorsSet(false),
   m_modifiedLineColorSet(false),
   m_savedLineColorSet(false),
   m_searchHighlightColorSet(false),
   m_replaceHighlightColorSet(false),
   m_lineMarkerColorSet (m_lineMarkerColor.size()),
   m_renderer (renderer)
{
  // init bitarray
  m_lineMarkerColorSet.fill (false);
}

KateRendererConfig::~KateRendererConfig ()
{
}

void KateRendererConfig::readConfig (const KConfigGroup &config)
{
  configStart ();

  // "Normal" Schema MUST BE THERE, see global kateschemarc
  setSchema (config.readEntry("Schema", "Normal"));

  setWordWrapMarker (config.readEntry("Word Wrap Marker", false ));

  setShowIndentationLines (config.readEntry( "Show Indentation Lines", false));

  setShowWholeBracketExpression (config.readEntry( "Show Whole Bracket Expression", false));

  configEnd ();
}

void KateRendererConfig::writeConfig (KConfigGroup& config)
{
  config.writeEntry ("Schema", schema());

  config.writeEntry("Word Wrap Marker", wordWrapMarker() );

  config.writeEntry("Show Indentation Lines", showIndentationLines());

  config.writeEntry("Show Whole Bracket Expression", showWholeBracketExpression());
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

const QString &KateRendererConfig::schema () const
{
  if (m_schemaSet || isGlobal())
    return m_schema;

  return s_global->schema();
}

void KateRendererConfig::setSchema (const QString &schema)
{
  configStart ();
  m_schemaSet = true;
  m_schema = schema;
  setSchemaInternal( schema );
  configEnd ();
}

void KateRendererConfig::reloadSchema()
{
  if ( isGlobal() ) {
    setSchemaInternal( m_schema );
    foreach (KateView* view, KateGlobal::self()->views() )
      view->renderer()->config()->reloadSchema();
  }

  else if ( m_renderer && m_schemaSet )
    setSchemaInternal( m_schema );
}

void KateRendererConfig::setSchemaInternal( const QString &schema )
{
  m_schemaSet = true;
  m_schema = schema;

  KConfigGroup config = KateGlobal::self()->schemaManager()->schema(schema);

  // NOTE keep in sync with KateSchemaConfigColorTab::schemaChanged
  KColorScheme schemeView(QPalette::Active, KColorScheme::View);
  KColorScheme schemeWindow(QPalette::Active, KColorScheme::Window);
  KColorScheme schemeSelection(QPalette::Active, KColorScheme::Selection);
  QColor tmp0( schemeView.background().color() );
  QColor tmp1( schemeSelection.background().color() );
  QColor tmp2( schemeView.background(KColorScheme::AlternateBackground).color() );
  // using KColorUtils::shade wasn't working really well
  qreal bgLuma = KColorUtils::luma( tmp0 );
  QColor tmp3( KColorUtils::tint(tmp0, schemeView.decoration(KColorScheme::HoverColor).color()) );
  QColor tmp4( KColorUtils::shade( tmp0, bgLuma > 0.3 ? -0.15 : 0.03 ) );
  QColor tmp5( KColorUtils::shade( tmp0, bgLuma > 0.7 ? -0.35 : 0.3 ) );
  QColor tmp6( schemeWindow.background().color() );
  QColor tmp7( schemeWindow.foreground().color() );
  QColor tmp8( schemeView.foreground(KColorScheme::NegativeText).color() );
  QColor tmp9( schemeView.background(KColorScheme::NegativeBackground).color() );
  QColor tmp10( schemeView.background(KColorScheme::PositiveBackground).color() );
  QColor tmp11( schemeView.background(KColorScheme::NeutralBackground).color() );
  QColor tmp12( KColorScheme(QPalette::Inactive, KColorScheme::Selection).background().color() );
  QColor tmp13( schemeView.foreground(KColorScheme::InactiveText).color() );

  m_backgroundColor = config.readEntry("Color Background", tmp0);
  m_backgroundColorSet = true;
  m_selectionColor = config.readEntry("Color Selection", tmp1);
  m_selectionColorSet = true;
  m_highlightedLineColor  = config.readEntry("Color Highlighted Line", tmp2);
  m_highlightedLineColorSet = true;
  m_highlightedBracketColor = config.readEntry("Color Highlighted Bracket", tmp3);
  m_highlightedBracketColorSet = true;
  m_wordWrapMarkerColor = config.readEntry("Color Word Wrap Marker", tmp4);
  m_wordWrapMarkerColorSet = true;
  m_tabMarkerColor = config.readEntry("Color Tab Marker", tmp5);
  m_tabMarkerColorSet = true;
  m_indentationLineColor = config.readEntry("Color Indentation Line", tmp5);
  m_indentationLineColorSet = true;
  m_iconBarColor  = config.readEntry("Color Icon Bar", tmp6);
  m_iconBarColorSet = true;
  m_foldingColor  = config.readEntry("Color Code Folding", tmp12);
  m_foldingColorSet = true;
  m_lineNumberColor = config.readEntry("Color Line Number", tmp7);
  m_lineNumberColorSet = true;
  m_separatorColor = config.readEntry("Color Separator", tmp13);
  m_separatorColorSet = true;
  m_spellingMistakeLineColor = config.readEntry("Color Spelling Mistake Line", tmp8);
  m_spellingMistakeLineColorSet = true;

  m_modifiedLineColor = config.readEntry("Color Modified Lines", tmp9);
  m_modifiedLineColorSet = true;
  m_savedLineColor = config.readEntry("Color Saved Lines", tmp10);
  m_savedLineColorSet = true;
  m_searchHighlightColor = config.readEntry("Color Search Highlight", QColor(Qt::yellow)); // tmp11);
  m_searchHighlightColorSet = true;
  m_replaceHighlightColor = config.readEntry("Color Replace Highlight", QColor(Qt::green)); // tmp10);
  m_replaceHighlightColorSet = true;


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
    QColor col = config.readEntry(QString("Color MarkType %1").arg(i), mark[i - 1]);
    int index = i-1;
    m_lineMarkerColorSet[index] = true;
    m_lineMarkerColor[index] = col;
  }

  QFont f (KGlobalSettings::fixedFont());

  m_font = config.readEntry("Font", f);
  m_fontMetrics = QFontMetricsF (m_font);
  m_fontSet = true;

  QColor c = schemeWindow.background().color();
  m_templateBackgroundColor = config.readEntry(QString("Color Template Background"), c);

  c = schemeView.background(KColorScheme::PositiveBackground).color();
  m_templateFocusedEditablePlaceholderColor = config.readEntry(QString("Color Template Focused Editable Placeholder"), c);

  c = schemeWindow.background(KColorScheme::PositiveBackground).color();
  m_templateEditablePlaceholderColor = config.readEntry(QString("Color Template Editable Placeholder"), c);

  c = schemeView.background(KColorScheme::NegativeBackground).color();
  m_templateNotEditablePlaceholderColor = config.readEntry(QString("Color Template Not Editable Placeholder"), c);

  m_templateColorsSet=true;
}

const QFont& KateRendererConfig::font() const
{
  if (m_fontSet || isGlobal())
    return m_font;

  return s_global->font();
}

const QFontMetricsF& KateRendererConfig::fontMetrics() const
{
  if (m_fontSet || isGlobal())
    return m_fontMetrics;

  return s_global->fontMetrics();
}

void KateRendererConfig::setFont(const QFont &font)
{
  configStart ();

  m_fontSet = true;
  m_font = font;
  m_fontMetrics = QFontMetricsF (m_font);

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

const QColor& KateRendererConfig::indentationLineColor() const
{
  if (m_indentationLineColorSet || isGlobal())
    return m_indentationLineColor;

  return s_global->indentationLineColor();
}

void KateRendererConfig::setIndentationLineColor (const QColor &col)
{
  configStart ();

  m_indentationLineColorSet = true;
  m_indentationLineColor = col;

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

const QColor& KateRendererConfig::foldingColor() const
{
  if (m_foldingColorSet || isGlobal())
    return m_foldingColor;

  return s_global->foldingColor();
}

void KateRendererConfig::setFoldingColor (const QColor &col)
{
  configStart ();

  m_foldingColorSet = true;
  m_foldingColor = col;

  configEnd ();
}

const QColor &KateRendererConfig::templateBackgroundColor() const
{
  if (m_templateColorsSet || isGlobal())
    return m_templateBackgroundColor;

  return s_global->templateBackgroundColor();
}

const QColor &KateRendererConfig::templateEditablePlaceholderColor() const
{
  if (m_templateColorsSet || isGlobal())
    return m_templateEditablePlaceholderColor;

  return s_global->templateEditablePlaceholderColor();
}

const QColor &KateRendererConfig::templateFocusedEditablePlaceholderColor() const
{
  if (m_templateColorsSet || isGlobal())
    return m_templateFocusedEditablePlaceholderColor;

  return s_global->templateFocusedEditablePlaceholderColor();
}

const QColor &KateRendererConfig::templateNotEditablePlaceholderColor() const
{
  if (m_templateColorsSet || isGlobal())
    return m_templateNotEditablePlaceholderColor;

  return s_global->templateNotEditablePlaceholderColor();
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

const QColor& KateRendererConfig::separatorColor() const
{
  if (m_separatorColorSet || isGlobal())
    return m_separatorColor;

  return s_global->separatorColor();
}

void KateRendererConfig::setSeparatorColor(const QColor& col)
{
  configStart ();

  m_separatorColorSet = true;
  m_separatorColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::spellingMistakeLineColor() const
{
  if (m_spellingMistakeLineColorSet || isGlobal())
    return m_spellingMistakeLineColor;

  return s_global->spellingMistakeLineColor();
}

void KateRendererConfig::setSpellingMistakeLineColor (const QColor &col)
{
  configStart ();

  m_spellingMistakeLineColorSet = true;
  m_spellingMistakeLineColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::modifiedLineColor() const
{
  if (m_modifiedLineColorSet || isGlobal())
    return m_modifiedLineColor;

  return s_global->modifiedLineColor();
}

void KateRendererConfig::setModifiedLineColor(const QColor &col)
{
  configStart ();

  m_modifiedLineColorSet = true;
  m_modifiedLineColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::savedLineColor() const
{
  if (m_savedLineColorSet || isGlobal())
    return m_savedLineColor;

  return s_global->savedLineColor();
}

void KateRendererConfig::setSavedLineColor(const QColor &col)
{
  configStart ();

  m_savedLineColorSet = true;
  m_savedLineColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::searchHighlightColor() const
{
  if (m_searchHighlightColorSet || isGlobal())
    return m_searchHighlightColor;

  return s_global->searchHighlightColor();
}

void KateRendererConfig::setSearchHighlightColor(const QColor &col)
{
  configStart ();

  m_searchHighlightColorSet = true;
  m_searchHighlightColor = col;

  configEnd ();
}

const QColor& KateRendererConfig::replaceHighlightColor() const
{
  if (m_replaceHighlightColorSet || isGlobal())
    return m_replaceHighlightColor;

  return s_global->replaceHighlightColor();
}

void KateRendererConfig::setReplaceHighlightColor(const QColor &col)
{
  configStart ();

  m_replaceHighlightColorSet = true;
  m_replaceHighlightColor = col;

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

bool KateRendererConfig::showWholeBracketExpression () const
{
  if (m_showWholeBracketExpressionSet || isGlobal())
    return m_showWholeBracketExpression;

  return s_global->showWholeBracketExpression();
}

void KateRendererConfig::setShowWholeBracketExpression (bool on)
{
  configStart ();

  m_showWholeBracketExpressionSet = true;
  m_showWholeBracketExpression = on;

  configEnd ();
}

//END

// kate: space-indent on; indent-width 2; replace-tabs on;
