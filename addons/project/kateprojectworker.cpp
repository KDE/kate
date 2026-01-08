/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectworker.h"
#include "kateprojectindex.h"
#include "kateprojectitem.h"

#include "hostprocess.h"
#include <bytearraysplitter.h>
#include <gitprocess.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>
#include <QtConcurrentMap>

#include <KLocalizedString>

#include <algorithm>
#include <vector>

KateProjectWorker::KateProjectWorker(const QString &baseDir, const QString &indexDir, const QVariantMap &projectMap, bool force)
    : m_baseDir(baseDir)
    , m_indexDir(indexDir)
    , m_projectMap(projectMap)
    , m_force(force)
{
    Q_ASSERT(!m_baseDir.isEmpty());
}

void KateProjectWorker::run()
{
    /**
     * Create dummy top level parent item and empty map inside shared pointers
     * then load the project recursively
     */
    KateProjectSharedQStandardItem topLevel(new QStandardItem());
    KateProjectSharedQHashStringItem file2Item(new QHash<QString, KateProjectItem *>());
    loadProject(topLevel.get(), m_projectMap, file2Item.get(), m_baseDir);

    /**
     * sort the stuff once recursively, this is a LOT faster than once sorting the list
     * as we have normally not all stuff in on level of directory
     */
    topLevel->sortChildren(0);

    /**
     * decide if we need to create an index
     * if we need to do so, we will need to create a copy of the file list for later use
     * before this was default on, which is dangerous for large repositories, e.g. out-of-memory or out-of-disk
     * if specified in project map; use that setting, otherwise fall back to global setting
     */
    bool indexEnabled = !m_indexDir.isEmpty();
    const QVariantMap ctagsMap = m_projectMap[QStringLiteral("ctags")].toMap();
    auto indexValue = ctagsMap[QStringLiteral("enable")];
    if (!indexValue.isNull()) {
        indexEnabled = indexValue.toBool();
    }

    /**
     * create some local backup of some data we need for further processing!
     * this is expensive, therefore only really do this if required!
     */
    QStringList files;
    if (indexEnabled) {
        files = file2Item->keys();
    }

    /**
     * hand out our model item & mapping to the main thread
     * that will let Kate already show the project, even before index processing starts
     */
    Q_EMIT loadDone(topLevel, file2Item);

    /**
     * without indexing, we are even done with all stuff here
     */
    if (!indexEnabled) {
        Q_EMIT loadIndexDone(KateProjectSharedProjectIndex());
        return;
    }

    /**
     * create new index, this will do the loading in the constructor
     * wrap it into shared pointer for transfer to main thread
     */
    KateProjectSharedProjectIndex index(new KateProjectIndex(m_baseDir, m_indexDir, files, ctagsMap, m_force));
    Q_EMIT loadIndexDone(index);
}

void KateProjectWorker::loadProject(QStandardItem *parent, const QVariantMap &project, QHash<QString, KateProjectItem *> *file2Item, const QString &baseDir)
{
    /**
     * recurse to sub-projects FIRST
     */
    const QVariantList subGroups = project[QStringLiteral("projects")].toList();
    for (const QVariant &subGroupVariant : subGroups) {
        /**
         * convert to map and get name, else skip
         */
        const QVariantMap subProject = subGroupVariant.toMap();
        const QString keyName = QStringLiteral("name");
        if (subProject[keyName].toString().isEmpty()) {
            continue;
        }

        /**
         * recurse
         */
        QStandardItem *subProjectItem = new KateProjectItem(KateProjectItem::Project, subProject[keyName].toString(), QString());
        loadProject(subProjectItem, subProject, file2Item, baseDir);
        parent->appendRow(subProjectItem);
    }

    /**
     * load all specified files
     */
    const QString keyFiles = QStringLiteral("files");
    const QVariantList files = project[keyFiles].toList();
    for (const QVariant &fileVariant : files) {
        loadFilesEntry(parent, fileVariant.toMap(), file2Item, baseDir);
    }
}

/**
 * small helper to construct directory parent items
 * @param dir2Item map for path => item
 * @param path current path we need item for
 * @return correct parent item for given path, will reuse existing ones
 */
