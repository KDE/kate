/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIClient>

#include <QNetworkAccessManager>
#include <QPointer>

#include <functional>
#include <unordered_map>

class Assistant : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit Assistant(QObject *parent, const QVariantList & = QVariantList());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    void sendPrompt(const QString &prompt, const QObject *context, const std::function<void(const QString &QString)> &resultHandler);

private:
    // handle arriving result for sent prompts
    void requestFinished(QNetworkReply *reply);

private:
    // own access manager to handle the async requests
    QNetworkAccessManager m_accessManager;

    // keep track of the context and result handler for each request
    struct PromptRequest {
        QPointer<const QObject> context;
        std::function<void(const QString &QString)> resultHandler;
    };
    std::unordered_map<QNetworkReply *, PromptRequest> m_promptRequests;
};

class AssistantView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit AssistantView(Assistant *plugin, KTextEditor::MainWindow *mainwindow);
    ~AssistantView();

    void openTestDialog();
    void displayMessage(const QString &msg, KTextEditor::Message::MessageType level);

private:
    Assistant *const m_assistant;
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QPointer<KTextEditor::Message> m_infoMessage;
};
