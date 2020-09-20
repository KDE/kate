/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectworker.h"
#include "kateproject.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QTime>

KateProjectWorker::KateProjectWorker(const QString &baseDir, const QString &indexDir, const QVariantMap &projectMap, bool force)
    : m_baseDir(baseDir)
    , m_indexDir(indexDir)
    , m_projectMap(projectMap)
    , m_force(force)
{
    Q_ASSERT(!m_baseDir.isEmpty());
}

void KateProjectWorker::run(ThreadWeaver::JobPointer, ThreadWeaver::Thread *)
{
    /**
     * Create dummy top level parent item and empty map inside shared pointers
     * then load the project recursively
     */
    KateProjectSharedQStandardItem topLevel(new QStandardItem());
    KateProjectSharedQMapStringItem file2Item(new QMap<QString, KateProjectItem *>());
    loadProject(topLevel.data(), m_projectMap, file2Item.data());

    /**
     * create some local backup of some data we need for further processing!
     */
    QStringList files = file2Item->keys();

    emit loadDone(topLevel, file2Item);

    // trigger index loading, will internally handle enable/disabled
    loadIndex(files, m_force);
}

void KateProjectWorker::loadProject(QStandardItem *parent, const QVariantMap &project, QMap<QString, KateProjectItem *> *file2Item)
{
    /**
     * recurse to sub-projects FIRST
     */
    QVariantList subGroups = project[QStringLiteral("projects")].toList();
    for (const QVariant &subGroupVariant : subGroups) {
        /**
         * convert to map and get name, else skip
         */
        QVariantMap subProject = subGroupVariant.toMap();
        const QString keyName = QStringLiteral("name");
        if (subProject[keyName].toString().isEmpty()) {
            continue;
        }

        /**
         * recurse
         */
        QStandardItem *subProjectItem = new KateProjectItem(KateProjectItem::Project, subProject[keyName].toString());
        loadProject(subProjectItem, subProject, file2Item);
        parent->appendRow(subProjectItem);
    }

    /**
     * load all specified files
     */
    const QString keyFiles = QStringLiteral("files");
    QVariantList files = project[keyFiles].toList();
    for (const QVariant &fileVariant : files) {
        loadFilesEntry(parent, fileVariant.toMap(), file2Item);
    }
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
    if (path == QLatin1String("/")) {
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
    const QString leftPart = path.left(slashIndex);
    const QString rightPart = path.right(path.size() - (slashIndex + 1));

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

    files.sort(Qt::CaseInsensitive);

    /**
     * construct paths first in tree and items in a map
     */
    QMap<QString, QStandardItem *> dir2Item;
    dir2Item[QString()] = parent;
    QList<QPair<QStandardItem *, QStandardItem *>> item2ParentPath;
    for (const QString &filePath : files) {
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

        // get the directory's relative path to the base directory
        QString dirRelPath = dir.relativeFilePath(fileInfo.absolutePath());
        // if the relative path is ".", clean it up
        if (dirRelPath == QLatin1Char('.')) {
            dirRelPath = QString();
        }

        item2ParentPath.append(QPair<QStandardItem *, QStandardItem *>(fileItem, directoryParent(dir2Item, dirRelPath)));
        fileItem->setData(filePath, Qt::UserRole);
        (*file2Item)[filePath] = fileItem;
    }

    /**
     * plug in the file items to the tree
     */
    auto i = item2ParentPath.constBegin();
    while (i != item2ParentPath.constEnd()) {
        i->second->appendRow(i->first);
        ++i;
    }
}

QStringList KateProjectWorker::findFiles(const QDir &dir, const QVariantMap &filesEntry)
{
    const bool recursive = !filesEntry.contains(QLatin1String("recursive")) || filesEntry[QStringLiteral("recursive")].toBool();

    if (filesEntry[QStringLiteral("git")].toBool()) {
        return filesFromGit(dir, recursive);
    } else if (filesEntry[QStringLiteral("hg")].toBool()) {
        return filesFromMercurial(dir, recursive);
    } else if (filesEntry[QStringLiteral("svn")].toBool()) {
        return filesFromSubversion(dir, recursive);
    } else if (filesEntry[QStringLiteral("darcs")].toBool()) {
        return filesFromDarcs(dir, recursive);
    } else {
        QStringList files = filesEntry[QStringLiteral("list")].toStringList();

        if (files.empty()) {
            QStringList filters = filesEntry[QStringLiteral("filters")].toStringList();
            files = filesFromDirectory(dir, recursive, filters);
        }

        return files;
    }
}

QStringList KateProjectWorker::filesFromGit(const QDir &dir, bool recursive)
{
    /**
     * query files via ls-files and make them absolute afterwards
     */
    const QStringList relFiles = gitLsFiles(dir);
    QStringList files;
    for (const QString &relFile : relFiles) {
        if (!recursive && (relFile.indexOf(QLatin1Char('/')) != -1)) {
            continue;
        }

        files.append(dir.absolutePath() + QLatin1Char('/') + relFile);
    }
    return files;
}

QStringList KateProjectWorker::gitLsFiles(const QDir &dir)
{
    /**
     * git ls-files -z results a bytearray where each entry is \0-terminated.
     * NOTE: Without -z, Umlauts such as "Der Bäcker/Das Brötchen.txt" do not work (#389415)
     *
     * use --recurse-submodules, there since git 2.11 (released 2016)
     * our own submodules handling code leads to file duplicates
     */
    QStringList args;
    args << QStringLiteral("ls-files") << QStringLiteral("-z") << QStringLiteral("--recurse-submodules") << QStringLiteral(".");

    QProcess git;
    git.setWorkingDirectory(dir.absolutePath());
    git.start(QStringLiteral("git"), args);
    QStringList files;
    if (!git.waitForStarted() || !git.waitForFinished(-1)) {
        return files;
    }

    const QList<QByteArray> byteArrayList = git.readAllStandardOutput().split('\0');
    for (const QByteArray &byteArray : byteArrayList) {
        files << QString::fromUtf8(byteArray);
    }

    return files;
}

QStringList KateProjectWorker::filesFromMercurial(const QDir &dir, bool recursive)
{
    QStringList files;

    QProcess hg;
    hg.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    args << QStringLiteral("manifest") << QStringLiteral(".");
    hg.start(QStringLiteral("hg"), args);
    if (!hg.waitForStarted() || !hg.waitForFinished(-1)) {
        return files;
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const QStringList relFiles = QString::fromLocal8Bit(hg.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);
#else
    const QStringList relFiles = QString::fromLocal8Bit(hg.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), Qt::SkipEmptyParts);
#endif

    for (const QString &relFile : relFiles) {
        if (!recursive && (relFile.indexOf(QLatin1Char('/')) != -1)) {
            continue;
        }

        files.append(dir.absolutePath() + QLatin1Char('/') + relFile);
    }

    return files;
}

QStringList KateProjectWorker::filesFromSubversion(const QDir &dir, bool recursive)
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
    if (!svn.waitForStarted() || !svn.waitForFinished(-1)) {
        return files;
    }

    /**
     * get output and split up into lines
     */
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const QStringList lines = QString::fromLocal8Bit(svn.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);
#else
    const QStringList lines = QString::fromLocal8Bit(svn.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), Qt::SkipEmptyParts);
