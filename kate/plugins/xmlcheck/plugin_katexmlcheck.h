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

#include <q3listview.h>
#include <qstring.h>
#include <QProcess>
//Added by qt3to4:
#include <Q3PtrList>

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <k3dockwidget.h>
#include <kiconloader.h>

class KTemporaryFile;
class KProcess;

class PluginKateXMLCheckView : public Q3ListView, public KXMLGUIClient
{
  Q_OBJECT

  public:
	PluginKateXMLCheckView(QWidget *parent,Kate::MainWindow *mainwin,const char* name);
	virtual ~PluginKateXMLCheckView();

	Kate::MainWindow *win;
	QWidget *dock;

  public slots:
	bool slotValidate();
	void slotClicked(Q3ListViewItem *item);
	void slotProcExited(int exitCode, QProcess::ExitStatus exitStatus);
	void slotUpdate();

  private:
	KTemporaryFile *m_tmp_file;
	KParts::ReadOnlyPart *part;
	bool m_validating;
	KProcess *m_proc;
	QString m_proc_stderr;
	QString m_dtdname;
};


class PluginKateXMLCheck : public Kate::Plugin, Kate::PluginViewInterface
{
  Q_OBJECT

  public:
	explicit PluginKateXMLCheck( QObject* parent = 0, const QStringList& = QStringList() );
	virtual ~PluginKateXMLCheck();

	void addView (Kate::MainWindow *win);
	void removeView (Kate::MainWindow *win);
	void storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
	void loadViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
	void storeGeneralConfig(KConfig* config, const QString& groupPrefix);
	void loadGeneralConfig(KConfig* config, const QString& groupPrefix);
  private:
	Q3PtrList<PluginKateXMLCheckView> m_views;
};

#endif // PLUGIN_KATEXMLCHECK_H
