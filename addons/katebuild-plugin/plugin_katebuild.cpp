/* plugin_katebuild.c                    Kate Plugin
**
** Copyright (C) 2013 by Alexander Neundorf <neundorf@kde.org>
** Copyright (C) 2006-2011 by Kåre Särs <kare.sars@iki.fi>
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

#include <QRegExp>
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

#include <QAction>
#include <kmessagebox.h>

#include <knotification.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>


#include "selecttargetdialog.h"

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
    , m_buildWidget(0)
    , m_outputWidgetWidth(0)
    , m_proc(0)
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
    m_win=mw;

    KXMLGUIClient::setComponentName (QLatin1String("katebuild"), i18n ("Kate Build Plugin"));
    setXMLFile(QLatin1String("ui.rc"));

    m_toolView  = mw->createToolView(plugin, QStringLiteral("kate_plugin_katebuildplugin"),
                                      KTextEditor::MainWindow::Bottom,
                                      QIcon::fromTheme(QStringLiteral("application-x-ms-dos-executable")),
                                      i18n("Build Output"));

    QAction *a = actionCollection()->addAction(QStringLiteral("run_make"));
    a->setText(i18n("Build Default Target"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotMake()));

    a = actionCollection()->addAction(QStringLiteral("make_clean"));
    a->setText(i18n("Clean"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotMakeClean()));

    a = actionCollection()->addAction(QStringLiteral("stop"));
    a->setText(i18n("Stop"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotStop()));

    a = actionCollection()->addAction(QStringLiteral("select_target"));
    a->setText(i18n("Build Target..."));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotSelectTarget()));

    a = actionCollection()->addAction(QStringLiteral("build_previous_target"));
    a->setText(i18n("Build Previous Target Again"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotBuildPreviousTarget()));

    a = actionCollection()->addAction(QStringLiteral("goto_next"));
    a->setText(i18n("Next Error"));
    actionCollection()->setDefaultShortcut(a, Qt::CTRL+Qt::ALT+Qt::Key_Right);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotNext()));

    a = actionCollection()->addAction(QStringLiteral("goto_prev"));
    a->setText(i18n("Previous Error"));
    actionCollection()->setDefaultShortcut(a, Qt::CTRL+Qt::ALT+Qt::Key_Left);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotPrev()));

    // KF5 FIXME m_targetSelectAction = actionCollection()->add<KSelectAction>( "targets" );
    // m_targetSelectAction->setText( i18n( "Sets of Targets" ) );
    // connect(m_targetSelectAction, SIGNAL(triggered(int)), this, SLOT(targetSelected(int)));

    a = actionCollection()->addAction(QStringLiteral("target_next"));
    a->setText(i18n("Next Set of Targets"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(targetNext()));

    m_buildWidget = new QWidget(m_toolView);
    m_buildUi.setupUi(m_buildWidget);
    m_targetsUi = new TargetsUi(m_buildUi.u_tabWidget);
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

    connect(m_buildUi.errTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            SLOT(slotItemSelected(QTreeWidgetItem*)));

    m_buildUi.plainTextEdit->setReadOnly(true);
    slotDisplayMode(0);

    connect(m_buildUi.displayModeSlider, SIGNAL(valueChanged(int)), this, SLOT(slotDisplayMode(int)));

    connect(m_buildUi.buildAgainButton, SIGNAL(clicked()), this, SLOT(slotBuildPreviousTarget()));
    connect(m_buildUi.cancelBuildButton, SIGNAL(clicked()), this, SLOT(slotStop()));
    connect(m_buildUi.buildAgainButton2, SIGNAL(clicked()), this, SLOT(slotBuildPreviousTarget()));
    connect(m_buildUi.cancelBuildButton2, SIGNAL(clicked()), this, SLOT(slotStop()));

    connect(m_targetsUi->browse, SIGNAL(clicked()), this, SLOT(slotBrowseClicked()));

    connect(m_targetsUi->addButton, SIGNAL(clicked()), this, SLOT(slotAddTargetClicked()));
    connect(m_targetsUi->buildButton, SIGNAL(clicked()), this, SLOT(slotBuildTargetClicked()));
    connect(m_targetsUi->deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteTargetClicked()));
    connect(m_targetsUi->targetsList, SIGNAL(cellChanged(int, int)), this, SLOT(slotCellChanged(int, int)));
    connect(m_targetsUi->targetsList, SIGNAL(itemSelectionChanged()), this, SLOT(slotSelectionChanged()));

    connect(m_targetsUi->targetsList->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(slotResizeColumn(int)));

    QCompleter* dirCompleter = new QCompleter(this);
    QStringList filter;
    dirCompleter->setModel(new QDirModel(filter, QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name, this));
    m_targetsUi->buildDir->setCompleter(dirCompleter);

    m_targetsUi->deleteButton->setEnabled(false);
    m_targetsUi->buildButton->setEnabled(false);

    m_proc = new QProcess();

    connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotProcExited(int,QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyReadStandardError()),this, SLOT(slotReadReadyStdErr()));
    connect(m_proc, SIGNAL(readyReadStandardOutput()),this, SLOT(slotReadReadyStdOut()));

    connect(m_targetsUi->targetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(targetSelected(int)));
    connect(m_targetsUi->targetCombo->lineEdit(), SIGNAL(textEdited(const QString&)), this, SLOT(slotTargetSetNameChanged(const QString&)));
    connect(m_targetsUi->newTarget, SIGNAL(clicked()), this, SLOT(targetNew()));
    connect(m_targetsUi->copyTarget, SIGNAL(clicked()), this, SLOT(targetCopy()));
    connect(m_targetsUi->deleteTarget, SIGNAL(clicked()), this, SLOT(targetDelete()));
    connect(m_targetsUi->buildDir, SIGNAL(textChanged(const QString&)), this, SLOT(slotBuildDirChanged(const QString&)));

    connect(m_win, SIGNAL(unhandledShortcutOverride(QEvent*)),
            this, SLOT(handleEsc(QEvent*)));

    m_toolView->installEventFilter(this);

    m_win->guiFactory()->addClient(this);

    // watch for project plugin view creation/deletion
    connect(m_win, SIGNAL(pluginViewCreated (const QString &, QObject *))
        , this, SLOT(slotPluginViewCreated (const QString &, QObject *)));

    connect(m_win, SIGNAL(pluginViewDeleted (const QString &, QObject *))
        , this, SLOT(slotPluginViewDeleted (const QString &, QObject *)));

    // update once project plugin state manually
    m_projectPluginView = m_win->pluginView (QStringLiteral("kateprojectplugin"));
    slotProjectMapChanged ();
    slotSelectionChanged();
}


/******************************************************************/
KateBuildView::~KateBuildView()
{
    m_win->guiFactory()->removeClient( this );
    delete m_proc;
    delete m_toolView;
}

