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
#include <QRegularExpression>
#include <QSet>
#include <QTime>

#ifdef LIBGIT2_FOUND
#include <git2.h>
#include <git2/oid.h>
#include <git2/repository.h>
#endif

KateProjectWorker::KateProjectWorker(const QString &baseDir, const QVariantMap &projectMap)
    : QObject()
    , ThreadWeaver::Job()
    , m_baseDir(baseDir)
    , m_projectMap(projectMap)
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
    KateProjectSharedQMapStringItem file2Item(new QMap<QString, KateProjectItem *> ());
    loadProject(topLevel.data(), m_projectMap, file2Item.data());

    /**
     * create some local backup of some data we need for further processing!
     */
    QStringList files = file2Item->keys();

    emit loadDone(topLevel, file2Item);

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
    for (const QVariant &subGroupVariant: subGroups) {
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

    files.sort();

    /**
     * construct paths first in tree and items in a map
     */
    QMap<QString, QStandardItem *> dir2Item;
    dir2Item[QString()] = parent;
    QList<QPair<QStandardItem *, QStandardItem *> > item2ParentPath;
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
        if (dirRelPath == QStringLiteral(".")) {
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

QStringList KateProjectWorker::findFiles(const QDir &dir, const QVariantMap& filesEntry)
{
    const bool recursive = !filesEntry.contains(QStringLiteral("recursive")) || filesEntry[QStringLiteral("recursive")].toBool();

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

#ifdef LIBGIT2_FOUND
namespace {
    struct git_walk_payload {
        QStringList *files;
        bool recursive;
        QString basedir;
    };

    int gitTreeWalker(const char *root, const git_tree_entry *entry, void *payload)
    {
        struct git_walk_payload *data = static_cast<git_walk_payload *>(payload);

        if (git_tree_entry_type(entry) == GIT_OBJ_BLOB) {
            QString name = QString::fromUtf8(git_tree_entry_name(entry));
            QString dir = QString::fromUtf8(root);
            QString filepath = QDir(data->basedir + dir).filePath(name);
            data->files->append(filepath);
        } else if (git_tree_entry_type(entry) == GIT_OBJ_TREE && !data->recursive) {
            return 1; // don't walk that way
        }

        return 0;
    }

    QStringList gitSearchTree(git_object *tree, const QString &basedir, bool recursive)
    {
        QStringList files;
        struct git_walk_payload payload = {&files, recursive, basedir};

        git_tree_walk((git_tree *)tree, GIT_TREEWALK_PRE, gitTreeWalker, (void *)&payload);

        return files;
    }

    int gitSubmoduleWalker(git_submodule *submodule, const char *, void *payload)
    {
        struct git_walk_payload *data = static_cast<git_walk_payload *>(payload);

        git_repository *repo = nullptr;

        if (git_submodule_open(&repo, submodule)) {
            return 1;
        }

        git_object *tree = nullptr;

        if (git_revparse_single(&tree, repo, "HEAD^{tree}")) {
            git_repository_free(repo);
            return 2;
        }

        QString submodulePath = QString::fromUtf8(git_submodule_path(submodule));
        QString basedir = data->basedir + submodulePath + QDir::separator();

        data->files->append(gitSearchTree(tree, basedir, true));

        git_object_free(tree);
        git_repository_free(repo);
        return 0;
    }

    QStringList gitSearchSubmodules(git_repository *repo, const QString &workdir)
    {
        QStringList files;

        struct git_walk_payload payload = {&files, true, workdir};
        git_submodule_foreach(repo, gitSubmoduleWalker, (void *)&payload);

        return files;
    }
}

QStringList KateProjectWorker::filesFromGit(const QDir &dir, bool recursive)
{
    // init libgit2, we require at least 0.22 which has this function!
    // do this here to have init in this thread done, shutdown afterwards again!
    git_libgit2_init();

    QStringList files;
    git_repository *repo = nullptr;
    git_object *root_tree = nullptr, *tree = nullptr;

    // check if the repo can be opened.
    // git_repository_open_ext() will return 0 if everything is OK;
    // if not, return an empty files list
    const QByteArray repoPathUtf8 = dir.path().toUtf8();
    if (git_repository_open_ext(&repo, repoPathUtf8.constData(), 0, NULL)) {
        git_libgit2_shutdown();
        return files;
    }

    // get the working directory of the repo
    // if none was found, return an empty files list
    const char *working_dir = nullptr;
    if ((working_dir = git_repository_workdir(repo)) == nullptr) {
        git_repository_free(repo);
        git_libgit2_shutdown();
        return files;
    }

    if (git_revparse_single(&root_tree, repo, "HEAD^{tree}")) {
        git_repository_free(repo);
        git_libgit2_shutdown();
        return files;
    }

    QDir workdir;
    workdir.setPath(QString::fromUtf8(working_dir));
    const QByteArray relpathUtf8 = workdir.relativeFilePath(dir.path()).toUtf8();

    if (relpathUtf8.isEmpty() || relpathUtf8 == ".") { // git_object_lookup_bypath is not able to resolv "." as path
        tree = root_tree;
    } else {
        if (git_object_lookup_bypath(&tree, root_tree, relpathUtf8.constData(), GIT_OBJ_TREE)) {
            git_object_free(root_tree);
            git_repository_free(repo);
            git_libgit2_shutdown();
            return files;
        }
    }

    QString path = workdir.absolutePath() + QDir::separator();

    files.append(gitSearchTree(tree, path, recursive));

    if (recursive && relpathUtf8.isEmpty()) {
        files.append(gitSearchSubmodules(repo, path));
    }

    if (tree != root_tree) {
        git_object_free(tree);
    }

    git_object_free(root_tree);
    git_repository_free(repo);
    git_libgit2_shutdown();
    return files;
}

#else

QStringList KateProjectWorker::filesFromGit(const QDir &dir, bool recursive)
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

    const QStringList relFiles = QString::fromLocal8Bit(git.readAllStandardOutput()).split(QRegExp(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);

    for (const QString &relFile : relFiles) {
        if (!recursive && (relFile.indexOf(QStringLiteral("/")) != -1)) {
            continue;
        }

        files.append(dir.absolutePath() + QLatin1Char('/') + relFile);
    }

    return files;
}
#endif

QStringList KateProjectWorker::filesFromMercurial(const QDir &dir, bool recursive)
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

    const QStringList relFiles = QString::fromLocal8Bit(hg.readAllStandardOutput()).split(QRegExp(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);

    for (const QString &relFile : relFiles) {
        if (!recursive && (relFile.indexOf(QStringLiteral("/")) != -1)) {
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
    if (!svn.waitForStarted() || !svn.waitForFinished()) {
        return files;
    }

    /**
     * get output and split up into lines
     */
    const QStringList lines = QString::fromLocal8Bit(svn.readAllStandardOutput()).split(QRegExp(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);

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

        if (!darcs.waitForStarted() || !darcs.waitForFinished())
            return files;

        auto str = QString::fromLocal8Bit(darcs.readAllStandardOutput());
        QRegularExpression exp(QStringLiteral("Root: ([^\\n\\r]*)"));
        auto match = exp.match(str);

        if(!match.hasMatch())
            return files;

        root = match.captured(1);
    }

    QStringList relFiles;
    {
        QProcess darcs;
        QStringList args;
        darcs.setWorkingDirectory(dir.absolutePath());
        args << QStringLiteral("list") << QStringLiteral("files")
             << QStringLiteral("--no-directories") << QStringLiteral("--pending");

        darcs.start(cmd, args);

        if(!darcs.waitForStarted() || !darcs.waitForFinished())
            return files;

        relFiles = QString::fromLocal8Bit(darcs.readAllStandardOutput())
            .split(QRegularExpression(QStringLiteral("[\n\r]")), QString::SkipEmptyParts);
    }

    for (const QString &relFile: relFiles) {
        const QString path = dir.relativeFilePath(root + QStringLiteral("/") + relFile);

        if ((!recursive && (relFile.indexOf(QStringLiteral("/")) != -1)) ||
            (recursive && (relFile.indexOf(QStringLiteral("..")) == 0))
        ) {
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

void KateProjectWorker::loadIndex(const QStringList &files)
{
    /**
     * create new index, this will do the loading in the constructor
     * wrap it into shared pointer for transfer to main thread
     */
    KateProjectSharedProjectIndex index(new KateProjectIndex(files));

    emit loadIndexDone(index);
}
