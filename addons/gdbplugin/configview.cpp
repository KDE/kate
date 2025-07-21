//
// configview.cpp
//
// Description: View for configuring the set of targets to be used with the debugger
//
//
// SPDX-FileCopyrightText: 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// SPDX-FileCopyrightText: 2012 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "configview.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSelectAction>

#include "dap/settings.h"
#include "json_placeholders.h"
#include "launch_json_reader.h"
#include "plugin_kategdb.h"
#include "sessionconfig.h"
#include "target_json_keys.h"
#include <json_utils.h>
#include <ktexteditor_utils.h>

using namespace TargetKeys;

void ConfigView::refreshUI()
{
    // first false then true to make sure a layout is set
    m_useBottomLayout = false;
    resizeEvent(nullptr);
    m_useBottomLayout = true;
    resizeEvent(nullptr);
}

static std::optional<QJsonDocument> loadJSON(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError) {
        return std::nullopt;
    }
    return json;
}

ConfigView::ConfigView(QWidget *parent, KTextEditor::MainWindow *mainWin, KatePluginGDB *plugin, KSelectAction *targetsAction)
    : QWidget(parent)
    , m_mainWindow(mainWin)
{
    setTargetsAction(targetsAction);
    m_clientCombo = new QComboBox(this);
    m_clientCombo->setEditable(false);
    m_dapConfigPath = plugin->configPath();
    readDAPSettings();

    m_targetCombo = new QComboBox(this);
    m_targetCombo->setEditable(true);
    // don't let Qt insert items when the user edits; new targets are only
    // added when the user explicitly says so
    m_targetCombo->setInsertPolicy(QComboBox::NoInsert);
    m_targetCombo->setDuplicatesEnabled(true);

    m_addTarget = new QToolButton(this);
    m_addTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    m_addTarget->setToolTip(i18n("Add new target"));

    m_copyTarget = new QToolButton(this);
    m_copyTarget->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    m_copyTarget->setToolTip(i18n("Copy target"));

    m_deleteTarget = new QToolButton(this);
    m_deleteTarget->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    m_deleteTarget->setToolTip(i18n("Delete target"));

    m_reloadLaunchJsonTargets = new QToolButton(this);
    m_reloadLaunchJsonTargets->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_reloadLaunchJsonTargets->setToolTip(i18n("Reload launch.json targets"));

    m_line = new QFrame(this);
    m_line->setFrameShadow(QFrame::Sunken);

    m_execLabel = new QLabel(i18n("Executable:"), this);
    m_execLabel->setBuddy(m_targetCombo);

    m_executable = new QLineEdit(this);
    auto *completer1 = new QCompleter(this);
    auto *model = new QFileSystemModel(this);
    model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    completer1->setModel(model);
    m_executable->setCompleter(completer1);
    m_executable->setClearButtonEnabled(true);
    m_browseExe = new QToolButton(this);
    m_browseExe->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));

    m_workingDirectory = new QLineEdit(this);
    auto *completer2 = new QCompleter(this);
    auto *model2 = new QFileSystemModel(completer2);

    completer2->setModel(model2);
    m_workingDirectory->setCompleter(completer2);
    m_workingDirectory->setClearButtonEnabled(true);
    m_workDirLabel = new QLabel(i18n("Working Directory:"), this);
    m_workDirLabel->setBuddy(m_workingDirectory);
    m_browseDir = new QToolButton(this);
    m_browseDir->setIcon(QIcon::fromTheme(QStringLiteral("inode-directory")));

    m_processId = new QSpinBox(this);
    m_processId->setMinimum(1);
    m_processId->setMaximum(4194304);
    m_processIdLabel = new QLabel(i18n("Process Id:"), this);
    m_processIdLabel->setBuddy(m_processId);

    m_arguments = new QLineEdit(this);
    m_arguments->setClearButtonEnabled(true);
    m_argumentsLabel = new QLabel(i18nc("Program argument list", "Arguments:"));
    m_argumentsLabel->setBuddy(m_arguments);

    m_takeFocus = new QCheckBox(i18nc("Checkbox to for keeping focus on the command line", "Keep focus"));
    m_takeFocus->setToolTip(i18n("Keep the focus on the command line"));

    m_redirectTerminal = new QCheckBox(i18n("Redirect IO"), this);
    m_redirectTerminal->setToolTip(i18n("Redirect the debugged programs IO to a separate tab"));

    m_checBoxLayout = nullptr;

    // ensure layout is set
    refreshUI();

    connect(m_targetCombo, &QComboBox::editTextChanged, this, &ConfigView::slotTargetEdited);
    connect(m_targetCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConfigView::slotTargetSelected);
    connect(m_addTarget, &QToolButton::clicked, this, &ConfigView::slotAddTarget);
    connect(m_copyTarget, &QToolButton::clicked, this, &ConfigView::slotCopyTarget);
    connect(m_deleteTarget, &QToolButton::clicked, this, &ConfigView::slotDeleteTarget);
    connect(m_reloadLaunchJsonTargets, &QCheckBox::clicked, this, &ConfigView::readTargetsFromLaunchJson);
    connect(m_browseExe, &QToolButton::clicked, this, &ConfigView::slotBrowseExec);
    connect(m_browseDir, &QToolButton::clicked, this, &ConfigView::slotBrowseDir);
    connect(m_redirectTerminal, &QCheckBox::toggled, this, &ConfigView::showIO);

    connect(m_clientCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigView::refreshUI);
}

