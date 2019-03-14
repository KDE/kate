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
#ifndef KTEXTEDITOR_MACRO_EXPANDER_H
#define KTEXTEDITOR_MACRO_EXPANDER_H

#include <KMacroExpander>

#include <QString>
#include <QVector>

namespace KTextEditor
{
class View;
}

using StringFunction = QString (*)(const QStringView& prefix, KTextEditor::View* view);
class Variable
{
public:
    Variable() = default;
    Variable(const QString& variable, const QString& description, StringFunction func)
        : m_variable(variable)
        , m_description(description)
        , m_function(func)
    {}

    QString variable() const
    {
        return m_variable;
    }

    QString description() const
    {
        return m_description;
    }

    QString evaluate(const QStringView& prefix, KTextEditor::View * view) const
    {
        return m_function(prefix, view);
    }

private:
    QString m_variable;
    QString m_description;
    StringFunction m_function;
};

/**
 * Helper class for macro expansion.
 */
class MacroExpander : public KWordMacroExpander
{
public:
    MacroExpander(KTextEditor::View* view);

    void registerExactMatch(const Variable & variable);
    void registerPrefix(const Variable & variable);

protected:
    bool expandMacro(const QString& str, QStringList& ret) override;

private:
    QVector<Variable> m_exactMatches;
    QVector<Variable> m_prefixMatches;
    KTextEditor::View* m_view;
    const QString m_url;
};

#endif // KTEXTEDITOR_MACRO_EXPANDER_H

// kate: space-indent on; indent-width 4; replace-tabs on;