/******************************************************************/
QWidget *KateBuildView::toolView() const
{
    return m_toolView;
}

/******************************************************************/
void KateBuildView::readSessionConfig(const KConfigGroup& cg)
{
    qDebug() << "";
    m_targetsUi->targetCombo->blockSignals(true);

    int numTargets = cg.readEntry(QStringLiteral("NumTargets"), 0);
    m_targetsUi->targetCombo->clear();
    m_targetList.clear();
    m_targetIndex = 0;
    int tmpIndex;
    if (numTargets == 0 ) {
        // either the config is empty or uses the older format
        m_targetList.append(TargetSet());

        m_targetList[0].name = QStringLiteral("Default");
        m_targetList[0].defaultTarget = QStringLiteral("build");
        m_targetList[0].cleanTarget = QStringLiteral("clean");
        m_targetList[0].defaultDir = cg.readEntry(QStringLiteral("Make Path"), QString());

        m_targetList[0].prevTarget.clear();

        m_targetList[0].targets[QStringLiteral("config")] = DefConfigCmd;
        m_targetList[0].targets[QStringLiteral("build")] = cg.readEntry(QStringLiteral("Make Command"), DefBuildCmd);
        m_targetList[0].targets[QStringLiteral("clean")] = cg.readEntry(QStringLiteral("Make Command"), DefCleanCmd);

        QString quickCmd = cg.readEntry(QStringLiteral("Quick Compile Command"));
        if (!quickCmd.isEmpty()) {
            m_targetList[0].targets[QStringLiteral("quick")] = quickCmd;
        }

        tmpIndex = 0;
    }
    else {

        for (int i=0; i<numTargets; i++) {
            m_targetList.append(TargetSet());
            m_targetList[i].name = cg.readEntry(QStringLiteral("%1 Target").arg(i), QStringLiteral("Target %1").arg(i+1));

            QStringList targetNames = cg.readEntry(QStringLiteral("%1 Target Names").arg(i), QStringList());
            QString defaultTarget = cg.readEntry(QStringLiteral("%1 Target Default").arg(i), QString());
            QString cleanTarget = cg.readEntry(QStringLiteral("%1 Target Clean").arg(i), QString());
            QString defaultDir = cg.readEntry(QStringLiteral("%1 BuildPath").arg(i), QString());

            if (targetNames.isEmpty()) {
                m_targetList[i].defaultTarget = QStringLiteral("build");
                m_targetList[i].cleanTarget = QStringLiteral("clean");
                m_targetList[i].prevTarget.clear();
                m_targetList[i].defaultDir = defaultDir;

                m_targetList[i].targets[QStringLiteral("build")] = cg.readEntry(QStringLiteral("%1 BuildCmd").arg(i), DefBuildCmd);
                m_targetList[i].targets[QStringLiteral("clean")] = cg.readEntry(QStringLiteral("%1 CleanCmd").arg(i), DefCleanCmd);

                QString quickCmd = cg.readEntry(QStringLiteral("%1 QuickCmd").arg(i));
                if (!quickCmd.isEmpty()) {
                    m_targetList[i].targets[QStringLiteral("quick")] = quickCmd;
                }
            }
            else {
                for (int tn=0; tn<targetNames.size(); ++tn) {
                    const QString& targetName = targetNames.at(tn);
                    if (defaultTarget.isEmpty() && (tn==0)) {
                        defaultTarget = targetName;
                    }
                    m_targetList[i].targets[targetName] = cg.readEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(targetName), DefBuildCmd);
                }

                m_targetList[i].defaultTarget = defaultTarget;
                m_targetList[i].cleanTarget = cleanTarget;
                m_targetList[i].prevTarget.clear();
                m_targetList[i].defaultDir = defaultDir;
            }

        }
        tmpIndex = cg.readEntry(QStringLiteral("Active Target Index"), 0);
    }

    for(int i=0; i<m_targetList.size(); i++) {
        m_targetsUi->targetCombo->addItem(m_targetList[i].name);
    }


    // update the targets menu
    targetsChanged();

    // select the last active target if possible
    m_targetsUi->targetCombo->setCurrentIndex(tmpIndex);
    targetSelected(tmpIndex);

    m_targetsUi->targetCombo->blockSignals(false);
}

