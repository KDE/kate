/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.ahmed@kdab.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kateapptest.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "katesessionmanager.h"
#include "kateviewmanager.h"

#include <KActionCollection>
#include <KConfig>
#include <KPluginMetaData>
#include <KTextEditor/View>
#include <KXMLGUIClient>

#include <QAction>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

static void checkPluginsLoadedCorrectly(KateApp *app, const QStringList &plugins)
{
    const auto pluginDir = []() -> QString {
        // kate_add_app_test() sets this, the plugins must come from the build directory and not from an installed Kate
        QString pluginDir = qEnvironmentVariable("KATE_APP_TEST_PLUGIN_DIR");
        if (!pluginDir.isEmpty()) {
            return pluginDir;
        }
        // fallback to looking at path beside app exe
        return qApp->applicationDirPath() + QStringLiteral("/kf6/ktexteditor");
    }();
    Q_ASSERT(!pluginDir.isEmpty());
    const QString canonicalPluginDir = QDir(pluginDir).canonicalPath();
    qDebug("Plugin path: %ls", qUtf16Printable(canonicalPluginDir));

    for (const QString &plugin : plugins) {
        if (!app->plugin(plugin)) {
            qFatal("Failed to load plugin %ls", qUtf16Printable(plugin));
        }
        const KPluginMetaData metaData = KPluginMetaData::findPluginById(QStringLiteral("kf6/ktexteditor"), plugin);
        const QString loadedFrom = QFileInfo(metaData.fileName()).canonicalPath();
        if (loadedFrom != canonicalPluginDir) {
            qFatal("Plugin %ls loaded from %ls, expected %ls", qUtf16Printable(plugin), qUtf16Printable(loadedFrom), qUtf16Printable(canonicalPluginDir));
        }
    }
}

KateAppTest::KateAppTest(const QStringList &plugins)
    : m_tempDir(std::make_unique<QTemporaryDir>())
    , m_parser(std::make_unique<QCommandLineParser>())
{
    // keep the user's config and sessions untouched, and don't load any plugins we didn't ask for
    QStandardPaths::setTestModeEnabled(true);

    if (!m_tempDir->isValid()) {
        qFatal("Failed to create a temporary directory");
    }

    KConfig::setMainConfigName(m_tempDir->filePath(QStringLiteral("kateapptestrc")));
    m_app = std::make_unique<KateApp>(*m_parser, KateApp::ApplicationKate, m_tempDir->filePath(QStringLiteral("sessions")), plugins);

    // activating a session loads the plugins, with their views in the main window
    m_app->sessionManager()->sessionNew();
    m_mainWindow = m_app->activeKateMainWindow();
    Q_ASSERT(m_mainWindow);

    checkPluginsLoadedCorrectly(m_app.get(), plugins);
}

KateAppTest::~KateAppTest() = default;

KTextEditor::MainWindow *KateAppTest::mainWindow() const
{
    return m_mainWindow->wrapper();
}

QObject *KateAppTest::pluginView(const QString &plugin) const
{
    return m_mainWindow->pluginView(plugin);
}

QAction *KateAppTest::action(const QString &plugin, const QString &name) const
{
    auto client = dynamic_cast<KXMLGUIClient *>(pluginView(plugin));
    return client ? client->actionCollection()->action(name) : nullptr;
}

QAction *KateAppTest::appAction(const QString &name) const
{
    return m_mainWindow->actionCollection()->action(name);
}

KTextEditor::View *KateAppTest::openFile(const QString &path, KTextEditor::Cursor cursor)
{
    if (path.isEmpty()) {
        return nullptr;
    }

    KTextEditor::View *view = m_mainWindow->viewManager()->openUrlWithView(QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()), QString());
    if (view && cursor.isValid()) {
        view->setCursorPosition(cursor);
    }
    return view;
}

QString KateAppTest::writeFile(const QString &fileName, const QString &text) const
{
    QString path = QDir(m_tempDir->path()).absoluteFilePath(fileName);
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return {};
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    file.write(text.toUtf8());
    return path;
}

bool KateAppTest::closeAllDocuments()
{
    // the documents of tests are unmodified or temporary, don't ask
    const auto documents = m_app->documentManager()->documentList();
    for (auto doc : documents) {
        doc->setModified(false);
    }
    return m_app->documentManager()->closeAllDocuments();
}

QWidget *KateAppTest::toolView(const QString &identifier) const
{
    // tool views have their identifier as object name
    const auto widgets = m_mainWindow->findChildren<QWidget *>(identifier);
    return widgets.size() == 1 ? widgets.constFirst() : nullptr;
}

bool KateAppTest::showToolView(const QString &identifier)
{
    QWidget *widget = toolView(identifier);
    return widget && m_mainWindow->showToolView(widget);
}

QString KateAppTest::tempDir() const
{
    return m_tempDir->path();
}
