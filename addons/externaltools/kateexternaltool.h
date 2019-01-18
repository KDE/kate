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

#include <QString>
#include <QStringList>

/**
 * This class defines a single external tool.
 */
class KateExternalTool
{
public:
    explicit KateExternalTool(const QString& name = QString(), const QString& command = QString(),
                              const QString& icon = QString(), const QString& executable = QString(),
                              const QStringList& mimetypes = QStringList(), const QString& acname = QString(),
                              const QString& cmdname = QString(), int save = 0);
    ~KateExternalTool() = default;

    QString name;       ///< The name used in the menu.
    QString icon;       ///< the icon to use in the menu.
    QString executable; ///< The name or path of the executable.
    QString arguments;  ///< The command line arguments.
    QString command;    ///< The command to execute.
    QStringList mimetypes; ///< Optional list of mimetypes for which this action is valid.
    bool hasexec;       ///< This is set by the constructor by calling checkExec(), if a
                        ///< value is present.
    QString acname;     ///< The name for the action. This is generated first time the
                        ///< action is is created.
    QString cmdname;    ///< The name for the commandline.
    int save;           ///< We can save documents prior to activating the tool command:
                        ///< 0 = nothing, 1 = current document, 2 = all documents.

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

private:
    QString m_exec; ///< The fully qualified path of the executable.
};

#endif // KTEXTEDITOR_KATE_EXTERNALTOOL_H

// kate: space-indent on; indent-width 4; replace-tabs on;
