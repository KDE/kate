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
#include <QIcon>
#include <QKeyEvent>
#include <QRegularExpressionMatch>
#include <QScrollBar>
#include <QString>
#include <QTimer>

#include <QAction>

#include <KActionCollection>
#include <KTextEditor/Application>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/MarkInterface>
#include <KTextEditor/MovingInterface>

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
static const QString NinjaPrefix = QStringLiteral("[ninja-detection]");

static QIcon messageIcon(KateBuildView::ErrorCategory severity)
{
    // clang-format off
#define RETURN_CACHED_ICON(name, fallbackname) \
    { \
        static QIcon icon(QIcon::fromTheme(QStringLiteral(name), \
                                           QIcon::fromTheme(QStringLiteral(fallbackname)))); \
        return icon; \
    }
    // clang-format on
    switch (severity) {
    case KateBuildView::CategoryError:
        RETURN_CACHED_ICON("data-error", "dialog-error")
    case KateBuildView::CategoryWarning:
        RETURN_CACHED_ICON("data-warning", "dialog-warning")
    default:
        break;
    }
    return QIcon();
}

struct ItemData {
    // ensure destruction, but not inadvertently so by a variant value copy
    QSharedPointer<KTextEditor::MovingCursor> cursor;
};

Q_DECLARE_METATYPE(ItemData)

/******************************************************************/
KateBuildPlugin::KateBuildPlugin(QObject *parent, const VariantList &)
    : KTextEditor::Plugin(parent)
{
    // KF5 FIXME KGlobal::locale()->insertCatalog("katebuild-plugin");
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

    a = actionCollection()->addAction(QStringLiteral("goto_next"));
    a->setText(i18n("Next Error"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    actionCollection()->setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_Right);
    connect(a, &QAction::triggered, this, &KateBuildView::slotNext);

    a = actionCollection()->addAction(QStringLiteral("goto_prev"));
    a->setText(i18n("Previous Error"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    actionCollection()->setDefaultShortcut(a, Qt::SHIFT | Qt::ALT | Qt::Key_Left);
    connect(a, &QAction::triggered, this, &KateBuildView::slotPrev);

    m_showMarks = a = actionCollection()->addAction(QStringLiteral("show_marks"));
    a->setText(i18n("Show Marks"));
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &KateBuildView::slotDisplayOption);

    m_buildWidget = new QWidget(m_toolView);
    m_buildUi.setupUi(m_buildWidget);
    int leftMargin = QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    m_buildUi.u_outpTopLayout->setContentsMargins(leftMargin, 0, 0, 0);
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
    m_buildUi.buildAgainButton2->setVisible(false);
    m_buildUi.cancelBuildButton2->setVisible(false);
    m_buildUi.buildStatusLabel2->setVisible(false);
    m_buildUi.extraLineLayout->setAlignment(Qt::AlignRight);
    m_buildUi.cancelBuildButton->setEnabled(false);
    m_buildUi.cancelBuildButton2->setEnabled(false);

    connect(m_buildUi.errTreeWidget, &QTreeWidget::itemClicked, this, &KateBuildView::slotErrorSelected);

    m_buildUi.plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_buildUi.plainTextEdit->setReadOnly(true);
    slotDisplayMode(FullOutput);

    auto updateEditorColors = [this](KTextEditor::Editor *e) {
        if (!e)
            return;
        auto theme = e->theme();
        auto bg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::BackgroundColor));
        auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::TextStyle::Normal));
        auto sel = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::TextSelection));
        auto pal = m_buildUi.plainTextEdit->palette();
        pal.setColor(QPalette::Base, bg);
        pal.setColor(QPalette::Text, fg);
        pal.setColor(QPalette::Highlight, sel);
        pal.setColor(QPalette::HighlightedText, fg);
        m_buildUi.plainTextEdit->setPalette(pal);
    };
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateEditorColors);

    connect(m_buildUi.displayModeSlider, &QSlider::valueChanged, this, &KateBuildView::slotDisplayMode);

    connect(m_buildUi.buildAgainButton, &QPushButton::clicked, this, &KateBuildView::slotBuildPreviousTarget);
    connect(m_buildUi.cancelBuildButton, &QPushButton::clicked, this, &KateBuildView::slotStop);
    connect(m_buildUi.buildAgainButton2, &QPushButton::clicked, this, &KateBuildView::slotBuildPreviousTarget);
    connect(m_buildUi.cancelBuildButton2, &QPushButton::clicked, this, &KateBuildView::slotStop);

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
    connect(m_win, &KTextEditor::MainWindow::viewChanged, this, &KateBuildView::slotViewChanged);

    m_toolView->installEventFilter(this);

    m_win->guiFactory()->addClient(this);

    // watch for project plugin view creation/deletion
    connect(m_win, &KTextEditor::MainWindow::pluginViewCreated, this, &KateBuildView::slotPluginViewCreated);
    connect(m_win, &KTextEditor::MainWindow::pluginViewDeleted, this, &KateBuildView::slotPluginViewDeleted);

    // Connect signals from project plugin to our slots
    m_projectPluginView = m_win->pluginView(QStringLiteral("kateprojectplugin"));
    slotPluginViewCreated(QStringLiteral("kateprojectplugin"), m_projectPluginView);
}

