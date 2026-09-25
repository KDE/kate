/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "deploymentplugin.h"

#include "ktexteditor_utils.h"

#include <KActionCollection>
#include <KDirOperator>
#include <KDirLister>
#include <KFileItem>
#include <KIO/DeleteJob>
#include <KIO/FileCopyJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/ListJob>
#include <KIO/MkpathJob>
#include <KIO/SimpleJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>
#include <KProtocolInfo>
#include <KProtocolManager>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KUrlNavigator>
#include <KXMLGUIFactory>

#include <QAction>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSet>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QToolBar>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>

K_PLUGIN_FACTORY_WITH_JSON(DeploymentPluginFactory, "plugin.json",
                           registerPlugin<DeploymentPlugin>();)

namespace {
struct ConfigFields {
  QString host;
  int port = 22;
  QString user;
  QString localRoot = QStringLiteral(".");
  QString remoteRoot;
  QStringList exclude;
};

ConfigFields fieldsFromMap(const QVariantMap &projectMap) {
  const QVariantMap deployment =
      projectMap.value(QStringLiteral("deployment")).toMap();
  ConfigFields fields;
  fields.host = deployment.value(QStringLiteral("host")).toString();
  fields.port = deployment.value(QStringLiteral("port"), 22).toInt();
  fields.user = deployment.value(QStringLiteral("user")).toString();
  fields.localRoot =
      deployment.value(QStringLiteral("localRoot"), QStringLiteral("."))
          .toString();
  fields.remoteRoot = deployment.value(QStringLiteral("remoteRoot")).toString();
  const QVariant exclude = deployment.value(QStringLiteral("exclude"));
  if (exclude.metaType().id() == QMetaType::QStringList) {
    fields.exclude = exclude.toStringList();
  } else {
    for (const QVariant &pattern : exclude.toList()) {
      fields.exclude.push_back(pattern.toString());
    }
  }
  return fields;
}

QVariantMap deploymentMap(const ConfigFields &fields) {
  return {
      {QStringLiteral("host"), fields.host.trimmed()},
      {QStringLiteral("port"), fields.port},
      {QStringLiteral("user"), fields.user},
      {QStringLiteral("localRoot"), fields.localRoot.trimmed()},
      {QStringLiteral("remoteRoot"), fields.remoteRoot.trimmed()},
      {QStringLiteral("exclude"), fields.exclude},
  };
}

bool editConfig(QWidget *parent, const QString &projectBase,
                const QVariantMap &projectMap, ConfigFields &fields) {
  QDialog dialog(parent);
  dialog.setWindowTitle(i18nc("@title:window", "Configure Deployment"));
  dialog.resize(520, 360);

  auto *layout = new QVBoxLayout(&dialog);
  auto *intro = new QLabel(
      i18n("Configure the SFTP destination for <filename>%1</filename>. "
           "Authentication is handled by KIO.",
           projectBase),
      &dialog);
  intro->setWordWrap(true);
  layout->addWidget(intro);

  auto *form = new QFormLayout;
  auto *host = new QLineEdit(fields.host, &dialog);
  auto *port = new QSpinBox(&dialog);
  port->setRange(1, 65535);
  port->setValue(fields.port);
  auto *user = new QLineEdit(fields.user, &dialog);
  auto *localRoot = new QLineEdit(fields.localRoot, &dialog);
  auto *remoteRoot = new QLineEdit(fields.remoteRoot, &dialog);
  auto *exclude = new QPlainTextEdit(&dialog);
  exclude->setPlainText(fields.exclude.join(u'\n'));
  exclude->setPlaceholderText(i18n("One project-relative glob per line"));

  form->addRow(i18nc("@label:textbox", "Host:"), host);
  form->addRow(i18nc("@label:spinbox", "Port:"), port);
  form->addRow(i18nc("@label:textbox", "User:"), user);
  form->addRow(i18nc("@label:textbox", "Local root:"), localRoot);
  form->addRow(i18nc("@label:textbox", "Remote root:"), remoteRoot);
  form->addRow(i18nc("@label:textbox", "Exclude:"), exclude);
  layout->addLayout(form);

  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog,
                   &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);
  layout->addWidget(buttons);

