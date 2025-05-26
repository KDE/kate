/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "assistantview.h"
#include "assistant.h"

#include <ktexteditor_utils.h>

#include <KActionCollection>
#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

using namespace Qt::Literals::StringLiterals;

AssistantView::AssistantView(Assistant *plugin, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
    , m_assistant(plugin)
    , m_mainWindow(mainwindow)
{
    KXMLGUIClient::setComponentName(u"assistant"_s, i18n("Assistant"));
    setXMLFile(u"ui.rc"_s);

    QAction *a = actionCollection()->addAction(u"assistant_prompt"_s);
    a->setText(i18n("Send current line as prompt"));
    connect(a, &QAction::triggered, this, &AssistantView::sendCurrentLineAsPrompt);

    m_mainWindow->guiFactory()->addClient(this);
}

AssistantView::~AssistantView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

void AssistantView::sendCurrentLineAsPrompt()
{
    // get current view, can't do anything if not there
    auto view = m_mainWindow->activeView();
    if (!view) {
        return;
    }

    // we will just use the current line as prompt, if empty we do nothing
    const auto line = view->cursorPosition().line();
    const auto lineText = view->document()->line(line).trimmed();
    if (lineText.isEmpty()) {
        return;
    }

    // send prompt, insert result in line below the request line
    m_assistant->sendPrompt(lineText, view, [view, line](const QString &answer) {
        view->document()->insertText(KTextEditor::Cursor(line + 1, 0), answer);
    });
}
