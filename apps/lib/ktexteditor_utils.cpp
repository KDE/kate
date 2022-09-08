/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "ktexteditor_utils.h"

#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QFontDatabase>
#include <QIcon>
#include <QPointer>
#include <QScrollBar>

#include <KActionCollection>
#include <KXMLGUIFactory>
#include <QMimeDatabase>

namespace Utils
{

class KateScrollBarRestorerPrivate
{
public:
    KateScrollBarRestorerPrivate(KTextEditor::View *view)
    {
        // Find KateScrollBar
        const auto scrollBars = view->findChildren<QScrollBar *>();
        kateScrollBar = [scrollBars] {
            for (auto scrollBar : scrollBars) {
                if (qstrcmp(scrollBar->metaObject()->className(), "KateScrollBar") == 0) {
                    return scrollBar;
                }
            }
            return static_cast<QScrollBar *>(nullptr);
        }();

        if (kateScrollBar) {
            oldScrollValue = kateScrollBar->value();
        }
    }

    void restore()
    {
        if (restored) {
            return;
        }

        if (kateScrollBar) {
            kateScrollBar->setValue(oldScrollValue);
        }
        restored = true;
    }

    ~KateScrollBarRestorerPrivate()
    {
        restore();
    }

private:
    QPointer<QScrollBar> kateScrollBar = nullptr;
    int oldScrollValue = 0;
    bool restored = false;
};

KateScrollBarRestorer::KateScrollBarRestorer(KTextEditor::View *view)
    : d(new KateScrollBarRestorerPrivate(view))
{
}

void KateScrollBarRestorer::restore()
{
    d->restore();
}

KateScrollBarRestorer::~KateScrollBarRestorer()
{
    delete d;
}

QFont editorFont()
{
    if (KTextEditor::Editor::instance()) {
        return KTextEditor::Editor::instance()->font();
    }
    qWarning() << __func__ << "Editor::instance() is null! falling back to system fixed font";
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}

QFont viewFont(KTextEditor::View *view)
{
    if (const auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(view)) {
        return ciface->configValue(QStringLiteral("font")).value<QFont>();
    }
    return editorFont();
}

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