  while (dialog.exec() == QDialog::Accepted) {
    ConfigFields candidate;
    candidate.host = host->text();
    candidate.port = port->value();
    candidate.user = user->text();
    candidate.localRoot = localRoot->text();
    candidate.remoteRoot = remoteRoot->text();
    candidate.exclude = exclude->toPlainText().split(u'\n', Qt::SkipEmptyParts);
    for (QString &pattern : candidate.exclude) {
      pattern = pattern.trimmed();
    }

    QVariantMap candidateProject = projectMap;
    candidateProject.insert(QStringLiteral("deployment"),
                            deploymentMap(candidate));
    const auto config =
        Deployment::Config::fromProjectMap(candidateProject, projectBase);
    if (config.valid && config.enabled) {
      fields = std::move(candidate);
      return true;
    }
    KMessageBox::error(
        &dialog, config.error,
        i18nc("@title:window", "Invalid Deployment Configuration"));
  }
  return false;
}

QString displayUrl(const QUrl &url) {
  return url.toDisplayString(QUrl::RemovePassword);
}

bool sameRemoteFile(const QUrl &left, const QUrl &right) {
  return left.scheme().compare(right.scheme(), Qt::CaseInsensitive) == 0 &&
         left.host().compare(right.host(), Qt::CaseInsensitive) == 0 &&
         left.port(22) == right.port(22) && left.userName() == right.userName() &&
         QDir::cleanPath(left.path()) == QDir::cleanPath(right.path());
}

bool hasOpenRemoteDocument(const QUrl &url) {
  const auto documents =
      KTextEditor::Editor::instance()->application()->documents();
  return std::any_of(documents.cbegin(), documents.cend(), [&url](const auto *doc) {
    return !doc->url().isLocalFile() && sameRemoteFile(doc->url(), url);
  });
}
} // namespace

DeploymentPlugin::DeploymentPlugin(QObject *parent)
    : KTextEditor::Plugin(parent) {}

QObject *DeploymentPlugin::createView(KTextEditor::MainWindow *mainWindow) {
  return new DeploymentView(this, mainWindow);
}

