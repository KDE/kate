/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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
#ifndef KTEXTEDITOR_KATE_EXTERNALTOOL_H
#define KTEXTEDITOR_KATE_EXTERNALTOOL_H

#include <QMetaType>
#include <QString>
#include <QStringList>

class KConfigGroup;

/**
 * This class defines a single external tool.
 */
class KateExternalTool
{
public:
    /**
     * Defines whether any document should be saved before running the tool.
     */
    enum class SaveMode {
        //! Do not save any document.
        None,
        //! Save current document.
        CurrentDocument,
        //! Save all documents
        AllDocuments
    };

    /**
     * Defines where to redirect stdout from the tool.
     */
    //     enum class OutputMode {
    //         Ignore,
    //         InsertAtCursor,
    //         ReplaceSelectedText,
    //         AppendToCurrentDocument,
    //         InsertInNewDocument,
    //         DisplayInPane
    //     }
    //     Q_ENUM(OutputMode)

public:
    /// The category used in the menu to categorize the tool.
    QString category;
    /// The name used in the menu.
    QString name;
    /// the icon to use in the menu.
    QString icon;
    /// The name or path of the executable.
    QString executable;
    /// The command line arguments.
    QString arguments;
    /// The stdin input.
    QString input;
    /// The working directory, if specified.
    QString workingDir;
    /// Optional list of mimetypes for which this action is valid.
    QStringList mimetypes;
    /// This is set by the constructor by calling checkExec(), if a
    /// value is present.
    bool hasexec = false;
    /// The name for the action. This is generated first time the
    /// action is is created.
    QString actionName;
    /// The name for the commandline.
    QString cmdname;
    /// Possibly save documents prior to activating the tool command.
    SaveMode saveMode = SaveMode::None;

    /// Possibly redirect the stdout output of the tool.
    // OutputMode outputMode;
    /// Include stderr output when running the tool.
    bool includeStderr = false;

    /**
     * @return true if mimetypes is empty, or the @p mimetype matches.
     */
    bool matchesMimetype(const QString& mimetype) const;

    /**
     * @return true if "executable" exists and has the executable bit set, or is
     * empty.
     * This is run at least once, and the tool is disabled if it fails.
     */
    bool checkExec() const;

    /**
     * Load tool data from the config group @p cg.
     */
    void load(const KConfigGroup& cg);

    /**
     * Save tool data to the config group @p cg.
     */
    void save(KConfigGroup& cg);
};

Q_DECLARE_METATYPE(KateExternalTool*)

#endif // KTEXTEDITOR_KATE_EXTERNALTOOL_H

// kate: space-indent on; indent-width 4; replace-tabs on;
