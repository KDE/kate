/* plugin_katebuild.c                    Kate Plugin
**
** SPDX-FileCopyrightText: 2013 Alexander Neundorf <neundorf@kde.org>
** SPDX-FileCopyrightText: 2006-2015 Kåre Särs <kare.sars@iki.fi>
** SPDX-FileCopyrightText: 2011 Ian Wakeling <ian.wakeling@ntlworld.com>
**
** This code is mostly a modification of the GPL'ed Make plugin
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

#include "plugin_katebuild.h"

#include "AppOutput.h"
#include "buildconfig.h"
#include "hostprocess.h"
#include "kate_buildplugin_debug.h"
#include "qcmakefileapi.h"

#include <QAction>
#include <QApplication>
#include <QCompleter>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QRegularExpressionMatch>
#include <QScrollBar>
#include <QString>
#include <QThread>
#include <QTimer>

#include <KActionCollection>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>

#include <KAboutData>
#include <KColorScheme>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include <kateapp.h>
#include <kterminallauncherjob.h>
#include <ktexteditor_utils.h>

#include <kde_terminal_interface.h>
#include <kparts/part.h>

K_PLUGIN_FACTORY_WITH_JSON(KateBuildPluginFactory, "katebuildplugin.json", registerPlugin<KateBuildPlugin>();)

static const QString DefConfigCmd = QStringLiteral("cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../");
static const QString DefConfClean;
static const QString DefTargetName = QStringLiteral("build");
static const QString DefBuildCmd = QStringLiteral("make");
static const QString DefCleanCmd = QStringLiteral("make clean");
static const QString DiagnosticsPrefix = QStringLiteral("katebuild");
static const QString ConfigAllowedCommands = QStringLiteral("AllowedCommandLines");
static const QString ConfigBlockedCommands = QStringLiteral("BlockedCommandLines");

namespace
{
class BoolTrueLocker
{
public:
    bool &m_toLock;
    BoolTrueLocker(bool &toLock)
        : m_toLock(toLock)
    {
        m_toLock = true;
    }
    ~BoolTrueLocker()
    {
        m_toLock = false;
    }
};

}

#ifdef Q_OS_WIN
/******************************************************************/
static QString caseFixed(const QString &path)
{
    QStringList paths = path.split(QLatin1Char('/'));
    if (paths.isEmpty()) {
        return path;
    }

    QString result = paths[0].toUpper() + QLatin1Char('/');
    for (int i = 1; i < paths.count(); ++i) {
        QDir curDir(result);
        const QStringList items = curDir.entryList();
        int j;
        for (j = 0; j < items.size(); ++j) {
            if (items[j].compare(paths[i], Qt::CaseInsensitive) == 0) {
                result += items[j];
                if (i < paths.count() - 1) {
                    result += QLatin1Char('/');
                }
                break;
            }
        }
        if (j == items.size()) {
            return path;
        }
    }
    return result;
}

// clang-format off
#include <windows.h>
#include <Tlhelp32.h>
// clang-format on

static void KillProcessTree(DWORD myprocID)
{
    if (myprocID == 0) {
        return;
    }
    PROCESSENTRY32 procEntry;
    memset(&procEntry, 0, sizeof(PROCESSENTRY32));
    procEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (::Process32First(hSnap, &procEntry)) {
        do {
            if (procEntry.th32ParentProcessID == myprocID) {
                KillProcessTree(procEntry.th32ProcessID);
            }
        } while (::Process32Next(hSnap, &procEntry));
    }

    // kill the main process
    HANDLE hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, myprocID);

    if (hProc) {
        ::TerminateProcess(hProc, 1);
        ::CloseHandle(hProc);
    }
}

static void terminateProcess(KProcess &proc)
{
    KillProcessTree(proc.processId());
}

#else
static QString caseFixed(const QString &path)
{
    return path;
}

static void terminateProcess(KProcess &proc)
{
    proc.terminate();
}
#endif

struct ItemData {
    // ensure destruction, but not inadvertently so by a variant value copy
    std::shared_ptr<KTextEditor::MovingCursor> cursor;
};

Q_DECLARE_METATYPE(ItemData)

/******************************************************************/
KateBuildPlugin::KateBuildPlugin(QObject *parent, const VariantList &)
    : KTextEditor::Plugin(parent)
{
}

/******************************************************************/
QObject *KateBuildPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new KateBuildView(this, mainWindow);
}

/******************************************************************/
int KateBuildPlugin::configPages() const
{
    return 1;
}

/******************************************************************/
KTextEditor::ConfigPage *KateBuildPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }

    KateBuildConfigPage *configPage = new KateBuildConfigPage(parent);
    connect(configPage, &KateBuildConfigPage::configChanged, this, &KateBuildPlugin::configChanged);
    return configPage;
}

