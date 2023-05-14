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

#include "hostprocess.h"

#include <cassert>

#include <QApplication>
#include <QCompleter>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QKeyEvent>
#include <QRegularExpressionMatch>
#include <QScrollBar>
#include <QString>
#include <QTimer>

#include <QAction>

#include <KActionCollection>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>

#include <KAboutData>
#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>
#include <KXMLGUIFactory>

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
#else
static QString caseFixed(const QString &path)
{
    return path;
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
KateBuildView::KateBuildView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw)
    : QObject(mw)
    , m_win(mw)
    , m_buildWidget(nullptr)
    , m_outputWidgetWidth(0)
    , m_proc(this)
    , m_stdOut()
    , m_stdErr()
    , m_buildCancelled(false)
    , m_displayModeBeforeBuild(1)
    // NOTE this will not allow spaces in file names.
    // e.g. from gcc: "main.cpp:14: error: cannot convert ‘std::string’ to ‘int’ in return"
    // e.g. from gcc: "main.cpp:14:8: error: cannot convert ‘std::string’ to ‘int’ in return"
    // e.g. from icpc: "main.cpp(14): error: no suitable conversion function from "std::string" to "int" exists"
    // e.g. from clang: ""main.cpp(14,8): fatal error: 'boost/scoped_array.hpp' file not found"
    , m_filenameDetector(QStringLiteral("((?:[a-np-zA-Z]:[\\\\/])?[^\\s:(]+)[:\\(](\\d+)[,:]?(\\d+)?[\\):]* (.*)"))
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

    a = actionCollection()->addAction(QStringLiteral("stop"));
    a->setText(i18n("Stop"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotStop);

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
    connect(m_targetsUi->copyTarget, &QToolButton::clicked, this, &KateBuildView::targetOrSetCopy);
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
}

/******************************************************************/
KateBuildView::~KateBuildView()
{
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.terminate();
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
    m_targetsUi->targetsModel.clear();

    for (int i = 0; i < numTargets; i++) {
        QStringList targetNames = cg.readEntry(QStringLiteral("%1 Target Names").arg(i), QStringList());
        QString targetSetName = cg.readEntry(QStringLiteral("%1 Target").arg(i), QString());
        QString buildDir = cg.readEntry(QStringLiteral("%1 BuildPath").arg(i), QString());

        QModelIndex index = m_targetsUi->targetsModel.addTargetSet(targetSetName, buildDir);

        // Keep a bit of backwards compatibility by ensuring that the "default" target is the first in the list
        QString defCmd = cg.readEntry(QStringLiteral("%1 Target Default").arg(i), QString());
        int defIndex = targetNames.indexOf(defCmd);
        if (defIndex > 0) {
            targetNames.move(defIndex, 0);
        }
        for (int tn = 0; tn < targetNames.size(); ++tn) {
            const QString &targetName = targetNames.at(tn);
            const QString &buildCmd = cg.readEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(targetName));
            const QString &runCmd = cg.readEntry(QStringLiteral("%1 RunCmd %2").arg(i).arg(targetName));
            m_targetsUi->targetsModel.addCommand(index, targetName, buildCmd, runCmd);
        }
    }

    // Add project targets, if any
    addProjectTarget();

    m_targetsUi->targetsView->expandAll();

    // pre-select the last active target or the first target of the first set
    int prevTargetSetRow = cg.readEntry(QStringLiteral("Active Target Index"), 0);
    int prevCmdRow = cg.readEntry(QStringLiteral("Active Target Command"), 0);
    QModelIndex rootIndex = m_targetsUi->targetsModel.index(prevTargetSetRow);
    QModelIndex cmdIndex = m_targetsUi->targetsModel.index(prevCmdRow, 0, rootIndex);
    cmdIndex = m_targetsUi->proxyModel.mapFromSource(cmdIndex);
    m_targetsUi->targetsView->setCurrentIndex(cmdIndex);

    m_targetsUi->updateTargetsButtonStates();
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

    QList<TargetModel::TargetSet> targets = m_targetsUi->targetsModel.targetSets();

    // Don't save project target-set, but save the row index
    m_projectTargetsetRow = 0;
    for (int i = 0; i < targets.size(); i++) {
        if (i18n("Project Plugin Targets") == targets[i].name) {
            m_projectTargetsetRow = i;
            targets.removeAt(i);
            break;
        }
    }

    cg.writeEntry("ProjectTargetSetRow", m_projectTargetsetRow);
    cg.writeEntry("NumTargets", targets.size());

    for (int i = 0; i < targets.size(); i++) {
        cg.writeEntry(QStringLiteral("%1 Target").arg(i), targets[i].name);
        cg.writeEntry(QStringLiteral("%1 BuildPath").arg(i), targets[i].workDir);
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

    // NOTE: Limit the number of items in the diagnostics view to 200 items.
    // Adding more items risks making the build slow. (standard item models are slow)
    if ((m_numErrors + m_numWarnings + m_numNotes) > 200) {
        return;
    }
    updateDiagnostics(KateBuildView::createDiagnostic(err.lineNr, err.column, err.message, severity), uri);
}