/******************************************************************/
void KateBuildView::writeSessionConfig(KConfigGroup& cg)
{
    qDebug() << "";
    cg.writeEntry(QStringLiteral("NumTargets"), m_targetList.size());
    for (int i=0; i<m_targetList.size(); i++) {
        cg.writeEntry(QStringLiteral("%1 Target").arg(i), m_targetList[i].name);
        cg.writeEntry(QStringLiteral("%1 Target Default").arg(i), m_targetList[i].defaultTarget);
        cg.writeEntry(QStringLiteral("%1 Target Clean").arg(i), m_targetList[i].cleanTarget);
        cg.writeEntry(QStringLiteral("%1 BuildPath").arg(i), m_targetList[i].defaultDir);

        QStringList targetNames;

        for(std::map<QString, QString>::const_iterator tgtIt = m_targetList[i].targets.begin();
            tgtIt != m_targetList[i].targets.end(); ++tgtIt) {
            const QString& tgtName = tgtIt->first;
            const QString& buildCmd = tgtIt->second;

            targetNames << tgtName;

            cg.writeEntry(QStringLiteral("%1 BuildCmd %2").arg(i).arg(tgtName), buildCmd);
        }

        cg.writeEntry(QStringLiteral("%1 Target Names").arg(i), targetNames);
    }
    cg.writeEntry(QStringLiteral("Active Target Index"), m_targetIndex);
}


/******************************************************************/
void KateBuildView::slotNext()
{
    const int itemCount = m_buildUi.errTreeWidget->topLevelItemCount();
    if (itemCount == 0) {
        return;
    }

    QTreeWidgetItem *item = m_buildUi.errTreeWidget->currentItem();
    if (item && item->isHidden()) item = 0;

    int i = (item == 0) ? -1 : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (++i < itemCount) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty() && !item->isHidden()) {
            m_buildUi.errTreeWidget->setCurrentItem(item);
            m_buildUi.errTreeWidget->scrollToItem(item);
            slotItemSelected(item);
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
    if (item && item->isHidden()) item = 0;

    int i = (item == 0) ? itemCount : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (--i >= 0) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty() && !item->isHidden()) {
            m_buildUi.errTreeWidget->setCurrentItem(item);
            m_buildUi.errTreeWidget->scrollToItem(item);
            slotItemSelected(item);
            return;
        }
    }
}

/******************************************************************/
void KateBuildView::slotItemSelected(QTreeWidgetItem *item)
{
    // get stuff
    const QString filename = item->data(0, Qt::UserRole).toString();
    if (filename.isEmpty()) return;
    const int line = item->data(1, Qt::UserRole).toInt();
    const int column = item->data(2, Qt::UserRole).toInt();

    // open file (if needed, otherwise, this will activate only the right view...)
    m_win->openUrl(QUrl::fromUserInput(filename));

    // any view active?
    if (!m_win->activeView()) {
        return;
    }

    // do it ;)
    m_win->activeView()->setCursorPosition(KTextEditor::Cursor(line-1, column));
    m_win->activeView()->setFocus();
}


/******************************************************************/
void KateBuildView::addError(const QString &filename, const QString &line,
                             const QString &column, const QString &message)
{
    bool isError=false;
    bool isWarning=false;
    QTreeWidgetItem* item = new QTreeWidgetItem(m_buildUi.errTreeWidget);
    item->setBackground(1, Qt::gray);
    // The strings are twice in case kate is translated but not make.
    if (message.contains(QStringLiteral("error")) ||
        message.contains(i18nc("The same word as 'make' uses to mark an error.","error")) ||
        message.contains(QStringLiteral("undefined reference")) ||
        message.contains(i18nc("The same word as 'ld' uses to mark an ...","undefined reference"))
       )
    {
        isError=true;
        item->setForeground(1, Qt::red);
        m_numErrors++;
        item->setHidden(false);
    }
    if (message.contains(QStringLiteral("warning")) ||
        message.contains(i18nc("The same word as 'make' uses to mark a warning.","warning"))
       )
    {
        isWarning=true;
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

    if ((isError==false) && (isWarning==false)) {
      item->setHidden(m_buildUi.displayModeSlider->value() > 1);
    }

    item->setData(0,Qt::UserRole+1,isError);
    item->setData(0,Qt::UserRole+2,isWarning);

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
        KMessageBox::sorry(0, i18n("There is no file or directory specified for building."));
        return false;
    }
    else if (!dir.isLocalFile()) {
        KMessageBox::sorry(0,  i18n("The file \"%1\" is not a local file. "
        "Non-local files cannot be compiled.", dir.path()));
        return false;
    }
    return true;
}