/******************************************************************/
KateBuildView::KateBuildView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw)
    : QObject(mw)
    , m_win(mw)
    , m_buildWidget(nullptr)
    , m_proc(this)
    , m_stdOut()
    , m_stdErr()
    , m_buildCancelled(false)
    // NOTE this will not allow spaces in file names.
    // e.g. from gcc: "main.cpp:14: error: cannot convert ‘std::string’ to ‘int’ in return"
    // e.g. from gcc: "main.cpp:14:8: error: cannot convert ‘std::string’ to ‘int’ in return"
    // e.g. from icpc: "main.cpp(14): error: no suitable conversion function from "std::string" to "int" exists"
    // e.g. from clang: ""main.cpp(14,8): fatal error: 'boost/scoped_array.hpp' file not found"
    , m_filenameDetector(QStringLiteral("(?<filename>(?:[a-np-zA-Z]:[\\\\/])?[^\\s:(]+)[:(](?<line>\\d+)[,:]?(?<column>\\d+)?[):]*\\s*(?<message>.*)"))
    , m_newDirDetector(QStringLiteral("make\\[.+\\]: .+ '(.*)'"))
    , m_diagnosticsProvider(mw, this)
{
    KXMLGUIClient::setComponentName(QStringLiteral("katebuild"), i18n("Build"));
    setXMLFile(QStringLiteral("ui.rc"));

    m_toolView = mw->createToolView(plugin,
                                    QStringLiteral("kate_plugin_katebuildplugin"),
                                    KTextEditor::MainWindow::Bottom,
                                    QIcon::fromTheme(QStringLiteral("run-build-clean")),
                                    i18n("Build"));

    QAction *a = actionCollection()->addAction(QStringLiteral("select_target"));
    a->setText(i18n("Select Target..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("select")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotSelectTarget);

    a = actionCollection()->addAction(QStringLiteral("build_selected_target"));
    a->setText(i18n("Build Selected Target"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("run-build")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotBuildSelectedTarget);

    a = actionCollection()->addAction(QStringLiteral("build_and_run_selected_target"));
    a->setText(i18n("Build and Run Selected Target"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotBuildAndRunSelectedTarget);

    a = actionCollection()->addAction(QStringLiteral("compile_current_file"));
    a->setText(i18n("Compile Current File"));
    a->setToolTip(i18n("Try to compile the current file by searching a compile_commands.json"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("run-build")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotCompileCurrentFile);

    a = actionCollection()->addAction(QStringLiteral("stop"));
    a->setText(i18n("Stop"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotStop);

    a = actionCollection()->addAction(QStringLiteral("load_targets_cmakefileapi"));
    a->setText(i18n("Load targets from CMake Build Dir"));
    connect(a, &QAction::triggered, this, &KateBuildView::slotLoadCMakeTargets);

    a = actionCollection()->addAction(QStringLiteral("focus_build_tab_left"));
    a->setText(i18nc("Left is also left in RTL mode", "Focus Next Tab to the Left"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    connect(a, &QAction::triggered, this, [this]() {
        int index = m_buildUi.u_tabWidget->currentIndex();
        if (!m_toolView->isVisible()) {
            m_win->showToolView(m_toolView);
        } else {
            index += qApp->layoutDirection() == Qt::RightToLeft ? 1 : -1;
            if (index >= m_buildUi.u_tabWidget->count()) {
                index = 0;
            }
            if (index < 0) {
                index = m_buildUi.u_tabWidget->count() - 1;
            }
        }
        m_buildUi.u_tabWidget->setCurrentIndex(index);
        m_buildUi.u_tabWidget->widget(index)->setFocus();
    });

    a = actionCollection()->addAction(QStringLiteral("focus_build_tab_right"));
    a->setText(i18nc("Right is right also in RTL mode", "Focus Next Tab to the Right"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    connect(a, &QAction::triggered, this, [this]() {
        int index = m_buildUi.u_tabWidget->currentIndex();
        if (!m_toolView->isVisible()) {
            m_win->showToolView(m_toolView);
        } else {
            index += qApp->layoutDirection() == Qt::RightToLeft ? -1 : 1;
            if (index >= m_buildUi.u_tabWidget->count()) {
                index = 0;
            }
            if (index < 0) {
                index = m_buildUi.u_tabWidget->count() - 1;
            }
        }
        m_buildUi.u_tabWidget->setCurrentIndex(index);
        m_buildUi.u_tabWidget->widget(index)->setFocus();
    });

    m_buildWidget = new QWidget(m_toolView);
    m_buildUi.setupUi(m_buildWidget);
    m_targetsUi = new TargetsUi(this, m_buildUi.u_tabWidget);
    m_buildUi.u_tabWidget->insertTab(0, m_targetsUi, i18nc("Tab label", "Target Settings"));
    m_buildUi.u_tabWidget->setCurrentWidget(m_targetsUi);
    m_buildUi.u_tabWidget->setTabsClosable(true);
    m_buildUi.u_tabWidget->tabBar()->tabButton(0, QTabBar::RightSide)->hide();
    m_buildUi.u_tabWidget->tabBar()->tabButton(1, QTabBar::RightSide)->hide();
    connect(m_buildUi.u_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        // FIXME check if the process is still running
        m_buildUi.u_tabWidget->widget(index)->deleteLater();
    });

    connect(m_buildUi.u_tabWidget->tabBar(), &QTabBar::tabBarClicked, this, [this](int index) {
        if (QWidget *tabWidget = m_buildUi.u_tabWidget->widget(index)) {
            tabWidget->setFocus();
        }
    });

    m_buildWidget->installEventFilter(this);

    m_buildUi.buildAgainButton->setVisible(true);
    m_buildUi.cancelBuildButton->setVisible(true);
    m_buildUi.buildStatusLabel->setVisible(true);
    m_buildUi.cancelBuildButton->setEnabled(false);

    m_buildUi.textBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_buildUi.textBrowser->setWordWrapMode(QTextOption::NoWrap);
    m_buildUi.textBrowser->setReadOnly(true);
    m_buildUi.textBrowser->setOpenLinks(false);
    connect(m_buildUi.textBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        static QRegularExpression fileRegExp(QStringLiteral("(.*):(\\d+):(\\d+)"));
        const auto match = fileRegExp.match(url.toString(QUrl::None));
        if (!match.hasMatch() || !m_win) {
            return;
        };

        QString filePath = match.captured(1);
        if (!QFile::exists(filePath)) {
            filePath = caseFixed(filePath);
            if (!QFile::exists(filePath)) {
                QString paths = m_searchPaths.join(QStringLiteral("<br>"));
                displayMessage(i18n("<b>File not found:</b> %1<br>"
                                    "<b>Search paths:</b><br>%2<br>"
                                    "Try adding a search path to the \"Working Directory\"",
                                    match.captured(1),
                                    paths),
                               KTextEditor::Message::Warning);
                return;
            }
        }

        QUrl fileUrl = QUrl::fromLocalFile(filePath);
        m_win->openUrl(fileUrl);
        if (!m_win->activeView()) {
            return;
        }
        int lineNr = match.captured(2).toInt();
        int column = match.captured(3).toInt();
        m_win->activeView()->setCursorPosition(KTextEditor::Cursor(lineNr - 1, column - 1));
        m_win->activeView()->setFocus();
    });
    m_outputTimer.setSingleShot(true);
    m_outputTimer.setInterval(100);
    connect(&m_outputTimer, &QTimer::timeout, this, &KateBuildView::updateTextBrowser);

    auto updateEditorColors = [this](KTextEditor::Editor *e) {
        if (!e)
            return;
        auto theme = e->theme();
        auto bg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::BackgroundColor));
        auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::TextStyle::Normal));
        auto sel = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::TextSelection));
        auto errBg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::MarkError));
        auto warnBg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::MarkWarning));
        auto noteBg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::MarkBookmark));
        errBg.setAlpha(30);
        warnBg.setAlpha(30);
        noteBg.setAlpha(30);
        auto pal = m_buildUi.textBrowser->palette();
        pal.setColor(QPalette::Base, bg);
        pal.setColor(QPalette::Text, fg);
        pal.setColor(QPalette::Highlight, sel);
        pal.setColor(QPalette::HighlightedText, fg);
        m_buildUi.textBrowser->setPalette(pal);
        m_buildUi.textBrowser->document()->setDefaultStyleSheet(QStringLiteral("a{text-decoration:none;}"
                                                                               "a:link{color:%1;}\n"
                                                                               ".err-text {color:%1; background-color: %2;}"
                                                                               ".warn-text {color:%1; background-color: %3;}"
                                                                               ".note-text {color:%1; background-color: %4;}")
                                                                    .arg(fg.name(QColor::HexArgb))
                                                                    .arg(errBg.name(QColor::HexArgb))
                                                                    .arg(warnBg.name(QColor::HexArgb))
                                                                    .arg(noteBg.name(QColor::HexArgb)));
        updateTextBrowser();
    };
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateEditorColors);

    connect(m_buildUi.buildAgainButton, &QPushButton::clicked, this, &KateBuildView::slotBuildPreviousTarget);
    connect(m_buildUi.cancelBuildButton, &QPushButton::clicked, this, &KateBuildView::slotStop);

    connect(m_targetsUi->newTarget, &QToolButton::clicked, this, &KateBuildView::targetSetNew);
    connect(m_targetsUi->copyTarget, &QToolButton::clicked, this, &KateBuildView::targetOrSetClone);
    connect(m_targetsUi->deleteTarget, &QToolButton::clicked, this, &KateBuildView::targetDelete);

    connect(m_targetsUi->addButton, &QToolButton::clicked, this, &KateBuildView::slotAddTargetClicked);
    connect(m_targetsUi->buildButton, &QToolButton::clicked, this, &KateBuildView::slotBuildSelectedTarget);
    connect(m_targetsUi->runButton, &QToolButton::clicked, this, &KateBuildView::slotBuildAndRunSelectedTarget);
    connect(m_targetsUi, &TargetsUi::enterPressed, this, &KateBuildView::slotBuildAndRunSelectedTarget);

    m_proc.setOutputChannelMode(KProcess::SeparateChannels);
    connect(&m_proc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateBuildView::slotProcExited);
    connect(&m_proc, &KProcess::readyReadStandardError, this, &KateBuildView::slotReadReadyStdErr);
    connect(&m_proc, &KProcess::readyReadStandardOutput, this, &KateBuildView::slotReadReadyStdOut);

    connect(m_win, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &KateBuildView::handleEsc);
    connect(m_win, &KTextEditor::MainWindow::viewChanged, this, &KateBuildView::enableCompileCurrentFile);

    m_toolView->installEventFilter(this);

    m_win->guiFactory()->addClient(this);

    // watch for project plugin view creation/deletion
    connect(m_win, &KTextEditor::MainWindow::pluginViewCreated, this, &KateBuildView::slotPluginViewCreated);
    connect(m_win, &KTextEditor::MainWindow::pluginViewDeleted, this, &KateBuildView::slotPluginViewDeleted);

    // Connect signals from project plugin to our slots
    m_projectPluginView = m_win->pluginView(QStringLiteral("kateprojectplugin"));
    slotPluginViewCreated(QStringLiteral("kateprojectplugin"), m_projectPluginView);

    m_diagnosticsProvider.name = i18n("Build Information");
    m_diagnosticsProvider.setPersistentDiagnostics(true);

    // Whenever a project target is changed, queue a saving of the targets
    m_saveProjTargetsTimer.setSingleShot(true);
    connect(&m_saveProjTargetsTimer, &QTimer::timeout, this, &KateBuildView::saveProjectTargets);
    connect(&m_targetsUi->targetsModel, &TargetModel::projectTargetChanged, this, [this]() {
        qDebug() << sender();
        if (!m_addingProjTargets) {
            m_saveProjTargetsTimer.start(1);
        }
    });

    connect(m_targetsUi->moveTargetUp, &QToolButton::clicked, this, [this]() {
        const QPersistentModelIndex &currentIndex = m_targetsUi->proxyModel.mapToSource(m_targetsUi->targetsView->currentIndex());
        if (currentIndex.isValid()) {
            m_targetsUi->targetsModel.moveRowUp(currentIndex);
        }
        m_targetsUi->targetsView->scrollTo(m_targetsUi->targetsView->currentIndex());
    });
    connect(m_targetsUi->moveTargetDown, &QToolButton::clicked, this, [this]() {
        const QPersistentModelIndex &currentIndex = m_targetsUi->proxyModel.mapToSource(m_targetsUi->targetsView->currentIndex());
        if (currentIndex.isValid()) {
            m_targetsUi->targetsModel.moveRowDown(currentIndex);
        }
        m_targetsUi->targetsView->scrollTo(m_targetsUi->targetsView->currentIndex());
    });

    KateBuildPlugin *bPlugin = qobject_cast<KateBuildPlugin *>(plugin);
    if (bPlugin) {
        connect(bPlugin, &KateBuildPlugin::configChanged, this, &KateBuildView::readConfig);
    }
    readConfig();
}

/******************************************************************/
KateBuildView::~KateBuildView()
{
    if (m_proc.state() != QProcess::NotRunning) {
        terminateProcess(m_proc);
        m_proc.waitForFinished();
    }
    clearDiagnostics();
    m_win->guiFactory()->removeClient(this);
    delete m_toolView;
}

