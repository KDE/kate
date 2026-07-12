/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientchatdock.h"
#include "acpclient_debug.h"
#include "acpclientchatwidget.h"
#include "acpclientplugin.h"

#include <KLocalizedString>

#include <QVBoxLayout>

ACPClientChatDock::ACPClientChatDock(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QDockWidget(i18n("ACP Chat"), parent)
    , m_plugin(plugin)
{
    qCDebug(ACPCLIENT) << "ACPClientChatDock created";

    setObjectName(QStringLiteral("ACPChatDock"));

    // Create the chat widget
    m_chatWidget = new ACPClientChatWidget(plugin, mainWindow, this);

    // Set up the dock layout
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setSpacing(0);
    layout->addWidget(m_chatWidget);

    setWidget(container);

    // Connect visibility signal
    connect(this, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        Q_EMIT visibilityChanged(visible);
    });
}

ACPClientChatDock::~ACPClientChatDock()
{
    qCDebug(ACPCLIENT) << "ACPClientChatDock destroyed";
    delete m_chatWidget;
}

ACPClientChatWidget *ACPClientChatDock::chatWidget() const
{
    return m_chatWidget;
}

void ACPClientChatDock::showChat(bool show)
{
    setVisible(show);
}
