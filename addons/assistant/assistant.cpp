/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "assistant.h"

#include <ktexteditor_utils.h>

#include <KActionCollection>
#include <KPluginFactory>

#include <QAction>
#include <QFileInfo>
#include <QIcon>
#include <QJsonDocument>
#include <QLayout>
#include <QMessageBox>
#include <QNetworkReply>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

using namespace Qt::Literals::StringLiterals;

K_PLUGIN_FACTORY_WITH_JSON(AssistantFactory, "plugin.json", registerPlugin<Assistant>();)

Assistant::Assistant(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
    // init the request handling infrastucture
    connect(&m_accessManager, &QNetworkAccessManager::finished, this, &Assistant::requestFinished);
}

QObject *Assistant::createView(KTextEditor::MainWindow *mainWindow)
{
    return new AssistantView(this, mainWindow);
}

void Assistant::sendPrompt(const QString &prompt, const QObject *context, const std::function<void(const QString &QString)> &resultHandler)
{
    // construct prompt request
    QNetworkRequest request;
    request.setRawHeader("Content-Type", "application/json");

    // TODO: make it configurable
    request.setUrl(QUrl(u"http://localhost:11434/api/generate"_s));

    // setup prompt json
    QJsonObject json{
        {u"prompt"_s, prompt},

        // TODO: make it configurable
        {u"model"_s, u"llama3.2"_s},
        {u"stream"_s, false},
    };

    // send request with JSON data for the request
    auto reply = m_accessManager.post(request, QJsonDocument(json).toJson());

    // for the finished handling we will remember reply to context/handler mapping
    const auto res = m_promptRequests.insert(std::make_pair(reply, PromptRequest{context, resultHandler}));

    // shall be a new entry
    Q_ASSERT(res.second);
}

void Assistant::requestFinished(QNetworkReply *reply)
{
    // we shall have a remembered request
    const auto it = m_promptRequests.find(reply);
    Q_ASSERT(it != m_promptRequests.end());

    // trigger the handler if context is still valid and we had no error
    if (it->second.context && reply->error() == QNetworkReply::NoError) {
        // extract the answer string
        const auto result = QJsonDocument::fromJson(reply->readAll()).object();
        const auto answer = result[u"response"_s].toString();

        // call our handler
        it->second.resultHandler(answer);
    }

    // show the error if any to the user
    if (reply->error() != QNetworkReply::NoError) {
        Utils::showMessage(i18n("Failed to send prompt to service.\nError: %1", reply->errorString()), QIcon(), i18n("Assistant"), MessageType::Error);
    }

    // ensure the reply will be deleted
    reply->deleteLater();

    // de-register the reply
    m_promptRequests.erase(it);
}

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

#include "assistant.moc"