DeploymentView::DeploymentView(DeploymentPlugin *plugin,
                               KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow), m_plugin(plugin), m_mainWindow(mainWindow) {
  setComponentName(QStringLiteral("katedeployment"), i18n("Deployment"));
  setXMLFile(QStringLiteral("ui.rc"));

  m_uploadCurrentAction =
      actionCollection()->addAction(QStringLiteral("deployment_upload_current"),
                                    this, &DeploymentView::uploadCurrentFile);
  m_uploadCurrentAction->setText(i18nc("@action", "Upload Current File"));
  m_uploadCurrentAction->setIcon(
      QIcon::fromTheme(QStringLiteral("document-send")));
  KActionCollection::setDefaultShortcut(
      m_uploadCurrentAction, QKeySequence(QStringLiteral("Ctrl+Alt+S"),
                                          QKeySequence::PortableText));

  m_uploadProjectAction =
      actionCollection()->addAction(QStringLiteral("deployment_upload_project"),
                                    this, &DeploymentView::uploadProject);
  m_uploadProjectAction->setText(i18nc("@action", "Upload Project"));
  m_uploadProjectAction->setIcon(
      QIcon::fromTheme(QStringLiteral("folder-upload")));

  m_cancelAction =
      actionCollection()->addAction(QStringLiteral("deployment_cancel"), this,
                                    &DeploymentView::cancelTransfers);
  m_cancelAction->setText(i18nc("@action", "Cancel Deployment Transfers"));
  m_cancelAction->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));

  m_configureAction = actionCollection()->addAction(
      QStringLiteral("deployment_configure"), this, &DeploymentView::configure);
  m_configureAction->setText(i18nc("@action", "Configure Deployment..."));
  m_configureAction->setIcon(QIcon::fromTheme(QStringLiteral("configure")));

  auto *browseRemoteAction = actionCollection()->addAction(
      QStringLiteral("deployment_browse_remote"));
  browseRemoteAction->setText(i18nc("@action", "Browse Remote Host"));
  browseRemoteAction->setIcon(
      QIcon::fromTheme(QStringLiteral("folder-remote")));

  m_remoteToolView = mainWindow->createToolView(
      plugin, QStringLiteral("katedeploymentremotehost"),
      KTextEditor::MainWindow::Left,
      QIcon::fromTheme(QStringLiteral("folder-remote")), i18n("Remote Host"));
  auto *remoteContainer = new QWidget(m_remoteToolView);
  m_remoteToolView->layout()->addWidget(remoteContainer);
  auto *remoteLayout = new QVBoxLayout(remoteContainer);
  remoteLayout->setContentsMargins(0, 0, 0, 0);
  remoteLayout->setSpacing(0);
  auto *remoteToolbar = new QToolBar(m_remoteToolView);
  remoteToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  remoteLayout->addWidget(remoteToolbar);
  m_remoteNavigator = new KUrlNavigator(nullptr, QUrl(), m_remoteToolView);
  m_remoteNavigator->setSupportedSchemes({QStringLiteral("sftp")});
  remoteLayout->addWidget(m_remoteNavigator);
  m_remoteOperator = new KDirOperator(QUrl(), m_remoteToolView);
  m_remoteOperator->dirLister()->setMainWindow(m_mainWindow->window());
  m_remoteOperator->setSupportedSchemes({QStringLiteral("sftp")});
  m_remoteOperator->setViewMode(KFile::Tree);
  m_remoteOperator->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Expanding);
  m_remoteStatus = new QLabel(m_remoteToolView);
  m_remoteStatus->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  m_remoteStatus->setMargin(6);
  remoteLayout->addWidget(m_remoteStatus);
  remoteLayout->addWidget(m_remoteOperator, 1);

  auto *homeAction =
      remoteToolbar->addAction(QIcon::fromTheme(QStringLiteral("go-home")),
                               i18nc("@action", "Deployment Root"));
  connect(homeAction, &QAction::triggered, this,
          [this] { navigateRemote(m_config.remoteBaseUrl()); });
  remoteToolbar->addAction(m_remoteOperator->action(KDirOperator::Back));
  remoteToolbar->addAction(m_remoteOperator->action(KDirOperator::Forward));
  remoteToolbar->addAction(m_remoteOperator->action(KDirOperator::Up));
  remoteToolbar->addAction(m_remoteOperator->action(KDirOperator::Reload));
  remoteToolbar->addAction(
      m_remoteOperator->action(KDirOperator::ShowHiddenFiles));

  m_transferToolView = mainWindow->createToolView(
      plugin, QStringLiteral("katedeploymenttransfers"),
      KTextEditor::MainWindow::Bottom,
      QIcon::fromTheme(QStringLiteral("document-send")), i18n("File Transfer"));
  auto *transferContainer = new QWidget(m_transferToolView);
  m_transferToolView->layout()->addWidget(transferContainer);
  auto *transferLayout = new QVBoxLayout(transferContainer);
  transferLayout->setContentsMargins(0, 0, 0, 0);
  auto *transferButtons = new QHBoxLayout;
  auto *cancelButton =
      new QPushButton(QIcon::fromTheme(QStringLiteral("process-stop")),
                      i18nc("@action:button", "Cancel"), m_transferToolView);
  auto *clearButton =
      new QPushButton(QIcon::fromTheme(QStringLiteral("edit-clear-all")),
                      i18nc("@action:button", "Clear"), m_transferToolView);
  transferButtons->addWidget(cancelButton);
  transferButtons->addWidget(clearButton);
  transferButtons->addStretch();
  transferLayout->addLayout(transferButtons);
  m_transferLog = new QPlainTextEdit(m_transferToolView);
  m_transferLog->setReadOnly(true);
  transferLayout->addWidget(m_transferLog);
  connect(cancelButton, &QPushButton::clicked, this,
          &DeploymentView::cancelTransfers);
  connect(clearButton, &QPushButton::clicked, m_transferLog,
          &QPlainTextEdit::clear);
  connect(browseRemoteAction, &QAction::triggered, this,
          [this] {
            m_mainWindow->showToolView(m_remoteToolView);
            if (m_config.enabled && m_config.valid) {
              navigateRemote(m_config.remoteBaseUrl());
              m_remoteOperator->rereadDir();
            }
          });

  connect(mainWindow, &KTextEditor::MainWindow::viewChanged, this,
          &DeploymentView::updateContext);
  connect(mainWindow, &KTextEditor::MainWindow::pluginViewCreated, this,
          [this](const QString &name, QObject *projectView) {
            if (name == QLatin1String("kateprojectplugin")) {
              connect(projectView, SIGNAL(projectMapChanged()), this,
                      SLOT(updateContext()), Qt::UniqueConnection);
              connect(projectView, SIGNAL(projectMapEdited()), this,
                      SLOT(updateContext()), Qt::UniqueConnection);
              connect(projectView, SIGNAL(projectFilesChanged()), this,
                      SLOT(updateContext()), Qt::UniqueConnection);
              connect(projectView,
                      SIGNAL(projectTreeContextMenuAboutToShow(
                          QMenu *, QString, QString, int)),
                      this,
                      SLOT(projectTreeContextMenuAboutToShow(
                          QMenu *, QString, QString, int)),
                      Qt::UniqueConnection);
              updateContext();
            }
          });
  connect(m_remoteNavigator, &KUrlNavigator::urlChanged, this,
          &DeploymentView::navigateRemote);
  connect(m_remoteOperator, &KDirOperator::urlEntered, this,
          &DeploymentView::navigateRemote);
  connect(m_remoteOperator, &KDirOperator::fileSelected, this,
          [this](const KFileItem &item) {
            if (!item.isDir() && isRemoteUrlAllowed(item.url())) {
              m_mainWindow->openUrl(item.url());
            }
          });
  connect(m_remoteOperator, &KDirOperator::contextMenuAboutToShow, this,
          &DeploymentView::remoteContextMenu);
  connect(m_remoteOperator->dirLister(), &KCoreDirLister::started, this,
          [this](const QUrl &url) {
            const QString message =
                i18n("Loading %1...", displayUrl(url));
            setRemoteStatus(message);
            appendLog(message);
          });
  connect(m_remoteOperator->dirLister(),
          &KCoreDirLister::listingDirCompleted, this,
          [this](const QUrl &url) {
            setRemoteStatus(i18n("Connected to %1", displayUrl(url)));
          });
  connect(m_remoteOperator->dirLister(), &KCoreDirLister::jobError, this,
          [this](KIO::Job *job) {
            QUrl failedUrl;
            if (auto *listJob = qobject_cast<KIO::ListJob *>(job)) {
              failedUrl = listJob->url();
            }
            const QString message = i18n(
                "Failed to list %1 (KIO error %2): %3",
                failedUrl.isEmpty() ? displayUrl(m_remoteOperator->url())
                                    : displayUrl(failedUrl),
                job->error(), job->errorString());
            setRemoteStatus(message, true);
            appendLog(message);
            m_mainWindow->showToolView(m_transferToolView);
          });

  if (QObject *projectView =
          m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"))) {
    connect(projectView, SIGNAL(projectMapChanged()), this,
            SLOT(updateContext()), Qt::UniqueConnection);
    connect(projectView, SIGNAL(projectMapEdited()), this,
            SLOT(updateContext()), Qt::UniqueConnection);
    connect(projectView, SIGNAL(projectFilesChanged()), this,
            SLOT(updateContext()), Qt::UniqueConnection);
    connect(projectView,
            SIGNAL(projectTreeContextMenuAboutToShow(QMenu *, QString, QString,
                                                     int)),
            this,
            SLOT(projectTreeContextMenuAboutToShow(QMenu *, QString, QString,
                                                   int)),
            Qt::UniqueConnection);
  }

  m_mainWindow->guiFactory()->addClient(this);
  updateContext();
}

