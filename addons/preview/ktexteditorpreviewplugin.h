/*
 *  SPDX-FileCopyrightText: 2017 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

// KF
#include <KTextEditor/Plugin>

class KTextEditorPreviewPlugin : public KTextEditor::Plugin
{
public:
    /**
     * Default constructor, with arguments as expected by KPluginFactory
     */
    KTextEditorPreviewPlugin(QObject *parentargs);

    ~KTextEditorPreviewPlugin() override;

public: // KTextEditor::Plugin API
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};
