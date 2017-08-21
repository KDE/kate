/*
 *  Copyright (C) 2017 by Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "ktexteditorpreviewplugin.h"

#include "ktexteditorpreviewview.h"
#include <ktexteditorpreview_debug.h>

// KF
#include <KTextEditor/MainWindow>
#include <KPluginFactory>


K_PLUGIN_FACTORY_WITH_JSON(KTextEditorPreviewPluginFactory, "ktexteditorpreview.json", registerPlugin<KTextEditorPreviewPlugin>();)


KTextEditorPreviewPlugin::KTextEditorPreviewPlugin(QObject* parent, const QVariantList& /*args*/)
    : KTextEditor::Plugin(parent)
{
}

KTextEditorPreviewPlugin::~KTextEditorPreviewPlugin() = default;

QObject* KTextEditorPreviewPlugin::createView(KTextEditor::MainWindow* mainwindow)
{
    return new KTextEditorPreviewView(this, mainwindow);
}


// needed for K_PLUGIN_FACTORY_WITH_JSON
#include <ktexteditorpreviewplugin.moc>