DeploymentView::~DeploymentView() {
  m_cancelled = true;
  m_pendingFiles.clear();
  if (!m_activeJob || m_activeJob->kill(KJob::Quietly)) {
    cleanupTemporaryFile();
  }
  m_mainWindow->guiFactory()->removeClient(this);
  delete m_remoteToolView;
  delete m_transferToolView;
}

DeploymentView::ProjectContext DeploymentView::currentProjectContext() const {
  ProjectContext context;
  KTextEditor::View *view = m_mainWindow->activeView();
  if (view && view->document()->url().isLocalFile()) {
    context.baseDir = Utils::projectBaseDirForDocument(view->document());
    context.projectMap = Utils::projectMapForDocument(view->document());
  }

  if (QObject *projectView =
          m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"))) {
    const QString selectedBase =
        projectView->property("projectBaseDir").toString();
    if (context.baseDir.isEmpty()) {
      context.baseDir = selectedBase;
      context.projectMap = projectView->property("projectMap").toMap();
    }
    if (selectedBase == context.baseDir) {
      context.projectFileName =
          projectView->property("projectFileName").toString();
      context.projectLocalConfigFileName =
          projectView->property("projectLocalConfigFileName").toString();
      context.files = projectView->property("projectFiles").toStringList();
    }
  }
  return context;
}

void DeploymentView::updateContext() {
  applyContext(currentProjectContext());
}

