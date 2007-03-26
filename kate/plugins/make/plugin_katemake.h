//Added by qt3to4:
#include <Q3PtrList>
#ifndef _KATEMAKE_H
#define _KATEMAKE_H
/* plugin_katemake.h                    Kate Plugin
**
** Copyright (C) 2003 by Adriaan de Groot
**
** This is the hader for the Make plugin.
**
** This code was mostly copied from the GPL'ed xmlcheck plugin
** by Daniel Naber.
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

// #define QT_NO_CAST_ASCII	(1)
// #define QT_NO_ASCII_CAST	(1)

class QRegExp;

#include <q3listview.h>
#include <qstring.h>

#include <kate/plugin.h>
#include <kate/application.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <k3dockwidget.h>
#include <kiconloader.h>
#include <k3process.h>

class PluginKateMakeView : public Q3ListView, public KXMLGUIClient
{
  Q_OBJECT

  public:
	PluginKateMakeView(QWidget *parent,
		Kate::MainWindow *mainwin,
		const char* name);
	virtual ~PluginKateMakeView();

	Kate::MainWindow *win;

  public slots:
	void slotClicked(Q3ListViewItem *item);
	void slotNext();
	void slotPrev();

	bool slotValidate();
	void slotProcExited(K3Process*);
	void slotReceivedProcStderr(K3Process*, char*, int);

	void slotConfigure();

protected:
	void processLine(const QString &);
	
  private:
	K3Process *m_proc;
	
	QString output_line;
	QString doc_name;
	QString document_dir;
	QString source_prefix,build_prefix;
	
	QRegExp *filenameDetector;
	
	Q3ListViewItem *running_indicator;
	bool found_error;
};


class PluginKateMake : public Kate::Plugin, Kate::PluginViewInterface
{
  Q_OBJECT

  public:
	PluginKateMake( QObject* parent = 0, const char* name = 0, const QStringList& = QStringList() );
	virtual ~PluginKateMake();

	void addView (Kate::MainWindow *win);
	void removeView (Kate::MainWindow *win);

  private:
	Q3PtrList<PluginKateMakeView> m_views;
};

#endif

