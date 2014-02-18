/* This file is part of the KDE project
   Copyright (C) 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "replicodeplugin.h"
#include "replicodeconfigpage.h"
#include "replicodesettings.h"
#include "replicodeconfig.h"

#include <QtGlobal>
#include <QProcess>
#include <QTemporaryFile>
#include <QPushButton>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>
#include <KAboutData>
#include <kate/application.h>
#include <QDebug>
#include <qfileinfo.h>
#include <kate/mainwindow.h>
#include <QMessageBox>
#include <KGlobal>
#include <KConfigGroup>
#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <QListWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QProcess>

K_PLUGIN_FACTORY(KateReplicodeFactory, registerPlugin<ReplicodePlugin>();)
K_EXPORT_PLUGIN(KateReplicodeFactory("katereplicodeplugin"))

ReplicodePlugin::ReplicodePlugin(QObject* parent, const QList< QVariant > &args)
: Kate::Plugin(qobject_cast<Kate::Application*>(parent))
{
    Q_UNUSED(args);
}

ReplicodePlugin::~ReplicodePlugin()
{
}
uint ReplicodePlugin::configPages() const
{
    return 1;
}

QString ReplicodePlugin::configPageFullName(uint number) const
{
    Q_ASSERT(number == 0);
    Q_UNUSED(number);
    return i18n("Replicode settings");
}

QString ReplicodePlugin::configPageName(uint number) const
{
    Q_ASSERT(number == 0);
    Q_UNUSED(number);
    return i18n("Replicode");
}


KIcon ReplicodePlugin::configPageIcon(uint number) const
{
    Q_UNUSED(number);
    return KIcon("code-block");
}

Kate::PluginConfigPage* ReplicodePlugin::configPage(uint number, QWidget* parent, const char* name)
{
    Q_UNUSED(name);
    Q_UNUSED(number);
    Q_ASSERT(number == 0);
    return new ReplicodeConfigPage(parent, name);
}

ReplicodeView::ReplicodeView(Kate::MainWindow* mainWindow)
: Kate::PluginView(mainWindow), Kate::XMLGUIClient(KateReplicodeFactory::componentData()), m_mainWindow(mainWindow), m_settingsFile(0), m_executor(0)
{
    m_runAction = new KAction(KIcon("code-block"), i18n("Run replicode"), this);
    m_runAction->setShortcut(Qt::Key_F8);
    connect(m_runAction, SIGNAL(triggered()), SLOT(runReplicode()));
    actionCollection()->addAction("katereplicode_run", m_runAction);

    m_stopAction = new KAction(KIcon("process-stop"), i18n("Stop replicode"), this);
    m_stopAction->setShortcut(Qt::Key_F9);
    connect(m_stopAction, SIGNAL(triggered()), SLOT(stopReplicode()));
    actionCollection()->addAction("katereplicode_stop", m_stopAction);
    m_stopAction->setEnabled(false);

    m_toolview = mainWindow->createToolView("kate_private_plugin_katereplicodeplugin_run", Kate::MainWindow::Bottom, SmallIcon("code-block"), i18n("Replicode Output"));
    m_replicodeOutput = new QListWidget(m_toolview);
    m_replicodeOutput->setSelectionMode(QAbstractItemView::ContiguousSelection);
    connect(m_replicodeOutput, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(outputClicked(QListWidgetItem*)));
    mainWindow->hideToolView(m_toolview);

    m_configSidebar = mainWindow->createToolView("kate_private_plugin_katereplicodeplugin_config", Kate::MainWindow::Right, SmallIcon("code-block"), i18n("Replicode Config"));
    m_configView = new ReplicodeConfig(m_configSidebar);

    m_runButton = new QPushButton(i18nc("shortcut for action", "Run (%1)", m_runAction->shortcut().toString()));
    m_stopButton = new QPushButton(i18nc("shortcut for action", "Stop (%1)", m_stopAction->shortcut().toString()));
    m_stopButton->setEnabled(false);

    QFormLayout *l = qobject_cast<QFormLayout*>(m_configView->widget(0)->layout());
    Q_ASSERT(l);
    l->addRow(m_runButton, m_stopButton);
    connect(m_runButton, SIGNAL(clicked()), m_runAction, SLOT(trigger()));
    connect(m_stopButton, SIGNAL(clicked()), m_stopAction, SLOT(trigger()));

    m_mainWindow->guiFactory()->addClient(this);
    connect(m_mainWindow, SIGNAL(viewChanged()), SLOT(viewChanged()));
}

ReplicodeView::~ReplicodeView()
{
    m_mainWindow->guiFactory()->removeClient(this);
    delete m_executor;
    delete m_settingsFile;
}

void ReplicodeView::viewChanged()
{
    if (m_mainWindow->activeView() && m_mainWindow->activeView()->document() && m_mainWindow->activeView()->document()->url().fileName().endsWith(".replicode")) {
        m_mainWindow->showToolView(m_configSidebar);
    } else {
        m_mainWindow->hideToolView(m_configSidebar);
        m_mainWindow->hideToolView(m_toolview);
    }
}

void ReplicodeView::runReplicode()
{
    m_mainWindow->showToolView(m_toolview);
    KTextEditor::View *editor = m_mainWindow->activeView();
    if (!editor || !editor->document()) {
        QMessageBox::warning(m_mainWindow->centralWidget(), i18n("Unable to find active file"), i18n("Can't find active file to run!"));
        return;
    }

    if (editor->document()->isEmpty()) {
        QMessageBox::warning(m_mainWindow->centralWidget(), i18n("Empty document"), i18n("Can't execute an empty document"));
        return;
    }

    QFileInfo sourceFile = QFileInfo(editor->document()->url().toLocalFile());

    if (!sourceFile.isReadable()) {
        QMessageBox::warning(m_mainWindow->centralWidget(), i18n("No file"), i18n("Unable to open source file for reading."));
        return;
    }

    KConfigGroup config(KGlobal::config(), "Replicode");
    QString executorPath = config.readEntry<QString>("replicodePath", QString());
    if (executorPath.isEmpty()) {
        QMessageBox::warning(m_mainWindow->centralWidget(), i18n("Can't find replicode executor"), i18n("Unable to find replicode executor.\nPlease go to settings and set the path to the Replicode executor."));
        return;
    }

    if (m_configView->settingsObject()->userOperatorPath.isEmpty()) {
        QMessageBox::warning(m_mainWindow->centralWidget(), i18n("Can't find user operator library"), i18n("Unable to find user operator library.\nPlease go to settings and set the path to the library."));
    }

    if (m_settingsFile) delete m_settingsFile;

    m_settingsFile = new QTemporaryFile;
    if (!m_settingsFile->open()) {
        delete m_settingsFile;
        m_settingsFile = 0;
        QMessageBox::warning(m_mainWindow->window(), tr("Unable to create file"), tr("Unable to create temporary file!"));
    }
    m_configView->settingsObject()->writeXml(m_settingsFile, sourceFile.canonicalFilePath());
    m_settingsFile->close();

    m_replicodeOutput->clear();

    if (m_executor) delete m_executor;
    m_executor = new QProcess(this);
    m_executor->setWorkingDirectory(sourceFile.canonicalPath());
    connect(m_executor, SIGNAL(readyReadStandardError()), SLOT(gotStderr()));
    connect(m_executor, SIGNAL(readyReadStandardOutput()), SLOT(gotStdout()));
    connect(m_executor, SIGNAL(finished(int)), SLOT(replicodeFinished()));
    connect(m_executor, SIGNAL(error(QProcess::ProcessError)), SLOT(runErrored(QProcess::ProcessError)));
    qDebug() << executorPath << sourceFile.canonicalPath();
    m_completed = false;
    m_executor->start(executorPath, QStringList() << m_settingsFile->fileName(), QProcess::ReadOnly);

    m_runAction->setEnabled(false);
    m_runButton->setEnabled(false);
    m_stopAction->setEnabled(true);
    m_stopButton->setEnabled(true);
}

void ReplicodeView::stopReplicode()
{
    if (m_executor) {
        m_executor->kill();
    }
}

void ReplicodeView::outputClicked(QListWidgetItem *item)
{
    QString output = item->text();
    QStringList pieces = output.split(':');

    if (pieces.length() < 2) return;

    QFileInfo file(pieces[0]);
    if (!file.isReadable()) return;

    bool ok = false;
    int lineNumber = pieces[1].toInt(&ok);
    qDebug() << lineNumber;
    if (!ok) return;

    KTextEditor::View *doc = m_mainWindow->openUrl(pieces[0]);
    doc->setCursorPosition(KTextEditor::Cursor(lineNumber, 0));
    qDebug() << doc->cursorPosition().line();
}

void ReplicodeView::runErrored(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    QListWidgetItem *item = new QListWidgetItem(i18n("Replicode execution failed: %1", m_executor->errorString()));
    item->setForeground(Qt::red);
    m_replicodeOutput->addItem(item);
    m_replicodeOutput->scrollToBottom();
    m_completed = true;
}

void ReplicodeView::replicodeFinished()
{
    if (!m_completed) {
        QListWidgetItem *item = new QListWidgetItem(i18n("Replicode execution finished!"));
        item->setForeground(Qt::blue);
        m_replicodeOutput->addItem(item);
        m_replicodeOutput->scrollToBottom();
    }

    m_runAction->setEnabled(true);
    m_runButton->setEnabled(true);
    m_stopAction->setEnabled(false);
    m_stopButton->setEnabled(false);
//    delete m_executor;
//    delete m_settingsFile;
//    m_executor = 0;
//    m_settingsFile = 0;
}

void ReplicodeView::gotStderr()
{
    QString output = m_executor->readAllStandardError();
    foreach(QString line, output.split('\n')) {
        line = line.simplified();
        if (line.isEmpty()) continue;
        QListWidgetItem *item = new QListWidgetItem(line);
        item->setForeground(Qt::red);
        m_replicodeOutput->addItem(item);
    }
    m_replicodeOutput->scrollToBottom();
}

void ReplicodeView::gotStdout()
{
    QString output = m_executor->readAllStandardOutput();
    foreach(QString line, output.split('\n')) {
        line = line.simplified();
        if (line.isEmpty()) continue;
        QListWidgetItem *item = new QListWidgetItem(' ' + line);
        if (line[0] == '>') item->setForeground(Qt::gray);
        m_replicodeOutput->addItem(item);
    }
    m_replicodeOutput->scrollToBottom();
}

#include "replicodeview.moc"
#include "replicodeplugin.moc"
