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

#include "search_open_files.h"
#include "search_open_files.moc"

SearchOpenFiles::SearchOpenFiles(QObject *parent) : QObject(parent), m_nextIndex(-1)
{
    connect(this, SIGNAL(searchNextFile()), this, SLOT(doSearchNextFile()), Qt::QueuedConnection);
}

void SearchOpenFiles::startSearch(const QList<KTextEditor::Document*> &list, const QRegExp &regexp)
{
    if (m_nextIndex != -1) return;

    m_docList = list;
    m_nextIndex = 0;
    m_regExp = regexp;
    m_cancelSearch = false;
    emit searchNextFile();
}

void SearchOpenFiles::cancelSearch()
{
    m_cancelSearch = true;
}

void SearchOpenFiles::doSearchNextFile()
{
    if (m_cancelSearch) {
        m_nextIndex = -1;
        emit searchDone();
        return;
    }

    int column;

    // NOTE The document managers signal documentWillBeDeleted() must be connected to
    // cancelSearch(). A closed file could lead to a crash if it is not handled.

    for (int line =0; line < m_docList[m_nextIndex]->lines(); line++) {
        column = m_regExp.indexIn(m_docList[m_nextIndex]->line(line));
        while (column != -1) {
            if (m_regExp.cap().isEmpty()) break;

            if (m_docList[m_nextIndex]->url().isLocalFile() ) {
                emit matchFound(m_docList[m_nextIndex]->url().path(), line, column,
                                m_docList[m_nextIndex]->line(line), m_regExp.matchedLength());
            }
            else  {
                emit matchFound(m_docList[m_nextIndex]->url().prettyUrl(), line, column,
                                m_docList[m_nextIndex]->line(line), m_regExp.matchedLength());
            }
            column = m_regExp.indexIn(m_docList[m_nextIndex]->line(line), column + m_regExp.cap().size());
        }
    }

    m_nextIndex++;
    if (m_nextIndex == m_docList.size()) {
        m_nextIndex = -1;
        emit searchDone();
    }
    else {
        emit searchNextFile();
    }
}
 
