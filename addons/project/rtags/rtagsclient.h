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

#ifndef KATE_PROJECT_RTAGS_CLIENT_H
#define KATE_PROJECT_RTAGS_CLIENT_H

#include <QObject>
#include <QStringList>

#include <KTextEditor/Document>

namespace rtags {

/**
 * Class to communicate with the rtags server rdm.
 *
 * This class is more or less a wrapper around the 'rc' tool that is part
 * of the rtags daemon 'rdm'.
 */
class Client : public QObject
{
public:
    /**
     * Construct an rtags client.
     */
    Client();

    /**
     * deconstruct project
     */
    ~Client();

//
// diagnostics
//
public:
    /**
     * Check if the rtags tools 'rc' and 'rdm' are installed.
     */
    bool rtagsAvailable() const;

    /**
     * Check if the rtags daemon is running.
     */
    bool rtagsDaemonRunning() const;

    /**
     * Check whether the rtags daemon 'rdm' is currently indexing.
     */
    bool isIndexing() const;

    /**
     * Get a list of indexed projects.
     */
    QStringList indexedProjects() const;

    void followLocation(KTextEditor::Document * document, const KTextEditor::Cursor & cursor);

//
//
//
public Q_SLOTS:
    /**
     * Tell rtags the current file, giving it a better chance of
     * quickly following the right project.
     */
    void setCurrentFile(KTextEditor::Document * document);
};

}

#endif
