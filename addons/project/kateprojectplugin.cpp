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
#include "kateprojectplugin.moc"

#include "kateproject.h"
#include "kateprojectpluginview.h"

#include <ktexteditor/editor.h>
#include <ktexteditor/application.h>
#include <ktexteditor/document.h>

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
const QString ProjectFileName = QLatin1String(".kateproject");
}

KateProjectPlugin::KateProjectPlugin(QObject *parent, const QList<QVariant>&)
    : KTextEditor::Plugin(parent)
    , m_completion(this)
{
    qRegisterMetaType<KateProjectSharedQStandardItem>("KateProjectSharedQStandardItem");
    qRegisterMetaType<KateProjectSharedQMapStringItem>("KateProjectSharedQMapStringItem");
    qRegisterMetaType<KateProjectSharedProjectIndex>("KateProjectSharedProjectIndex");

    connect(KTextEditor::Editor::instance()->application(), SIGNAL(documentCreated(KTextEditor::Document *)), this, SLOT(slotDocumentCreated(KTextEditor::Document *)));
    connect(&m_fileWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(slotDirectoryChanged(const QString &)));

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

    foreach (KTextEditor::Document *document, KTextEditor::Editor::instance()->application()->documents()) {
        slotDocumentCreated(document);
    }
}

KateProjectPlugin::~KateProjectPlugin()
{
    Q_FOREACH (KateProject *project, m_projects) {
        m_fileWatcher.removePath(QFileInfo(project->fileName()).canonicalPath());
        delete project;
    }
    m_projects.clear();
}

QObject *KateProjectPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new KateProjectPluginView(this, mainWindow);
}

KateProject *KateProjectPlugin::createProjectForFileName(const QString &fileName)
{
    KateProject *project = new KateProject();

    if (!project->load(fileName)) {
        delete project;
        return 0;
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

        Q_FOREACH(KateProject *project, m_projects) {
            if (project->baseDir() == canonicalPath || project->fileName() == canonicalFileName) {
                return project;
            }
        }

        if (dir.exists(ProjectFileName)) {
            return createProjectForFileName(canonicalFileName);
        }

        /**
         * else: cd up, if possible or abort
         */
        if (!dir.cdUp()) {
            break;
        }
    }

    return 0;
}

KateProject* KateProjectPlugin::projectForUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isLocalFile()) {
        return 0;
    }

    return projectForDir(QFileInfo(url.toLocalFile()).absoluteDir());
}

void KateProjectPlugin::slotDocumentCreated(KTextEditor::Document *document)
{
    connect(document, SIGNAL(documentUrlChanged(KTextEditor::Document *)), this, SLOT(slotDocumentUrlChanged(KTextEditor::Document *)));
    connect(document, SIGNAL(destroyed(QObject *)), this, SLOT(slotDocumentDestroyed(QObject *)));

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
    Q_FOREACH(KateProject *project, m_projects) {
        if (project->fileName() == fileName) {
            project->reload();
            break;
        }
    }
}
