/***************************************************************************
                          pybrowse_part.h  -  description
                             -------------------
    begin                : Tue Aug 28 2001
    copyright            : (C) 2001 by Christian Bird
    email                : chrisb@lineo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PYBROWSE_PART_H_
#define _PYBROWSE_PART_H_

#include <kate/application.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kate/documentmanager.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <k3dockwidget.h>
#include <klibloader.h>
#include <klocale.h>
#include <qstring.h>
//Added by qt3to4:
#include <Q3PtrList>
#include "kpybrowser.h"

class PluginViewPyBrowse : public QObject, KXMLGUIClient
{
  Q_OBJECT

  friend class KatePluginPyBrowse;

  public:
    PluginViewPyBrowse (Kate::MainWindow *w);
    ~PluginViewPyBrowse ();

  public slots:
    void slotShowPyBrowser();
    void slotSelected(QString name, int line);
    void slotUpdatePyBrowser();

  private:
    Kate::MainWindow *win;
    QWidget *my_dock;
    KPyBrowser *kpybrowser;
};

class KatePluginPyBrowse : public Kate::Plugin, public Kate::PluginViewInterface
{
  Q_OBJECT

  public:
    explicit KatePluginPyBrowse( QObject* parent = 0, const QStringList& = QStringList() );
    ~KatePluginPyBrowse();

    void addView(Kate::MainWindow *win);
    void removeView(Kate::MainWindow *win);
	void storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
	void loadViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
	void storeGeneralConfig(KConfig* config, const QString& groupPrefix);
	void loadGeneralConfig(KConfig* config, const QString& groupPrefix);
  private:
    Q3PtrList<PluginViewPyBrowse> m_views;
};

#endif
