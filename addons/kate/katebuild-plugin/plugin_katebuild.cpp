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

#include "plugin_katebuild.moc"

#include <cassert>

#include <qfile.h>
#include <qinputdialog.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qpalette.h>
#include <qlabel.h>
#include <QApplication>
#include <QCompleter>
#include <QDirModel>
#include <QScrollBar>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDir>


#include <kaction.h>
#include <kactioncollection.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kpassivepopup.h>
#include <klineedit.h>
#include <kconfig.h>
#include <kfiledialog.h>

#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>


#include "edittargetdialog.h"
#include "selecttargetdialog.h"

K_PLUGIN_FACTORY(KateBuildPluginFactory, registerPlugin<KateBuildPlugin>();)
K_EXPORT_PLUGIN(KateBuildPluginFactory(KAboutData("katebuild",
                                                  "katebuild-plugin",
                                                  ki18n("Build Plugin"),
                                                  "0.1",
                                                  ki18n( "Build Plugin"))))

static const QString DefConfigCmd = "cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ../";
static const QString DefConfClean = "";
static const QString DefBuildCmd = "make";
static const QString DefCleanCmd = "make clean";
static const QString DefQuickCmd = "gcc -Wall -g %f";


/******************************************************************/
KateBuildPlugin::KateBuildPlugin(QObject *parent, const VariantList&):
Kate::Plugin ((Kate::Application*)parent, "kate-build-plugin")
{
    KGlobal::locale()->insertCatalog("katebuild-plugin");
}

/******************************************************************/
Kate::PluginView *KateBuildPlugin::createView (Kate::MainWindow *mainWindow)
{
    return new KateBuildView(mainWindow);
}

