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

#include <QCompleter>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KLocalizedString>
#include <KMessageBox>

#include "dap/settings.h"
#include "json_placeholders.h"

#include <json_utils.h>

#ifdef WIN32
static const QLatin1Char pathSeparator(';');
#else
static const QLatin1Char pathSeparator(':');
#endif

void ConfigView::refreshUI()
{
    // first false then true to make sure a layout is set
    m_useBottomLayout = false;
    resizeEvent(nullptr);
    m_useBottomLayout = true;
    resizeEvent(nullptr);
}

std::optional<QJsonDocument> loadJSON(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        //         qCritical() << "file error: " << file.errorString();
        return std::nullopt;
    }
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError) {
        //         qCritical() << "JSON parse error: " << error.errorString();
        return std::nullopt;
    }
    return json;
}

ConfigView::ConfigView(QWidget *parent, KTextEditor::MainWindow *mainWin)
    : QWidget(parent)
    , m_mainWindow(mainWin)
{
    m_clientCombo = new QComboBox();
    m_clientCombo->setEditable(false);
    m_clientCombo->addItem(QStringLiteral("GDB"));
    m_clientCombo->insertSeparator(1);
    readDAPSettings();

    m_targetCombo = new QComboBox();
    m_targetCombo->setEditable(true);
    // don't let Qt insert items when the user edits; new targets are only
    // added when the user explicitly says so
    m_targetCombo->setInsertPolicy(QComboBox::NoInsert);
    m_targetCombo->setDuplicatesEnabled(true);

    m_addTarget = new QToolButton();
    m_addTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    m_addTarget->setToolTip(i18n("Add new target"));

    m_copyTarget = new QToolButton();
    m_copyTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-copy")));
    m_copyTarget->setToolTip(i18n("Copy target"));

    m_deleteTarget = new QToolButton();
    m_deleteTarget->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    m_deleteTarget->setToolTip(i18n("Delete target"));

    m_line = new QFrame(this);
    m_line->setFrameShadow(QFrame::Sunken);

    m_execLabel = new QLabel(i18n("Executable:"));
    m_execLabel->setBuddy(m_targetCombo);

    m_executable = new QLineEdit();
    QCompleter *completer1 = new QCompleter(this);
    QFileSystemModel *model = new QFileSystemModel(this);
    model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    completer1->setModel(model);
    m_executable->setCompleter(completer1);
    m_executable->setClearButtonEnabled(true);
    m_browseExe = new QToolButton(this);
    m_browseExe->setIcon(QIcon::fromTheme(QStringLiteral("application-x-executable")));

    m_workingDirectory = new QLineEdit();
    QCompleter *completer2 = new QCompleter(this);
    QFileSystemModel *model2 = new QFileSystemModel(completer2);

    completer2->setModel(model2);
    m_workingDirectory->setCompleter(completer2);
    m_workingDirectory->setClearButtonEnabled(true);
    m_workDirLabel = new QLabel(i18n("Working Directory:"));
    m_workDirLabel->setBuddy(m_workingDirectory);
    m_browseDir = new QToolButton(this);
    m_browseDir->setIcon(QIcon::fromTheme(QStringLiteral("inode-directory")));

    m_processId = new QSpinBox();
    m_processId->setMinimum(1);
    m_processId->setMaximum(4194304);
    m_processIdLabel = new QLabel(i18n("Process Id:"));
    m_processIdLabel->setBuddy(m_processId);

    m_arguments = new QLineEdit();
    m_arguments->setClearButtonEnabled(true);
    m_argumentsLabel = new QLabel(i18nc("Program argument list", "Arguments:"));
    m_argumentsLabel->setBuddy(m_arguments);

    m_takeFocus = new QCheckBox(i18nc("Checkbox to for keeping focus on the command line", "Keep focus"));
    m_takeFocus->setToolTip(i18n("Keep the focus on the command line"));

    m_redirectTerminal = new QCheckBox(i18n("Redirect IO"));
    m_redirectTerminal->setToolTip(i18n("Redirect the debugged programs IO to a separate tab"));

    m_advancedSettings = new QPushButton(i18n("Advanced Settings"));

    m_checBoxLayout = nullptr;

    // ensure layout is set
    refreshUI();

    m_advanced = new AdvancedGDBSettings(this);
    m_advanced->hide();

    connect(m_targetCombo, &QComboBox::editTextChanged, this, &ConfigView::slotTargetEdited);
    connect(m_targetCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConfigView::slotTargetSelected);
    connect(m_addTarget, &QToolButton::clicked, this, &ConfigView::slotAddTarget);
    connect(m_copyTarget, &QToolButton::clicked, this, &ConfigView::slotCopyTarget);
    connect(m_deleteTarget, &QToolButton::clicked, this, &ConfigView::slotDeleteTarget);
    connect(m_browseExe, &QToolButton::clicked, this, &ConfigView::slotBrowseExec);
    connect(m_browseDir, &QToolButton::clicked, this, &ConfigView::slotBrowseDir);
    connect(m_redirectTerminal, &QCheckBox::toggled, this, &ConfigView::showIO);
    connect(m_advancedSettings, &QPushButton::clicked, this, &ConfigView::slotAdvancedClicked);

    connect(m_clientCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigView::refreshUI);
}

