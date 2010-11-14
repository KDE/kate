/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009 Dominik Haumann <dhaumann kde org>
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

#ifndef KATE_COMMANDLINE_SCRIPT_H
#define KATE_COMMANDLINE_SCRIPT_H

#include "katescript.h"
#include "kateview.h"

#include <QtCore/QPair>
#include <ktexteditor/commandinterface.h>

class KateScriptDocument;

class KateCommandLineScriptHeader
{
  public:
    KateCommandLineScriptHeader()
    {}

    inline void setFunctions(const QStringList& functions)
    { m_functions = functions; }
    inline const QStringList& functions() const
    { return m_functions; }

  private:
    QStringList m_functions; ///< the functions the script contains
};

class ScriptActionInfo
{
  public:
    ScriptActionInfo() {}
    
    inline bool isValid() const { return !(m_command.isEmpty() || m_text.isEmpty()); }

    inline void setCommand(const QString& command) { m_command = command; }
    inline QString command() const { return m_command; }
    inline void setText(const QString& text) { m_text = text; }
    inline QString text() const { return m_text; }
    inline void setIcon(const QString& icon) { m_icon = icon; }
    inline QString icon() const { return m_icon; }
    inline void setCategory(const QString& category) { m_category = category; }
    inline QString category() const { return m_category; }
    inline void setInteractive(bool interactive) { m_interactive = interactive; }
    inline bool interactive() const { return m_interactive; }
    inline void setShortcut(const QString& shortcut) { m_shortcut = shortcut; }
    inline QString shortcut() const { return m_shortcut; }

  private:
    QString m_command;
    QString m_text;
    QString m_icon;
    QString m_category;
    bool m_interactive;
    QString m_shortcut;
};

/**
 * A specialized class for scripts that are of type
 * KateScriptInformation::IndentationScript
 */
class KateCommandLineScript : public KateScript, public KTextEditor::Command
{
  public:
    KateCommandLineScript(const QString &url, const KateCommandLineScriptHeader &header);
    virtual ~KateCommandLineScript();

    const KateCommandLineScriptHeader& commandHeader();

    bool callFunction(const QString& cmd, const QStringList args, QString &errorMessage);
    
    ScriptActionInfo actionInfo(const QString& cmd);

  //
  // KTextEditor::Command interface
  //
  public:
    virtual const QStringList &cmds ();
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg);
    virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg);

  private:
    KateCommandLineScriptHeader m_commandHeader;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
