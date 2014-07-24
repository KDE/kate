/***************************************************************************
  A KTextEditor (Kate Part) plugin for speaking text.

  Copyright:
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
  (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>

  Original Author: Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATEKTTSD_H_
#define _KATEKTTSD_H_

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>
#include <kate/plugin.h>

#include <QtCore/QObject>

class KateKttsdPlugin : public Kate::Plugin
{
    Q_OBJECT

    public:
        explicit KateKttsdPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
        virtual ~KateKttsdPlugin() {};
        Kate::PluginView *createView(Kate::MainWindow *mainWindow);
};

class KateKttsdPluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

    public:
        explicit KateKttsdPluginView(Kate::MainWindow *mw );
    ~KateKttsdPluginView();

    public Q_SLOTS:
        void slotReadOut();
};

#endif      // _KATEKTTSD_H_
