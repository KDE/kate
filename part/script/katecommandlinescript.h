// This file is part of the KDE libraries
// Copyright (C) 2009 Dominik Haumann <dhaumann kde org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#ifndef KATE_COMMANDLINE_SCRIPT_H
#define KATE_COMMANDLINE_SCRIPT_H

#include "katescript.h"
#include "kateview.h"

#include <QtCore/QPair>
#include <ktexteditor/commandinterface.h>

class KateScriptDocument;

class KateCommandLineScriptHeader : public KateScriptHeader
{
  public:
    KateCommandLineScriptHeader() : KateScriptHeader()
    {}

    KateCommandLineScriptHeader(const KateScriptHeader& scriptHeader)
    : KateScriptHeader()
    {
      *static_cast<KateScriptHeader*>(this) = scriptHeader;
    }

    inline void setFunctions(const QStringList& functions)
    { m_functions = functions; }
    inline const QStringList& functions() const
    { return m_functions; }

  private:
    QStringList m_functions; ///< the functions the script contains
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

    const KateCommandLineScriptHeader& header();

    bool callFunction(const QString& cmd, const QStringList args, QString &errorMessage);

  //
  // KTextEditor::Command interface
  //
  public:
    virtual const QStringList &cmds ();
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg);
    virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg);

  private:
    KateCommandLineScriptHeader m_header;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
