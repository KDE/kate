/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QRunnable>
#include <QSet>
#include <QVariant>

class QDir;
class KateProjectItem;
class QStandardItem;
class KateProjectIndex;

typedef std::shared_ptr<QStandardItem> KateProjectSharedQStandardItem;
typedef std::shared_ptr<QHash<QString, KateProjectItem *>> KateProjectSharedQHashStringItem;
typedef std::shared_ptr<KateProjectIndex> KateProjectSharedProjectIndex;

/**
 * Class representing a project background worker.
 * This worker will build up the model for the project on load and do other stuff in the background.
 */
class KateProjectWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    /**
     * Type for QueuedConnection
     */
    typedef QHash<QString, KateProjectItem *> MapString2Item;

    explicit KateProjectWorker(const QString &baseDir, const QString &indexDir, const QVariantMap &projectMap, bool force);

    void run() override;

    static QStandardItem *directoryParent(const QDir &base, QHash<QString, QStandardItem *> &dir2Item, QString path);

Q_SIGNALS:
    void loadDone(KateProjectSharedQStandardItem topLevel, KateProjectSharedQHashStringItem file2Item);
    void loadIndexDone(KateProjectSharedProjectIndex index);
    void errorOccurred(const QString &);

private:
    struct FileEntry {
        QString filePath;
        QString fullFilePath = QString(); // the explicit QString() is for clangd
        KateProjectItem *projectItem = nullptr;
    };

    /**
     * Load one project inside the project tree.
     * Fill data from JSON storage to model and recurse to sub-projects.
     * @param parent parent standard item in the model
     * @param project variant map for this group
     * @param file2Item mapping file => item, will be filled
     */
    void loadProject(QStandardItem *parent, const QVariantMap &project, QHash<QString, KateProjectItem *> *file2Item, const QString &baseDir);

    /**
     * Load one files entry in the current parent item.
     * @param parent parent standard item in the model
     * @param filesEntry one files entry specification to load
     * @param file2Item mapping file => item, will be filled
     */
    void loadFilesEntry(QStandardItem *parent, const QVariantMap &filesEntry, QHash<QString, KateProjectItem *> *file2Item, const QString &baseDir);

    void findFiles(const QDir &dir, const QVariantMap &filesEntry, std::vector<FileEntry> &outFiles);

    void filesFromGit(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles);
    void filesFromMercurial(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles);
    void filesFromSubversion(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles);
    void filesFromDarcs(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles);
    void filesFromFossil(const QDir &dir, bool recursive, std::vector<FileEntry> &outFiles);
    static void filesFromDirectory(QDir dir, bool recursive, const QVariantMap &filesEntry, std::vector<FileEntry> &outFiles);

    static void gitFiles(const QDir &dir, bool recursive, const QStringList &args, std::vector<FileEntry> &outFiles);

private:
    static QString notInstalledErrorString(const QString &program);

    /**
     * project base directory name
     */
    const QString m_baseDir;

    /**
     * index directory
     */
    const QString m_indexDir;

    const QVariantMap m_projectMap;
    const bool m_force;
};
