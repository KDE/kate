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
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QToolTip>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KMessageWidget>
#include <QTimer>

#include <KTextEditor/MainWindow>

void DocumentOnSaveTracker::setDocument(KTextEditor::Document *doc)
{
    if (doc == m_doc) {
        return;
    }

    if (m_doc) {
        disconnect(m_doc, &KTextEditor::Document::documentSavedOrUploaded, &m_timer, qOverload<>(&QTimer::start));
    }
    m_doc = doc;
    if (m_doc) {
        connect(m_doc, &KTextEditor::Document::documentSavedOrUploaded, &m_timer, qOverload<>(&QTimer::start));
    }
}

class ToolFilterProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        if (!m_activeDoc) {
            return false;
        }
        auto tool = sourceModel()->index(source_row, 0, source_parent).data(Qt::UserRole + 1).value<KateProjectCodeAnalysisTool *>();
        const auto file = m_activeDoc->url().toLocalFile();
        if (file.isEmpty()) {
            return false;
        }
        if (tool) {
            const auto extensions = tool->fileExtensions().split(QLatin1Char('|'));
            for (const auto &ext : extensions) {
                if (file.endsWith(ext)) {
                    return true;
                }
            }
        }
        return false;
    }

    void setActiveDoc(KTextEditor::Document *doc)
    {
        m_activeDoc = doc;
        invalidateFilter();
    }

private:
    QPointer<KTextEditor::Document> m_activeDoc;
};

KateProjectInfoViewCodeAnalysis::KateProjectInfoViewCodeAnalysis(KateProjectPluginView *pluginView, KateProject *project)
    : m_pluginView(pluginView)
    , m_project(project)
    , m_messageWidget(nullptr)
    , m_startStopAnalysis(new QPushButton(i18n("Start Analysis...")))
    , m_analyzer(nullptr)
    , m_analysisTool(nullptr)
    , m_toolSelector(new QComboBox())
    , m_proxyModel(new ToolFilterProxyModel(this))
    , m_diagnosticProvider(new DiagnosticsProvider(this))
{
    Utils::registerDiagnosticsProvider(m_diagnosticProvider, m_pluginView->mainWindow());

    /**
     * Connect selection change callback
     * and attach model to code analysis selector
     */
    connect(m_toolSelector,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &KateProjectInfoViewCodeAnalysis::slotToolSelectionChanged);
    m_proxyModel->setSourceModel(KateProjectCodeAnalysisSelector::model(this));
    m_toolSelector->setModel(m_proxyModel);
    m_toolSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    connect(m_pluginView->mainWindow(), &KTextEditor::MainWindow::viewChanged, this, [this](KTextEditor::View *v) {
        m_onSaveTracker.setDocument(v ? v->document() : nullptr);
        static_cast<ToolFilterProxyModel *>(m_proxyModel)->setActiveDoc(v ? v->document() : nullptr);
    });
    connect(&m_onSaveTracker, &DocumentOnSaveTracker::saved, this, &KateProjectInfoViewCodeAnalysis::onSaved);

    /**
     * layout widget
     */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    // top: selector and buttons...
    QHBoxLayout *hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);
    hlayout->setSpacing(0);
    hlayout->addWidget(m_toolSelector);
    auto infoButton = new QPushButton(QIcon::fromTheme(QStringLiteral("documentinfo")), QString(), this);
    infoButton->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    connect(infoButton, &QPushButton::clicked, this, [this]() {
        QToolTip::showText(QCursor::pos(), m_toolInfoText);
    });
    hlayout->addWidget(infoButton);
    hlayout->addWidget(m_startStopAnalysis);
    hlayout->addStretch();
    // below: result list...
    layout->addStretch();
    setLayout(layout);

    /**
     * connect needed signals
     */
    connect(m_startStopAnalysis, &QPushButton::clicked, this, [this] {
        m_invocationType = UserClickedButton;
        slotStartStopClicked();
    });
}

KateProjectInfoViewCodeAnalysis::~KateProjectInfoViewCodeAnalysis()
{
    m_onSaveTracker.setDocument(nullptr);
    Utils::unregisterDiagnosticsProvider(m_diagnosticProvider, m_pluginView->mainWindow());
    if (m_analyzer && m_analyzer->state() != QProcess::NotRunning) {
        m_analyzer->kill();
        m_analyzer->blockSignals(true);
        m_analyzer->waitForFinished();
    }
    delete m_analyzer;
}