/******************************************************************/
void KateBuildView::readSessionConfig(const KConfigGroup &cg)
{
    int numTargets = cg.readEntry(QStringLiteral("NumTargets"), 0);
    m_projectTargetsetRow = cg.readEntry("ProjectTargetSetRow", 0);
    m_targetsUi->targetsModel.clear(m_projectTargetsetRow > 0);

    QModelIndex setIndex = m_targetsUi->targetsModel.sessionRootIndex();

    for (int i = 0; i < numTargets; i++) {
        QStringList targetNames = cg.readEntry(QStringLiteral("%1 Target Names").arg(i), QStringList());
        QString targetSetName = cg.readEntry(QStringLiteral("%1 Target").arg(i), QString());
        QString buildDir = cg.readEntry(QStringLiteral("%1 BuildPath").arg(i), QString());
        bool loadedViaCMake = cg.readEntry(QStringLiteral("%1 LoadedViaCMake").arg(i), false);
        QString cmakeConfigName = cg.readEntry(QStringLiteral("%1 CMakeConfig").arg(i), QStringLiteral("NONE"));

        if (loadedViaCMake) {
            QCMakeFileApi cmakeFA(buildDir, false);
            if (cmakeFA.haveKateReplyFiles()) {
                cmakeFA.readReplyFiles();

                setIndex = createCMakeTargetSet(setIndex, targetSetName, cmakeFA, cmakeConfigName);
            }

            continue;
        }

        setIndex = m_targetsUi->targetsModel.insertTargetSetAfter(setIndex, targetSetName, buildDir);

        // Keep a bit of backwards compatibility by ensuring that the "default" target is the first in the list
        QString defCmd = cg.readEntry(QStringLiteral("%1 Target Default").arg(i), QString());
        int defIndex = targetNames.indexOf(defCmd);
        if (defIndex > 0) {
            targetNames.move(defIndex, 0);
        }
        QModelIndex cmdIndex = setIndex;
        for (int tn = 0; tn < targetNames.size(); ++tn) {
            const QString &targetName = targetNames.at(tn);
            const QString &buildCmd = cg.readEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(targetName));
            const QString &runCmd = cg.readEntry(QStringLiteral("%1 RunCmd %2").arg(i).arg(targetName));
            m_targetsUi->targetsModel.addCommandAfter(cmdIndex, targetName, buildCmd, runCmd);
        }
    }

    // Add project targets, if any
    addProjectTargets();

    m_targetsUi->targetsView->expandAll();

    // pre-select the last active target or the first target of the first set
    int prevTargetSetRow = cg.readEntry(QStringLiteral("Active Target Index"), 0);
    int prevCmdRow = cg.readEntry(QStringLiteral("Active Target Command"), 0);
    QModelIndex rootIndex = m_targetsUi->targetsModel.index(prevTargetSetRow);
    QModelIndex cmdIndex = m_targetsUi->targetsModel.index(prevCmdRow, 0, rootIndex);
    cmdIndex = m_targetsUi->proxyModel.mapFromSource(cmdIndex);
    m_targetsUi->targetsView->setCurrentIndex(cmdIndex);

    m_targetsUi->updateTargetsButtonStates();

    m_targetsUi->targetsView->resizeColumnToContents(1);
    m_targetsUi->targetsView->resizeColumnToContents(2);
}

/******************************************************************/
void KateBuildView::writeSessionConfig(KConfigGroup &cg)
{
    // Save the active target
    QModelIndex activeIndex = m_targetsUi->targetsView->currentIndex();
    activeIndex = m_targetsUi->proxyModel.mapToSource(activeIndex);
    if (activeIndex.isValid()) {
        if (activeIndex.parent().isValid()) {
            cg.writeEntry(QStringLiteral("Active Target Index"), activeIndex.parent().row());
            cg.writeEntry(QStringLiteral("Active Target Command"), activeIndex.row());
        } else {
            cg.writeEntry(QStringLiteral("Active Target Index"), activeIndex.row());
            cg.writeEntry(QStringLiteral("Active Target Command"), 0);
        }
    }

    const QList<TargetModel::TargetSet> targets = m_targetsUi->targetsModel.sessionTargetSets();

    // Don't save project target-set, but save the row index
    QModelIndex projRootIndex = m_targetsUi->targetsModel.projectRootIndex();
    m_projectTargetsetRow = projRootIndex.row();
    cg.writeEntry("ProjectTargetSetRow", m_projectTargetsetRow);
    cg.writeEntry("NumTargets", targets.size());

    for (int i = 0; i < targets.size(); i++) {
        cg.writeEntry(QStringLiteral("%1 Target").arg(i), targets[i].name);
        cg.writeEntry(QStringLiteral("%1 BuildPath").arg(i), targets[i].workDir);
        cg.writeEntry(QStringLiteral("%1 LoadedViaCMake").arg(i), targets[i].loadedViaCMake);
        cg.writeEntry(QStringLiteral("%1 CMakeConfig").arg(i), targets[i].cmakeConfigName);

        if (targets[i].loadedViaCMake) {
            // don't save the build commands, they'll be reloaded via the CMake file API
            continue;
        }

        QStringList cmdNames;

        for (int j = 0; j < targets[i].commands.count(); j++) {
            const QString &cmdName = targets[i].commands[j].name;
            const QString &buildCmd = targets[i].commands[j].buildCmd;
            const QString &runCmd = targets[i].commands[j].runCmd;
            cmdNames << cmdName;
            cg.writeEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(cmdName), buildCmd);
            cg.writeEntry(QStringLiteral("%1 RunCmd %2").arg(i).arg(cmdName), runCmd);
        }
        cg.writeEntry(QStringLiteral("%1 Target Names").arg(i), cmdNames);
    }
}

/******************************************************************/
void KateBuildView::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("BuildConfig"));
    m_addDiagnostics = config.readEntry(QStringLiteral("UseDiagnosticsOutput"), true);
    m_autoSwitchToOutput = config.readEntry(QStringLiteral("AutoSwitchToOutput"), true);

    // read allow + block lists as two separate keys, let block always win
    const auto allowed = config.readEntry(ConfigAllowedCommands, QStringList());
    const auto blocked = config.readEntry(ConfigBlockedCommands, QStringList());
    m_commandLineToAllowedState.clear();
    for (const auto &cmd : allowed) {
        m_commandLineToAllowedState[cmd] = true;
    }
    for (const auto &cmd : blocked) {
        m_commandLineToAllowedState[cmd] = false;
    }
}

/******************************************************************/
void KateBuildView::writeConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("BuildConfig"));

    // write allow + block lists as two separate keys
    QStringList allowed, blocked;
    for (const auto &it : m_commandLineToAllowedState) {
        if (it.second) {
            allowed.push_back(it.first);
        } else {
            blocked.push_back(it.first);
        }
    }
    config.writeEntry(ConfigAllowedCommands, allowed);
    config.writeEntry(ConfigBlockedCommands, blocked);

    // Q_EMIT configUpdated();
}

/******************************************************************/
static Diagnostic createDiagnostic(int line, int column, const QString &message, const DiagnosticSeverity &severity)
{
    Diagnostic d;
    d.message = message;
    d.source = DiagnosticsPrefix;
    d.severity = severity;
    d.range = KTextEditor::Range(KTextEditor::Cursor(line - 1, column - 1), 0);
    return d;
}

/******************************************************************/
void KateBuildView::addError(const KateBuildView::OutputLine &err)
{
    // Get filediagnostic by filename or create new if there is none
    auto uri = QUrl::fromLocalFile(err.file);
    if (!uri.isValid()) {
        return;
    }
    DiagnosticSeverity severity = DiagnosticSeverity::Unknown;
    if (err.category == Category::Error) {
        m_numErrors++;
        severity = DiagnosticSeverity::Error;
    } else if (err.category == Category::Warning) {
        m_numWarnings++;
        severity = DiagnosticSeverity::Warning;
    } else if (err.category == Category::Info) {
        m_numNotes++;
        severity = DiagnosticSeverity::Information;
    }

    if (!m_addDiagnostics) {
        return;
    }

    // NOTE: Limit the number of items in the diagnostics view to 200 items.
    // Adding more items risks making the build slow. (standard item models are slow)
    if ((m_numErrors + m_numWarnings + m_numNotes) > 200) {
        return;
    }
    updateDiagnostics(createDiagnostic(err.lineNr, err.column, err.message, severity), uri);
}

/******************************************************************/
void KateBuildView::updateDiagnostics(Diagnostic diagnostic, const QUrl uri)
{
    FileDiagnostics fd;
    fd.uri = uri;
    fd.diagnostics.append(diagnostic);
    Q_EMIT m_diagnosticsProvider.diagnosticsAdded(fd);
}

/******************************************************************/
void KateBuildView::clearDiagnostics()
{
    Q_EMIT m_diagnosticsProvider.requestClearDiagnostics(&m_diagnosticsProvider);
}