ConfigView::~ConfigView()
{
}

void ConfigView::readDAPSettings()
{
    // read servers file
    const auto json = loadJSON(QStringLiteral(":/dapclient/servers.json"));
    if (!json)
        return;

    auto baseObject = json->object();

    {
        const QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QDir::separator() + QStringLiteral("gdbplugin")
            + QDir::separator() + QStringLiteral("dap.json");

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

void ConfigView::registerActions(KActionCollection *actionCollection)
{
    m_targetSelectAction = actionCollection->add<KSelectAction>(QStringLiteral("targets"));
    m_targetSelectAction->setText(i18n("Targets"));
    connect(m_targetSelectAction, &KSelectAction::indexTriggered, this, &ConfigView::slotTargetSelected);
}

void ConfigView::readConfig(const KConfigGroup &group)
{
    m_targetCombo->clear();

    int version = group.readEntry(QStringLiteral("version"), 4);
    int targetCount = group.readEntry(QStringLiteral("targetCount"), 1);
    int lastTarget = group.readEntry(QStringLiteral("lastTarget"), 0);
    QString targetKey(QStringLiteral("target_%1"));

    QStringList targetConfStrs;

    for (int i = 0; i < targetCount; i++) {
        targetConfStrs = group.readEntry(targetKey.arg(i), QStringList());
        if (targetConfStrs.count() == 0) {
            continue;
        }

        if ((version == 1) && (targetConfStrs.count() == 3)) {
            // valid old style config, translate it now; note the
            // reordering happening here!
            QStringList temp;
            temp << targetConfStrs[2];
            temp << targetConfStrs[1];
            targetConfStrs = temp;
        }

        if (version < 4) {
            targetConfStrs.prepend(targetConfStrs[0].right(15));
        }

        if (targetConfStrs.count() > NameIndex) {
            m_targetCombo->addItem(targetConfStrs[NameIndex], targetConfStrs);
        }
    }

    if (version < 4) {
        // all targets now have only one argument string
        int argListsCount = group.readEntry(QStringLiteral("argsCount"), 0);
        QString argsKey(QStringLiteral("args_%1"));
        QString targetName(QStringLiteral("%1<%2>"));

        QString argStr;
        int count = m_targetCombo->count();

        for (int i = 0; i < argListsCount; i++) {
            argStr = group.readEntry(argsKey.arg(i), QString());
            for (int j = 0; j < count; j++) {
                targetConfStrs = m_targetCombo->itemData(j).toStringList();
                if (i > 0) {
                    // copy the firsts and change the arguments
                    targetConfStrs[0] = targetName.arg(targetConfStrs[0]).arg(i + 1);
                    if (targetConfStrs.count() > 3) {
                        targetConfStrs[3] = argStr;
                    }
                    m_targetCombo->addItem(targetConfStrs[0], targetConfStrs);
                }
            }
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

    m_takeFocus->setChecked(group.readEntry("alwaysFocusOnInput", false));

    m_redirectTerminal->setChecked(group.readEntry("redirectTerminal", false));
}

void ConfigView::writeConfig(KConfigGroup &group)
{
    // make sure the data is up to date before writing
    saveCurrentToIndex(m_currentTarget);

    group.writeEntry("version", 4);

    QString targetKey(QStringLiteral("target_%1"));
    QStringList targetConfStrs;

    group.writeEntry("targetCount", m_targetCombo->count());
    group.writeEntry("lastTarget", m_targetCombo->currentIndex());
    for (int i = 0; i < m_targetCombo->count(); i++) {
        targetConfStrs = m_targetCombo->itemData(i).toStringList();
        group.writeEntry(targetKey.arg(i), targetConfStrs);
    }

    group.writeEntry("alwaysFocusOnInput", m_takeFocus->isChecked());
    group.writeEntry("redirectTerminal", m_redirectTerminal->isChecked());
}

const GDBTargetConf ConfigView::currentGDBTarget() const
{
    GDBTargetConf cfg;
    cfg.targetName = m_targetCombo->currentText();
    cfg.executable = m_executable->text();
    cfg.workDir = m_workingDirectory->text();
    cfg.arguments = m_arguments->text();
    cfg.customInit = m_advanced->configs();
    // Note: AdvancedGDBSettings::GDBIndex == 0
    if ((cfg.customInit.size() >= 0) && !cfg.customInit[0].isEmpty()) {
        cfg.gdbCmd = cfg.customInit[0];
        cfg.customInit.removeFirst();
    } else {
        cfg.gdbCmd = QStringLiteral("gdb");
    }
    // remove empty strings in the customInit
    int i = cfg.customInit.size() - 1;
    while (i >= 0) {
        if (cfg.customInit[i].isEmpty()) {
            cfg.customInit.removeAt(i);
        } else if (cfg.customInit[i].startsWith(QLatin1String("set directories "))) {
            QString paths = cfg.customInit[i];
            paths.remove(QStringLiteral("set directories "));
            cfg.srcPaths = paths.split(pathSeparator, Qt::SkipEmptyParts);
        }
        i--;
    }
    return cfg;
}

const static QString F_FILE = QStringLiteral("file");
const static QString F_WORKDIR = QStringLiteral("workdir");
const static QString F_ARGS = QStringLiteral("args");
const static QString F_PID = QStringLiteral("pid");

const DAPTargetConf ConfigView::currentDAPTarget(bool full) const
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
                    cfg.dapSettings = itP.value();
                }
                found = true;
                break;
            }
        }
    }
    const QStringList &variables = m_clientCombo->currentData().toStringList();
    for (const auto &field : variables) {
        // file
        if (field == F_FILE) {
            cfg.variables[F_FILE] = m_executable->text();
            // working dir
        } else if (field == F_WORKDIR) {
            cfg.variables[F_WORKDIR] = m_workingDirectory->text();
            // pid
        } else if (field == F_PID) {
            cfg.variables[F_PID] = m_processId->value();
            // arguments
        } else if (field == F_ARGS) {
            cfg.variables[F_ARGS] = m_arguments->text();
            // other
        } else if (m_dapFields.contains(field)) {
            cfg.variables[field] = m_dapFields[field].second->text();
        }
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

    if (clientIndex == 0) {
        setAdvancedOptions();
    }

    // Keep combo box and menu in sync
    m_targetCombo->setCurrentIndex(index);
    m_targetSelectAction->setCurrentItem(index);

    m_clientCombo->setCurrentIndex(clientIndex);
}

