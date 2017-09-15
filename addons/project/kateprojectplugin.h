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

#ifndef _PLUGIN_KATE_PROJECT_H_
#define _PLUGIN_KATE_PROJECT_H_

#include <QFileSystemWatcher>
#include <QDir>

#include <ktexteditor/document.h>
#include <ktexteditor/mainwindow.h>
#include <KTextEditor/Plugin>

#include <kxmlguiclient.h>

#include "kateproject.h"
#include "kateprojectcompletion.h"

namespace ThreadWeaver {
    class Queue;
}

class KateProjectPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateProjectPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KateProjectPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override;
    KTextEditor::ConfigPage *configPage (int number = 0, QWidget *parent = nullptr) override;

    /**
     * Create new project for given project filename.
     * Null pointer if no project can be opened.
     * File name will be canonicalized!
     * @param fileName canonicalized file name for the project
     * @return project or null if not openable
     */
    KateProject *createProjectForFileName(const QString &fileName);

    /**
     * Search and open project for given dir, if possible.
     * Will search upwards for .kateproject file.
     * Will use internally projectForFileName if project file is found.
     * @param dir dir to search matching project for
     * @return project or null if not openable
     */
    KateProject *projectForDir(QDir dir);

    /**
     * Search and open project that contains given url, if possible.
     * Will search upwards for .kateproject file, if the url is a local file.
     * Will use internally projectForDir.
     * @param url url to search matching project for
     * @return project or null if not openable
     */
    KateProject *projectForUrl(const QUrl &url);

    /**
     * get list of all current open projects
     * @return list of all open projects
     */
    QList<KateProject *> projects() const {
        return m_projects;
    }

    /**
     * Get global code completion.
     * @return global completion object for KTextEditor::View
     */
    KateProjectCompletion *completion() {
        return &m_completion;
    }

    /**
     * Map current open documents to projects.
     * @param document document we want to know which project it belongs to
     * @return project or 0 if none found for this document
     */
    KateProject *projectForDocument(KTextEditor::Document *document) {
        return m_document2Project.value(document);
    }

    void setAutoRepository(bool onGit, bool onSubversion, bool onMercurial);
    bool autoGit() const;
    bool autoSubversion() const;
    bool autoMercurial() const;

Q_SIGNALS:
    /**
     * Signal that a new project got created.
     * @param project new created project
     */
    void projectCreated(KateProject *project);

public Q_SLOTS:
    /**
     * New document got created, we need to update our connections
     * @param document new created document
     */
    void slotDocumentCreated(KTextEditor::Document *document);

    /**
     * Document got destroyed.
     * @param document deleted document
     */
    void slotDocumentDestroyed(QObject *document);

    /**
     * Url changed, to auto-load projects
     */
    void slotDocumentUrlChanged(KTextEditor::Document *document);

    /**
     * did some project file change?
     * @param path name of directory that did change
     */
    void slotDirectoryChanged(const QString &path);

private:
    KateProject *createProjectForRepository(const QString &type, const QDir &dir);
    KateProject *detectGit(const QDir &dir);
    KateProject *detectSubversion(const QDir &dir);
    KateProject *detectMercurial(const QDir &dir);

    void readConfig();
    void writeConfig();

private:
    /**
     * open plugins, maps project base directory => project
     */
    QList<KateProject *> m_projects;

    /**
     * filesystem watcher to keep track of all project files
     * and auto-reload
     */
    QFileSystemWatcher m_fileWatcher;

    /**
     * Mapping document => project
     */
    QHash<QObject *, KateProject *> m_document2Project;

    /**
     * Project completion
     */
    KateProjectCompletion m_completion;

    bool m_autoGit : 1;
    bool m_autoSubversion : 1;
    bool m_autoMercurial : 1;

    ThreadWeaver::Queue *m_weaver;
};

#endif