/******************************************************************/
QUrl KateBuildView::docUrl()
{
    KTextEditor::View *kv = m_win->activeView();
    if (!kv) {
        qDebug() << "no KTextEditor::View";
        return QUrl();
    }

    if (kv->document()->isModified()) {
        kv->document()->save();
    }
    return kv->document()->url();
}

void KateBuildView::sendError(const QString &msg)
{
    Utils::showMessage(msg, QIcon::fromTheme(QStringLiteral("run-build")), i18n("Build"), MessageType::Error, m_win);
}

/******************************************************************/
bool KateBuildView::checkLocal(const QUrl &dir)
{
    if (dir.path().isEmpty()) {
        sendError(i18n("There is no file or directory specified for building."));
        return false;
    } else if (!dir.isLocalFile()) {
        sendError(
            i18n("The file \"%1\" is not a local file. "
                 "Non-local files cannot be compiled.",
                 dir.path()));
        return false;
    }
    return true;
}

/******************************************************************/
void KateBuildView::clearBuildResults()
{
    m_buildUi.textBrowser->clear();
    m_stdOut.clear();
    m_stdErr.clear();
    m_htmlOutput = QStringLiteral("<pre>");
    m_scrollStopPos = -1;
    m_numOutputLines = 0;
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;
    m_makeDirStack.clear();
    clearDiagnostics();
}

/******************************************************************/
QString KateBuildView::parseWorkDir(QString dir) const
{
    // When adding new placeholders, also update the tooltip in TargetHtmlDelegate::createEditor()
    if (m_projectPluginView) {
        const QFileInfo baseDir(m_projectPluginView->property("projectBaseDir").toString());
        dir.replace(QStringLiteral("%B"), baseDir.absoluteFilePath());
        dir.replace(QStringLiteral("%b"), baseDir.baseName());
    }
    return dir;
}

/******************************************************************/
bool KateBuildView::startProcess(const QString &dir, const QString &command)
{
    if (m_proc.state() != QProcess::NotRunning) {
        return false;
    }

    // clear previous runs
    clearBuildResults();

    if (m_autoSwitchToOutput) {
        // activate the output tab
        m_buildUi.u_tabWidget->setCurrentIndex(1);
        m_win->showToolView(m_toolView);
    }

    m_buildUi.u_tabWidget->setTabIcon(1, QIcon::fromTheme(QStringLiteral("system-run")));

    QFont font = Utils::editorFont();
    m_buildUi.textBrowser->setFont(font);

    // set working directory
    m_makeDir = dir;
    m_makeDirStack.push(m_makeDir);

    if (!QFile::exists(m_makeDir)) {
        sendError(i18n("Cannot run command: %1\nWork path does not exist: %2", command, m_makeDir));
        return false;
    }

    // chdir used by QProcess will resolve symbolic links.
    // Define PWD so that shell scripts can get a path with symbolic links intact
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PWD"), QDir(m_makeDir).absolutePath());
    m_proc.setProcessEnvironment(env);
    m_proc.setWorkingDirectory(m_makeDir);
    m_proc.setShellCommand(command);
    startHostProcess(m_proc);

    if (!m_proc.waitForStarted(500)) {
        sendError(i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc.exitStatus()));
        return false;
    }

    m_buildUi.cancelBuildButton->setEnabled(true);
    m_buildUi.buildAgainButton->setEnabled(false);
    m_targetsUi->setCursor(Qt::BusyCursor);
    return true;
}

/******************************************************************/
bool KateBuildView::slotStop()
{
    if (m_proc.state() != QProcess::NotRunning) {
        m_buildCancelled = true;
        QString msg = i18n("Building <b>%1</b> cancelled", m_currentlyBuildingTarget);
        m_buildUi.buildStatusLabel->setText(msg);
        terminateProcess(m_proc);
        return true;
    }
    return false;
}

/******************************************************************/
void KateBuildView::enableCompileCurrentFile()
{
    QAction *a = actionCollection()->action(QStringLiteral("compile_current_file"));
    if (!a) {
        return;
    }

    bool enable = false;
    if (m_win && m_win->activeView()) {
        KTextEditor::Document *currentDocument = m_win->activeView()->document();
        if (currentDocument) {
            const QString currentFile = currentDocument->url().path();
            QString compileCommandsFile = findCompileCommands(currentFile);
            enable = !compileCommandsFile.isEmpty();
        }
    }

    a->setEnabled(enable);
}

/******************************************************************/
QString KateBuildView::findCompileCommands(const QString &file) const
{
    QSet<QString> visitedDirs;

    QDir dir = QFileInfo(file).absoluteDir();

    while (true) {
        if (dir.exists(QStringLiteral("compile_commands.json"))) {
            return dir.filePath(QStringLiteral("compile_commands.json"));
        }
        if (dir.isRoot() || (dir == QDir::home())) { // don't "escape" the users home dir
            break;
        }
        visitedDirs.insert(dir.canonicalPath());
        dir.cdUp();
        if (visitedDirs.contains(dir.canonicalPath())) {
            break;
        }
    }

    return QString();
}

/******************************************************************/
KateBuildView::CompileCommands KateBuildView::parseCompileCommandsFile(const QString &compileCommandsFile) const
{
    qCDebug(KTEBUILD) << "parseCompileCommandsFile(): " << compileCommandsFile;
    CompileCommands res;
    res.filename = compileCommandsFile;
    res.date = QFileInfo(compileCommandsFile).lastModified();

    QFile file(compileCommandsFile);
    file.open(QIODevice::ReadOnly);
    QByteArray fileContents = file.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileContents);

    QJsonArray cmds = jsonDoc.array();
    qCDebug(KTEBUILD) << "parseCompileCommandsFile(): got " << cmds.count() << " entries";

    for (int i = 0; i < cmds.count(); i++) {
        QJsonObject cmdObj = cmds.at(i).toObject();
        const QString filename = cmdObj.value(QStringLiteral("file")).toString();
        const QString command = cmdObj.value(QStringLiteral("command")).toString();
        const QString dir = cmdObj.value(QStringLiteral("directory")).toString();
        if (dir.isEmpty() || command.isEmpty() || filename.isEmpty()) {
            qCDebug(KTEBUILD) << "parseCompileCommandsFile(): got empty entry at " << i << " !";
            continue; // should not happen
        }
        res.commands[filename] = {dir, command};
    }

    return res;
}

/******************************************************************/
void KateBuildView::slotCompileCurrentFile()
{
    qCDebug(KTEBUILD) << "slotCompileCurrentFile()";
    KTextEditor::Document *currentDocument = m_win->activeView()->document();
    if (!currentDocument) {
        qCDebug(KTEBUILD) << "slotCompileCurrentFile(): no file";
        return;
    }

    const QString currentFile = currentDocument->url().path();
    QString compileCommandsFile = findCompileCommands(currentFile);
    qCDebug(KTEBUILD) << "slotCompileCurrentFile(): file: " << currentFile << " compile_commands: " << compileCommandsFile;

    if (compileCommandsFile.isEmpty()) {
        QString msg = i18n("Did not find a compile_commands.json for file \"%1\". ", currentFile);
        Utils::showMessage(msg, QIcon::fromTheme(QStringLiteral("run-build")), i18n("Build"), MessageType::Warning, m_win);
        return;
    }

    if ((m_parsedCompileCommands.filename != compileCommandsFile) || (m_parsedCompileCommands.date < QFileInfo(compileCommandsFile).lastModified())) {
        qCDebug(KTEBUILD) << "slotCompileCurrentFile(): loading compile_commands.json";
        m_parsedCompileCommands = parseCompileCommandsFile(compileCommandsFile);
    }

    auto it = m_parsedCompileCommands.commands.find(currentFile);

    if (it == m_parsedCompileCommands.commands.end()) {
        QString msg = i18n("Did not find a compile command for file \"%1\" in \"%2\". ", currentFile, compileCommandsFile);
        Utils::showMessage(msg, QIcon::fromTheme(QStringLiteral("run-build")), i18n("Build"), MessageType::Warning, m_win);
        return;
    }

    qCDebug(KTEBUILD) << "slotCompileCurrentFile(): starting build: " << it->second.command << " in " << it->second.workingDir;
    startProcess(it->second.workingDir, it->second.command);
}

/******************************************************************/
void KateBuildView::slotBuildSelectedTarget()
{
    QModelIndex currentIndex = m_targetsUi->targetsView->currentIndex();
    if (!currentIndex.isValid() || (m_firstBuild && !m_targetsUi->targetsView->isVisible())) {
        slotSelectTarget();
        return;
    }
    m_firstBuild = false;

    if (!currentIndex.parent().isValid()) {
        // This is a root item, try to build the first command
        currentIndex = m_targetsUi->targetsView->model()->index(0, 0, currentIndex.siblingAtColumn(0));
        if (currentIndex.isValid()) {
            m_targetsUi->targetsView->setCurrentIndex(currentIndex);
        } else {
            slotSelectTarget();
            return;
        }
    }
    buildCurrentTarget();
}

