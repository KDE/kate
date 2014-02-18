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

#ifndef REPLICODEVIEW_H
#define REPLICODEVIEW_H

#include <QObject>
#include <KXMLGUIClient>
#include <kate/plugin.h>
#include <QProcess>

class QListWidgetItem;
class QListWidget;
class QTemporaryFile;
class QProcess;
class QListWidget;
class QPushButton;
class KAction;
class ReplicodeConfig;

class ReplicodeView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

    public:
        explicit ReplicodeView(Kate::MainWindow *mainWindow);
        ~ReplicodeView();

    private slots:
        void runReplicode();
        void stopReplicode();
        void replicodeFinished();
        void gotStderr();
        void gotStdout();
        void runErrored(QProcess::ProcessError);
        void outputClicked(QListWidgetItem *item);
        void viewChanged();

    private:
        Kate::MainWindow *m_mainWindow;
        QTemporaryFile *m_settingsFile;
        QProcess *m_executor;
        QListWidget *m_replicodeOutput;
        QWidget *m_toolview;
        QWidget *m_configSidebar;
        QPushButton *m_runButton;
        QPushButton *m_stopButton;
        KAction *m_runAction;
        KAction *m_stopAction;
        ReplicodeConfig *m_configView;
        bool m_completed;
};

#endif