/******************************************************************/
KateBuildView::~KateBuildView()
{
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.terminate();
        m_proc.waitForFinished();
    }
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

    auto showMarks = cg.readEntry(QStringLiteral("Show Marks"), false);
    m_showMarks->setChecked(showMarks);

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
    cg.writeEntry(QStringLiteral("Show Marks"), m_showMarks->isChecked());
}

/******************************************************************/
void KateBuildView::slotNext()
{
    const int itemCount = m_buildUi.errTreeWidget->topLevelItemCount();
    if (itemCount == 0) {
        return;
    }

    QTreeWidgetItem *item = m_buildUi.errTreeWidget->currentItem();
    if (item && item->isHidden()) {
        item = nullptr;
    }

    int i = (item == nullptr) ? -1 : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (++i < itemCount) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        // Search item which fit view settings and has desired data
        if (!item->text(1).isEmpty() && !item->isHidden() && item->data(1, Qt::UserRole).toInt()) {
            m_buildUi.errTreeWidget->setCurrentItem(item);
            m_buildUi.errTreeWidget->scrollToItem(item);
            slotErrorSelected(item);
            return;
        }
    }
}

/******************************************************************/
void KateBuildView::slotPrev()
{
    const int itemCount = m_buildUi.errTreeWidget->topLevelItemCount();
    if (itemCount == 0) {
        return;
    }

    QTreeWidgetItem *item = m_buildUi.errTreeWidget->currentItem();
    if (item && item->isHidden()) {
        item = nullptr;
    }

    int i = (item == nullptr) ? itemCount : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (--i >= 0) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        // Search item which fit view settings and has desired data
        if (!item->text(1).isEmpty() && !item->isHidden() && item->data(1, Qt::UserRole).toInt()) {
            m_buildUi.errTreeWidget->setCurrentItem(item);
            m_buildUi.errTreeWidget->scrollToItem(item);
            slotErrorSelected(item);
            return;
        }
    }
}

#ifdef Q_OS_WIN
/******************************************************************/
QString KateBuildView::caseFixed(const QString &path)
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
#endif

void KateBuildView::slotErrorSelected(QTreeWidgetItem *item)
{
    // any view active?
    if (!m_win->activeView()) {
        return;
    }

    // Avoid garish highlighting of the selected line
    m_win->activeView()->setFocus();

    // Search the item where the data we need is stored
    while (!item->data(1, Qt::UserRole).toInt()) {
        item = m_buildUi.errTreeWidget->itemAbove(item);
        if (!item) {
            return;
        }
    }

    // get stuff
    QString filename = item->data(0, Qt::UserRole).toString();
    if (filename.isEmpty()) {
        return;
    }

    int line = item->data(1, Qt::UserRole).toInt();
    int column = item->data(2, Qt::UserRole).toInt();
    // check with moving cursor
    auto data = item->data(0, DataRole).value<ItemData>();
    if (data.cursor) {
        line = data.cursor->line();
        column = data.cursor->column();
    }

#ifdef Q_OS_WIN
    filename = caseFixed(filename);
#endif

    // Check if the file exists
    if (!QFileInfo::exists(filename)) {
        displayMessage(xi18nc("@info",
                              "<title>Could not open file:</title><nl/>%1<br/>Try adding a search path to the working directory in the Target Settings",
                              filename),
                       KTextEditor::Message::Error);
        return;
    }

    // open file (if needed, otherwise, this will activate only the right view...)
    m_win->openUrl(QUrl::fromLocalFile(filename));

    // do it ;)
    m_win->activeView()->setCursorPosition(KTextEditor::Cursor(line - 1, column - 1));
}

