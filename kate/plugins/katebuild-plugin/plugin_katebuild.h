#ifndef PLUGIN_KATEBUILD_H
#define PLUGIN_KATEBUILD_H
/* plugin_katebuild.h                    Kate Plugin
**
** Copyright (C) 2008 by Kåre Särs <kare.sars@iki.fi>
**
** This code is almost a total rewrite of the GPL'ed Make plugin
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

class QRegExp;

#include <QTreeWidgetItem>
#include <QString>
#include <QStack>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <kprocess.h>

#include "ui_build.h"

/******************************************************************/
class KateBuildView : public Kate::PluginView, public KXMLGUIClient
{
    Q_OBJECT

    public:
        KateBuildView(Kate::MainWindow *mw);
        ~KateBuildView();

        // overwritten: read and write session config
        void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
        void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

        QWidget *toolView() const;

    public slots:
        // selecting warnings
        void slotItemSelected(QTreeWidgetItem *item);
        void slotNext();
        void slotPrev();

        bool slotMake();
        bool slotMakeClean();
        bool slotQuickCompile();
        bool slotStop();

        void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
        void slotReadReadyStdErr();
        void slotReadReadyStdOut();

        // settings
        void slotBrowseClicked();

    protected:
        void processLine(const QString &);
        void addError(const QString &filename, const QString &line,
                      const QString &column, const QString &message);
        bool startProcess(const QString &);

    private:
        Kate::MainWindow *m_win;
        QWidget          *m_toolView;
        Ui::build         buildUi;
        KProcess         *m_proc;
        QString           m_output_lines;
        KUrl              m_make_dir;
        QStack<KUrl>      m_make_dir_stack;
        QRegExp           m_filenameDetector;
        QRegExp           m_newDirDetector;
        bool              m_found_error;
};


/******************************************************************/
class KateBuildPlugin : public Kate::Plugin
{
    Q_OBJECT

    public:
        explicit KateBuildPlugin(QObject* parent = 0, const QStringList& = QStringList());
        virtual ~KateBuildPlugin() {}

        Kate::PluginView *createView(Kate::MainWindow *mainWindow);

 };

#endif

