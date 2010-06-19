// This file is part of the KDE libraries
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
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

#ifndef KATE_INDENT_SCRIPT_H
#define KATE_INDENT_SCRIPT_H

#include "katescript.h"
#include "kateview.h"

#include <QtCore/QPair>

class KateScriptDocument;

class KateIndentScriptHeader
{
  public:
    KateIndentScriptHeader() : m_priority(0)
    {}

    inline void setName(const QString& name)
    { m_name = name; }
    inline const QString& name() const
    { return m_name; }

    inline void setRequiredStyle(const QString& requiredStyle)
    { m_requiredStyle = requiredStyle; }
    inline const QString& requiredStyle() const
    { return m_requiredStyle; }

    inline void setIndentLanguages(const QStringList& indentLanguages)
    { m_indentLanguages = indentLanguages; }
    inline const QStringList& indentLanguages() const
    { return m_indentLanguages; }

    inline void setPriority(int priority)
    { m_priority = priority; }
    inline int priority() const
    { return m_priority; }

    inline void setBaseName(const QString& baseName)
    { m_baseName = baseName; }
    inline const QString& baseName() const
    { return m_baseName; }

  private:
    QString m_name; ///< indenter name, e.g. Python

   /**
    * If this is an indenter, then this specifies the required syntax
    * highlighting style that must be used for this indenter to work properly.
    * If this property is empty, the indenter doesn't require a specific style.
    */
    QString m_requiredStyle;
    /**
    * If this script is an indenter, then the indentLanguages member specifies
    * which languages this is an indenter for. The values must correspond with
    * the name of a programming language given in a highlighting file (e.g "TI Basic")
    */
    QStringList m_indentLanguages;
    /**
    * If this script is an indenter, this value controls the priority it will take
    * when an indenter for one of the supported languages is requested and multiple
    * indenters are found
    */
    int m_priority;

    /**
     * basename of script
     */
    QString m_baseName;
};


/**
 * A specialized class for scripts that are of type
 * KateScriptInformation::IndentationScript
 */
class KateIndentScript : public KateScript
{
  public:
    KateIndentScript(const QString &url, const KateIndentScriptHeader &header);

    const QString &triggerCharacters();

    const KateIndentScriptHeader& indentHeader() const;

    /**
     * Returns a pair where the first value is the indent amount, and the second
     * value is the alignment.
     */
    QPair<int, int> indent(KateView* view, const KTextEditor::Cursor& position,
                           QChar typedCharacter, int indentWidth);

  private:
    QString m_triggerCharacters;
    bool m_triggerCharactersSet;
    KateIndentScriptHeader m_indentHeader;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
