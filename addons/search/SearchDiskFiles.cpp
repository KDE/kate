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

#include <QDir>
#include <QTextStream>

SearchDiskFiles::SearchDiskFiles(QObject *parent) : QThread(parent)
,m_cancelSearch(true)
,m_matchCount(0)
{}

SearchDiskFiles::~SearchDiskFiles()
{
    m_cancelSearch = true;
    wait();
}

void SearchDiskFiles::startSearch(const QStringList &files,
                                  const QRegularExpression &regexp)
{
    if (files.size() == 0) {
        emit searchDone();
        return;
    }
    m_cancelSearch = false;
    m_files = files;
    m_regExp = regexp;
    m_matchCount = 0;
    m_statusTime.restart();
    start();
}

void SearchDiskFiles::run()
{
    foreach (QString fileName, m_files) {
        if (m_cancelSearch) {
            break;
        }

        if (m_statusTime.elapsed() > 100) {
            m_statusTime.restart();
            emit searching(fileName);
        }

        if (m_regExp.pattern().contains(QStringLiteral("\\n"))) {
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
    QRegularExpressionMatch match;
    while (!(line=stream.readLine()).isNull()) {
        if (m_cancelSearch) break;
        match = m_regExp.match(line);
        column = match.capturedStart();
        while (column != -1 && !match.captured().isEmpty()) {
            // limit line length
            if (line.length() > 1024) line = line.left(1024);
            emit matchFound(fileName, fileName, i, column, line, match.capturedLength());
            match = m_regExp.match(line, column + match.capturedLength());
            column = match.capturedStart();
            m_matchCount++;
            // NOTE: This sleep is here so that the main thread will get a chance to
            // handle any stop button clicks if there are a lot of matches
            if (m_matchCount%50) msleep(1);
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
    QRegularExpression tmpRegExp = m_regExp;

    if (!file.open(QFile::ReadOnly)) {
        return;
    }

    QTextStream stream (&file);
    fullDoc = stream.readAll();
    fullDoc.remove(QLatin1Char('\r'));

    lineStart.clear();
    lineStart << 0;
    for (int i=0; i<fullDoc.size()-1; i++) {
        if (fullDoc[i] == QLatin1Char('\n')) {
            lineStart << i+1;
        }
    }
    if (tmpRegExp.pattern().endsWith(QStringLiteral("$"))) {
        fullDoc += QLatin1Char('\n');
        QString newPatern = tmpRegExp.pattern();
        newPatern.replace(QStringLiteral("$"), QStringLiteral("(?=\\n)"));
        tmpRegExp.setPattern(newPatern);
    }

    QRegularExpressionMatch match;
    match = tmpRegExp.match(fullDoc);
    column = match.capturedStart();
    while (column != -1 && !match.captured().isEmpty()) {
        if (m_cancelSearch) break;
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
        emit matchFound(fileName,fileName,
                        line,
                        (column - lineStart[line]),
                        fullDoc.mid(lineStart[line], column - lineStart[line])+match.captured(),
                        match.capturedLength());
        match = tmpRegExp.match(fullDoc, column + match.capturedLength());
        column = match.capturedStart();
        m_matchCount++;
        // NOTE: This sleep is here so that the main thread will get a chance to
        // handle any stop button clicks if there are a lot of matches
        if (m_matchCount%50) msleep(1);
    }
}

