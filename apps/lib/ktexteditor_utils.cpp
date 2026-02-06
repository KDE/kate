/*
    SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "ktexteditor_utils.h"
#include "diffwidget.h"
#include "exec_io_utils.h"
#include "kateapp.h"

#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QMimeDatabase>
#include <QPointer>
#include <QScrollBar>
#include <QTimer>
#include <QVariant>

#include <KActionCollection>
#include <KLocalizedString>
#include <KMultiTabBar>
#include <KNetworkMounts>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

#ifdef HAVE_MALLOC_TRIM
#include <malloc.h>
#endif

static bool isKateApp()
{
    static const bool isKateApp = qobject_cast<KateApp *>(KTextEditor::Editor::instance()->application()->parent()) != nullptr;
    return isKateApp;
}

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
    qWarning("%s Editor::instance() is null! falling back to system fixed font", __func__);
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
    // Pinned icon if the doc is pinned
    else if (isKateApp() && KateApp::self()->documentManager()->isDocumentPinned(doc)) {
        icon = QIcon::fromTheme(QStringLiteral("pin"));
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
        return c->componentName() == QLatin1String("toolviewmanager");
    });

    if (it == clients.end()) {
        qWarning("%s Unexpected unable to find toolviewmanager KXMLGUIClient, toolviewName: %ls", Q_FUNC_INFO, qUtf16Printable(toolviewName));
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

static void showMessage(KTextEditor::MainWindow *mainWindow, const QString &text, KTextEditor::Message::MessageType level)
{
    KTextEditor::View *view = mainWindow->activeView();
    if (!view || !view->document()) {
        return;
    }

    auto kmsg = new KTextEditor::Message(text, level);
    kmsg->setPosition(KTextEditor::Message::BottomInView);
    kmsg->setAutoHide(500);
    kmsg->setView(view);
    view->document()->postMessage(kmsg);
}

static bool goToDocumentLocation(KTextEditor::MainWindow *mainWindow,
                                 KTextEditor::View *targetView,
                                 const KTextEditor::Range &location,
                                 KTextEditor::View *activeView,
                                 const GoToOptions &options)
{
    auto targetDoc = targetView->document();
    // check if document is really loaded by now
    if (!targetDoc || targetDoc->lines() == 0 || !targetDoc->isReadWrite())
        return false;

    KTextEditor::Cursor cdef = location.start();
    if (!cdef.isValid())
        return true;
    if (options.record) {
        if (activeView) {
            // save current position for location history
            Utils::addPositionToHistory(activeView->document()->url(), activeView->cursorPosition(), mainWindow);
        }
        // save the position to which we are jumping in location history
        Utils::addPositionToHistory(targetView->document()->url(), cdef, mainWindow);
    }
    targetView->setCursorPosition(cdef);
    if (options.highlight) {
        highlightLandingLocation(targetView, location);
    }
    if (options.focus) {
        mainWindow->window()->raise();
        mainWindow->window()->setFocus();
    }

    return true;
}

void goToDocumentLocation(KTextEditor::MainWindow *mainWindow, const QUrl &uri, const std::optional<KTextEditor::Range> &location, const GoToOptions &options)
{
    KTextEditor::Cursor cdef = location ? location->start() : KTextEditor::Cursor();
    if (uri.isEmpty() || !cdef.isValid()) {
        return;
    }

    KTextEditor::View *activeView = mainWindow->activeView();
    KTextEditor::Document *document = activeView ? activeView->document() : nullptr;
    KTextEditor::View *targetView = nullptr;
    if (document && uri == document->url()) {
        targetView = activeView;
    } else {
        // skip remote unmapped path url
        if (uri.scheme() == ExecPrefixManager::scheme() && uri.host().isEmpty()) {
            showMessage(mainWindow, i18n("Can not open %1", uri.toDisplayString()), KTextEditor::Message::Information);
            return;
        }
        targetView = mainWindow->openUrl(uri);
    }

    // try to go at once
    if (targetView && location && !goToDocumentLocation(mainWindow, targetView, *location, activeView, options)) {
        // setup retry in case delayed non-file load
        auto timer = new QTimer(mainWindow);
        auto h = [mainWindow, targetView, activeView = QPointer(activeView), location, options, timer, count = 0]() mutable {
            // mainWindow is parent, targetView is receiver
            // so still need to check activeView
            if (activeView.isNull())
                return;
            // note; opening remote url temporarily positions cursor at top of that new view
            // so try to avoid that in the position history by using the current view instead
            if (goToDocumentLocation(mainWindow, targetView, *location, activeView, options) || ++count > 5) {
                timer->deleteLater();
            }
        };
        QObject::connect(timer, &QTimer::timeout, targetView, h);
        timer->start(500);
    }
}

void highlightLandingLocation(KTextEditor::View *view, const KTextEditor::Range &location)
{
    auto doc = view->document();
    if (!doc) {
        return;
    }
    auto mr = doc->newMovingRange(location);
    KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute);
    attr->setUnderlineStyle(QTextCharFormat::SingleUnderline);
    mr->setView(view);
    mr->setAttribute(attr);
    QTimer::singleShot(1000, doc, [mr] {
        mr->setRange(KTextEditor::Range::invalid());
        delete mr;
    });
}

void showMessage(const QString &message, const QIcon &icon, const QString &category, MessageType type, KTextEditor::MainWindow *mainWindow)
{
    if (!mainWindow) {
        mainWindow = KTextEditor::Editor::instance()->application()->activeMainWindow();
    }

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
    mainWindow->showMessage(msg);
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
    if (url.isLocalFile()) {
        // Resolve symbolic links for local files
        auto localFile = url.toLocalFile();
        if (!KNetworkMounts::self()->isOptionEnabledForPath(localFile, KNetworkMounts::StrongSideEffectsOptimizations)) {
            const auto normalizedUrl = QFileInfo(localFile).canonicalFilePath();
            if (!normalizedUrl.isEmpty()) {
                localFile = normalizedUrl;
            }
        }

        // ensure we always do basic stuff like uppercase drive letters, bug 509085
        return QUrl::fromLocalFile(QFileInfo(localFile).absoluteFilePath());
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

bool isDocumentPinned(KTextEditor::Document *doc)
{
    return isKateApp() && KateApp::self()->documentManager()->isDocumentPinned(doc);
}

void togglePinDocument(KTextEditor::Document *document)
{
    if (isKateApp()) {
        KateApp::self()->documentManager()->togglePinDocument(document);
    }
}

void releaseMemoryToOperatingSystem()
{
#ifdef HAVE_MALLOC_TRIM
    malloc_trim(0);
#endif
}

QString fileNameFromPath(const QString &path)
{
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    return lastSlash == -1 ? path : path.mid(lastSlash + 1);
}

QString formatUrl(const QUrl &url)
{
    return url.toString(QUrl::PreferLocalFile | QUrl::RemovePassword);
}
}