/******************************************************************/
KateBuildView::KateBuildView(Kate::MainWindow *mw)
    : Kate::PluginView (mw)
    , Kate::XMLGUIClient(KateBuildPluginFactory::componentData())
     , m_toolView (mw->createToolView ("kate_private_plugin_katebuildplugin",
                Kate::MainWindow::Bottom,
                SmallIcon("application-x-ms-dos-executable"),
                i18n("Build Output"))
               )
    , m_proc(0)
    // NOTE this will not allow spaces in file names.
    , m_filenameDetector("([a-np-zA-Z]:[\\\\/])?[a-zA-Z0-9_\\.\\-/\\\\]+\\.[a-zA-Z0-9]+:[0-9]+"),
    m_newDirDetector("make\\[.+\\]: .+ `.*'")
{
    m_win=mw;

    KAction *a = actionCollection()->addAction("run_make");
    a->setText(i18n("Build default target"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotMake()));

    a = actionCollection()->addAction("make_clean");
    a->setText(i18n("Clean"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotMakeClean()));

    a = actionCollection()->addAction("quick_compile");
    a->setText(i18n("Quick Compile"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotQuickCompile()));

    a = actionCollection()->addAction("stop");
    a->setText(i18n("Stop"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotStop()));

    a = actionCollection()->addAction("select_target");
    a->setText(i18n("Build Target..."));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotSelectTarget()));

    a = actionCollection()->addAction("build_previous_target");
    a->setText(i18n("Build previous target again"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotBuildPreviousTarget()));

    a = actionCollection()->addAction("goto_next");
    a->setText(i18n("Next Error"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Right));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotNext()));

    a = actionCollection()->addAction("goto_prev");
    a->setText(i18n("Previous Error"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Left));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotPrev()));

    m_targetSelectAction = actionCollection()->add<KSelectAction>( "targets" );
    m_targetSelectAction->setText( i18n( "Targets" ) );
    connect(m_targetSelectAction, SIGNAL(triggered(int)), this, SLOT(targetSelected(int)));

    a = actionCollection()->addAction("target_next");
    a->setText(i18n("Next Target"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(targetNext()));

    QWidget *buildWidget = new QWidget(m_toolView);
    m_buildUi.setupUi(buildWidget);
    m_targetsUi = new TargetsUi(m_buildUi.ktabwidget);
    m_buildUi.ktabwidget->addTab(m_targetsUi, i18nc("Tab label", "Target Settings"));
    m_buildUi.ktabwidget->setCurrentWidget(m_targetsUi);


    connect(m_buildUi.errTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            SLOT(slotItemSelected(QTreeWidgetItem*)));

    m_buildUi.plainTextEdit->setReadOnly(true);

    connect(m_buildUi.showErrorsButton, SIGNAL(toggled(bool)), this, SLOT(slotShowErrors(bool)));
    connect(m_buildUi.showWarningsButton, SIGNAL(toggled(bool)), this, SLOT(slotShowWarnings(bool)));
    connect(m_buildUi.showOthersButton, SIGNAL(toggled(bool)), this, SLOT(slotShowOthers(bool)));

    connect(m_targetsUi->browse, SIGNAL(clicked()), this, SLOT(slotBrowseClicked()));

    connect(m_targetsUi->addButton, SIGNAL(clicked()), this, SLOT(slotAddTargetClicked()));
    connect(m_targetsUi->editButton, SIGNAL(clicked()), this, SLOT(slotEditTargetClicked()));
    connect(m_targetsUi->buildButton, SIGNAL(clicked()), this, SLOT(slotBuildTargetClicked()));
    connect(m_targetsUi->deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteTargetClicked()));
    connect(m_targetsUi->defButton, SIGNAL(clicked()), this, SLOT(slotDefTargetClicked()));
    connect(m_targetsUi->cleanButton, SIGNAL(clicked()), this, SLOT(slotCleanTargetClicked()));
    connect(m_targetsUi->targetsList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(slotEditTargetClicked()));


    // set the default values of the build settings. (I think loading a plugin should also trigger
    // a read of the session config data, but it does not)
   //m_targetsUi->buildCmds->setText("make");
   //m_targetsUi->cleanCmds->setText("make clean");
   //m_targetsUi->quickCmds->setText(DefQuickCmd);

    QCompleter* dirCompleter = new QCompleter(this);
    QStringList filter;
    dirCompleter->setModel(new QDirModel(filter, QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name, this));
    m_targetsUi->buildDir->setCompleter(dirCompleter);

    m_targetsUi->deleteButton->setEnabled(false);
    m_targetsUi->buildButton->setEnabled(false);
    m_targetsUi->editButton->setEnabled(false);
    m_targetsUi->defButton->setEnabled(false);
    m_targetsUi->cleanButton->setEnabled(false);

    m_proc = new KProcess();

    connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotProcExited(int,QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyReadStandardError()),this, SLOT(slotReadReadyStdErr()));
    connect(m_proc, SIGNAL(readyReadStandardOutput()),this, SLOT(slotReadReadyStdOut()));

    connect(m_targetsUi->targetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(targetSelected(int)));
    connect(m_targetsUi->targetCombo, SIGNAL(editTextChanged(QString)), this, SLOT(targetsChanged()));
    connect(m_targetsUi->newTarget, SIGNAL(clicked()), this, SLOT(targetNew()));
    connect(m_targetsUi->copyTarget, SIGNAL(clicked()), this, SLOT(targetCopy()));
    connect(m_targetsUi->deleteTarget, SIGNAL(clicked()), this, SLOT(targetDelete()));

    connect(mainWindow(), SIGNAL(unhandledShortcutOverride(QEvent*)),
            this, SLOT(handleEsc(QEvent*)));

    m_toolView->installEventFilter(this);

    mainWindow()->guiFactory()->addClient(this);

    // watch for project plugin view creation/deletion
    connect(mainWindow(), SIGNAL(pluginViewCreated (const QString &, Kate::PluginView *))
        , this, SLOT(slotPluginViewCreated (const QString &, Kate::PluginView *)));

    connect(mainWindow(), SIGNAL(pluginViewDeleted (const QString &, Kate::PluginView *))
        , this, SLOT(slotPluginViewDeleted (const QString &, Kate::PluginView *)));

    // update once project plugin state manually
    m_projectPluginView = mainWindow()->pluginView ("kateprojectplugin");
    slotProjectMapChanged ();
}


/******************************************************************/
KateBuildView::~KateBuildView()
{
    mainWindow()->guiFactory()->removeClient( this );
    delete m_proc;
    delete m_toolView;
}

/******************************************************************/
QWidget *KateBuildView::toolView() const
{
    return m_toolView;
}

