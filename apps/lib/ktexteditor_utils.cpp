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

QAction *toolviewShowAction(KTextEditor::MainWindow *mainWindow, const QString &toolviewName)
{
    Q_ASSERT(mainWindow);

    const auto clients = mainWindow->guiFactory()->clients();
    static const QString prefix = QStringLiteral("kate_mdi_toolview_");
    auto it = std::find_if(clients.begin(), clients.end(), [](const KXMLGUIClient *c) {
        return c->componentName() == QStringLiteral("toolviewmanager");
    });

    if (it == clients.end()) {
        qWarning() << Q_FUNC_INFO << "Unexpected unable to find toolviewmanager KXMLGUIClient, toolviewName: " << toolviewName;
        return nullptr;
    }
    return (*it)->actionCollection()->action(prefix + toolviewName);
}
}
