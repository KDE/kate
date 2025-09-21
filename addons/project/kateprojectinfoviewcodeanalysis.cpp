/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectinfoviewcodeanalysis.h"
#include "hostprocess.h"
#include "kateproject.h"
#include "kateprojectcodeanalysistool.h"
#include "kateprojectpluginview.h"
#include "tools/codeanalysisselector.h"

#include "diagnostics/diagnostic_types.h"
#include "diagnostics/diagnosticview.h"
#include "ktexteditor_utils.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QToolTip>
#include <QVBoxLayout>

#include <KLocalizedString>

#include <KTextEditor/MainWindow>

KateProjectInfoViewCodeAnalysis::KateProjectInfoViewCodeAnalysis(KateProjectPluginView *pluginView, KateProject *project)
    : m_pluginView(pluginView)
    , m_project(project)
    , m_startStopAnalysis(new QPushButton(i18n("Start Analysis...")))
    , m_analyzer(nullptr)
    , m_analysisTool(nullptr)
    , m_toolSelector(new QComboBox())
    , m_toolInfoLabel(new QLabel(this))
    , m_diagnosticProvider(new DiagnosticsProvider(pluginView->mainWindow(), this))
{
    m_diagnosticProvider->setObjectName(QStringLiteral("CodeAnalysisDiagnosticProvider"));
    m_diagnosticProvider->name = i18nc("'%1' refers to project name, e.g,. Code Analysis - MyProject", "Code Analysis - %1", project->name());

    // We don't want the diagnostics to be cleared automatically if a file closes
    m_diagnosticProvider->setPersistentDiagnostics(true);

    /**
     * Connect selection change callback
     * and attach model to code analysis selector
     */
    connect(m_toolSelector,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &KateProjectInfoViewCodeAnalysis::slotToolSelectionChanged);
    m_toolSelector->setModel(KateProjectCodeAnalysisSelector::model(this));
    m_toolSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    /**
     * layout widget
     */
    auto *layout = new QVBoxLayout;
    // top: selector and buttons...
    auto *hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);
    hlayout->addWidget(m_toolSelector);
    hlayout->addWidget(m_startStopAnalysis);
    hlayout->addStretch();

    layout->addWidget(m_toolInfoLabel);

    // below: result list...
    layout->addStretch();
    setLayout(layout);

    /**
     * connect needed signals
     */
    connect(m_startStopAnalysis, &QPushButton::clicked, this, &KateProjectInfoViewCodeAnalysis::slotStartStopClicked);
}

KateProjectInfoViewCodeAnalysis::~KateProjectInfoViewCodeAnalysis()
{
    if (m_analyzer && m_analyzer->state() != QProcess::NotRunning) {
        m_analyzer->kill();
        m_analyzer->blockSignals(true);
        m_analyzer->waitForFinished();
    }
    delete m_analyzer;
}

void KateProjectInfoViewCodeAnalysis::slotToolSelectionChanged(int)
{
    m_analysisTool = m_toolSelector->currentData(Qt::UserRole + 1).value<KateProjectCodeAnalysisTool *>();
    if (m_analysisTool) {
        const QString fullExecutable = safeExecutableName(m_analysisTool->path());
        if (fullExecutable.isEmpty()) {
            m_startStopAnalysis->setEnabled(false);
            m_toolInfoLabel->setText(
                i18n("'%1' is not installed on your system, %2.<br/><br/>%3. The tool will be run on all project files which match this list of file "
                     "extensions:<br/><b>%4</b>",
                     m_analysisTool->name(),
                     m_analysisTool->notInstalledMessage(),
                     m_analysisTool->description(),
                     m_analysisTool->fileExtensions()));

        } else {
            m_startStopAnalysis->setEnabled(true);
            m_toolInfoLabel->setText(
                i18n("Using %1 installed at: %2.<br/><br/>%3. The tool will be run on all project files which match this list of file "
                     "extensions:<br/><b>%4</b>",
                     m_analysisTool->name(),
                     fullExecutable,
                     m_analysisTool->description(),
                     m_analysisTool->fileExtensions()));
        }
    }
}