/******************************************************************/
void KateBuildView::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    m_targetsUi->targetCombo->blockSignals(true);

    KConfigGroup cg(config, groupPrefix + ":build-plugin");
    int numTargets = cg.readEntry("NumTargets", 0);
    m_targetsUi->targetCombo->clear();
    m_targetList.clear();
    m_targetIndex = 0;
    int tmpIndex;
    if (numTargets == 0 ) {
        // either the config is empty or uses the older format
        m_targetList.append(TargetSet());

        m_targetList[0].name = "Default";
        m_targetList[0].quickCmd = cg.readEntry(QString("Quick Compile Command"), DefQuickCmd);
        m_targetList[0].defaultTarget = QString("build");
        m_targetList[0].cleanTarget = QString("clean");
        m_targetList[0].defaultDir = cg.readEntry(QString("Make Path"), QString());

        m_targetList[0].prevTarget = m_targetList[0].defaultTarget;

        m_targetList[0].targets["config"] = DefConfigCmd;
        m_targetList[0].targets["build"] = cg.readEntry(QString("Make Command"), DefBuildCmd);
        m_targetList[0].targets["clean"] = cg.readEntry(QString("Make Command"), DefCleanCmd);

        tmpIndex = 0;
    }
    else {

        for (int i=0; i<numTargets; i++) {
            m_targetList.append(TargetSet());
            m_targetList[i].name = cg.readEntry(QString("%1 Target").arg(i), QString("Target %1").arg(i+1));

            QStringList targetNames = cg.readEntry(QString("%1 Target Names").arg(i), QStringList());
            QString defaultTarget = cg.readEntry(QString("%1 Target Default").arg(i), QString());
            QString cleanTarget = cg.readEntry(QString("%1 Target Clean").arg(i), QString());
            QString quickCmd = cg.readEntry(QString("%1 QuickCmd").arg(i), DefQuickCmd);
            QString prevTarget = cg.readEntry(QString("%1 Target Previous").arg(i), defaultTarget);
            QString defaultDir = cg.readEntry(QString("%1 BuildPath").arg(i), QString());

            if (targetNames.isEmpty()) {
                m_targetList[i].quickCmd = quickCmd;
                m_targetList[i].defaultTarget = QString("build");
                m_targetList[i].cleanTarget = QString("clean");
                m_targetList[i].prevTarget = m_targetList[i].defaultTarget;
                m_targetList[i].defaultDir = defaultDir;

                m_targetList[i].targets["build"] = cg.readEntry(QString("%1 BuildCmd").arg(i), DefBuildCmd);
                m_targetList[i].targets["clean"] = cg.readEntry(QString("%1 CleanCmd").arg(i), DefCleanCmd);
            }
            else {
                for (int tn=0; tn<targetNames.size(); ++tn) {
                    const QString& targetName = targetNames.at(tn);
                    if (defaultTarget.isEmpty() && (tn==0)) {
                        defaultTarget = targetName;
                    }
                    m_targetList[i].targets[targetName] = cg.readEntry(QString("%1 BuildCmd %2").arg(i).arg(targetName), DefBuildCmd);
                }

                m_targetList[i].quickCmd = quickCmd;
                m_targetList[i].defaultTarget = defaultTarget;
                m_targetList[i].cleanTarget = cleanTarget;
                m_targetList[i].prevTarget = prevTarget;
                m_targetList[i].defaultDir = defaultDir;
            }

        }
        tmpIndex = cg.readEntry(QString("Active Target Index"), 0);
    }

    for(int i=0; i<m_targetList.size(); i++) {
        m_targetsUi->targetCombo->addItem(m_targetList[i].name);
    }

    m_targetsUi->targetCombo->blockSignals(false);

    // update the targets menu
    targetsChanged();

    // select the last active target if possible
    m_targetsUi->targetCombo->setCurrentIndex(tmpIndex);

}


/******************************************************************/
void KateBuildView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    // Ensure that all settings are saved in the list
//     targetSelected(m_targetIndex);

    KConfigGroup cg(config, groupPrefix + ":build-plugin");
    cg.writeEntry("NumTargets", m_targetList.size());
    for (int i=0; i<m_targetList.size(); i++) {

        cg.writeEntry(QString("%1 Target").arg(i), m_targetList[i].name);
        cg.writeEntry(QString("%1 Target Default").arg(i), m_targetList[i].defaultTarget);
        cg.writeEntry(QString("%1 Target Clean").arg(i), m_targetList[i].cleanTarget);
        cg.writeEntry(QString("%1 Target Previous").arg(i), m_targetList[i].prevTarget);
        cg.writeEntry(QString("%1 QuickCmd").arg(i), m_targetList[i].quickCmd);
        cg.writeEntry(QString("%1 BuildPath").arg(i), m_targetList[i].defaultDir);

        QStringList targetNames;

        for(std::map<QString, QString>::const_iterator tgtIt = m_targetList[i].targets.begin();
            tgtIt != m_targetList[i].targets.end(); ++tgtIt) {
            const QString& tgtName = tgtIt->first;
            const QString& buildCmd = tgtIt->second;

            targetNames << tgtName;

            cg.writeEntry(QString("%1 BuildCmd %2").arg(i).arg(tgtName), buildCmd);
        }

        cg.writeEntry(QString("%1 Target Names").arg(i), targetNames);
    }
    cg.writeEntry(QString("Active Target Index"), m_targetIndex);
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
    m_win->openUrl(KUrl(filename));

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
    if (message.contains("error") ||
        message.contains(i18nc("The same word as 'make' uses to mark an error.","error")) ||
        message.contains("undefined reference") ||
        message.contains(i18nc("The same word as 'ld' uses to mark an ...","undefined reference"))
       )
    {
        isError=true;
        item->setForeground(1, Qt::red);
        m_numErrors++;
        item->setHidden(!m_buildUi.showErrorsButton->isChecked());
    }
    if (message.contains("warning") ||
        message.contains(i18nc("The same word as 'make' uses to mark a warning.","warning"))
       )
    {
        isWarning=true;
        item->setForeground(1, Qt::yellow);
        m_numWarnings++;
        item->setHidden(!m_buildUi.showWarningsButton->isChecked());
    }
    item->setTextAlignment(1, Qt::AlignRight);

    // visible text
    //remove path from visible file name
    KUrl file(filename);
    item->setText(0, file.fileName());
    item->setText(1, line);
    item->setText(2, message.trimmed());

    // used to read from when activating an item
    item->setData(0, Qt::UserRole, filename);
    item->setData(1, Qt::UserRole, line);
    item->setData(2, Qt::UserRole, column);

    if ((isError==false) && (isWarning==false)) {
      item->setHidden(!m_buildUi.showOthersButton->isChecked());
    }

    item->setData(0,Qt::UserRole+1,isError);
    item->setData(0,Qt::UserRole+2,isWarning);

    // add tooltips in all columns
    // The enclosing <qt>...</qt> enables word-wrap for long error messages
    item->setData(0, Qt::ToolTipRole, filename);
    item->setData(1, Qt::ToolTipRole, QString("<qt>" + message + "</qt>"));
    item->setData(2, Qt::ToolTipRole, QString("<qt>" + message + "</qt>"));
}