/******************************************************************/
void KateBuildView::addError(const QString &filename, const QString &line, const QString &column, const QString &message)
{
    ErrorCategory errorCategory = CategoryInfo;
    QTreeWidgetItem *item = new QTreeWidgetItem(m_buildUi.errTreeWidget);
    item->setBackground(1, Qt::gray);
    static QRegularExpression errorRegExp(QStringLiteral("\berror\b"));
    static QRegularExpression errorRegExpTr(QStringLiteral("\b%1\b").arg(i18nc("The same word as 'make' uses to mark an error.", "error")));
    // The strings are twice in case kate is translated but not make.
    if (message.contains(errorRegExp) || message.contains(errorRegExpTr) || message.contains(QLatin1String("undefined reference"))
        || message.contains(i18nc("The same word as 'ld' uses to mark an ...", "undefined reference"))) {
        errorCategory = CategoryError;
        item->setForeground(1, Qt::red);
        m_numErrors++;
        item->setHidden(false);
    }
    static QRegularExpression warningRegExp(QStringLiteral("\bwarning\b"));
    static QRegularExpression warningRegExpTr(QStringLiteral("\b%1\b").arg(i18nc("The same word as 'make' uses to mark a warning.", "warning")));
    if (message.contains(warningRegExp) || message.contains(warningRegExpTr)) {
        errorCategory = CategoryWarning;
        item->setForeground(1, Qt::yellow);
        m_numWarnings++;
        item->setHidden(m_buildUi.displayModeSlider->value() > 2);
    }
    item->setTextAlignment(1, Qt::AlignRight);

    // visible text
    // remove path from visible file name
    QFileInfo file(filename);

    item->setText(0, file.fileName());
    item->setText(1, line);
    item->setText(2, message);

    // used to read from when activating an item
    item->setData(0, Qt::UserRole, filename);
    item->setData(1, Qt::UserRole, line);
    item->setData(2, Qt::UserRole, column);

    if (errorCategory == CategoryInfo) {
        item->setHidden(m_buildUi.displayModeSlider->value() > 1);
    }

    item->setData(0, ErrorRole, errorCategory);

    // add tooltips in all columns
    // The enclosing <qt>...</qt> enables word-wrap for long error messages
    item->setData(0, Qt::ToolTipRole, filename);
    item->setData(1, Qt::ToolTipRole, QStringLiteral("<qt>%1</qt>").arg(message));
    item->setData(2, Qt::ToolTipRole, QStringLiteral("<qt>%1</qt>").arg(message));
}

void KateBuildView::clearMarks()
{
    for (auto &doc : m_markedDocs) {
        if (!doc) {
            continue;
        }

        KTextEditor::MarkInterface *iface = qobject_cast<KTextEditor::MarkInterface *>(doc);
        if (iface) {
            const QHash<int, KTextEditor::Mark *> marks = iface->marks();
            QHashIterator<int, KTextEditor::Mark *> i(marks);
            while (i.hasNext()) {
                i.next();
                auto markType = KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning;
                if (i.value()->type & markType) {
                    iface->removeMark(i.value()->line, markType);
                }
            }
        }
    }

    m_markedDocs.clear();
}