QStandardItem *KateProjectWorker::directoryParent(const QDir &base, QHash<QString, QStandardItem *> &dir2Item, QString path)
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
    const auto existingIt = dir2Item.find(path);
    if (existingIt != dir2Item.end()) {
        return existingIt.value();
    }

    /**
     * else: construct recursively
     */
    const int slashIndex = path.lastIndexOf(QLatin1Char('/'));

    /**
     * no slash?
     * simple, no recursion, append new item toplevel
     */
    if (slashIndex < 0) {
        const auto item = new KateProjectItem(KateProjectItem::Directory, path, base.absoluteFilePath(path));
        dir2Item[path] = item;
        dir2Item[QString()]->appendRow(item);
        return item;
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
        return directoryParent(base, dir2Item, leftPart.isEmpty() ? rightPart : leftPart);
    }

    /**
     * else: recurse on left side
     */
    const auto item = new KateProjectItem(KateProjectItem::Directory, rightPart, base.absoluteFilePath(path));
    dir2Item[path] = item;
    directoryParent(base, dir2Item, leftPart)->appendRow(item);
    return item;
}

void KateProjectWorker::loadFilesEntry(QStandardItem *parent,
                                       const QVariantMap &filesEntry,
                                       QHash<QString, KateProjectItem *> *file2Item,
                                       const QString &baseDir)
{
    QDir dir(baseDir);
    if (QString filesEntryDir = filesEntry[QStringLiteral("directory")].toString(); !dir.cd(filesEntryDir)) {
        Q_EMIT errorOccurred(i18n("Unable to find directory: %1", filesEntryDir));
        return;
    }

    /**
     * handle linked projects, if any
     * one can reference other projects by specifying the path to them
     */
    QStringList linkedProjects = filesEntry[QStringLiteral("projects")].toStringList();
    if (!linkedProjects.empty()) {
        /**
         * ensure project files are made absolute in respect to correct base dir
         */
        for (auto &project : linkedProjects) {
            project = dir.absoluteFilePath(project);
        }

        /**
         * users might have specified duplicates, this can't happen for the other ways
         */
        linkedProjects.removeDuplicates();

        /**
         * filter out all directories that have no .kateproject inside!
         */
        linkedProjects.erase(std::remove_if(linkedProjects.begin(),
                                            linkedProjects.end(),
                                            [](const QString &item) {
                                                const QFileInfo projectFile(item + QLatin1String("/.kateproject"));
                                                return !projectFile.exists() || !projectFile.isFile();
                                            }),
                             linkedProjects.end());

        /**
         * we sort the projects, below we require that we walk them in order:
         * lala
         * lala/test
         * mow
         * mow/test2
         */
        std::sort(linkedProjects.begin(), linkedProjects.end());

        /**
         * now add our projects to the current item parent
         * later the tree view will e.g. allow to jump to the sub-projects
         */
        QHash<QString, QStandardItem *> dir2Item;
        dir2Item[QString()] = parent;
        for (const auto &filePath : std::as_const(linkedProjects)) {
            /**
             * cheap file name computation
             * we do this A LOT, QFileInfo is very expensive just for this operation
             */
            const int slashIndex = filePath.lastIndexOf(QLatin1Char('/'));
            const QString fileName = (slashIndex < 0) ? filePath : filePath.mid(slashIndex + 1);
            const QString filePathName = (slashIndex < 0) ? QString() : filePath.left(slashIndex);

            /**
             * construct the item with right directory prefix
             * already hang in directories in tree
             */
            auto *fileItem = new KateProjectItem(KateProjectItem::LinkedProject, fileName, filePath);

            /**
             * projects are directories, register them, we walk in order over the projects
             * even if the nest, toplevel ones would have been done before!
             */
            dir2Item[dir.relativeFilePath(filePath)] = fileItem;

            // get the directory's relative path to the base directory
            QString dirRelPath = dir.relativeFilePath(filePathName);
            // if the relative path is ".", clean it up
            if (dirRelPath == QLatin1Char('.')) {
                dirRelPath = QString();
            }

            // put in our item to the right directory parent
            directoryParent(dir, dir2Item, dirRelPath)->appendRow(fileItem);
        }

        /**
         * files with linked projects will ignore all other stuff inside
         */
        return;
    }

    /**
     * get list of files for this directory, might query the VCS
     */
    std::vector<FileEntry> preparedItems;
    findFiles(dir, filesEntry, preparedItems);

    /**
     * precompute regex to exclude stuff for the worker threads
     */
    const QStringList excludeFolderPatterns = m_projectMap.value(QStringLiteral("exclude_patterns")).toStringList();
    std::vector<QRegularExpression> excludeRegexps;
    excludeRegexps.reserve(excludeFolderPatterns.size());
    for (const auto &pattern : excludeFolderPatterns) {
        excludeRegexps.push_back(QRegularExpression(pattern, QRegularExpression::DontCaptureOption));
    }

    /**
     * sort out non-files
     * even for git, that just reports non-directories, we need to filter out e.g. sym-links to directories
     * we use map, not filter, less locking!
     * we compute here already the KateProjectItem items we want to use later
     * this happens in the threads, we later skip all nullptr entries
     */
    QtConcurrent::blockingMap(preparedItems, [dir, excludeRegexps](FileEntry &item) {
        /**
         * cheap file name computation
         * we do this A LOT, QFileInfo is very expensive just for this operation
         * we remember fullFilePath for later use and overwrite filePath with the part without the filename for later use, too
         */
        auto &[filePath, fullFilePath, projectItem] = item;
        const QFileInfo info(dir, filePath);
        fullFilePath = info.absoluteFilePath();

        for (const auto &excludePattern : excludeRegexps) {
            if (excludePattern.match(filePath).hasMatch()) {
                return;
            }
        }

        const int slashIndex = filePath.lastIndexOf(QLatin1Char('/'));
        const QString fileName = (slashIndex < 0) ? filePath : filePath.mid(slashIndex + 1);
        filePath = (slashIndex < 0) ? QString() : filePath.left(slashIndex);

        /**
         * construct the item with info about filename + full file path
         */
        if (info.isFile()) {
            projectItem = new KateProjectItem(KateProjectItem::File, fileName, fullFilePath);
        } else if (info.isDir() && QDir(fullFilePath).isEmpty()) {
            projectItem = new KateProjectItem(KateProjectItem::Directory, fileName, fullFilePath);
        }
    });

    /**
     * put the pre-computed file items in our file2Item hash + create the needed directory items
     * all other stuff was already handled inside the worker threads to avoid main thread stalling
     */
    QHash<QString, QStandardItem *> dir2Item;
    dir2Item[QString()] = parent;
    file2Item->reserve(file2Item->size() + preparedItems.size());
    for (const auto &item : preparedItems) {
        /**
         * skip all entries without an item => that are filtered out non-files
         */
        const auto &[filePath, fullFilePath, projectItem] = item;
        if (!projectItem) {
            continue;
        }

        /**
         * register the item in the full file path => item hash
         * create needed directory parents
         */
        (*file2Item)[fullFilePath] = projectItem;
        directoryParent(dir, dir2Item, filePath)->appendRow(projectItem);
    }
}

