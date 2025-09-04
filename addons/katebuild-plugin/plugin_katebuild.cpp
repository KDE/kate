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
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QRegularExpressionMatch>
#include <QScrollBar>
#include <QString>
#include <QTextBlock>
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

#include <QPalette>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextEdit>
#include <kde_terminal_interface.h>
#include <kparts/part.h>

using namespace Qt::Literals::StringLiterals;

K_PLUGIN_FACTORY_WITH_JSON(KateBuildPluginFactory, "katebuildplugin.json", registerPlugin<KateBuildPlugin>();)

static const char ConfigAllowedCommands[] = "AllowedCommandLines";
static const char ConfigBlockedCommands[] = "BlockedCommandLines";

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

/******************************************************************/
KateBuildPlugin::KateBuildPlugin(QObject *parent, const VariantList &)
    : KTextEditor::Plugin(parent)
{
    readConfig();
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

    auto *configPage = new KateBuildConfigPage(this, parent);
    return configPage;
}

/******************************************************************/
void KateBuildPlugin::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("BuildConfig"));
    m_addDiagnostics = config.readEntry(QStringLiteral("UseDiagnosticsOutput"), true);
    m_autoSwitchToOutput = config.readEntry(QStringLiteral("AutoSwitchToOutput"), true);
    m_showBuildProgress = config.readEntry("ShowBuildProgress", false);

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
void KateBuildPlugin::writeConfig() const
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("BuildConfig"));
    config.writeEntry("UseDiagnosticsOutput", m_addDiagnostics);
    config.writeEntry("AutoSwitchToOutput", m_autoSwitchToOutput);
    config.writeEntry("ShowBuildProgress", m_showBuildProgress);

    // write allow + block lists as two separate keys
    QStringList allowed;
    QStringList blocked;
    for (const auto &[command, isCommandAllowed] : m_commandLineToAllowedState) {
        if (isCommandAllowed) {
            allowed.push_back(command);
        } else {
            blocked.push_back(command);
        }
    }
    config.writeEntry(ConfigAllowedCommands, allowed);
    config.writeEntry(ConfigBlockedCommands, blocked);
}