ConfigView::~ConfigView()
{
}

void ConfigView::readDAPSettings()
{
    // read servers file
    const auto json = loadJSON(QStringLiteral(":/debugger/dap.json"));
    if (!json)
        return;

    auto baseObject = json->object();

    {
        const QString settingsPath = m_dapConfigPath.toLocalFile();

        const auto userJson = loadJSON(settingsPath);
        if (userJson) {
            baseObject = json::merge(baseObject, userJson->object());
        }
    }

    const auto servers = baseObject[QStringLiteral("dap")].toObject();

    int index = m_clientCombo->count();

    for (auto itServer = servers.constBegin(); itServer != servers.constEnd(); ++itServer) {
        const auto server = itServer.value().toObject();
        const auto jsonProfiles = dap::settings::expandConfigurations(server);
        if (!jsonProfiles)
            continue;

        QHash<QString, DAPAdapterSettings> profiles;

        for (auto itProfile = jsonProfiles->constBegin(); itProfile != jsonProfiles->constEnd(); ++itProfile) {
            profiles[itProfile.key()] = DAPAdapterSettings();
            DAPAdapterSettings &conf = profiles[itProfile.key()];
            conf.settings = itProfile->toObject();
            conf.index = index++;

            QSet<QString> variables;
            json::findVariables(conf.settings, variables);

            for (const auto &var : variables) {
                if (var.startsWith(QStringLiteral("#")))
                    continue;
                conf.variables.append(var);
            }

            m_clientCombo->addItem(QStringLiteral("%1 | %2").arg(itServer.key()).arg(itProfile.key()), conf.variables);
        }

        m_dapAdapterSettings[itServer.key()] = profiles;

        m_clientCombo->insertSeparator(index++);
    }
}

void ConfigView::readTargetsFromLaunchJson()
{
    // remove the first dummy target
    if (m_targetCombo->count() == 1) {
        auto json = m_targetCombo->itemData(0).toJsonObject();
        const QString file = json.value(F_FILE).toString();
        const QString args = json.value(F_ARGS).toString();
        const QString cwd = json.value(F_WORKDIR).toString();
        if (file.isEmpty() && args.isEmpty() && cwd.isEmpty()) {
            m_targetCombo->removeItem(0);
        }
    }

    QObject *project = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    if (!project) {
        return;
    }
    const auto property = project->property("allProjects");
    if (!property.isValid()) {
        return;
    }

    const QStringList allProjectBaseDirs = property.value<QMap<QString, QString>>().keys();
    const QList<QJsonValue> configurations = readLaunchJsonConfigs(allProjectBaseDirs);
    for (const auto &configValue : configurations) {
        QJsonObject configObject = configValue.toObject();
        const QString name = configObject.value(QLatin1String("name")).toString();
        const QString request = configObject.value(QLatin1String("request")).toString();
        if (name.isEmpty() || request != QLatin1String("launch")) {
            continue;
        }

        // Add item only if such an item doesn't exist
        int existingItemIndex = m_targetCombo->findData(configObject);
        if (existingItemIndex == -1) {
            m_targetCombo->addItem(name, configObject);
        }
    }

    if (m_targetCombo->count() == 0) {
        slotAddTarget();
    }
}

