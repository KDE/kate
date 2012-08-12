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

#ifndef _SEARCH_PROJECT_H_
#define _SEARCH_PROJECT_H_

#include <QThread>
#include <QRegExp>
#include <QFileInfo>
#include <ktexteditor/document.h>

class SearchProject: public QThread
{
    Q_OBJECT

public:
    SearchProject(QObject *parent = 0);

    void startSearch(const QStringList &files,
                     const QRegExp &regexp);
    void run();

public Q_SLOTS:
    void cancelSearch();

Q_SIGNALS:
    void matchFound(const QString &url, int line, int column,
                    const QString &lineContent, int matchLen);
    void searchDone();
private:
    QRegExp          m_regExp;
    bool             m_cancelSearch;
    QStringList      m_files;

};

#endif
