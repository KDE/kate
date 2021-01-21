/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef _KATE_PROJECT_PLUGIN_H_
#define _KATE_PROJECT_PLUGIN_H_

#include <QDir>
#include <QFileSystemWatcher>
#include <QThreadPool>

#include <KTextEditor/Plugin>
#include <ktexteditor/document.h>
#include <ktexteditor/mainwindow.h>

#include <KXMLGUIClient>

#include "kateproject.h"
#include "kateprojectcompletion.h"

class KateProjectPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateProjectPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KateProjectPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

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
    QList<KateProject *> projects() const
    {
        return m_projects;
    }

    /**
     * Get global code completion.
     * @return global completion object for KTextEditor::View
     */
    KateProjectCompletion *completion()
    {
        return &m_completion;
    }

    /**
     * Map current open documents to projects.
     * @param document document we want to know which project it belongs to
     * @return project or 0 if none found for this document
     */
    KateProject *projectForDocument(KTextEditor::Document *document)
    {
        return m_document2Project.value(document);
    }

    void setAutoRepository(bool onGit, bool onSubversion, bool onMercurial);
    bool autoGit() const;
    bool autoSubversion() const;
    bool autoMercurial() const;

    void setIndex(bool enabled, const QUrl &directory);
    bool getIndexEnabled() const;
    QUrl getIndexDirectory() const;

    void setMultiProject(bool completion, bool gotoSymbol);
    bool multiProjectCompletion() const;
    bool multiProjectGoto() const;

Q_SIGNALS:
    /**
     * Signal that a new project got created.
     * @param project new created project
     */
    void projectCreated(KateProject *project);

    /**
     * Signal that plugin configuration changed
     */
    void configUpdated();

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

    void registerVariables();
    void unregisterVariables();

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
    bool m_indexEnabled : 1;
    bool m_multiProjectCompletion : 1;
    bool m_multiProjectGoto : 1;
    QUrl m_indexDirectory;


    /**
     * thread pool for our workers
     */
    QThreadPool m_threadPool;
};

#endif
