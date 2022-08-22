/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "ktexteditor_utils.h"

#include <KActionCollection>
#include <KXMLGUIFactory>
#include <QMimeDatabase>

namespace Utils
{

QIcon iconForDocument(KTextEditor::Document *doc)
{
    // simple modified indicator if modified
    QIcon icon;
    if (doc->isModified()) {
        icon = QIcon::fromTheme(QStringLiteral("modified"));
    }

    // else mime-type icon
    else {
        icon = QIcon::fromTheme(QMimeDatabase().mimeTypeForName(doc->mimeType()).iconName());
    }

    // ensure we always have a valid icon
    if (icon.isNull()) {
        icon = QIcon::fromTheme(QStringLiteral("text-plain"));
    }
    return icon;
}

QAction *toolviewShowAction(KXMLGUIClient *client, const QString &toolviewName)
{
    if (!client) {
        qWarning() << Q_FUNC_INFO << "invalid null client, toolviewName: " << toolviewName;
        Q_ASSERT(false);
        return nullptr;
    }

    static const QString prefix = QStringLiteral("kate_mdi_toolview_");
    if (client->componentName() == QStringLiteral("toolviewmanager")) {
        return client->actionCollection()->action(prefix + toolviewName);
    }

    KXMLGUIClient *toolviewmanager = nullptr;
    if (!client->factory()) {
        qWarning() << Q_FUNC_INFO << "don't have a client factory, toolviewName: " << toolviewName;
        return nullptr;
    }
    const auto clients = client->factory()->clients();
    for (auto client : clients) {
        if (client->componentName() == QStringLiteral("toolviewmanager")) {
            toolviewmanager = client;
            break;
        }
    }

    if (!toolviewmanager) {
        qWarning() << Q_FUNC_INFO << "Unexpected unable to find toolviewmanager KXMLGUIClient, toolviewName: " << toolviewName;
        return nullptr;
    }

    return toolviewmanager->actionCollection()->action(prefix + toolviewName);
}
}
