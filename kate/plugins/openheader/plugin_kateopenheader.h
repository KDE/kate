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

#ifndef PLUGIN_KATEOPENHEADER_H
#define PLUGIN_KATEOPENHEADER_H

#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <kxmlguiclient.h>

#include <QList>

class PluginKateOpenHeader : public Kate::Plugin
{
  Q_OBJECT

  public:
    PluginKateOpenHeader( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~PluginKateOpenHeader();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

  public slots:
    void slotOpenHeader ();
    void tryOpen( const KUrl& url, const QStringList& extensions );   
};

class PluginViewKateOpenHeader: public Kate::PluginView, KXMLGUIClient {
    Q_OBJECT
    public:
        PluginViewKateOpenHeader(PluginKateOpenHeader* plugin, Kate::MainWindow *mainwindow);
        virtual ~PluginViewKateOpenHeader();
};

#endif // PLUGIN_KATEOPENHEADER_H
