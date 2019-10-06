/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateappadaptor.h"

#include "kateapp.h"
#include "katedocmanager.h"
#include "katemainwindow.h"
#include "katesessionmanager.h"

#include "katedebug.h"

#include <KStartupInfo>
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

void KateAppAdaptor::activate()
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
    KStartupInfo::setNewStartupId(win, KStartupInfo::startupId());
    KWindowSystem::activateWindow(win->effectiveWinId());
}

bool KateAppAdaptor::openUrl(const QString &url, const QString &encoding)
{
    return m_app->openUrl(QUrl(url), encoding, false);
}

bool KateAppAdaptor::openUrl(const QString &url, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURL";

    return m_app->openUrl(QUrl(url), encoding, isTempFile);
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
    return QStringLiteral("%1").arg((qptrdiff)doc);
}

QString KateAppAdaptor::tokenOpenUrl(const QString &url, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURL";
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    return QStringLiteral("%1").arg((qptrdiff)doc);
}

QString KateAppAdaptor::tokenOpenUrlAt(const QString &url, int line, int column, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURLAt";
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    m_app->setCursor(line, column);
    return QStringLiteral("%1").arg((qptrdiff)doc);
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
    KWindowInfo appInfo(m_app->activeKateMainWindow()->winId(), NET::WMDesktop);
    return appInfo.desktop();
}

QString KateAppAdaptor::activeSession()
{
    return m_app->sessionManager()->activeSession()->name();
}

void KateAppAdaptor::emitExiting()
{
    emit exiting();
}

void KateAppAdaptor::emitDocumentClosed(const QString &token)
{
    documentClosed(token);
}
