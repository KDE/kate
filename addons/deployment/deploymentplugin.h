/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "deploymentconfig.h"

#include <KXMLGUIClient>
#include <ktexteditor/plugin.h>

#include <QPointer>
#include <QQueue>

class QAction;
class KDirOperator;
class KFileItem;
class KJob;
class KUrlNavigator;
class QLabel;
class QMenu;
class QPlainTextEdit;
class QWidget;

namespace KTextEditor {
class MainWindow;
}

class DeploymentPlugin : public KTextEditor::Plugin {
  Q_OBJECT

public:
  explicit DeploymentPlugin(QObject *parent);
  QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class DeploymentView : public QObject, public KXMLGUIClient {
  Q_OBJECT

public:
  DeploymentView(DeploymentPlugin *plugin, KTextEditor::MainWindow *mainWindow);
  ~DeploymentView() override;

private Q_SLOTS:
  void updateContext();
  void projectTreeContextMenuAboutToShow(QMenu *menu, const QString &path,
                                         const QString &projectBaseDir,
                                         int itemType);

private:
  struct PendingUpload {
    QString localPath;
    QUrl remoteUrl;
    QUrl remoteBaseUrl;
  };

  struct ProjectContext {
    QString baseDir;
    QString projectFileName;
    QString projectLocalConfigFileName;
    QVariantMap projectMap;
    QStringList files;
  };

  ProjectContext currentProjectContext() const;
  void applyContext(ProjectContext context);
  void updateActions();

  void configure();
  void uploadCurrentFile();
  void uploadLocalFile(const QString &path);
  void uploadLocalFolder(const QString &path);
  void uploadLocalFiles(const QStringList &paths);
  void uploadProject();
  void queueFiles(const QStringList &files);
  void startNextUpload();
  void startUpload(const QString &localPath, const QUrl &remoteUrl,
                   const QUrl &remoteBaseUrl);
  void finishUpload(const QString &error = {});
  void cancelTransfers();
  void setActiveJob(KJob *job);
  void cleanupTemporaryFile();

  void configureRemoteBrowser();
  void navigateRemote(const QUrl &url);
  bool isRemoteUrlAllowed(const QUrl &url) const;
  void remoteContextMenu(const KFileItem &item, QMenu *menu);
  void openMappedLocalFile(const QUrl &remoteUrl);
  void setRemoteStatus(const QString &message, bool error = false);

  void appendLog(const QString &message);
  void showError(const QString &message);

  DeploymentPlugin *m_plugin;
  KTextEditor::MainWindow *m_mainWindow;
  ProjectContext m_context;
  Deployment::Config m_config;

  QAction *m_uploadCurrentAction = nullptr;
  QAction *m_uploadProjectAction = nullptr;
  QAction *m_cancelAction = nullptr;
  QAction *m_configureAction = nullptr;

  QWidget *m_remoteToolView = nullptr;
  KUrlNavigator *m_remoteNavigator = nullptr;
  KDirOperator *m_remoteOperator = nullptr;
  QLabel *m_remoteStatus = nullptr;
  QWidget *m_transferToolView = nullptr;
  QPlainTextEdit *m_transferLog = nullptr;

  QQueue<PendingUpload> m_pendingFiles;
  QPointer<KJob> m_activeJob;
  QString m_activeLocalPath;
  QUrl m_activeRemoteUrl;
  QUrl m_temporaryRemoteUrl;
  bool m_cancelled = false;
};