void KateProjectInfoViewCodeAnalysis::slotToolSelectionChanged(int)
{
    auto oldTool = m_analysisTool;
    auto layout = static_cast<QVBoxLayout *>(this->layout());
    if (oldTool && oldTool->configWidget()) {
        oldTool->configWidget()->hide();
    }

    m_analysisTool = m_toolSelector->currentData(Qt::UserRole + 1).value<KateProjectCodeAnalysisTool *>();
    if (m_analysisTool) {
        m_toolInfoText = i18n("%1<br/><br/>The tool will be run on all project files which match this list of file extensions:<br/><br/><b>%2</b>",
                              m_analysisTool->description(),
                              m_analysisTool->fileExtensions());
        auto w = m_analysisTool->configWidget();
        if (w) {
            w->show();
            bool found = false;
            for (int i = 0; i < layout->count(); ++i) {
                auto item = layout->itemAt(i);
                if (item && item->widget() == w) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                layout->insertWidget(layout->count() - 1, w);
            }
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
    m_diagnosticProvider->requestClearDiagnostics(m_diagnosticProvider);

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

    if (m_messageWidget) {
        delete m_messageWidget;
        m_messageWidget = nullptr;
    }

    if (fullExecutable.isEmpty() || !m_analyzer->waitForStarted()) {
        m_messageWidget = new KMessageWidget(this);
        m_messageWidget->setCloseButtonVisible(true);
        m_messageWidget->setMessageType(KMessageWidget::Warning);
        m_messageWidget->setWordWrap(false);
        m_messageWidget->setText(m_analysisTool->notInstalledMessage());
        static_cast<QVBoxLayout *>(layout())->addWidget(m_messageWidget);
        m_messageWidget->animatedShow();
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

void KateProjectInfoViewCodeAnalysis::onSaved(KTextEditor::Document *)
{
    if (m_analyzer && m_analyzer->state() == QProcess::Running) {
        return;
    }
    m_invocationType = OnSave;

    KateProjectCodeAnalysisTool *toolToRun = nullptr;
    if (m_analysisTool && m_analysisTool->canRunOnSave()) {
        toolToRun = m_analysisTool;
    } else {
        for (int i = 0; i < m_toolSelector->count(); ++i) {
            auto tool = m_toolSelector->itemData(i, Qt::UserRole + 1).value<KateProjectCodeAnalysisTool *>();
            if (tool && tool->canRunOnSave()) {
                toolToRun = tool;
                m_toolSelector->setCurrentIndex(i);
                break;
            }
        }
    }
    if (toolToRun) {
        slotStartStopClicked();
    }
}

void KateProjectInfoViewCodeAnalysis::slotReadyRead()
{
    /**
     * get results of analysis
     */
    QHash<QUrl, QVector<Diagnostic>> fileDiagnostics;
    while (m_analyzer->canReadLine()) {
        /**
         * get one line, split it, skip it, if too few elements
         */
        QString line = QString::fromLocal8Bit(m_analyzer->readLine());
        FileDiagnostics fd = m_analysisTool->parseLine(line);
        if (!fd.uri.isValid()) {
            continue;
        }
        fileDiagnostics[fd.uri] << fd.diagnostics;

        continue;
    }

    for (auto it = fileDiagnostics.cbegin(); it != fileDiagnostics.cend(); ++it) {
        m_diagnosticProvider->diagnosticsAdded(FileDiagnostics{it.key(), it.value()});
    }

    if (m_invocationType == UserClickedButton && !fileDiagnostics.isEmpty()) {
        m_diagnosticProvider->showDiagnosticsView();
    }
}

void KateProjectInfoViewCodeAnalysis::finished(int exitCode, QProcess::ExitStatus)
{
    m_invocationType = None;
    m_startStopAnalysis->setEnabled(true);
    m_messageWidget = new KMessageWidget(this);
    m_messageWidget->setCloseButtonVisible(true);
    m_messageWidget->setWordWrap(false);

    if (m_analysisTool->isSuccessfulExitCode(exitCode)) {
        // normally 0 is successful but there are exceptions
        m_messageWidget->setMessageType(KMessageWidget::Information);
        m_messageWidget->setText(i18np("Analysis on %1 file finished.", "Analysis on %1 files finished.", m_analysisTool->getActualFilesCount()));

        // hide after 3 seconds
        QTimer::singleShot(3000, this, [this]() {
            if (m_messageWidget) {
                m_messageWidget->animatedHide();
            }
        });
    } else {
        // unfortunately, output was eaten by slotReadyRead()
        // TODO: get stderr output, show it here
        m_messageWidget->setMessageType(KMessageWidget::Warning);
        m_messageWidget->setText(i18np("Analysis on %1 file failed with exit code %2.",
                                       "Analysis on %1 files failed with exit code %2.",
                                       m_analysisTool->getActualFilesCount(),
                                       exitCode));
    }

    static_cast<QVBoxLayout *>(layout())->addWidget(m_messageWidget);
    m_messageWidget->animatedShow();
}
