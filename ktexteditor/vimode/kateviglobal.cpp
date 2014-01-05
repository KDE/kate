/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2011 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <stdio.h>

#include "kateviglobal.h"
#include "katevikeyparser.h"
#include "kateviemulatedcommandbar.h"

#include "kconfiggroup.h"
#include "katepartdebug.h"
#include <ktexteditor/movingcursor.h>
#include <QApplication>
#include <QClipboard>

KateViGlobal::KateViGlobal()
{
}

KateViGlobal::~KateViGlobal()
{
}

void KateViGlobal::writeConfig( KConfigGroup &config ) const
{
  writeMappingsToConfig(config, QLatin1String("Normal"), NormalModeMapping);
  writeMappingsToConfig(config, QLatin1String("Visual"), VisualModeMapping);
  writeMappingsToConfig(config, QLatin1String("Insert"), InsertModeMapping);
  writeMappingsToConfig(config, QLatin1String("Command"), CommandModeMapping);

  QStringList macroRegisters;
  foreach(const QChar& macroRegister, m_macroForRegister.keys())
  {
    macroRegisters.append(macroRegister);
  }
  QStringList macroContents;
  foreach(const QChar& macroRegister, m_macroForRegister.keys())
  {
    macroContents.append(KateViKeyParser::self()->decodeKeySequence(m_macroForRegister[macroRegister]));
  }
  QStringList macroCompletions;
  foreach(const QChar& macroRegister, m_macroForRegister.keys())
  {
    macroCompletions.append(QString::number(m_macroCompletionsForRegister[macroRegister].length()));
    foreach(KateViInputModeManager::Completion completionForMacro, m_macroCompletionsForRegister[macroRegister])
    {
      macroCompletions.append(encodeMacroCompletionForConfig(completionForMacro));
    }
  }
  config.writeEntry("Macro Registers", macroRegisters);
  config.writeEntry("Macro Contents", macroContents);
  config.writeEntry("Macro Completions", macroCompletions);
}

void KateViGlobal::readConfig( const KConfigGroup &config )
{
  readMappingsFromConfig(config, QLatin1String("Normal"), NormalModeMapping);
  readMappingsFromConfig(config, QLatin1String("Visual"), VisualModeMapping);
  readMappingsFromConfig(config, QLatin1String("Insert"), InsertModeMapping);
  readMappingsFromConfig(config, QLatin1String("Command"), CommandModeMapping);

  const QStringList macroRegisters = config.readEntry("Macro Registers", QStringList());
  const QStringList macroContents = config.readEntry("Macro Contents", QStringList());
  const QStringList macroCompletions = config.readEntry("Macro Completions", QStringList());
  int macroCompletionsIndex = 0;
  if (macroRegisters.length() == macroContents.length())
  {
    for (int macroIndex = 0; macroIndex < macroRegisters.length(); macroIndex++)
    {
      const QChar macroRegister = macroRegisters[macroIndex].at(0);
      m_macroForRegister[macroRegister] = KateViKeyParser::self()->encodeKeySequence(macroContents[macroIndex]);
      macroCompletionsIndex = readMacroCompletions(macroRegister, macroCompletions, macroCompletionsIndex);
    }
  }
}

KateViRegister KateViGlobal::getRegister( const QChar &reg ) const
{
  KateViRegister regPair;
  QChar _reg = ( reg != QLatin1Char('"') ? reg : m_defaultRegister );

  if ( _reg >= QLatin1Char('1') && _reg <= QLatin1Char('9') ) { // numbered register
    int index = QString( _reg ).toInt()-1;
    if ( m_numberedRegisters.size() > index) {
      regPair = m_numberedRegisters.at( index );
    }
  } else if ( _reg == QLatin1Char('+') ) { // system clipboard register
      QString regContent = QApplication::clipboard()->text( QClipboard::Clipboard );
      regPair = KateViRegister( regContent, CharWise );
  } else if ( _reg == QLatin1Char('*') ) { // system selection register
      QString regContent = QApplication::clipboard()->text( QClipboard::Selection );
      regPair = KateViRegister( regContent, CharWise );
  } else { // regular, named register
    if ( m_registers.contains( _reg ) ) {
      regPair = m_registers.value( _reg );
    }
  }

  return regPair;
}