void KateBuildView::updateDiagnostics(Diagnostic diagnostic, const QUrl uri)
{
    FileDiagnostics fd;
    fd.uri = uri;
    fd.diagnostics.append(diagnostic);
    Q_EMIT m_diagnosticsProvider.diagnosticsAdded(fd);
}

void KateBuildView::clearDiagnostics()
{
    Q_EMIT m_diagnosticsProvider.requestClearDiagnostics(&m_diagnosticsProvider);
}

Diagnostic KateBuildView::createDiagnostic(int line, int column, const QString &message, const DiagnosticSeverity &severity)
{
    Diagnostic d;
    d.message = message;
    d.source = DiagnosticsPrefix;
    d.severity = severity;
    d.range = KTextEditor::Range(KTextEditor::Cursor(line - 1, column - 1), 0);
    return d;
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

/******************************************************************/
bool KateBuildView::checkLocal(const QUrl &dir)
{
    if (dir.path().isEmpty()) {
        KMessageBox::error(nullptr, i18n("There is no file or directory specified for building."));
        return false;
    } else if (!dir.isLocalFile()) {
        KMessageBox::error(nullptr,
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
bool KateBuildView::startProcess(const QString &dir, const QString &command)
{
    if (m_proc.state() != QProcess::NotRunning) {
        return false;
    }

    // clear previous runs
    clearBuildResults();

    // activate the output tab
    m_buildUi.u_tabWidget->setCurrentIndex(1);
    m_buildUi.u_tabWidget->setTabIcon(1, QIcon::fromTheme(QStringLiteral("system-run")));
    m_win->showToolView(m_toolView);

    QFont font = Utils::editorFont();
    m_buildUi.textBrowser->setFont(font);

    // set working directory
    m_makeDir = dir;
    m_makeDirStack.push(m_makeDir);

    if (!QFile::exists(m_makeDir)) {
        KMessageBox::error(nullptr, i18n("Cannot run command: %1\nWork path does not exist: %2", command, m_makeDir));
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
        KMessageBox::error(nullptr, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc.exitStatus()));
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
        m_proc.terminate();
        return true;
    }
    return false;
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
        KMessageBox::error(nullptr, i18n("No target available for building."));
        return false;
    }

    QString buildCmd = TargetModel::command(ind);
    QString cmdName = TargetModel::cmdName(ind);
    m_searchPaths = TargetModel::searchPaths(ind);
    QString workDir = TargetModel::workDir(ind);
    QString targetSet = TargetModel::targetName(ind);

    QString dir = workDir;
    if (workDir.isEmpty()) {
        dir = docFInfo.absolutePath();
        if (dir.isEmpty()) {
            KMessageBox::error(nullptr, i18n("There is no local file or directory specified for building."));
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
    // a single target can serve to build lots of projects with similar directory layout
    if (m_projectPluginView) {
        const QFileInfo baseDir(m_projectPluginView->property("projectBaseDir").toString());
        dir.replace(QStringLiteral("%B"), baseDir.absoluteFilePath());
        dir.replace(QStringLiteral("%b"), baseDir.baseName());
    }

    // Check if the command contains the file name or directory
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
void KateBuildView::displayBuildResult(const QString &msg, KTextEditor::Message::MessageType level)
{
    KTextEditor::View *kv = m_win->activeView();
    if (!kv) {
        return;
    }

    delete m_infoMessage;
    m_infoMessage = new KTextEditor::Message(xi18nc("@info", "<title>Make Results:</title><nl/>%1", msg), level);
    m_infoMessage->setWordWrap(true);
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
    m_infoMessage->setWordWrap(true);
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

    // Filter the diagnostics output to make the next/previous error handle only the build items
    if ((m_numErrors + m_numWarnings + m_numNotes) > 0) {
        m_diagnosticsProvider.filterDiagnosticsViewTo(&m_diagnosticsProvider);
    }

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
    const QString workDir = TargetModel::workDir(idx);
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
        return {Category::Normal, line, line, QString(), 0, 0};
    }

    QString filename = match.captured(1);
    const QString line_n = match.captured(2);
    const QString col_n = match.captured(3);
    const QString msg = match.captured(4);

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
    return {category, line, msg, filename, line_n.toInt(), col_n.toInt()};
}

/******************************************************************/
void KateBuildView::slotAddTargetClicked()
{
    QModelIndex current = m_targetsUi->targetsView->currentIndex();
    QString currName = DefTargetName;
    QString currCmd;
    QString currRun;

    if (current.parent().isValid()) {
        // we need the root item
        current = current.parent();
    }
    current = m_targetsUi->proxyModel.mapToSource(current);
    QModelIndex index = m_targetsUi->targetsModel.addCommand(current, currName, currCmd, currRun);
    index = m_targetsUi->proxyModel.mapFromSource(index);
    m_targetsUi->targetsView->setCurrentIndex(index);
}

/******************************************************************/
void KateBuildView::targetSetNew()
{
    m_targetsUi->targetFilterEdit->setText(QString());
    QModelIndex index = m_targetsUi->targetsModel.addTargetSet(i18n("Target Set"), QString());
    QModelIndex buildIndex = m_targetsUi->targetsModel.addCommand(index, i18n("Build"), DefBuildCmd, QString());
    m_targetsUi->targetsModel.addCommand(index, i18n("Clean"), DefCleanCmd, QString());
    m_targetsUi->targetsModel.addCommand(index, i18n("Config"), DefConfigCmd, QString());
    m_targetsUi->targetsModel.addCommand(index, i18n("ConfigClean"), DefConfClean, QString());
    buildIndex = m_targetsUi->proxyModel.mapFromSource(buildIndex);
    m_targetsUi->targetsView->setCurrentIndex(buildIndex);
}

/******************************************************************/
void KateBuildView::targetOrSetCopy()
{
    QModelIndex currentIndex = m_targetsUi->targetsView->currentIndex();
    currentIndex = m_targetsUi->proxyModel.mapToSource(currentIndex);
    m_targetsUi->targetFilterEdit->setText(QString());
    QModelIndex newIndex = m_targetsUi->targetsModel.copyTargetOrSet(currentIndex);
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
        addProjectTarget();
        connect(pluginView, SIGNAL(projectMapChanged()), this, SLOT(slotProjectMapChanged()), Qt::UniqueConnection);
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewDeleted(const QString &name, QObject *)
{
    // remove view
    if (name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = nullptr;
        m_targetsUi->targetsModel.deleteTargetSet(i18n("Project Plugin Targets"));
    }
}

/******************************************************************/
void KateBuildView::slotProjectMapChanged()
{
    // only do stuff with valid project
    if (!m_projectPluginView) {
        return;
    }
    m_targetsUi->targetsModel.deleteTargetSet(i18n("Project Plugin Targets"));
    addProjectTarget();
}

/******************************************************************/
void KateBuildView::addProjectTarget()
{
    // only do stuff with valid project
    if (!m_projectPluginView) {
        return;
    }
    // query new project map
    QVariantMap projectMap = m_projectPluginView->property("projectMap").toMap();

    // do we have a valid map for build settings?
    QVariantMap buildMap = projectMap.value(QStringLiteral("build")).toMap();
    if (buildMap.isEmpty()) {
        return;
    }

    // Delete any old project plugin targets
    m_targetsUi->targetsModel.deleteTargetSet(i18n("Project Plugin Targets"));

    // handle build directory as relative to project file, if possible, see bug 413306
    QString projectsBuildDir = buildMap.value(QStringLiteral("directory")).toString();
    const QString projectsBaseDir = m_projectPluginView->property("projectBaseDir").toString();
    if (!projectsBaseDir.isEmpty()) {
        projectsBuildDir = QDir(projectsBaseDir).absoluteFilePath(projectsBuildDir);
    }

    m_projectTargetsetRow = std::min(m_projectTargetsetRow, m_targetsUi->targetsModel.rowCount());
    const QModelIndex set = m_targetsUi->targetsModel.insertTargetSet(m_projectTargetsetRow, i18n("Project Plugin Targets"), projectsBuildDir);
    const QString defaultTarget = buildMap.value(QStringLiteral("default_target")).toString();

    const QVariantList targetsets = buildMap.value(QStringLiteral("targets")).toList();
    for (const QVariant &targetVariant : targetsets) {
        QVariantMap targetMap = targetVariant.toMap();
        QString tgtName = targetMap[QStringLiteral("name")].toString();
        QString buildCmd = targetMap[QStringLiteral("build_cmd")].toString();
        QString runCmd = targetMap[QStringLiteral("run_cmd")].toString();

        if (tgtName.isEmpty() || buildCmd.isEmpty()) {
            continue;
        }
        QPersistentModelIndex idx = m_targetsUi->targetsModel.addCommand(set, tgtName, buildCmd, runCmd);
        if (tgtName == defaultTarget) {
            // A bit of backwards compatibility, move the "default" target to the top
            while (idx.row() > 0) {
                m_targetsUi->targetsModel.moveRowUp(idx);
            }
        }
    }

    if (!set.model()->index(0, 0, set).isValid()) {
        QString buildCmd = buildMap.value(QStringLiteral("build")).toString();
        QString cleanCmd = buildMap.value(QStringLiteral("clean")).toString();
        QString quickCmd = buildMap.value(QStringLiteral("quick")).toString();
        if (!buildCmd.isEmpty()) {
            // we have loaded an "old" project file (<= 4.12)
            m_targetsUi->targetsModel.addCommand(set, i18n("build"), buildCmd, QString());
        }
        if (!cleanCmd.isEmpty()) {
            m_targetsUi->targetsModel.addCommand(set, i18n("clean"), cleanCmd, QString());
        }
        if (!quickCmd.isEmpty()) {
            m_targetsUi->targetsModel.addCommand(set, i18n("quick"), quickCmd, QString());
        }
    }
    const auto index = m_targetsUi->proxyModel.mapFromSource(set);
    if (index.isValid()) {
        m_targetsUi->targetsView->expand(index);
    }
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

#include "plugin_katebuild.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
