/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATESQLPLUGIN_H
#define KATESQLPLUGIN_H



#include <ktexteditor/view.h>
#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/mainwindow.h>
#include <kate/pluginconfigpageinterface.h>

#include <kpluginfactory.h>

class KateSQLPlugin : public Kate::Plugin, public Kate::PluginConfigPageInterface
{
  Q_OBJECT
  Q_INTERFACES(Kate::PluginConfigPageInterface)

  public:
    explicit KateSQLPlugin(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());

    virtual ~KateSQLPlugin();

    Kate::PluginView *createView(Kate::MainWindow *mainWindow);

    // PluginConfigPageInterface

    uint configPages() const { return 1; };
    Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0);
    QString configPageName (uint number = 0) const;
    QString configPageFullName (uint number = 0) const;
    KIcon configPageIcon (uint number = 0) const;

  signals:
    void globalSettingsChanged();
};

K_PLUGIN_FACTORY_DECLARATION(KateSQLFactory)

#endif // KATESQLPLUGIN_H