void ConfigView::clearClosedProjectLaunchJsonTargets(const QString &baseDir, const QString &name)
{
    Q_UNUSED(name)
    // Remove all targets whose project is closed
    for (int i = 0; i < m_targetCombo->count(); ++i) {
        const auto targetConf = m_targetCombo->itemData(i).toJsonObject();
        if (!targetConf.value(F_IS_LAUNCH_JSON).toBool()) {
            continue;
        }
        QString projectBaseDir = targetConf[F_LAUNCH_JSON_PROJECT].toString();
        if (projectBaseDir == baseDir) {
            m_targetCombo->removeItem(i);
            i--;
        }
    }
}

void ConfigView::setTargetsAction(KSelectAction *action)
{
    m_targetSelectAction = action;
    connect(m_targetSelectAction, &KSelectAction::indexTriggered, this, &ConfigView::slotTargetSelected);
}

void ConfigView::readConfig(const DebugPluginSessionConfig::ConfigData &config)
{
    m_targetCombo->clear();

    int lastTarget = config.lastTarget;
    const QString targetKey(QStringLiteral("target_%1"));

    for (const auto &targetConf : config.targetConfigs) {
        if (!targetConf.isEmpty()) {
            m_targetCombo->addItem(targetConf[QStringLiteral("target")].toString(), targetConf);
        }
    }

    // make sure there is at least one item.
    if (m_targetCombo->count() == 0) {
        slotAddTarget();
    }

    QStringList targetNames;
    for (int i = 0; i < m_targetCombo->count(); i++) {
        targetNames << m_targetCombo->itemText(i);
    }
    m_targetSelectAction->setItems(targetNames);

    if (lastTarget < 0 || lastTarget >= m_targetCombo->count()) {
        lastTarget = 0;
    }
    m_targetCombo->setCurrentIndex(lastTarget);
    m_takeFocus->setChecked(config.alwaysFocusOnInput);
    m_redirectTerminal->setChecked(config.redirectTerminal);

    initProjectPlugin();
}

void ConfigView::writeConfig(DebugPluginSessionConfig::ConfigData &config)
{
    // make sure the data is up to date before writing
    saveCurrentToIndex(m_currentTarget);

    config.lastTarget = m_targetCombo->currentIndex();
    // group.writeEntry("lastTarget", m_targetCombo->currentIndex());
    int targetIdx = 0;
    for (int i = 0; i < m_targetCombo->count(); i++) {
        QJsonObject targetConf = m_targetCombo->itemData(i).toJsonObject();
        if (targetConf.value(F_IS_LAUNCH_JSON).toBool()) {
            // skip objects from launch.json
            continue;
        }
        config.targetConfigs.push_back(targetConf);
    }
    config.targetCount = targetIdx;
    config.alwaysFocusOnInput = m_takeFocus->isChecked();
    config.redirectTerminal = m_redirectTerminal->isChecked();
}

