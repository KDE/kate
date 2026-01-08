/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectplugin.h"

#include "kateproject.h"
#include "kateprojectconfigpage.h"
#include "kateprojectpluginview.h"
#include "ktexteditor_utils.h"

#include <kateapp.h>

#include <ktexteditor/application.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/view.h>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KNetworkMounts>
#include <KSharedConfig>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMessageBox>
#include <QString>
#include <QTime>
#include <QTimer>

#include <memory_resource>
#include <unordered_set>
#include <vector>

namespace
{
const QString ProjectFileName = QStringLiteral(".kateproject");
const QString GitFolderName = QStringLiteral(".git");
const QString SubversionFolderName = QStringLiteral(".svn");
const QString MercurialFolderName = QStringLiteral(".hg");
const QString FossilCheckoutFileName = QStringLiteral(".fslckout");

const QString GitConfig = QStringLiteral("git");
const QString SubversionConfig = QStringLiteral("subversion");
const QString MercurialConfig = QStringLiteral("mercurial");
const QString FossilConfig = QStringLiteral("fossil");

const QStringList DefaultConfig = QStringList{GitConfig, SubversionConfig, MercurialConfig};
}

KateProjectPlugin::KateProjectPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
    , m_completion(this)
    , m_commands(this)
{
    qRegisterMetaType<KateProjectSharedQStandardItem>("KateProjectSharedQStandardItem");
    qRegisterMetaType<KateProjectSharedQHashStringItem>("KateProjectSharedQHashStringItem");
    qRegisterMetaType<KateProjectSharedProjectIndex>("KateProjectSharedProjectIndex");

    connect(KTextEditor::Editor::instance()->application(), &KTextEditor::Application::documentCreated, this, &KateProjectPlugin::slotDocumentCreated);

    // read configuration prior to cwd project setup below
    readConfig();

    // register all already open documents, later we keep track of all newly created ones
    const auto docs = KTextEditor::Editor::instance()->application()->documents();
    for (auto document : docs) {
        slotDocumentCreated(document);
    }

    // make project plugin variables known to KTextEditor::Editor
    registerVariables();

    // forward to meta-object system friendly version
    connect(this, &KateProjectPlugin::projectCreated, this, &KateProjectPlugin::projectAdded);
    connect(this, &KateProjectPlugin::pluginViewProjectClosing, this, &KateProjectPlugin::projectRemoved);

    /**
     * heuristic:
     * clear our cache of directories without projects when a new project got created
     */
    connect(this, &KateProjectPlugin::projectCreated, this, [this]() {
        m_dirsWithNoProjectsCache.clear();
    });
}

KateProjectPlugin::~KateProjectPlugin()
{
    unregisterVariables();

    for (KateProject *project : std::as_const(m_projects)) {
        delete project;
    }
    m_projects.clear();
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
    // check if we already have the needed project opened
    if (auto project = openProjectForDirectory(QFileInfo(fileName).dir())) {
        return project;
    }

    auto *project = new KateProject(m_threadPool, this, fileName);
    if (!project->isValid()) {
        delete project;
        return nullptr;
    }

    m_projects.append(project);
    Q_EMIT projectCreated(project);
    return project;
}

KateProject *KateProjectPlugin::openProjectForDirectory(const QDir &dir)
{
    // check for project and load it if found
    const QString absolutePath = dir.absolutePath();
    for (KateProject *project : std::as_const(m_projects)) {
        if (project->projectRoots().contains(absolutePath)) {
            return project;
        }
    }
    return nullptr;
}

