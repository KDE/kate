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
#include <kselectaction.h>

#include <map>

#include "ui_build.h"
#include "targets.h"

/******************************************************************/
class KateBuildView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

    public:

       struct TargetSet {
         QString name;
         QString defaultDir;
         QString defaultTarget;
         QString cleanTarget;
         QString prevTarget;
         std::map<QString, QString> targets;
       };
  
        KateBuildView(Kate::MainWindow *mw);
        ~KateBuildView();

        // overwritten: read and write session config
        void readSessionConfig(KConfigBase* config, const QString& groupPrefix);
        void writeSessionConfig(KConfigBase* config, const QString& groupPrefix);

        QWidget *toolView() const;

        TargetSet* currentTargetSet();

        bool buildTarget(const QString& targetName);

    private Q_SLOTS:
        // selecting warnings
        void slotItemSelected(QTreeWidgetItem *item);
        void slotNext();
        void slotPrev();

        bool slotMake();
        bool slotMakeClean();
        bool slotStop();

        void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
        void slotReadReadyStdErr();
        void slotReadReadyStdOut();

        void slotSelectTarget();
        void slotBuildPreviousTarget();

        // settings
        void slotBrowseClicked();
        void targetSelected(int index);
        void targetsChanged();
        void targetNew();
        void targetCopy();
        void targetDelete();
        void targetNext();
        void slotBuildDirChanged(const QString& dir);
        void slotTargetSetNameChanged(const QString& name);

        void slotDisplayMode(int mode);

        void handleEsc(QEvent *e);

        /**
         * keep track if the project plugin is alive and if the project map did change
         */
        void slotPluginViewCreated(const QString &name, Kate::PluginView *pluginView);
        void slotPluginViewDeleted(const QString &name, Kate::PluginView *pluginView);
        void slotProjectMapChanged();
        void slotAddProjectTarget();
        void slotRemoveProjectTarget();

        void slotAddTargetClicked();
        void slotBuildTargetClicked();
        void slotDeleteTargetClicked();
        void slotCellChanged(int row, int column);
        void slotSelectionChanged();
        void slotResizeColumn(int column);

    protected:
        bool eventFilter(QObject *obj, QEvent *ev);

    private:
        void processLine(const QString &);
        void addError(const QString &filename, const QString &line,
                      const QString &column, const QString &message);
        bool startProcess(const KUrl &dir, const QString &command);
        KUrl docUrl();
        bool checkLocal(const KUrl &dir);
        void setTargetRowContents(int row, const TargetSet& tgtSet, const QString& name, const QString& buildCmd);
        QString makeTargetNameUnique(const QString& name);
        QString makeUniqueTargetSetName() const;
        void clearBuildResults();

        Kate::MainWindow *m_win;
        QWidget          *m_toolView;
        Ui::build         m_buildUi;
        QWidget          *m_buildWidget;
        int               m_outputWidgetWidth;
        TargetsUi        *m_targetsUi;
        KProcess         *m_proc;
        QString           m_output_lines;
        QString           m_currentlyBuildingTarget;
        bool              m_buildCancelled;
        int               m_displayModeBeforeBuild;
        KUrl              m_make_dir;
        QStack<KUrl>      m_make_dir_stack;
        QRegExp           m_filenameDetector;
        QRegExp           m_filenameDetectorIcpc;
        bool              m_filenameDetectorGccWorked;
        QRegExp           m_newDirDetector;
        unsigned int      m_numErrors;
        unsigned int      m_numWarnings;
        QList<TargetSet>  m_targetList;
        int               m_targetIndex;
        KSelectAction*    m_targetSelectAction;
        QString           m_prevItemContent;

        /**
        * current project plugin view, if any
        */
        Kate::PluginView *m_projectPluginView;
};


typedef QList<QVariant> VariantList;

/******************************************************************/
class KateBuildPlugin : public Kate::Plugin
{
    Q_OBJECT

    public:
        explicit KateBuildPlugin(QObject* parent = 0, const VariantList& = VariantList());
        virtual ~KateBuildPlugin() {}

        Kate::PluginView *createView(Kate::MainWindow *mainWindow);

 };

#endif

