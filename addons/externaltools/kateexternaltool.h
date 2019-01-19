/*
   This file is part of the Kate text editor of the KDE project.
   It describes a "external tools" action for kate and provides a dialog
   page to configure that.

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

   ---
   Copyright (C) 2004 Anders Lund <anders@alweb.dk>
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
*/

#ifndef KTEXTEDITOR_KATE_EXTERNALTOOL_H
#define KTEXTEDITOR_KATE_EXTERNALTOOL_H

#include <QObject>
#include <QString>
#include <QStringList>

class KConfigGroup;

/**
 * This class defines a single external tool.
 */
class KateExternalTool
{
    Q_GADGET

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
    Q_ENUM(SaveMode)

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
    explicit KateExternalTool(const QString& name = QString(), const QString& command = QString(),
                              const QString& icon = QString(), const QString& executable = QString(),
                              const QStringList& mimetypes = QStringList(), const QString& actionName = QString(),
                              const QString& cmdname = QString(), SaveMode saveMode = SaveMode::None);
    ~KateExternalTool() = default;

    /// The name used in the menu.
    QString name;
    /// the icon to use in the menu.
    QString icon;
    /// The name or path of the executable.
    QString executable;
    /// The command line arguments.
    QString arguments;
    /// The command to execute.
    QString command;
    /// Optional list of mimetypes for which this action is valid.
    QStringList mimetypes;
    /// This is set by the constructor by calling checkExec(), if a
    /// value is present.
    bool hasexec;
    /// The name for the action. This is generated first time the
    /// action is is created.
    QString actionName;
    /// The name for the commandline.
    QString cmdname;
    /// Possibly save documents prior to activating the tool command.
    SaveMode saveMode = SaveMode::None;

    /// Possibly redirect the stdout output of the tool.
    //OutputMode outputMode;

    /**
     * @return true if mimetypes is empty, or the @p mimetype matches.
     */
    bool valid(const QString& mimetype) const;
    /**
     * @return true if "executable" exists and has the executable bit set, or is
     * empty.
     * This is run at least once, and the tool is disabled if it fails.
     */
    bool checkExec();

    /**
     * Load tool data from the config group @p cg.
     */
    void load(const KConfigGroup & cg);

    /**
     * Save tool data to the config group @p cg.
     */
    void save(KConfigGroup & cg);

private:
    QString m_exec; ///< The fully qualified path of the executable.
};

#endif // KTEXTEDITOR_KATE_EXTERNALTOOL_H

// kate: space-indent on; indent-width 4; replace-tabs on;