QString KateViGlobal::getRegisterContent( const QChar &reg ) const
{
  return getRegister( reg ).first;
}

OperationMode KateViGlobal::getRegisterFlag( const QChar &reg ) const
{
  return getRegister( reg ).second;
}

void KateViGlobal::addToNumberedRegister( const QString &text, OperationMode flag )
{
  if ( m_numberedRegisters.size() == 9 ) {
    m_numberedRegisters.removeLast();
  }

  // register 0 is used for the last yank command, so insert at position 1
  m_numberedRegisters.prepend( KateViRegister(text, flag ) );

  qCDebug(LOG_PART) << "Register 1-9:";
  for ( int i = 0; i < m_numberedRegisters.size(); i++ ) {
      qCDebug(LOG_PART) << "\t Register " << i+1 << ": " << m_numberedRegisters.at( i );
  }
}

void KateViGlobal::fillRegister( const QChar &reg, const QString &text, OperationMode flag )
{
  // the specified register is the "black hole register", don't do anything
  if ( reg == QLatin1Char('_') ) {
    return;
  }

  if ( reg >= QLatin1Char('1') && reg <= QLatin1Char('9') ) { // "kill ring" registers
    addToNumberedRegister( text );
  } else if ( reg == QLatin1Char('+') ) { // system clipboard register
      QApplication::clipboard()->setText( text,  QClipboard::Clipboard );
  } else if ( reg == QLatin1Char('*') ) { // system selection register
      QApplication::clipboard()->setText( text, QClipboard::Selection );
  } else {
    m_registers.insert( reg, KateViRegister(text, flag) );
  }

  qCDebug(LOG_PART) << "Register " << reg << " set to " << getRegisterContent( reg );

  if ( reg == QLatin1Char('0') || reg == QLatin1Char('1') || reg == QLatin1Char('-') ) {
    m_defaultRegister = reg;
    qCDebug(LOG_PART) << "Register " << '"' << " set to point to \"" << reg;
  }
}

void KateViGlobal::addMapping( MappingMode mode, const QString& from, const QString& to, KateViGlobal::MappingRecursion recursion )
{
  const QString encodedMapping = KateViKeyParser::self()->encodeKeySequence( from );
  const QString encodedTo = KateViKeyParser::self()->encodeKeySequence( to );
  const Mapping mapping(encodedTo, recursion == KateViGlobal::Recursive);
  if ( !from.isEmpty() ) {
    m_mappingsForMode[mode][encodedMapping] = mapping;
  }
}

void KateViGlobal::removeMapping(MappingMode mode, const QString& from)
{
  m_mappingsForMode[mode].remove(from);
}

const QString KateViGlobal::getMapping( MappingMode mode, const QString& from, bool decode ) const
{
    const QString ret = m_mappingsForMode[mode][from].mappedKeyPresses;

    if ( decode ) {
      return KateViKeyParser::self()->decodeKeySequence( ret );
    }
    return ret;
}

const QStringList KateViGlobal::getMappings( MappingMode mode, bool decode ) const
{
  const QHash <QString, Mapping> mappingsForMode = m_mappingsForMode[mode];

  QStringList mappings;
  foreach(const QString mapping, mappingsForMode.keys())
  {
    if ( decode ) {
      mappings << KateViKeyParser::self()->decodeKeySequence(mapping);
    } else {
      mappings << mapping;
    }
  }

  return mappings;
}

bool KateViGlobal::isMappingRecursive(MappingMode mode, const QString& from) const
{
    return m_mappingsForMode[mode][from].isRecursive;
}