DAPTargetConf ConfigView::currentDAPTarget(bool full, QString &errorMessage) const
{
    DAPTargetConf cfg;
    cfg.targetName = m_targetCombo->currentText();

    const int comboIndex = m_clientCombo->currentIndex();
    bool found = false;
    // find config
    for (auto itS = m_dapAdapterSettings.constBegin(); !found && itS != m_dapAdapterSettings.constEnd(); ++itS) {
        for (auto itP = itS->constBegin(); itP != itS->constEnd(); ++itP) {
            if (itP->index == comboIndex) {
                cfg.debugger = itS.key();
                cfg.debuggerProfile = itP.key();
                if (full) {
                    auto dapSettings = itP.value();
                    const auto data = m_targetCombo->currentData().toJsonObject();
                    // merge data from launch.json except for the fields that are already there
                    if (data.value(F_IS_LAUNCH_JSON).toBool()) {
                        auto &settings = dapSettings.settings;
                        auto request = settings[dap::settings::REQUEST].toObject();
                        for (auto it = data.begin(); it != data.end(); ++it) {
                            if (!request.contains(it.key())) {
                                request[it.key()] = it.value();
                            }
                        }
                        settings[dap::settings::REQUEST] = request;
                    }
                    dapSettings.settings[dap::settings::RUN_IN_TERMINAL] = true;
                    cfg.dapSettings = dapSettings;
                }
                found = true;
                break;
            }
        }
    }
    auto editor = KTextEditor::Editor::instance();
    const auto view = m_mainWindow->activeView();
    auto expand = [&](QString text) {
        return full ? editor->expandText(text, view) : text;
    };
    const QStringList &variables = m_clientCombo->currentData().toStringList();
    for (const auto &field : variables) {
        // file
        if (field == F_FILE) {
            QString filePath = m_executable->text();

            if (filePath.isEmpty()) {
                errorMessage = i18nc("Error message", "No executable target set");
            }
            cfg.variables[F_FILE] = expand(filePath);
            // working dir
        } else if (field == F_WORKDIR) {
            cfg.variables[F_WORKDIR] = expand(m_workingDirectory->text());
            // pid
        } else if (field == F_PID) {
            cfg.variables[F_PID] = m_processId->value();
            if (m_processId->value() == 0) {
                errorMessage = i18nc("Error message", "Invalid process id");
            }
            // arguments
        } else if (field == F_ARGS) {
            cfg.variables[F_ARGS] = expand(m_arguments->text());
            // other
        } else if (m_dapFields.contains(field)) {
            cfg.variables[field] = expand(m_dapFields[field].input->text());
        }
    }
    // also support overlay of config with fragments in kateproject config
    // in essence, the current project is then also a toggle/configuration switch upon launch
    if (full) {
        // use the currently active view/document, which is the focus of user attention
        QVariant projectMap;
        if (view && view->document())
            projectMap = Utils::projectMapForDocument(view->document());
        const auto projectConfig = QJsonDocument::fromVariant(projectMap).object();
        const auto dapConfig = projectConfig.value(QStringLiteral("dap")).toObject();
        auto dc = dapConfig.value(cfg.debugger).toObject();
        // as previously expanded, merge both run and configurations parts
        auto top = dc.value(dap::settings::RUN).toObject();
        top = json::merge(top, dc.value(dap::settings::CONFIGURATIONS).toObject().value(cfg.debuggerProfile).toObject());
        // merge on top of general dap settings
        // note; these should be in current expanded form; with merged run/configuration
        auto &settings = cfg.dapSettings->settings;
        settings = json::merge(settings, top);

        // var expansion in cmdline
        auto cmdline = settings.value(dap::settings::COMMAND).toArray();
        QStringList cmd;
        for (const auto &e : cmdline) {
            cmd.push_back(expand(e.toString()));
        }
        settings[dap::settings::COMMAND] = QJsonArray::fromStringList(cmd);
    }
    return cfg;
}

bool ConfigView::takeFocusAlways() const
{
    return m_takeFocus->isChecked();
}

bool ConfigView::showIOTab() const
{
    return m_redirectTerminal->isChecked();
}

void ConfigView::slotTargetEdited(const QString &newText)
{
    QString newComboText(newText);
    for (int i = 0; i < m_targetCombo->count(); ++i) {
        if (i != m_targetCombo->currentIndex() && m_targetCombo->itemText(i) == newComboText) {
            newComboText = newComboText + QStringLiteral(" 2");
        }
    }
    int cursorPosition = m_targetCombo->lineEdit()->cursorPosition();
    m_targetCombo->setItemText(m_targetCombo->currentIndex(), newComboText);
    m_targetCombo->lineEdit()->setCursorPosition(cursorPosition);

    // rebuild the target menu
    QStringList targets;
    for (int i = 0; i < m_targetCombo->count(); ++i) {
        targets.append(m_targetCombo->itemText(i));
    }
    m_targetSelectAction->setItems(targets);
    m_targetSelectAction->setCurrentItem(m_targetCombo->currentIndex());
}

void ConfigView::slotTargetSelected(int index)
{
    if ((index < 0) || (index >= m_targetCombo->count())) {
        return;
    }

    if ((m_currentTarget > 0) && (m_currentTarget < m_targetCombo->count())) {
        saveCurrentToIndex(m_currentTarget);
    }

    const int clientIndex = loadFromIndex(index);
    if (clientIndex < 0)
        return;

    m_currentTarget = index;

    // Keep combo box and menu in sync
    m_targetCombo->setCurrentIndex(index);
    m_targetSelectAction->setCurrentItem(index);

    m_clientCombo->setCurrentIndex(clientIndex);
}

