/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateappadaptor.h"

#include "kateapp.h"
#include "katedocmanager.h"
#include "katemainwindow.h"
#include "katesessionmanager.h"

#include "katedebug.h"

// X11 startup handling
#if __has_include(<KStartupInfo>)
#include <KStartupInfo>
#endif

#include <KWindowSystem>

#include <QApplication>

/**
 * add the adapter to the global application instance to have
 * it auto-register with KDBusService, see bug 410742
 */
KateAppAdaptor::KateAppAdaptor(KateApp *app)
    : QDBusAbstractAdaptor(qApp)
    , m_app(app)
{
}

void KateAppAdaptor::activate(const QString &token)
{
    KateMainWindow *win = m_app->activeKateMainWindow();
    if (!win) {
        return;
    }

    // like QtSingleApplication
    win->setWindowState(win->windowState() & ~Qt::WindowMinimized);
    win->raise();
    win->activateWindow();

    // try to raise window, see bug 407288
    if (KWindowSystem::isPlatformX11()) {
#if __has_include(<KStartupInfo>)
        KStartupInfo::setNewStartupId(win->windowHandle(), token.toUtf8());
#endif
    } else if (KWindowSystem::isPlatformWayland()) {
        KWindowSystem::setCurrentXdgActivationToken(token);
    }

    KWindowSystem::activateWindow(win->windowHandle());
}

bool KateAppAdaptor::openUrl(const QString &url, const QString &encoding)
{
    return m_app->openDocUrl(QUrl(url), encoding, false);
}

bool KateAppAdaptor::openUrl(const QString &url, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURL";

    return m_app->openDocUrl(QUrl(url), encoding, isTempFile);
}

bool KateAppAdaptor::isOnActivity(const QString &activity)
{
    return m_app->isOnActivity(activity);
}

//-----------
QString KateAppAdaptor::tokenOpenUrl(const QString &url, const QString &encoding)
{
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, false);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    return QStringLiteral("%1").arg(reinterpret_cast<qptrdiff>(doc));
}

QString KateAppAdaptor::tokenOpenUrl(const QString &url, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURL";
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    return QStringLiteral("%1").arg(reinterpret_cast<qptrdiff>(doc));
}

QString KateAppAdaptor::tokenOpenUrlAt(const QString &url, int line, int column, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURLAt";
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    m_app->setCursor(line, column);
    return QStringLiteral("%1").arg(reinterpret_cast<qptrdiff>(doc));
}
//--------

bool KateAppAdaptor::setCursor(int line, int column)
{
    return m_app->setCursor(line, column);
}

bool KateAppAdaptor::openInput(const QString &text, const QString &encoding)
{
    return m_app->openInput(text, encoding);
}

bool KateAppAdaptor::activateSession(const QString &session)
{
    return m_app->sessionManager()->activateSession(session);
}

int KateAppAdaptor::desktopNumber()
{
    if (KWindowSystem::isPlatformX11()) {
        KWindowInfo appInfo(m_app->activeKateMainWindow()->winId(), NET::WMDesktop);
        return appInfo.desktop();
    }

    return 0;
}

QString KateAppAdaptor::activeSession()
{
    return m_app->sessionManager()->activeSession()->name();
}

void KateAppAdaptor::emitExiting()
{
    Q_EMIT exiting();
}

void KateAppAdaptor::emitDocumentClosed(const QString &token)
{
    documentClosed(token);
}
