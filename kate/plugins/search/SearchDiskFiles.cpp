/*   Kate search plugin
 * 
 * Copyright (C) 2011-2013 by Kåre Särs <kare.sars@iki.fi>
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

#include "SearchDiskFiles.h"
#include "SearchDiskFiles.moc"
#include <kdebug.h>

#include <QDir>
#include <QTextStream>

SearchDiskFiles::SearchDiskFiles(QObject *parent) : QThread(parent), m_cancelSearch(true) {}

SearchDiskFiles::~SearchDiskFiles()
{
    m_cancelSearch = true;
    wait();
}

void SearchDiskFiles::startSearch(const QStringList &files,
                               const QRegExp &regexp)
{
    if (files.size() == 0) {
        emit searchDone();
        return;
    }
    m_cancelSearch = false;
    m_files = files;
    m_regExp = regexp;

    start();
}

void SearchDiskFiles::run()
{
    foreach (QString fileName, m_files) {
        if (m_cancelSearch) {
            break;
        }

        QFile file (fileName);

        if (!file.open(QFile::ReadOnly))
            continue;

        QTextStream stream (&file);
        QString line;
        int i = 0;
        int column;
        while (!(line=stream.readLine()).isNull()) {
            if (m_cancelSearch) break;
            column = m_regExp.indexIn(line);
            while (column != -1) {
                if (m_regExp.cap().isEmpty()) break;
                // limit line length
                if (line.length() > 512) line = line.left(512);
                emit matchFound(fileName, i, column, line, m_regExp.matchedLength());
                column = m_regExp.indexIn(line, column + m_regExp.cap().size());
            }
            i++;
        }
    }
    emit searchDone();
    m_cancelSearch = true;
}

void SearchDiskFiles::cancelSearch()
{
    m_cancelSearch = true;
}

bool SearchDiskFiles::searching()
{
    return !m_cancelSearch;
}
