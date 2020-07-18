/*   Kate search plugin
 *
 * Copyright (C) 2020 by Kåre Särs <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file called COPYING; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef KateSearchCommand_h
#define KateSearchCommand_h

#include <KTextEditor/Command>
#include <QString>

namespace KTextEditor
{
class View;
class Range;
}

class KateSearchCommand : public KTextEditor::Command
{
    Q_OBJECT
public:
    KateSearchCommand(QObject *parent);

Q_SIGNALS:
    void setSearchPlace(int place);
    void setCurrentFolder();
    void setSearchString(const QString &pattern);
    void startSearch();
    void newTab();

    //
    // KTextEditor::Command
    //
public:
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
