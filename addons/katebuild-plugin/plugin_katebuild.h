#pragma once
/* plugin_katebuild.h                    Kate Plugin
**
** SPDX-FileCopyrightText: 2008-2015 Kåre Särs <kare.sars@iki.fi>
**
** This code is almost a total rewrite of the GPL'ed Make plugin
** by Adriaan de Groot.
*/

/*
** SPDX-License-Identifier: GPL-2.0-or-later
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/

#include <KProcess>
#include <QFileInfo>
#include <QHash>
#include <QPointer>
#include <QRegularExpression>
#include <QStack>
#include <QString>
#include <QTimer>

#include <KTextEditor/ConfigPage>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/View>

#include <KConfigGroup>
#include <KXMLGUIClient>
#include <QTextDocument>

#include "StatusOverlay.h"
#include "diagnostics/diagnosticview.h"
#include "targets.h"
#include "ui_build.h"

#include <optional>

class KateBuildPlugin;

class QCMakeFileApi;

/******************************************************************/
class KateBuildView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)
    Q_PROPERTY(QUrl docUrl READ docUrl)

public:
    enum ResultDetails {
        FullOutput,
        ParsedOutput,
        ErrorsAndWarnings,
        OnlyErrors,
    };

    enum TreeWidgetRoles {
        ErrorRole = Qt::UserRole + 1,
        DataRole,
    };

    enum class Category {
        Normal,
        Info,
        Warning,
        Error,
    };

    KateBuildView(KateBuildPlugin *plugin, KTextEditor::MainWindow *mw);
    ~KateBuildView() override;

    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;

    QUrl docUrl();

    void sendError(const QString &);

public Q_SLOTS:
    void loadCMakeTargets(const QString &cmakeFile);

private Q_SLOTS:

    // Building
    bool trySetCommands();
    void slotSelectTarget();
    std::optional<QString> cmdSubstitutionsApplied(const QString &command, const QFileInfo &docFileInfo, const QString &workDir);
    void slotUpdateRunTabs();
    void slotRunAfterBuild();
    void slotBuildSelectedTarget();
    void slotBuildAndRunSelectedTarget();
    void slotReBuildPreviousTarget();
    void slotCompileCurrentFile();
    bool slotStop();

    void slotLoadCMakeTargets();

    // Parse output
    void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
    void slotReadReadyStdOut();
    void slotUpdateTextBrowser();

    void slotSearchBuildOutput();
    void slotSearchPatternChanged();

    void handleEsc(QEvent *e);
    void tabForToolViewAdded(QWidget *toolView, QWidget *tab);

    /**
     * keep track if the project plugin is alive and if the project map did change
     */
    void slotPluginViewCreated(const QString &name, QObject *pluginView);
    void slotPluginViewDeleted(const QString &name, QObject *pluginView);
    void slotProjectMapEdited();
    void slotProjectChanged();

    /**
     * Save the project build target updates
     */
    void saveProjectTargets();

    void enableCompileCurrentFile();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void doSearchAll(QString text);
    void gotoNthFound(qsizetype n);

private:
    struct OutputLine {
        Category category = Category::Normal;
        QString lineStr;
        QString message;
        QString file;
        int lineNr;
        int column;
    };

    // Support for compile_commands.json
    struct CompileCommand {
        QString workingDir;
        QString command;
    };

    struct CompileCommands {
        std::map<QString /* file*/, CompileCommand> commands;
        QString filename;
        QDateTime date;
    };

    CompileCommands m_parsedCompileCommands;

    void buildSelectedTarget();
    OutputLine processOutputLine(const QString &line);
    QString toOutputHtml(const KateBuildView::OutputLine &out);
    void addError(const OutputLine &err);
    void updateDiagnostics(Diagnostic diagnostic, QUrl uri);
    void clearDiagnostics();
    bool startProcess(const QString &dir, const QString &command);
    bool checkLocal(const QUrl &dir);
    void clearBuildResults();
    QString parseWorkDir(QString dir) const;

    void displayBuildResult(const QString &message, KTextEditor::Message::MessageType level);
    void displayMessage(const QString &message, KTextEditor::Message::MessageType level);
    void displayProgress(const QString &message, KTextEditor::Message::MessageType level);

    void updateProjectTargets();
    QModelIndex createCMakeTargetSet(QModelIndex setIndex, const QString &name, const QCMakeFileApi &cmakeFA, const QString &cmakeConfig);

    /** Check if given command line is allowed to be executed.
     * Might ask the user for permission.
     * @param cmdline full command line including program to check
     * @return execution allowed?
     */
    bool isCommandLineAllowed(const QStringList &cmdline);

    QString findCompileCommands(const QString &file) const;
    CompileCommands parseCompileCommandsFile(const QString &compileCommandsFile) const;

    void updateStatusOverlay();

    KateBuildPlugin *const m_plugin;
    KTextEditor::MainWindow *const m_win;
    QWidget *m_toolView = nullptr;
    QPointer<StatusOverlay> m_tabStatusOverlay = nullptr;
    QPointer<StatusOverlay> m_buildStatusOverlay = nullptr;
    bool m_buildStatusSeen = false;
    Ui::build m_buildUi{};
    QWidget *m_buildWidget = nullptr;
    TargetsUi *m_targetsUi = nullptr;
    KProcess m_proc;
    QString m_stdOut;
    QString m_stdErr;
    QString m_pendingHtmlOutput;
    int m_scrollStopLine = -1;
    int m_numOutputLines = 0;
    int m_numNonUpdatedLines = 0;

    QTimer m_outputTimer;
    QString m_buildTargetSetName;
    QString m_buildTargetName;
    QString m_buildBuildCmd;
    QString m_buildRunCmd;
    QString m_buildWorkDir;
    bool m_buildCancelled = false;

    QString m_makeDir;
    QStack<QString> m_makeDirStack;

    QStringList m_searchPaths;
    QRegularExpression m_filenameDetector;
    QRegularExpression m_newDirDetector;
    uint64_t m_numErrors = 0;
    uint64_t m_numWarnings = 0;
    uint64_t m_numNotes = 0;
    int32_t m_exitCode = 0;
    QString m_progress;

    QPersistentModelIndex m_previousIndex;
    QPointer<KTextEditor::Message> m_infoMessage;
    QPointer<KTextEditor::Message> m_progressMessage;
    int m_projectTargetsetRow = 0;
    bool m_firstBuild = true;
    DiagnosticsProvider m_diagnosticsProvider;
    QTimer m_saveProjTargetsTimer;
    bool m_addingProjTargets = false;
    QSet<QString> m_saveProjectTargetDirs;

    QList<QTextCursor> m_searchFound;
    qsizetype m_currentFound = 0;

    /**
     * current project plugin view, if any
     */
    QObject *m_projectPluginView = nullptr;
};

typedef QVariantList VariantList;

/******************************************************************/
class KateBuildPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateBuildPlugin(QObject *parent = nullptr, const VariantList & = VariantList());
    ~KateBuildPlugin() override
    {
    }

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    /**
     * Read the non-session configurations
     */
    void readConfig();

    void writeConfig() const;

    bool m_addDiagnostics = true;
    bool m_autoSwitchToOutput = true;
    bool m_showBuildProgress = true;

    // hash of allowed and blacklisted command lines
    std::map<QString, bool> m_commandLineToAllowedState;
};