void KateProjectWorker::findFiles(const QDir &dir, const QVariantMap &filesEntry, std::vector<FileEntry> &outFiles)
{
    /**
     * shall we collect files recursively or not?
     */
    const bool recursive = !filesEntry.contains(QLatin1String("recursive")) || filesEntry[QStringLiteral("recursive")].toBool();

    /**
     * try the different version control systems first
     */

    if (filesEntry[QStringLiteral("git")].toBool()) {
        filesFromGit(dir, recursive, outFiles);
        return;
    }

    if (filesEntry[QStringLiteral("svn")].toBool()) {
        filesFromSubversion(dir, recursive, outFiles);
        return;
    }

    if (filesEntry[QStringLiteral("hg")].toBool()) {
        filesFromMercurial(dir, recursive, outFiles);
        return;
    }

    if (filesEntry[QStringLiteral("darcs")].toBool()) {
        filesFromDarcs(dir, recursive, outFiles);
        return;
    }

    if (filesEntry[QStringLiteral("fossil")].toBool()) {
        filesFromFossil(dir, recursive, outFiles);
        return;
    }

    /**
     * if we arrive here, we have some manual specification of files, no VCS
     */

    /**
     * try explicit list of stuff
     */
    QStringList userGivenFilesList = filesEntry[QStringLiteral("list")].toStringList();
    if (!userGivenFilesList.empty()) {
        /**
         * make the files relative to current dir
         * later code requires that, see bug 512981
         */
        for (auto &file : userGivenFilesList) {
            file = dir.relativeFilePath(file);
        }

        /**
         * users might have specified duplicates, this can't happen for the other ways
         */
        userGivenFilesList.removeDuplicates();
        for (const auto &file : std::as_const(userGivenFilesList)) {
            outFiles.push_back(FileEntry{.filePath = file});
        }
        return;
    }

    /**
     * if nothing found for that, try to use filters to scan the directory
     * here we only get files
     */
    filesFromDirectory(dir, recursive, filesEntry, outFiles);
}

