 /***************************************************************************
                          plugin_katetextfilter.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PLUGIN_KANT_HEADER_H
#define _PLUGIN_KANT_HEADER_H

#include <kate/plugin.h>
#include <kate/mainwindow.h>

#include <QList>

class PluginKateOpenHeader : public Kate::Plugin, Kate::PluginViewInterface
{
  Q_OBJECT

  public:
    PluginKateOpenHeader( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~PluginKateOpenHeader();

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

    void storeGeneralConfig(KConfig* config,const QString& groupPrefix);
    void loadGeneralConfig(KConfig* config,const QString& groupPrefix);

    void storeViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix);
    void loadViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix);

  public slots:
    void slotOpenHeader ();
    void tryOpen( const KUrl& url, const QStringList& extensions );   

  private:
  QList<class PluginView*> m_views;
};

#endif // _PLUGIN_KANT_OPENHEADER_H
