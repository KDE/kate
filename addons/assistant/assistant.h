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

#include <QPointer>

class Assistant : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit Assistant(QObject *parent, const QVariantList & = QVariantList());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
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
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QPointer<KTextEditor::Message> m_infoMessage;
};
