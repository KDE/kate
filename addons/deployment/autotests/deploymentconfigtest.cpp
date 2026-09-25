/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "deploymentconfig.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class DeploymentConfigTest : public QObject {
  Q_OBJECT

private Q_SLOTS:
  void parse();
  void invalidConfig();
  void ipv6AndEncoding();
  void mapping();
  void rejectOutsideAndTraversal();
  void symlinkEscape();
  void excludes();
};

static QVariantMap projectConfig(const QVariantMap &deployment) {
  return {{QStringLiteral("deployment"), deployment}};
}

static Deployment::Config configFor(const QString &base,
                                    const QVariantMap &extra = {}) {
  QVariantMap deployment{
      {QStringLiteral("host"), QStringLiteral("files.example.org")},
      {QStringLiteral("user"), QStringLiteral("kate")},
      {QStringLiteral("localRoot"), QStringLiteral("src")},
      {QStringLiteral("remoteRoot"), QStringLiteral("/srv/project")},
  };
  for (auto it = extra.cbegin(); it != extra.cend(); ++it) {
    deployment.insert(it.key(), it.value());
  }
  return Deployment::Config::fromProjectMap(projectConfig(deployment), base);
}

void DeploymentConfigTest::parse() {
  QTemporaryDir project;
  QVERIFY(project.isValid());
  QVERIFY(QDir(project.path()).mkdir(QStringLiteral("src")));

  const auto disabled = Deployment::Config::fromProjectMap({}, project.path());
  QVERIFY(!disabled.enabled);
  QVERIFY(disabled.valid);
  QVERIFY(disabled.error.isEmpty());

  const auto config = configFor(
      project.path(),
      {{QStringLiteral("port"), 2222},
       {QStringLiteral("exclude"),
        QStringList{QStringLiteral("*.o"), QStringLiteral("build")}}});
  QVERIFY2(config.valid, qPrintable(config.error));
  QVERIFY(config.enabled);
  QCOMPARE(config.host, QStringLiteral("files.example.org"));
  QCOMPARE(config.port, 2222);
  QCOMPARE(config.user, QStringLiteral("kate"));
  QCOMPARE(config.localRoot,
           QDir(project.path()).canonicalPath() + QStringLiteral("/src"));
  QCOMPARE(config.remoteRoot, QStringLiteral("/srv/project"));
  QCOMPARE(config.excludePatterns.size(), 2);
  QCOMPARE(config.remoteBaseUrl().toEncoded(),
           QByteArray("sftp://kate@files.example.org:2222/srv/project"));
  QVERIFY(config.remoteBaseUrl().password().isEmpty());
}

void DeploymentConfigTest::invalidConfig() {
  QTemporaryDir project;
  QTemporaryDir outside;
  QVERIFY(project.isValid());
  QVERIFY(outside.isValid());
  QVERIFY(QDir(project.path()).mkdir(QStringLiteral("src")));

  QVERIFY(
      !configFor(project.path(), {{QStringLiteral("host"), QString()}}).valid);
  QVERIFY(!configFor(project.path(), {{QStringLiteral("port"), 70000}}).valid);
  QVERIFY(!configFor(project.path(), {{QStringLiteral("remoteRoot"),
                                       QStringLiteral("relative")}})
               .valid);
  QVERIFY(!configFor(project.path(), {{QStringLiteral("remoteRoot"),
                                       QStringLiteral("/srv/../secret")}})
               .valid);
  QVERIFY(!configFor(project.path(), {{QStringLiteral("localRoot"),
                                       QStringLiteral("../outside")}})
               .valid);
  QVERIFY(!configFor(project.path(),
                     {{QStringLiteral("localRoot"), outside.path()}})
               .valid);
  QVERIFY(!configFor(QString()).valid);
  QVERIFY(!Deployment::Config::fromProjectMap(
               {{QStringLiteral("deployment"), QStringLiteral("invalid")}},
               project.path())
               .valid);
}

void DeploymentConfigTest::ipv6AndEncoding() {
  QTemporaryDir project;
  QVERIFY(project.isValid());
  QVERIFY(QDir(project.path()).mkdir(QStringLiteral("src")));
  QFile file(project.filePath(QStringLiteral("src/a #file.txt")));
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.close();

  const auto config = configFor(
      project.path(), {{QStringLiteral("host"), QStringLiteral("2001:db8::1")},
                       {QStringLiteral("user"), QStringLiteral("a user")}});
  QVERIFY2(config.valid, qPrintable(config.error));
  const auto url = config.remoteUrlForLocalPath(file.fileName());
  QVERIFY(url);
  QCOMPARE(url->toEncoded(),
           QByteArray(
               "sftp://a%20user@[2001:db8::1]:22/srv/project/a%20%23file.txt"));
  QCOMPARE(config.localPathForRemoteUrl(*url), std::optional(file.fileName()));
}

