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

#include "../katewildcardmatcher.h"
#include <QtGlobal>
#include <QtDebug>



bool testCase(const char * candidate, const char * wildcard) {
    qDebug("\"%s\" / \"%s\"", candidate, wildcard);
	return KateWildcardMatcher::exactMatch(QString(candidate), QString(wildcard));
}



int main() {
    Q_ASSERT(testCase("abc.txt", "*.txt"));
    Q_ASSERT(!testCase("abc.txt", "*.cpp"));

    Q_ASSERT(testCase("Makefile.am", "*Makefile*"));

    Q_ASSERT(testCase("control", "control"));

    Q_ASSERT(testCase("abcd", "a??d"));

    Q_ASSERT(testCase("a", "?"));
    Q_ASSERT(testCase("a", "*?*"));
    Q_ASSERT(testCase("a", "*"));
    Q_ASSERT(testCase("a", "**"));
    Q_ASSERT(testCase("a", "***"));

    Q_ASSERT(testCase("", "*"));
    Q_ASSERT(testCase("", "**"));
    Q_ASSERT(!testCase("", "?"));

    Q_ASSERT(testCase("ab", "a*"));
    Q_ASSERT(testCase("ab", "*b"));
    Q_ASSERT(testCase("ab", "a?"));
    Q_ASSERT(testCase("ab", "?b"));

    Q_ASSERT(testCase("aXXbXXbYYaYc", "a*b*c"));

    
    qDebug() << endl << "DONE";
	return 0;
}