/******************************************************************/
void KateBuildView::slotBuildAndRunSelectedTarget()
{
    QModelIndex currentIndex = m_targetsUi->targetsView->currentIndex();
    if (!currentIndex.isValid() || (m_firstBuild && !m_targetsUi->targetsView->isVisible())) {
        slotSelectTarget();
        return;
    }
    m_firstBuild = false;

    if (!currentIndex.parent().isValid()) {
        // This is a root item, try to build the first command
        currentIndex = m_targetsUi->targetsView->model()->index(0, 0, currentIndex.siblingAtColumn(0));
        if (currentIndex.isValid()) {
            m_targetsUi->targetsView->setCurrentIndex(currentIndex);
        } else {
            slotSelectTarget();
            return;
        }
    }

    m_runAfterBuild = true;
    buildCurrentTarget();
}

/******************************************************************/
void KateBuildView::slotBuildPreviousTarget()
{
    if (!m_previousIndex.isValid()) {
        slotSelectTarget();
    } else {
        m_targetsUi->targetsView->setCurrentIndex(m_previousIndex);
        buildCurrentTarget();
    }
}

/******************************************************************/
void KateBuildView::slotSelectTarget()
{
    m_buildUi.u_tabWidget->setCurrentIndex(0);
    m_win->showToolView(m_toolView);
    QPersistentModelIndex selected = m_targetsUi->targetsView->currentIndex();
    m_targetsUi->targetFilterEdit->setText(QString());
    m_targetsUi->targetFilterEdit->setFocus();

    // Flash the target selection line-edit to show that something happened
    // and where your focus went/should go
    QPalette palette = m_targetsUi->targetFilterEdit->palette();
    KColorScheme::adjustBackground(palette, KColorScheme::ActiveBackground);
    m_targetsUi->targetFilterEdit->setPalette(palette);
    QTimer::singleShot(500, this, [this]() {
        m_targetsUi->targetFilterEdit->setPalette(QPalette());
    });

    m_targetsUi->targetsView->expandAll();
    if (!selected.isValid()) {
        // We do not have a selected item. Select the first target of the first target-set
        QModelIndex root = m_targetsUi->targetsView->model()->index(0, 0, QModelIndex());
        if (root.isValid()) {
            selected = root.model()->index(0, 0, root);
        }
    }
    if (selected.isValid()) {
        m_targetsUi->targetsView->setCurrentIndex(selected);
        m_targetsUi->targetsView->scrollTo(selected);
    }
}

/******************************************************************/
bool KateBuildView::buildCurrentTarget()
{
    const QFileInfo docFInfo(docUrl().toLocalFile()); // docUrl() saves the current document

    QModelIndex ind = m_targetsUi->targetsView->currentIndex();
    m_previousIndex = ind;
    if (!ind.isValid()) {
        sendError(i18n("No target available for building."));
        return false;
    }

    QString buildCmd = ind.data(TargetModel::CommandRole).toString();
    QString cmdName = ind.data(TargetModel::CommandNameRole).toString();
    m_searchPaths = ind.data(TargetModel::SearchPathsRole).toStringList();
    QString workDir = ind.data(TargetModel::WorkDirRole).toString();
    QString targetSet = ind.data(TargetModel::TargetSetNameRole).toString();

    QString dir = parseWorkDir(workDir);
    if (workDir.isEmpty()) {
        dir = docFInfo.absolutePath();
        if (dir.isEmpty()) {
            sendError(i18n("There is no local file or directory specified for building."));
            return false;
        }
    }

    if (m_proc.state() != QProcess::NotRunning) {
        displayBuildResult(i18n("Already building..."), KTextEditor::Message::Warning);
        return false;
    }

    if (m_runAfterBuild && buildCmd.isEmpty()) {
        slotRunAfterBuild();
        return true;
    }

    // Check if the command contains the file name or directory
    // When adding new placeholders, also update the tooltip in TargetHtmlDelegate::createEditor()
    if (buildCmd.contains(QLatin1String("%f")) || buildCmd.contains(QLatin1String("%d")) || buildCmd.contains(QLatin1String("%n"))) {
        if (docFInfo.absoluteFilePath().isEmpty()) {
            return false;
        }

        buildCmd.replace(QStringLiteral("%n"), docFInfo.baseName());
        buildCmd.replace(QStringLiteral("%f"), docFInfo.absoluteFilePath());
        buildCmd.replace(QStringLiteral("%d"), docFInfo.absolutePath());
    }
    m_currentlyBuildingTarget = QStringLiteral("%1: %2").arg(targetSet, cmdName);
    m_buildCancelled = false;
    QString msg = i18n("Building target <b>%1</b> ...", m_currentlyBuildingTarget);
    m_buildUi.buildStatusLabel->setText(msg);
    return startProcess(dir, buildCmd);
}

/******************************************************************/
void KateBuildView::slotLoadCMakeTargets()
{
    const QString cmakeFile = QFileDialog::getOpenFileName(nullptr,
                                                           QStringLiteral("Select CMake Build Dir by Selecting the CMakeCache.txt"),
                                                           QDir::currentPath(),
                                                           QStringLiteral("CMake Cache file (CMakeCache.txt)"));
    qCDebug(KTEBUILD) << "Loading cmake targets for file " << cmakeFile;
    if (cmakeFile.isEmpty()) {
        return;
    }

    loadCMakeTargets(cmakeFile);
}

/******************************************************************/
void KateBuildView::loadCMakeTargets(const QString &cmakeFile)
{
    QCMakeFileApi cmakeFA(cmakeFile, false);
    if (cmakeFA.getCMakeExecutable().isEmpty()) {
        sendError(i18n("Cannot load targets, the file %1 does not contain a proper CMAKE_COMMAND entry !", cmakeFile));
        return;
    }

    const QString compileCommandsFile = cmakeFA.getBuildDir() + QStringLiteral("/compile_commands.json");
    if (!cmakeFA.haveKateReplyFiles() || !QFile::exists(compileCommandsFile)) {
        QStringList commandLine = cmakeFA.getCMakeRequestCommandLine();
        if (!isCommandLineAllowed(commandLine)) {
            return;
        }

        bool success = cmakeFA.writeQueryFiles();
        if (!success) {
            sendError(i18n("Could not write CMake File API query files for build directory %1 !", cmakeFA.getBuildDir()));

            return;
        }
        success = cmakeFA.runCMake();
        if (!success) {
            sendError(i18n("Could not run CMake (%2) for build directory %1 !", cmakeFA.getBuildDir(), cmakeFA.getCMakeExecutable()));

            return;
        }
    }

    if (!cmakeFA.haveKateReplyFiles()) {
        qCDebug(KTEBUILD) << "Generating CMake reply files failed !";
        sendError(
            i18n("Generating CMake File API reply files for build directory %1 failed (using %2) !", cmakeFA.getBuildDir(), cmakeFA.getCMakeExecutable()));
        return;
    }

    bool success = cmakeFA.readReplyFiles();
    qCDebug(KTEBUILD) << "CMake reply success: " << success;

    for (const QString &config : cmakeFA.getConfigurations()) {
        QString projectName = QStringLiteral("%1@%2 - [%3]").arg(cmakeFA.getProjectName()).arg(cmakeFA.getBuildDir()).arg(config);

        QModelIndex setIndex = m_targetsUi->targetsModel.sessionRootIndex();

        createCMakeTargetSet(setIndex, projectName, cmakeFA, config);
    }

    const QString compileCommandsDest = cmakeFA.getSourceDir() + QStringLiteral("/compile_commands.json");
#ifdef Q_OS_WIN
    QFile::remove(compileCommandsDest);
    QFile::copy(compileCommandsFile, compileCommandsDest);
#else
    QFile::link(compileCommandsFile, compileCommandsDest);
#endif

    QUrl fileUrl = QUrl::fromLocalFile(cmakeFA.getSourceDir() + QStringLiteral("/CMakeLists.txt"));
    m_win->openUrl(fileUrl);
    // see whether the project plugin can load a project for the source directory
    if (QObject *plugin = KTextEditor::Editor::instance()->application()->plugin(QStringLiteral("kateprojectplugin"))) {
        QMetaObject::invokeMethod(plugin, "projectForDir", Q_ARG(QDir, cmakeFA.getSourceDir()), Q_ARG(bool, true));
    }
}