void KateProjectWorker::filesFromGit(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles)
{
    /**
     * query files via ls-files and make them absolute afterwards
     */

    /**
     * git ls-files -z results a bytearray where each entry is \0-terminated.
     * NOTE: Without -z, Umlauts such as "Der Bäcker/Das Brötchen.txt" do not work (#389415)
     *
     * use --recurse-submodules, there since git 2.11 (released 2016)
     * our own submodules handling code leads to file duplicates
     */

    QStringList lsFilesArgs{QStringLiteral("ls-files"), QStringLiteral("-z"), QStringLiteral("--recurse-submodules"), QStringLiteral(".")};

    /**
     * ls-files untracked
     */
    QStringList lsFilesUntrackedArgs{QStringLiteral("ls-files"),
                                     QStringLiteral("-z"),
                                     QStringLiteral("--others"),
                                     QStringLiteral("--exclude-standard"),
                                     QStringLiteral(".")};

    /**
     * for recent enough git versions ensure we don't show duplicated files
     */
    const auto [major, minor] = getGitVersion(dir.absolutePath());
    if (major > 2 || (major == 2 && minor >= 31)) {
        lsFilesArgs.insert(3, QStringLiteral("--deduplicate"));
        lsFilesUntrackedArgs.insert(4, QStringLiteral("--deduplicate"));
    }

    if (major == -1) {
        Q_EMIT errorOccurred(notInstalledErrorString(QStringLiteral("'git'")));
        return;
    }

    // ls-files + ls-files untracked
    gitFiles(dir, recursive, lsFilesArgs, outFiles);
    gitFiles(dir, recursive, lsFilesUntrackedArgs, outFiles);
}

void KateProjectWorker::gitFiles(const QDir &dir, bool recursive, const QStringList &args, std::vector<FileEntry> &outFiles)
{
    QProcess git;
    if (!setupGitProcess(git, dir.absolutePath(), args)) {
        return;
    }
    startHostProcess(git, QProcess::ReadOnly);
    if (!git.waitForStarted() || !git.waitForFinished(-1)) {
        return;
    }

    const QByteArray b = git.readAllStandardOutput();
    for (strview byteArray : ByteArraySplitter(b, '\0')) {
        if (byteArray.empty()) {
            continue;
        }
        if (!recursive && (byteArray.find('/') != std::string::npos)) {
            continue;
        }
        outFiles.push_back(FileEntry{.filePath = byteArray.toString()});
    }
}

void KateProjectWorker::filesFromMercurial(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles)
{
    // only use version control from PATH
    static const auto fullExecutablePath = safeExecutableName(QStringLiteral("hg"));
    if (fullExecutablePath.isEmpty()) {
        Q_EMIT errorOccurred(notInstalledErrorString(QStringLiteral("'hg'")));
        return;
    }

    QProcess hg;
    hg.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    args << QStringLiteral("manifest") << QStringLiteral(".");
    startHostProcess(hg, fullExecutablePath, args, QProcess::ReadOnly);
    if (!hg.waitForStarted() || !hg.waitForFinished(-1)) {
        return;
    }

    const QStringList relFiles = QString::fromLocal8Bit(hg.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), Qt::SkipEmptyParts);

    outFiles.reserve(relFiles.size());
    for (const QString &relFile : relFiles) {
        if (!recursive && (relFile.indexOf(QLatin1Char('/')) != -1)) {
            continue;
        }

        outFiles.push_back(FileEntry{.filePath = relFile});
    }
}

void KateProjectWorker::filesFromSubversion(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles)
{
    // only use version control from PATH
    static const auto fullExecutablePath = safeExecutableName(QStringLiteral("svn"));
    if (fullExecutablePath.isEmpty()) {
        Q_EMIT errorOccurred(notInstalledErrorString(QStringLiteral("'svn'")));
        return;
    }

    QProcess svn;
    svn.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    args << QStringLiteral("status") << QStringLiteral("--verbose") << QStringLiteral(".");
    if (recursive) {
        args << QStringLiteral("--depth=infinity");
    } else {
        args << QStringLiteral("--depth=files");
    }
    startHostProcess(svn, fullExecutablePath, args, QProcess::ReadOnly);
    if (!svn.waitForStarted() || !svn.waitForFinished(-1)) {
        return;
    }

    /**
     * get output and split up into lines
     */
    const QStringList lines = QString::fromLocal8Bit(svn.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), Qt::SkipEmptyParts);

    /**
     * remove start of line that is no filename, sort out unknown and ignore
     */
    bool first = true;
    int prefixLength = -1;

    outFiles.reserve(lines.size());
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
            outFiles.push_back(FileEntry{.filePath = line.right(line.size() - prefixLength)});
        }
    }
}