/******************************************************************/
KUrl KateBuildView::docUrl()
{
    KTextEditor::View *kv = mainWindow()->activeView();
    if (!kv) {
        kDebug() << "no KTextEditor::View" << endl;
        return KUrl();
    }

    if (kv->document()->isModified()) kv->document()->save();
    return kv->document()->url();
}

/******************************************************************/
bool KateBuildView::checkLocal(const KUrl &dir)
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
        KMessageBox::sorry(0, i18n("No target set as default target."));
    }

    buildTarget(tgtSet->prevTarget, false);
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

    return buildTarget(tgtSet->defaultTarget, false);
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

    return buildTarget(tgtSet->cleanTarget, false);
}

/******************************************************************/
bool KateBuildView::slotQuickCompile()
{
    QString cmd =m_targetsUi->quickCmd->text();
    if (cmd.isEmpty()) {
        KMessageBox::sorry(0, i18n("The custom command is empty."));
        return false;
    }

    KUrl url(docUrl());
    KUrl dir = url.upUrl();// url is a file -> remove the file with upUrl()
    // Check if the command contains the file name or directory
    if (cmd.contains("%f") || cmd.contains("%d") || cmd.contains("%n")) {
        if (!checkLocal(url)) return false;

        cmd.replace("%n", QFileInfo(url.toLocalFile()).baseName());
        cmd.replace("%f", url.toLocalFile());
        cmd.replace("%d", dir.toLocalFile());
    }

    return startProcess(dir, cmd);
}

/******************************************************************/
bool KateBuildView::startProcess(const KUrl &dir, const QString &command)
{
    if (m_proc->state() != QProcess::NotRunning) {
        return false;
    }

    // clear previous runs
    m_buildUi.plainTextEdit->clear();
    m_buildUi.errTreeWidget->clear();
    m_output_lines.clear();
    m_numErrors = 0;
    m_numWarnings = 0;
    m_make_dir_stack.clear();

    // activate the output tab
    m_buildUi.ktabwidget->setCurrentIndex(1);
    mainWindow()->showToolView(m_toolView);

    // set working directory
    m_make_dir = dir;
    m_make_dir_stack.push(m_make_dir);
    m_proc->setWorkingDirectory(m_make_dir.toLocalFile(KUrl::AddTrailingSlash));
    m_proc->setShellCommand(command);
    m_proc->setOutputChannelMode(KProcess::SeparateChannels);
    m_proc->start();

    if(!m_proc->waitForStarted(500)) {
        KMessageBox::error(0, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc->exitStatus()));
        return false;
    }

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    return true;
}

/******************************************************************/
bool KateBuildView::slotStop()
{
    if (m_proc->state() != QProcess::NotRunning) {
        m_proc->terminate();
        return true;
    }
    return false;
}

/******************************************************************/
void KateBuildView::fillTargetList(const TargetSet* targetSet, QStringList* stringList) const
{
    for(std::map<QString, QString>::const_iterator tgtIt = targetSet->targets.begin(); tgtIt != targetSet->targets.end(); ++tgtIt) {
        (*stringList) << tgtIt->first;
    }
}