/******************************************************************/
void KateBuildView::slotBuildPreviousTarget() {
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    if (tgtSet->prevTarget.isEmpty()) {
        KMessageBox::sorry(0, i18n("No previous target to build."));
        return;
    }

    buildTarget(tgtSet->prevTarget);
}


/******************************************************************/
bool KateBuildView::slotMake(void)
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return false;
    }

    if (tgtSet->defaultTarget.isEmpty()) {
        KMessageBox::sorry(0, i18n("No target set as default target."));
        return false;
    }

    return buildTarget(tgtSet->defaultTarget);
}

/******************************************************************/
bool KateBuildView::slotMakeClean(void)
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return false;
    }

    if (tgtSet->cleanTarget.isEmpty()) {
        KMessageBox::sorry(0, i18n("No target set as clean target."));
        return false;
    }

    return buildTarget(tgtSet->cleanTarget);
}

/******************************************************************/
void KateBuildView::clearBuildResults()
{
    m_buildUi.plainTextEdit->clear();
    m_buildUi.errTreeWidget->clear();
    m_output_lines.clear();
    m_numErrors = 0;
    m_numWarnings = 0;
    m_make_dir_stack.clear();
}

bool KateBuildView::startProcess(const QString &dir, const QString &command)
{
    if (m_proc->state() != QProcess::NotRunning) {
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
    m_proc->setWorkingDirectory(m_make_dir);
    m_proc->start(command);

    if(!m_proc->waitForStarted(500)) {
        KMessageBox::error(0, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc->exitStatus()));
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
    if (m_proc->state() != QProcess::NotRunning) {
        m_buildCancelled = true;
        QString msg = i18n("Building <b>%1</b> cancelled", m_currentlyBuildingTarget);
        m_buildUi.buildStatusLabel->setText(msg);
        m_buildUi.buildStatusLabel2->setText(msg);
        m_proc->terminate();
        return true;
    }
    return false;
}

/******************************************************************/
void KateBuildView::slotSelectTarget() {
    TargetSet* targetSet = currentTargetSet();
    if (targetSet == 0) {
        return;
    }

    SelectTargetDialog* dlg = new SelectTargetDialog(m_targetList, 0);
    dlg->setTargetSet(targetSet->name);

    int result = dlg->exec();
    if (result == QDialog::Accepted) {
        QString target = dlg->selectedTarget();
        buildTarget(target);
    }
    delete dlg;
    dlg = 0;
}


/******************************************************************/
KateBuildView::TargetSet* KateBuildView::currentTargetSet()
{
    if (m_targetIndex >= m_targetList.size()) {
        return 0;
    }

    return &m_targetList[m_targetIndex];
}


/******************************************************************/
bool KateBuildView::buildTarget(const QString& targetName)
{
    QFileInfo docFInfo = docUrl().toLocalFile(); // docUrl() saves the current document

    TargetSet* targetSet = currentTargetSet();

    if (targetSet == 0) {
        return false;
    }

    std::map<QString, QString>::const_iterator tgtIt = targetSet->targets.find(targetName);
    if (tgtIt == targetSet->targets.end()) {
        KMessageBox::sorry(0, i18n("Target \"%1\" not found for building.", targetName));
        return false;
    }

    QString buildCmd = tgtIt->second;
    qDebug() << targetName << docUrl() << targetSet->defaultDir;
    QString dir;
    if (targetSet->defaultDir.isEmpty()) {
        dir = docFInfo.absolutePath();
        if (dir.isEmpty()) {
            KMessageBox::sorry(0, i18n("There is no local file or directory specified for building."));
            return false;
        }
    }
    else {
        dir = targetSet->defaultDir;
    }

    targetSet->prevTarget = targetName;

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
    m_currentlyBuildingTarget = targetName;
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

    QPointer<KTextEditor::Message> message = new KTextEditor::Message(xi18nc("@info", "<title>Make Results:</title><nl/><pre><code>%1</code></pre>", msg), level);
    message->setWordWrap(true);
    message->setAutoHide(1000);
    kv->document()->postMessage(message);
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
    QString l= QString::fromUtf8(m_proc->readAllStandardOutput());
    l.remove(QLatin1Char('\r'));
    m_output_lines += l;

    QString tmp;

    int end=0;


    // handle one line at a time
    do {
        end = m_output_lines.indexOf(QLatin1Char('\n'));
        if (end < 0) break;
        end++;
        tmp = m_output_lines.mid(0, end);
        tmp.remove(QLatin1Char('\n'));
        m_buildUi.plainTextEdit->appendPlainText(tmp);
        //qDebug() << tmp;
        if (tmp.indexOf(m_newDirDetector) >=0) {
            //qDebug() << "Enter/Exit dir found";
            int open = tmp.indexOf(QLatin1Char('`'));
            int close = tmp.indexOf(QLatin1Char('\''));
            QString newDir = tmp.mid(open+1, close-open-1);
            qDebug () << "New dir = " << newDir;

            if ((m_make_dir_stack.size() > 1) && (m_make_dir_stack.top() == newDir)) {
                m_make_dir_stack.pop();
                newDir = m_make_dir_stack.top();
            }
            else {
                m_make_dir_stack.push(newDir);
            }

            m_make_dir = newDir;
        }


        m_output_lines.remove(0,end);

    } while (1);

}

/******************************************************************/
void KateBuildView::slotReadReadyStdErr()
{
    // FIXME This works for utf8 but not for all charsets
    QString l= QString::fromUtf8(m_proc->readAllStandardError());
    l.remove(QLatin1Char('\r'));
    m_output_lines += l;

    QString tmp;

    int end=0;

    do {
        end = m_output_lines.indexOf(QLatin1Char('\n'));
        if (end < 0) break;
        end++;
        tmp = m_output_lines.mid(0, end);
        tmp.remove(QLatin1Char('\n'));
        m_buildUi.plainTextEdit->appendPlainText(tmp);

        processLine(tmp);

        m_output_lines.remove(0,end);

    } while (1);

}

/******************************************************************/
void KateBuildView::processLine(const QString &line)
{
    //qDebug() << line ;
    
    //look for a filename
    int index = m_filenameDetector.indexIn(line);

    QRegExp* rx = 0;
    if (index >= 0)
    {
        m_filenameDetectorGccWorked = true;
        rx = &m_filenameDetector;
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
            index = m_filenameDetectorIcpc.indexIn(line);
            if (index >= 0)
            {
                rx = &m_filenameDetectorIcpc;
            }
        }
    }

    if (!rx)
    {
        addError(QString(), QStringLiteral("0"), QString(), line);
        //kDebug() << "A filename was not found in the line ";
        return;
    }

    QString filename = rx->cap(1);
    QString line_n = rx->cap(3);
    QString msg = rx->cap(4);

    //qDebug() << "File Name:"<<filename<< " msg:"<< msg;
    //add path to file
    if (QFile::exists(m_make_dir + QLatin1Char('/') + filename)) {
        filename = m_make_dir + QLatin1Char('/') + filename;
    }

    // Now we have the data we need show the error/warning
    addError(filename, line_n, QString(), msg);

}

