/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "ktexteditor_utils.h"
#include "katemainwindow.h"

#include <QFontDatabase>
#include <QIcon>
#include <QMimeDatabase>
#include <QPointer>
#include <QScrollBar>
#include <QVariant>

#include <KActionCollection>
#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

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

void showMessage(const QString &message, const QIcon &icon, const QString &category, const QString &type, KTextEditor::MainWindow *mainWindow)
{
    QVariantMap msg;
    msg.insert(QStringLiteral("type"), type);
    msg.insert(QStringLiteral("category"), category);
    msg.insert(QStringLiteral("categoryIcon"), icon);
    msg.insert(QStringLiteral("text"), message);
    showMessage(msg, mainWindow);
}

void showMessage(const QVariantMap &map, KTextEditor::MainWindow *mainWindow)
{
    if (!mainWindow) {
        mainWindow = KTextEditor::Editor::instance()->application()->activeMainWindow();
    }
    if (auto kmw = qobject_cast<KateMainWindow *>(mainWindow->window())) {
        kmw->showMessage(map);
    }
}

void showDiff(const QByteArray &diff, const DiffParams &params, KTextEditor::MainWindow *mainWindow)
{
    if (auto kmw = qobject_cast<KateMainWindow *>(mainWindow->window())) {
        kmw->showDiff(diff, params);
    }
}

void addWidget(QWidget *widget, KTextEditor::MainWindow *mainWindow)
{
    if (auto kmw = qobject_cast<KateMainWindow *>(mainWindow->window())) {
        kmw->addWidget(widget);
    }
}

QString projectBaseDirForDocument(KTextEditor::Document *doc)
{
    QString baseDir;
    if (QObject *plugin = KTextEditor::Editor::instance()->application()->plugin(QStringLiteral("kateprojectplugin"))) {
        QMetaObject::invokeMethod(plugin, "projectBaseDirForDocument", Q_RETURN_ARG(QString, baseDir), Q_ARG(KTextEditor::Document *, doc));
    }
    return baseDir;
}

QVariantMap projectMapForDocument(KTextEditor::Document *doc)
{
    QVariantMap projectMap;
    if (QObject *plugin = KTextEditor::Editor::instance()->application()->plugin(QStringLiteral("kateprojectplugin"))) {
        QMetaObject::invokeMethod(plugin, "projectMapForDocument", Q_RETURN_ARG(QVariantMap, projectMap), Q_ARG(KTextEditor::Document *, doc));
    }
    return projectMap;
}
}