KateProject *KateProjectPlugin::projectForDir(QDir dir, bool userSpecified)
{
    /**
     * Save dir to create a project from directory if nothing works
     */
    const QDir originalDir = dir;

    /**
     * check our cache if not user triggered, avoids to stat a lot below
     */
    if (!userSpecified && m_dirsWithNoProjectsCache.contains(originalDir.absolutePath())) {
        return nullptr;
    }

    /**
     * search project file upwards
     * with recursion guard
     * do this first for all level and only after this fails try to invent projects
     * otherwise one e.g. invents projects for .kateproject tree structures with sub .git clones
     */
    std::byte buffer[60 * 1000];
    std::pmr::monotonic_buffer_resource memory(buffer, sizeof(buffer));

    std::pmr::unordered_set<QString> seenDirectories(&memory);
    std::pmr::vector<QString> directoryStack(&memory);
    while (!seenDirectories.contains(dir.absolutePath())) {
        // update guard
        seenDirectories.insert(dir.absolutePath());

        // remember directory for later project creation based on other criteria
        directoryStack.push_back(dir.absolutePath());

        // check for project and load it if found
        if (auto project = openProjectForDirectory(dir)) {
            return project;
        }

        // project file found => done
        if (dir.exists(ProjectFileName)) {
            return createProjectForFileName(dir.filePath(ProjectFileName));
        }

        // else: cd up, if possible or abort
        if (!dir.cdUp()) {
            break;
        }
    }

    /**
     * if we arrive here, we found no .kateproject
     * => we want to invent a project based on e.g. version control system info
     */
    for (const QString &dirPath : directoryStack) {
        // try to invent project based on CMake & version control stuff, try CMake first
        KateProject *project = nullptr;
        if ((project = detectCMake(dirPath)) || (project = detectGit(dirPath)) || (project = detectSubversion(dirPath)) || (project = detectMercurial(dirPath))
            || (project = detectFossil(dirPath))) {
            return project;
        }
    }

    /**
     * Version control not found? Load the directory as project
     */
    if (userSpecified) {
        return createProjectForDirectory(originalDir);
    }

    /**
     * Give up
     * remember we failed, we only arrive here for non user triggered auto detection!
     */
    m_dirsWithNoProjectsCache.insert(originalDir.absolutePath());
    return nullptr;
}

void KateProjectPlugin::closeProject(KateProject *project)
{
    // collect all documents we have mapped to the projects we want to close
    // we can not delete projects that still have some mapping
    QList<KTextEditor::Document *> projectDocuments;
    for (const auto &[document, documentProject] : m_document2Project) {
        if (documentProject == project) {
            projectDocuments.append(document);
        }
    }

    // if we have some documents open for this project, ask if we want to close, else just do it
    if (!projectDocuments.isEmpty()) {
        QWidget *window = KTextEditor::Editor::instance()->application()->activeMainWindow()->window();
        const QString title = i18n("Confirm project closing: %1", project->name());
        const QString text = i18n("Do you want to close the project %1 and the related %2 open documents?", project->name(), projectDocuments.size());
        if (QMessageBox::Yes != QMessageBox::question(window, title, text, QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes)) {
            return;
        }

        // best effort document closing, some might survive
        KTextEditor::Editor::instance()->application()->closeDocuments(projectDocuments);
    }

    // check: did all documents of the project go away? if not we shall not close it
    if (!projectHasOpenDocuments(project)) {
        // ensure we no longer can look up this projects in stuff triggered by pluginViewProjectClosing, bug 501103
        m_projects.removeOne(project);

        // cleanup views and Co.
        Q_EMIT pluginViewProjectClosing(project);

        // purge the project, now there shall be no references left
        delete project;

        // try to release memory, see bug 509126
        Utils::releaseMemoryToOperatingSystem();
    }
}

QList<QObject *> KateProjectPlugin::projectsObjects() const
{
    QList<QObject *> list;
    for (auto &p : m_projects) {
        list.push_back(p);
    }
    return list;
}

bool KateProjectPlugin::projectHasOpenDocuments(KateProject *project) const
{
    for (const auto &it : m_document2Project) {
        if (it.second == project) {
            return true;
        }
    }
    return false;
}

