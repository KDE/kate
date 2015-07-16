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

#ifndef KATE_RTAGS_LOCATION_H
#define KATE_RTAGS_LOCATION_H

#include <QUrl>
#include <KTextEditor/Cursor>

namespace rtags {

/**
 * Class that represents an rtags location.
 *
 * The Location class represents a location in terms of a file and an associated
 * cursor position in this file. The file must be a local file on disk, otherwise
 * isValid() returns false.
 */
class Location
{
public:
    /**
     * Creates an invalid location.
     */
    Location();

    /**
     * Creates a Location object pointing to @p url at text position @p cursor.
     */
    Location(const QUrl & url, const KTextEditor::Cursor & cursor);

    /**
     * Destructor.
     */
    ~Location();

    /**
     * Returns, whether this location object points to a valid text position.
     * A text position is only valid, if the url() is valid and points to a
     * local file.
     */
    bool isValid() const;

    /**
     * Returns the url pointing to the file of the location.
     */
    QUrl url() const;

    /**
     * Returns the cursor position of the location.
     */
    KTextEditor::Cursor cursor() const;

    /**
     * Creates a Location object from the QString that is of the form
     * 'file:line:column'. Note, that line and column counting starts at 1.
     */
    static Location fromString(const QString & location);

private:
    QUrl m_url;
    KTextEditor::Cursor m_cursor;
};

}

#endif