void ConfigView::slotAddTarget()
{
    QStringList targetConfStrs;

    targetConfStrs << i18n("Target %1", m_targetCombo->count() + 1);
    targetConfStrs << QString();
    targetConfStrs << QString();
    targetConfStrs << QString();

    m_targetCombo->addItem(targetConfStrs[NameIndex], targetConfStrs);
    m_targetCombo->setCurrentIndex(m_targetCombo->count() - 1);
}

void ConfigView::slotCopyTarget()
{
    QStringList tmp = m_targetCombo->itemData(m_targetCombo->currentIndex()).toStringList();
    if (tmp.empty()) {
        slotAddTarget();
        return;
    }
    tmp[NameIndex] = i18n("Target %1", m_targetCombo->count() + 1);
    m_targetCombo->addItem(tmp[NameIndex], tmp);
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

bool ConfigView::debuggerIsGDB() const
{
    return m_clientCombo->currentIndex() == 0;
}

void ConfigView::resizeEvent(QResizeEvent *)
{
    const bool toVertical = m_useBottomLayout && size().height() > size().width();
    const bool toHorizontal = !m_useBottomLayout && (size().height() < size().width());

    if (!toVertical && !toHorizontal)
        return;

    const bool is_dbg = debuggerIsGDB();
    const QStringList debuggerVariables = m_clientCombo->currentData().toStringList();

    // check if preformatted inputs are required

    m_advancedSettings->setVisible(is_dbg);

    // exe
    const bool needsExe = is_dbg || debuggerVariables.contains(F_FILE);
    m_execLabel->setVisible(needsExe);
    m_executable->setVisible(needsExe);
    m_browseExe->setVisible(needsExe);

    // working dir
    const bool needsWdir = is_dbg || debuggerVariables.contains(F_WORKDIR);
    m_workDirLabel->setVisible(needsWdir);
    m_workingDirectory->setVisible(needsWdir);
    m_browseDir->setVisible(needsWdir);

    // arguments
    const bool needsArgs = is_dbg || debuggerVariables.contains(F_ARGS);
    m_argumentsLabel->setVisible(needsArgs);
    m_arguments->setVisible(needsArgs);

    // pid
    const bool needsPid = !is_dbg && debuggerVariables.contains(F_PID);
    m_processIdLabel->setVisible(needsPid);
    m_processId->setVisible(needsPid);

    // additional dap fields
    for (auto it = m_dapFields.cbegin(); it != m_dapFields.cend(); ++it) {
        const bool visible = debuggerVariables.contains(it.key());
        it->first->setVisible(visible);
        it->second->setVisible(visible);
    }

    if (toVertical) {
        // Set layout for the side
        delete m_checBoxLayout;
        m_checBoxLayout = nullptr;
        delete layout();
        QGridLayout *layout = new QGridLayout(this);

        layout->addWidget(m_clientCombo, 0, 0);
        layout->addWidget(m_targetCombo, 1, 0);
        layout->addWidget(m_addTarget, 1, 1);
        layout->addWidget(m_copyTarget, 1, 2);
        layout->addWidget(m_deleteTarget, 1, 3);
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

            layout->addWidget(field.first, ++row, 0, Qt::AlignLeft);
            layout->addWidget(field.second, ++row, 0, 1, 4);
        }

        layout->addWidget(m_takeFocus, ++row, 0, 1, 4);
        layout->addWidget(m_redirectTerminal, ++row, 0, 1, 4);
        if (is_dbg) {
            layout->addWidget(m_advancedSettings, ++row, 0, 1, 4);
        }

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
        if (is_dbg) {
            m_checBoxLayout->addWidget(m_advancedSettings, 0);
        }

        QGridLayout *layout = new QGridLayout(this);
        layout->addWidget(m_clientCombo, 0, 0, 1, 6);

        layout->addWidget(m_targetCombo, 1, 0, 1, 3);

        layout->addWidget(m_addTarget, 2, 0);
        layout->addWidget(m_copyTarget, 2, 1);
        layout->addWidget(m_deleteTarget, 2, 2);

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

            layout->addWidget(field.first, ++row, 5, Qt::AlignRight);
            layout->addWidget(field.second, row, 6);
        }

        layout->addLayout(m_checBoxLayout, ++row, 5, 1, 3);

        layout->addItem(new QSpacerItem(1, 1), ++row, 0);
        layout->setColumnStretch(6, 100);
        layout->setRowStretch(row, 100);

        m_line->setFrameShape(QFrame::VLine);
        layout->addWidget(m_line, 1, 3, row - 1, 1);

        m_useBottomLayout = true;
    }
}

