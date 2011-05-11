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

#include "search_folder.h"
#include "search_folder.moc"

#include <QDir>

SearchFolder::SearchFolder(QObject *parent) : QThread(parent)
{
}

void SearchFolder::startSearch(const QString &folder,
                               bool recursive,
                               bool hidden,
                               bool symlinks,
                               const QString &types,
                               const QRegExp &regexp)
{
    m_cancelSearch = false;
    m_recursive    = recursive;
    m_hidden       = hidden;
    m_symlinks     = symlinks;
    m_types        = types.split(',');
    m_folder       = folder;
    m_regExp       = regexp;

    start();
}

void SearchFolder::run()
{
    handleNextItem(QFileInfo(m_folder));
    emit searchDone();
}

void SearchFolder::cancelSearch()
{
    m_cancelSearch = true;
}

void SearchFolder::handleNextItem(const QFileInfo &item)
{
    if (m_cancelSearch) {
        return;
    }

    if (item.isFile()) {
        return searchFile(item);
    }
    else {
        QDir currentDir(item.canonicalFilePath());
        
        if (!currentDir.isReadable()) {
            kDebug() << currentDir.absolutePath() << "Not readable";
            return;
        }

        QDir::Filters    filter  = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
        if (m_hidden)    filter |= QDir::Hidden;
        if (m_recursive) filter |= QDir::AllDirs;
        if (!m_symlinks) filter |= QDir::NoSymLinks;

        QFileInfoList currentItems = currentDir.entryInfoList (m_types, filter);
        
        for (int i = 0; i<currentItems.size(); ++i) {
            if (m_cancelSearch) return;
            handleNextItem(currentItems[i]);
        }
    }
}

void SearchFolder::searchFile(const QFileInfo &item)
{
    if (m_cancelSearch) return;
    
    QFile file (item.absoluteFilePath());

    if (!file.open(QFile::ReadOnly)) {
        return;
    }

    QTextStream stream (&file);
    QString line;
    int i = 0;
    int column;
    while (!(line=stream.readLine()).isNull()) {
        if (m_cancelSearch) return;
        column = m_regExp.indexIn(line);
        if (column != -1) {
            emit matchFound(item.canonicalFilePath(), i, line);
        }
        i++;
    }
}
