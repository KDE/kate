/***************************************************************************
                          plugin_katexmlcheck.h
                          -------------------
   begin                : 2002-07-06
   copyright            : (C) 2002 by Daniel Naber
   email                : daniel.naber@t-online.de
***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/

#pragma once

#include "diagnostics/diagnosticview.h"

#include <KTextEditor/Document>
#include <ktexteditor/application.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>

#include <QProcess>
#include <QString>
#include <QVariantList>

class QTreeWidget;
class QTreeWidgetItem;
class QTemporaryFile;

class PluginKateXMLCheckView : public QObject, public KXMLGUIClient
{
public:
    PluginKateXMLCheckView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainwin);
    ~PluginKateXMLCheckView() override;

    KTextEditor::MainWindow *m_mainWindow;

public:
    bool slotValidate();
    void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
    static void slotUpdate();

private:
    QTemporaryFile *m_tmp_file;
    bool m_validating = false;
    QProcess m_proc;
    QString m_proc_stderr;
    QString m_dtdname;
    DiagnosticsProvider m_provider;
};

class PluginKateXMLCheck : public KTextEditor::Plugin
{
public:
    explicit PluginKateXMLCheck(QObject *parent);

    ~PluginKateXMLCheck() override;
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};