void KateBuildView::addMarks(KTextEditor::Document *doc, bool mark)
{
    KTextEditor::MarkInterfaceV2 *iface = qobject_cast<KTextEditor::MarkInterfaceV2 *>(doc);
    KTextEditor::MovingInterface *miface = qobject_cast<KTextEditor::MovingInterface *>(doc);
    if (!iface || m_markedDocs.contains(doc)) {
        return;
    }

    QTreeWidgetItemIterator it(m_buildUi.errTreeWidget, QTreeWidgetItemIterator::All);
    while (*it) {
        QTreeWidgetItem *item = *it;
        ++it;

        auto filename = item->data(0, Qt::UserRole).toString();
        auto url = QUrl::fromLocalFile(filename);
        if (url != doc->url()) {
            continue;
        }

        auto line = item->data(1, Qt::UserRole).toInt();
        if (mark) {
            ErrorCategory category = static_cast<ErrorCategory>(item->data(0, ErrorRole).toInt());
            KTextEditor::MarkInterface::MarkTypes markType{};

            switch (category) {
            case CategoryError: {
                markType = KTextEditor::MarkInterface::Error;
                iface->setMarkDescription(markType, i18n("Error"));
                break;
            }
            case CategoryWarning: {
                markType = KTextEditor::MarkInterface::Warning;
                iface->setMarkDescription(markType, i18n("Warning"));
                break;
            }
            default:
                break;
            }

            if (markType) {
                iface->setMarkIcon(markType, messageIcon(category));
                iface->addMark(line - 1, markType);
            }
            m_markedDocs.insert(doc, doc);
        }

        // add moving cursor so link between message and location
        // is not broken by document changes
        if (miface) {
            auto data = item->data(0, DataRole).value<ItemData>();
            if (!data.cursor) {
                auto column = item->data(2, Qt::UserRole).toInt();
                data.cursor.reset(miface->newMovingCursor({line, column}));
                QVariant var;
                var.setValue(data);
                item->setData(0, DataRole, var);
            }
        }
    }

    // ensure cleanup
    // clang-format off
    if (miface) {
        auto conn = connect(doc,
                            SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
                            this,
                            SLOT(slotInvalidateMoving(KTextEditor::Document*)),
                            Qt::UniqueConnection);
        conn = connect(doc,
                       SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)),
                       this,
                       SLOT(slotInvalidateMoving(KTextEditor::Document*)),
                       Qt::UniqueConnection);
    }

    connect(doc,
            SIGNAL(markClicked(KTextEditor::Document*,KTextEditor::Mark,bool&)),
            this,
            SLOT(slotMarkClicked(KTextEditor::Document*,KTextEditor::Mark,bool&)),
            Qt::UniqueConnection);
    // clang-format on
}

void KateBuildView::slotInvalidateMoving(KTextEditor::Document *doc)
{
    QTreeWidgetItemIterator it(m_buildUi.errTreeWidget, QTreeWidgetItemIterator::All);
    while (*it) {
        QTreeWidgetItem *item = *it;
        ++it;

        auto data = item->data(0, DataRole).value<ItemData>();
        if (data.cursor && data.cursor->document() == doc) {
            item->setData(0, DataRole, 0);
        }
    }
}

void KateBuildView::slotMarkClicked(KTextEditor::Document *doc, KTextEditor::Mark mark, bool &handled)
{
    auto tree = m_buildUi.errTreeWidget;
    QTreeWidgetItemIterator it(tree, QTreeWidgetItemIterator::All);
    while (*it) {
        QTreeWidgetItem *item = *it;
        ++it;

        auto filename = item->data(0, Qt::UserRole).toString();
        auto line = item->data(1, Qt::UserRole).toInt();
        // prefer moving cursor's opinion if so available
        auto data = item->data(0, DataRole).value<ItemData>();
        if (data.cursor) {
            line = data.cursor->line();
        }
        if (line - 1 == mark.line && QUrl::fromLocalFile(filename) == doc->url()) {
            tree->blockSignals(true);
            tree->setCurrentItem(item);
            tree->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            tree->blockSignals(false);
            handled = true;
            break;
        }
    }
}

void KateBuildView::slotViewChanged()
{
    KTextEditor::View *activeView = m_win->activeView();
    auto doc = activeView ? activeView->document() : nullptr;

    if (doc) {
        addMarks(doc, m_showMarks->isChecked());
    }
}