std::pair<QLabel *, QLineEdit *> &ConfigView::getDapField(const QString &fieldName)
{
    if (!m_dapFields.contains(fieldName)) {
        m_dapFields[fieldName] = std::make_pair(new QLabel(fieldName, this), new QLineEdit(this));
    }
    return m_dapFields[fieldName];
}

void ConfigView::setAdvancedOptions()
{
    QStringList tmp = m_targetCombo->itemData(m_targetCombo->currentIndex()).toStringList();

    // make sure we have enough strings;
    while (tmp.count() < CustomStartIndex) {
        tmp << QString();
    }

    if (tmp[GDBIndex].isEmpty()) {
        tmp[GDBIndex] = QStringLiteral("gdb");
    }

    // Remove the strings that are not part of the advanced settings
    for (int i = 0; i < GDBIndex; i++) {
        tmp.takeFirst();
    }

    m_advanced->setConfigs(tmp);
}

void ConfigView::slotAdvancedClicked()
{
    setAdvancedOptions();

    QStringList newList = m_targetCombo->itemData(m_targetCombo->currentIndex()).toStringList();
    // make sure we have enough strings;
    while (newList.count() < GDBIndex) {
        newList << QString();
    }
    // Remove old advanced settings
    while (newList.count() > GDBIndex) {
        newList.takeLast();
    }

    if (m_advanced->exec() == QDialog::Accepted) {
        // save the new values
        newList << m_advanced->configs();
        m_targetCombo->setItemData(m_targetCombo->currentIndex(), newList);
        Q_EMIT configChanged();
    }
}