/******************************************************************/
void KateBuildView::slotSelectTarget() {
    SelectTargetDialog* dlg = new SelectTargetDialog(0);
    TargetSet* targetSet = currentTargetSet();
    QStringList targets;
    fillTargetList(targetSet, &targets);

    dlg->fillTargets(targets);

    int result = dlg->exec();
    if (result == QDialog::Accepted) {
        QString target = dlg->selectedTarget();
        buildTarget(target, true);
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
bool KateBuildView::buildTarget(const QString& targetName, bool keepAsPrevTarget)
{
    KUrl dir(docUrl()); // docUrl() saves the current document

    TargetSet* targetSet = currentTargetSet();

    if (targetSet == 0) {
        return false;
    }

    std::map<QString, QString>::const_iterator tgtIt = targetSet->targets.find(targetName);
    if (tgtIt == targetSet->targets.end()) {
        return false;
    }

    const QString& buildCmd = tgtIt->second;

    if (targetSet->defaultDir.isEmpty()) {
        if (!checkLocal(dir)) return false;
        // dir is a file -> remove the file with upUrl().
        dir = dir.upUrl();
    }
    else {
        dir = KUrl(targetSet->defaultDir);
    }

    if (keepAsPrevTarget) {
        targetSet->prevTarget = targetName;
    }

    return startProcess(dir, buildCmd);
}



/******************************************************************/
void KateBuildView::slotProcExited(int exitCode, QProcess::ExitStatus)
{
    QApplication::restoreOverrideCursor();

    // did we get any errors?
    if (m_numErrors || m_numWarnings || (exitCode != 0)) {
       m_buildUi.ktabwidget->setCurrentIndex(0);
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
        }
        if (m_numWarnings) {
            msgs << i18np("Found one warning.", "Found %1 warnings.", m_numWarnings);
        }
        KPassivePopup::message(i18n("Make Results"), msgs.join("\n"), m_toolView);
    }
    else if (exitCode != 0) {
        KPassivePopup::message(i18n("Make Results"), i18n("Build failed."), m_toolView);
    }
    else {
        KPassivePopup::message(i18n("Make Results"), i18n("Build completed without problems."), m_toolView);
    }

}


/******************************************************************/
void KateBuildView::slotReadReadyStdOut()
{
    // read data from procs stdout and add
    // the text to the end of the output
    // FIXME This works for utf8 but not for all charsets
    QString l= QString::fromUtf8(m_proc->readAllStandardOutput());
    l.remove('\r');
    m_output_lines += l;

    QString tmp;

    int end=0;


    // handle one line at a time
    do {
        end = m_output_lines.indexOf('\n');
        if (end < 0) break;
        end++;
        tmp = m_output_lines.mid(0, end);
        tmp.remove('\n');
        m_buildUi.plainTextEdit->appendPlainText(tmp);
        //kDebug() << tmp;
        if (tmp.indexOf(m_newDirDetector) >=0) {
            //kDebug() << "Enter/Exit dir found";
            int open = tmp.indexOf("`");
            int close = tmp.indexOf("'");
            KUrl newDir = KUrl(tmp.mid(open+1, close-open-1));
            kDebug () << "New dir = " << newDir;

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
    l.remove('\r');
    m_output_lines += l;

    QString tmp;

    int end=0;

    do {
        end = m_output_lines.indexOf('\n');
        if (end < 0) break;
        end++;
        tmp = m_output_lines.mid(0, end);
        tmp.remove('\n');
        m_buildUi.plainTextEdit->appendPlainText(tmp);

        processLine(tmp);

        m_output_lines.remove(0,end);

    } while (1);

}

/******************************************************************/
void KateBuildView::processLine(const QString &line)
{
    QString l = line;
    //kDebug() << l ;

    //look for a filename
    if (l.indexOf(m_filenameDetector)<0)
    {
        addError(QString(), 0, QString(), l);
        //kDebug() << "A filename was not found in the line ";
        return;
    }

    int match_start = m_filenameDetector.indexIn(l, 0);
    int match_len = m_filenameDetector.matchedLength();

    QString file_n_line = l.mid(match_start, match_len);

    int name_end = file_n_line.lastIndexOf(':');
    QString filename = file_n_line.left(name_end);
    QString line_n = file_n_line.mid(name_end+1);
    QString msg = l.remove(m_filenameDetector);

    //kDebug() << "File Name:"<<filename<< " msg:"<< msg;
    //add path to file
    if (QFile::exists(m_make_dir.toLocalFile(KUrl::AddTrailingSlash)+filename)) {
        filename = m_make_dir.toLocalFile(KUrl::AddTrailingSlash)+filename;
    }

    // Now we have the data we need show the error/warning
    addError(filename, line_n, QString(), msg);

}

/******************************************************************/
void KateBuildView::slotBrowseClicked()
{
    KUrl defDir(m_targetsUi->buildDir->text());

    if (m_targetsUi->buildDir->text().isEmpty()) {
        // try current document dir
        KTextEditor::View *kv = mainWindow()->activeView();
        if (kv != 0) {
            defDir = kv->document()->url();
        }
    }

    QString newDir = KFileDialog::getExistingDirectory(defDir, 0, QString());
    if (!newDir.isEmpty()) {
        m_targetsUi->buildDir->setText(newDir);
    }
}

/******************************************************************/
void KateBuildView::slotAddTargetClicked()
{
    TargetSet* targetSet = currentTargetSet();
    if (targetSet == 0) {
        return;
    }

    QStringList targets;
    fillTargetList(targetSet, &targets);

    EditTargetDialog* dlg = new EditTargetDialog(targets);
    dlg->setCaption(i18n("Add new target"));
    int result = dlg->exec();
    if (result == QDialog::Accepted) {
        QString name = dlg->targetName();
        QString buildCmd = dlg->targetCommand();

        m_targetList[m_targetIndex].targets[name] = buildCmd;

        QTreeWidgetItem* item = new QTreeWidgetItem();
        m_targetsUi->targetsList->addTopLevelItem(item);

        setTargetItemContents(item, m_targetList[m_targetIndex], name, buildCmd);

        m_targetsUi->deleteButton->setEnabled(true);
        m_targetsUi->buildButton->setEnabled(true);
        m_targetsUi->editButton->setEnabled(true);
        m_targetsUi->defButton->setEnabled(true);
        m_targetsUi->cleanButton->setEnabled(true);
    }
    delete dlg;
    dlg = 0;

}

/******************************************************************/
void KateBuildView::slotEditTargetClicked()
{
    TargetSet* targetSet = currentTargetSet();
    if (targetSet == 0) {
        return;
    }

    QTreeWidgetItem* currentItem = m_targetsUi->targetsList->currentItem();
    if (currentItem == 0) {
        return;
    }

    QStringList targets;
    fillTargetList(targetSet, &targets);

    QString oldName = currentItem->text(2);
    EditTargetDialog* dlg = new EditTargetDialog(targets);
    dlg->setCaption(QString(i18n("Edit target %1")).arg(oldName));
    dlg->setTargetName(oldName);
    dlg->setTargetCommand(currentItem->text(3));
    int result = dlg->exec();
    if (result == QDialog::Accepted) {
        QString newName = dlg->targetName();
        QString buildCmd = dlg->targetCommand();
        m_targetList[m_targetIndex].targets.erase(oldName);
        m_targetList[m_targetIndex].targets[newName] = buildCmd;

        setTargetItemContents(currentItem, m_targetList[m_targetIndex], newName, buildCmd);

    }
    delete dlg;
    dlg = 0;
}

/******************************************************************/
void KateBuildView::slotBuildTargetClicked()
{
    const QTreeWidgetItem* currentItem = m_targetsUi->targetsList->currentItem();
    if (currentItem == 0) {
        return;
    }

    QString target = currentItem->text(2);
    buildTarget(target, true);
}

/******************************************************************/
void KateBuildView::slotDeleteTargetClicked()
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    QTreeWidgetItem* item = m_targetsUi->targetsList->currentItem();
    if (item == 0) {
        return;
    }

    QString target = item->text(2);

    int result = KMessageBox::questionYesNo(0, QString(i18n("Really delete target %1 ?")).arg(target));
    if (result == KMessageBox::No) {
        return;
    }

    delete item;
    item = 0;

    if (tgtSet->cleanTarget == target) {
        tgtSet->cleanTarget = QString("");
    }

    if (tgtSet->defaultTarget == target) {
        tgtSet->defaultTarget = QString("");
    }

    tgtSet->targets.erase(target);

    bool enableButtons = (m_targetsUi->targetsList->topLevelItemCount() > 0);
    m_targetsUi->deleteButton->setEnabled(enableButtons);
    m_targetsUi->buildButton->setEnabled(enableButtons);
    m_targetsUi->editButton->setEnabled(enableButtons);
    m_targetsUi->defButton->setEnabled(enableButtons);
    m_targetsUi->cleanButton->setEnabled(enableButtons);
}

/******************************************************************/
void KateBuildView::slotDefTargetClicked()
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    QTreeWidgetItem* currentItem = m_targetsUi->targetsList->currentItem();
    if (currentItem == 0) {
        return;
    }

    QString target = currentItem->text(2);

    tgtSet->defaultTarget = target;
    for(int i=0; i<m_targetsUi->targetsList->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = m_targetsUi->targetsList->topLevelItem(i);
        item->setText(0, item->text(2) == target ? "X" : "");
    }
}

/******************************************************************/
void KateBuildView::slotCleanTargetClicked()
{
    TargetSet* tgtSet = currentTargetSet();
    if (tgtSet == 0) {
        return;
    }

    QTreeWidgetItem* currentItem = m_targetsUi->targetsList->currentItem();
    if (currentItem == 0) {
        return;
    }

    QString target = currentItem->text(2);

    tgtSet->cleanTarget = target;
    for(int i=0; i<m_targetsUi->targetsList->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = m_targetsUi->targetsList->topLevelItem(i);
        item->setText(1, item->text(2) == target ? "X" : "");
    }
}

/******************************************************************/
void KateBuildView::targetSelected(int index)
{
    if (index >= m_targetList.size() || (index < 0)) {
        kDebug() << "Invalid target";
        return;
    }


    // Set the new values
    m_targetsUi->buildDir->setText(m_targetList[index].defaultDir);
    m_targetsUi->quickCmd->setText(m_targetList[index].quickCmd);

    m_targetsUi->targetsList->clear();

    for(std::map<QString, QString>::const_iterator it = m_targetList[index].targets.begin(); it != m_targetList[index].targets.end(); ++it) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        m_targetsUi->targetsList->addTopLevelItem(item);
        setTargetItemContents(item, m_targetList[index], it->first, it->second);
        if (m_targetsUi->targetsList->topLevelItemCount() == 1) {
            m_targetsUi->targetsList->setCurrentItem(item);
        }
    }

    for(int i=0; i<4; i++) {
        m_targetsUi->targetsList->resizeColumnToContents(i);
    }

    m_targetIndex = index;

    // make sure that both the combo box and the menu are updated
    m_targetsUi->targetCombo->setCurrentIndex(index);
    m_targetSelectAction->setCurrentItem(index);

    const bool enableButtons = (m_targetsUi->targetsList->currentItem() != 0);

    m_targetsUi->deleteButton->setEnabled(enableButtons);
    m_targetsUi->buildButton->setEnabled(enableButtons);
    m_targetsUi->editButton->setEnabled(enableButtons);
    m_targetsUi->defButton->setEnabled(enableButtons);
    m_targetsUi->cleanButton->setEnabled(enableButtons);
}