KateViGlobal::MappingMode KateViGlobal::mappingModeForCurrentViMode(KateView* view)
{
  if (view->viModeEmulatedCommandBar()->isActive())
  {
    return CommandModeMapping;
  }
  const ViMode mode = view->getCurrentViMode();
  switch(mode)
  {
    case NormalMode:
      return NormalModeMapping;
    case VisualMode:
    case VisualLineMode:
    case VisualBlockMode:
      return VisualModeMapping;
    case InsertMode:
    case ReplaceMode:
      return InsertModeMapping;
    default:
      Q_ASSERT(false && "unrecognised ViMode!");
      return NormalModeMapping; // Return arbitrary mode to satisfy compiler.
  }
}

void KateViGlobal::clearMappings( MappingMode mode )
{
  m_mappingsForMode[mode].clear();
}

void KateViGlobal::clearSearchHistory()
{
  m_searchHistory.clear();
}

QStringList KateViGlobal::searchHistory()
{
  return m_searchHistory.items();
}

void KateViGlobal::appendSearchHistoryItem(const QString& searchHistoryItem)
{
  m_searchHistory.appendItem(searchHistoryItem);
}

QStringList KateViGlobal::commandHistory()
{
  return m_commandHistory.items();
}

void KateViGlobal::clearCommandHistory()
{
  m_commandHistory.clear();
}

void KateViGlobal::appendCommandHistoryItem(const QString& commandHistoryItem)
{
  m_commandHistory.appendItem(commandHistoryItem);
}

QStringList KateViGlobal::replaceHistory()
{
  return m_replaceHistory.items();
}

void KateViGlobal::appendReplaceHistoryItem(const QString& replaceHistoryItem)
{
  m_replaceHistory.appendItem(replaceHistoryItem);
}

void KateViGlobal::clearReplaceHistory()
{
  m_replaceHistory.clear();
}

void KateViGlobal::clearAllMacros()
{
  m_macroForRegister.clear();
}

void KateViGlobal::clearMacro(QChar macroRegister)
{
  m_macroForRegister[macroRegister].clear();
}

void KateViGlobal::storeMacro(QChar macroRegister, const QList< QKeyEvent > macroKeyEventLog, const QList< KateViInputModeManager::Completion > completions)
{
  m_macroForRegister[macroRegister].clear();
  QList <QKeyEvent> withoutClosingQ = macroKeyEventLog;
  Q_ASSERT(!macroKeyEventLog.isEmpty() && macroKeyEventLog.last().key() == Qt::Key_Q);
  withoutClosingQ.pop_back();
  foreach(QKeyEvent keyEvent, withoutClosingQ)
  {
    const QChar key = KateViKeyParser::self()->KeyEventToQChar(keyEvent);
    m_macroForRegister[macroRegister].append(key);
  }
  m_macroCompletionsForRegister[macroRegister] = completions;
}

QString KateViGlobal::getMacro(QChar macroRegister)
{
  return m_macroForRegister[macroRegister];
}

QList< KateViInputModeManager::Completion > KateViGlobal::getMacroCompletions(QChar macroRegister)
{
  return m_macroCompletionsForRegister[macroRegister];
}

void KateViGlobal::writeMappingsToConfig(KConfigGroup& config, const QString& mappingModeName, MappingMode mappingMode) const
{
  config.writeEntry( mappingModeName + QLatin1String(" Mode Mapping Keys"), getMappings( mappingMode, true ) );
  QStringList l;
  QList<bool> isRecursive;
  foreach( const QString &s, getMappings( mappingMode ) ) {
    l << KateViKeyParser::self()->decodeKeySequence( getMapping( mappingMode, s ) );
    isRecursive << isMappingRecursive( mappingMode, s );
  }
  config.writeEntry( mappingModeName + QLatin1String(" Mode Mappings"), l );
  config.writeEntry( mappingModeName + QLatin1String(" Mode Mappings Recursion"), isRecursive );
}