void DeploymentView::projectTreeContextMenuAboutToShow(
    QMenu *menu, const QString &path, const QString &projectBaseDir,
    int itemType) {
  Q_UNUSED(itemType)
  const QFileInfo info(path);
  if (!menu || (!info.isFile() && !info.isDir())) {
    return;
  }

  QObject *projectView =
      m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
  if (!projectView ||
      projectView->property("projectBaseDir").toString() != projectBaseDir) {
    return;
  }

  ProjectContext context;
  context.baseDir = projectBaseDir;
  context.projectFileName =
      projectView->property("projectFileName").toString();
  context.projectLocalConfigFileName =
      projectView->property("projectLocalConfigFileName").toString();
  context.projectMap = projectView->property("projectMap").toMap();
  context.files = projectView->property("projectFiles").toStringList();

  const auto config =
      Deployment::Config::fromProjectMap(context.projectMap, context.baseDir);
  if (!config.enabled || !config.valid || config.isExcluded(path) ||
      !config.remoteUrlForLocalPath(path)) {
    return;
  }

  menu->addSeparator();
  auto *upload = menu->addAction(
      QIcon::fromTheme(QStringLiteral("document-send")),
      info.isDir()
          ? i18nc("@action:inmenu", "Upload Folder to Remote Host")
          : i18nc("@action:inmenu", "Upload to Remote Host"));
  upload->setEnabled(!m_activeJob && m_pendingFiles.isEmpty());
  connect(upload, &QAction::triggered, this,
          [this, context = std::move(context), path,
           isDirectory = info.isDir()]() mutable {
            applyContext(std::move(context));
            if (isDirectory) {
              uploadLocalFolder(path);
            } else {
              uploadLocalFile(path);
            }
          });
}

void DeploymentView::applyContext(ProjectContext context) {
  const bool changed = context.baseDir != m_context.baseDir ||
                       context.projectMap != m_context.projectMap;
  m_context = std::move(context);
  m_config = Deployment::Config::fromProjectMap(m_context.projectMap,
                                                m_context.baseDir);
  if (changed) {
    configureRemoteBrowser();
  }
  updateActions();
}

void DeploymentView::updateActions() {
  const bool configured = m_config.enabled && m_config.valid;
  const auto *view = m_mainWindow->activeView();
  const bool localDocument =
      view && view->document()->url().isLocalFile() &&
      m_config.remoteUrlForLocalPath(view->document()->url().toLocalFile())
          .has_value();
  m_uploadCurrentAction->setEnabled(configured && localDocument &&
                                    m_pendingFiles.isEmpty() && !m_activeJob);
  m_uploadProjectAction->setEnabled(configured && !m_context.files.isEmpty() &&
                                    m_pendingFiles.isEmpty() && !m_activeJob);
  m_cancelAction->setEnabled(!m_pendingFiles.isEmpty() || m_activeJob);
  const bool fileBackedProject = QFileInfo(m_context.projectFileName).isFile();
  m_configureAction->setEnabled(fileBackedProject && m_pendingFiles.isEmpty() &&
                                !m_activeJob);
}

void DeploymentView::configure() {
  ProjectContext context = currentProjectContext();
  if (context.baseDir.isEmpty()) {
    showError(
        i18n("Open or select a Kate project before configuring deployment."));
    return;
  }
  if (!QFileInfo(context.projectFileName).isFile()) {
    showError(i18n("Deployment configuration requires a file-backed "
                   "<filename>.kateproject</filename>."));
    return;
  }

  ConfigFields fields = fieldsFromMap(context.projectMap);
  if (!editConfig(m_remoteToolView, context.baseDir, context.projectMap,
                  fields)) {
    return;
  }

  const QString localFileName = context.projectLocalConfigFileName;
  QJsonObject root;
  QFile input(localFileName);
  if (input.exists()) {
    if (!input.open(QFile::ReadOnly)) {
      showError(i18n("Could not read <filename>%1</filename>.", localFileName));
      return;
    }
    QJsonParseError error;
    const QJsonDocument document =
        QJsonDocument::fromJson(input.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
      showError(
          i18n("Could not update malformed JSON in <filename>%1</filename>: %2",
               localFileName, error.errorString()));
      return;
    }
    root = document.object();
  }
  root.insert(QStringLiteral("deployment"),
              QJsonObject::fromVariantMap(deploymentMap(fields)));

  QSaveFile output(localFileName);
  if (!output.open(QFile::WriteOnly) ||
      output.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 ||
      !output.commit()) {
    showError(i18n("Could not write <filename>%1</filename>.", localFileName));
    return;
  }

  context.projectMap.insert(QStringLiteral("deployment"),
                            deploymentMap(fields));
  applyContext(std::move(context));
  appendLog(i18n("Saved deployment configuration to %1", localFileName));

  if (QObject *projectPlugin =
          KTextEditor::Editor::instance()->application()->plugin(
              QStringLiteral("kateprojectplugin"))) {
    QMetaObject::invokeMethod(projectPlugin, "reloadProjectForBaseDir",
                              Q_ARG(QString, m_context.baseDir));
  }
}