void ConfigView::slotAddTarget()
{
    QJsonObject targetConf;

    targetConf[F_TARGET] = i18n("Target %1", m_targetCombo->count() + 1);

    m_targetCombo->addItem(targetConf[F_TARGET].toString(), targetConf);
    m_targetCombo->setCurrentIndex(m_targetCombo->count() - 1);
}

void ConfigView::slotCopyTarget()
{
    QJsonObject tmp = m_targetCombo->itemData(m_targetCombo->currentIndex()).toJsonObject();
    if (tmp.isEmpty()) {
        slotAddTarget();
        return;
    }
    tmp[F_TARGET] = i18n("Target %1", m_targetCombo->count() + 1);
    m_targetCombo->addItem(tmp[F_TARGET].toString(), tmp);
    m_targetCombo->setCurrentIndex(m_targetCombo->count() - 1);
}

void ConfigView::slotDeleteTarget()
{
    m_targetCombo->blockSignals(true);
    int currentIndex = m_targetCombo->currentIndex();
    m_targetCombo->removeItem(currentIndex);
    if (m_targetCombo->count() == 0) {
        slotAddTarget();
    }

    const int clientIndex = loadFromIndex(m_targetCombo->currentIndex());
    m_targetCombo->blockSignals(false);

    if (clientIndex >= 0) {
        m_clientCombo->setCurrentIndex(clientIndex);
    }
}

