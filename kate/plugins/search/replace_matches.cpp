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

#include "replace_matches.h"
#include "replace_matches.moc"

ReplaceMatches::ReplaceMatches(QObject *parent) : QObject(parent), m_nextIndex(-1)
{
    connect(this, SIGNAL(replaceNextMatch()), this, SLOT(doReplaceNextMatch()), Qt::QueuedConnection);
}

void ReplaceMatches::replaceChecked(QTreeWidget *tree, const QRegExp &regexp, const QString &replace)
{
    if (m_nextIndex != -1) return;

    m_tree = tree;
    m_nextIndex = 0;
    m_regExp = regexp;
    m_replace = replace;
    m_cancelReplace = false;
    emit replaceNextMatch();
}

void ReplaceMatches::cancelReplace()
{
    m_cancelReplace = true;
}

void ReplaceMatches::doReplaceNextMatch()
{
    if (m_cancelReplace) {
        m_nextIndex = -1;
        emit replaceDone();
        return;
    }
    kDebug();
//     int column;

    // NOTE The document managers signal documentWillBeDeleted() must be connected to
    // cancelReplace(). A closed file could lead to a crash if it is not handled.

//     for (int line =0; line < m_docList[m_nextIndex]->lines(); line++) {
//         column = m_regExp.indexIn(m_docList[m_nextIndex]->line(line));
//         while (column != -1) {
//             if (m_docList[m_nextIndex]->url().isLocalFile() ) {
//                 emit matchFound(m_docList[m_nextIndex]->url().path(), line, column,
//                                 m_docList[m_nextIndex]->line(line), m_regExp.matchedLength());
//             }
//             else  {
//                 emit matchFound(m_docList[m_nextIndex]->url().prettyUrl(), line, column,
//                                 m_docList[m_nextIndex]->line(line), m_regExp.matchedLength());
//             }
//             column = m_regExp.indexIn(m_docList[m_nextIndex]->line(line), column + 1);
//         }
//     }

//     m_nextIndex++;
//     if (m_nextIndex == m_docList.size()) {
//         m_nextIndex = -1;
        emit replaceDone();
//     }
//     else {
//         emit replaceNextMatch();
//     }
}
 
