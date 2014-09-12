/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprojectworker.h"
#include "kateproject.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QTime>

KateProjectWorker::KateProjectWorker(QObject *project)
    : QObject()
    , m_project(project)
{
}

KateProjectWorker::~KateProjectWorker()
{
}

void KateProjectWorker::loadProject(QString baseDir, QVariantMap projectMap)
{
    /**
     * setup project base directory
     * this should be FIX after initial setting
     */
    Q_ASSERT(m_baseDir.isEmpty() || (m_baseDir == baseDir));
    m_baseDir = baseDir;

    /**
     * Create dummy top level parent item and empty map inside shared pointers
     * then load the project recursively
     */
    KateProjectSharedQStandardItem topLevel(new QStandardItem());
    KateProjectSharedQMapStringItem file2Item(new QMap<QString, KateProjectItem *> ());
    loadProject(topLevel.data(), projectMap, file2Item.data());

    /**
     * create some local backup of some data we need for further processing!
     */
    QStringList files = file2Item->keys();

    /**
     * feed back our results
     */
    QMetaObject::invokeMethod(m_project, "loadProjectDone", Qt::QueuedConnection, Q_ARG(KateProjectSharedQStandardItem, topLevel), Q_ARG(KateProjectSharedQMapStringItem, file2Item));

    /**
     * load index
     */
    loadIndex(files);
}

void KateProjectWorker::loadProject(QStandardItem *parent, const QVariantMap &project, QMap<QString, KateProjectItem *> *file2Item)
{
    /**
     * recurse to sub-projects FIRST
     */
    QVariantList subGroups = project[QStringLiteral("projects")].toList();
    foreach(const QVariant & subGroupVariant, subGroups) {
        /**
         * convert to map and get name, else skip
         */
        QVariantMap subProject = subGroupVariant.toMap();
        if (subProject[QStringLiteral("name")].toString().isEmpty()) {
            continue;
        }

        /**
         * recurse
         */
        QStandardItem *subProjectItem = new KateProjectItem(KateProjectItem::Project, subProject[QStringLiteral("name")].toString());
        loadProject(subProjectItem, subProject, file2Item);
        parent->appendRow(subProjectItem);
    }

    /**
     * load all specified files
     */
    QVariantList files = project[QStringLiteral("files")].toList();
    foreach(const QVariant & fileVariant, files)
    loadFilesEntry(parent, fileVariant.toMap(), file2Item);
}

/**
 * small helper to construct directory parent items
 * @param dir2Item map for path => item
 * @param path current path we need item for
 * @return correct parent item for given path, will reuse existing ones
 */
static QStandardItem *directoryParent(QMap<QString, QStandardItem *> &dir2Item, QString path)
{
    /**
     * throw away simple /
     */
    if (path == QStringLiteral("/")) {
        path = QString();
    }

    /**
     * quick check: dir already seen?
     */
    if (dir2Item.contains(path)) {
        return dir2Item[path];
    }

    /**
     * else: construct recursively
     */
    int slashIndex = path.lastIndexOf(QLatin1Char('/'));

    /**
     * no slash?
     * simple, no recursion, append new item toplevel
     */
    if (slashIndex < 0) {
        dir2Item[path] = new KateProjectItem(KateProjectItem::Directory, path);
        dir2Item[QString()]->appendRow(dir2Item[path]);
        return dir2Item[path];
    }

    /**
     * else, split and recurse
     */
    QString leftPart = path.left(slashIndex);
    QString rightPart = path.right(path.size() - (slashIndex + 1));

    /**
     * special handling if / with nothing on one side are found
     */
    if (leftPart.isEmpty() || rightPart.isEmpty()) {
        return directoryParent(dir2Item, leftPart.isEmpty() ? rightPart : leftPart);
    }

    /**
     * else: recurse on left side
     */
    dir2Item[path] = new KateProjectItem(KateProjectItem::Directory, rightPart);
    directoryParent(dir2Item, leftPart)->appendRow(dir2Item[path]);
    return dir2Item[path];
}

void KateProjectWorker::loadFilesEntry(QStandardItem *parent, const QVariantMap &filesEntry, QMap<QString, KateProjectItem *> *file2Item)
{
    QDir dir(m_baseDir);
    if (!dir.cd(filesEntry[QStringLiteral("directory")].toString())) {
        return;
    }

    QStringList files = findFiles(dir, filesEntry);

    if (files.isEmpty()) {
        return;
    }

    files.sort();

    /**
     * construct paths first in tree and items in a map
     */
    QMap<QString, QStandardItem *> dir2Item;
    dir2Item[QString()] = parent;
    QList<QPair<QStandardItem *, QStandardItem *> > item2ParentPath;
    Q_FOREACH(auto filePath, files) {
        /**
          * skip dupes
          */
        if (file2Item->contains(filePath)) {
            continue;
        }

        /**
         * get file info and skip NON-files
         */
        QFileInfo fileInfo(filePath);
        if (!fileInfo.isFile()) {
            continue;
        }

        /**
         * construct the item with right directory prefix
         * already hang in directories in tree
         */
        KateProjectItem *fileItem = new KateProjectItem(KateProjectItem::File, fileInfo.fileName());
        fileItem->setData(filePath, Qt::ToolTipRole);
        item2ParentPath.append(QPair<QStandardItem *, QStandardItem *>(fileItem, directoryParent(dir2Item, dir.relativeFilePath(fileInfo.absolutePath()))));
        fileItem->setData(filePath, Qt::UserRole);
        (*file2Item)[filePath] = fileItem;
    }

    /**
     * plug in the file items to the tree
     */
    QList<QPair<QStandardItem *, QStandardItem *> >::const_iterator i = item2ParentPath.constBegin();
    while (i != item2ParentPath.constEnd()) {
        i->second->appendRow(i->first);
        ++i;
    }
}

