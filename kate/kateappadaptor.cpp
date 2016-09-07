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
#include "katesessionmanager.h"
#include "katedocmanager.h"
#include "katemainwindow.h"

#include "katedebug.h"
#include <KWindowSystem>

KateAppAdaptor::KateAppAdaptor(KateApp *app)
    : QDBusAbstractAdaptor(app)
    , m_app(app)
{}

void KateAppAdaptor::activate()
{
    KateMainWindow *win = m_app->activeKateMainWindow();
    if (!win) {
        return;
    }

    win->show();
    win->activateWindow();
    win->raise();

    KWindowSystem::forceActiveWindow(win->winId());
    KWindowSystem::raiseWindow(win->winId());
    KWindowSystem::demandAttention(win->winId());
}

bool KateAppAdaptor::openUrl(QString url, QString encoding)
{
    return m_app->openUrl(QUrl(url), encoding, false);
}

bool KateAppAdaptor::openUrl(QString url, QString encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURL";

    return m_app->openUrl(QUrl(url), encoding, isTempFile);
}

//-----------
QString KateAppAdaptor::tokenOpenUrl(QString url, QString encoding)
{
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, false);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    return QString::fromLatin1("%1").arg((qptrdiff)doc);
}

QString KateAppAdaptor::tokenOpenUrl(QString url, QString encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURL";
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    return QString::fromLatin1("%1").arg((qptrdiff)doc);
}

QString KateAppAdaptor::tokenOpenUrlAt(QString url, int line, int column, QString encoding, bool isTempFile)
{
    qCDebug(LOG_KATE) << "openURLAt";
    KTextEditor::Document *doc = m_app->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    m_app->setCursor(line, column);
    return QString::fromLatin1("%1").arg((qptrdiff)doc);
}
//--------

bool KateAppAdaptor::setCursor(int line, int column)
{
    return m_app->setCursor(line, column);
}

bool KateAppAdaptor::openInput(QString text, QString encoding)
{
    return m_app->openInput(text, encoding);
}

bool KateAppAdaptor::activateSession(QString session)
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

