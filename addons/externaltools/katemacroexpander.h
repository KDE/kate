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

namespace KTextEditor {
    class View;
}

/**
 * Helper class for macro expansion.
 */
class MacroExpander : public KWordMacroExpander
{
public:
    MacroExpander(KTextEditor::View * view);

protected:
    bool expandMacro(const QString& str, QStringList& ret) override;

private:
    KTextEditor::View * m_view;
};

#endif // KTEXTEDITOR_MACRO_EXPANDER_H

// kate: space-indent on; indent-width 4; replace-tabs on;