/******************************************************************/
void KateBuildView::slotBrowseClicked()
{
    QUrl defDir = QUrl::fromUserInput(m_targetsUi->buildDir->text());

    if (m_targetsUi->buildDir->text().isEmpty()) {
        // try current document dir
        KTextEditor::View *kv = m_win->activeView();
        if (kv != 0) {
            defDir = kv->document()->url();
        }
    }

    QString newDir = QFileDialog::getExistingDirectory(0, QString(), defDir.toLocalFile());
    if (!newDir.isEmpty()) {
        m_targetsUi->buildDir->setText(newDir);
    }
}

/******************************************************************/
void KateBuildView::slotBuildDirChanged(const QString& dir)
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    tgtSet->defaultDir = dir;
}

/******************************************************************/
void KateBuildView::slotCellChanged(int row, int column)
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    bool wasBlocked = m_targetsUi->targetsList->blockSignals(true);
    QTableWidgetItem* item = m_targetsUi->targetsList->item(row, column);

    QString prevTargetName = m_targetsUi->targetsList->item(row, COL_NAME)->text();
    if (column == COL_NAME) {
        prevTargetName = m_prevItemContent;
    }

    QString prevCommand = m_targetsUi->targetsList->item(row, COL_COMMAND)->text();

    switch (column)
    {
    case COL_DEFAULT_TARGET:
    case COL_CLEAN_TARGET:
        for(int i=0; i<m_targetsUi->targetsList->rowCount(); i++) {
            m_targetsUi->targetsList->item(i, column)->setCheckState(Qt::Unchecked);
        }
        item->setCheckState(Qt::Checked);
        if (column == COL_DEFAULT_TARGET) {
            tgtSet->defaultTarget = prevTargetName;
        }
        else {
            tgtSet->cleanTarget = prevTargetName;
        }
        break;
    case COL_NAME:
    {
        QString newName  = item->text();
        if (newName.isEmpty()) {
            item->setText(prevTargetName);
        }
        else {
            m_targetList[m_targetIndex].targets.erase(prevTargetName);
            newName = makeTargetNameUnique(newName);
            m_targetList[m_targetIndex].targets[newName] = prevCommand;
        }
        break;
    }
    case COL_COMMAND:
        m_targetList[m_targetIndex].targets[prevTargetName] = item->text();
        break;
    }

    m_targetsUi->targetsList->blockSignals(wasBlocked);
}