/******************************************************************/
void KateBuildView::setTargetItemContents(QTreeWidgetItem* item, const TargetSet& tgtSet, const QString& name, const QString& buildCmd)
{
    item->setText(0, (name == tgtSet.defaultTarget) ? QString("X") : QString (""));
    item->setText(1, (name == tgtSet.cleanTarget) ? QString("X") : QString (""));
    item->setText(2, name);
    item->setText(3, buildCmd);
}


/******************************************************************/
void KateBuildView::targetsChanged()
{
    QStringList items;

    for( int i = 0; i < m_targetList.size(); ++i ) {
        items.append( m_targetList[i].name );
    }
    m_targetSelectAction->setItems( items );
    m_targetSelectAction->setCurrentItem( m_targetIndex );
}

/******************************************************************/
void KateBuildView::targetNew()
{
    m_targetList.append(TargetSet());
    m_targetIndex = m_targetList.size()-1;
    m_targetList[m_targetIndex].name = i18n("Target %1", m_targetList.size());
    m_targetList[m_targetIndex].quickCmd = DefQuickCmd;
    m_targetList[m_targetIndex].defaultTarget = "Build";
    m_targetList[m_targetIndex].prevTarget = "Build";
    m_targetList[m_targetIndex].cleanTarget = "Clean";
    m_targetList[m_targetIndex].defaultDir = QString();

    m_targetList[m_targetIndex].targets[QString("Build")] = DefBuildCmd;
    m_targetList[m_targetIndex].targets[QString("Clean")] = DefCleanCmd;
    m_targetList[m_targetIndex].targets[QString("Config")] = DefConfigCmd;

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
    m_targetList[m_targetIndex].name = i18n("Target %1", m_targetList.size());

    m_targetsUi->targetCombo->addItem(m_targetList[m_targetIndex].name);
    m_targetsUi->targetCombo->setCurrentIndex(m_targetIndex);
    m_targetsUi->deleteTarget->setDisabled(false);

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
        newTargetIndex = m_targetIndex - 1;

    }
    else {
        m_targetsUi->targetCombo->clear();
        m_targetList.clear();

        m_targetList.append(TargetSet());
        m_targetList[0].name = i18n("Target");
        m_targetList[0].quickCmd = DefQuickCmd;
        m_targetList[0].defaultTarget = "Build";
        m_targetList[0].cleanTarget = "Clean";
        m_targetList[0].prevTarget = m_targetList[0].defaultTarget;
        m_targetList[0].defaultDir = QString();

        m_targetList[0].targets["Build"] = DefBuildCmd;
        m_targetList[0].targets["Clean"] = DefCleanCmd;
        m_targetList[0].targets["Config"] = DefConfigCmd;
        m_targetList[0].targets["ConfigClean"] = DefConfClean;

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
    if (m_toolView->isVisible() && m_buildUi.ktabwidget->currentIndex() == 2) {
        int index = m_targetsUi->targetCombo->currentIndex();
        index++;
        if (index == m_targetsUi->targetCombo->count()) index = 0;

        m_targetsUi->targetCombo->setCurrentIndex(index);
    }
    else {
        m_win->showToolView(m_toolView);
        m_buildUi.ktabwidget->setCurrentIndex(2);
    }
}

