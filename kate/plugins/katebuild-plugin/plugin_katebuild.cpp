/* plugin_katebuild.c                    Kate Plugin
**
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
#include <qfileinfo.h>
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

K_PLUGIN_FACTORY(KateBuildPluginFactory, registerPlugin<KateBuildPlugin>();)
K_EXPORT_PLUGIN(KateBuildPluginFactory(KAboutData("katebuild",
                                                  "katebuild-plugin",
                                                  ki18n("Build Plugin"),
                                                  "0.1",
                                                  ki18n( "Build Plugin"))))

static const QString DefConfigCmd = "cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ../";
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
    m_targetList.append(Target());

    m_win=mw;

    KAction *a = actionCollection()->addAction("run_make");
    a->setText(i18n("Build"));
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

    QWidget *buildWidget = new QWidget(m_toolView);
    m_buildUi.setupUi(buildWidget);
    m_targetsUi = new TargetsUi(m_buildUi.ktabwidget);
    m_buildUi.ktabwidget->addTab(m_targetsUi, i18nc("Tab label", "Target Settings"));
    m_buildUi.ktabwidget->setCurrentWidget(m_targetsUi);


    connect(m_buildUi.errTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            SLOT(slotItemSelected(QTreeWidgetItem *)));

    m_buildUi.plainTextEdit->setReadOnly(true);

    connect(m_targetsUi->browse, SIGNAL(clicked()), this, SLOT(slotBrowseClicked()));

    // set the default values of the build settings. (I think loading a plugin should also trigger
    // a read of the session config data, but it does not)
   //m_targetsUi->buildCmds->setText("make");
   //m_targetsUi->cleanCmds->setText("make clean");
   //m_targetsUi->quickCmds->setText(DefQuickCmd);

    QCompleter* dirCompleter = new QCompleter(this);
    QStringList filter;
    dirCompleter->setModel(new QDirModel(filter, QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name, this));
    m_targetsUi->buildDir->setCompleter(dirCompleter);

    m_proc = new KProcess();

    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcExited(int, QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyReadStandardError()),this, SLOT(slotReadReadyStdErr()));
    connect(m_proc, SIGNAL(readyReadStandardOutput()),this, SLOT(slotReadReadyStdOut()));

    connect(m_targetsUi->targetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(targetSelected(int)));
    connect(m_targetsUi->targetCombo, SIGNAL(editTextChanged(QString)), this, SLOT(targetsChanged(QString)));
    connect(m_targetsUi->newTarget, SIGNAL(clicked()), this, SLOT(targetNew()));
    connect(m_targetsUi->copyTarget, SIGNAL(clicked()), this, SLOT(targetCopy()));
    connect(m_targetsUi->deleteTarget, SIGNAL(clicked()), this, SLOT(targetDelete()));

    mainWindow()->guiFactory()->addClient(this);
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
    if (numTargets == 0 ) {
        // either the config is empty or uses the older format
        m_targetList.append(Target());
        m_targetList[0].name = cg.readEntry(QString(""), QString("Target 1"));
        m_targetsUi->targetCombo->addItem(m_targetList[0].name);
        m_targetList[0].buildDir = cg.readEntry(QString("Make Path"), QString());
        m_targetList[0].buildCmds << cg.readEntry(QString("Make Command"), DefBuildCmd);
        m_targetList[0].buildCmdIndex = 0;
        m_targetList[0].cleanCmds << cg.readEntry(QString("Clean Command"), DefCleanCmd);
        m_targetList[0].cleanCmdIndex = 0;
        m_targetList[0].quickCmds << cg.readEntry(QString("Quick Compile Command"), DefQuickCmd);
        m_targetList[0].quickCmdIndex = 0;
    }
    else {
        int size;
        QString tmp;
        for (int i=0; i<numTargets; i++) {
            m_targetList.append(Target());
            m_targetList[i].name = cg.readEntry(QString("%1 Target").arg(i), QString("Target %1").arg(i+1));
            m_targetsUi->targetCombo->addItem(m_targetList[i].name);
            m_targetList[i].buildDir =      cg.readEntry(QString("%1 BuildPath").arg(i), QString());

            // Build CMD
            size = cg.readEntry(QString("%1 BuildCmdCount").arg(i), 0);
            for (int j=0; j<size; j++) {
                if (j==0) tmp = DefConfigCmd;
                else if (j==1) tmp = DefBuildCmd;
                else tmp.clear();
                m_targetList[i].buildCmds << cg.readEntry(QString("%1 %2 BuildCmd").arg(i).arg(j), tmp);
            }
            m_targetList[i].buildCmdIndex = cg.readEntry(QString("%1 BuildCmdIndex").arg(i), 0);

            // Clean CMD
            size = cg.readEntry(QString("%1 CleanCmdCount").arg(i), 0);
            for (int j=0; j<size; j++) {
                tmp = (j==0) ? DefCleanCmd : "";
                m_targetList[i].cleanCmds << cg.readEntry(QString("%1 %2 CleanCmd").arg(i).arg(j), tmp);
            }
            m_targetList[i].cleanCmdIndex = cg.readEntry(QString("%1 CleanCmdIndex").arg(i), 0);

            // Quick CMD
            size = cg.readEntry(QString("%1 QuickCmdCount").arg(i), 0);
            for (int j=0; j<size; j++) {
                tmp = (j==0) ? DefQuickCmd : "";
                m_targetList[i].quickCmds << cg.readEntry(QString("%1 %2 QuickCmd").arg(i).arg(j), tmp);
            }
            m_targetList[i].quickCmdIndex = cg.readEntry(QString("%1 QuickCmdIndex").arg(i), 0);
        }
    }
    m_targetsUi->buildDir->setText(m_targetList[0].buildDir);
    m_targetsUi->buildCmds->clear();
    m_targetsUi->buildCmds->addItems(m_targetList[0].buildCmds);
    m_targetsUi->buildCmds->setCurrentIndex(m_targetList[0].buildCmdIndex);

    m_targetsUi->cleanCmds->clear();
    m_targetsUi->cleanCmds->addItems(m_targetList[0].cleanCmds);
    m_targetsUi->cleanCmds->setCurrentIndex(m_targetList[0].cleanCmdIndex);

    m_targetsUi->quickCmds->clear();
    m_targetsUi->quickCmds->addItems(m_targetList[0].quickCmds);
    m_targetsUi->quickCmds->setCurrentIndex(m_targetList[0].quickCmdIndex);

    m_targetsUi->targetCombo->blockSignals(false);
    if (numTargets > 1)  {
       m_targetsUi->deleteTarget->setDisabled(false);
    }
    else {
       m_targetsUi->deleteTarget->setDisabled(true);
    }

    // select the last active target if possible
    m_targetsUi->targetCombo->setCurrentIndex(cg.readEntry(QString("Active Target Index"), 0));

}

/******************************************************************/
void KateBuildView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    // Ensure that all settings are saved in the list
    targetSelected(m_targetIndex);

    KConfigGroup cg(config, groupPrefix + ":build-plugin");
    cg.writeEntry("NumTargets", m_targetList.size());
    for (int i=0; i<m_targetList.size(); i++) {
        cg.writeEntry(QString("%1 Target").arg(i), m_targetList[i].name);
        cg.writeEntry(QString("%1 BuildPath").arg(i), m_targetList[i].buildDir);
        for (int j=0; j<m_targetList[i].buildCmds.size(); j++) {
            cg.writeEntry(QString("%1 %2 BuildCmd").arg(i).arg(j), m_targetList[i].buildCmds[j]);
        }
        cg.writeEntry(QString("%1 BuildCmdCount").arg(i), m_targetList[i].buildCmds.size());
        cg.writeEntry(QString("%1 BuildCmdIndex").arg(i), m_targetList[i].buildCmdIndex);
        for (int j=0; j<m_targetList[i].cleanCmds.size(); j++) {
            cg.writeEntry(QString("%1 %2 CleanCmd").arg(i).arg(j), m_targetList[i].cleanCmds[j]);
        }
        cg.writeEntry(QString("%1 CleanCmdCount").arg(i), m_targetList[i].cleanCmds.size());
        cg.writeEntry(QString("%1 CleanCmdIndex").arg(i), m_targetList[i].cleanCmdIndex);
        for (int j=0; j<m_targetList[i].quickCmds.size(); j++) {
            cg.writeEntry(QString("%1 %2 QuickCmd").arg(i).arg(j), m_targetList[i].quickCmds[j]);
        }
        cg.writeEntry(QString("%1 QuickCmdCount").arg(i), m_targetList[i].quickCmds.size());
        cg.writeEntry(QString("%1 QuickCmdIndex").arg(i), m_targetList[i].quickCmdIndex);
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

    int i = (item == 0) ? -1 : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (++i < itemCount) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty()) {
            m_buildUi.errTreeWidget->setCurrentItem(item);
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

    int i = (item == 0) ? itemCount : m_buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (--i >= 0) {
        item = m_buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty()) {
            m_buildUi.errTreeWidget->setCurrentItem(item);
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
    QTreeWidgetItem* item = new QTreeWidgetItem(m_buildUi.errTreeWidget);
    item->setBackground(1, Qt::gray);
    // The strings are twice in case kate is translated but not make.
    if (message.contains("error") ||
        message.contains(i18nc("The same word as 'make' uses to mark an error.","error")) ||
        message.contains("undefined reference") ||
        message.contains(i18nc("The same word as 'ld' uses to mark an ...","undefined reference"))
       )
    {
        item->setForeground(1, Qt::red);
        m_numErrors++;
    }
    if (message.contains("warning") ||
        message.contains(i18nc("The same word as 'make' uses to mark a warning.","warning"))
       )
    {
        item->setForeground(1, Qt::yellow);
        m_numWarnings++;
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

    // add tooltips in all columns
    // The enclosing <qt>...</qt> enables word-wrap for long error messages
    item->setData(0, Qt::ToolTipRole, filename);
    item->setData(1, Qt::ToolTipRole, "<qt>" + message + "</qt>");
    item->setData(2, Qt::ToolTipRole, "<qt>" + message + "</qt>");
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
bool KateBuildView::slotMake(void)
{
    KUrl dir(docUrl()); // docUrl() saves the current document

    if (m_targetsUi->buildDir->text().isEmpty()) {
        if (!checkLocal(dir)) return false;
        // dir is a file -> remove the file with upUrl().
        dir = dir.upUrl();
    }
    else {
        dir = KUrl(m_targetsUi->buildDir->text());
    }
    return startProcess(dir, m_targetsUi->buildCmds->currentText());
}

/******************************************************************/
bool KateBuildView::slotMakeClean(void)
{
    KUrl dir(docUrl()); // docUrl() saves the current document

    if (m_targetsUi->buildDir->text().isEmpty()) {
        if (!checkLocal(dir)) return false;
        // dir is a file -> remove the file with upUrl().
        dir = dir.upUrl();
    }
    else {
        dir = KUrl(m_targetsUi->buildDir->text());
    }

    return startProcess(dir, m_targetsUi->cleanCmds->currentText());
}

/******************************************************************/
bool KateBuildView::slotQuickCompile()
{
    QString cmd =m_targetsUi->quickCmds->currentText();
    if (cmd.isEmpty()) {
        KMessageBox::sorry(0, i18n("The custom command is empty."));
        return false;
    }

    KUrl url(docUrl());
    KUrl dir = url.upUrl();// url is a file -> remove the file with upUrl()
    // Check if the command contains the file name or directory
    if (cmd.contains("%f") || cmd.contains("%d")) {
        if (!checkLocal(url)) return false;

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
    } else {
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

   m_targetsUi->buildDir->setText(KFileDialog::getExistingDirectory(defDir, 0, QString()));
}

/******************************************************************/
void KateBuildView::targetSelected(int index)
{
    if (index >= m_targetList.size() || (index < 0)) {
        kDebug() << "Invalid target";
        return;
    }

    if (m_targetIndex >= m_targetList.size() || (m_targetIndex < 0)) {
        kDebug() << "Invalid m_targetIndex";
        return;
    }

    // save the previous settings
    m_targetList[m_targetIndex].name = m_targetsUi->targetCombo->itemText(m_targetIndex);

    m_targetList[m_targetIndex].buildDir = m_targetsUi->buildDir->text();

    m_targetList[m_targetIndex].buildCmds.clear();
    m_targetList[m_targetIndex].buildCmdIndex = m_targetsUi->buildCmds->currentIndex();
    for (int i=0; i<m_targetsUi->buildCmds->count(); i++) {
        m_targetList[m_targetIndex].buildCmds << m_targetsUi->buildCmds->itemText(i);
    }

    m_targetList[m_targetIndex].cleanCmds.clear();
    m_targetList[m_targetIndex].cleanCmdIndex = m_targetsUi->cleanCmds->currentIndex();
    for (int i=0; i<m_targetsUi->cleanCmds->count(); i++) {
        m_targetList[m_targetIndex].cleanCmds << m_targetsUi->cleanCmds->itemText(i);
    }
    m_targetList[m_targetIndex].quickCmds.clear();
    m_targetList[m_targetIndex].quickCmdIndex = m_targetsUi->quickCmds->currentIndex();
    for (int i=0; i<m_targetsUi->quickCmds->count(); i++) {
        m_targetList[m_targetIndex].quickCmds << m_targetsUi->quickCmds->itemText(i);
    }

    // Set the new values
    m_targetsUi->buildDir->setText(m_targetList[index].buildDir);

    m_targetsUi->buildCmds->clear();
    m_targetsUi->buildCmds->addItems(m_targetList[index].buildCmds);
    m_targetsUi->buildCmds->setCurrentIndex(m_targetList[index].buildCmdIndex);

    m_targetsUi->cleanCmds->clear();
    m_targetsUi->cleanCmds->addItems(m_targetList[index].cleanCmds);
    m_targetsUi->cleanCmds->setCurrentIndex(m_targetList[index].cleanCmdIndex);

    m_targetsUi->quickCmds->clear();
    m_targetsUi->quickCmds->addItems(m_targetList[index].quickCmds);
    m_targetsUi->quickCmds->setCurrentIndex(m_targetList[index].quickCmdIndex);

    m_targetIndex = index;

    // make sure that both the combo box and the menu are updated
    m_targetsUi->targetCombo->setCurrentIndex( index );
    m_targetSelectAction->setCurrentItem( index );
}

void KateBuildView::targetsChanged(QString)
{
    QStringList items;

    for( int i = 0; i < m_targetList.size(); ++i )
    {
        items.append( m_targetList[i].name );
    }
    m_targetSelectAction->setItems( items );
    m_targetSelectAction->setCurrentItem( m_targetIndex );
}

/******************************************************************/
void KateBuildView::targetNew()
{
    QStringList build; build << DefConfigCmd << DefBuildCmd;;

    m_targetList.append(Target());
    m_targetsUi->targetCombo->addItem(i18n("Target %1", m_targetList.size()));
    // Set the new defult values
    m_targetsUi->buildDir->setText(QString());
    m_targetsUi->buildCmds->clear();
    m_targetsUi->buildCmds->addItems(build);
    m_targetsUi->cleanCmds->clear();
    m_targetsUi->cleanCmds->addItem(DefCleanCmd);
    m_targetsUi->quickCmds->clear();
    m_targetsUi->quickCmds->addItem(DefQuickCmd);
    m_targetIndex = m_targetList.size()-1;
    m_targetsUi->targetCombo->setCurrentIndex(m_targetIndex);
    m_targetsUi->deleteTarget->setDisabled(false);
}

/******************************************************************/
void KateBuildView::targetCopy()
{
    m_targetList.append(Target());
    m_targetsUi->targetCombo->addItem(i18n("Target %1", m_targetList.size()));
    m_targetIndex = m_targetList.size() -1;
    m_targetsUi->targetCombo->setCurrentIndex(m_targetIndex);
    m_targetsUi->deleteTarget->setDisabled(false);
}

/******************************************************************/
void KateBuildView::targetDelete()
{
    m_targetsUi->targetCombo->blockSignals(true);

    m_targetsUi->targetCombo->removeItem(m_targetIndex);
    m_targetList.removeAt(m_targetIndex);

    m_targetIndex =m_targetsUi->targetCombo->currentIndex();
    // Set the new values
    m_targetsUi->buildDir->setText(m_targetList[m_targetIndex].buildDir);
    m_targetsUi->buildCmds->clear();
    m_targetsUi->buildCmds->addItems(m_targetList[m_targetIndex].buildCmds);
    m_targetsUi->cleanCmds->clear();
    m_targetsUi->cleanCmds->addItems(m_targetList[m_targetIndex].cleanCmds);
    m_targetsUi->quickCmds->clear();
    m_targetsUi->quickCmds->addItems(m_targetList[m_targetIndex].quickCmds);

    if (m_targetList.size() == 1) {
        m_targetsUi->deleteTarget->setDisabled(true);
    }
    m_targetsUi->targetCombo->blockSignals(false);
}

/******************************************************************/
void KateBuildView::targetNext()
{
    int index = m_targetsUi->targetCombo->currentIndex();
    index++;
    if (index == m_targetsUi->targetCombo->count()) index = 0;

    m_targetsUi->targetCombo->setCurrentIndex(index);

    m_win->showToolView(m_toolView);
    m_buildUi.ktabwidget->setCurrentIndex(2);

}