void KateBuildView::slotDisplayOption()
{
    if (m_showMarks) {
        if (!m_showMarks->isChecked()) {
            clearMarks();
        } else {
            slotViewChanged();
        }
    }
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
    clearMarks();
    m_buildUi.plainTextEdit->clear();
    m_buildUi.errTreeWidget->clear();
    m_stdOut.clear();
    m_stdErr.clear();
    m_numErrors = 0;
    m_numWarnings = 0;
    m_make_dir_stack.clear();
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
    m_displayModeBeforeBuild = m_buildUi.displayModeSlider->value();
    m_buildUi.displayModeSlider->setValue(0);
    m_win->showToolView(m_toolView);

    QFont font = Utils::editorFont();
    m_buildUi.errTreeWidget->setFont(font);
    m_buildUi.plainTextEdit->setFont(font);

    // set working directory
    m_make_dir = dir;
    m_make_dir_stack.push(m_make_dir);

    if (!QFile::exists(m_make_dir)) {
        KMessageBox::error(nullptr, i18n("Cannot run command: %1\nWork path does not exist: %2", command, m_make_dir));
        return false;
    }

    // ninja build tool sends all output to stdout,
    // so follow https://github.com/ninja-build/ninja/issues/1537 to separate ninja and compiler output
    auto env = QProcessEnvironment::systemEnvironment();
    const auto nstatus = QStringLiteral("NINJA_STATUS");
    auto curr = env.value(nstatus, QStringLiteral("[%f/%t] "));
    // add marker to search on later on
    env.insert(nstatus, NinjaPrefix + curr);
    m_ninjaBuildDetected = false;

    m_proc.setProcessEnvironment(env);
    m_proc.setWorkingDirectory(m_make_dir);
    m_proc.setShellCommand(command);
    startHostProcess(m_proc);

    if (!m_proc.waitForStarted(500)) {
        KMessageBox::error(nullptr, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc.exitStatus()));
        return false;
    }

    m_buildUi.cancelBuildButton->setEnabled(true);
    m_buildUi.cancelBuildButton2->setEnabled(true);
    m_buildUi.buildAgainButton->setEnabled(false);
    m_buildUi.buildAgainButton2->setEnabled(false);

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
        m_buildUi.buildStatusLabel2->setText(msg);
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
    if (selected.isValid()) {
        m_targetsUi->targetsView->setCurrentIndex(selected);
    }
    m_targetsUi->targetsView->scrollTo(selected);
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
    m_buildUi.buildStatusLabel2->setText(msg);
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
    m_buildUi.cancelBuildButton2->setEnabled(false);
    m_buildUi.buildAgainButton->setEnabled(true);
    m_buildUi.buildAgainButton2->setEnabled(true);

    QString buildStatus = i18n("Building <b>%1</b> completed.", m_currentlyBuildingTarget);

    // did we get any errors?
    if (m_numErrors || m_numWarnings || (exitCode != 0)) {
        m_buildUi.u_tabWidget->setCurrentIndex(1);
        if (m_buildUi.displayModeSlider->value() == 0) {
            m_buildUi.displayModeSlider->setValue(m_displayModeBeforeBuild > 0 ? m_displayModeBeforeBuild : 1);
        }
        m_buildUi.errTreeWidget->resizeColumnToContents(0);
        m_buildUi.errTreeWidget->resizeColumnToContents(1);
        m_buildUi.errTreeWidget->resizeColumnToContents(2);
        m_buildUi.errTreeWidget->horizontalScrollBar()->setValue(0);
        m_win->showToolView(m_toolView);
    }

    bool buildSuccess = true;
    if (m_numErrors || m_numWarnings) {
        QStringList msgs;
        if (m_numErrors) {
            msgs << i18np("Found one error.", "Found %1 errors.", m_numErrors);
            buildStatus = i18n("Building <b>%1</b> had errors.", m_currentlyBuildingTarget);
        } else if (m_numWarnings) {
            msgs << i18np("Found one warning.", "Found %1 warnings.", m_numWarnings);
            buildStatus = i18n("Building <b>%1</b> had warnings.", m_currentlyBuildingTarget);
        }
        displayBuildResult(msgs.join(QLatin1Char('\n')), m_numErrors ? KTextEditor::Message::Error : KTextEditor::Message::Warning);
    } else if (exitCode != 0) {
        buildSuccess = false;
        m_runAfterBuild = false;
        displayBuildResult(i18n("Build failed."), KTextEditor::Message::Warning);
    } else {
        displayBuildResult(i18n("Build completed without problems."), KTextEditor::Message::Positive);
    }

    if (!m_buildCancelled) {
        m_buildUi.buildStatusLabel->setText(buildStatus);
        m_buildUi.buildStatusLabel2->setText(buildStatus);
        m_buildCancelled = false;
        // add marks
        slotViewChanged();
    }

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

        connect(out, &AppOutput::runningChanhged, this, [this]() {
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

static void appendPlainTextTo(QPlainTextEdit *edit, const QString &text)
{
    // NOTE We can't use edit->appendPlainText() as that adds a new paragraph
    // and we might need to add text that finishes the previous line.

    // Get the scroll position to restore it if not at the end
    int scrollValue = edit->verticalScrollBar()->value();
    int scrollMax = edit->verticalScrollBar()->maximum();
    // Save the selection and restore it after adding the text
    QTextCursor cursor = edit->textCursor();

    // Insert the new text
    edit->moveCursor(QTextCursor::End);
    edit->insertPlainText(text);

    // Restore selection and scroll position
    edit->setTextCursor(cursor);
    if (scrollValue == scrollMax) {
        edit->verticalScrollBar()->setValue(edit->verticalScrollBar()->maximum());
    } else {
        edit->verticalScrollBar()->setValue(scrollValue);
    }
}

/******************************************************************/
void KateBuildView::slotReadReadyStdOut()
{
    // read data from procs stdout and add
    // the text to the end of the output
    // FIXME This works for utf8 but not for all charsets
    QString l = QString::fromUtf8(m_proc.readAllStandardOutput());

    l.remove(QLatin1Char('\r'));
    m_stdOut += l;

    // Remove the Ninja workaround string
    l.remove(NinjaPrefix);
    appendPlainTextTo(m_buildUi.plainTextEdit, l);

    // handle one line at a time
    do {
        const int end = m_stdOut.indexOf(QLatin1Char('\n'));
        if (end < 0) {
            break;
        }

        QStringView line = QStringView(m_stdOut).mid(0, end);
        const bool ninjaOutput = line.startsWith(NinjaPrefix);
        m_ninjaBuildDetected |= ninjaOutput;
        if (ninjaOutput) {
            line = line.mid(NinjaPrefix.length());
        }
        // qDebug() << line;

        QRegularExpressionMatch match = m_newDirDetector.match(line);

        if (match.hasMatch()) {
            // qDebug() << "Enter/Exit dir found";
            QString newDir = match.captured(1);
            // qDebug () << "New dir = " << newDir;

            if ((m_make_dir_stack.size() > 1) && (m_make_dir_stack.top() == newDir)) {
                m_make_dir_stack.pop();
                newDir = m_make_dir_stack.top();
            } else {
                m_make_dir_stack.push(newDir);
            }

            m_make_dir = newDir;
        } else if (m_ninjaBuildDetected && !ninjaOutput) {
            processLine(line);
        }

        m_stdOut.remove(0, end + 1);
    } while (1);
}

/******************************************************************/
void KateBuildView::slotReadReadyStdErr()
{
    // FIXME This works for utf8 but not for all charsets
    QString l = QString::fromUtf8(m_proc.readAllStandardError());
    l.remove(QLatin1Char('\r'));
    appendPlainTextTo(m_buildUi.plainTextEdit, l);
    m_stdErr += l;

    do {
        const int end = m_stdErr.indexOf(QLatin1Char('\n'));
        if (end < 0) {
            break;
        }
        const QString line = m_stdErr.mid(0, end);
        processLine(line);

        m_stdErr.remove(0, end + 1);
    } while (1);
}

/******************************************************************/
void KateBuildView::processLine(QStringView line)
{
    // qDebug() << line ;

    // look for a filename
    QRegularExpressionMatch match = m_filenameDetector.match(line);

    if (!match.hasMatch()) {
        addError(QString(), QStringLiteral("0"), QString(), line.toString());
        // kDebug() << "A filename was not found in the line ";
        return;
    }

    QString filename = match.captured(1);
    const QString line_n = match.captured(2);
    const QString col_n = match.captured(3);
    const QString msg = match.captured(4);

#ifdef Q_OS_WIN
    // convert '\' to '/' so the concatenation works
    filename = QFileInfo(filename).filePath();
#endif

    // qDebug() << "File Name:"<<m_make_dir << filename << " msg:"<< msg;
    // add path to file
    if (QFile::exists(m_make_dir + QLatin1Char('/') + filename)) {
        filename = m_make_dir + QLatin1Char('/') + filename;
    }

    // If we still do not have a file name try the extra search paths
    int i = 1;
    while (!QFile::exists(filename) && i < m_searchPaths.size()) {
        if (QFile::exists(m_searchPaths[i] + QLatin1Char('/') + filename)) {
            filename = m_searchPaths[i] + QLatin1Char('/') + filename;
        }
        i++;
    }

    // get canonical path, if possible, to avoid duplicated opened files
    auto canonicalFilePath(QFileInfo(filename).canonicalFilePath());
    if (!canonicalFilePath.isEmpty()) {
        filename = canonicalFilePath;
    }

    // Now we have the data we need show the error/warning
    addError(filename, line_n, col_n, msg);
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
void KateBuildView::slotDisplayMode(int mode)
{
    QTreeWidget *tree = m_buildUi.errTreeWidget;
    tree->setVisible(mode != 0);
    m_buildUi.plainTextEdit->setVisible(mode == 0);

    QString modeText;
    switch (mode) {
    case OnlyErrors:
        modeText = i18n("Only Errors");
        break;
    case ErrorsAndWarnings:
        modeText = i18n("Errors and Warnings");
        break;
    case ParsedOutput:
        modeText = i18n("Parsed Output");
        break;
    case FullOutput:
        modeText = i18n("Full Output");
        break;
    }
    m_buildUi.displayModeLabel->setText(modeText);

    if (mode < 1) {
        return;
    }

    const int itemCount = tree->topLevelItemCount();

    for (int i = 0; i < itemCount; i++) {
        QTreeWidgetItem *item = tree->topLevelItem(i);

        const ErrorCategory errorCategory = static_cast<ErrorCategory>(item->data(0, ErrorRole).toInt());

        switch (errorCategory) {
        case CategoryInfo:
            item->setHidden(mode > 1);
            break;
        case CategoryWarning:
            item->setHidden(mode > 2);
            break;
        case CategoryError:
            item->setHidden(false);
            break;
        }
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
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if ((obj == m_toolView) && (ke->key() == Qt::Key_Escape)) {
            m_win->hideToolView(m_toolView);
            event->accept();
            return true;
        }
    }
    if ((event->type() == QEvent::Resize) && (obj == m_buildWidget)) {
        if (m_buildUi.u_tabWidget->currentIndex() == 1) {
            if ((m_outputWidgetWidth == 0) && m_buildUi.buildAgainButton->isVisible()) {
                QSize msh = m_buildWidget->minimumSizeHint();
                m_outputWidgetWidth = msh.width();
            }
        }
        bool useVertLayout = (m_buildWidget->width() < m_outputWidgetWidth);
        m_buildUi.buildAgainButton->setVisible(!useVertLayout);
        m_buildUi.cancelBuildButton->setVisible(!useVertLayout);
        m_buildUi.buildStatusLabel->setVisible(!useVertLayout);
        m_buildUi.buildAgainButton2->setVisible(useVertLayout);
        m_buildUi.cancelBuildButton2->setVisible(useVertLayout);
        m_buildUi.buildStatusLabel2->setVisible(useVertLayout);
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
