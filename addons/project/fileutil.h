/* This file is part of the KDE project
 *
 *  SPDX-FileCopyrightText: 2014 Gregor Mi <codestruct@posteo.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QString>

class FileUtil
{
public:
    /**
     * @Returns the common path of two paths. E.g.
     * path1=/usr/local/bin
     * path2=/usr/bin
     * result=/usr
     *
     * TODO: Extend QUrl with this method and submit patch to QT
     */
    static QString commonParent(const QString &path1, const QString &path2);
};

// kate: space-indent on; indent-width 4; replace-tabs on;
