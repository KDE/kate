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

#include <kcoreaddons_version.h>
#include <ktexteditor/application.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KNetworkMounts>
#include <KSharedConfig>

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMessageBox>
#include <QString>
#include <QTime>
#include <QTimer>

#include <vector>

#ifdef HAVE_CTERMID
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#endif

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

const QStringList DefaultConfig = QStringList() << GitConfig << SubversionConfig << MercurialConfig;
}

KateProjectPlugin::KateProjectPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
    , m_completion(this)
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
}

KateProjectPlugin::~KateProjectPlugin()
{
    unregisterVariables();

    for (KateProject *project : qAsConst(m_projects)) {
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

    KateProject *project = new KateProject(m_threadPool, this, fileName);
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
    const QString canonicalPath = dir.canonicalPath();
    const QString canonicalFileName = dir.filePath(ProjectFileName);
    for (KateProject *project : qAsConst(m_projects)) {
        if (project->baseDir() == canonicalPath || project->fileName() == canonicalFileName) {
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
     * search project file upwards
     * with recursion guard
     * do this first for all level and only after this fails try to invent projects
     * otherwise one e.g. invents projects for .kateproject tree structures with sub .git clones
     */
    QSet<QString> seenDirectories;
    std::vector<QString> directoryStack;
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
    for (const QString &dir : directoryStack) {
        // try to invent project based on version control stuff
        KateProject *project = nullptr;
        if ((project = detectGit(dir)) || (project = detectSubversion(dir)) || (project = detectMercurial(dir)) || (project = detectFossil(dir))) {
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
     */
    return nullptr;
}

void KateProjectPlugin::closeProject(KateProject *project)
{
    // collect all documents we have mapped to the projects we want to close
    // we can not delete projects that still have some mapping
    QList<KTextEditor::Document *> projectDocuments;
    for (const auto &it : m_document2Project) {
        if (it.second == project) {
            projectDocuments.append(it.first);
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
        Q_EMIT pluginViewProjectClosing(project);
        m_projects.removeOne(project);
        delete project;
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

KateProject *KateProjectPlugin::detectGit(const QDir &dir)
{
    // allow .git as dir and file (file for git worktree stuff, https://git-scm.com/docs/git-worktree)
    if (m_autoGit && dir.exists(GitFolderName)) {
        return createProjectForRepository(QStringLiteral("git"), dir);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::detectSubversion(const QDir &dir)
{
    if (m_autoSubversion && dir.exists(SubversionFolderName) && QFileInfo(dir, SubversionFolderName).isDir()) {
        return createProjectForRepository(QStringLiteral("svn"), dir);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::detectMercurial(const QDir &dir)
{
    if (m_autoMercurial && dir.exists(MercurialFolderName) && QFileInfo(dir, MercurialFolderName).isDir()) {
        return createProjectForRepository(QStringLiteral("hg"), dir);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::detectFossil(const QDir &dir)
{
    if (m_autoFossil && dir.exists(FossilCheckoutFileName) && QFileInfo(dir, FossilCheckoutFileName).isReadable()) {
        return createProjectForRepository(QStringLiteral("fossil"), dir);
    }

    return nullptr;
}

KateProject *KateProjectPlugin::createProjectForRepository(const QString &type, const QDir &dir)
{
    // check if we already have the needed project opened
    if (auto project = openProjectForDirectory(dir)) {
        return project;
    }

    QVariantMap cnf, files;
    files[type] = 1;
    cnf[QStringLiteral("name")] = dir.dirName();
    cnf[QStringLiteral("files")] = (QVariantList() << files);

    KateProject *project = new KateProject(m_threadPool, this, cnf, dir.canonicalPath());

    m_projects.append(project);

    Q_EMIT projectCreated(project);
    return project;
}

KateProject *KateProjectPlugin::createProjectForDirectory(const QDir &dir)
{
    // check if we already have the needed project opened
    if (auto project = openProjectForDirectory(dir)) {
        return project;
    }

    QVariantMap cnf, files;
    files[QStringLiteral("directory")] = QStringLiteral("./");
    cnf[QStringLiteral("name")] = dir.dirName();
    cnf[QStringLiteral("files")] = (QVariantList() << files);

    KateProject *project = new KateProject(m_threadPool, this, cnf, dir.canonicalPath());

    m_projects.append(project);

    Q_EMIT projectCreated(project);
    return project;
}

KateProject *KateProjectPlugin::createProjectForDirectory(const QDir &dir, const QVariantMap &projectMap)
{
    // check if we already have the needed project opened
    if (auto project = openProjectForDirectory(dir)) {
        return project;
    }

    KateProject *project = new KateProject(m_threadPool, this, projectMap, dir.canonicalPath());
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

void KateProjectPlugin::setGitStatusShowNumStat(bool show)
{
    m_gitNumStat = show;
    writeConfig();
}

bool KateProjectPlugin::showGitStatusWithNumStat() const
{
    return m_gitNumStat;
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
    KConfigGroup config(KSharedConfig::openConfig(), "project");

    const QStringList autorepository = config.readEntry("autorepository", DefaultConfig);
    m_autoGit = autorepository.contains(GitConfig);
    m_autoSubversion = autorepository.contains(SubversionConfig);
    m_autoMercurial = autorepository.contains(MercurialConfig);
    m_autoFossil = autorepository.contains(FossilConfig);

    m_indexEnabled = config.readEntry("index", false);
    m_indexDirectory = config.readEntry("indexDirectory", QUrl());

    m_multiProjectCompletion = config.readEntry("multiProjectCompletion", false);
    m_multiProjectGoto = config.readEntry("multiProjectCompletion", false);

    m_gitNumStat = config.readEntry("gitStatusNumStat", true);
    m_singleClickAction = (ClickAction)config.readEntry("gitStatusSingleClick", (int)ClickAction::NoAction);
    m_doubleClickAction = (ClickAction)config.readEntry("gitStatusDoubleClick", (int)ClickAction::StageUnstage);

    m_restoreProjectsForSession = config.readEntry("restoreProjectsForSessions", false);

    Q_EMIT configUpdated();
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

    if (m_autoFossil) {
        repos << FossilConfig;
    }

    config.writeEntry("autorepository", repos);

    config.writeEntry("index", m_indexEnabled);
    config.writeEntry("indexDirectory", m_indexDirectory);

    config.writeEntry("multiProjectCompletion", m_multiProjectCompletion);
    config.writeEntry("multiProjectGoto", m_multiProjectGoto);

    config.writeEntry("gitStatusNumStat", m_gitNumStat);
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
                                  [](const QStringView &, KTextEditor::View *view) {
                                      if (!view) {
                                          return QString();
                                      }
                                      auto projectPlugin = findProjectPlugin();
                                      if (!projectPlugin) {
                                          return QString();
                                      }
                                      auto kateProject = findProjectPlugin()->projectForUrl(view->document()->url());
                                      if (!kateProject) {
                                          return QString();
                                      }
                                      return QDir(kateProject->baseDir()).absolutePath();
                                  });

    editor->registerVariableMatch(QStringLiteral("Project:NativePath"),
                                  i18n("Full path to current project excluding the file name, with native path separator (backslash on Windows)."),
                                  [](const QStringView &, KTextEditor::View *view) {
                                      if (!view) {
                                          return QString();
                                      }
                                      auto projectPlugin = findProjectPlugin();
                                      if (!projectPlugin) {
                                          return QString();
                                      }
                                      auto kateProject = findProjectPlugin()->projectForUrl(view->document()->url());
                                      if (!kateProject) {
                                          return QString();
                                      }
                                      return QDir::toNativeSeparators(QDir(kateProject->baseDir()).absolutePath());
                                  });
}
void KateProjectPlugin::unregisterVariables()
{
    auto editor = KTextEditor::Editor::instance();
    editor->unregisterVariableMatch(QStringLiteral("Project:Path"));
    editor->unregisterVariableMatch(QStringLiteral("Project:NativePath"));
}

void KateProjectPlugin::readSessionConfig(const KConfigGroup &config)
{
    // de-serialize all open projects as list of JSON documents if allowed
    if (restoreProjectsForSession()) {
        const auto projectList = config.readEntry("projects", QStringList());
        for (const auto &project : projectList) {
            const QVariantMap sMap = QJsonDocument::fromJson(project.toUtf8()).toVariant().toMap();

            // valid file backed project?
            if (const auto file = sMap[QStringLiteral("file")].toString(); !file.isEmpty() && QFileInfo(file).exists()) {
                createProjectForFileName(file);
                continue;
            }

            // valid path + data project?
            if (const auto path = sMap[QStringLiteral("path")].toString(); !path.isEmpty() && QFileInfo(path).exists()) {
                createProjectForDirectory(QDir(path), sMap[QStringLiteral("data")].toMap());
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
    for (const QString &arg : qAsConst(args)) {
        QFileInfo info(arg);
        if (info.isDir()) {
            projectToActivate = projectForDir(info.absoluteFilePath(), true);
        }
    }

#ifdef HAVE_CTERMID
    /**
     * open project for our current working directory, if this kate has a terminal
     * https://stackoverflow.com/questions/1312922/detect-if-stdin-is-a-terminal-or-pipe-in-c-c-qt
     */
    if (!projectToActivate) {
        char tty[L_ctermid + 1] = {0};
        ctermid(tty);
        if (int fd = ::open(tty, O_RDONLY); fd >= 0) {
            projectToActivate = projectForDir(QDir::current());
            ::close(fd);
        }
    }
#endif

    // if we have some project opened, ensure it is the active one, this happens after session restore
    if (projectToActivate) {
        Q_EMIT activateProject(projectToActivate);
    }
}

void KateProjectPlugin::writeSessionConfig(KConfigGroup &config)
{
    // serialize all open projects as list of JSON documents if allowed, always write the list to not leave over old data forever
    QStringList projectList;
    if (restoreProjectsForSession()) {
        for (const auto project : projects()) {
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
    QVariantMap genericMessage;
    genericMessage.insert(QStringLiteral("type"), error ? QStringLiteral("Error") : QStringLiteral("Info"));
    genericMessage.insert(QStringLiteral("category"), i18n("Project"));
    genericMessage.insert(QStringLiteral("categoryIcon"), QIcon::fromTheme(QStringLiteral("project-open")));
    genericMessage.insert(QStringLiteral("text"), text);
    Utils::showMessage(genericMessage);
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
