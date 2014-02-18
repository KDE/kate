/* This file is part of the KDE project
   Copyright (C) 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef REPLICODEPLUGIN_H
#define REPLICODEPLUGIN_H

#include "replicodeview.h"
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>


class ReplicodePlugin : public Kate::Plugin, public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

  public:
    // Constructor
    explicit ReplicodePlugin(QObject *parent = 0, const QList<QVariant> &args = QList<QVariant>());
    // Destructor
    virtual ~ReplicodePlugin();

    Kate::PluginView *createView(Kate::MainWindow *mainWindow) {
        return new ReplicodeView(mainWindow);
    }

    // Config interface
    uint configPages () const;
    Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0 );
    QString configPageFullName (uint number = 0) const;
    QString configPageName(uint number = 0) const;
    KIcon configPageIcon (uint number = 0) const;
};

#endif
