/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateoutputview.h"

#include <QTreeView>
#include <QVBoxLayout>

KateOutputView::KateOutputView(KateMainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
{
    // simple vbox layout with just the tree view ATM
    // TODO: e.g. filter and such!
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_messagesTreeView = new QTreeView(this);
    m_messagesTreeView->setModel(&m_messagesModel);
    layout->addWidget(m_messagesTreeView);
}

void KateOutputView::slotMessage(const QVariantMap &message)
{
    // first dummy implementation: just add message 1:1 as text to output
    if (message.contains(QStringLiteral("plainText"))) {
        m_messagesModel.appendRow(new QStandardItem(message.value(QStringLiteral("plainText")).toString().trimmed()));
    }
}
