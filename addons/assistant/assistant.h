/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "assistantcommand.h"

#include <KTextEditor/Plugin>

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

    /**
     * Read config from config file.
     * Will emit configUpdated().
     */
    void readConfig();

    /**
     * Write config to config file.
     * Will emit configUpdated().
     */
    void writeConfig();

    /**
     * Public accessible config.
     * Use readConfig/writeConfig if needed to read/write them back.
     */
    struct {
        // url for completion API, e.g. http://localhost:11434/api/generate
        QUrl urlCompletion;

        // model to use for completion, e.g. llama3.2
        QString modelCompletion;
    } config;

Q_SIGNALS:
    /**
     * Signal that plugin configuration changed
     */
    void configUpdated();

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

    // register our text editor commands
    const AssistantCommand m_cmd{this};
};
