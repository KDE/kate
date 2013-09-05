/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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

#ifndef KATE_VI_GLOBAL_H_INCLUDED
#define KATE_VI_GLOBAL_H_INCLUDED

#include <QMap>
#include <QHash>
#include <QList>
#include <QPair>

#include "katevimodebase.h"
#include "kateviinputmodemanager.h"
#include "katepartprivate_export.h"

class QString;
class QChar;
class KConfigGroup;


namespace KateVi {
  const unsigned int EOL = 99999;
}

typedef QPair<QString, OperationMode> KateViRegister;

class KATEPART_TESTS_EXPORT KateViGlobal
{
public:
    KateViGlobal();
    ~KateViGlobal();

    void writeConfig( KConfigGroup &config ) const;
    void readConfig( const KConfigGroup &config );
    QString getRegisterContent( const QChar &reg ) const;
    OperationMode getRegisterFlag( const QChar &reg ) const;
    void addToNumberedRegister( const QString &text, OperationMode flag = CharWise );
    void fillRegister( const QChar &reg, const QString &text, OperationMode flag = CharWise);
    const QMap<QChar, KateViRegister>* getRegisters() const { return &m_registers; }

    enum MappingRecursion { Recursive, NonRecursive };
    enum MappingMode { NormalModeMapping, VisualModeMapping, InsertModeMapping, CommandModeMapping };
    void clearMappings( MappingMode mode );
    void addMapping( MappingMode mode, const QString& from, const QString& to, MappingRecursion recursion );
    void removeMapping(MappingMode mode, const QString& from);
    const QString getMapping( MappingMode mode, const QString &from, bool decode = false ) const;
    const QStringList getMappings( MappingMode mode, bool decode = false ) const;
    bool isMappingRecursive(MappingMode mode, const QString& from) const;
    /**
     * Returns CommandModeMapping if the emulated command bar is active, else the mapping mode
     * corresponding to the current Vi mode.
     */
    static MappingMode mappingModeForCurrentViMode(KateView* view);

    QStringList searchHistory();
    void clearSearchHistory();
    void appendSearchHistoryItem(const QString& searchHistoryItem);

    QStringList commandHistory();
    void clearCommandHistory();
    void appendCommandHistoryItem(const QString& commandHistoryItem);

    QStringList replaceHistory();
    void clearReplaceHistory();
    void appendReplaceHistoryItem(const QString& replaceHistoryItem);

    void clearAllMacros();
    void clearMacro(QChar macroRegister);
    void storeMacro(QChar macroRegister, const QList<QKeyEvent> macroKeyEventLog, const QList<KateViInputModeManager::Completion> completions);
    /**
     * Get the named macro in a format suitable for passing to feedKeyPresses.
     */
    QString getMacro(QChar macroRegister);
    QList< KateViInputModeManager::Completion > getMacroCompletions(QChar macroRegister);

private:
    // registers
    QList<KateViRegister> m_numberedRegisters;
    QMap<QChar, KateViRegister> m_registers;
    QChar m_defaultRegister;
    QString m_registerTemp;
    KateViRegister getRegister( const QChar &reg ) const;

    // Mappings.
    struct Mapping
    {
      Mapping(const QString& mappedKeyPresses, bool isRecursive)
        : mappedKeyPresses(mappedKeyPresses), isRecursive(isRecursive)
      {
      }
      Mapping(const Mapping& other)
        : mappedKeyPresses(other.mappedKeyPresses), isRecursive(other.isRecursive)
      {
      }
      Mapping()
        : mappedKeyPresses(), isRecursive(false)
      {
      }
      Mapping& operator=(const Mapping& other)
      {
        mappedKeyPresses = other.mappedKeyPresses;
        isRecursive = other.isRecursive;
        return *this;
      }
      QString mappedKeyPresses;
      bool isRecursive;
    };

    QHash <MappingMode, QHash<QString, Mapping> > m_mappingsForMode;

    void writeMappingsToConfig(KConfigGroup& config, const QString& mappingModeName, MappingMode mappingMode) const;
    void readMappingsFromConfig(const KConfigGroup& config, const QString& mappingModeName, MappingMode mappingMode);
    int readMacroCompletions(QChar macroRegister, const QStringList& encodedMacroCompletions, int macroCompletionIndex);
    QString encodeMacroCompletionForConfig(const KateViInputModeManager::Completion& completionForMacro) const;
    KateViInputModeManager::Completion decodeMacroCompletionFromConfig(const QString& encodedMacroCompletion);

    class History
    {
    public:
      void appendItem(const QString& historyItem)
      {
        if (historyItem.isEmpty())
        {
          return;
        }
        const int HISTORY_SIZE_LIMIT = 100;
        m_items.removeAll(historyItem);
        if (m_items.size() == HISTORY_SIZE_LIMIT)
        {
          m_items.removeFirst();
        }
        m_items.append(historyItem);
      }
      QStringList items() const
      {
        return m_items;
      }
      void clear()
      {
        m_items.clear();
      }
    private:
      QStringList m_items;
    };

    History m_searchHistory;
    History m_commandHistory;
    History m_replaceHistory;

    QHash<QChar, QString > m_macroForRegister;
    QHash<QChar, QList<KateViInputModeManager::Completion> > m_macroCompletionsForRegister;
};

#endif