void KateProjectWorker::filesFromDarcs(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles)
{
    // only use version control from PATH
    static const auto fullExecutablePath = safeExecutableName(QStringLiteral("darcs"));
    if (fullExecutablePath.isEmpty()) {
        Q_EMIT errorOccurred(notInstalledErrorString(QStringLiteral("'darcs'")));
        return;
    }

    QString root;
    {
        QProcess darcs;
        darcs.setWorkingDirectory(dir.absolutePath());
        QStringList args;
        args << QStringLiteral("list") << QStringLiteral("repo");

        startHostProcess(darcs, fullExecutablePath, args, QProcess::ReadOnly);

        if (!darcs.waitForStarted() || !darcs.waitForFinished(-1)) {
            return;
        }

        auto str = QString::fromLocal8Bit(darcs.readAllStandardOutput());
        static const QRegularExpression exp(QStringLiteral("Root: ([^\\n\\r]*)"));
        auto match = exp.match(str);

        if (!match.hasMatch()) {
            return;
        }

        root = match.captured(1);
    }

    QStringList relFiles;
    {
        QProcess darcs;
        QStringList args;
        darcs.setWorkingDirectory(dir.absolutePath());
        args << QStringLiteral("list") << QStringLiteral("files") << QStringLiteral("--no-directories") << QStringLiteral("--pending");

        startHostProcess(darcs, fullExecutablePath, args, QProcess::ReadOnly);

        if (!darcs.waitForStarted() || !darcs.waitForFinished(-1)) {
            return;
        }

        relFiles = QString::fromLocal8Bit(darcs.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), Qt::SkipEmptyParts);
    }

    outFiles.reserve(relFiles.size());
    for (const QString &relFile : std::as_const(relFiles)) {
        const QString path = dir.relativeFilePath(root + QLatin1String("/") + relFile);

        if ((!recursive && (relFile.indexOf(QLatin1Char('/')) != -1)) || (recursive && (relFile.indexOf(QLatin1String("..")) == 0))) {
            continue;
        }

        outFiles.push_back(FileEntry{.filePath = path});
    }
}

void KateProjectWorker::filesFromFossil(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles)
{
    // only use version control from PATH
    static const auto fullExecutablePath = safeExecutableName(QStringLiteral("fossil"));
    if (fullExecutablePath.isEmpty()) {
        Q_EMIT errorOccurred(notInstalledErrorString(QStringLiteral("'fossil'")));
        return;
    }

    QProcess fossil;
    fossil.setWorkingDirectory(dir.absolutePath());
    QStringList args;
    args << QStringLiteral("ls");
    startHostProcess(fossil, fullExecutablePath, args, QProcess::ReadOnly);
    if (!fossil.waitForStarted() || !fossil.waitForFinished(-1)) {
        return;
    }

    const QStringList relFiles = QString::fromLocal8Bit(fossil.readAllStandardOutput()).split(QRegularExpression(QStringLiteral("[\n\r]")), Qt::SkipEmptyParts);

    outFiles.reserve(relFiles.size());
    for (const QString &relFile : relFiles) {
        if (!recursive && (relFile.indexOf(QLatin1Char('/')) != -1)) {
            continue;
        }

        outFiles.push_back(FileEntry{.filePath = relFile});
    }
}

void KateProjectWorker::filesFromDirectory(QDir dir, bool recursive, const QVariantMap &filesEntry, std::vector<FileEntry> &outFiles)
{
    /**
     * setup our filters
     */
    QDir::Filters filterFlags = QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot;
    if (filesEntry.value(QStringLiteral("hidden")).toBool()) {
        filterFlags |= QDir::Hidden;
    }
    dir.setFilter(filterFlags);
    if (const auto filters = filesEntry.value(QStringLiteral("filters")).toStringList(); !filters.isEmpty()) {
        dir.setNameFilters(filters);
    }

    /**
     * construct flags for iterator
     */
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) {
        flags |= QDirIterator::Subdirectories;
        if (filesEntry.value(QStringLiteral("symlinks")).toBool()) {
            flags |= QDirIterator::FollowSymlinks;
        }
    }

    /**
     * trigger potential recursive directory search
     */
    QDirIterator dirIterator(dir, flags);
    const QString dirPath = dir.path() + QLatin1Char('/');
    while (dirIterator.hasNext()) {
        dirIterator.next();
        // make it relative path
        outFiles.push_back(FileEntry{.filePath = dirIterator.filePath().remove(dirPath)});
    }
}

QString KateProjectWorker::notInstalledErrorString(const QString &program)
{
    return i18n(
        "Unable to load %1 based project because either %1 is not installed or it wasn't found in PATH environment variable. Please install %1 or "
        "alternatively disable the option 'Autoload Repositories && Build Trees' in project settings.",
        program);
}

#include "moc_kateprojectworker.cpp"
