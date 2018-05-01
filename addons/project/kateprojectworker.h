/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_PROJECT_WORKER_H
#define KATE_PROJECT_WORKER_H

#include "kateprojectitem.h"
#include "kateproject.h"

#include <ThreadWeaver/Job>

#include <QStandardItemModel>
#include <QMap>

class QDir;

/**
 * Class representing a project background worker.
 * This worker will build up the model for the project on load and do other stuff in the background.
 */
class KateProjectWorker : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

public:
    /**
     * Type for QueuedConnection
     */
    typedef QMap<QString, KateProjectItem *> MapString2Item;

    explicit KateProjectWorker(const QString &baseDir, const QVariantMap &projectMap);

    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;

Q_SIGNALS:
    void loadDone(KateProjectSharedQStandardItem topLevel, KateProjectSharedQMapStringItem file2Item);
    void loadIndexDone(KateProjectSharedProjectIndex index);

private:
    /**
     * Load one project inside the project tree.
     * Fill data from JSON storage to model and recurse to sub-projects.
     * @param parent parent standard item in the model
     * @param project variant map for this group
     * @param file2Item mapping file => item, will be filled
     */
    void loadProject(QStandardItem *parent, const QVariantMap &project, QMap<QString, KateProjectItem *> *file2Item);

    /**
     * Load one files entry in the current parent item.
     * @param parent parent standard item in the model
     * @param filesEntry one files entry specification to load
     * @param file2Item mapping file => item, will be filled
     */
    void loadFilesEntry(QStandardItem *parent, const QVariantMap &filesEntry, QMap<QString, KateProjectItem *> *file2Item);

    /**
     * Load index for whole project.
     * @param files list of all project files to index
     */
    void loadIndex(const QStringList &files);

    QStringList findFiles(const QDir &dir, const QVariantMap &filesEntry);

    QStringList filesFromGit(const QDir &dir, bool recursive);
    QStringList filesFromMercurial(const QDir &dir, bool recursive);
    QStringList filesFromSubversion(const QDir &dir, bool recursive);
    QStringList filesFromDarcs(const QDir &dir, bool recursive);
    QStringList filesFromDirectory(const QDir &dir, bool recursive, const QStringList &filters);

    QStringList gitLsFiles(const QDir &dir);
    QStringList gitSubmodulesFiles(const QDir &dir);

private:
    /**
     * our project, only as QObject, we only send messages back and forth!
     */
    QObject *m_project;

    /**
     * project base directory name
     */
    QString m_baseDir;
    QVariantMap m_projectMap;
};

#endif