/******************************************************************/
void KateBuildView::slotSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_targetsUi->targetsList->selectedItems();
    bool enableButtons = (selectedItems.size() > 0);
    if (enableButtons) {
        m_prevItemContent = selectedItems.at(0)->text();
    }
    m_targetsUi->deleteButton->setEnabled(enableButtons);
    m_targetsUi->buildButton->setEnabled(enableButtons);
}


/******************************************************************/
void KateBuildView::slotResizeColumn(int column)
{
    m_targetsUi->targetsList->resizeColumnToContents(column);
}

/******************************************************************/
QString KateBuildView::makeTargetNameUnique(const QString& name)
{
    TargetSet* targetSet = currentTargetSet();
    if (targetSet == 0) {
        return name;
    }

    QString uniqueName = name;
    int count = 2;

    while (m_targetList[m_targetIndex].targets.find(uniqueName) != m_targetList[m_targetIndex].targets.end()) {
        uniqueName = QStringLiteral("%1_%2").arg(name).arg(count);
        count++;
    }

    return uniqueName;
}


/******************************************************************/
void KateBuildView::slotAddTargetClicked()
{
    TargetSet* targetSet = currentTargetSet();
    if (targetSet == 0) {
        return;
    }

    bool wasBlocked = m_targetsUi->targetsList->blockSignals(true);
    QString newName = makeTargetNameUnique(DefTargetName);

    int rowCount = m_targetList[m_targetIndex].targets.size();
    m_targetsUi->targetsList->setRowCount(rowCount + 1);
    setTargetRowContents(rowCount, m_targetList[m_targetIndex], newName, DefBuildCmd);

    m_targetList[m_targetIndex].targets[newName] = DefBuildCmd;

    m_targetsUi->deleteButton->setEnabled(true);
    m_targetsUi->buildButton->setEnabled(true);
    m_targetsUi->targetsList->blockSignals(wasBlocked);
}


/******************************************************************/
void KateBuildView::slotBuildTargetClicked()
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    QList<QTableWidgetItem*> selectedItems = m_targetsUi->targetsList->selectedItems();
    if (selectedItems.size() == 0) {
        return;
    }

    int row = selectedItems.at(0)->row();

    QString target = m_targetsUi->targetsList->item(row, COL_NAME)->text();

    buildTarget(target);
}

/******************************************************************/
void KateBuildView::slotDeleteTargetClicked()
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    QList<QTableWidgetItem*> selectedItems = m_targetsUi->targetsList->selectedItems();
    if (selectedItems.size() == 0) {
        return;
    }

    int row = selectedItems.at(0)->row();

    QString target = m_targetsUi->targetsList->item(row, COL_NAME)->text();

    int result = KMessageBox::questionYesNo(0, i18n("Really delete target %1?", target));
    if (result == KMessageBox::No) {
        return;
    }

    m_targetsUi->targetsList->removeRow(row);

    if (tgtSet->cleanTarget == target) {
        tgtSet->cleanTarget.clear();
    }

    if (tgtSet->defaultTarget == target) {
        tgtSet->defaultTarget.clear();
    }

    tgtSet->targets.erase(target);

    bool enableButtons = (m_targetsUi->targetsList->rowCount() > 0);
    m_targetsUi->deleteButton->setEnabled(enableButtons);
    m_targetsUi->buildButton->setEnabled(enableButtons);
}


/******************************************************************/
void KateBuildView::targetSelected(int index)
{
    if (index >= m_targetList.size() || (index < 0)) {
        qDebug() << "Invalid target";
        return;
    }

    // Set the new values
    bool wasBlocked = m_targetsUi->targetsList->blockSignals(true);
    bool dirWasBlocked = m_targetsUi->buildDir->blockSignals(true);

    m_targetsUi->buildDir->setText(m_targetList[index].defaultDir);

    m_targetsUi->targetsList->setRowCount(m_targetList[index].targets.size());

    int row=0;
    for(std::map<QString, QString>::const_iterator it = m_targetList[index].targets.begin(); it != m_targetList[index].targets.end(); ++it) {
        setTargetRowContents(row, m_targetList[index], it->first, it->second);
        row++;
    }

    m_targetsUi->targetsList->blockSignals(wasBlocked);
    m_targetsUi->buildDir->blockSignals(dirWasBlocked);

    m_targetsUi->targetsList->resizeColumnsToContents();

    m_targetIndex = index;

    // make sure that both the combo box and the menu are updated
    m_targetsUi->targetCombo->setCurrentIndex(index);
    //m_targetSelectAction->setCurrentItem(index);

    const bool enableButtons = (m_targetsUi->targetsList->currentItem() != 0);

    m_targetsUi->deleteButton->setEnabled(enableButtons);
    m_targetsUi->buildButton->setEnabled(enableButtons);

    clearBuildResults();
    m_currentlyBuildingTarget.clear();
    m_buildUi.buildStatusLabel->setText(i18n("Nothing built yet."));
    m_buildUi.buildStatusLabel2->setText(i18n("Nothing built yet."));
}

