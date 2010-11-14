/* This file is part of the KDE libraries
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_WILDCARD_MATCHER_H
#define KATE_WILDCARD_MATCHER_H



class QString;



namespace KateWildcardMatcher {

    /**
     * Matches a string against a given wildcard.
     * The wildcard supports '*' (".*" in regex) and '?' ("." in regex), not more.
     *
     * @param candidate       Text to match
     * @param wildcard        Wildcard to use
     * @param caseSensitive   Case-sensitivity flag
     * @return                True for an exact match, false otherwise
     */
    bool exactMatch(const QString & candidate, const QString & wildcard,
            bool caseSensitive = true);

}



#endif // KATE_WILDCARD_MATCHER_H

