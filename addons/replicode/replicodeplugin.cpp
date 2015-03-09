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

#include "replicodeplugin.h"
#include "replicodeconfigpage.h"

#include <KPluginFactory>
#include <KPluginLoader>

#include <KTextEditor/Application>

K_PLUGIN_FACTORY_WITH_JSON (KateReplicodePluginFactory, "katereplicodeplugin.json", registerPlugin<ReplicodePlugin>();)

ReplicodePlugin::ReplicodePlugin(QObject* parent, const QList< QVariant > &args)
: KTextEditor::Plugin(qobject_cast<KTextEditor::Application*>(parent))
{
    Q_UNUSED(args);
}

ReplicodePlugin::~ReplicodePlugin()
{
}

KTextEditor::ConfigPage* ReplicodePlugin::configPage(int number, QWidget* parent)
{
    Q_UNUSED(number);
    Q_ASSERT(number == 0);
    return new ReplicodeConfigPage(parent);
}

#include "replicodeplugin.moc"
