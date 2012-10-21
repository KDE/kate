/*   Kate search plugin
 *
 * Copyright (C) 2012 by Kåre Särs <kare.sars@iki.fi>
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

#ifndef _SEARCH_WHILE_TYPING_H_
#define _SEARCH_WHILE_TYPING_H_

#include <QObject>
#include <QRegExp>
#include <ktexteditor/document.h>

class SearchWhileTyping: public QObject
{
    Q_OBJECT

public:
    void startSearch(const KTextEditor::Document *doc, const QRegExp &regExp);

Q_SIGNALS:
    void matchFound(const QString &url, int line, int column, const QString &lineContent, int matchLen);
    void searchDone();
};


#endif