/******************************************************************/
QModelIndex KateBuildView::createCMakeTargetSet(QModelIndex setIndex, const QString &name, const QCMakeFileApi &cmakeFA, const QString &cmakeConfig)
{
    const int numCores = QThread::idealThreadCount();

    const QList<TargetModel::TargetSet> existingTargetSets = m_targetsUi->targetsModel.sessionTargetSets();
    for (int i = 0; i < existingTargetSets.size(); i++) {
        if (existingTargetSets[i].loadedViaCMake && (existingTargetSets[i].cmakeConfigName == cmakeConfig)
            && (existingTargetSets[i].workDir == cmakeFA.getBuildDir())) {
            // this target set has already been loaded, don't add it once more
            return setIndex;
        }
    }

    setIndex = m_targetsUi->targetsModel.insertTargetSetAfter(setIndex, name, cmakeFA.getBuildDir(), true, cmakeConfig);
    const QModelIndex targetSetIndex = setIndex;

    setIndex = m_targetsUi->targetsModel.addCommandAfter(setIndex,
                                                         QStringLiteral("All"),
                                                         QStringLiteral("%1 --build \"%2\" --config \"%3\" --parallel %4")
                                                             .arg(cmakeFA.getCMakeExecutable())
                                                             .arg(cmakeFA.getBuildDir())
                                                             .arg(cmakeConfig)
                                                             .arg(numCores),
                                                         QString());
    const QModelIndex allIndex = setIndex;

    setIndex = m_targetsUi->targetsModel.addCommandAfter(setIndex,
                                                         QStringLiteral("Clean"),
                                                         QStringLiteral("%1 --build \"%2\" --config \"%3\" --parallel %4 --target clean")
                                                             .arg(cmakeFA.getCMakeExecutable())
                                                             .arg(cmakeFA.getBuildDir())
                                                             .arg(cmakeConfig)
                                                             .arg(numCores),
                                                         QString());

    setIndex = m_targetsUi->targetsModel.addCommandAfter(setIndex,
                                                         QStringLiteral("Rerun CMake"),
                                                         QStringLiteral("%1 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B \"%2\" -S \"%3\"")
                                                             .arg(cmakeFA.getCMakeExecutable())
                                                             .arg(cmakeFA.getBuildDir())
                                                             .arg(cmakeFA.getSourceDir()),
                                                         QString());

    QString cmakeGui = cmakeFA.getCMakeGuiExecutable();
    qCDebug(KTEBUILD) << "Creating cmake targets, cmakeGui: " << cmakeGui;
    if (!cmakeGui.isEmpty()) {
        setIndex = m_targetsUi->targetsModel.addCommandAfter(
            setIndex,
            QStringLiteral("Run CMake-Gui"),
            QStringLiteral("%1 -B \"%2\" -S \"%3\"").arg(cmakeGui).arg(cmakeFA.getBuildDir()).arg(cmakeFA.getSourceDir()),
            QString());
    }

    std::vector<QCMakeFileApi::Target> targets = cmakeFA.getTargets(cmakeConfig);
    std::sort(targets.begin(), targets.end(), [](const QCMakeFileApi::Target &a, const QCMakeFileApi::Target &b) {
        if (a.type == b.type) {
            return (a.name < b.name);
        }
        return (int(a.type) < int(b.type));
    });
    for (const QCMakeFileApi::Target &tgt : targets) {
        setIndex = m_targetsUi->targetsModel.addCommandAfter(setIndex,
                                                             tgt.name,
                                                             QStringLiteral("%1 --build \"%2\" --config \"%3\" --parallel %4 --target %5")
                                                                 .arg(cmakeFA.getCMakeExecutable())
                                                                 .arg(cmakeFA.getBuildDir())
                                                                 .arg(cmakeConfig)
                                                                 .arg(numCores)
                                                                 .arg(tgt.name),
                                                             QString());
    }

    // open the new target set subtree and select the "Build all" target
    m_targetsUi->targetsView->expand(m_targetsUi->proxyModel.mapFromSource(targetSetIndex));
    m_targetsUi->targetsView->setCurrentIndex(m_targetsUi->proxyModel.mapFromSource(allIndex));

    return setIndex;
}

bool KateBuildView::isCommandLineAllowed(const QStringList &cmdline)
{
    // check our allow list
    // if we already have stored some value, perfect, just use that one
    const QString fullCommandLineString = cmdline.join(QStringLiteral(" "));
    if (const auto it = m_commandLineToAllowedState.find(fullCommandLineString); it != m_commandLineToAllowedState.end()) {
        return it->second;
    }

    // ask user if the start should be allowed
    QPointer<QMessageBox> msgBox(new QMessageBox(QApplication::activeWindow()));
    msgBox->setWindowTitle(i18n("Build plugin wants to execute program"));
    msgBox->setTextFormat(Qt::RichText);
    msgBox->setText(
        i18n("The Kate build plugin needs to execute an external command to read the targets from the build tree.<br><br>"
             "The full command line is:<br><br><b>%1</b><br><br>"
             "Proceed and allow to run this command ?<br><br>"
             "The choice can be altered via the config page of the plugin.",
             fullCommandLineString.toHtmlEscaped()));
    msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::Yes);
    const bool allowed = (msgBox->exec() == QMessageBox::Yes);

    // store new configured value
    m_commandLineToAllowedState.emplace(fullCommandLineString, allowed);

    // inform the user if it was forbidden! do this here to just emit this once
    // if (!allowed) {
    // Q_EMIT showMessage(KTextEditor::Message::Information,
    // i18n("User permanently blocked start of: '%1'.\nUse the config page of the plugin to undo this block.", fullCommandLineString));
    // }

    // flush config to not loose that setting
    // writeConfig();
    return allowed;
}

/******************************************************************/
void KateBuildView::displayBuildResult(const QString &msg, KTextEditor::Message::MessageType level)
{
    KTextEditor::View *kv = m_win->activeView();
    if (!kv) {
        return;
    }

    delete m_infoMessage;
    m_infoMessage = new KTextEditor::Message(xi18nc("@info", "<title>Make Results:</title><nl/>%1", msg), level);
    m_infoMessage->setWordWrap(false);
    m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
    m_infoMessage->setAutoHide(5000);
    m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
    m_infoMessage->setView(kv);
    kv->document()->postMessage(m_infoMessage);
}

/******************************************************************/
void KateBuildView::displayMessage(const QString &msg, KTextEditor::Message::MessageType level)
{
    KTextEditor::View *kv = m_win->activeView();
    if (!kv) {
        return;
    }

    delete m_infoMessage;
    m_infoMessage = new KTextEditor::Message(msg, level);
    m_infoMessage->setWordWrap(false);
    m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
    m_infoMessage->setAutoHide(8000);
    m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
    m_infoMessage->setView(kv);
    kv->document()->postMessage(m_infoMessage);
}

/******************************************************************/
void KateBuildView::slotProcExited(int exitCode, QProcess::ExitStatus)
{
    m_targetsUi->unsetCursor();
    m_buildUi.u_tabWidget->setTabIcon(1, QIcon::fromTheme(QStringLiteral("format-justify-left")));
    m_buildUi.cancelBuildButton->setEnabled(false);
    m_buildUi.buildAgainButton->setEnabled(true);

    QString buildStatus =
        i18n("Build <b>%1</b> completed. %2 error(s), %3 warning(s), %4 note(s)", m_currentlyBuildingTarget, m_numErrors, m_numWarnings, m_numNotes);

    bool buildSuccess = true;
    if (m_numErrors || m_numWarnings) {
        QStringList msgs;
        if (m_numErrors) {
            msgs << i18np("Found one error.", "Found %1 errors.", m_numErrors);
            buildSuccess = false;
        }
        if (m_numWarnings) {
            msgs << i18np("Found one warning.", "Found %1 warnings.", m_numWarnings);
        }
        if (m_numNotes) {
            msgs << i18np("Found one note.", "Found %1 notes.", m_numNotes);
        }
        displayBuildResult(msgs.join(QLatin1Char('\n')), m_numErrors ? KTextEditor::Message::Error : KTextEditor::Message::Warning);
    } else if (exitCode != 0) {
        buildSuccess = false;
        displayBuildResult(i18n("Build failed."), KTextEditor::Message::Warning);
    } else {
        displayBuildResult(i18n("Build completed without problems."), KTextEditor::Message::Positive);
    }

    if (m_buildCancelled) {
        buildStatus =
            i18n("Build <b>%1 canceled</b>. %2 error(s), %3 warning(s), %4 note(s)", m_currentlyBuildingTarget, m_numErrors, m_numWarnings, m_numNotes);
    }
    m_buildUi.buildStatusLabel->setText(buildStatus);

    if (buildSuccess && m_runAfterBuild) {
        m_runAfterBuild = false;
        slotRunAfterBuild();
    }
}

