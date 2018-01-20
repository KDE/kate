/* plugin_katebuild.c                    Kate Plugin
**
** Copyright (C) 2013 by Alexander Neundorf <neundorf@kde.org>
** Copyright (C) 2006-2015 by Kåre Särs <kare.sars@iki.fi>
** Copyright (C) 2011 by Ian Wakeling <ian.wakeling@ntlworld.com>
**
** This code is mostly a modification of the GPL'ed Make plugin
** by Adriaan de Groot.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <cassert>

#include <QRegularExpressionMatch>
#include <QString>
#include <QScrollBar>
#include <QCompleter>
#include <QDirModel>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>

#include <QAction>

#include <KTextEditor/Application>
#include <KXMLGUIFactory>
#include <KActionCollection>

#include <kmessagebox.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>


#include "SelectTargetView.h"

K_PLUGIN_FACTORY_WITH_JSON (KateBuildPluginFactory, "katebuildplugin.json", registerPlugin<KateBuildPlugin>();)

static const QString DefConfigCmd = QStringLiteral("cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ../");
static const QString DefConfClean;
static const QString DefTargetName = QStringLiteral("all");
static const QString DefBuildCmd = QStringLiteral("make");
static const QString DefCleanCmd = QStringLiteral("make clean");


/******************************************************************/
KateBuildPlugin::KateBuildPlugin(QObject *parent, const VariantList&):
KTextEditor::Plugin(parent)
{
    // KF5 FIXME KGlobal::locale()->insertCatalog("katebuild-plugin");
}

/******************************************************************/
QObject *KateBuildPlugin::createView (KTextEditor::MainWindow *mainWindow)
{
    return new KateBuildView(this, mainWindow);
}

