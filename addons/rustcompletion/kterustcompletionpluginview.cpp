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

#include <QAction>

#include <KActionCollection>
#include <KLocalizedString>
#include <KStandardAction>
#include <KXMLGUIFactory>

#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

KTERustCompletionPluginView::KTERustCompletionPluginView(KTERustCompletionPlugin *plugin, KTextEditor::MainWindow *mainWin)
    : QObject(mainWin)
    , m_plugin(plugin)
    , m_mainWindow(mainWin)
{
    KXMLGUIClient::setComponentName(QStringLiteral("kterustcompletion"), i18n("Rust code completion"));
    setXMLFile(QStringLiteral("ui.rc"));

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KTERustCompletionPluginView::viewChanged);
    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, &KTERustCompletionPluginView::viewCreated);

    foreach(KTextEditor::View *view, m_mainWindow->views()) {
        viewCreated(view);
    }

    auto a = actionCollection()->addAction(QStringLiteral("rust_find_definition"), this, SLOT(goToDefinition()));
    a->setText(i18n("Go to Definition"));

    viewChanged();

    m_mainWindow->guiFactory()->addClient(this);
}

KTERustCompletionPluginView::~KTERustCompletionPluginView()
{
}

void KTERustCompletionPluginView::goToDefinition()
{
    KTextEditor::View *activeView = m_mainWindow->activeView();

    if (!activeView) {
        return;
    }

    const KTextEditor::Document *document = activeView->document();
    const KTextEditor::Cursor cursor = activeView->cursorPosition();

    QList<CompletionMatch> matches = m_plugin->completion()->getMatches(document,
        KTERustCompletion::FindDefinition, cursor);

    if (matches.count()) {
        const CompletionMatch &match = matches.at(0);

        if (!(match.line != -1 && match.col != -1)) {
            return;
        }

        KTextEditor::Cursor def(match.line, match.col);

        if (match.url == document->url()) {
            activeView->setCursorPosition(def);
        } else if (match.url.isValid()) {
            KTextEditor::View *view = m_mainWindow->openUrl(match.url);

            if (view) {
                view->setCursorPosition(def);
            }
        }
    }
}

void KTERustCompletionPluginView::viewChanged()
{
    const KTextEditor::View *activeView = m_mainWindow->activeView();

    auto a = actionCollection()->action(QStringLiteral("rust_find_definition"));

    if (a) {
        bool isRust = isRustView(activeView);
        a->setEnabled(isRust);
        a->setVisible(isRust);
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
    bool isRust = isRustView(view);

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

bool KTERustCompletionPluginView::isRustView(const KTextEditor::View *view)
{
    if (view) {
        return (view->document()->url().path().endsWith(QStringLiteral(".rs")) ||
            view->document()->highlightingMode() == QStringLiteral("Rust"));
    }

    return false;
}