/******************************************************************/
void KateBuildView::setTargetRowContents(int row, const TargetSet& tgtSet, const QString& name, const QString& buildCmd)
{
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);
    QTableWidgetItem* cmdItem = new QTableWidgetItem(buildCmd);
    QTableWidgetItem* def = new QTableWidgetItem();
    QTableWidgetItem* clean = new QTableWidgetItem();

    def->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
    clean->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);

    def->setCheckState(name == tgtSet.defaultTarget ? Qt::Checked : Qt::Unchecked);
    clean->setCheckState(name == tgtSet.cleanTarget ? Qt::Checked : Qt::Unchecked);

    m_targetsUi->targetsList->setItem(row, COL_DEFAULT_TARGET, def);
    m_targetsUi->targetsList->setItem(row, COL_CLEAN_TARGET, clean);
    m_targetsUi->targetsList->setItem(row, COL_NAME, nameItem);
    m_targetsUi->targetsList->setItem(row, COL_COMMAND, cmdItem);
}


/******************************************************************/
void KateBuildView::slotTargetSetNameChanged(const QString& name)
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    tgtSet->name = name;
    targetsChanged();
}

/******************************************************************/
void KateBuildView::targetsChanged()
{
    QStringList items;

    for( int i = 0; i < m_targetList.size(); ++i ) {
        items.append( m_targetList[i].name );
    }
    // m_targetSelectAction->setItems( items );
    // m_targetSelectAction->setCurrentItem( m_targetIndex );
}

/******************************************************************/
QString KateBuildView::makeUniqueTargetSetName() const
{
    QString uniqueName;

    int count = 0;
    bool nameAlreadyUsed = false;
    do {
        count++;
        uniqueName = i18n("Target Set %1", count);

        nameAlreadyUsed = false;
        for (int i=0; i<m_targetList.size(); i++) {
            if (m_targetList[i].name == uniqueName) {
                nameAlreadyUsed = true;
                break;
            }
        }
    } while (nameAlreadyUsed == true);

    return uniqueName;
}

/******************************************************************/
void KateBuildView::targetNew()
{
    m_targetList.append(TargetSet());
    m_targetIndex = m_targetList.size()-1;
    m_targetList[m_targetIndex].name = makeUniqueTargetSetName();
    m_targetList[m_targetIndex].defaultTarget = QStringLiteral("Build");
    m_targetList[m_targetIndex].prevTarget.clear();
    m_targetList[m_targetIndex].cleanTarget = QStringLiteral("Clean");
    m_targetList[m_targetIndex].defaultDir.clear();

    m_targetList[m_targetIndex].targets[QStringLiteral("Build")] = DefBuildCmd;
    m_targetList[m_targetIndex].targets[QStringLiteral("Clean")] = DefCleanCmd;
    m_targetList[m_targetIndex].targets[QStringLiteral("Config")] = DefConfigCmd;

    m_targetsUi->targetCombo->addItem(m_targetList[m_targetIndex].name);
    m_targetsUi->targetCombo->setCurrentIndex(m_targetIndex);

    // update the targets menu
    targetsChanged();
}

/******************************************************************/
void KateBuildView::targetCopy()
{
    TargetSet tgt = *currentTargetSet();
    m_targetList.append(tgt);
    m_targetIndex = m_targetList.size()-1;
    m_targetList[m_targetIndex].name = makeUniqueTargetSetName();

    m_targetsUi->targetCombo->addItem(m_targetList[m_targetIndex].name);
    m_targetsUi->targetCombo->setCurrentIndex(m_targetIndex);

    // update the targets menu
    targetsChanged();
}

/******************************************************************/
void KateBuildView::targetDelete()
{
    m_targetsUi->targetCombo->blockSignals(true);

    int newTargetIndex = 0;

    if (m_targetList.size() > 1) {
        m_targetList.removeAt(m_targetIndex);
        m_targetsUi->targetCombo->removeItem(m_targetIndex);
        if (m_targetIndex > 0) {
            newTargetIndex = m_targetIndex - 1;
        }

    }
    else {
        m_targetsUi->targetCombo->clear();
        m_targetList.clear();

        m_targetList.append(TargetSet());
        m_targetList[0].name = i18n("Target");
        m_targetList[0].defaultTarget = QStringLiteral("Build");
        m_targetList[0].cleanTarget = QStringLiteral("Clean");
        m_targetList[0].prevTarget.clear();
        m_targetList[0].defaultDir.clear();

        m_targetList[0].targets[QStringLiteral("Build")] = DefBuildCmd;
        m_targetList[0].targets[QStringLiteral("Clean")] = DefCleanCmd;
        m_targetList[0].targets[QStringLiteral("Config")] = DefConfigCmd;
        m_targetList[0].targets[QStringLiteral("ConfigClean")] = DefConfClean;

        m_targetsUi->targetCombo->addItem(m_targetList[0].name);

    }

    m_targetsUi->targetCombo->blockSignals(false);

    targetSelected(newTargetIndex);

    // update the targets menu
    targetsChanged();
}

