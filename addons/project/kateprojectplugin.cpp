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

#include "kateprojectplugin.h"

#include "kateproject.h"
#include "kateprojectconfigpage.h"
#include "kateprojectpluginview.h"

#include <ktexteditor/editor.h>
#include <ktexteditor/application.h>
#include <ktexteditor/document.h>

#include <KSharedConfig>
#include <KConfigGroup>
#include <ThreadWeaver/Queue>

#include <QFileInfo>
#include <QTime>

#include "config.h"

#ifdef HAVE_CTERMID
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace
{
const QString ProjectFileName = QStringLiteral(".kateproject");
const QString GitFolderName = QStringLiteral(".git");
const QString SubversionFolderName = QStringLiteral(".svn");
const QString MercurialFolderName = QStringLiteral(".hg");

const QString GitConfig = QStringLiteral("git");
const QString SubversionConfig = QStringLiteral("subversion");
const QString MercurialConfig = QStringLiteral("mercurial");

const QStringList DefaultConfig = QStringList() << GitConfig << SubversionConfig << MercurialConfig;
}

KateProjectPlugin::KateProjectPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
    , m_completion(this)
    , m_autoGit(true)
    , m_autoSubversion(true)
    , m_autoMercurial(true)
    , m_weaver(new ThreadWeaver::Queue(this))
{
    qRegisterMetaType<KateProjectSharedQStandardItem>("KateProjectSharedQStandardItem");
    qRegisterMetaType<KateProjectSharedQMapStringItem>("KateProjectSharedQMapStringItem");
    qRegisterMetaType<KateProjectSharedProjectIndex>("KateProjectSharedProjectIndex");

    connect(KTextEditor::Editor::instance()->application(), &KTextEditor::Application::documentCreated, this, &KateProjectPlugin::slotDocumentCreated);
    connect(&m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &KateProjectPlugin::slotDirectoryChanged);

#ifdef HAVE_CTERMID
    /**
     * open project for our current working directory, if this kate has a terminal
     * http://stackoverflow.com/questions/1312922/detect-if-stdin-is-a-terminal-or-pipe-in-c-c-qt
     */
    char tty[L_ctermid + 1] = {0};
    ctermid(tty);
    int fd = ::open(tty, O_RDONLY);

    if (fd >= 0) {
        projectForDir(QDir::current());
        ::close(fd);
    }
#endif

    readConfig();

    for (auto document : KTextEditor::Editor::instance()->application()->documents()) {
        slotDocumentCreated(document);
    }
}

KateProjectPlugin::~KateProjectPlugin()
{
    for (KateProject *project : m_projects) {
        m_fileWatcher.removePath(QFileInfo(project->fileName()).canonicalPath());
        delete project;
    }
    m_projects.clear();

    m_weaver->shutDown();
    delete m_weaver;
}

QObject *KateProjectPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new KateProjectPluginView(this, mainWindow);
}

int KateProjectPlugin::configPages() const
{
    return 1;
}

KTextEditor::ConfigPage *KateProjectPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    return new KateProjectConfigPage(parent, this);
}

KateProject *KateProjectPlugin::createProjectForFileName(const QString &fileName)
{
    KateProject *project = new KateProject(m_weaver);

    if (!project->loadFromFile(fileName)) {
        delete project;
        return nullptr;
    }

    m_projects.append(project);
    m_fileWatcher.addPath(QFileInfo(fileName).canonicalPath());
    emit projectCreated(project);
    return project;
}

KateProject *KateProjectPlugin::projectForDir(QDir dir)
{
    /**
     * search projects upwards
     * with recursion guard
     */
    QSet<QString> seenDirectories;
    while (!seenDirectories.contains(dir.absolutePath())) {
        /**
         * fill recursion guard
         */
        seenDirectories.insert(dir.absolutePath());

        /**
         * check for project and load it if found
         */
        QString canonicalPath = dir.canonicalPath();
        QString canonicalFileName = dir.filePath(ProjectFileName);

        for (KateProject *project : m_projects) {
            if (project->baseDir() == canonicalPath || project->fileName() == canonicalFileName) {
                return project;
            }
        }

        if (dir.exists(ProjectFileName)) {
            return createProjectForFileName(canonicalFileName);
        }

        KateProject *project;
        if ((project = detectGit(dir)) || (project = detectSubversion(dir)) || (project = detectMercurial(dir))) {
            return project;
        }

        /**
         * else: cd up, if possible or abort
         */
        if (!dir.cdUp()) {
            break;
        }
    }

    return nullptr;
}

