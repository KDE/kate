/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KATE_PROJECT_WORKER_H
#define KATE_PROJECT_WORKER_H

#include "kateproject.h"
#include "kateprojectitem.h"

#include <QHash>
#include <QRunnable>
#include <QStandardItemModel>

class QDir;

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

Q_SIGNALS:
    void loadDone(KateProjectSharedQStandardItem topLevel, KateProjectSharedQHashStringItem file2Item);
    void loadIndexDone(KateProjectSharedProjectIndex index);

private:
    /**
     * Load one project inside the project tree.
     * Fill data from JSON storage to model and recurse to sub-projects.
     * @param parent parent standard item in the model
     * @param project variant map for this group
     * @param file2Item mapping file => item, will be filled
     */
    void loadProject(QStandardItem *parent, const QVariantMap &project, QHash<QString, KateProjectItem *> *file2Item);

    /**
     * Load one files entry in the current parent item.
     * @param parent parent standard item in the model
     * @param filesEntry one files entry specification to load
     * @param file2Item mapping file => item, will be filled
     */
    void loadFilesEntry(QStandardItem *parent, const QVariantMap &filesEntry, QHash<QString, KateProjectItem *> *file2Item);

    QStringList findFiles(const QDir &dir, const QVariantMap &filesEntry);

    QStringList filesFromGit(const QDir &dir, bool recursive);
    QStringList filesFromMercurial(const QDir &dir, bool recursive);
    QStringList filesFromSubversion(const QDir &dir, bool recursive);
    QStringList filesFromDarcs(const QDir &dir, bool recursive);
    QStringList filesFromDirectory(const QDir &dir, bool recursive, const QStringList &filters);

    QStringList gitLsFiles(const QDir &dir);

private:
    /**
     * our project, only as QObject, we only send messages back and forth!
     */
    QObject *m_project = nullptr;

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

#endif
