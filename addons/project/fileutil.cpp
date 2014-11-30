/* This file is part of the KDE project
 *
 *  Copyright 2014 Gregor Mi <codestruct@posteo.org>
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

#include "fileutil.h"

// code taken from http://stackoverflow.com/questions/15713529/get-common-parent-of-2-qdir
// note that there is unit test
const QString FileUtil::commonParent(const QString& path1, const QString& path2)
{
    QString ret = path2;

    while (!path1.startsWith(ret))
        ret.chop(1);

    if (ret.isEmpty())
        return ret;

    while (!ret.endsWith(QLatin1String("/")))
        ret.chop(1);

    return ret;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