KateProject *KateProjectPlugin::projectForUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isLocalFile()) {
        return nullptr;
    }

    return projectForDir(QFileInfo(url.toLocalFile()).absoluteDir());
}

void KateProjectPlugin::slotDocumentCreated(KTextEditor::Document *document)
{
    connect(document, &KTextEditor::Document::documentUrlChanged, this, &KateProjectPlugin::slotDocumentUrlChanged);
    connect(document, &KTextEditor::Document::destroyed, this, &KateProjectPlugin::slotDocumentDestroyed);

    slotDocumentUrlChanged(document);
}

void KateProjectPlugin::slotDocumentDestroyed(QObject *document)
{
    if (KateProject *project = m_document2Project.value(document)) {
        project->unregisterDocument(static_cast<KTextEditor::Document *>(document));
    }

    m_document2Project.remove(document);
}

void KateProjectPlugin::slotDocumentUrlChanged(KTextEditor::Document *document)
{
    KateProject *project = projectForUrl(document->url());

    if (KateProject *project = m_document2Project.value(document)) {
        project->unregisterDocument(document);
    }

    if (!project) {
        m_document2Project.remove(document);
    } else {
        m_document2Project[document] = project;
    }

    if (KateProject *project = m_document2Project.value(document)) {
        project->registerDocument(document);
    }
}

void KateProjectPlugin::slotDirectoryChanged(const QString &path)
{
    QString fileName = QDir(path).filePath(ProjectFileName);
    for (KateProject * project : m_projects) {
        if (project->fileName() == fileName) {
            QDateTime lastModified = QFileInfo(fileName).lastModified();
            if (project->fileLastModified().isNull() || (lastModified > project->fileLastModified())) {
                project->reload();
            }
            break;
        }
    }
}

KateProject* KateProjectPlugin::detectGit(const QDir &dir)
{
    // allow .git as dir and file (file for git worktree stuff, https://git-scm.com/docs/git-worktree)
    if (m_autoGit && dir.exists(GitFolderName)) {
        return createProjectForRepository(QStringLiteral("git"), dir);
    }

    return nullptr;
}

KateProject* KateProjectPlugin::detectSubversion(const QDir &dir)
{
    if (m_autoSubversion && dir.exists(SubversionFolderName) && QFileInfo(dir, SubversionFolderName).isDir()) {
        return createProjectForRepository(QStringLiteral("svn"), dir);
    }

    return nullptr;
}

KateProject* KateProjectPlugin::detectMercurial(const QDir &dir)
{
    if (m_autoMercurial && dir.exists(MercurialFolderName) && QFileInfo(dir, MercurialFolderName).isDir()) {
        return createProjectForRepository(QStringLiteral("hg"), dir);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::createProjectForRepository(const QString &type, const QDir &dir)
{
    QVariantMap cnf, files;
    files[type] = 1;
    cnf[QLatin1String("name")] = dir.dirName();
    cnf[QLatin1String("files")] = (QVariantList() << files);

    KateProject *project = new KateProject(m_weaver);
    project->loadFromData(cnf, dir.canonicalPath());

    m_projects.append(project);

    emit projectCreated(project);
    return project;
}

void KateProjectPlugin::setAutoRepository(bool onGit, bool onSubversion, bool onMercurial)
{
    m_autoGit = onGit;
    m_autoSubversion = onSubversion;
    m_autoMercurial = onMercurial;
    writeConfig();
}

bool KateProjectPlugin::autoGit() const
{
    return m_autoGit;
}

bool KateProjectPlugin::autoSubversion() const
{
    return m_autoSubversion;
}

bool KateProjectPlugin::autoMercurial() const
{
    return m_autoMercurial;
}

void KateProjectPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "project");
    QStringList autorepository = config.readEntry("autorepository", DefaultConfig);

    m_autoGit = m_autoSubversion = m_autoMercurial = false;

    if (autorepository.contains(GitConfig)) {
        m_autoGit = true;
    }

    if (autorepository.contains(SubversionConfig)) {
        m_autoSubversion = true;
    }

    if (autorepository.contains(MercurialConfig)) {
        m_autoMercurial = true;
    }
}

void KateProjectPlugin::writeConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "project");
    QStringList repos;

    if (m_autoGit) {
        repos << GitConfig;
    }

    if (m_autoSubversion) {
        repos << SubversionConfig;
    }

    if (m_autoMercurial) {
        repos << MercurialConfig;
    }

    config.writeEntry("autorepository", repos);
}