void DeploymentView::uploadCurrentFile() {
  updateContext();
  KTextEditor::View *view = m_mainWindow->activeView();
  if (!view || !view->document()->url().isLocalFile()) {
    showError(i18n("The current document is not a local file."));
    return;
  }
  uploadLocalFile(view->document()->url().toLocalFile());
}

void DeploymentView::uploadLocalFile(const QString &path) {
  uploadLocalFiles({path});
}

void DeploymentView::uploadLocalFolder(const QString &path) {
  QStringList files;
  QDirIterator iterator(path, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
  while (iterator.hasNext()) {
    files.push_back(iterator.next());
  }
  uploadLocalFiles(files);
}

void DeploymentView::uploadLocalFiles(const QStringList &paths) {
  QSet<QString> canonicalPaths;
  for (const QString &path : paths) {
    const QString canonicalPath = QFileInfo(path).canonicalFilePath();
    if (!canonicalPath.isEmpty()) {
      canonicalPaths.insert(canonicalPath);
    }
  }

  const auto documents =
      KTextEditor::Editor::instance()->application()->documents();
  for (KTextEditor::Document *document : documents) {
    if (document->url().isLocalFile() && document->isModified() &&
        canonicalPaths.contains(
            QFileInfo(document->url().toLocalFile()).canonicalFilePath()) &&
        !document->save()) {
      showError(i18n("The file could not be saved before upload."));
      return;
    }
  }
  queueFiles(paths);
}

void DeploymentView::uploadProject() {
  updateContext();
  uploadLocalFiles(m_context.files);
}

void DeploymentView::queueFiles(const QStringList &files) {
  if (!m_config.enabled || !m_config.valid) {
    showError(m_config.error.isEmpty()
                  ? i18n("Deployment is not configured for this project.")
                  : m_config.error);
    return;
  }
  if (m_activeJob || !m_pendingFiles.isEmpty()) {
    showError(i18n("A deployment transfer is already running."));
    return;
  }

  QQueue<PendingUpload> pendingFiles;
  for (const QString &path : files) {
    const QFileInfo info(path);
    const auto remoteUrl = m_config.remoteUrlForLocalPath(path);
    if (info.isFile() && !m_config.isExcluded(path) && remoteUrl) {
      if (hasOpenRemoteDocument(*remoteUrl)) {
        showError(i18n("Cannot upload %1 because its remote version is open "
                       "in Kate.",
                       info.absoluteFilePath()));
        return;
      }
      pendingFiles.enqueue(PendingUpload{info.absoluteFilePath(), *remoteUrl,
                                         m_config.remoteBaseUrl()});
    }
  }
  m_pendingFiles = std::move(pendingFiles);
  if (m_pendingFiles.isEmpty()) {
    showError(i18n("No deployable files were selected."));
    return;
  }

  m_cancelled = false;
  m_mainWindow->showToolView(m_transferToolView);
  appendLog(i18np("Queued one file for upload.", "Queued %1 files for upload.",
                  m_pendingFiles.size()));
  updateActions();
  startNextUpload();
}

void DeploymentView::startNextUpload() {
  if (m_cancelled || m_pendingFiles.isEmpty()) {
    m_activeJob = nullptr;
    m_activeLocalPath.clear();
    m_activeRemoteUrl.clear();
    updateActions();
    return;
  }

  const PendingUpload upload = m_pendingFiles.dequeue();
  m_activeLocalPath = upload.localPath;
  if (hasOpenRemoteDocument(upload.remoteUrl)) {
    finishUpload(i18n("The remote file is open in Kate."));
    return;
  }
  startUpload(upload.localPath, upload.remoteUrl, upload.remoteBaseUrl);
}

void DeploymentView::startUpload(const QString &localPath,
                                  const QUrl &remoteUrl,
                                  const QUrl &remoteBaseUrl) {
  m_activeRemoteUrl = remoteUrl;
  appendLog(i18n("Uploading %1 to %2", localPath, displayUrl(remoteUrl)));

  const QUrl parentUrl =
      remoteUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
  auto *mkdirJob =
      KIO::mkpath(parentUrl, remoteBaseUrl, KIO::HideProgressInfo);
  setActiveJob(mkdirJob);
  connect(
      mkdirJob, &KJob::result, this,
      [this, localPath, remoteUrl, parentUrl](KJob *job) {
        m_activeJob = nullptr;
        if (job->error()) {
          finishUpload(job->errorString());
          return;
        }
        if (m_cancelled) {
          finishUpload();
          return;
        }

        m_temporaryRemoteUrl = remoteUrl;
        const QString temporaryName =
            QStringLiteral(".%1.kate-upload-%2")
                .arg(QFileInfo(remoteUrl.path()).fileName(),
                     QUuid::createUuid().toString(QUuid::WithoutBraces));
        QString temporaryPath = parentUrl.path();
        if (!temporaryPath.endsWith(u'/')) {
          temporaryPath += u'/';
        }
        m_temporaryRemoteUrl.setPath(temporaryPath + temporaryName);

        auto *copyJob =
            KIO::file_copy(QUrl::fromLocalFile(localPath), m_temporaryRemoteUrl,
                           -1, KIO::Overwrite | KIO::HideProgressInfo);
        setActiveJob(copyJob);
        connect(copyJob, &KJob::percentChanged, this,
                [this](KJob *, unsigned long percent) {
                  m_cancelAction->setText(i18nc(
                      "@action", "Cancel Deployment Transfers (%1%)", percent));
                });
        connect(copyJob, &KJob::result, this, [this](KJob *copyResult) {
          m_activeJob = nullptr;
          m_cancelAction->setText(
              i18nc("@action", "Cancel Deployment Transfers"));
          if (copyResult->error()) {
            cleanupTemporaryFile();
            finishUpload(copyResult->errorString());
            return;
          }
          if (m_cancelled) {
            cleanupTemporaryFile();
            finishUpload();
            return;
          }
          if (hasOpenRemoteDocument(m_activeRemoteUrl)) {
            cleanupTemporaryFile();
            finishUpload(i18n("The remote file is open in Kate."));
            return;
          }

          auto *renameJob = KIO::rename(m_temporaryRemoteUrl, m_activeRemoteUrl,
                                        KIO::Overwrite | KIO::HideProgressInfo);
          setActiveJob(renameJob);
          connect(renameJob, &KJob::result, this, [this](KJob *renameResult) {
            m_activeJob = nullptr;
            if (renameResult->error()) {
              cleanupTemporaryFile();
              finishUpload(renameResult->errorString());
              return;
            }
            m_temporaryRemoteUrl.clear();
            finishUpload();
          });
        });
      });
}

void DeploymentView::finishUpload(const QString &error) {
  if (!error.isEmpty()) {
    appendLog(i18n("Failed to upload %1: %2", m_activeLocalPath, error));
  } else if (!m_cancelled && !m_activeLocalPath.isEmpty()) {
    appendLog(i18n("Uploaded %1", m_activeLocalPath));
    if (m_remoteOperator && isRemoteUrlAllowed(m_activeRemoteUrl)) {
      m_remoteOperator->updateDir();
    }
  }
  m_activeLocalPath.clear();
  m_activeRemoteUrl.clear();
  m_activeJob = nullptr;
  startNextUpload();
}

void DeploymentView::cancelTransfers() {
  if (!m_activeJob && m_pendingFiles.isEmpty()) {
    return;
  }
  m_cancelled = true;
  m_pendingFiles.clear();
  if (m_activeJob && m_activeJob->capabilities().testFlag(KJob::Killable)) {
    m_activeJob->kill(KJob::EmitResult);
  }
  appendLog(i18n("Cancelled deployment transfers."));
  updateActions();
}

void DeploymentView::setActiveJob(KJob *job) {
  m_activeJob = job;
  QWidget *window = m_mainWindow->window();
  if (!window) {
    window = m_transferToolView->window();
  }
  if (!job->uiDelegate()) {
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(
        KJobUiDelegate::AutoHandlingDisabled, window));
  }
  KJobWidgets::setWindow(job, window);
  updateActions();
}