void ConfigView::resizeEvent(QResizeEvent *)
{
    const bool toVertical = m_useBottomLayout && size().height() > size().width();
    const bool toHorizontal = !m_useBottomLayout && (size().height() < size().width());

    if (!toVertical && !toHorizontal)
        return;

    const QStringList debuggerVariables = m_clientCombo->currentData().toStringList();

    // check if preformatted inputs are required
    const bool needsExe = debuggerVariables.contains(F_FILE);
    const bool needsWdir = debuggerVariables.contains(F_WORKDIR);
    const bool needsArgs = debuggerVariables.contains(F_ARGS);
    const bool needsPid = debuggerVariables.contains(F_PID);

    if (toVertical) {
        // Set layout for the side
        delete m_checBoxLayout;
        m_checBoxLayout = nullptr;
        delete layout();
        auto *layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(m_clientCombo, 0, 0);
        layout->addWidget(m_targetCombo, 1, 0);
        layout->addWidget(m_addTarget, 1, 1);
        layout->addWidget(m_copyTarget, 1, 2);
        layout->addWidget(m_deleteTarget, 1, 3);
        layout->addWidget(m_reloadLaunchJsonTargets, 1, 4);

        m_line->setFrameShape(QFrame::HLine);
        layout->addWidget(m_line, 2, 0, 1, 4);

        int row = 3;

        if (needsExe) {
            layout->addWidget(m_execLabel, ++row, 0, Qt::AlignLeft);
            layout->addWidget(m_executable, ++row, 0, 1, 3);
            layout->addWidget(m_browseExe, row, 3);
        }

        if (needsWdir) {
            layout->addWidget(m_workDirLabel, ++row, 0, Qt::AlignLeft);
            layout->addWidget(m_workingDirectory, ++row, 0, 1, 3);
            layout->addWidget(m_browseDir, row, 3);
        }

        if (needsArgs) {
            layout->addWidget(m_argumentsLabel, ++row, 0, Qt::AlignLeft);
            layout->addWidget(m_arguments, ++row, 0, 1, 4);
        }

        if (needsPid) {
            layout->addWidget(m_processIdLabel, ++row, 0, Qt::AlignLeft);
            layout->addWidget(m_processId, ++row, 0, 1, 4);
        }

        for (const auto &fieldName : debuggerVariables) {
            if (fieldName == F_FILE)
                continue;
            if (fieldName == F_ARGS)
                continue;
            if (fieldName == F_PID)
                continue;
            if (fieldName == F_WORKDIR)
                continue;

            const auto &field = getDapField(fieldName);

            layout->addWidget(field.label, ++row, 0, Qt::AlignLeft);
            layout->addWidget(field.input, ++row, 0, 1, 4);
        }

        layout->addWidget(m_takeFocus, ++row, 0, 1, 4);
        layout->addWidget(m_redirectTerminal, ++row, 0, 1, 4);

        layout->addItem(new QSpacerItem(1, 1), ++row, 0);
        layout->setColumnStretch(0, 1);
        layout->setRowStretch(row, 1);

        m_useBottomLayout = false;
    } else if (toHorizontal) {
        // Set layout for the bottom
        delete m_checBoxLayout;
        delete layout();
        m_checBoxLayout = new QHBoxLayout();
        m_checBoxLayout->addWidget(m_takeFocus, 10);
        m_checBoxLayout->addWidget(m_redirectTerminal, 10);

        auto *layout = new QGridLayout(this);
        layout->addWidget(m_clientCombo, 0, 0, 1, 6);
        layout->addWidget(m_targetCombo, 1, 0, 1, 4);

        layout->addWidget(m_addTarget, 2, 0);
        layout->addWidget(m_copyTarget, 2, 1);
        layout->addWidget(m_deleteTarget, 2, 2);
        layout->addWidget(m_reloadLaunchJsonTargets, 2, 3);

        int row = 0;

        if (needsExe) {
            layout->addWidget(m_execLabel, ++row, 5, Qt::AlignRight);
            layout->addWidget(m_executable, row, 6);
            layout->addWidget(m_browseExe, row, 7);
        }

        if (needsWdir) {
            layout->addWidget(m_workDirLabel, ++row, 5, Qt::AlignRight);
            layout->addWidget(m_workingDirectory, row, 6);
            layout->addWidget(m_browseDir, row, 7);
        }

        if (needsArgs) {
            layout->addWidget(m_argumentsLabel, ++row, 5, Qt::AlignRight);
            layout->addWidget(m_arguments, row, 6, 1, 2);
        }

        if (needsPid) {
            layout->addWidget(m_processIdLabel, ++row, 5, Qt::AlignRight);
            layout->addWidget(m_processId, row, 6);
        }

        for (const auto &fieldName : debuggerVariables) {
            if (fieldName == F_FILE)
                continue;
            if (fieldName == F_ARGS)
                continue;
            if (fieldName == F_PID)
                continue;
            if (fieldName == F_WORKDIR)
                continue;

            const auto &field = getDapField(fieldName);

            layout->addWidget(field.label, ++row, 5, Qt::AlignRight);
            layout->addWidget(field.input, row, 6);
        }

        layout->addLayout(m_checBoxLayout, ++row, 5, 1, 3);

        layout->addItem(new QSpacerItem(1, 1), ++row, 0);
        layout->setColumnStretch(6, 100);
        layout->setRowStretch(row, 100);

        m_line->setFrameShape(QFrame::VLine);
        layout->addWidget(m_line, 1, 4, row - 1, 1);

        m_useBottomLayout = true;
    }

    if (toVertical || toHorizontal) {
        // exe
        m_execLabel->setVisible(needsExe);
        m_executable->setVisible(needsExe);
        m_browseExe->setVisible(needsExe);

        // working dir
        m_workDirLabel->setVisible(needsWdir);
        m_workingDirectory->setVisible(needsWdir);
        m_browseDir->setVisible(needsWdir);

        // arguments
        m_argumentsLabel->setVisible(needsArgs);
        m_arguments->setVisible(needsArgs);

        // pid
        m_processIdLabel->setVisible(needsPid);
        m_processId->setVisible(needsPid);

        // additional dap fields
        for (auto it = m_dapFields.cbegin(); it != m_dapFields.cend(); ++it) {
            const bool visible = debuggerVariables.contains(it.key());
            it->label->setVisible(visible);
            it->input->setVisible(visible);
        }
    }
}

ConfigView::Field &ConfigView::getDapField(const QString &fieldName)
{
    if (!m_dapFields.contains(fieldName)) {
        m_dapFields[fieldName] = Field{.label = new QLabel(fieldName, this), .input = new QLineEdit(this)};
    }
    return m_dapFields[fieldName];
}

void ConfigView::slotBrowseExec()
{
    QString exe = m_executable->text();

    if (m_executable->text().isEmpty()) {
        // try current document dir
        if (KTextEditor::View *view = m_mainWindow->activeView(); view && view->document()->url().isLocalFile()) {
            exe = QFileInfo(view->document()->url().toLocalFile()).absolutePath();
        }
    }
    m_executable->setText(QFileDialog::getOpenFileName(this, i18n("Select Executable"), exe));
}