/******************************************************************/
bool KateBuildView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if ((obj == m_toolView) && (ke->key() == Qt::Key_Escape)) {
            mainWindow()->hideToolView(m_toolView);
            event->accept();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

/******************************************************************/
void KateBuildView::handleEsc(QEvent *e)
{
    if (!mainWindow()) return;

    QKeyEvent *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        if (m_toolView->isVisible()) {
            mainWindow()->hideToolView(m_toolView);
        }
    }
}


/******************************************************************/
void KateBuildView::slotShowErrors(bool showItems) {
    QTreeWidget *tree=m_buildUi.errTreeWidget;
    const int itemCount = tree->topLevelItemCount();

    for (int i=0;i<itemCount;i++) {
        QTreeWidgetItem* item=tree->topLevelItem(i);
        if (item->data(0,Qt::UserRole+1).toBool()==true) {
            item->setHidden(!showItems);
        }
    }
}

/******************************************************************/
void KateBuildView::slotShowWarnings(bool showItems) {
    QTreeWidget *tree=m_buildUi.errTreeWidget;
    const int itemCount = tree->topLevelItemCount();

    for (int i=0;i<itemCount;i++) {
        QTreeWidgetItem* item=tree->topLevelItem(i);
        if (item->data(0,Qt::UserRole+2).toBool()==true) {
            item->setHidden(!showItems);
        }
    }
}