QStringList KateProjectWorker::findFiles(const QDir &dir, const QVariantMap& filesEntry)
{
    const bool recursive = !filesEntry.contains(QStringLiteral("recursive")) || filesEntry[QStringLiteral("recursive")].toBool();

    if (filesEntry[QStringLiteral("git")].toBool()) {
        return filesFromGit(dir, recursive);
    } else if (filesEntry[QStringLiteral("hg")].toBool()) {
        return filesFromMercurial(dir, recursive);
    } else if (filesEntry[QStringLiteral("svn")].toBool()) {
        return filesFromSubversion(dir, recursive);
    } else {
        QStringList files = filesEntry[QStringLiteral("list")].toStringList();

        if (files.empty()) {
            QStringList filters = filesEntry[QStringLiteral("filters")].toStringList();
            files = filesFromDirectory(dir, recursive, filters);
        }

        return files;
    }
}

QStringList KateProjectWorker::filesFromGit(const QDir& dir, bool recursive)
{
    QStringList files;

    QProcess git;
    git.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    args << QStringLiteral("ls-files") << QStringLiteral(".");
    git.start(QStringLiteral("git"), args);
    if (!git.waitForStarted() || !git.waitForFinished()) {
        return files;
    }

    QStringList relFiles = QString::fromLocal8Bit(git.readAllStandardOutput()).split(QRegExp(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);

    Q_FOREACH(auto relFile, relFiles) {
        if (!recursive && (relFile.indexOf(QStringLiteral("/")) != -1)) {
            continue;
        }

        files.append(dir.absolutePath() + QLatin1Char('/') + relFile);
    }

    return files;
}

QStringList KateProjectWorker::filesFromMercurial(const QDir& dir, bool recursive)
{
    QStringList files;

    QProcess hg;
    hg.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    args << QStringLiteral("manifest") << QStringLiteral(".");
    hg.start(QStringLiteral("hg"), args);
    if (!hg.waitForStarted() || !hg.waitForFinished()) {
        return files;
    }

    QStringList relFiles = QString::fromLocal8Bit(hg.readAllStandardOutput()).split(QRegExp(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);

    Q_FOREACH(auto relFile, relFiles) {
        if (!recursive && (relFile.indexOf(QStringLiteral("/")) != -1)) {
            continue;
        }

        files.append(dir.absolutePath() + QLatin1Char('/') + relFile);
    }

    return files;
}

QStringList KateProjectWorker::filesFromSubversion(const QDir& dir, bool recursive)
{
    QStringList files;

    QProcess svn;
    svn.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    args << QStringLiteral("status") << QStringLiteral("--verbose") << QStringLiteral(".");
    if (recursive) {
        args << QStringLiteral("--depth=infinity");
    } else {
        args << QStringLiteral("--depth=files");
    }
    svn.start(QStringLiteral("svn"), args);
    if (!svn.waitForStarted() || !svn.waitForFinished()) {
        return files;
    }

    /**
     * get output and split up into lines
     */
    QStringList lines = QString::fromLocal8Bit(svn.readAllStandardOutput()).split(QRegExp(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);

    /**
     * remove start of line that is no filename, sort out unknown and ignore
     */
    bool first = true;
    int prefixLength = -1;

    Q_FOREACH(auto line, lines) {
        /**
         * get length of stuff to cut
         */
        if (first) {
            /**
             * try to find ., else fail
             */
            prefixLength = line.lastIndexOf(QStringLiteral("."));
            if (prefixLength < 0) {
                break;
            }

            /**
             * skip first
             */
            first = false;
            continue;
        }

        /**
         * get file, if not unknown or ignored
         * prepend directory path
         */
        if ((line.size() > prefixLength) && line[0] != QLatin1Char('?') && line[0] != QLatin1Char('I')) {
            files.append(dir.absolutePath() + QLatin1Char('/') + line.right(line.size() - prefixLength));
        }
    }

    return files;
}

QStringList KateProjectWorker::filesFromDirectory(const QDir& _dir, bool recursive, const QStringList &filters)
{
    QStringList files;

    QDir dir(_dir);
    dir.setFilter(QDir::Files);

    if (!filters.isEmpty()) {
        dir.setNameFilters(filters);
    }

    /**
     * construct flags for iterator
     */
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) {
        flags = flags | QDirIterator::Subdirectories;
    }

    /**
     * create iterator and collect all files
     */
    QDirIterator dirIterator(dir, flags);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        files.append(dirIterator.filePath());
    }

    return files;
}

void KateProjectWorker::loadIndex(const QStringList &files)
{
    /**
     * create new index, this will do the loading in the constructor
     * wrap it into shared pointer for transfer to main thread
     */
    KateProjectSharedProjectIndex index(new KateProjectIndex(files));

    /**
     * send new index object back to project
     */
    QMetaObject::invokeMethod(m_project, "loadIndexDone", Qt::QueuedConnection, Q_ARG(KateProjectSharedProjectIndex, index));
}