void ConfigView::slotBrowseDir()
{
    QString dir = m_workingDirectory->text();

    if (m_workingDirectory->text().isEmpty()) {
        // try current document dir
        if (KTextEditor::View *view = m_mainWindow->activeView(); view && view->document()->url().isLocalFile()) {
            dir = QFileInfo(view->document()->url().toLocalFile()).absolutePath();
        }
    }
    m_workingDirectory->setText(QFileDialog::getExistingDirectory(this, i18n("Select Working Directory"), dir));
}

void ConfigView::saveCurrentToIndex(int index)
{
    if ((index < 0) || (index >= m_targetCombo->count())) {
        return;
    }

    QJsonObject tmp = m_targetCombo->itemData(index).toJsonObject();
    if (tmp.value(F_IS_LAUNCH_JSON).toBool()) {
        return;
    }

    tmp[F_TARGET] = m_targetCombo->itemText(index);
    QString error;
    const auto cfg = currentDAPTarget(/*full=*/false, error);
    tmp[F_DEBUGGER] = cfg.debugger;
    tmp[F_PROFILE] = cfg.debuggerProfile;
    tmp[QStringLiteral("variables")] = QJsonObject::fromVariantHash(cfg.variables);

    m_targetCombo->setItemData(index, tmp);
}

int ConfigView::loadFromIndex(int index)
{
    if ((index < 0) || (index >= m_targetCombo->count())) {
        return -1;
    }

    QJsonObject tmp = m_targetCombo->itemData(index).toJsonObject();
    // qDebug().noquote().nospace() << "Load from index" << QJsonDocument(tmp).toJson();
    // The custom init strings are set in slotAdvancedClicked().

    const QString debuggerKey = tmp[F_DEBUGGER].toString();
    if (!m_dapAdapterSettings.contains(debuggerKey))
        return -1;
    const QString &debuggerProfile = tmp[F_PROFILE].toString();
    const QHash<QString, DAPAdapterSettings> &debuggerProfiles = m_dapAdapterSettings[debuggerKey];
    if (debuggerProfiles.size() > 1 && !debuggerProfiles.contains(debuggerProfile))
        return -1;

    const bool isFromLaunchJson = tmp.value(F_IS_LAUNCH_JSON).toBool();

    auto map = isFromLaunchJson ? tmp : tmp[QStringLiteral("variables")].toObject();

    m_executable->setText(map[F_FILE].toString());
    map.remove(F_FILE);
    m_workingDirectory->setText(map[F_WORKDIR].toString());
    map.remove(F_WORKDIR);
    m_arguments->setText(map[F_ARGS].toString());
    map.remove(F_ARGS);
    m_processId->setValue(map[F_PID].toInt());
    map.remove(F_PID);

    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const auto &field = getDapField(it.key());
        field.input->setText(it.value().toString());
    }

    if (debuggerProfiles.size() == 1) {
        return debuggerProfiles.begin()->index;
    }

    return m_dapAdapterSettings[debuggerKey][debuggerProfile].index;
}

void ConfigView::initProjectPlugin()
{
    auto slot = [this](const QString &pluginName, QObject *pluginView) {
        if (pluginView && pluginName == QLatin1String("kateprojectplugin")) {
            connect(pluginView, SIGNAL(pluginProjectAdded(QString, QString)), this, SLOT(readTargetsFromLaunchJson()), Qt::UniqueConnection);
            connect(pluginView,
                    SIGNAL(pluginProjectRemoved(QString, QString)),
                    this,
                    SLOT(clearClosedProjectLaunchJsonTargets(QString, QString)),
                    Qt::UniqueConnection);
            readTargetsFromLaunchJson();
        }
    };
    QString projectPlugin = QLatin1String("kateprojectplugin");
    QObject *pluginView = m_mainWindow->pluginView(QLatin1String("kateprojectplugin"));
    slot(QLatin1String("kateprojectplugin"), pluginView);
    connect(m_mainWindow, &KTextEditor::MainWindow::pluginViewCreated, this, slot);
}

#include "moc_configview.cpp"
