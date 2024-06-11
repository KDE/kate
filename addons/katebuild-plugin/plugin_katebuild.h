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

#include "diagnostics/diagnosticview.h"
#include "targets.h"
#include "ui_build.h"

class QCMakeFileApi;

/******************************************************************/
class KateBuildView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)
    Q_PROPERTY(QUrl docUrl READ docUrl)

public:
    enum ResultDetails { FullOutput, ParsedOutput, ErrorsAndWarnings, OnlyErrors };

    enum TreeWidgetRoles { ErrorRole = Qt::UserRole + 1, DataRole };

    enum class Category { Normal, Info, Warning, Error };

    KateBuildView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw);
    ~KateBuildView() override;

    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;

    bool buildCurrentTarget();

    QUrl docUrl();

    void sendError(const QString &);

public Q_SLOTS:
    void loadCMakeTargets(const QString &cmakeFile);

private Q_SLOTS:

    // Building
    void slotSelectTarget();
    void slotBuildSelectedTarget();
    void slotBuildAndRunSelectedTarget();
    void slotBuildPreviousTarget();
    void slotCompileCurrentFile();
    bool slotStop();

    void slotLoadCMakeTargets();

    // Parse output
    void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
    void slotReadReadyStdErr();
    void slotReadReadyStdOut();
    void slotRunAfterBuild();
    void updateTextBrowser();

    // Settings
    void targetSetNew();
    void targetOrSetClone();
    void targetDelete();

    void slotAddTargetClicked();

    void handleEsc(QEvent *e);

    /**
     * keep track if the project plugin is alive and if the project map did change
     */
    void slotPluginViewCreated(const QString &name, QObject *pluginView);
    void slotPluginViewDeleted(const QString &name, QObject *pluginView);
    void slotProjectMapChanged();

    /**
     * Read the non-session configurations
     */
    void readConfig();

    void writeConfig();

    /**
     * Save the project build target updates
     */
    void saveProjectTargets();

    void enableCompileCurrentFile();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

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

    OutputLine processOutputLine(QString line);
    QString toOutputHtml(const KateBuildView::OutputLine &out);
    void addError(const OutputLine &err);
    void updateDiagnostics(Diagnostic diagnostic, const QUrl uri);
    void clearDiagnostics();
    bool startProcess(const QString &dir, const QString &command);
    bool checkLocal(const QUrl &dir);
    void clearBuildResults();
    QString parseWorkDir(QString dir) const;

    void displayBuildResult(const QString &message, KTextEditor::Message::MessageType level);
    void displayMessage(const QString &message, KTextEditor::Message::MessageType level);

    void addProjectTargets();
    QModelIndex createCMakeTargetSet(QModelIndex setIndex, const QString &name, const QCMakeFileApi &cmakeFA, const QString &cmakeConfig);

    /** Check if given command line is allowed to be executed.
     * Might ask the user for permission.
     * @param cmdline full command line including program to check
     * @return execution allowed?
     */
    bool isCommandLineAllowed(const QStringList &cmdline);

    QString findCompileCommands(const QString& file) const;
    CompileCommands parseCompileCommandsFile(const QString& compileCommandsFile) const;

    KTextEditor::MainWindow *m_win;
    QWidget *m_toolView;
    Ui::build m_buildUi{};
    QWidget *m_buildWidget;
    TargetsUi *m_targetsUi;
    KProcess m_proc;
    QString m_stdOut;
    QString m_stdErr;
    QString m_htmlOutput;
    int m_scrollStopPos = -1;
    int m_numOutputLines = 0;
    QTimer m_outputTimer;
    QString m_currentlyBuildingTarget;
    bool m_buildCancelled;
    bool m_runAfterBuild = false;
    QString m_makeDir;
    QStack<QString> m_makeDirStack;
    QStringList m_searchPaths;
    QRegularExpression m_filenameDetector;
    QRegularExpression m_newDirDetector;
    unsigned int m_numErrors = 0;
    unsigned int m_numWarnings = 0;
    unsigned int m_numNotes = 0;
    QPersistentModelIndex m_previousIndex;
    QPointer<KTextEditor::Message> m_infoMessage;
    int m_projectTargetsetRow = 0;
    bool m_firstBuild = true;
    DiagnosticsProvider m_diagnosticsProvider;
    bool m_addDiagnostics = true;
    bool m_autoSwitchToOutput = true;
    QTimer m_saveProjTargetsTimer;
    bool m_addingProjTargets = false;

    // hash of allowed and blacklisted command lines
    std::map<QString, bool> m_commandLineToAllowedState;

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

Q_SIGNALS:
    void configChanged();
};