void ConfigView::slotBrowseExec()
{
    QString exe = m_executable->text();

    if (m_executable->text().isEmpty()) {
        // try current document dir
        KTextEditor::View *view = m_mainWindow->activeView();

        if (view != nullptr) {
            exe = view->document()->url().toLocalFile();
        }
    }
    m_executable->setText(QFileDialog::getOpenFileName(nullptr, QString(), exe, QStringLiteral("application/x-executable")));
}

void ConfigView::slotBrowseDir()
{
    QString dir = m_workingDirectory->text();

    if (m_workingDirectory->text().isEmpty()) {
        // try current document dir
        KTextEditor::View *view = m_mainWindow->activeView();

        if (view != nullptr) {
            dir = view->document()->url().toLocalFile();
        }
    }
    m_workingDirectory->setText(QFileDialog::getExistingDirectory(this, QString(), dir));
}

QByteArray serializeHash(const QVariantHash map)
{
    const QJsonDocument doc(QJsonObject::fromVariantHash(map));
    return doc.toJson(QJsonDocument::Compact);
}

QJsonObject unserializeDAPVariables(const QString map)
{
    const auto doc = QJsonDocument::fromJson(map.toLatin1());
    return doc.object();
}

void ConfigView::saveCurrentToIndex(int index)
{
    if ((index < 0) || (index >= m_targetCombo->count())) {
        return;
    }

    QStringList tmp = m_targetCombo->itemData(index).toStringList();
    // make sure we have enough strings. The custom init strings are set in slotAdvancedClicked().
    while (tmp.count() < CustomStartIndex) {
        tmp << QString();
    }

    tmp[NameIndex] = m_targetCombo->itemText(index);
    if (debuggerIsGDB()) {
        tmp[ExecIndex] = m_executable->text();
        tmp[WorkDirIndex] = m_workingDirectory->text();
        tmp[ArgsIndex] = m_arguments->text();
    } else {
        const auto cfg = currentDAPTarget();
        tmp[DebuggerKey] = cfg.debugger;
        tmp[DebuggerProfile] = cfg.debuggerProfile;
        tmp[DAPVariables] = QString::fromLatin1(serializeHash(cfg.variables));
    }

    m_targetCombo->setItemData(index, tmp);
}

int ConfigView::loadFromIndex(int index)
{
    if ((index < 0) || (index >= m_targetCombo->count())) {
        return -1;
    }

    QStringList tmp = m_targetCombo->itemData(index).toStringList();
    // make sure we have enough strings. The custom init strings are set in slotAdvancedClicked().
    while (tmp.count() < CustomStartIndex) {
        tmp << QString();
    }

    const QString &debuggerKey = tmp[DebuggerKey];
    if (debuggerKey.isNull() || debuggerKey.isEmpty()) {
        // GDB
        m_executable->setText(tmp[ExecIndex]);
        m_workingDirectory->setText(tmp[WorkDirIndex]);
        m_arguments->setText(tmp[ArgsIndex]);

        return 0;
    } else {
        // DAP
        const QString &debuggerKey = tmp[DebuggerKey];
        if (!m_dapAdapterSettings.contains(debuggerKey))
            return -1;
        const QString &debuggerProfile = tmp[DebuggerProfile];
        if (!m_dapAdapterSettings[debuggerKey].contains(debuggerProfile))
            return -1;
        auto map = unserializeDAPVariables(tmp[DAPVariables]);

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
            field.second->setText(it.value().toString());
        }

        return m_dapAdapterSettings[debuggerKey][debuggerProfile].index;
    }
}