/******************************************************************/
KateBuildView::KateBuildView(KateBuildPlugin *plugin, KTextEditor::MainWindow *mw)
    : QObject(mw)
    , m_plugin(plugin)
    , m_win(mw)
    , m_proc(this)
    // NOTE this will not allow spaces in file names.
    // e.g. from gcc: "main.cpp:14: error: cannot convert ‘std::string’ to ‘int’ in return"
    // e.g. from gcc: "main.cpp:14:8: error: cannot convert ‘std::string’ to ‘int’ in return"
    // e.g. from icpc: "main.cpp(14): error: no suitable conversion function from "std::string" to "int" exists"
    // e.g. from clang: ""main.cpp(14,8): fatal error: 'boost/scoped_array.hpp' file not found"
    , m_filenameDetector(QStringLiteral("(?<filename>(?:[a-np-zA-Z]:[\\\\/])?[^\\s:[(]+)[:(](?<line>\\d+)[,:]?(?<column>\\d+)?[):]*\\s*(?<message>.*)"))
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
    m_buildUi.u_outpTopLayout->setContentsMargins(m_buildWidget->style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                                  m_buildWidget->style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                                  m_buildWidget->style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                                  0);
    m_buildUi.search->setContentsMargins(m_buildWidget->style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                         0,
                                         m_buildWidget->style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                         0);
    m_buildUi.search->setHidden(true);

    m_targetsUi = new TargetsUi(this, m_buildUi.u_tabWidget);
    m_buildUi.u_tabWidget->setDocumentMode(true);
    m_buildUi.u_tabWidget->insertTab(0, m_targetsUi, i18nc("Tab label", "Target Settings"));
    m_buildUi.u_tabWidget->setCurrentWidget(m_targetsUi);
    m_buildUi.u_tabWidget->setTabsClosable(true);
    // Remove close button. Don't just hide it or else tab button will still reserve room for it.
    m_buildUi.u_tabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    m_buildUi.u_tabWidget->tabBar()->setTabButton(1, QTabBar::RightSide, nullptr);
    connect(m_buildUi.u_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        // FIXME check if the process is still running
        m_buildUi.u_tabWidget->widget(index)->deleteLater();
    });

    connect(m_buildUi.u_tabWidget->tabBar(), &QTabBar::currentChanged, this, [this]() {
        updateStatusOverlay();
    });

    connect(m_buildUi.u_tabWidget->tabBar(), &QTabBar::tabBarClicked, this, [this](int index) {
        if (QWidget *tabWidget = m_buildUi.u_tabWidget->widget(index)) {
            tabWidget->setFocus();
        }
    });

    m_buildStatusOverlay = new StatusOverlay(m_buildUi.u_statusFrame);

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
        static const QRegularExpression fileRegExp(QStringLiteral("(.*):(\\d+):(\\d+)"));
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
    connect(&m_outputTimer, &QTimer::timeout, this, &KateBuildView::slotUpdateTextBrowser);

    auto updateEditorColors = [this](KTextEditor::Editor *e) {
        if (!e)
            return;
        auto theme = e->theme();
        auto bg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::BackgroundColor));
        auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::TextStyle::Normal));
        auto sel = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::TextSelection));
        auto linkBg = fg;
        auto errBg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::MarkError));
        auto warnBg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::MarkWarning));
        auto noteBg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::MarkBookmark));
        linkBg.setAlpha(15);
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
                                                                               "a:link{color:%1; background-color: %2;}\n"
                                                                               ".err-text {color:%1; background-color: %3;}"
                                                                               ".warn-text {color:%1; background-color: %4;}"
                                                                               ".note-text {color:%1; background-color: %5;}"
                                                                               "pre{margin:0px;}")
                                                                    .arg(fg.name(QColor::HexArgb))
                                                                    .arg(linkBg.name(QColor::HexArgb))
                                                                    .arg(errBg.name(QColor::HexArgb))
                                                                    .arg(warnBg.name(QColor::HexArgb))
                                                                    .arg(noteBg.name(QColor::HexArgb)));
        slotUpdateTextBrowser();
    };
    updateEditorColors(KTextEditor::Editor::instance());
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateEditorColors);

    connect(m_buildUi.buildAgainButton, &QPushButton::clicked, this, &KateBuildView::slotReBuildPreviousTarget);
    connect(m_buildUi.cancelBuildButton, &QPushButton::clicked, this, &KateBuildView::slotStop);

    connect(m_buildUi.searchPattern, &QLineEdit::editingFinished, this, &KateBuildView::slotSearchBuildOutput);
    connect(m_buildUi.searchPattern, &QLineEdit::textChanged, this, &KateBuildView::slotSearchPatternChanged);
    connect(m_buildUi.searchNext, &QToolButton::clicked, this, [this]() {
        gotoNthFound(m_currentFound + 1);
    });
    connect(m_buildUi.searchPrev, &QToolButton::clicked, this, [this]() {
        gotoNthFound(m_currentFound - 1);
    });

    connect(m_targetsUi->buildButton, &QToolButton::clicked, this, &KateBuildView::slotBuildSelectedTarget);
    connect(m_targetsUi->runButton, &QToolButton::clicked, this, &KateBuildView::slotBuildAndRunSelectedTarget);
    connect(m_targetsUi, &TargetsUi::enterPressed, this, &KateBuildView::slotBuildAndRunSelectedTarget);

    m_proc.setOutputChannelMode(KProcess::MergedChannels);
    connect(&m_proc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateBuildView::slotProcExited);
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
    connect(&m_targetsUi->targetsModel, &TargetModel::projectTargetChanged, this, [this](const QString &projectBaseDir) {
        if (!m_addingProjTargets) {
            m_saveProjectTargetDirs.insert(projectBaseDir);
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

    connect(mw->window(), SIGNAL(tabForToolViewAdded(QWidget *, QWidget *)), this, SLOT(tabForToolViewAdded(QWidget *, QWidget *)));
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
    int numTargets = cg.readEntry(u"NumTargets"_s, 0);
    m_projectTargetsetRow = cg.readEntry(u"ProjectTargetSetRow"_s, 0);
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
    updateProjectTargets();

    // pre-select the last active target or the first target of the first set
    const QVector<int> treePath = cg.readEntry(u"Active Target Index Tree"_s, QVector<int>{0, 0, 0});
    QModelIndex activeIndex;
    for (const int row : treePath) {
        const auto idx = m_targetsUi->targetsModel.index(row, 0, activeIndex);
        if (idx.isValid()) {
            activeIndex = idx;
        }
    }
    activeIndex = m_targetsUi->proxyModel.mapFromSource(activeIndex);
    m_targetsUi->targetsView->setCurrentIndex(activeIndex);
    m_targetsUi->targetsView->scrollTo(activeIndex);
    m_targetsUi->updateTargetsButtonStates();
}

/******************************************************************/
void KateBuildView::writeSessionConfig(KConfigGroup &cg)
{
    // Save the active target
    QModelIndex activeIndex = m_targetsUi->targetsView->currentIndex();
    activeIndex = m_targetsUi->proxyModel.mapToSource(activeIndex);
    if (activeIndex.isValid()) {
        QVector<int> treePath;
        treePath.prepend(activeIndex.row());
        while (activeIndex.parent().isValid()) {
            activeIndex = activeIndex.parent();
            treePath.prepend(activeIndex.row());
        }
        cg.writeEntry(u"Active Target Index Tree"_s, treePath);
    }

    // Don't save project target-sets, but save the root-row index
    QModelIndex projRootIndex = m_targetsUi->targetsModel.projectRootIndex();
    m_projectTargetsetRow = projRootIndex.row();
    cg.writeEntry("ProjectTargetSetRow", m_projectTargetsetRow);

    // Save session target-sets
    QModelIndex sessionRootIndex = m_targetsUi->targetsModel.sessionRootIndex();
    QJsonObject sessionSetsObj = m_targetsUi->targetsModel.indexToJsonObj(sessionRootIndex);
    QJsonArray setsArray = sessionSetsObj[QStringLiteral("target_sets")].toArray();

    cg.writeEntry("NumTargets", setsArray.size());

    for (int i = 0; i < setsArray.size(); ++i) {
        auto setObj = setsArray[i].toObject();
        bool loadedViaCMake = setObj[QStringLiteral("loaded_via_cmake")].toBool();
        cg.writeEntry(QStringLiteral("%1 Target").arg(i), setObj[QStringLiteral("name")].toString());
        cg.writeEntry(QStringLiteral("%1 BuildPath").arg(i), setObj[QStringLiteral("directory")].toString());
        cg.writeEntry(QStringLiteral("%1 LoadedViaCMake").arg(i), loadedViaCMake);
        cg.writeEntry(QStringLiteral("%1 CMakeConfig").arg(i), setObj[QStringLiteral("cmake_config")].toString());

        if (loadedViaCMake) {
            // don't save the build commands, they'll be reloaded via the CMake file API
            continue;
        }

        QStringList cmdNames;
        QJsonArray targetsArray = setObj[QStringLiteral("targets")].toArray();

        for (int j = 0; j < targetsArray.size(); ++j) {
            auto targetObj = targetsArray[j].toObject();
            const QString &cmdName = targetObj[QStringLiteral("name")].toString();
            const QString &buildCmd = targetObj[QStringLiteral("build_cmd")].toString();
            const QString &runCmd = targetObj[QStringLiteral("run_cmd")].toString();
            cmdNames << cmdName;
            cg.writeEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(cmdName), buildCmd);
            cg.writeEntry(QStringLiteral("%1 RunCmd %2").arg(i).arg(cmdName), runCmd);
        }
        cg.writeEntry(QStringLiteral("%1 Target Names").arg(i), cmdNames);
    }
}

/******************************************************************/
static Diagnostic createDiagnostic(int line, int column, const QString &message, const DiagnosticSeverity &severity)
{
    Diagnostic d;
    d.message = message;
    d.source = QStringLiteral("katebuild");
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

    if (!m_plugin->m_addDiagnostics) {
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
void KateBuildView::updateDiagnostics(Diagnostic diagnostic, QUrl uri)
{
    FileDiagnostics fd;
    fd.uri = std::move(uri);
    fd.diagnostics.append(std::move(diagnostic));
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
        qDebug("no KTextEditor::View");
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
    m_buildUi.buildStatusLabel->clear();
    m_stdOut.clear();
    m_stdErr.clear();
    m_pendingHtmlOutput.clear();
    m_scrollStopLine = -1;
    m_numOutputLines = 0;
    m_numNonUpdatedLines = 0;
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;
    m_makeDirStack.clear();
    m_progress.clear();
    m_buildCancelled = false;
    m_buildStatusSeen = false;
    m_exitCode = 0;
    updateStatusOverlay();
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

    if (m_plugin->m_autoSwitchToOutput) {
        // activate the output tab
        m_buildUi.u_tabWidget->setCurrentIndex(1);
        m_win->showToolView(m_toolView);
    }

    m_buildUi.u_tabWidget->setTabIcon(1, QIcon::fromTheme(QStringLiteral("system-run")));

    QFont font = Utils::editorFont();
    m_buildUi.textBrowser->setFont(font);

    QModelIndex ind = m_targetsUi->targetsView->currentIndex();
    QString targetSet = ind.data(TargetModel::TargetSetNameRole).toString();

    // set working directory
    m_makeDir = dir;
    m_makeDirStack.push(m_makeDir);

    if (!QFile::exists(m_makeDir)) {
        // The build directory does not exist ask if it should be created
        QMessageBox msgBox(m_win->window());
        msgBox.setWindowTitle(i18n("Create directory"));
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(
            i18n("The configured working directory <b>%1</b> for the targetset <b>%2</b> does not exist.<br><br>"
                 "Create the directory?",
                 m_makeDir,
                 targetSet));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);

        if (msgBox.exec() == QMessageBox::Yes && !QDir().mkpath(m_makeDir)) {
            displayMessage(i18n("Failed to create the directory <b>%1</b>", m_makeDir), KTextEditor::Message::Error);
        }

        if (!QFile::exists(m_makeDir)) {
            return false;
        }
    }

    m_buildStatusSeen = false;

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
    updateStatusOverlay();
    return true;
}

/******************************************************************/
bool KateBuildView::slotStop()
{
    if (m_proc.state() != QProcess::NotRunning) {
        m_buildCancelled = true;
        QString msg = i18n("Building <b>%1: %2</b> cancelled", m_buildTargetSetName, m_buildTargetName);
        m_buildUi.buildStatusLabel->setText(msg);
        terminateProcess(m_proc);
        m_buildTargetSetName.clear();
        m_buildTargetName.clear();
        m_buildBuildCmd.clear();
        m_buildRunCmd.clear();
        m_buildWorkDir.clear();
        updateStatusOverlay();
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
    qCDebug(KTEBUILD, "%s: %ls", Q_FUNC_INFO, qUtf16Printable(compileCommandsFile));
    CompileCommands res;
    res.filename = compileCommandsFile;
    res.date = QFileInfo(compileCommandsFile).lastModified();

    QFile file(compileCommandsFile);
    file.open(QIODevice::ReadOnly);
    QByteArray fileContents = file.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileContents);

    QJsonArray cmds = jsonDoc.array();
    qCDebug(KTEBUILD, "%s: got %lld entries", Q_FUNC_INFO, cmds.count());

    for (int i = 0; i < cmds.count(); i++) {
        QJsonObject cmdObj = cmds.at(i).toObject();
        const QString filename = cmdObj.value(QStringLiteral("file")).toString();
        const QString command = cmdObj.value(QStringLiteral("command")).toString();
        const QString dir = cmdObj.value(QStringLiteral("directory")).toString();
        if (dir.isEmpty() || command.isEmpty() || filename.isEmpty()) {
            qCDebug(KTEBUILD, "parseCompileCommandsFile(): got empty entry at %d !", i);
            continue; // should not happen
        }
        res.commands[filename] = {.workingDir = dir, .command = command};
    }

    return res;
}

/******************************************************************/
void KateBuildView::slotCompileCurrentFile()
{
    qCDebug(KTEBUILD, "slotCompileCurrentFile()");
    KTextEditor::Document *currentDocument = m_win->activeView()->document();
    if (!currentDocument) {
        qCDebug(KTEBUILD, "slotCompileCurrentFile(): no file");
        return;
    }

    const QString currentFile = currentDocument->url().path();
    QString compileCommandsFile = findCompileCommands(currentFile);
    qCDebug(KTEBUILD, "slotCompileCurrentFile(): file: %ls compile_commands: %ls", qUtf16Printable(currentFile), qUtf16Printable(compileCommandsFile));

    if (compileCommandsFile.isEmpty()) {
        QString msg = i18n("Did not find a compile_commands.json for file \"%1\". ", currentFile);
        Utils::showMessage(msg, QIcon::fromTheme(QStringLiteral("run-build")), i18n("Build"), MessageType::Warning, m_win);
        return;
    }

    if ((m_parsedCompileCommands.filename != compileCommandsFile) || (m_parsedCompileCommands.date < QFileInfo(compileCommandsFile).lastModified())) {
        qCDebug(KTEBUILD, "slotCompileCurrentFile(): loading compile_commands.json");
        m_parsedCompileCommands = parseCompileCommandsFile(compileCommandsFile);
    }

    auto it = m_parsedCompileCommands.commands.find(currentFile);

    if (it == m_parsedCompileCommands.commands.end()) {
        QString msg = i18n("Did not find a compile command for file \"%1\" in \"%2\". ", currentFile, compileCommandsFile);
        Utils::showMessage(msg, QIcon::fromTheme(QStringLiteral("run-build")), i18n("Build"), MessageType::Warning, m_win);
        return;
    }

    qCDebug(KTEBUILD, "slotCompileCurrentFile(): starting build: %ls in %ls", qUtf16Printable(it->second.command), qUtf16Printable(it->second.workingDir));
    startProcess(it->second.workingDir, it->second.command);
}

/******************************************************************/
std::optional<QString> KateBuildView::cmdSubstitutionsApplied(const QString &command, const QFileInfo &docFInfo, const QString &workDir)
{
    QString cmd = command;

    // When adding new placeholders, also update the tooltip in TargetHtmlDelegate::createEditor()
    if (docFInfo.absoluteFilePath().isEmpty()) {
        if (cmd.contains(u"%f"_s)) {
            displayMessage(i18n("Cannot make \"%f\" substitution. No open file or the current file is untitled!"), KTextEditor::Message::Error);
            return std::nullopt;
        }
        if (cmd.contains(u"%d"_s)) {
            displayMessage(i18n("Cannot make \"%d\" substitution. No open file or the current file is untitled!"), KTextEditor::Message::Error);
            return std::nullopt;
        }
        if (cmd.contains(u"%n"_s)) {
            displayMessage(i18n("Cannot make \"%n\" substitution. No open file or the current file is untitled!"), KTextEditor::Message::Error);
            return std::nullopt;
        }
    }

    cmd.replace(u"%n"_s, docFInfo.baseName());
    cmd.replace(u"%f"_s, docFInfo.absoluteFilePath());
    cmd.replace(u"%d"_s, docFInfo.absolutePath());

    if (cmd.contains(u"%B"_s)) {
        if (m_targetsUi->currentProjectBaseDir.isEmpty()) {
            displayMessage(i18n("Cannot make project substitution (%B). No open project!"), KTextEditor::Message::Error);
            return std::nullopt;
        }
        cmd.replace(u"%B"_s, docFInfo.absolutePath());
    }
    cmd.replace(u"%w"_s, workDir);
    return cmd;
}

/******************************************************************/
bool KateBuildView::trySetCommands()
{
    if (m_proc.state() != QProcess::NotRunning) {
        displayMessage(i18n("Already building..."), KTextEditor::Message::Warning);
        return false;
    }

    QModelIndex currentIndex = m_targetsUi->targetsView->currentIndex();
    if (!currentIndex.isValid() || (m_firstBuild && !m_targetsUi->targetsView->isVisible())) {
        slotSelectTarget();
        return false;
    }

    // If the index is not a "command row" open the target selection. The first child/command, might not be the
    // intended command
    if (currentIndex.data(TargetModel::RowTypeRole).toInt() != TargetModel::CommandRow) {
        m_targetsUi->targetsView->expand(currentIndex);
        if (currentIndex.data(TargetModel::RowTypeRole).toInt() == TargetModel::RootRow) {
            m_targetsUi->targetsView->expand(currentIndex);
            currentIndex = m_targetsUi->targetsView->model()->index(0, 0, currentIndex.siblingAtColumn(0));
        }
        if (currentIndex.data(TargetModel::RowTypeRole).toInt() == TargetModel::TargetSetRow) {
            m_targetsUi->targetsView->expand(currentIndex);
            currentIndex = m_targetsUi->targetsView->model()->index(0, 0, currentIndex.siblingAtColumn(0));
        }
        m_targetsUi->targetsView->setCurrentIndex(currentIndex);
        slotSelectTarget();
        return false;
    }

    // Now we have a command index
    const QFileInfo docFInfo(docUrl().toLocalFile()); // docUrl() saves the current document

    m_buildTargetSetName = currentIndex.data(TargetModel::TargetSetNameRole).toString();
    m_buildTargetName = currentIndex.data(TargetModel::CommandNameRole).toString();

    m_buildWorkDir = currentIndex.data(TargetModel::WorkDirRole).toString();
    m_buildWorkDir = parseWorkDir(m_buildWorkDir);
    if (m_buildWorkDir.isEmpty()) {
        m_buildWorkDir = docFInfo.absolutePath();
        if (m_buildWorkDir.isEmpty()) {
            displayMessage(i18n("Cannot execute: %1: %2 No working directory set.\n"
                                "There is no local file or directory specified for building.",
                                m_buildTargetSetName,
                                m_buildTargetName),
                           KTextEditor::Message::Error);
            return false;
        }
    }

    // Build command
    m_buildBuildCmd = currentIndex.data(TargetModel::CommandRole).toString();
    auto buildCmd = cmdSubstitutionsApplied(m_buildBuildCmd, docFInfo, m_buildWorkDir);
    if (!buildCmd.has_value()) {
        m_buildBuildCmd.clear();
        slotSelectTarget();
        return false;
    }
    m_buildBuildCmd = buildCmd.value();

    // Run Command
    m_buildRunCmd = currentIndex.data(TargetModel::RunCommandRole).toString();
    auto runCmd = cmdSubstitutionsApplied(m_buildRunCmd, docFInfo, m_buildWorkDir);
    if (!runCmd.has_value()) {
        m_buildBuildCmd.clear();
        m_buildRunCmd.clear();
        slotSelectTarget();
        return false;
    }
    m_buildRunCmd = runCmd.value();

    m_searchPaths = currentIndex.data(TargetModel::SearchPathsRole).toStringList();
    return true;
}

/******************************************************************/
void KateBuildView::slotBuildSelectedTarget()
{
    if (!trySetCommands()) {
        return;
    }

    // don't execute the run command
    m_buildRunCmd.clear();
    if (m_buildBuildCmd.isEmpty()) {
        // We don't have a command to run so open the target selection
        slotSelectTarget();
    }

    buildSelectedTarget();
}

/******************************************************************/
void KateBuildView::slotBuildAndRunSelectedTarget()
{
    if (!trySetCommands()) {
        return;
    }

    buildSelectedTarget();
}

/******************************************************************/
void KateBuildView::slotReBuildPreviousTarget()
{
    if (!m_previousIndex.isValid()) {
        slotSelectTarget();
    } else {
        m_targetsUi->targetsView->setCurrentIndex(m_previousIndex);
        slotBuildSelectedTarget();
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
void KateBuildView::buildSelectedTarget()
{
    m_firstBuild = false;
    m_previousIndex = m_targetsUi->targetsView->currentIndex();

    if (m_proc.state() != QProcess::NotRunning) {
        return;
    }

    clearBuildResults();

    if (m_buildBuildCmd.isEmpty()) {
        slotRunAfterBuild();
        return;
    }

    QString msg = i18n("Building target <b>%1: %2</b> ...", m_buildTargetSetName, m_buildTargetName);
    m_buildUi.buildStatusLabel->setText(msg);
    startProcess(m_buildWorkDir, m_buildBuildCmd);
}

/******************************************************************/
void KateBuildView::slotLoadCMakeTargets()
{
    QString path = QDir::currentPath();
    QUrl url = docUrl();
    QString projRoot;
    if (m_projectPluginView) {
        projRoot = m_projectPluginView->property("projectBaseDir").toString();
    }
    if (!projRoot.isEmpty()) {
        path = projRoot;
    } else if (!url.isEmpty() && url.isLocalFile()) {
        path = QFileInfo(url.toLocalFile()).dir().absolutePath();
    }

    const QString cmakeFile = QFileDialog::getOpenFileName(nullptr,
                                                           QStringLiteral("Select CMake Build Dir by Selecting the CMakeCache.txt"),
                                                           path,
                                                           QStringLiteral("CMake Cache file (CMakeCache.txt)"));
    qCDebug(KTEBUILD, "Loading cmake targets for file %ls", qUtf16Printable(cmakeFile));
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
        qCDebug(KTEBUILD, "Generating CMake reply files failed !");
        sendError(
            i18n("Generating CMake File API reply files for build directory %1 failed (using %2) !", cmakeFA.getBuildDir(), cmakeFA.getCMakeExecutable()));
        return;
    }

    bool success = cmakeFA.readReplyFiles();
    qCDebug(KTEBUILD, "CMake reply success: %d", success);

    QModelIndex rootIndex = m_targetsUi->targetsModel.projectRootIndex();
    for (const QString &config : cmakeFA.getConfigurations()) {
        QString projectName = QStringLiteral("%1@%2 - [%3]").arg(cmakeFA.getProjectName()).arg(cmakeFA.getBuildDir()).arg(config);
        createCMakeTargetSet(rootIndex, projectName, cmakeFA, config);
    }

    const QString compileCommandsDest = cmakeFA.getSourceDir() + QStringLiteral("/compile_commands.json");
#ifdef Q_OS_WIN
    QFile::remove(compileCommandsDest);
    QFile::copy(compileCommandsFile, compileCommandsDest);
#else
    QFile::link(compileCommandsFile, compileCommandsDest);
#endif
}

/******************************************************************/
QModelIndex KateBuildView::createCMakeTargetSet(QModelIndex setIndex, const QString &name, const QCMakeFileApi &cmakeFA, const QString &cmakeConfig)
{
    const int numCores = QThread::idealThreadCount();

    QModelIndex sessionRootIndex = m_targetsUi->targetsModel.sessionRootIndex();
    QJsonObject sessionSetsObj = m_targetsUi->targetsModel.indexToJsonObj(sessionRootIndex);
    QJsonArray setsArray = sessionSetsObj[QStringLiteral("target_sets")].toArray();

    for (const auto &setValue : setsArray) {
        const auto &setObj = setValue.toObject();
        const bool loadedViaCMake = setObj[QStringLiteral("loaded_via_cmake")].toBool();
        const QString setCmakeConfig = setObj[QStringLiteral("cmake_config")].toString();
        const QString buildDir = setObj[QStringLiteral("directory")].toString();
        if (loadedViaCMake && setCmakeConfig == cmakeConfig && buildDir == cmakeFA.getBuildDir()) {
            // this target set has already been loaded, don't add it once more
            return setIndex;
        }
    }

    setIndex = m_targetsUi->targetsModel.insertTargetSetAfter(setIndex, name, cmakeFA.getBuildDir(), true, cmakeConfig, cmakeFA.getSourceDir());
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
    qCDebug(KTEBUILD, "Creating cmake targets, cmakeGui: %ls", qUtf16Printable(cmakeGui));
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
    if (const auto it = m_plugin->m_commandLineToAllowedState.find(fullCommandLineString); it != m_plugin->m_commandLineToAllowedState.end()) {
        return it->second;
    }

    // ask user if the start should be allowed
    QPointer<QMessageBox> msgBox(new QMessageBox(m_win->window()));
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
    m_plugin->m_commandLineToAllowedState.emplace(fullCommandLineString, allowed);

    // inform the user if it was forbidden! do this here to just emit this once
    // if (!allowed) {
    // Q_EMIT showMessage(KTextEditor::Message::Information,
    // i18n("User permanently blocked start of: '%1'.\nUse the config page of the plugin to undo this block.", fullCommandLineString));
    // }

    // flush config to not loose that setting
    m_plugin->writeConfig();
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
    m_infoMessage = new KTextEditor::Message(xi18nc("@info", "<title>Build Finished:</title>%1", msg), level);
    m_infoMessage->setWordWrap(false);
    m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
    m_infoMessage->setAutoHide(5000);
    m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
    m_infoMessage->setView(kv);
    kv->document()->postMessage(m_infoMessage);
    updateStatusOverlay();
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
void KateBuildView::displayProgress(const QString &msg, KTextEditor::Message::MessageType level)
{
    KTextEditor::View *kv = m_win->activeView();
    if (!kv) {
        return;
    }

    // delete the message if needed
    if (m_progressMessage && (m_progressMessage->view() != kv || m_progressMessage->messageType() != level)) {
        delete m_progressMessage;
    }

    if (!m_progressMessage) {
        m_progressMessage = new KTextEditor::Message(msg, level);
        m_progressMessage->setWordWrap(false);
        m_progressMessage->setPosition(KTextEditor::Message::BottomInView);
        m_progressMessage->setAutoHide(100000000);
        m_progressMessage->setAutoHideMode(KTextEditor::Message::Immediate);
        m_progressMessage->setView(kv);
        kv->document()->postMessage(m_progressMessage);
    } else {
        m_progressMessage->setText(msg);
    }
}
/******************************************************************/
void KateBuildView::slotProcExited(int exitCode, QProcess::ExitStatus)
{
    m_targetsUi->unsetCursor();
    m_buildUi.u_tabWidget->setTabIcon(1, QIcon::fromTheme(QStringLiteral("format-justify-left")));
    m_buildUi.cancelBuildButton->setEnabled(false);
    m_buildUi.buildAgainButton->setEnabled(true);
    delete m_progressMessage;
    m_progress.clear();

    QString buildStatus = i18n("Build <b>%1: %2</b> completed. %3 error(s), %4 warning(s), %5 note(s)",
                               m_buildTargetSetName,
                               m_buildTargetName,
                               m_numErrors,
                               m_numWarnings,
                               m_numNotes);

    m_exitCode = exitCode;
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
    } else if (m_buildCancelled) {
        displayBuildResult(i18n("Build canceled."), KTextEditor::Message::Warning);
        buildSuccess = false;
    } else if (exitCode != 0) {
        buildSuccess = false;
        displayBuildResult(i18n("Build failed."), KTextEditor::Message::Warning);
    } else {
        displayBuildResult(i18n("Build completed without problems."), KTextEditor::Message::Positive);
    }

    if (m_buildCancelled) {
        buildStatus = i18n("Build <b>%1: %2 canceled</b>. %3 error(s), %4 warning(s), %5 note(s)",
                           m_buildTargetSetName,
                           m_buildTargetName,
                           m_numErrors,
                           m_numWarnings,
                           m_numNotes);
    }
    m_buildUi.buildStatusLabel->setText(buildStatus);

    slotUpdateTextBrowser();

    m_buildBuildCmd.clear();
    if (buildSuccess) {
        slotRunAfterBuild();
    }
}

void KateBuildView::slotUpdateRunTabs()
{
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
}

void KateBuildView::slotRunAfterBuild()
{
    if (m_buildRunCmd.isEmpty()) {
        return;
    }

    AppOutput *out = nullptr;
    for (int i = 2; i < m_buildUi.u_tabWidget->count(); ++i) {
        QString tabToolTip = m_buildUi.u_tabWidget->tabToolTip(i);
        if (m_buildRunCmd == tabToolTip) {
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
        int tabIndex = m_buildUi.u_tabWidget->addTab(out, m_buildTargetName);
        m_buildUi.u_tabWidget->setCurrentIndex(tabIndex);
        m_buildUi.u_tabWidget->setTabToolTip(tabIndex, m_buildRunCmd);
        m_buildUi.u_tabWidget->setTabIcon(tabIndex, QIcon::fromTheme(QStringLiteral("media-playback-start")));

        // Update the tab icon when the run state changes
        connect(out, &AppOutput::runningChanged, this, &KateBuildView::slotUpdateRunTabs);
        // Clear the running icon in case the command exits faster than the runningChanged timeout
        QTimer::singleShot(1000, this, &KateBuildView::slotUpdateRunTabs);
    }

    out->setWorkingDir(m_buildWorkDir);
    out->runCommand(m_buildRunCmd);

    if (m_win->activeView()) {
        m_win->activeView()->setFocus();
    }

    m_buildTargetSetName.clear();
    m_buildTargetName.clear();
    m_buildBuildCmd.clear();
    m_buildRunCmd.clear();
    m_buildWorkDir.clear();
}

QString KateBuildView::toOutputHtml(const KateBuildView::OutputLine &out)
{
    QString htmlStr = u"<pre>"_s;
    if (!out.file.isEmpty()) {
        htmlStr += u"<a href=\"%1:%2:%3\">"_s.arg(out.file).arg(out.lineNr).arg(out.column);
    }
    switch (out.category) {
    case Category::Error:
        htmlStr += u"<span class=\"err-text\">"_s;
        break;
    case Category::Warning:
        htmlStr += u"<span class=\"warn-text\">"_s;
        break;
    case Category::Info:
        htmlStr += u"<span class=\"note-text\">"_s;
        break;
    case Category::Normal:
        htmlStr += u"<span>"_s;
        break;
    }
    htmlStr += out.lineStr.toHtmlEscaped();
    htmlStr += u"</span>"_s;
    if (!out.file.isEmpty()) {
        htmlStr += u"</a>"_s;
    }
    htmlStr += u"</pre>\n"_s;

    return htmlStr;
}

void KateBuildView::slotUpdateTextBrowser()
{
    if (m_pendingHtmlOutput.isEmpty()) {
        return;
    }

    QTextBrowser *edit = m_buildUi.textBrowser;
    // Get the scroll position to restore it if not at the end
    int scrollValuePx = edit->verticalScrollBar()->value();
    int scrollMaxPx = edit->verticalScrollBar()->maximum();
    int scrollPageStepPx = edit->verticalScrollBar()->pageStep();

    if ((scrollMaxPx - scrollValuePx) < (scrollPageStepPx / 20)) {
        // In case we are at the end of the output stay there
        scrollValuePx = std::numeric_limits<int>::max();
        // Except if we have a "stop line" (and some output)
        double pxPerLine = 0.0;
        if ((m_numOutputLines - m_numNonUpdatedLines) > 0) {
            pxPerLine = (double)(scrollMaxPx + scrollPageStepPx) / (m_numOutputLines - m_numNonUpdatedLines);
        } else {
            // Fallback pxPerLine uses font size plus magic multiplier for line spacing
            pxPerLine = edit->fontInfo().pixelSize() * 1.17;
        }

        if (m_scrollStopLine != -1) {
            if (pxPerLine > 1) {
                int stopLine = std::max(m_scrollStopLine - 6, 0);
                scrollValuePx = stopLine * pxPerLine;
            } else {
                // Fallback add one empty line
                qDebug("Have no known line height");
            }
        }
    }

    if (scrollValuePx < scrollMaxPx) {
        m_scrollStopLine = -1;
    }

    // Save the selection and restore it after adding the text
    QTextCursor saveCursor = edit->textCursor();

    // Add the new lines
    QTextCursor cursor = saveCursor;
    cursor.movePosition(QTextCursor::End);
    // NOTE: The insertHTML() is tricky as we do not have control over the internal structures.
    // We add this <pre/> tag to add a new line for the next iteration so that we do not get two separate lines
    // written on the same line.
    m_pendingHtmlOutput += u"<pre/>"_s;
    cursor.insertHtml(m_pendingHtmlOutput);
    m_pendingHtmlOutput.clear();
    // Restore selection and scroll position
    edit->setTextCursor(saveCursor);

    m_numNonUpdatedLines = 0;
    edit->verticalScrollBar()->setValue(scrollValuePx);

    bool outputVisible = m_buildUi.u_tabWidget->currentIndex() == 1 && m_toolView->isVisible();

    KTextEditor::Message::MessageType type = m_numErrors != 0 ? KTextEditor::Message::Error
        : m_numWarnings != 0                                  ? KTextEditor::Message::Warning
                                                              : KTextEditor::Message::Information;
    bool running = m_proc.state() != QProcess::NotRunning;
    if (!outputVisible && m_plugin->m_showBuildProgress && !m_progress.isEmpty() && running) {
        displayProgress(m_progress, type);
    } else {
        delete m_progressMessage;
    }
    updateStatusOverlay();
}

/******************************************************************/
void KateBuildView::slotReadReadyStdOut()
{
    // read data from procs stdout and add
    // the text to the end of the output
    QString l = QString::fromUtf8(m_proc.readAllStandardOutput());
    l.remove(QLatin1Char('\r'));
    m_stdOut += l;

    static const QRegularExpression progressReg(u"(?<progress>\\[\\d+/\\d+\\]|\\[\\s*\\d+%\\]).*"_s);
    // handle one line at a time
    int end = -1;
    int start = 0;
    while ((end = m_stdOut.indexOf(u'\n', start)) >= 0) {
        const QString line = m_stdOut.mid(start, end - start);
        // Check if this is a new directory for Make
        QRegularExpressionMatch match = m_newDirDetector.matchView(line);
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
        m_pendingHtmlOutput += toOutputHtml(out);
        m_numOutputLines++;
        m_numNonUpdatedLines++;
        if (out.category != Category::Normal) {
            addError(out);
            if (m_scrollStopLine == -1) {
                m_scrollStopLine = m_numOutputLines;
            }
        }

        QRegularExpressionMatch progressMatch = progressReg.matchView(line);
        if (progressMatch.hasMatch()) {
            m_progress = progressMatch.captured(u"progress"_s);
        }

        start = end + 1;
    }

    if (!m_stdOut.endsWith(u'\n')) {
        auto i = m_stdOut.lastIndexOf(u'\n');
        if (i != -1) {
            m_stdOut.remove(0, i + 1);
        }
    } else {
        m_stdOut.clear();
    }

    if (!m_outputTimer.isActive()) {
        m_outputTimer.start();
    }
}

/******************************************************************/
KateBuildView::OutputLine KateBuildView::processOutputLine(const QString &line)
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

    // qDebug("File Name:")<<m_makeDir << filename << " msg:"<< msg;
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
    static const QRegularExpression errorRegExp(QStringLiteral("error:"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression errorRegExpTr(QStringLiteral("%1:").arg(i18nc("The same word as 'gcc' uses for an error.", "error")),
                                                  QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression warningRegExp(QStringLiteral("warning:"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression warningRegExpTr(QStringLiteral("%1:").arg(i18nc("The same word as 'gcc' uses for a warning.", "warning")),
                                                    QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression infoRegExp(QStringLiteral("(info|note):"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression infoRegExpTr(QStringLiteral("(%1):").arg(i18nc("The same words as 'gcc' uses for note or info.", "note|info")),
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

/** Build a list of search results */
void KateBuildView::doSearchAll(QString text)
{
    auto buildOutput = m_buildUi.textBrowser;

    if (buildOutput->document()->isEmpty() || text.isEmpty()) {
        return;
    }

    m_searchFound.clear();

    auto saveCursor = buildOutput->textCursor();
    auto cursor = saveCursor;
    cursor.movePosition(QTextCursor::Start);
    buildOutput->setTextCursor(cursor);

    while (buildOutput->find(text)) {
        m_searchFound.append(buildOutput->textCursor());
    }

    buildOutput->setTextCursor(saveCursor);
}

/** Change the "Base" color of a widget. */
/** For QLineEdit this is the background color. */
void setBaseColor(QWidget *w, const QColor &color)
{
    if (w == nullptr) {
        return;
    }

    auto palette = w->palette();
    palette.setColor(QPalette::Base, color);
    w->setPalette(palette);
}

/** Highlights the Nth search result. */
void KateBuildView::gotoNthFound(qsizetype n)
{
    auto buildOutput = m_buildUi.textBrowser;
    auto searchText = m_buildUi.searchPattern->text();

    if (buildOutput->document()->isEmpty() || searchText.isEmpty()) {
        m_buildUi.searchStatus->clear();
        return;
    }

    if (m_searchFound.empty()) {
        doSearchAll(searchText);
        if (m_searchFound.empty()) {
            m_buildUi.searchStatus->clear();
            setBaseColor(m_buildUi.searchPattern, KColorScheme().background(KColorScheme::NegativeBackground).color());
            return;
        }
        n = 0;
    }

    if (n < 0)
        n = m_searchFound.size() - 1;
    if (n >= m_searchFound.size())
        n = 0;

    m_buildUi.searchStatus->setText(QStringLiteral("%1/%2").arg(n + 1).arg(m_searchFound.size()));
    buildOutput->setTextCursor(m_searchFound.at(n));
    m_currentFound = n;
}

/** The user has entered a search pattern. */
void KateBuildView::slotSearchBuildOutput()
{
    auto input = qobject_cast<QLineEdit *>(sender());
    if (input == nullptr) {
        return;
    }

    if (input->text().isEmpty()) {
        return;
    }

    doSearchAll(input->text());
    KColorScheme c;
    setBaseColor(input,
                 m_searchFound.size() > 0 ? c.background(KColorScheme::PositiveBackground).color() : c.background(KColorScheme::NegativeBackground).color());
    gotoNthFound(0);
}

/** The user is changing the search pattern. */
void KateBuildView::slotSearchPatternChanged()
{
    m_searchFound = {};
    m_currentFound = 0;

    auto cursor = m_buildUi.textBrowser->textCursor();
    cursor.clearSelection();
    m_buildUi.textBrowser->setTextCursor(cursor);

    setBaseColor(m_buildUi.searchPattern, KColorScheme().background(KColorScheme::NormalBackground).color());
    m_buildUi.searchStatus->clear();
}

/******************************************************************/
void KateBuildView::slotPluginViewCreated(const QString &name, QObject *pluginView)
{
    // add view
    if (pluginView && name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = pluginView;
        updateProjectTargets();
        pluginView->disconnect(this);
        connect(pluginView, SIGNAL(projectMapEdited()), this, SLOT(slotProjectMapEdited()), Qt::QueuedConnection);
        connect(pluginView, SIGNAL(pluginProjectAdded(QString, QString)), this, SLOT(slotProjectMapEdited()), Qt::QueuedConnection);
        connect(pluginView, SIGNAL(pluginProjectRemoved(QString, QString)), this, SLOT(slotProjectMapEdited()), Qt::QueuedConnection);
        connect(pluginView, SIGNAL(projectMapChanged()), this, SLOT(slotProjectChanged()), Qt::QueuedConnection);
        slotProjectChanged();
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewDeleted(const QString &name, QObject *)
{
    // remove view
    if (name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = nullptr;
        m_targetsUi->targetsModel.deleteProjectTargetsExcept();
        slotProjectChanged();
    }
}

/******************************************************************/
void KateBuildView::slotProjectMapEdited()
{
    // only do stuff with valid project
    if (!m_projectPluginView) {
        m_targetsUi->targetsModel.deleteProjectTargetsExcept();
        return;
    }
    updateProjectTargets();
}

/******************************************************************/
void KateBuildView::slotProjectChanged()
{
    if (m_projectPluginView) {
        m_targetsUi->currentProjectBaseDir = m_projectPluginView->property("projectBaseDir").toString();
    } else {
        m_targetsUi->currentProjectBaseDir.clear();
    }
}

/******************************************************************/
void KateBuildView::updateProjectTargets()
{
    // only do stuff with a valid project
    if (!m_projectPluginView) {
        return;
    }

    BoolTrueLocker locker(m_addingProjTargets);

    const auto allProjects = m_projectPluginView->property("allProjects").value<QMap<QString, QString>>();

    const QModelIndex projRootIndex = m_targetsUi->targetsModel.projectRootIndex();
    // Delete any old project plugin targets
    m_targetsUi->targetsModel.deleteProjectTargetsExcept(allProjects.keys());

    for (const auto &[projectBaseDir, projectName] : allProjects.asKeyValueRange()) {
        QJsonObject root;
        // Read the targets from the override if available
        const QString userOverride = projectBaseDir + u"/.kateproject.build"_s;
        if (QFile::exists(userOverride)) {
            // We have user modified commands
            QFile file(userOverride);
            if (file.open(QIODevice::ReadOnly)) {
                const QByteArray jsonString = file.readAll();
                QJsonParseError error;
                const QJsonDocument doc = QJsonDocument::fromJson(jsonString, &error);
                if (error.error != QJsonParseError::NoError) {
                    qWarning("Could not parse the provided Json");
                } else {
                    root = doc.object();
                    QJsonArray targetSets = root[u"target_sets"_s].toArray();
                    for (int i = targetSets.size() - 1; i >= 0; --i) {
                        QJsonObject set = targetSets[i].toObject();
                        if (set[u"loaded_via_cmake"_s].toBool()) {
                            QString buildDir = set[u"directory"_s].toString();
                            QTimer::singleShot(0, this, [this, buildDir]() {
                                loadCMakeTargets(buildDir);
                            });
                            targetSets.removeAt(i);
                        }
                    }
                    root.insert(u"target_sets"_s, targetSets);
                    root.remove(u"Auto_generated");
                }
            }
        } else {
            // do we have a valid map for build settings?
            QVariantMap projectMap;
            QMetaObject::invokeMethod(m_projectPluginView,
                                      "projectMapFor",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(QVariantMap, projectMap),
                                      Q_ARG(QString, projectBaseDir));

            // ignore it if we have no targets there
            const QVariantMap buildMap = projectMap.value(u"build"_s).toMap();
            const QVariantList targets = buildMap.value(u"targets"_s).toList();
            if (targets.isEmpty()) {
                continue;
            }

            // handle build directory as relative to project file, if possible, see bug 413306
            QString projectsBuildDir = buildMap.value(u"directory"_s).toString();
            projectsBuildDir = QDir(projectBaseDir).absoluteFilePath(projectsBuildDir);
            QJsonObject jsonTargetSet;
            jsonTargetSet.insert(u"cmake_config"_s, QString());
            jsonTargetSet.insert(u"directory"_s, projectsBuildDir);
            jsonTargetSet.insert(u"loaded_via_cmake"_s, false);
            jsonTargetSet.insert(u"name"_s, projectMap.value(u"name"_s).toString());

            const QString defaultTarget = buildMap.value(u"default_target"_s).toString();
            QJsonArray jsonTargets;
            for (const QVariant &targetVariant : targets) {
                QJsonObject jsonTarget;

                const QVariantMap targetMap = targetVariant.toMap();
                jsonTarget.insert(u"name"_s, targetMap[u"name"_s].toString());
                jsonTarget.insert(u"build_cmd"_s, targetMap[u"build_cmd"_s].toString());
                jsonTarget.insert(u"run_cmd"_s, targetMap[u"run_cmd"_s].toString());
                jsonTargets.append(jsonTarget);
            }
            jsonTargetSet.insert(u"targets", jsonTargets);
            QJsonArray jsonTargetSets;
            jsonTargetSets.append(jsonTargetSet);
            root.insert(u"target_sets"_s, jsonTargetSets);
        }

        QJsonObject currentRoot = m_targetsUi->targetsModel.projectTargetsToJsonObj(projectBaseDir);
        if (currentRoot != root) {
            // TODO Create an update projectarget function that only updates the targets and does not delete and recreate
            m_targetsUi->targetsModel.deleteProjectTargets(projectBaseDir);
            QModelIndex addedIndex = m_targetsUi->targetsModel.insertAfter(projRootIndex, root, projectBaseDir);
            if (addedIndex != projRootIndex) {
                continue;
            }
        }
    }
}

/******************************************************************/
void KateBuildView::saveProjectTargets()
{
    // only do stuff with a valid project
    if (!m_projectPluginView) {
        return;
    }

    const QSet<QString> dirs = m_saveProjectTargetDirs;
    m_saveProjectTargetDirs = {};

    // traverse over all project directories
    for (const auto &projectBaseDir : dirs) {
        const QString userOverride = projectBaseDir + u"/.kateproject.build"_s;
        // get the target sets for this project
        QJsonObject root = m_targetsUi->targetsModel.projectTargetsToJsonObj(projectBaseDir);
        if (root.isEmpty()) {
            // if there are not target remove the override
            QFile::remove(userOverride);
            continue;
        }
        root[u"Auto_generated"] = u"This file is auto-generated. Any extra tags or formatting will be lost"_s;
        // Do not save CMake auto generated targets
        QJsonArray targetSets = root[u"target_sets"_s].toArray();
        for (int i = targetSets.size() - 1; i >= 0; --i) {
            QJsonObject set = targetSets[i].toObject();
            if (set[u"loaded_via_cmake"_s].toBool()) {
                // These will be loaded again when loadCMakeTargets is called
                set[u"targets"_s] = {};
                targetSets[i] = set;
            }
        }

        if (targetSets.isEmpty()) {
            // Nothing left for the userOverride -> remove the file
            QFile::remove(userOverride);
            // There are no target-sets left, try reloading the original project-targets
            updateProjectTargets();
            return;
        }
        root[u"target_sets"_s] = targetSets;

        QFile file(userOverride);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            displayMessage(i18n("Cannot save build targets in: %1", userOverride), KTextEditor::Message::Error);
            return;
        }
        QJsonDocument doc(root);
        file.write(doc.toJson());
        file.close();
    }
}

/******************************************************************/
bool KateBuildView::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress: {
        auto *ke = static_cast<QKeyEvent *>(event);
        if ((obj == m_toolView) && (ke->key() == Qt::Key_Escape)) {
            m_win->hideToolView(m_toolView);
            event->accept();
            return true;
        }
        break;
    }
    case QEvent::ShortcutOverride: {
        auto *ke = static_cast<QKeyEvent *>(event);
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
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Copy) || ke->matches(QKeySequence::SelectAll)) {
            event->accept();
            return true;
        }
        break;
    }
    case QEvent::Show:
        updateStatusOverlay();
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

    auto *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        if (m_toolView->isVisible()) {
            m_win->hideToolView(m_toolView);
        }
    }
}

void KateBuildView::tabForToolViewAdded(QWidget *toolView, QWidget *tab)
{
    if (m_toolView == toolView) {
        m_tabStatusOverlay = new StatusOverlay(tab);
    }
}

static double toProgress(const QString &progress)
{
    if (!progress.isEmpty()) {
        static const QRegularExpression ratioReg(u"\\[(\\d+)/(\\d+)\\]"_s);
        static const QRegularExpression percentReg(u"\\[\\s*(\\d+)%\\]"_s);

        if (const auto &match = ratioReg.match(progress); match.hasMatch()) {
            double dividend = match.captured(1).toInt();
            int divisor = match.captured(2).toInt();
            return divisor == 0 ? 0.0 : dividend / divisor;
        } else if (const auto &match = percentReg.match(progress); match.hasMatch()) {
            return match.captured(1).toInt();
        }
    }
    return 0.0;
}

void KateBuildView::updateStatusOverlay()
{
    if (!m_tabStatusOverlay) {
        return;
    }
    bool running = m_proc.state() != QProcess::NotRunning;

    StatusOverlay::Type type = StatusOverlay::Type::None;
    if (m_numErrors != 0) {
        type = StatusOverlay::Type::Error;
    } else if (m_numWarnings != 0 || m_buildCancelled || m_exitCode != 0) {
        type = StatusOverlay::Type::Warning;
    } else if (running) {
        type = StatusOverlay::Type::Positive;
    } else if (!m_buildUi.textBrowser->document()->isEmpty()) {
        type = StatusOverlay::Type::Positive;
    }

    double progress = toProgress(m_progress);

    m_buildStatusOverlay->setType(type);
    m_buildStatusOverlay->setGlowing(running);
    m_buildStatusOverlay->setProgress(progress);

    bool outputVisible = m_buildUi.u_tabWidget->currentIndex() == 1 && m_toolView->isVisible();
    if (outputVisible && !running) {
        m_buildStatusSeen = true;
    }

    if (m_buildStatusSeen || outputVisible) {
        type = StatusOverlay::Type::None;
        progress = 0;
    }
    m_tabStatusOverlay->setType(type);
    m_tabStatusOverlay->setGlowing(running);
    m_tabStatusOverlay->setProgress(progress);
}

#include "moc_plugin_katebuild.cpp"
#include "plugin_katebuild.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
