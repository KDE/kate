#ifndef PLUGIN_KATEBUILD_H
#define PLUGIN_KATEBUILD_H
/* plugin_katebuild.h                    Kate Plugin
**
** Copyright (C) 2008-2015 by Kåre Särs <kare.sars@iki.fi>
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

#include <QRegularExpression>
#include <QString>
#include <QStack>
#include <QPointer>
#include <KProcess>

#include <KTextEditor/MainWindow>
#include <KTextEditor/Document>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/Message>

#include <KXMLGUIClient>
#include <KConfigGroup>

#include "ui_build.h"
#include "targets.h"

/******************************************************************/
class KateBuildView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)
    Q_PROPERTY(QUrl docUrl READ docUrl)

    public:

        enum ResultDetails {
            FullOutput,
            ParsedOutput,
            ErrorsAndWarnings,
            OnlyErrors
        };

        enum TreeWidgetRoles {
            ErrorRole = Qt::UserRole+1
        };

        enum ErrorCategory {
            CategoryInfo,
            CategoryWarning,
            CategoryError
        };

       KateBuildView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw);
        ~KateBuildView() override;

        // reimplemented: read and write session config
        void readSessionConfig(const KConfigGroup& config) override;
        void writeSessionConfig(KConfigGroup& config) override;

        bool buildCurrentTarget();

        QUrl docUrl();

    private Q_SLOTS:

        // Building
        void slotSelectTarget();
        void slotBuildActiveTarget();
        void slotBuildPreviousTarget();
        void slotBuildDefaultTarget();
        bool slotStop();

        // Parse output
        void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
        void slotReadReadyStdErr();
        void slotReadReadyStdOut();

        // Selecting warnings/errors
        void slotNext();
        void slotPrev();
        void slotErrorSelected(QTreeWidgetItem *item);

        // Settings
        void targetSetNew();
        void targetOrSetCopy();
        void targetDelete();

        void slotAddTargetClicked();

        void slotDisplayMode(int mode);

        void handleEsc(QEvent *e);

        /**
         * keep track if the project plugin is alive and if the project map did change
         */
        void slotPluginViewCreated(const QString &name, QObject *pluginView);
        void slotPluginViewDeleted(const QString &name, QObject *pluginView);
        void slotProjectMapChanged();
        void slotAddProjectTarget();

    protected:
        bool eventFilter(QObject *obj, QEvent *ev) override;

    private:
        void processLine(const QString &);
        void addError(const QString &filename, const QString &line,
                      const QString &column, const QString &message);
        bool startProcess(const QString &dir, const QString &command);
        bool checkLocal(const QUrl &dir);
        void clearBuildResults();

        void displayBuildResult(const QString &message, KTextEditor::Message::MessageType level);

        KTextEditor::MainWindow *m_win;
        QWidget          *m_toolView;
        Ui::build         m_buildUi;
        QWidget          *m_buildWidget;
        int               m_outputWidgetWidth;
        TargetsUi        *m_targetsUi;
        KProcess          m_proc;
        QString           m_stdOut;
        QString           m_stdErr;
        QString           m_currentlyBuildingTarget;
        bool              m_buildCancelled;
        int               m_displayModeBeforeBuild;
        QString           m_make_dir;
        QStack<QString>   m_make_dir_stack;
        QRegularExpression m_filenameDetector;
        QRegularExpression m_filenameDetectorIcpc;
        bool              m_filenameDetectorGccWorked;
        QRegularExpression m_newDirDetector;
        unsigned int      m_numErrors;
        unsigned int      m_numWarnings;
        QString           m_prevItemContent;
        QModelIndex       m_previousIndex;
        QPointer<KTextEditor::Message> m_infoMessage;


        /**
        * current project plugin view, if any
        */
        QObject *m_projectPluginView;
};


typedef QList<QVariant> VariantList;

/******************************************************************/
class KateBuildPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    public:
        explicit KateBuildPlugin(QObject* parent = nullptr, const VariantList& = VariantList());
        ~KateBuildPlugin() override {}

        QObject *createView(KTextEditor::MainWindow *mainWindow) override;
 };

#endif

