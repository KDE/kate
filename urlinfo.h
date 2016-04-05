/* This file is part of the KDE project
   Copyright (C) 2015 Milian Wolff <mail@milianw.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef URLINFO_H
#define URLINFO_H

#include <KTextEditor/Cursor>
#include <QRegularExpression>
#include <QString>

// Represents a file to be opened, consisting of its URL and the cursor to jump to.
struct UrlInfo
{
    // Parses a file path argument and determines its line number and column and full path
    UrlInfo(QString path)
        : cursor(KTextEditor::Cursor::invalid())
    {
        // convert to an url
        const QRegularExpression withProtocol(QStringLiteral("^[a-zA-Z]+://")); // TODO: remove after Qt supports this on its own
        if (withProtocol.match(path).hasMatch()) {
            url = QUrl::fromUserInput(path);
        } else {
            url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
        }

        if (url.isLocalFile() && !QFile::exists(path)) {
            // Allow opening specific lines in documents, like mydoc.cpp:10
            // also supports columns, i.e. mydoc.cpp:10:42
            // ignores trailing colons, as compile errors often use that format
            static const QRegularExpression pattern(QStringLiteral(":(\\d+)(?::(\\d+))?:?$"));
            const auto match = pattern.match(path);
            if (match.isValid()) {
                path.chop(match.capturedLength());
                int line = match.captured(1).toInt() - 1;
                // don't use an invalid column when the line is valid
                int column = qMax(0, match.captured(2).toInt() - 1);
                url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
                cursor = {line, column};
            }
        }
    }

    QUrl url;
    KTextEditor::Cursor cursor;
};

#endif // URLINFO_H
