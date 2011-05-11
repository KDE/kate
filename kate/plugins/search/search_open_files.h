/*   Kate search plugin
 * 
 * Copyright (C) 2011 by Kåre Särs <kare.sars@iki.fi>
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

#ifndef _SEARCH_OPEN_FILES_H_
#define _SEARCH_OPEN_FILES_H_

#include <QObject>
#include <QRegExp>
#include <ktexteditor/document.h>

class SearchOpenFiles: public QObject
{
    Q_OBJECT

public:
    SearchOpenFiles(QObject *parent = 0);

    void startSearch(const QList<KTextEditor::Document*> &list,const QRegExp &regexp);

public Q_SLOTS:
    void cancelSearch();

private Q_SLOTS:
    void doSearchNextFile();

Q_SIGNALS:
    void searchNextFile();
    void matchFound(const QString &url, int line, const QString &lineContent);
    void searchDone();
    
private:
    QList<KTextEditor::Document*> m_docList;
    int                           m_nextIndex;
    QRegExp                       m_regExp;
    bool                          m_cancelSearch;
    
};


#endif
