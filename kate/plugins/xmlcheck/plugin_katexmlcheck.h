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

#ifndef PLUGIN_KATEXMLCHECK_H
#define PLUGIN_KATEXMLCHECK_H

#include <QProcess>

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

class QTreeWidget;
class QTreeWidgetItem;
class KTemporaryFile;
class KProcess;

class PluginKateXMLCheckView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    PluginKateXMLCheckView(Kate::MainWindow *mainwin);
    virtual ~PluginKateXMLCheckView();

    Kate::MainWindow *win;
    QWidget *dock;

public slots:
    bool slotValidate();
    void slotClicked(QTreeWidgetItem *item, int column);
    void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
    void slotUpdate();

private:
    KTemporaryFile *m_tmp_file;
    KParts::ReadOnlyPart *part;
    bool m_validating;
    KProcess *m_proc;
    QString m_proc_stderr;
    QString m_dtdname;
    QTreeWidget *listview;
};


class PluginKateXMLCheck : public Kate::Plugin
{
  Q_OBJECT

public:
  explicit PluginKateXMLCheck( QObject* parent = 0, const QVariantList& = QVariantList() );

    virtual ~PluginKateXMLCheck();
    Kate::PluginView *createView(Kate::MainWindow *mainWindow);
};

#endif // PLUGIN_KATEXMLCHECK_H
