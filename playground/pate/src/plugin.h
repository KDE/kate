// This file is part of Pate, Kate' Python scripting plugin.
//
// Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#ifndef PATE_PLUGIN_H
#define PATE_PLUGIN_H

#include <kate/plugin.h>
#include <kate/mainwindow.h>

#include <kxmlguiclient.h>

#include "Python.h"


namespace Pate {

class Plugin : public Kate::Plugin {
    Q_OBJECT

public:
    explicit Plugin(QObject *parent = 0, const QStringList& = QStringList());
    virtual ~Plugin();
    
    Kate::PluginView *createView(Kate::MainWindow*);

    void readSessionConfig(KConfigBase *config, const QString& groupPrefix);
    void writeSessionConfig(KConfigBase *config, const QString& groupPrefix);
};

class PluginView : public Kate::PluginView {
    Q_OBJECT
public:
    PluginView(Kate::MainWindow *window);
};

} // namespace Pate

#endif