/******************************************************************/
void KateBuildView::targetNext()
{
    if (m_toolView->isVisible() && m_buildUi.u_tabWidget->currentIndex() == 0) {
        int index = m_targetsUi->targetCombo->currentIndex();
        index++;
        if (index == m_targetsUi->targetCombo->count()) index = 0;

        m_targetsUi->targetCombo->setCurrentIndex(index);
    }
    else {
        m_win->showToolView(m_toolView);
        m_buildUi.u_tabWidget->setCurrentIndex(0);
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


/******************************************************************/
void KateBuildView::slotDisplayMode(int mode) {
    QTreeWidget *tree=m_buildUi.errTreeWidget;
    tree->setVisible(mode != 0);
    m_buildUi.plainTextEdit->setVisible(mode == 0);

    QString modeText;
    switch(mode)
    {
        case 3:
            modeText = i18n("Only Errors");
            break;
        case 2:
            modeText = i18n("Errors and Warnings");
            break;
        case 1:
            modeText = i18n("Parsed Output");
            break;
        case 0:
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

        if ( (item->data(0,Qt::UserRole+1).toBool()==false) && (item->data(0,Qt::UserRole+2).toBool()==false) ) {
            item->setHidden(mode > 1);
        }

        if (item->data(0,Qt::UserRole+2).toBool()==true) {
            item->setHidden(mode > 2);
        }

        if (item->data(0,Qt::UserRole+1).toBool()==true) {
            item->setHidden(false);
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
        connect (pluginView, SIGNAL(projectMapChanged()), this, SLOT(slotProjectMapChanged()));
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewDeleted (const QString &name, QObject *)
{
    // remove view
    if (name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = 0;
        slotRemoveProjectTarget();
    }
}

/******************************************************************/
void KateBuildView::slotProjectMapChanged ()
{
    // only do stuff with valid project
    if (!m_projectPluginView) {
        return;
    }
    slotRemoveProjectTarget();
    slotAddProjectTarget();

}

/******************************************************************/
void KateBuildView::slotAddProjectTarget()
{
    // query new project map
    QVariantMap projectMap = m_projectPluginView->property("projectMap").toMap();

    // do we have a valid map for build settings?
    QVariantMap buildMap = projectMap.value(QStringLiteral("build")).toMap();
    if (buildMap.isEmpty()) {
        return;
    }

    int index = m_targetList.size();
    m_targetList.append(TargetSet());
    m_targetList[index].name = i18n("Project Plugin Targets");
    m_targetList[index].cleanTarget = buildMap.value(QStringLiteral("clean_target")).toString();
    m_targetList[index].defaultTarget = buildMap.value(QStringLiteral("default_target")).toString();
    m_targetList[index].prevTarget.clear();
    m_targetList[index].defaultDir = buildMap.value(QStringLiteral("directory")).toString();

    // get build dir
    QString defaultBuildDir = buildMap.value(QStringLiteral("directory")).toString();

    QVariantList targets = buildMap.value(QStringLiteral("targets")).toList();
    foreach (const QVariant &targetVariant, targets) {
        QVariantMap targetMap = targetVariant.toMap();
        QString tgtName = targetMap[QStringLiteral("name")].toString();
        QString buildCmd = targetMap[QStringLiteral("build_cmd")].toString();

        if (tgtName.isEmpty() || buildCmd.isEmpty()) {
            continue;
        }

        m_targetList[index].targets[tgtName] = buildCmd;
    }

    if (targets.empty()) {
        // support "old" project files
        QString buildCmd = buildMap.value(QStringLiteral("build")).toString();
        QString cleanCmd = buildMap.value(QStringLiteral("clean")).toString();
        QString quickCmd = buildMap.value(QStringLiteral("quick")).toString();
        if (!buildCmd.isEmpty()) {
            // we have loaded an "old" project file (<= 4.12)
            m_targetList[index].cleanTarget = QStringLiteral("clean");
            m_targetList[index].defaultTarget = QStringLiteral("build");
            m_targetList[index].prevTarget.clear();
            m_targetList[index].targets[QStringLiteral("build")] = buildCmd;
            m_targetList[index].targets[QStringLiteral("clean")] = cleanCmd;
            m_targetList[index].targets[QStringLiteral("quick")] = quickCmd;
        }
    }

    m_targetsUi->targetCombo->addItem(m_targetList[index].name);

    targetSelected(index);


    // set build dir

    // update the targets menu
    targetsChanged();
}

/******************************************************************/
void KateBuildView::slotRemoveProjectTarget()
{
    int i;
    for (i=0; i<m_targetList.size(); i++) {
        if (m_targetList[i].name == i18n("Project Plugin Targets")) {
            break;
        }
    }
    if (i >= m_targetList.size()) {
        // not found
        return;
    }

    targetSelected(i);
    targetDelete();
}

#include "plugin_katebuild.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
