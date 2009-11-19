/**
 * This file is part of the KDE libraries
 * Copyright (C) 2009 Milian Wolff <mail@milianw.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "exporterplugin.h"

#include <kpluginloader.h>

#include <ktexteditor/view.h>

#include "exporterpluginview.h"

K_PLUGIN_FACTORY_DEFINITION(ExporterPluginFactory,
        registerPlugin<ExporterPlugin>("ktexteditor_exporter");
        )
K_EXPORT_PLUGIN(ExporterPluginFactory("ktexteditor_exporter", "ktexteditor_plugins"))

ExporterPlugin::ExporterPlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent)
{
    Q_UNUSED(args);
}

ExporterPlugin::~ExporterPlugin()
{
}

void ExporterPlugin::addView(KTextEditor::View *view)
{
  // no need to keep track of, QObject inheritance will take care of deletion
  new ExporterPluginView(view);
}

#include "exporterplugin.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