void DeploymentConfigTest::mapping() {
  QTemporaryDir project;
  QVERIFY(project.isValid());
  QVERIFY(QDir(project.path()).mkpath(QStringLiteral("src/lib")));
  QFile file(project.filePath(QStringLiteral("src/lib/main.cpp")));
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.close();

  const auto config = configFor(project.path());
  QVERIFY2(config.valid, qPrintable(config.error));
  const auto remote = config.remoteUrlForLocalPath(file.fileName());
  QVERIFY(remote);
  QCOMPARE(remote->path(), QStringLiteral("/srv/project/lib/main.cpp"));
  QCOMPARE(config.localPathForRemoteUrl(*remote),
           std::optional(file.fileName()));
  QCOMPARE(config.remoteUrlForLocalPath(config.localRoot),
           std::optional(config.remoteBaseUrl()));
  QCOMPARE(config.localPathForRemoteUrl(config.remoteBaseUrl()),
           std::optional(config.localRoot));
}

void DeploymentConfigTest::rejectOutsideAndTraversal() {
  QTemporaryDir project;
  QVERIFY(project.isValid());
  QVERIFY(QDir(project.path()).mkdir(QStringLiteral("src")));
  const auto config = configFor(project.path());
  QVERIFY(config.valid);

  QVERIFY(!config.remoteUrlForLocalPath(
      project.filePath(QStringLiteral("src2/file.cpp"))));
  QVERIFY(!config.remoteUrlForLocalPath(
      project.filePath(QStringLiteral("src/../outside.cpp"))));
  QVERIFY(!config.localPathForRemoteUrl(QUrl(QStringLiteral(
      "sftp://kate@files.example.org:22/srv/project2/file.cpp"))));
  QVERIFY(!config.localPathForRemoteUrl(QUrl(QStringLiteral(
      "sftp://kate@files.example.org:22/srv/project/%2e%2e/secret"))));
  QVERIFY(!config.localPathForRemoteUrl(QUrl(QStringLiteral(
      "sftp://other@files.example.org:22/srv/project/file.cpp"))));
}

void DeploymentConfigTest::symlinkEscape() {
  QTemporaryDir project;
  QTemporaryDir outside;
  QVERIFY(project.isValid());
  QVERIFY(outside.isValid());
  QVERIFY(QDir(project.path()).mkdir(QStringLiteral("src")));
  const QString link = project.filePath(QStringLiteral("src/link"));
  if (!QFile::link(outside.path(), link)) {
    QSKIP("Creating directory symlinks is not supported");
  }
  const QFileInfo linkInfo(link);
  if (!linkInfo.isSymLink() || !linkInfo.isDir()) {
    QSKIP("QFile::link did not create a directory symlink");
  }

  const auto config = configFor(project.path());
  QVERIFY(config.valid);
  QVERIFY(!config.remoteUrlForLocalPath(link + QStringLiteral("/file.cpp")));
  QVERIFY(!config.localPathForRemoteUrl(QUrl(QStringLiteral(
      "sftp://kate@files.example.org:22/srv/project/link/file.cpp"))));
}

void DeploymentConfigTest::excludes() {
  QTemporaryDir project;
  QVERIFY(project.isValid());
  QVERIFY(QDir(project.path()).mkpath(QStringLiteral("src/lib/build/deep")));
  QVERIFY(QDir(project.path()).mkpath(QStringLiteral("src/generated/api")));
  const auto config =
      configFor(project.path(),
                {{QStringLiteral("exclude"),
                  QStringList{QStringLiteral("*.o"), QStringLiteral("build"),
                              QStringLiteral("generated/api")}}});
  QVERIFY(config.valid);

  QVERIFY(
      config.isExcluded(project.filePath(QStringLiteral("src/lib/object.o"))));
  QVERIFY(config.isExcluded(
      project.filePath(QStringLiteral("src/lib/build/deep/file.cpp"))));
  QVERIFY(config.isExcluded(
      project.filePath(QStringLiteral("src/generated/api/file.cpp"))));
  QVERIFY(!config.isExcluded(
      project.filePath(QStringLiteral("src/generated/other.cpp"))));
  QVERIFY(!config.isExcluded(
      project.filePath(QStringLiteral("src/lib/object.cpp"))));
  QVERIFY(!config.isExcluded(project.filePath(QStringLiteral("outside.o"))));
}

QTEST_GUILESS_MAIN(DeploymentConfigTest)

#include "deploymentconfigtest.moc"