KateProject *KateProjectPlugin::projectForUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isLocalFile()
        || KNetworkMounts::self()->isOptionEnabledForPath(url.toLocalFile(), KNetworkMounts::MediumSideEffectsOptimizations)) {
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
    const auto it = m_document2Project.find(static_cast<KTextEditor::Document *>(document));
    if (it == m_document2Project.end()) {
        return;
    }

    it->second->unregisterDocument(static_cast<KTextEditor::Document *>(document));
    m_document2Project.erase(it);
}

void KateProjectPlugin::slotDocumentUrlChanged(KTextEditor::Document *document)
{
    // unregister from old mapping
    slotDocumentDestroyed(document);

    // register for new project, if any
    if (KateProject *project = projectForUrl(document->url())) {
        m_document2Project.emplace(document, project);
        project->registerDocument(document);
    }
}

KateProject *KateProjectPlugin::detectGit(const QDir &dir, const QVariantMap &baseProjectMap)
{
    // allow .git as dir and file (file for git worktree stuff, https://git-scm.com/docs/git-worktree)
    if (m_autoGit && dir.exists(GitFolderName)) {
        return createProjectForRepository(QStringLiteral("git"), dir, baseProjectMap);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::detectSubversion(const QDir &dir, const QVariantMap &baseProjectMap)
{
    if (m_autoSubversion && dir.exists(SubversionFolderName) && QFileInfo(dir, SubversionFolderName).isDir()) {
        return createProjectForRepository(QStringLiteral("svn"), dir, baseProjectMap);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::detectMercurial(const QDir &dir, const QVariantMap &baseProjectMap)
{
    if (m_autoMercurial && dir.exists(MercurialFolderName) && QFileInfo(dir, MercurialFolderName).isDir()) {
        return createProjectForRepository(QStringLiteral("hg"), dir, baseProjectMap);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::detectFossil(const QDir &dir, const QVariantMap &baseProjectMap)
{
    if (m_autoFossil && dir.exists(FossilCheckoutFileName) && QFileInfo(dir, FossilCheckoutFileName).isReadable()) {
        return createProjectForRepository(QStringLiteral("fossil"), dir, baseProjectMap);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::detectCMake(const QDir &dir)
{
    if (m_autoCMake && dir.exists(QStringLiteral("CMakeCache.txt"))) {
        // detect the source dir by fast grepping the file
        static const QByteArray sourceDirPrefix("CMAKE_HOME_DIRECTORY:INTERNAL=");
        QFile cache(dir.absolutePath() + QStringLiteral("/CMakeCache.txt"));
        if (cache.open(QFile::ReadOnly)) {
            while (!cache.atEnd()) {
                auto line = cache.readLine();
                if (!line.startsWith(sourceDirPrefix)) {
                    continue;
                }

                // kill trailing newline stuff
                while (line.back() == QLatin1Char('\n') || line.back() == QLatin1Char('\r')) {
                    line.removeLast();
                }

                // we assume proper UTF-8 encoding
                const QDir sourceDir(QString::fromUtf8(line.mid(sourceDirPrefix.size())));

                // avoid empty dir or non-existing
                if (sourceDir.absolutePath().isEmpty() || !sourceDir.exists()) {
                    return nullptr;
                }

                // add build dir to base project config, we need that to have a terminal there
                QVariantMap cnf, build;
                build[QStringLiteral("directory")] = dir.absolutePath();
                cnf[QStringLiteral("build")] = build;

                // trigger load via other heuristics or explicit directory load
                // mimics behavior of old CMake addon generator
                KateProject *project = nullptr;
                if ((project = detectGit(sourceDir, cnf)) || (project = detectSubversion(sourceDir, cnf)) || (project = detectMercurial(sourceDir, cnf))
                    || (project = detectFossil(sourceDir, cnf)) || (project = createProjectForDirectory(sourceDir, cnf))) {
                    // trigger CMake target load
                    QTimer::singleShot(0, this, [dir]() {
                        if (auto buildPluginView =
                                KTextEditor::Editor::instance()->application()->activeMainWindow()->pluginView(QStringLiteral("katebuildplugin"))) {
                            QMetaObject::invokeMethod(buildPluginView, "loadCMakeTargets", Q_ARG(QString, dir.absolutePath()));
                        }
                    });

                    return project;
                }

                // bad luck
                return nullptr;
            }
        }
    }

    return nullptr;
}

KateProject *KateProjectPlugin::createProjectForRepository(const QString &type, const QDir &dir, const QVariantMap &baseProjectMap)
{
    QVariantMap cnf = baseProjectMap, files;
    files[type] = 1;
    cnf[QStringLiteral("name")] = dir.dirName();
    cnf[QStringLiteral("files")] = (QVariantList() << files);
    return createProjectForDirectoryWithProjectMap(dir, cnf);
}

KateProject *KateProjectPlugin::createProjectForDirectory(const QDir &dir, const QVariantMap &baseProjectMap)
{
    QVariantMap cnf = baseProjectMap, files;
    files[QStringLiteral("directory")] = QStringLiteral("./");
    cnf[QStringLiteral("name")] = dir.dirName();
    cnf[QStringLiteral("files")] = (QVariantList() << files);
    return createProjectForDirectoryWithProjectMap(dir, cnf);
}

KateProject *KateProjectPlugin::createProjectForDirectoryWithProjectMap(const QDir &dir, const QVariantMap &projectMap)
{
    // check if we already have the needed project opened
    if (auto project = openProjectForDirectory(dir)) {
        return project;
    }

    auto *project = new KateProject(m_threadPool, this, projectMap, dir.absolutePath());
    if (!project->isValid()) {
        delete project;
        return nullptr;
    }

    m_projects.append(project);
    Q_EMIT projectCreated(project);
    return project;
}

void KateProjectPlugin::setAutoRepository(bool onGit, bool onSubversion, bool onMercurial, bool onFossil)
{
    m_autoGit = onGit;
    m_autoSubversion = onSubversion;
    m_autoMercurial = onMercurial;
    m_autoFossil = onFossil;
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

bool KateProjectPlugin::autoFossil() const
{
    return m_autoFossil;
}

void KateProjectPlugin::setIndex(bool enabled, const QUrl &directory)
{
    m_indexEnabled = enabled;
    m_indexDirectory = directory;
    writeConfig();
}

bool KateProjectPlugin::getIndexEnabled() const
{
    return m_indexEnabled;
}

QUrl KateProjectPlugin::getIndexDirectory() const
{
    return m_indexDirectory;
}

bool KateProjectPlugin::multiProjectCompletion() const
{
    return m_multiProjectCompletion;
}

bool KateProjectPlugin::multiProjectGoto() const
{
    return m_multiProjectGoto;
}

void KateProjectPlugin::setSingleClickAction(ClickAction cb)
{
    m_singleClickAction = cb;
    writeConfig();
}

ClickAction KateProjectPlugin::singleClickAcion()
{
    return m_singleClickAction;
}

void KateProjectPlugin::setDoubleClickAction(ClickAction cb)
{
    m_doubleClickAction = cb;
    writeConfig();
}

ClickAction KateProjectPlugin::doubleClickAcion()
{
    return m_doubleClickAction;
}

void KateProjectPlugin::setMultiProject(bool completion, bool gotoSymbol)
{
    m_multiProjectCompletion = completion;
    m_multiProjectGoto = gotoSymbol;
    writeConfig();
}

void KateProjectPlugin::setRestoreProjectsForSession(bool enabled)
{
    m_restoreProjectsForSession = enabled;
    writeConfig();
}

bool KateProjectPlugin::restoreProjectsForSession() const
{
    return m_restoreProjectsForSession;
}

void KateProjectPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("project"));

    const QStringList autorepository = config.readEntry("autorepository", DefaultConfig);
    m_autoGit = autorepository.contains(GitConfig);
    m_autoSubversion = autorepository.contains(SubversionConfig);
    m_autoMercurial = autorepository.contains(MercurialConfig);
    m_autoFossil = autorepository.contains(FossilConfig);

    m_autoCMake = config.readEntry("autoCMake", true);

    m_indexEnabled = config.readEntry("index", false);
    m_indexDirectory = config.readEntry("indexDirectory", QUrl());

    m_multiProjectCompletion = config.readEntry("multiProjectCompletion", false);
    m_multiProjectGoto = config.readEntry("multiProjectCompletion", false);

    m_singleClickAction = (ClickAction)config.readEntry("gitStatusSingleClick", (int)ClickAction::NoAction);
    m_doubleClickAction = (ClickAction)config.readEntry("gitStatusDoubleClick", (int)ClickAction::StageUnstage);

    m_restoreProjectsForSession = config.readEntry("restoreProjectsForSessions", false);

    Q_EMIT configUpdated();
}

void KateProjectPlugin::writeConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("project"));
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

    if (m_autoFossil) {
        repos << FossilConfig;
    }

    config.writeEntry("autorepository", repos);

    config.writeEntry("autoCMake", m_autoCMake);

    config.writeEntry("index", m_indexEnabled);
    config.writeEntry("indexDirectory", m_indexDirectory);

    config.writeEntry("multiProjectCompletion", m_multiProjectCompletion);
    config.writeEntry("multiProjectGoto", m_multiProjectGoto);

    config.writeEntry("gitStatusSingleClick", (int)m_singleClickAction);
    config.writeEntry("gitStatusDoubleClick", (int)m_doubleClickAction);

    config.writeEntry("restoreProjectsForSessions", m_restoreProjectsForSession);

    Q_EMIT configUpdated();
}

static KateProjectPlugin *findProjectPlugin()
{
    auto plugin = KTextEditor::Editor::instance()->application()->plugin(QStringLiteral("kateprojectplugin"));
    return qobject_cast<KateProjectPlugin *>(plugin);
}

void KateProjectPlugin::registerVariables()
{
    auto editor = KTextEditor::Editor::instance();
    editor->registerVariableMatch(QStringLiteral("Project:Path"),
                                  i18n("Full path to current project excluding the file name."),
                                  [](const QStringView &, KTextEditor::View *view) -> QString {
                                      if (!view) {
                                          return {};
                                      }
                                      auto projectPlugin = findProjectPlugin();
                                      if (!projectPlugin) {
                                          return {};
                                      }
                                      auto kateProject = findProjectPlugin()->projectForUrl(view->document()->url());
                                      if (!kateProject) {
                                          return {};
                                      }
                                      return QDir(kateProject->baseDir()).absolutePath();
                                  });

    editor->registerVariableMatch(QStringLiteral("Project:NativePath"),
                                  i18n("Full path to current project excluding the file name, with native path separator (backslash on Windows)."),
                                  [](const QStringView &, KTextEditor::View *view) -> QString {
                                      if (!view) {
                                          return {};
                                      }
                                      auto projectPlugin = findProjectPlugin();
                                      if (!projectPlugin) {
                                          return {};
                                      }
                                      auto kateProject = findProjectPlugin()->projectForUrl(view->document()->url());
                                      if (!kateProject) {
                                          return {};
                                      }
                                      return QDir::toNativeSeparators(QDir(kateProject->baseDir()).absolutePath());
                                  });
}
void KateProjectPlugin::unregisterVariables()
{
    auto editor = KTextEditor::Editor::instance();
    editor->unregisterVariable(QStringLiteral("Project:Path"));
    editor->unregisterVariable(QStringLiteral("Project:NativePath"));
}

void KateProjectPlugin::readSessionConfig(const KConfigGroup &config)
{
    // de-serialize all open projects as list of JSON documents if allowed
    if (restoreProjectsForSession()) {
        const auto projectList = config.readEntry("projects", QStringList());
        for (const auto &project : projectList) {
            const QVariantMap sMap = QJsonDocument::fromJson(project.toUtf8()).toVariant().toMap();

            // valid file backed project?
            if (const auto file = sMap[QStringLiteral("file")].toString(); !file.isEmpty() && QFileInfo::exists(file)) {
                createProjectForFileName(file);
                continue;
            }

            // valid path + data project?
            if (const auto path = sMap[QStringLiteral("path")].toString(); !path.isEmpty() && QFileInfo::exists(path)) {
                createProjectForDirectoryWithProjectMap(QDir(path), sMap[QStringLiteral("data")].toMap());
                continue;
            }

            // we might arrive here if invalid data is store, just ignore that, we just loose session data
        }
    }

    // always load projects from command line or current working directory first time we arrive here
    if (m_initialReadSessionConfigDone) {
        return;
    }
    m_initialReadSessionConfigDone = true;

    /**
     * delayed activation after session restore
     * we do this both for session restoration to not take preference and
     * to be able to signal errors during project loading via message() signals
     */
    KateProject *projectToActivate = nullptr;

    // open directories as projects
    auto args = qApp->arguments();
    args.removeFirst(); // The first argument is the executable name
    for (const QString &arg : std::as_const(args)) {
        QFileInfo info(arg);
        if (info.isDir()) {
            projectToActivate = projectForDir(info.absoluteFilePath(), true);
        }
    }

    /**
     * open project for our current working directory, if this kate has a terminal
     */
    if (!projectToActivate && KateApp::isInsideTerminal()) {
        projectToActivate = projectForDir(QDir::current());
    }

    // if we have some project opened, ensure it is the active one, this happens after session restore
    if (projectToActivate) {
        // delay this to ensure main windows are already there
        QTimer::singleShot(0, projectToActivate, [projectToActivate]() {
            if (auto pluginView = KTextEditor::Editor::instance()->application()->activeMainWindow()->pluginView(QStringLiteral("kateprojectplugin"))) {
                static_cast<KateProjectPluginView *>(pluginView)->openProject(projectToActivate);
            }
        });
    }
}

void KateProjectPlugin::writeSessionConfig(KConfigGroup &config)
{
    // serialize all open projects as list of JSON documents if allowed, always write the list to not leave over old data forever
    QStringList projectList;
    if (restoreProjectsForSession()) {
        const auto projectsList = projects();
        for (const auto project : projectsList) {
            QVariantMap sMap;

            // for file backed stuff, we just remember the file
            if (project->isFileBacked()) {
                sMap[QStringLiteral("file")] = project->fileName();
            }

            // otherwise we remember the data we generated purely in memory
            else {
                sMap[QStringLiteral("data")] = project->projectMap();
                sMap[QStringLiteral("path")] = project->baseDir();
            }

            // encode as one-lines JSON string
            projectList.push_back(QString::fromUtf8(QJsonDocument::fromVariant(QVariant(sMap)).toJson(QJsonDocument::Compact)));
        }
    }
    config.writeEntry("projects", projectList);
}

void KateProjectPlugin::sendMessage(const QString &text, bool error)
{
    const auto icon = QIcon::fromTheme(QStringLiteral("project-open"));
    Utils::showMessage(text, icon, i18n("Project"), error ? MessageType::Error : MessageType::Info);
}

QString KateProjectPlugin::projectBaseDirForDocument(KTextEditor::Document *doc)
{
    // quick lookup first, then search
    auto project = projectForDocument(doc);
    if (!project) {
        project = projectForUrl(doc->url());
    }
    return project ? project->baseDir() : QString();
}

QVariantMap KateProjectPlugin::projectMapForDocument(KTextEditor::Document *doc)
{
    // quick lookup first, then search
    auto project = projectForDocument(doc);
    if (!project) {
        project = projectForUrl(doc->url());
    }
    return project ? project->projectMap() : QVariantMap();
}

#include "moc_kateprojectplugin.cpp"
