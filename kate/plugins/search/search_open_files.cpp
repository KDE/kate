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

#include "search_open_files.h"
#include "search_open_files.moc"

#include <QTime>

SearchOpenFiles::SearchOpenFiles(QObject *parent) : QObject(parent), m_nextIndex(-1), m_cancelSearch(true)
{
    connect(this, SIGNAL(searchNextFile(int)), this, SLOT(doSearchNextFile(int)), Qt::QueuedConnection);
}

bool SearchOpenFiles::searching() { return !m_cancelSearch; }

void SearchOpenFiles::startSearch(const QList<KTextEditor::Document*> &list, const QRegExp &regexp)
{
    if (m_nextIndex != -1) return;

    m_docList = list;
    m_nextIndex = 0;
    m_regExp = regexp;
    m_cancelSearch = false;
    emit searchNextFile(0);
}

void SearchOpenFiles::cancelSearch()
{
    m_cancelSearch = true;
}

void SearchOpenFiles::doSearchNextFile(int startLine)
{
    if (m_cancelSearch) {
        m_nextIndex = -1;
        m_cancelSearch = true;
        emit searchDone();
        return;
    }

    // NOTE The document managers signal documentWillBeDeleted() must be connected to
    // cancelSearch(). A closed file could lead to a crash if it is not handled.
    int line = searchOpenFile(m_docList[m_nextIndex], m_regExp, startLine);
    if (line == 0) {
        // file searched go to next
        m_nextIndex++;
        if (m_nextIndex == m_docList.size()) {
            m_nextIndex = -1;
            m_cancelSearch = true;
            emit searchDone();
        }
        else {
            emit searchNextFile(0);
        }
    }
    else {
        emit searchNextFile(line);
    }
}

int SearchOpenFiles::searchOpenFile(KTextEditor::Document *doc, const QRegExp &regExp, int startLine)
{
    int column;
    QTime time;

    time.start();
    for (int line = startLine; line < doc->lines(); line++) {
        if (time.elapsed() > 100) {
            kDebug() << "Search time exceeded" << time.elapsed() << line;
            return line;
        }
        column = regExp.indexIn(doc->line(line));
        while (column != -1) {
            if (regExp.cap().isEmpty()) break;
            emit matchFound(doc->url().pathOrUrl(), line, column,
                            doc->line(line), regExp.matchedLength());
            column = regExp.indexIn(doc->line(line), column + regExp.cap().size());
        }
    }
    return 0;
}