#endif

    /**
     * remove start of line that is no filename, sort out unknown and ignore
     */
    bool first = true;
    int prefixLength = -1;

    for (const QString &line : lines) {
        /**
         * get length of stuff to cut
         */
        if (first) {
            /**
             * try to find ., else fail
             */
            prefixLength = line.lastIndexOf(QLatin1Char('.'));
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

QStringList KateProjectWorker::filesFromDarcs(const QDir &dir, bool recursive)
{
    QStringList files;

    const QString cmd = QStringLiteral("darcs");
    QString root;

    {
        QProcess darcs;
        darcs.setWorkingDirectory(dir.absolutePath());
        QStringList args;
        args << QStringLiteral("list") << QStringLiteral("repo");

        darcs.start(cmd, args);

        if (!darcs.waitForStarted() || !darcs.waitForFinished(-1))
            return files;

        auto str = QString::fromLocal8Bit(darcs.readAllStandardOutput());
        QRegularExpression exp(QStringLiteral("Root: ([^\\n\\r]*)"));
        auto match = exp.match(str);

        if (!match.hasMatch())
            return files;

        root = match.captured(1);
    }

    QStringList relFiles;
    {
        QProcess darcs;
        QStringList args;
        darcs.setWorkingDirectory(dir.absolutePath());
        args << QStringLiteral("list") << QStringLiteral("files") << QStringLiteral("--no-directories") << QStringLiteral("--pending");

        darcs.start(cmd, args);

        if (!darcs.waitForStarted() || !darcs.waitForFinished(-1))
            return files;

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        relFiles = QString::fromLocal8Bit(darcs.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);
#else
        relFiles = QString::fromLocal8Bit(darcs.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), Qt::SkipEmptyParts);
#endif
    }

    for (const QString &relFile : relFiles) {
        const QString path = dir.relativeFilePath(root + QLatin1String("/") + relFile);

        if ((!recursive && (relFile.indexOf(QLatin1Char('/')) != -1)) || (recursive && (relFile.indexOf(QLatin1String("..")) == 0))) {
            continue;
        }

        files.append(dir.absoluteFilePath(path));
    }

    return files;
}

QStringList KateProjectWorker::filesFromDirectory(const QDir &_dir, bool recursive, const QStringList &filters)
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

void KateProjectWorker::loadIndex(const QStringList &files, bool force)
{
    const QString keyCtags = QStringLiteral("ctags");
    const QVariantMap ctagsMap = m_projectMap[keyCtags].toMap();
    /**
     * load index, if enabled
     * before this was default on, which is dangerous for large repositories, e.g. out-of-memory or out-of-disk
     * if specified in project map; use that setting, otherwise fall back to global setting
     */
    bool indexEnabled = !m_indexDir.isEmpty();
    auto indexValue = ctagsMap[QStringLiteral("enable")];
    if (!indexValue.isNull()) {
        indexEnabled = indexValue.toBool();
    }
    if (!indexEnabled) {
        emit loadIndexDone(KateProjectSharedProjectIndex());
        return;
    }

    /**
     * create new index, this will do the loading in the constructor
     * wrap it into shared pointer for transfer to main thread
     */
    KateProjectSharedProjectIndex index(new KateProjectIndex(m_baseDir, m_indexDir, files, ctagsMap, force));

    emit loadIndexDone(index);
}
