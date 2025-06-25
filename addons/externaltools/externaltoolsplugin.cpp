/* This file is part of the KDE project
 *
 *  SPDX-FileCopyrightText: 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "externaltoolsplugin.h"

#include "kateexternaltool.h"
#include "kateexternaltoolscommand.h"
#include "kateexternaltoolsconfigwidget.h"
#include "kateexternaltoolsview.h"
#include "katetoolrunner.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <QAction>
#include <kparts/part.h>

#include <KAuthorized>
#include <KConfig>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include <QClipboard>
#include <QGuiApplication>

#include <ktexteditor_utils.h>

static QString toolsConfigDir()
{
    static const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/kate/externaltools/");
    return dir;
}

static QList<KateExternalTool> readDefaultTools()
{
    QDir dir(QStringLiteral(":/kconfig/externaltools-config/"));
    const QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::Files);

    QList<KateExternalTool> tools;
    for (const auto &file : entries) {
        KConfig config(dir.absoluteFilePath(file));
        KConfigGroup cg = config.group(QStringLiteral("General"));

        KateExternalTool tool;
        tool.load(cg);
        tools.push_back(tool);
    }

    return tools;
}

K_PLUGIN_FACTORY_WITH_JSON(KateExternalToolsFactory, "externaltoolsplugin.json", registerPlugin<KateExternalToolsPlugin>();)

KateExternalToolsPlugin::KateExternalToolsPlugin(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
    m_config = KSharedConfig::openConfig(QStringLiteral("kate-externaltoolspluginrc"), KConfig::NoGlobals, QStandardPaths::GenericConfigLocation);
    QDir().mkdir(toolsConfigDir());

    migrateConfig();

    // load config from disk
    reload();
}

KateExternalToolsPlugin::~KateExternalToolsPlugin()
{
    clearTools();
}

void KateExternalToolsPlugin::migrateConfig()
{
    const QString oldFile = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("externaltools"));

    if (!oldFile.isEmpty()) {
        KConfig oldConf(oldFile);
        KConfigGroup oldGroup(&oldConf, QStringLiteral("Global"));

        const bool isFirstRun = oldGroup.readEntry("firststart", true);
        m_config->group(QStringLiteral("Global")).writeEntry("firststart", isFirstRun);

        const int toolCount = oldGroup.readEntry("tools", 0);
        for (int i = 0; i < toolCount; ++i) {
            oldGroup = oldConf.group(QStringLiteral("Tool %1").arg(i));
            const QString name = KateExternalTool::configFileName(oldGroup.readEntry("name"));
            const QString newConfPath = toolsConfigDir() + name;
            if (QFileInfo::exists(newConfPath)) { // Already migrated ?
                continue;
            }

            KConfig newConfig(newConfPath);
            KConfigGroup newGroup = newConfig.group(QStringLiteral("General"));
            oldGroup.copyTo(&newGroup, KConfigBase::Persistent);
        }

        QFile::remove(oldFile);
    }
}

QObject *KateExternalToolsPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    auto *view = new KateExternalToolsPluginView(mainWindow, this);
    connect(this, &KateExternalToolsPlugin::externalToolsChanged, view, &KateExternalToolsPluginView::rebuildMenu);
    return view;
}

void KateExternalToolsPlugin::clearTools()
{
    delete m_command;
    m_command = nullptr;
    m_commands.clear();
    qDeleteAll(m_tools);
    m_tools.clear();
}

void KateExternalToolsPlugin::addNewTool(KateExternalTool *tool)
{
    m_tools.push_back(tool);
    if (tool->canExecute() && !tool->cmdname.isEmpty()) {
        m_commands.push_back(tool->cmdname);
    }
    if (KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
        m_command = new KateExternalToolsCommand(this);
    }
}

void KateExternalToolsPlugin::removeTools(const std::vector<KateExternalTool *> &toRemove)
{
    for (auto *tool : toRemove) {
        if (!tool) {
            continue;
        }

        if (QString configFile = KateExternalTool::configFileName(tool->name); !configFile.isEmpty()) {
            QFile::remove(toolsConfigDir() + configFile);
        }

        // remove old name variant, too
        if (QString configFile = KateExternalTool::configFileNameOldStyleOnlyForRemove(tool->name); !configFile.isEmpty()) {
            QFile::remove(toolsConfigDir() + configFile);
        }

        delete tool;
    }

    auto it = std::remove_if(m_tools.begin(), m_tools.end(), [&toRemove](KateExternalTool *tool) {
        return std::find(toRemove.cbegin(), toRemove.cend(), tool) != toRemove.cend();
    });
    m_tools.erase(it, m_tools.end());
}

void KateExternalToolsPlugin::save(KateExternalTool *tool, const QString &oldName)
{
    const QString name = KateExternalTool::configFileName(tool->name);
    KConfig config(toolsConfigDir() + name);
    KConfigGroup cg = config.group(QStringLiteral("General"));
    tool->save(cg);

    // The tool was renamed, remove the old config file
    if (!oldName.isEmpty()) {
        const QString oldFile = toolsConfigDir() + KateExternalTool::configFileName(oldName);
        QFile::remove(oldFile);

        // remove old variant, too
        const QString oldFile2 = toolsConfigDir() + KateExternalTool::configFileNameOldStyleOnlyForRemove(oldName);
        QFile::remove(oldFile2);
    }
}

void KateExternalToolsPlugin::reload()
{
    KConfigGroup group(m_config, QStringLiteral("Global"));
    const bool firstStart = group.readEntry("firststart", true);

    if (!firstStart) {
        // read user config
        QDir dir(toolsConfigDir());
        const QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::Files);
        for (const auto &file : entries) {
            KConfig config(dir.absoluteFilePath(file));
            KConfigGroup cg = config.group(QStringLiteral("General"));

            auto t = new KateExternalTool();
            t->load(cg);
            m_tools.push_back(t);
        }
    } else {
        // first start -> use system config
        const auto defaultTools = this->defaultTools();
        for (const auto &tool : defaultTools) {
            m_tools.push_back(new KateExternalTool(tool));
            save(m_tools.back(), {});
        }
        // not first start anymore
        group.writeEntry("firststart", false);
    }

    // FIXME test for a command name first!
    for (auto *tool : std::as_const(m_tools)) {
        if (tool->canExecute() && !tool->cmdname.isEmpty()) {
            m_commands.push_back(tool->cmdname);
        }
    }

    if (KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
        m_command = new KateExternalToolsCommand(this);
    }

    Q_EMIT externalToolsChanged();
}

QStringList KateExternalToolsPlugin::commands() const
{
    return m_commands;
}

const KateExternalTool *KateExternalToolsPlugin::toolForCommand(const QString &cmd) const
{
    for (auto tool : m_tools) {
        if (tool->cmdname == cmd) {
            return tool;
        }
    }
    return nullptr;
}

const QList<KateExternalTool *> &KateExternalToolsPlugin::tools() const
{
    return m_tools;
}

QList<KateExternalTool> KateExternalToolsPlugin::defaultTools() const
{
    if (m_defaultTools.isEmpty()) {
        const_cast<KateExternalToolsPlugin *>(this)->m_defaultTools = readDefaultTools();
    }
    return m_defaultTools;
}

KateToolRunner *KateExternalToolsPlugin::runnerForTool(const KateExternalTool &tool, KTextEditor::View *view, bool executingSaveTrigger)
{
    // expand the macros in command if any,
    // and construct a command with an absolute path
    auto mw = view->mainWindow();

    // save documents if requested
    if (!executingSaveTrigger) {
        if (tool.saveMode == KateExternalTool::SaveMode::CurrentDocument) {
            // only save if modified, to avoid unnecessary recompiles
            if (view->document()->isModified() && view->document()->url().isValid()) {
                view->document()->save();
            }
        } else if (tool.saveMode == KateExternalTool::SaveMode::AllDocuments) {
            const auto guiClients = mw->guiFactory()->clients();
            for (KXMLGUIClient *client : guiClients) {
                if (QAction *a = client->actionCollection()->action(QStringLiteral("file_save_all"))) {
                    a->trigger();
                    break;
                }
            }
        }
    }

    // copy tool
    std::unique_ptr<KateExternalTool> copy(new KateExternalTool(tool));

    // clear previous toolview data
    auto pluginView = viewForMainWindow(mw);
    pluginView->clearToolView();

    // expand macros
    auto editor = KTextEditor::Editor::instance();
    copy->executable = editor->expandText(copy->executable, view);
    copy->arguments = editor->expandText(copy->arguments, view);
    copy->workingDir = editor->expandText(copy->workingDir, view);
    copy->input = editor->expandText(copy->input, view);

    if (!copy->checkExec()) {
        Utils::showMessage(
            i18n("Failed to find executable '%1'. Please make sure the executable file exists and that variable names, if used, are correct", tool.executable),
            QIcon::fromTheme(QStringLiteral("system-run")),
            i18n("External Tools"),
            MessageType::Error,
            pluginView->mainWindow());
        return nullptr;
    }

    const QString messageText = copy->input.isEmpty() ? i18n("Running %1: %2 %3", copy->name, copy->executable, copy->arguments)
                                                      : i18n("Running %1: %2 %3 with input %4", copy->name, copy->executable, copy->arguments, tool.input);

    // use generic output view for status
    Utils::showMessage(messageText, QIcon::fromTheme(QStringLiteral("system-run")), i18n("External Tools"), MessageType::Info, pluginView->mainWindow());

    // Allocate runner on heap such that it lives as long as the child
    // process is running and does not block the main thread.
    return new KateToolRunner(std::move(copy), view, this);
}

void KateExternalToolsPlugin::runTool(const KateExternalTool &tool, KTextEditor::View *view, bool executingSaveTrigger)
{
    auto runner = runnerForTool(tool, view, executingSaveTrigger);
    if (!runner) {
        return;
    }
    // use QueuedConnection, since handleToolFinished deletes the runner
    connect(runner, &KateToolRunner::toolFinished, this, &KateExternalToolsPlugin::handleToolFinished, Qt::QueuedConnection);
    runner->run();
}

void KateExternalToolsPlugin::blockingRunTool(const KateExternalTool &tool, KTextEditor::View *view, bool executingSaveTrigger)
{
    auto runner = runnerForTool(tool, view, executingSaveTrigger);
    if (!runner) {
        return;
    }
    connect(runner, &KateToolRunner::toolFinished, this, &KateExternalToolsPlugin::handleToolFinished);
    runner->run();
    runner->waitForFinished();
}

void KateExternalToolsPlugin::handleToolFinished(KateToolRunner *runner, int exitCode, bool crashed) const
{
    auto view = runner->view();
    if (view && !runner->outputData().isEmpty()) {
        switch (runner->tool()->outputMode) {
        case KateExternalTool::OutputMode::InsertAtCursor: {
            KTextEditor::Document::EditingTransaction transaction(view->document());
            view->removeSelection();
            view->insertText(runner->outputData());
            break;
        }
        case KateExternalTool::OutputMode::ReplaceSelectedText: {
            KTextEditor::Document::EditingTransaction transaction(view->document());
            view->removeSelectionText();
            view->insertText(runner->outputData());
            break;
        }
        case KateExternalTool::OutputMode::ReplaceCurrentDocument: {
            KTextEditor::Document::EditingTransaction transaction(view->document());
            auto cursor = view->cursorPosition();
            view->document()->clear();
            view->insertText(runner->outputData());
            view->setCursorPosition(cursor);
            break;
        }
        case KateExternalTool::OutputMode::AppendToCurrentDocument: {
            view->document()->insertText(view->document()->documentEnd(), runner->outputData());
            break;
        }
        case KateExternalTool::OutputMode::InsertInNewDocument: {
            auto mainWindow = view->mainWindow();
            auto newView = mainWindow->openUrl({});
            newView->insertText(runner->outputData());
            mainWindow->activateView(newView->document());
            break;
        }
        case KateExternalTool::OutputMode::CopyToClipboard: {
            QGuiApplication::clipboard()->setText(runner->outputData());
            break;
        }
        default:
            break;
        }
    }

    if (view && runner->tool()->reload || view->document()->documentHasAutoReloadConfiguration()) {
        // updates-enabled trick: avoid some flicker
        const bool wereUpdatesEnabled = view->updatesEnabled();
        view->setUpdatesEnabled(false);

        Utils::KateScrollBarRestorer scrollRestorer(view);

        // Reload doc
        view->document()->documentReload();

        scrollRestorer.restore();

        view->setUpdatesEnabled(wereUpdatesEnabled);
    }

    KateExternalToolsPluginView *pluginView = runner->view() ? viewForMainWindow(runner->view()->mainWindow()) : nullptr;
    if (pluginView) {
        bool hasOutputInPane = false;
        if (runner->tool()->outputMode == KateExternalTool::OutputMode::DisplayInPane) {
            pluginView->setOutputData(runner->outputData());
            hasOutputInPane = !runner->outputData().isEmpty();
        }

        QString messageBody;
        MessageType messageType = MessageType::Info;
        if (!runner->errorData().isEmpty()) {
            messageBody += i18n("Data written to stderr:\n");
            messageBody += runner->errorData();
            messageBody += QStringLiteral("\n");
            messageType = MessageType::Warning;
        }
        if (crashed || exitCode != 0) {
            messageType = MessageType::Error;
        }

        // print crash or exit code
        if (crashed) {
            messageBody += i18n("%1 crashed", runner->tool()->translatedName());
        } else if (exitCode != 0) {
            messageBody += i18n("%1 finished with exit code %2", runner->tool()->translatedName(), exitCode);
        }

        // use generic output view for status
        Utils::showMessage(messageBody, QIcon::fromTheme(QStringLiteral("system-run")), i18n("External Tools"), messageType, pluginView->mainWindow());

        // on successful execution => show output
        // otherwise the global output pane settings will ensure we see the error output
        if (!(crashed || exitCode != 0) && hasOutputInPane) {
            pluginView->showToolView();
        }
    }

    delete runner;
}

int KateExternalToolsPlugin::configPages() const
{
    return 1;
}

KTextEditor::ConfigPage *KateExternalToolsPlugin::configPage(int number, QWidget *parent)
{
    if (number == 0) {
        return new KateExternalToolsConfigWidget(parent, this);
    }
    return nullptr;
}

void KateExternalToolsPlugin::registerPluginView(KateExternalToolsPluginView *view)
{
    Q_ASSERT(!m_views.contains(view));
    m_views.push_back(view);
}

void KateExternalToolsPlugin::unregisterPluginView(KateExternalToolsPluginView *view)
{
    Q_ASSERT(m_views.contains(view));
    m_views.removeAll(view);
}

KateExternalToolsPluginView *KateExternalToolsPlugin::viewForMainWindow(KTextEditor::MainWindow *mainWindow) const
{
    for (auto view : m_views) {
        if (view->mainWindow() == mainWindow) {
            return view;
        }
    }
    return nullptr;
}

#include "externaltoolsplugin.moc"
#include "moc_externaltoolsplugin.cpp"

// kate: space-indent on; indent-width 4; replace-tabs on;