/******************************************************************/
KateBuildView::KateBuildView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw)
    : QObject (mw)
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
    , m_filenameDetector(QStringLiteral("(([a-np-zA-Z]:[\\\\/])?[a-zA-Z0-9_\\.\\-/\\\\]+\\.[a-zA-Z0-9]+):([0-9]+)(.*)"))
    // e.g. from icpc: "main.cpp(14): error: no suitable conversion function from "std::string" to "int" exists"
    , m_filenameDetectorIcpc(QStringLiteral("(([a-np-zA-Z]:[\\\\/])?[a-zA-Z0-9_\\.\\-/\\\\]+\\.[a-zA-Z0-9]+)\\(([0-9]+)\\)(:.*)"))
    , m_filenameDetectorGccWorked(false)
    , m_newDirDetector(QStringLiteral("make\\[.+\\]: .+ `.*'"))
{
    KXMLGUIClient::setComponentName (QLatin1String("katebuild"), i18n ("Kate Build Plugin"));
    setXMLFile(QLatin1String("ui.rc"));

    m_toolView  = mw->createToolView(plugin, QStringLiteral("kate_plugin_katebuildplugin"),
                                      KTextEditor::MainWindow::Bottom,
                                      QIcon::fromTheme(QStringLiteral("application-x-ms-dos-executable")),
                                      i18n("Build Output"));

    QAction *a = actionCollection()->addAction(QStringLiteral("select_target"));
    a->setText(i18n("Select Target..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("select")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotSelectTarget);
    a = actionCollection()->addAction(QStringLiteral("build_default_target"));
    a->setText(i18n("Build Default Target"));
    connect(a, &QAction::triggered, this, &KateBuildView::slotBuildDefaultTarget);

    a = actionCollection()->addAction(QStringLiteral("build_previous_target"));
    a->setText(i18n("Build Previous Target"));
    connect(a, &QAction::triggered, this, &KateBuildView::slotBuildPreviousTarget);

    a = actionCollection()->addAction(QStringLiteral("stop"));
    a->setText(i18n("Stop"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    connect(a, &QAction::triggered, this, &KateBuildView::slotStop);

    a = actionCollection()->addAction(QStringLiteral("goto_next"));
    a->setText(i18n("Next Error"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    actionCollection()->setDefaultShortcut(a, Qt::SHIFT+Qt::ALT+Qt::Key_Right);
    connect(a, &QAction::triggered, this, &KateBuildView::slotNext);

    a = actionCollection()->addAction(QStringLiteral("goto_prev"));
    a->setText(i18n("Previous Error"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    actionCollection()->setDefaultShortcut(a, Qt::SHIFT+Qt::ALT+Qt::Key_Left);
    connect(a, &QAction::triggered, this, &KateBuildView::slotPrev);


    m_buildWidget = new QWidget(m_toolView);
    m_buildUi.setupUi(m_buildWidget);
    m_targetsUi = new TargetsUi(this, m_buildUi.u_tabWidget);
    m_buildUi.u_tabWidget->insertTab(0, m_targetsUi, i18nc("Tab label", "Target Settings"));
    m_buildUi.u_tabWidget->setCurrentWidget(m_targetsUi);

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

    connect(m_buildUi.errTreeWidget, &QTreeWidget::itemClicked,
            this, &KateBuildView::slotErrorSelected);

    m_buildUi.plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_buildUi.plainTextEdit->setReadOnly(true);
    slotDisplayMode(FullOutput);

    connect(m_buildUi.displayModeSlider, &QSlider::valueChanged, this, &KateBuildView::slotDisplayMode);

    connect(m_buildUi.buildAgainButton, &QPushButton::clicked, this, &KateBuildView::slotBuildPreviousTarget);
    connect(m_buildUi.cancelBuildButton, &QPushButton::clicked, this, &KateBuildView::slotStop);
    connect(m_buildUi.buildAgainButton2, &QPushButton::clicked, this, &KateBuildView::slotBuildPreviousTarget);
    connect(m_buildUi.cancelBuildButton2, &QPushButton::clicked, this, &KateBuildView::slotStop);

    connect(m_targetsUi->newTarget, &QToolButton::clicked, this, &KateBuildView::targetSetNew);
    connect(m_targetsUi->copyTarget, &QToolButton::clicked, this, &KateBuildView::targetOrSetCopy);
    connect(m_targetsUi->deleteTarget, &QToolButton::clicked, this, &KateBuildView::targetDelete);

    connect(m_targetsUi->addButton, &QToolButton::clicked, this, &KateBuildView::slotAddTargetClicked);
    connect(m_targetsUi->buildButton, &QToolButton::clicked, this, &KateBuildView::slotBuildActiveTarget);
    connect(m_targetsUi, &TargetsUi::enterPressed, this, &KateBuildView::slotBuildActiveTarget);

    m_proc.setOutputChannelMode(KProcess::SeparateChannels);
    connect(&m_proc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateBuildView::slotProcExited);
    connect(&m_proc, &KProcess::readyReadStandardError, this, &KateBuildView::slotReadReadyStdErr);
    connect(&m_proc, &KProcess::readyReadStandardOutput, this, &KateBuildView::slotReadReadyStdOut);

    connect(m_win, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &KateBuildView::handleEsc);

    m_toolView->installEventFilter(this);

    m_win->guiFactory()->addClient(this);

    // watch for project plugin view creation/deletion
    connect(m_win, &KTextEditor::MainWindow::pluginViewCreated,
            this, &KateBuildView::slotPluginViewCreated);

    connect(m_win, &KTextEditor::MainWindow::pluginViewDeleted,
            this, &KateBuildView::slotPluginViewDeleted);
    // update once project plugin state manually
    m_projectPluginView = m_win->pluginView (QStringLiteral("kateprojectplugin"));
    slotProjectMapChanged ();
}


/******************************************************************/
KateBuildView::~KateBuildView()
{
    m_win->guiFactory()->removeClient( this );
    delete m_toolView;
}

/******************************************************************/
void KateBuildView::readSessionConfig(const KConfigGroup& cg)
{
    int numTargets = cg.readEntry(QStringLiteral("NumTargets"), 0);
    m_targetsUi->targetsModel.clear();
    int tmpIndex;
    int tmpCmd;

    if (numTargets == 0 ) {
        // either the config is empty or uses the older format
        m_targetsUi->targetsModel.addTargetSet(i18n("Target Set"), QString());
        m_targetsUi->targetsModel.addCommand(0, i18n("build"), cg.readEntry(QStringLiteral("Make Command"), DefBuildCmd));
        m_targetsUi->targetsModel.addCommand(0, i18n("clean"), cg.readEntry(QStringLiteral("Clean Command"), DefCleanCmd));
        m_targetsUi->targetsModel.addCommand(0, i18n("config"), DefConfigCmd);

        QString quickCmd = cg.readEntry(QStringLiteral("Quick Compile Command"));
        if (!quickCmd.isEmpty()) {
            m_targetsUi->targetsModel.addCommand(0, i18n("quick"), quickCmd);
        }
        tmpIndex = 0;
        tmpCmd = 0;
    }
    else {
        for (int i=0; i<numTargets; i++) {
            QStringList targetNames = cg.readEntry(QStringLiteral("%1 Target Names").arg(i), QStringList());
            QString targetSetName = cg.readEntry(QStringLiteral("%1 Target").arg(i), QString());
            QString buildDir = cg.readEntry(QStringLiteral("%1 BuildPath").arg(i), QString());

            m_targetsUi->targetsModel.addTargetSet(targetSetName, buildDir);

            if (targetNames.isEmpty()) {
                QString quickCmd = cg.readEntry(QStringLiteral("%1 QuickCmd").arg(i));
                m_targetsUi->targetsModel.addCommand(i, i18n("build"), cg.readEntry(QStringLiteral("%1 BuildCmd"), DefBuildCmd));
                m_targetsUi->targetsModel.addCommand(i, i18n("clean"), cg.readEntry(QStringLiteral("%1 CleanCmd"), DefCleanCmd));
                if (!quickCmd.isEmpty()) {
                    m_targetsUi->targetsModel.addCommand(i, i18n("quick"), quickCmd);
                }
                m_targetsUi->targetsModel.setDefaultCmd(i, i18n("build"));
            }
            else {
                for (int tn=0; tn<targetNames.size(); ++tn) {
                    const QString targetName = targetNames.at(tn);
                    m_targetsUi->targetsModel.addCommand(i, targetName, cg.readEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(targetName), DefBuildCmd));
                }
                QString defCmd = cg.readEntry(QStringLiteral("%1 Target Default").arg(i), QString());
                m_targetsUi->targetsModel.setDefaultCmd(i, defCmd);
            }

        }
        tmpIndex = cg.readEntry(QStringLiteral("Active Target Index"), 0);
        tmpCmd = cg.readEntry(QStringLiteral("Active Target Command"), 0);
    }

    m_targetsUi->targetsView->expandAll();
    m_targetsUi->targetsView->resizeColumnToContents(0);
    m_targetsUi->targetsView->collapseAll();

    QModelIndex root = m_targetsUi->targetsModel.index(tmpIndex);
    QModelIndex cmdIndex = m_targetsUi->targetsModel.index(tmpCmd, 0, root);
    m_targetsUi->targetsView->setCurrentIndex(cmdIndex);
}

/******************************************************************/
void KateBuildView::writeSessionConfig(KConfigGroup& cg)
{
    m_targetsUi->targetsModel.deleteTargetSet(i18n("Project Plugin Targets"));
    QList<TargetModel::TargetSet> targets = m_targetsUi->targetsModel.targetSets();

    cg.writeEntry("NumTargets", targets.size());

    for (int i=0; i<targets.size(); i++) {
        cg.writeEntry(QStringLiteral("%1 Target").arg(i), targets[i].name);
        cg.writeEntry(QStringLiteral("%1 BuildPath").arg(i), targets[i].workDir);
        QStringList cmdNames;

        for (int j=0; j<targets[i].commands.count(); j++) {
            const QString& cmdName = targets[i].commands[j].first;
            const QString& buildCmd = targets[i].commands[j].second;
            cmdNames << cmdName;
            cg.writeEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(cmdName), buildCmd);
        }
        cg.writeEntry(QStringLiteral("%1 Target Names").arg(i), cmdNames);
        cg.writeEntry(QStringLiteral("%1 Target Default").arg(i), targets[i].defaultCmd);
    }
    int setRow = 0;
    int set = 0;
    QModelIndex ind = m_targetsUi->targetsView->currentIndex();
    if (ind.internalId() == TargetModel::InvalidIndex) {
        set = ind.row();
    }
    else {
        set = ind.internalId();
        setRow = ind.row();
    }
    if (setRow < 0) setRow = 0;

    cg.writeEntry(QStringLiteral("Active Target Index"), set);
    cg.writeEntry(QStringLiteral("Active Target Command"), setRow);
    slotAddProjectTarget();
}


/******************************************************************/
void KateBuildView::slotNext()
{
    const int itemCount = m_buildUi.errTreeWidget->topLevelItemCount();
    if (itemCount == 0) {
        return;
    }

    QTreeWidgetItem *item = m_buildUi.errTreeWidget->currentItem();
    if (item && item->isHidden()) item = nullptr;

    int i = (item == nullptr) ? -1 : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (++i < itemCount) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty() && !item->isHidden()) {
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
    if (item && item->isHidden()) item = nullptr;

    int i = (item == nullptr) ? itemCount : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (--i >= 0) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty() && !item->isHidden()) {
            m_buildUi.errTreeWidget->setCurrentItem(item);
            m_buildUi.errTreeWidget->scrollToItem(item);
            slotErrorSelected(item);
            return;
        }
    }
}

/******************************************************************/
void KateBuildView::slotErrorSelected(QTreeWidgetItem *item)
{
    // get stuff
    const QString filename = item->data(0, Qt::UserRole).toString();
    if (filename.isEmpty()) return;
    const int line = item->data(1, Qt::UserRole).toInt();
    const int column = item->data(2, Qt::UserRole).toInt();

    // open file (if needed, otherwise, this will activate only the right view...)
    m_win->openUrl(QUrl::fromLocalFile(filename));

    // any view active?
    if (!m_win->activeView()) {
        return;
    }

    // do it ;)
    m_win->activeView()->setCursorPosition(KTextEditor::Cursor(line-1, column-1));
    m_win->activeView()->setFocus();
}


/******************************************************************/
void KateBuildView::addError(const QString &filename, const QString &line,
                             const QString &column, const QString &message)
{
    ErrorCategory errorCategory = CategoryInfo;
    QTreeWidgetItem* item = new QTreeWidgetItem(m_buildUi.errTreeWidget);
    item->setBackground(1, Qt::gray);
    // The strings are twice in case kate is translated but not make.
    if (message.contains(QStringLiteral("error")) ||
        message.contains(i18nc("The same word as 'make' uses to mark an error.","error")) ||
        message.contains(QStringLiteral("undefined reference")) ||
        message.contains(i18nc("The same word as 'ld' uses to mark an ...","undefined reference"))
       )
    {
        errorCategory = CategoryError;
        item->setForeground(1, Qt::red);
        m_numErrors++;
        item->setHidden(false);
    }
    if (message.contains(QStringLiteral("warning")) ||
        message.contains(i18nc("The same word as 'make' uses to mark a warning.","warning"))
       )
    {
        errorCategory = CategoryWarning;
        item->setForeground(1, Qt::yellow);
        m_numWarnings++;
        item->setHidden(m_buildUi.displayModeSlider->value() > 2);
    }
    item->setTextAlignment(1, Qt::AlignRight);

    // visible text
    //remove path from visible file name
    QFileInfo file(filename);
    item->setText(0, file.fileName());
    item->setText(1, line);
    item->setText(2, message.trimmed());

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

/******************************************************************/
QUrl KateBuildView::docUrl()
{
    KTextEditor::View *kv = m_win->activeView();
    if (!kv) {
        qDebug() << "no KTextEditor::View" << endl;
        return QUrl();
    }

    if (kv->document()->isModified()) kv->document()->save();
    return kv->document()->url();
}

/******************************************************************/
bool KateBuildView::checkLocal(const QUrl &dir)
{
    if (dir.path().isEmpty()) {
        KMessageBox::sorry(nullptr, i18n("There is no file or directory specified for building."));
        return false;
    }
    else if (!dir.isLocalFile()) {
        KMessageBox::sorry(nullptr,  i18n("The file \"%1\" is not a local file. "
        "Non-local files cannot be compiled.", dir.path()));
        return false;
    }
    return true;
}



/******************************************************************/
void KateBuildView::clearBuildResults()
{
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
    m_displayModeBeforeBuild = m_buildUi.displayModeSlider->value();
    m_buildUi.displayModeSlider->setValue(0);
    m_win->showToolView(m_toolView);

    // set working directory
    m_make_dir = dir;
    m_make_dir_stack.push(m_make_dir);
    // FIXME check
    m_proc.setWorkingDirectory(m_make_dir);
    m_proc.setShellCommand(command);
    m_proc.start();

    if(!m_proc.waitForStarted(500)) {
        KMessageBox::error(nullptr, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc.exitStatus()));
        return false;
    }

    m_buildUi.cancelBuildButton->setEnabled(true);
    m_buildUi.cancelBuildButton2->setEnabled(true);
    m_buildUi.buildAgainButton->setEnabled(false);
    m_buildUi.buildAgainButton2->setEnabled(false);

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
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
void KateBuildView::slotBuildActiveTarget() {
    if (!m_targetsUi->targetsView->currentIndex().isValid()) {
        slotSelectTarget();
    }
    else {
        buildCurrentTarget();
    }
}

/******************************************************************/
void KateBuildView::slotBuildPreviousTarget() {
    if (!m_previousIndex.isValid()) {
        slotSelectTarget();
    }
    else {
        m_targetsUi->targetsView->setCurrentIndex(m_previousIndex);
        buildCurrentTarget();
    }
}


/******************************************************************/
void KateBuildView::slotBuildDefaultTarget() {
    QModelIndex defaultTarget = m_targetsUi->targetsModel.defaultTarget(m_targetsUi->targetsView->currentIndex());
    m_targetsUi->targetsView->setCurrentIndex(defaultTarget);
    buildCurrentTarget();
}


/******************************************************************/
void KateBuildView::slotSelectTarget() {
    SelectTargetView *dialog = new SelectTargetView(&(m_targetsUi->targetsModel));

    dialog->setCurrentIndex(m_targetsUi->targetsView->currentIndex());

    int result = dialog->exec();
    if (result == QDialog::Accepted) {
        m_targetsUi->targetsView->setCurrentIndex(dialog->currentIndex());
        buildCurrentTarget();
    }
    delete dialog;
    dialog = nullptr;
}


/******************************************************************/
bool KateBuildView::buildCurrentTarget()
{
    if (m_proc.state() != QProcess::NotRunning) {
        displayBuildResult(i18n("Already building..."), KTextEditor::Message::Warning);
        return false;
    }

    QFileInfo docFInfo = docUrl().toLocalFile(); // docUrl() saves the current document

    QModelIndex ind = m_targetsUi->targetsView->currentIndex();
    m_previousIndex = ind;
    if (!ind.isValid()) {
        KMessageBox::sorry(nullptr, i18n("No target available for building."));
        return false;
    }

    QString buildCmd = m_targetsUi->targetsModel.command(ind);
    QString cmdName = m_targetsUi->targetsModel.cmdName(ind);
    QString workDir = m_targetsUi->targetsModel.workDir(ind);
    QString targetSet = m_targetsUi->targetsModel.targetName(ind);

    QString dir = workDir;
    if (workDir.isEmpty()) {
        dir = docFInfo.absolutePath();
        if (dir.isEmpty()) {
            KMessageBox::sorry(nullptr, i18n("There is no local file or directory specified for building."));
            return false;
        }
    }

    // Check if the command contains the file name or directory
    if (buildCmd.contains(QStringLiteral("%f")) ||
        buildCmd.contains(QStringLiteral("%d")) ||
        buildCmd.contains(QStringLiteral("%n")))
    {

        if (docFInfo.absoluteFilePath().isEmpty()) {
            return false;
        }

        buildCmd.replace(QStringLiteral("%n"), docFInfo.baseName());
        buildCmd.replace(QStringLiteral("%f"), docFInfo.absoluteFilePath());
        buildCmd.replace(QStringLiteral("%d"), docFInfo.absolutePath());
    }
    m_filenameDetectorGccWorked = false;
    m_currentlyBuildingTarget = QStringLiteral("%1: %2").arg(targetSet).arg(cmdName);
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
    if (!kv) return;

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
void KateBuildView::slotProcExited(int exitCode, QProcess::ExitStatus)
{
    QApplication::restoreOverrideCursor();
    m_buildUi.cancelBuildButton->setEnabled(false);
    m_buildUi.cancelBuildButton2->setEnabled(false);
    m_buildUi.buildAgainButton->setEnabled(true);
    m_buildUi.buildAgainButton2->setEnabled(true);

    QString buildStatus = i18n("Building <b>%1</b> completed.", m_currentlyBuildingTarget);

    // did we get any errors?
    if (m_numErrors || m_numWarnings || (exitCode != 0)) {
       m_buildUi.u_tabWidget->setCurrentIndex(1);
       if (m_buildUi.displayModeSlider->value() == 0) {
           m_buildUi.displayModeSlider->setValue(m_displayModeBeforeBuild > 0 ? m_displayModeBeforeBuild: 1);
       }
       m_buildUi.errTreeWidget->resizeColumnToContents(0);
       m_buildUi.errTreeWidget->resizeColumnToContents(1);
       m_buildUi.errTreeWidget->resizeColumnToContents(2);
       m_buildUi.errTreeWidget->horizontalScrollBar()->setValue(0);
        //m_buildUi.errTreeWidget->setSortingEnabled(true);
        m_win->showToolView(m_toolView);
    }

    if (m_numErrors || m_numWarnings) {
        QStringList msgs;
        if (m_numErrors) {
            msgs << i18np("Found one error.", "Found %1 errors.", m_numErrors);
            buildStatus = i18n("Building <b>%1</b> had errors.", m_currentlyBuildingTarget);
        }
        else if (m_numWarnings) {
            msgs << i18np("Found one warning.", "Found %1 warnings.", m_numWarnings);
            buildStatus = i18n("Building <b>%1</b> had warnings.", m_currentlyBuildingTarget);
        }
        displayBuildResult(msgs.join(QLatin1Char('\n')), m_numErrors ? KTextEditor::Message::Error : KTextEditor::Message::Warning);
    }
    else if (exitCode != 0) {
        displayBuildResult(i18n("Build failed."), KTextEditor::Message::Warning);
    }
    else {
        displayBuildResult(i18n("Build completed without problems."), KTextEditor::Message::Positive);
    }

    if (!m_buildCancelled) {
        m_buildUi.buildStatusLabel->setText(buildStatus);
        m_buildUi.buildStatusLabel2->setText(buildStatus);
        m_buildCancelled = false;
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

    // handle one line at a time
    do {
        const int end = m_stdOut.indexOf(QLatin1Char('\n'));
        if (end < 0) break;

        const QString line = m_stdOut.mid(0, end);
        m_buildUi.plainTextEdit->appendPlainText(line);
        //qDebug() << line;

        if (m_newDirDetector.match(line).hasMatch()) {
            //qDebug() << "Enter/Exit dir found";
            int open = line.indexOf(QLatin1Char('`'));
            int close = line.indexOf(QLatin1Char('\''));
            QString newDir = line.mid(open+1, close-open-1);
            //qDebug () << "New dir = " << newDir;

            if ((m_make_dir_stack.size() > 1) && (m_make_dir_stack.top() == newDir)) {
                m_make_dir_stack.pop();
                newDir = m_make_dir_stack.top();
            }
            else {
                m_make_dir_stack.push(newDir);
            }

            m_make_dir = newDir;
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
    m_stdErr += l;

    do {
        const int end = m_stdErr.indexOf(QLatin1Char('\n'));
        if (end < 0) break;

        const QString line = m_stdErr.mid(0, end);
        m_buildUi.plainTextEdit->appendPlainText(line);

        processLine(line);

        m_stdErr.remove(0, end + 1);
    } while (1);
}

/******************************************************************/
void KateBuildView::processLine(const QString &line)
{
    //qDebug() << line ;

    //look for a filename
    QRegularExpressionMatch match = m_filenameDetector.match(line);

    if (match.hasMatch())
    {
        m_filenameDetectorGccWorked = true;
    }
    else
    {
        if (!m_filenameDetectorGccWorked)
        {
            // let's see whether the icpc regexp works:
            // so for icpc users error detection will be a bit slower,
            // since always both regexps are checked.
            // But this should be the minority, for gcc and clang users
            // both regexes will only be checked until the first regex
            // matched the first time.
            match = m_filenameDetectorIcpc.match(line);
        }
    }

    if (!match.hasMatch())
    {
        addError(QString(), QStringLiteral("0"), QString(), line);
        //kDebug() << "A filename was not found in the line ";
        return;
    }

    QString filename = match.captured(1);
    const QString line_n = match.captured(3);
    const QString msg = match.captured(4);

    //qDebug() << "File Name:"<<filename<< " msg:"<< msg;
    //add path to file
    if (QFile::exists(m_make_dir + QLatin1Char('/') + filename)) {
        filename = m_make_dir + QLatin1Char('/') + filename;
    }

    // get canonical path, if possible, to avoid duplicated opened files
    auto canonicalFilePath(QFileInfo(filename).canonicalFilePath());
    if (!canonicalFilePath.isEmpty()) {
        filename = canonicalFilePath;
    }

    // Now we have the data we need show the error/warning
    addError(filename, line_n, QStringLiteral("1"), msg);
}


/******************************************************************/
void KateBuildView::slotAddTargetClicked()
{
    QModelIndex current = m_targetsUi->targetsView->currentIndex();
    if (current.parent().isValid()) {
        current = current.parent();
    }
    QModelIndex index = m_targetsUi->targetsModel.addCommand(current.row(), DefTargetName, DefBuildCmd);
    m_targetsUi->targetsView->setCurrentIndex(index);
}


/******************************************************************/
void KateBuildView::targetSetNew()
{
    int row = m_targetsUi->targetsModel.addTargetSet(i18n("Target Set"), QString());
    QModelIndex buildIndex = m_targetsUi->targetsModel.addCommand(row, i18n("Build"), DefBuildCmd);
    m_targetsUi->targetsModel.addCommand(row, i18n("Clean"), DefCleanCmd);
    m_targetsUi->targetsModel.addCommand(row, i18n("Config"), DefConfigCmd);
    m_targetsUi->targetsModel.addCommand(row, i18n("ConfigClean"), DefConfClean);
    m_targetsUi->targetsView->setCurrentIndex(buildIndex);
}

/******************************************************************/
void KateBuildView::targetOrSetCopy()
{
    QModelIndex newIndex = m_targetsUi->targetsModel.copyTargetOrSet(m_targetsUi->targetsView->currentIndex());
    if (m_targetsUi->targetsModel.hasChildren(newIndex)) {
        m_targetsUi->targetsView->setCurrentIndex(newIndex.child(0,0));
        return;
    }
    m_targetsUi->targetsView->setCurrentIndex(newIndex);
}


/******************************************************************/
void KateBuildView::targetDelete()
{
    QModelIndex current = m_targetsUi->targetsView->currentIndex();
    m_targetsUi->targetsModel.deleteItem(current);

    if (m_targetsUi->targetsModel.rowCount() == 0) {
        targetSetNew();
    }
}


/******************************************************************/
void KateBuildView::slotDisplayMode(int mode) {
    QTreeWidget *tree=m_buildUi.errTreeWidget;
    tree->setVisible(mode != 0);
    m_buildUi.plainTextEdit->setVisible(mode == 0);

    QString modeText;
    switch(mode)
    {
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

    for (int i=0;i<itemCount;i++) {
        QTreeWidgetItem* item=tree->topLevelItem(i);

        const ErrorCategory errorCategory = static_cast<ErrorCategory>(item->data(0, ErrorRole).toInt());

        switch (errorCategory)
        {
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
void KateBuildView::slotPluginViewCreated (const QString &name, QObject *pluginView)
{
    // add view
    if (name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = pluginView;
        slotProjectMapChanged ();
        connect(pluginView, SIGNAL(projectMapChanged()), this, SLOT(slotProjectMapChanged()));
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewDeleted (const QString &name, QObject *)
{
    // remove view
    if (name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = nullptr;
        m_targetsUi->targetsModel.deleteTargetSet(i18n("Project Plugin Targets"));
    }
}

/******************************************************************/
void KateBuildView::slotProjectMapChanged ()
{
    // only do stuff with valid project
    if (!m_projectPluginView) {
        return;
    }
    m_targetsUi->targetsModel.deleteTargetSet(i18n("Project Plugin Targets"));
    slotAddProjectTarget();
}

/******************************************************************/
void KateBuildView::slotAddProjectTarget()
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

    int set = m_targetsUi->targetsModel.addTargetSet(i18n("Project Plugin Targets"),
                                                     buildMap.value(QStringLiteral("directory")).toString());

    QVariantList targetsets = buildMap.value(QStringLiteral("targets")).toList();
    foreach (const QVariant &targetVariant, targetsets) {
        QVariantMap targetMap = targetVariant.toMap();
        QString tgtName = targetMap[QStringLiteral("name")].toString();
        QString buildCmd = targetMap[QStringLiteral("build_cmd")].toString();

        if (tgtName.isEmpty() || buildCmd.isEmpty()) {
            continue;
        }
        m_targetsUi->targetsModel.addCommand(set, tgtName, buildCmd);
    }

    QModelIndex ind = m_targetsUi->targetsModel.index(set);
    if (!ind.child(0,0).data().isValid()) {
        QString buildCmd = buildMap.value(QStringLiteral("build")).toString();
        QString cleanCmd = buildMap.value(QStringLiteral("clean")).toString();
        QString quickCmd = buildMap.value(QStringLiteral("quick")).toString();
        if (!buildCmd.isEmpty()) {
            // we have loaded an "old" project file (<= 4.12)
            m_targetsUi->targetsModel.addCommand(set, i18n("build"), buildCmd);
        }
        if (!cleanCmd.isEmpty()) {
            m_targetsUi->targetsModel.addCommand(set, i18n("clean"), cleanCmd);
        }
        if (!quickCmd.isEmpty()) {
            m_targetsUi->targetsModel.addCommand(set, i18n("quick"), quickCmd);
        }
    }
}

/******************************************************************/
bool KateBuildView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
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
    if (!m_win) return;

    QKeyEvent *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        if (m_toolView->isVisible()) {
            m_win->hideToolView(m_toolView);
        }
    }
}


#include "plugin_katebuild.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