void DeploymentView::cleanupTemporaryFile() {
  if (m_temporaryRemoteUrl.isEmpty()) {
    return;
  }
  KIO::del(m_temporaryRemoteUrl, KIO::HideProgressInfo);
  m_temporaryRemoteUrl.clear();
}

void DeploymentView::configureRemoteBrowser() {
  const bool configured = m_config.enabled && m_config.valid;
  m_remoteNavigator->setVisible(configured);
  m_remoteOperator->setVisible(configured);
  m_remoteStatus->setVisible(configured);
  if (configured) {
    setRemoteStatus(i18n("Not connected"));
    navigateRemote(m_config.remoteBaseUrl());
  }
}

bool DeploymentView::isRemoteUrlAllowed(const QUrl &url) const {
  return m_config.localPathForRemoteUrl(url).has_value();
}

void DeploymentView::navigateRemote(const QUrl &url) {
  if (!m_config.enabled || !m_config.valid) {
    return;
  }
  const QUrl target = isRemoteUrlAllowed(url) ? url : m_config.remoteBaseUrl();
  if (!KProtocolInfo::isKnownProtocol(QStringLiteral("sftp")) ||
      !KProtocolManager::supportsListing(target)) {
    const QString message = i18n(
        "The SFTP KIO worker is not installed or does not support directory "
        "listing.");
    setRemoteStatus(message, true);
    appendLog(message);
    return;
  }
  if (m_remoteNavigator->locationUrl() != target) {
    const QSignalBlocker blocker(m_remoteNavigator);
    m_remoteNavigator->setLocationUrl(target);
  }
  if (m_remoteOperator->url() != target) {
    m_remoteOperator->setUrl(target, true);
  }
}