void KateProjectInfoViewCodeAnalysis::slotStartStopClicked()
{
    /**
     * get files for the external tool
     */
    m_analysisTool = m_toolSelector->currentData(Qt::UserRole + 1).value<KateProjectCodeAnalysisTool *>();
    m_analysisTool->setProject(m_project);
    m_analysisTool->setMainWindow(m_pluginView->mainWindow());

    /**
     * clear existing entries
     */
    Q_EMIT m_diagnosticProvider->requestClearDiagnostics(m_diagnosticProvider);

    /**
     * launch selected tool
     */
    delete m_analyzer;
    m_analyzer = new QProcess;
    m_analyzer->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_analyzer, &QProcess::readyRead, this, &KateProjectInfoViewCodeAnalysis::slotReadyRead);
    connect(m_analyzer, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateProjectInfoViewCodeAnalysis::finished);

    // ensure we only run the code analyzer from PATH
    const QString fullExecutable = safeExecutableName(m_analysisTool->path());
    if (!fullExecutable.isEmpty()) {
        m_analyzer->setWorkingDirectory(m_project->baseDir());
        startHostProcess(*m_analyzer, fullExecutable, m_analysisTool->arguments());
    }

    if (fullExecutable.isEmpty() || !m_analyzer->waitForStarted()) {
        Utils::showMessage(m_analysisTool->notInstalledMessage(), {}, i18n("CodeAnalysis"), MessageType::Warning);
        return;
    }

    m_startStopAnalysis->setEnabled(false);

    /**
     * write files list and close write channel
     */
    const QString stdinMessage = m_analysisTool->stdinMessages();
    if (!stdinMessage.isEmpty()) {
        m_analyzer->write(stdinMessage.toLocal8Bit());
    }
    m_analyzer->closeWriteChannel();
}

void KateProjectInfoViewCodeAnalysis::slotReadyRead()
{
    /**
     * get results of analysis
     */
    m_errOutput = {};
    QHash<QUrl, QList<Diagnostic>> fileDiagnostics;
    while (m_analyzer->canReadLine()) {
        /**
         * get one line, split it, skip it, if too few elements
         */
        auto rawLine = m_analyzer->readLine();
        QString line = QString::fromLocal8Bit(rawLine);
        FileDiagnostics fd = m_analysisTool->parseLine(line);
        if (!fd.uri.isValid()) {
            m_errOutput += rawLine;
            continue;
        }
        fileDiagnostics[fd.uri] << fd.diagnostics;
    }

    for (auto it = fileDiagnostics.cbegin(); it != fileDiagnostics.cend(); ++it) {
        m_diagnosticProvider->diagnosticsAdded(FileDiagnostics{.uri = it.key(), .diagnostics = it.value()});
    }

    if (!fileDiagnostics.isEmpty()) {
        m_diagnosticProvider->showDiagnosticsView();
    }
}

void KateProjectInfoViewCodeAnalysis::finished(int exitCode, QProcess::ExitStatus)
{
    m_startStopAnalysis->setEnabled(true);

    if (m_analysisTool->isSuccessfulExitCode(exitCode)) {
        // normally 0 is successful but there are exceptions
        const QString msg = i18ncp(
            "Message to the user that analysis finished. %1 is the name of the program that did the analysis, %2 is a number. e.g., [clang-tidy]Analysis on 5 "
            "files finished",
            "[%1]Analysis on %2 file finished.",
            "[%1]Analysis on %2 files finished.",
            m_analysisTool->name(),
            m_analysisTool->getActualFilesCount());
        // We only log here because once the analysis starts, the user will be taken to diagnosticview to see the results
        Utils::showMessage(msg, {}, i18n("CodeAnalysis"), MessageType::Log, m_pluginView->mainWindow());
    } else {
        const QString err = QString::fromUtf8(m_errOutput);
        const QString message = i18n("Analysis failed with exit code %1, Error: %2", exitCode, err);
        Utils::showMessage(message, {}, i18n("CodeAnalysis"), MessageType::Error, m_pluginView->mainWindow());
    }
    m_errOutput = {};
}
