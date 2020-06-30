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

#include "KateSearchCommand.h"
#include "plugin_search.h"


KateSearchCommand::KateSearchCommand(QObject *parent)
    : KTextEditor::Command(QStringList() << QStringLiteral("grep") << QStringLiteral("newGrep") << QStringLiteral("search") << QStringLiteral("newSearch") << QStringLiteral("pgrep") << QStringLiteral("newPGrep"), parent)
{
}

bool KateSearchCommand::exec(KTextEditor::View * /*view*/, const QString &cmd, QString & /*msg*/, const KTextEditor::Range &)
{
    // create a list of args
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList args(cmd.split(QLatin1Char(' '), QString::KeepEmptyParts));
#else
    QStringList args(cmd.split(QLatin1Char(' '), Qt::KeepEmptyParts));
#endif
    QString command = args.takeFirst();
    QString searchText = args.join(QLatin1Char(' '));

    if (command == QLatin1String("grep") || command == QLatin1String("newGrep")) {
        emit setSearchPlace(KatePluginSearchView::Folder);
        emit setCurrentFolder();
        if (command == QLatin1String("newGrep"))
            emit newTab();
    }

    else if (command == QLatin1String("search") || command == QLatin1String("newSearch")) {
        emit setSearchPlace(KatePluginSearchView::OpenFiles);
        if (command == QLatin1String("newSearch"))
            emit newTab();
    }

    else if (command == QLatin1String("pgrep") || command == QLatin1String("newPGrep")) {
        emit setSearchPlace(KatePluginSearchView::Project);
        if (command == QLatin1String("newPGrep"))
            emit newTab();
    }

    emit setSearchString(searchText);
    emit startSearch();

    return true;
}

bool KateSearchCommand::help(KTextEditor::View * /*view*/, const QString &cmd, QString &msg)
{
    if (cmd.startsWith(QLatin1String("grep"))) {
        msg = i18n("Usage: grep [pattern to search for in folder]");
    } else if (cmd.startsWith(QLatin1String("newGrep"))) {
        msg = i18n("Usage: newGrep [pattern to search for in folder]");
    }

    else if (cmd.startsWith(QLatin1String("search"))) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    } else if (cmd.startsWith(QLatin1String("newSearch"))) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    }

    else if (cmd.startsWith(QLatin1String("pgrep"))) {
        msg = i18n("Usage: pgrep [pattern to search for in current project]");
    } else if (cmd.startsWith(QLatin1String("newPGrep"))) {
        msg = i18n("Usage: newPGrep [pattern to search for in current project]");
    }

    return true;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
