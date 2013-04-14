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

        if (m_regExp.pattern().contains("\\n")) {
            searchMultiLineRegExp(fileName);
        }
        else {
            searchSingleLineRegExp(fileName);
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

void SearchDiskFiles::searchSingleLineRegExp(const QString &fileName)
{
    QFile file (fileName);

    if (!file.open(QFile::ReadOnly)) {
        return;
    }

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

void SearchDiskFiles::searchMultiLineRegExp(const QString &fileName)
{
    QFile file (fileName);
    int column = 0;
    int line = 0;
    static QString fullDoc;
    static QVector<int> lineStart;


    if (!file.open(QFile::ReadOnly)) {
        return;
    }

    QTextStream stream (&file);
    fullDoc = stream.readAll();
    fullDoc.remove('\r');

    lineStart.clear();
    lineStart << 0;
    for (int i=0; i<fullDoc.size()-1; i++) {
        if (fullDoc[i] == '\n') {
            lineStart << i+1;
        }
    }

    column = m_regExp.indexIn(fullDoc, column);
    while (column != -1) {
        if (m_cancelSearch) break;
        if (m_regExp.cap().isEmpty()) break;
        // search for the line number of the match
        int i;
        line = -1;
        for (i=1; i<lineStart.size(); i++) {
            if (lineStart[i] > column) {
                line = i-1;
                break;
            }
        }
        if (line == -1) {
            break;
        }
        emit matchFound(fileName,
                        line,
                        (column - lineStart[line]),
                        fullDoc.mid(lineStart[line], column - lineStart[line])+m_regExp.cap(),
                        m_regExp.matchedLength());
        column = m_regExp.indexIn(fullDoc, column + m_regExp.matchedLength());
    }
}