void DeploymentView::setRemoteStatus(const QString &message, bool error) {
  m_remoteStatus->setText(message);
  m_remoteStatus->setToolTip(message);
  m_remoteStatus->setStyleSheet(
      error ? QStringLiteral("QLabel { color: palette(link-visited); }")
            : QString());
}

void DeploymentView::remoteContextMenu(const KFileItem &item, QMenu *menu) {
  if (item.isNull() || !isRemoteUrlAllowed(item.url())) {
    return;
  }
  menu->addSeparator();
  if (!item.isDir()) {
    auto *openRemote = menu->addAction(
        QIcon::fromTheme(QStringLiteral("document-open-remote")),
        i18nc("@action:inmenu", "Open Remote File"));
    connect(openRemote, &QAction::triggered, this,
            [this, url = item.url()] {
              if (isRemoteUrlAllowed(url)) {
                m_mainWindow->openUrl(url);
              }
            });

    const auto localPath = m_config.localPathForRemoteUrl(item.url());
    auto *openLocal =
        menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")),
                        i18nc("@action:inmenu", "Open Mapped Local File"));
    openLocal->setEnabled(localPath && QFileInfo::exists(*localPath));
    connect(openLocal, &QAction::triggered, this,
            [this, url = item.url()] { openMappedLocalFile(url); });

    auto *uploadLocal =
        menu->addAction(QIcon::fromTheme(QStringLiteral("document-send")),
                        i18nc("@action:inmenu", "Upload Mapped Local File"));
    uploadLocal->setEnabled(localPath && QFileInfo(*localPath).isFile() &&
                            !m_config.isExcluded(*localPath));
    connect(uploadLocal, &QAction::triggered, this, [this, localPath] {
      if (localPath && m_config.remoteUrlForLocalPath(*localPath)) {
        queueFiles({*localPath});
      }
    });
  }
  auto *copyUrl = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")),
                                  i18nc("@action:inmenu", "Copy Remote URL"));
  connect(copyUrl, &QAction::triggered, this, [url = item.url()] {
    QGuiApplication::clipboard()->setText(
        url.toDisplayString(QUrl::RemovePassword));
  });
  auto *copyPath = menu->addAction(
      QIcon::fromTheme(QStringLiteral("edit-copy")),
      i18nc("@action:inmenu", "Copy Remote Path"));
  connect(copyPath, &QAction::triggered, this, [url = item.url()] {
    QGuiApplication::clipboard()->setText(url.path());
  });
}

void DeploymentView::openMappedLocalFile(const QUrl &remoteUrl) {
  const auto localPath = m_config.localPathForRemoteUrl(remoteUrl);
  if (!localPath || !QFileInfo::exists(*localPath)) {
    showError(
        i18n("No mapped local file exists for %1.", displayUrl(remoteUrl)));
    return;
  }
  m_mainWindow->openUrl(QUrl::fromLocalFile(*localPath));
}

void DeploymentView::appendLog(const QString &message) {
  const QString timestamp =
      QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
  m_transferLog->appendPlainText(
      QStringLiteral("[%1] %2").arg(timestamp, message));
}

void DeploymentView::showError(const QString &message) {
  appendLog(message);
  m_mainWindow->showToolView(m_transferToolView);
  Utils::showMessage(message, QIcon::fromTheme(QStringLiteral("document-send")),
                     i18n("Deployment"), MessageType::Error, m_mainWindow);
}

#include "deploymentplugin.moc"
#include "moc_deploymentplugin.cpp"
