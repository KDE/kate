/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "ktexteditor_utils.h"
#include "diagnostics/diagnosticview.h"
#include "diffwidget.h"

#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QMimeDatabase>
#include <QPointer>
#include <QScrollBar>
#include <QVariant>

#include <KActionCollection>
#include <KLocalizedString>
#include <KMultiTabBar>
#include <KNetworkMounts>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

namespace Utils
{

class KateScrollBarRestorerPrivate
{
public:
    explicit KateScrollBarRestorerPrivate(KTextEditor::View *view)
    {
        // Find KateScrollBar
        const QList<QScrollBar *> scrollBars = view->findChildren<QScrollBar *>();
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

KATE_PRIVATE_EXPORT KTextEditor::Range getVisibleRange(KTextEditor::View *view)
{
    Q_ASSERT(view);
    auto doc = view->document();
    auto first = view->firstDisplayedLine();
    auto last = view->lastDisplayedLine();
    auto lastLineLen = doc->line(last).size();
    return KTextEditor::Range(first, 0, last, lastLineLen);
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
        const QString mimeType = doc->mimeType();
        // Shortcut: common case, don't initialize the mime database.
        if (mimeType != QLatin1String("text/plain")) {
            icon = QIcon::fromTheme(QMimeDatabase().mimeTypeForName(mimeType).iconName());
        }
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

    const QList<KXMLGUIClient *> clients = mainWindow->guiFactory()->clients();
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

QWidget *toolviewForName(KTextEditor::MainWindow *mainWindow, const QString &toolviewName)
{
    QWidget *toolView = nullptr;
    QMetaObject::invokeMethod(mainWindow->parent(), "toolviewForName", Qt::DirectConnection, Q_RETURN_ARG(QWidget *, toolView), Q_ARG(QString, toolviewName));
    return toolView;
}

QWidget *activeToolviewForSide(KTextEditor::MainWindow *mainWindow, int side)
{
    QWidget *toolView = nullptr;
    QMetaObject::invokeMethod(mainWindow->parent(), "activeViewToolView", Qt::DirectConnection, Q_RETURN_ARG(QWidget *, toolView), Q_ARG(int, side));
    return toolView;
}

void showMessage(const QString &message, const QIcon &icon, const QString &category, MessageType type, KTextEditor::MainWindow *mainWindow)
{
    Q_ASSERT(type >= MessageType::Log && type <= MessageType::Error);
    QVariantMap msg;
    static const QString msgToString[] = {
        QStringLiteral("Log"),
        QStringLiteral("Info"),
        QStringLiteral("Warning"),
        QStringLiteral("Error"),
    };
    msg.insert(QStringLiteral("type"), msgToString[type]);
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
    bool ret = true;
    QMetaObject::invokeMethod(mainWindow->parent(), "showMessage", Qt::DirectConnection, Q_RETURN_ARG(bool, ret), Q_ARG(QVariantMap, map));
}

void showDiff(const QByteArray &diff, const DiffParams &params, KTextEditor::MainWindow *mainWindow)
{
    DiffWidgetManager::openDiff(diff, params, mainWindow);
}

QWidgetList widgets(KTextEditor::MainWindow *mainWindow)
{
    QWidgetList ret;
    QMetaObject::invokeMethod(mainWindow->parent(), "widgets", Qt::DirectConnection, Q_RETURN_ARG(QWidgetList, ret));
    return ret;
}

void insertWidgetInStatusbar(QWidget *widget, KTextEditor::MainWindow *mainWindow)
{
    QMetaObject::invokeMethod(mainWindow->parent(), "insertWidgetInStatusbar", Qt::DirectConnection, Q_ARG(QWidget *, widget));
}

void addPositionToHistory(const QUrl &url, KTextEditor::Cursor c, KTextEditor::MainWindow *mainWindow)
{
    QMetaObject::invokeMethod(mainWindow->parent(), "addPositionToHistory", Qt::DirectConnection, Q_ARG(QUrl, url), Q_ARG(KTextEditor::Cursor, c));
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

QString niceFileNameWithPath(const QUrl &url)
{
    // we want some filename @ folder output to have chance to keep important stuff even on elide
    if (url.isLocalFile()) {
        // perhaps shorten the path
        const QString homePath = QDir::homePath();
        QString path = url.toString(QUrl::RemoveFilename | QUrl::PreferLocalFile | QUrl::StripTrailingSlash);
        if (path.startsWith(homePath)) {
            path = QLatin1String("~") + path.right(path.length() - homePath.length());
        }
        return url.fileName() + QStringLiteral(" @ ") + path;
    }
    return url.toDisplayString();
}

QUrl normalizeUrl(QUrl url)
{
    // ensure proper file:// url if in doubt because no scheme set
    if (url.isRelative()) {
        url.setScheme(QStringLiteral("file"));
    }

    // Resolve symbolic links for local files
    if (url.isLocalFile() && !KNetworkMounts::self()->isOptionEnabledForPath(url.toLocalFile(), KNetworkMounts::StrongSideEffectsOptimizations)) {
        QString normalizedUrl = QFileInfo(url.toLocalFile()).canonicalFilePath();
        if (!normalizedUrl.isEmpty()) {
            return QUrl::fromLocalFile(normalizedUrl);
        }
    }

    // else: cleanup only the .. stuff
    return url.adjusted(QUrl::NormalizePathSegments);
}

QUrl absoluteUrl(QUrl url)
{
    // ensure proper file:// url if in doubt because no scheme set
    if (url.isRelative()) {
        url.setScheme(QStringLiteral("file"));
    }

    // Get absolute path if local file
    if (url.isLocalFile()) {
        return QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absoluteFilePath());
    }

    // else: cleanup only the .. stuff
    return url.adjusted(QUrl::NormalizePathSegments);
}
}
