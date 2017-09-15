/*   Kate search plugin
 * 
 * Copyright (C) 2013 by Kåre Särs <kare.sars@iki.fi>
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

#ifndef FolderFilesList_h
#define FolderFilesList_h

#include <QThread>
#include <QRegExp>
#include <QFileInfo>
#include <QVector>
#include <QStringList>
#include <QTime>

class FolderFilesList: public QThread
{
    Q_OBJECT

public:
    FolderFilesList(QObject *parent = nullptr);
    ~FolderFilesList() override;

    void run() override;

    void generateList(const QString &folder,
                      bool recursive,
                      bool hidden,
                      bool symlinks,
                      bool binary,
                      const QString &types,
                      const QString &excludes);

    QStringList fileList();

public Q_SLOTS:
    void cancelSearch();

Q_SIGNALS:
    void searching(const QString &path);

private:
    void checkNextItem(const QFileInfo &item);

private:
    QString          m_folder;
    QStringList      m_files;
    bool             m_cancelSearch;

    bool             m_recursive;
    bool             m_hidden;
    bool             m_symlinks;
    bool             m_binary;
    QStringList      m_types;
    QVector<QRegExp> m_excludeList;
    QTime            m_time;
};


#endif
