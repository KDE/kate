/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2015 Dominik Haumann <dhaumann@kde.org>
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

#include "rtagslocation.h"

namespace rtags {

Location::Location()
    : m_url()
    , m_cursor(KTextEditor::Cursor::invalid())
{
}

Location::Location(const QUrl & url, const KTextEditor::Cursor & cursor)
    : m_url(url)
    , m_cursor(cursor)
{
}

Location::~Location()
{
}

bool Location::isValid() const
{
    return m_url.isValid()
        && m_url.isLocalFile()
        && m_cursor.isValid();
}

QUrl Location::url() const
{
    return m_url;
}

KTextEditor::Cursor Location::cursor() const
{
    return m_cursor;
}

Location Location::fromString(const QString & location)
{
    // location must be of the form: file:line:column
    const QStringList list = location.split(QLatin1Char(':'), QString::SkipEmptyParts);
    if (list.size() < 3) {
        return Location();
    }

    const QString file = list[0];
    const int line = list[1].toInt() - 1;
    const int column = list[2].toInt() - 1;

    return Location(QUrl::fromLocalFile(file), KTextEditor::Cursor(line, column));
}

}
