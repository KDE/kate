/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateappadaptor.h"

#include "kateapp.h"
#include "katesessionmanager.h"

#include "katedebug.h"

#include <QApplication>

/**
 * add the adapter to the global application instance to have
 * it auto-register with KDBusService, see bug 410742
 */
KateAppAdaptor::KateAppAdaptor()
    : QDBusAbstractAdaptor(qApp)
{
}

void KateAppAdaptor::activate(const QString &token)
{
    KateApp::self()->activate(token);
}

bool KateAppAdaptor::openUrl(const QString &url, const QString &encoding)
{
    return KateApp::self()->openDocUrl(QUrl(url), encoding, false);
}

bool KateAppAdaptor::openUrl(const QString &url, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE, "openURL");

    return KateApp::self()->openDocUrl(QUrl(url), encoding, isTempFile);
}

//-----------
QString KateAppAdaptor::tokenOpenUrl(const QString &url, const QString &encoding)
{
    KTextEditor::Document *doc = KateApp::self()->openDocUrl(QUrl(url), encoding, false);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    return QStringLiteral("%1").arg(reinterpret_cast<qptrdiff>(doc));
}

QString KateAppAdaptor::tokenOpenUrl(const QString &url, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE, "openURL");
    KTextEditor::Document *doc = KateApp::self()->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    return QStringLiteral("%1").arg(reinterpret_cast<qptrdiff>(doc));
}

QString KateAppAdaptor::tokenOpenUrlAt(const QString &url, int line, int column, const QString &encoding, bool isTempFile)
{
    qCDebug(LOG_KATE, "openURLAt");
    KTextEditor::Document *doc = KateApp::self()->openDocUrl(QUrl(url), encoding, isTempFile);
    if (!doc) {
        return QStringLiteral("ERROR");
    }
    KateApp::self()->setCursor(line, column);
    return QStringLiteral("%1").arg(reinterpret_cast<qptrdiff>(doc));
}
//--------

bool KateAppAdaptor::setCursor(int line, int column)
{
    return KateApp::self()->setCursor(line, column);
}

bool KateAppAdaptor::openInput(const QString &text, const QString &encoding)
{
    return KateApp::self()->openInput(text, encoding);
}

bool KateAppAdaptor::activateSession(const QString &session)
{
    return KateApp::self()->sessionManager()->activateSession(session);
}

qint64 KateAppAdaptor::lastActivationChange() const
{
    return KateApp::self()->lastActivationChange();
}

QString KateAppAdaptor::activeSession() const
{
    return KateApp::self()->sessionManager()->activeSession()->name();
}

void KateAppAdaptor::emitExiting()
{
    Q_EMIT exiting();
}

void KateAppAdaptor::emitDocumentClosed(const QString &token)
{
    documentClosed(token);
}

#include "moc_kateappadaptor.cpp"