void KateBuildView::slotRunAfterBuild()
{
    if (!m_previousIndex.isValid()) {
        return;
    }
    QModelIndex idx = m_previousIndex;
    QModelIndex runIdx = idx.siblingAtColumn(2);
    const QString runCmd = runIdx.data().toString();
    if (runCmd.isEmpty()) {
        // Nothing to run, and not a problem
        return;
    }
    const QString workDir = parseWorkDir(idx.data(TargetModel::WorkDirRole).toString());
    if (workDir.isEmpty()) {
        displayBuildResult(i18n("Cannot execute: %1 No working directory set.", runCmd), KTextEditor::Message::Warning);
        return;
    }
    QModelIndex nameIdx = idx.siblingAtColumn(0);
    QString name = nameIdx.data().toString();

    AppOutput *out = nullptr;
    for (int i = 2; i < m_buildUi.u_tabWidget->count(); ++i) {
        QString tabToolTip = m_buildUi.u_tabWidget->tabToolTip(i);
        if (runCmd == tabToolTip) {
            out = qobject_cast<AppOutput *>(m_buildUi.u_tabWidget->widget(i));
            if (!out || !out->runningProcess().isEmpty()) {
                out = nullptr;
                continue;
            }
            // We have a winner, re-use this tab
            m_buildUi.u_tabWidget->setCurrentIndex(i);
            break;
        }
    }
    if (!out) {
        // This is means we create a new tab
        out = new AppOutput();
        int tabIndex = m_buildUi.u_tabWidget->addTab(out, name);
        m_buildUi.u_tabWidget->setCurrentIndex(tabIndex);
        m_buildUi.u_tabWidget->setTabToolTip(tabIndex, runCmd);
        m_buildUi.u_tabWidget->setTabIcon(tabIndex, QIcon::fromTheme(QStringLiteral("media-playback-start")));

        connect(out, &AppOutput::runningChanged, this, [this]() {
            // Update the tab icon when the run state changes
            for (int i = 2; i < m_buildUi.u_tabWidget->count(); ++i) {
                AppOutput *tabOut = qobject_cast<AppOutput *>(m_buildUi.u_tabWidget->widget(i));
                if (!tabOut) {
                    continue;
                }
                if (!tabOut->runningProcess().isEmpty()) {
                    m_buildUi.u_tabWidget->setTabIcon(i, QIcon::fromTheme(QStringLiteral("media-playback-start")));
                } else {
                    m_buildUi.u_tabWidget->setTabIcon(i, QIcon::fromTheme(QStringLiteral("media-playback-pause")));
                }
            }
        });
    }

    out->setWorkingDir(workDir);
    out->runCommand(runCmd);

    if (m_win->activeView()) {
        m_win->activeView()->setFocus();
    }
}

QString KateBuildView::toOutputHtml(const KateBuildView::OutputLine &out)
{
    QString htmlStr;
    if (!out.file.isEmpty()) {
        htmlStr += QStringLiteral("<a href=\"%1:%2:%3\">").arg(out.file).arg(out.lineNr).arg(out.column);
    }
    switch (out.category) {
    case Category::Error:
        htmlStr += QStringLiteral("<span class=\"err-text\">");
        break;
    case Category::Warning:
        htmlStr += QStringLiteral("<span class=\"warn-text\">");
        break;
    case Category::Info:
        htmlStr += QStringLiteral("<span class=\"note-text\">");
        break;
    case Category::Normal:
        htmlStr += QStringLiteral("<span>");
        break;
    }
    htmlStr += out.lineStr.toHtmlEscaped();
    htmlStr += QStringLiteral("</span>\n");
    if (!out.file.isEmpty()) {
        htmlStr += QStringLiteral("</a>");
    }
    return htmlStr;
}

void KateBuildView::updateTextBrowser()
{
    QTextBrowser *edit = m_buildUi.textBrowser;
    // Get the scroll position to restore it if not at the end
    int scrollValue = edit->verticalScrollBar()->value();
    int scrollMax = edit->verticalScrollBar()->maximum();
    // Save the selection and restore it after adding the text
    QTextCursor cursor = edit->textCursor();

    // set the new document
    edit->setHtml(m_htmlOutput);

    // Restore selection and scroll position
    edit->setTextCursor(cursor);
    if (scrollValue != scrollMax) {
        // We had stopped scrolling already
        edit->verticalScrollBar()->setValue(scrollValue);
        return;
    }

    // We were at the bottom before adding lines.
    int newPos = edit->verticalScrollBar()->maximum();
    if (m_scrollStopPos != -1) {
        int targetPos = ((newPos + edit->verticalScrollBar()->pageStep()) * m_scrollStopPos) / m_numOutputLines;
        if (targetPos < newPos) {
            newPos = targetPos;
            // if we want to continue scrolling, just scroll to the end and it will continue
            m_scrollStopPos = -1;
        }
    }
    edit->verticalScrollBar()->setValue(newPos);
}

/******************************************************************/
void KateBuildView::slotReadReadyStdOut()
{
    // read data from procs stdout and add
    // the text to the end of the output

    // FIXME unify the parsing of stdout and stderr
    QString l = QString::fromUtf8(m_proc.readAllStandardOutput());
    l.remove(QLatin1Char('\r'));
    m_stdOut += l;

    // handle one line at a time
    int end = -1;
    while ((end = m_stdOut.indexOf(QLatin1Char('\n'))) >= 0) {
        const QString line = m_stdOut.mid(0, end);

        // Check if this is a new directory for Make
        QRegularExpressionMatch match = m_newDirDetector.match(line);
        if (match.hasMatch()) {
            QString newDir = match.captured(1);
            if ((m_makeDirStack.size() > 1) && (m_makeDirStack.top() == newDir)) {
                m_makeDirStack.pop();
                newDir = m_makeDirStack.top();
            } else {
                m_makeDirStack.push(newDir);
            }
            m_makeDir = newDir;
        }

        // Add the new output to the output and possible error/warnings to the diagnostics output
        KateBuildView::OutputLine out = processOutputLine(line);
        m_htmlOutput += toOutputHtml(out);
        m_numOutputLines++;
        if (out.category != Category::Normal) {
            addError(out);
            if (m_scrollStopPos == -1) {
                // stop the scroll a couple of lines before the top of the view
                m_scrollStopPos = std::max(m_numOutputLines - 4, 0);
            }
        }
        m_stdOut.remove(0, end + 1);
    }
    if (!m_outputTimer.isActive()) {
        m_outputTimer.start();
    }
}

/******************************************************************/
void KateBuildView::slotReadReadyStdErr()
{
    // FIXME This works for utf8 but not for all charsets
    QString l = QString::fromUtf8(m_proc.readAllStandardError());
    l.remove(QLatin1Char('\r'));
    m_stdErr += l;

    int end = -1;
    while ((end = m_stdErr.indexOf(QLatin1Char('\n'))) >= 0) {
        const QString line = m_stdErr.mid(0, end);
        KateBuildView::OutputLine out = processOutputLine(line);
        m_htmlOutput += toOutputHtml(out);
        m_numOutputLines++;
        if (out.category != Category::Normal) {
            addError(out);
            if (m_scrollStopPos == -1) {
                // stop the scroll a couple of lines before the top of the view
                // a small improvement could be achieved by storing the number of lines added...
                m_scrollStopPos = std::max(m_numOutputLines - 4, 0);
            }
        }
        m_stdErr.remove(0, end + 1);
    }
    if (!m_outputTimer.isActive()) {
        m_outputTimer.start();
    }
}