/******************************************************************/
void KateBuildView::slotShowOthers(bool showItems) {
    QTreeWidget *tree=m_buildUi.errTreeWidget;
    const int itemCount = tree->topLevelItemCount();

    for (int i=0;i<itemCount;i++) {
        QTreeWidgetItem* item=tree->topLevelItem(i);
        if ( (item->data(0,Qt::UserRole+1).toBool()==false) && (item->data(0,Qt::UserRole+2).toBool()==false) ) {
            item->setHidden(!showItems);
        }
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewCreated (const QString &name, Kate::PluginView *pluginView)
{
    // add view
    if (name == "kateprojectplugin") {
        m_projectPluginView = pluginView;
        slotProjectMapChanged ();
        connect (pluginView, SIGNAL(projectMapChanged()), this, SLOT(slotProjectMapChanged()));
    }
}

/******************************************************************/
void KateBuildView::slotPluginViewDeleted (const QString &name, Kate::PluginView *)
{
    // remove view
    if (name == "kateprojectplugin") {
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
    QVariantMap buildMap = projectMap.value("build").toMap();
    if (buildMap.isEmpty()) {
        return;
    }

    int index = m_targetList.size();
    m_targetList.append(TargetSet());
    m_targetList[index].name = i18n("Project Plugin Target");
    m_targetList[index].cleanTarget = buildMap.value("clean_target").toString();
    m_targetList[index].defaultTarget = buildMap.value("default_target").toString();
    m_targetList[index].prevTarget = buildMap.value("default_target").toString();
    m_targetList[index].quickCmd = buildMap.value("quick_cmd").toString();
    m_targetList[index].defaultDir = buildMap.value("directory").toString();

    // get build dir
    QString defaultBuildDir = buildMap.value("directory").toString();
//     QDir dir (QFileInfo (m_projectPluginView->property("projectFileName").toString()).absoluteDir());
//     if (dir.cd (buildMap.value("directory").toString())) {
//         m_targetsUi->buildDir->setText(dir.absolutePath());
//     }

    QVariantList targets = buildMap.value("targets").toList();
    foreach (const QVariant &targetVariant, targets) {
        QVariantMap targetMap = targetVariant.toMap();
        QString tgtName = targetMap["name"].toString();
        QString buildCmd = targetMap["build_cmd"].toString();

        if (tgtName.isEmpty() || buildCmd.isEmpty()) {
            continue;
        }

        m_targetList[index].targets[tgtName] = buildCmd;
    }

    m_targetsUi->targetCombo->addItem(m_targetList[index].name);
    m_targetsUi->deleteTarget->setDisabled(true); // false);

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
        if (m_targetList[i].name == i18n("Project Plugin Target")) {
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


// kate: space-indent on; indent-width 4; replace-tabs on;
