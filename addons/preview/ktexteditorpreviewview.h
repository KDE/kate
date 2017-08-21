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

#ifndef KTEXTEDITORPREVIEWVIEW_H
#define KTEXTEDITORPREVIEWVIEW_H

// KF
#include <KTextEditor/SessionConfigInterface>
// Qt
#include <QObject>
#include <QPointer>

namespace KTextEditorPreview {
class PreviewWidget;
}

namespace KTextEditor {
class MainWindow;
class View;
}

class KTextEditorPreviewPlugin;

class QWidget;

class KTextEditorPreviewView: public QObject, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)

public:
    KTextEditorPreviewView(KTextEditorPreviewPlugin* plugin, KTextEditor::MainWindow* mainWindow);
    ~KTextEditorPreviewView() override;

    void readSessionConfig(const KConfigGroup& config) override;
    void writeSessionConfig(KConfigGroup& config) override;

private:
    QPointer<QWidget> m_toolView;
    KTextEditorPreview::PreviewWidget* m_previewView;
};

#endif