/******************************************************************/
KateBuildView::OutputLine KateBuildView::processOutputLine(QString line)
{
    // look for a filename
    QRegularExpressionMatch match = m_filenameDetector.match(line);

    if (!match.hasMatch()) {
        return {.category = Category::Normal, .lineStr = line, .message = line, .file = QString(), .lineNr = 0, .column = 0};
    }

    QString filename = match.captured(QStringLiteral("filename"));
    const QString line_n = match.captured(QStringLiteral("line"));
    const QString col_n = match.captured(QStringLiteral("column"));
    const QString msg = match.captured(QStringLiteral("message"));

#ifdef Q_OS_WIN
    // convert '\' to '/' so the concatenation works
    filename = QFileInfo(filename).filePath();
#endif

    // qDebug() << "File Name:"<<m_makeDir << filename << " msg:"<< msg;
    // add path to file
    if (QFile::exists(m_makeDir + QLatin1Char('/') + filename)) {
        filename = m_makeDir + QLatin1Char('/') + filename;
    }

    // If we still do not have a file name try the extra search paths
    int i = 1;
    while (!QFile::exists(filename) && i < m_searchPaths.size()) {
        if (QFile::exists(m_searchPaths[i] + QLatin1Char('/') + filename)) {
            filename = m_searchPaths[i] + QLatin1Char('/') + filename;
        }
        i++;
    }

    Category category = Category::Normal;
    static QRegularExpression errorRegExp(QStringLiteral("error:"), QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression errorRegExpTr(QStringLiteral("%1:").arg(i18nc("The same word as 'gcc' uses for an error.", "error")),
                                            QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression warningRegExp(QStringLiteral("warning:"), QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression warningRegExpTr(QStringLiteral("%1:").arg(i18nc("The same word as 'gcc' uses for a warning.", "warning")),
                                              QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression infoRegExp(QStringLiteral("(info|note):"), QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression infoRegExpTr(QStringLiteral("(%1):").arg(i18nc("The same words as 'gcc' uses for note or info.", "note|info")),
                                           QRegularExpression::CaseInsensitiveOption);
    if (msg.contains(errorRegExp) || msg.contains(errorRegExpTr) || msg.contains(QLatin1String("undefined reference"))
        || msg.contains(i18nc("The same word as 'ld' uses to mark an ...", "undefined reference"))) {
        category = Category::Error;
    } else if (msg.contains(warningRegExp) || msg.contains(warningRegExpTr)) {
        category = Category::Warning;
    } else if (msg.contains(infoRegExp) || msg.contains(infoRegExpTr)) {
        category = Category::Info;
    }
    // Now we have the data we need show the error/warning
    return {.category = category, .lineStr = line, .message = msg, .file = filename, .lineNr = line_n.toInt(), .column = col_n.toInt()};
}

/******************************************************************/
void KateBuildView::slotAddTargetClicked()
{
    QModelIndex current = m_targetsUi->targetsView->currentIndex();
    QString currName = DefTargetName;
    QString currCmd;
    QString currRun;

    current = m_targetsUi->proxyModel.mapToSource(current);
    QModelIndex index = m_targetsUi->targetsModel.addCommandAfter(current, currName, currCmd, currRun);
    index = m_targetsUi->proxyModel.mapFromSource(index);
    m_targetsUi->targetsView->setCurrentIndex(index);
}

/******************************************************************/
void KateBuildView::targetSetNew()
{
    m_targetsUi->targetFilterEdit->setText(QString());
    QModelIndex currentIndex = m_targetsUi->proxyModel.mapToSource(m_targetsUi->targetsView->currentIndex());
    QModelIndex index = m_targetsUi->targetsModel.insertTargetSetAfter(currentIndex, i18n("Target Set"), QString());
    QModelIndex buildIndex = m_targetsUi->targetsModel.addCommandAfter(index, i18n("Build"), DefBuildCmd, QString());
    m_targetsUi->targetsModel.addCommandAfter(index, i18n("Clean"), DefCleanCmd, QString());
    m_targetsUi->targetsModel.addCommandAfter(index, i18n("Config"), DefConfigCmd, QString());
    m_targetsUi->targetsModel.addCommandAfter(index, i18n("ConfigClean"), DefConfClean, QString());
    buildIndex = m_targetsUi->proxyModel.mapFromSource(buildIndex);
    m_targetsUi->targetsView->setCurrentIndex(buildIndex);
}

/******************************************************************/
void KateBuildView::targetOrSetClone()
{
    QModelIndex currentIndex = m_targetsUi->targetsView->currentIndex();
    currentIndex = m_targetsUi->proxyModel.mapToSource(currentIndex);
    m_targetsUi->targetFilterEdit->setText(QString());
    QModelIndex newIndex = m_targetsUi->targetsModel.cloneTargetOrSet(currentIndex);
    if (m_targetsUi->targetsModel.hasChildren(newIndex)) {
        newIndex = m_targetsUi->proxyModel.mapFromSource(newIndex);
        m_targetsUi->targetsView->setCurrentIndex(newIndex.model()->index(0, 0, newIndex));
        return;
    }
    newIndex = m_targetsUi->proxyModel.mapFromSource(newIndex);
    m_targetsUi->targetsView->setCurrentIndex(newIndex);
}

/******************************************************************/
void KateBuildView::targetDelete()
{
    QModelIndex currentIndex = m_targetsUi->targetsView->currentIndex();
    currentIndex = m_targetsUi->proxyModel.mapToSource(currentIndex);
    m_targetsUi->targetsModel.deleteItem(currentIndex);

    if (m_targetsUi->targetsModel.rowCount() == 0) {
        targetSetNew();
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewCreated(const QString &name, QObject *pluginView)
{
    // add view
    if (pluginView && name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = pluginView;
        addProjectTargets();
        connect(pluginView, SIGNAL(projectMapChanged()), this, SLOT(slotProjectMapChanged()), Qt::UniqueConnection);
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewDeleted(const QString &name, QObject *)
{
    // remove view
    if (name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = nullptr;
        m_targetsUi->targetsModel.deleteProjectTargerts();
    }
}

/******************************************************************/
void KateBuildView::slotProjectMapChanged()
{
    // only do stuff with valid project
    if (!m_projectPluginView) {
        return;
    }
    m_targetsUi->targetsModel.deleteProjectTargerts();
    addProjectTargets();
}

/******************************************************************/
void KateBuildView::addProjectTargets()
{
    // only do stuff with a valid project
    if (!m_projectPluginView) {
        return;
    }

    BoolTrueLocker locker(m_addingProjTargets);

    // Delete any old project plugin targets
    m_targetsUi->targetsModel.deleteProjectTargerts();

    const QModelIndex projRootIndex = m_targetsUi->targetsModel.projectRootIndex();
    const QString projectsBaseDir = m_projectPluginView->property("projectBaseDir").toString();

    // Read the targets from the override if available
    const QString userOverride = projectsBaseDir + QStringLiteral("/.kateproject.build");
    if (QFile::exists(userOverride)) {
        // We have user modified commands
        QFile file(userOverride);
        if (file.open(QIODevice::ReadOnly)) {
            QString jsonString = QString::fromUtf8(file.readAll());
            QModelIndex addedIndex = m_targetsUi->targetsModel.insertAfter(projRootIndex, jsonString);
            if (addedIndex != projRootIndex) {
                return;
                // The user file overrides the rest
                // TODO maybe add heuristics for when to re-read the project data
            }
        }
    }

    // query new project map
    QVariantMap projectMap = m_projectPluginView->property("projectMap").toMap();

    // do we have a valid map for build settings?
    QVariantMap buildMap = projectMap.value(QStringLiteral("build")).toMap();
    if (buildMap.isEmpty()) {
        return;
    }

    // handle build directory as relative to project file, if possible, see bug 413306
    QString projectsBuildDir = buildMap.value(QStringLiteral("directory")).toString();
    QString projectName = m_projectPluginView->property("projectName").toString();
    if (!projectsBaseDir.isEmpty()) {
        projectsBuildDir = QDir(projectsBaseDir).absoluteFilePath(projectsBuildDir);
    }

    const QModelIndex set = m_targetsUi->targetsModel.insertTargetSetAfter(projRootIndex, projectName, projectsBuildDir);
    const QString defaultTarget = buildMap.value(QStringLiteral("default_target")).toString();

    const QVariantList targetsets = buildMap.value(QStringLiteral("targets")).toList();
    for (const QVariant &targetVariant : targetsets) {
        const QVariantMap targetMap = targetVariant.toMap();
        const QString tgtName = targetMap[QStringLiteral("name")].toString();
        const QString buildCmd = targetMap[QStringLiteral("build_cmd")].toString();
        const QString runCmd = targetMap[QStringLiteral("run_cmd")].toString();

        if (tgtName.isEmpty() || buildCmd.isEmpty()) {
            continue;
        }
        QPersistentModelIndex idx = m_targetsUi->targetsModel.addCommandAfter(set, tgtName, buildCmd, runCmd);
        if (tgtName == defaultTarget) {
            // A bit of backwards compatibility, move the "default" target to the top
            while (idx.row() > 0) {
                m_targetsUi->targetsModel.moveRowUp(idx);
            }
        }
    }

    const auto index = m_targetsUi->proxyModel.mapFromSource(set);
    if (index.isValid()) {
        m_targetsUi->targetsView->expand(index);
    }

    // FIXME read CMakePresets.json for more build target sets
}

/******************************************************************/
void KateBuildView::saveProjectTargets()
{
    // only do stuff with a valid project
    if (!m_projectPluginView) {
        return;
    }
    qDebug() << "#### SAVING Proj Targets";

    const QModelIndex projRootIndex = m_targetsUi->targetsModel.projectRootIndex();
    const QString projectsBaseDir = m_projectPluginView->property("projectBaseDir").toString();
    const QString userOverride = projectsBaseDir + QStringLiteral("/.kateproject.build");

    QFile file(userOverride);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        displayMessage(i18n("Cannot save build targets in: %1", userOverride), KTextEditor::Message::Error);
        return;
    }

    QJsonObject root = m_targetsUi->targetsModel.indexToJsonObj(projRootIndex);
    root[QStringLiteral("Auto_generated")] = QStringLiteral("This file is auto-generated. Any extra tags or formatting will be lost");
    QJsonDocument doc(root);

    file.write(doc.toJson());
    file.close();
}

/******************************************************************/
bool KateBuildView::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if ((obj == m_toolView) && (ke->key() == Qt::Key_Escape)) {
            m_win->hideToolView(m_toolView);
            event->accept();
            return true;
        }
        break;
    }
    case QEvent::ShortcutOverride: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Copy)) {
            m_buildUi.textBrowser->copy();
            event->accept();
            return true;
        } else if (ke->matches(QKeySequence::SelectAll)) {
            m_buildUi.textBrowser->selectAll();
            event->accept();
            return true;
        }
        break;
    }
    case QEvent::KeyRelease: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Copy) || ke->matches(QKeySequence::SelectAll)) {
            event->accept();
            return true;
        }
        break;
    }
    default: {
    }
    }
    return QObject::eventFilter(obj, event);
}

/******************************************************************/
void KateBuildView::handleEsc(QEvent *e)
{
    if (!m_win) {
        return;
    }

    QKeyEvent *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        if (m_toolView->isVisible()) {
            m_win->hideToolView(m_toolView);
        }
    }
}

#include "moc_plugin_katebuild.cpp"
#include "plugin_katebuild.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