void KateViGlobal::readMappingsFromConfig(const KConfigGroup& config, const QString& mappingModeName, MappingMode mappingMode)
{
  const QStringList keys = config.readEntry( mappingModeName + QLatin1String(" Mode Mapping Keys"), QStringList() );
  const QStringList mappings = config.readEntry( mappingModeName + QLatin1String(" Mode Mappings"), QStringList() );
  const QList<bool> isRecursive = config.readEntry( mappingModeName + QLatin1String(" Mode Mappings Recursion"), QList<bool>());

  // sanity check
  if ( keys.length() == mappings.length() ) {
    for ( int i = 0; i < keys.length(); i++ ) {
      // "Recursion" is a newly-introduced part of the config that some users won't have,
      // so rather than abort (and lose our mappings) if there are not enough entries, simply
      // treat any missing ones as Recursive (for backwards compatibility).
      MappingRecursion recursion = Recursive;
      if (isRecursive.size() > i && !isRecursive.at(i))
      {
        recursion = NonRecursive;
      }
      addMapping( mappingMode, keys.at( i ), mappings.at( i ), recursion);
      qCDebug(LOG_PART) <<  + " mapping " << keys.at( i ) << " -> " << mappings.at( i );
    }
  } else {
    qCDebug(LOG_PART) << "Error when reading mappings from " << mappingModeName << " config: number of keys != number of values";
  }
}

int KateViGlobal::readMacroCompletions(QChar macroRegister, const QStringList& encodedMacroCompletions, int macroCompletionsIndex)
{
  if (macroCompletionsIndex < encodedMacroCompletions.length())
  {
    bool parsedNumCompletionsSuccessfully = false;
    const QString numCompletionsAsString = encodedMacroCompletions[macroCompletionsIndex++];
    const int numCompletions = numCompletionsAsString.toInt(&parsedNumCompletionsSuccessfully);
    int count = 0;
    m_macroCompletionsForRegister[macroRegister].clear();
    while (count < numCompletions && macroCompletionsIndex < encodedMacroCompletions.length())
    {
      const QString encodedMacroCompletion = encodedMacroCompletions[macroCompletionsIndex++];
      count++;
      m_macroCompletionsForRegister[macroRegister].append(decodeMacroCompletionFromConfig(encodedMacroCompletion));

    }
  }
  return macroCompletionsIndex;
}

QString KateViGlobal::encodeMacroCompletionForConfig(const KateViInputModeManager::Completion& completionForMacro) const
{
  const bool endedWithSemiColon = completionForMacro.completedText().endsWith(QLatin1String(";"));
  QString encodedMacroCompletion = completionForMacro.completedText().remove(QLatin1String("()")).remove(QLatin1String(";"));
  if (completionForMacro.completionType() == KateViInputModeManager::Completion::FunctionWithArgs)
  {
    encodedMacroCompletion += QLatin1String("(...)");
  }
  else if (completionForMacro.completionType() == KateViInputModeManager::Completion::FunctionWithoutArgs)
  {
    encodedMacroCompletion += QLatin1String("()");
  }
  if (endedWithSemiColon)
  {
    encodedMacroCompletion += QLatin1Char(';');
  }
  if (completionForMacro.removeTail())
  {
    encodedMacroCompletion += QLatin1Char('|');
  }
  return encodedMacroCompletion;
}

KateViInputModeManager::Completion KateViGlobal::decodeMacroCompletionFromConfig(const QString& encodedMacroCompletion)
{
  const bool removeTail = encodedMacroCompletion.endsWith(QLatin1String("|"));
  KateViInputModeManager::Completion::CompletionType completionType = KateViInputModeManager::Completion::PlainText;
  if (encodedMacroCompletion.contains(QLatin1String("(...)")))
  {
    completionType = KateViInputModeManager::Completion::FunctionWithArgs;
  }
  else if (encodedMacroCompletion.contains(QLatin1String("()")))
  {
    completionType = KateViInputModeManager::Completion::FunctionWithoutArgs;
  }
  QString completionText = encodedMacroCompletion;
  completionText.replace(QLatin1String("(...)"), QLatin1String("()")).remove(QLatin1String("|"));

  qCDebug(LOG_PART) << "Loaded completion: " << completionText << " , " << removeTail << " , " << completionType;

  return KateViInputModeManager::Completion(completionText, removeTail, completionType);

}
