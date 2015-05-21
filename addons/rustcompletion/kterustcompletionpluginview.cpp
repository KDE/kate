/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "kterustcompletionpluginview.h"
#include "kterustcompletionplugin.h"

#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

KTERustCompletionPluginView::KTERustCompletionPluginView(KTERustCompletionPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_plugin(plugin)
    , m_mainWindow(mainWin)
{
    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated,
        this, &KTERustCompletionPluginView::viewCreated);

    foreach(KTextEditor::View *view, m_mainWindow->views()) {
        viewCreated(view);
    }
}

KTERustCompletionPluginView::~KTERustCompletionPluginView()
{
    foreach(KTextEditor::View *view, m_completionViews) {
        KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);

        if (cci) {
            cci->unregisterCompletionModel(m_plugin->completion());
        }
    }
}

void KTERustCompletionPluginView::viewCreated(KTextEditor::View *view)
{
    connect(view->document(), &KTextEditor::Document::documentUrlChanged,
        this, &KTERustCompletionPluginView::documentChanged,
        Qt::UniqueConnection);
    connect(view->document(), &KTextEditor::Document::highlightingModeChanged,
        this, &KTERustCompletionPluginView::documentChanged,
        Qt::UniqueConnection);

    registerCompletion(view);
}

void KTERustCompletionPluginView::viewDestroyed(QObject *view)
{
    m_completionViews.remove(static_cast<KTextEditor::View *>(view));
}

void KTERustCompletionPluginView::documentChanged(KTextEditor::Document *document)
{
    foreach(KTextEditor::View *view, document->views()) {
        registerCompletion(view);
    }
}

void KTERustCompletionPluginView::registerCompletion(KTextEditor::View *view) {
    bool registered = m_completionViews.contains(view);
    bool isRust = (view->document()->url().path().endsWith(QStringLiteral(".rs")) ||
        view->document()->highlightingMode() == QStringLiteral("Rust"));

    KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);

    if (!cci) {
        return;
    }

    if (!registered && isRust) {
        cci->registerCompletionModel(m_plugin->completion());
        m_completionViews.insert(view);

        connect(view, &KTextEditor::View::destroyed, this,
            &KTERustCompletionPluginView::viewDestroyed,
            Qt::UniqueConnection);
    }

    if (registered && !isRust) {
        cci->unregisterCompletionModel(m_plugin->completion());
        m_completionViews.remove(view);
    }
}
