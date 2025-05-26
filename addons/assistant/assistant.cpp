/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "assistant.h"
#include "assistantview.h"

#include <ktexteditor_utils.h>

#include <KPluginFactory>

#include <QJsonDocument>
#include <QNetworkReply>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

using namespace Qt::Literals::StringLiterals;

K_PLUGIN_FACTORY_WITH_JSON(AssistantFactory, "plugin.json", registerPlugin<Assistant>();)

Assistant::Assistant(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
    // init api config
    readConfig();

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
    request.setUrl(config.urlCompletion);

    // setup prompt json
    QJsonObject json{
        {u"prompt"_s, prompt},
        {u"model"_s, config.modelCompletion},
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

void Assistant::readConfig()
{
    const KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("assistant"));

    // configure completion api
    config.urlCompletion = cg.readEntry("urlCompletion", QUrl(u"http://localhost:11434/api/generate"_s));
    config.modelCompletion = cg.readEntry("modelCompletion", u"llama3.2"_s);

    // inform about potential changes
    Q_EMIT configUpdated();
}

void Assistant::writeConfig()
{
    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("assistant"));

    // configure completion api
    cg.writeEntry("urlCompletion", config.urlCompletion);
    cg.writeEntry("modelCompletion", config.modelCompletion);

    // inform about potential changes
    Q_EMIT configUpdated();
}

#include "assistant.moc"
