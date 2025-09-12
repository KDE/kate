/*
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDBusAbstractAdaptor>

class KateAppAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Kate.Application")
    Q_PROPERTY(QString activeSession READ activeSession)
    Q_PROPERTY(qint64 lastActivationChange READ lastActivationChange)
public:
    explicit KateAppAdaptor();

    /**
     * emit the exiting signal
     */
    void emitExiting();
    void emitDocumentClosed(const QString &token);

public Q_SLOTS:
    /**
     * open a file with given url and encoding
     * will get view created
     * @param url url of the file
     * @param encoding encoding name
     * @return success
     */
    bool openUrl(const QString &url, const QString &encoding);

    /**
     * open a file with given url and encoding
     * will get view created
     * @param url url of the file
     * @param encoding encoding name
     * @return token or ERROR
     */
    QString tokenOpenUrl(const QString &url, const QString &encoding);

    /**
     * Like the above, but adds an option to let the documentManager know
     * if the file should be deleted when closed.
     * @p isTempFile should be set to true with the --tempfile option set ONLY,
     * files opened with this set to true will be deleted when closed.
     */
    bool openUrl(const QString &url, const QString &encoding, bool isTempFile);

    QString tokenOpenUrl(const QString &url, const QString &encoding, bool isTempFile);

    QString tokenOpenUrlAt(const QString &url, int line, int column, const QString &encoding, bool isTempFile);

    /**
     * set cursor of active view in active main window
     * will clear selection
     * @param line line for cursor
     * @param column column for cursor
     * @return success
     */
    bool setCursor(int line, int column);

    /**
     * helper to handle stdin input
     * open a new document/view, fill it with the text given
     * @param text text to fill in the new doc/view
     * @param encoding encoding to set for the document, if any
     * @return success
     */
    bool openInput(const QString &text, const QString &encoding);

    /**
     * activate a given session
     * @param session session name
     * @return success
     */
    bool activateSession(const QString &session);

    /**
     * activate this kate instance
     */
    void activate(const QString &token = QString());

Q_SIGNALS:
    /**
     * Notify the world that this kate instance is exiting.
     * All apps should stop using the dbus interface of this instance after this signal got emitted.
     */
    void exiting();
    void documentClosed(const QString &token);

public:
    QString activeSession() const;

    /**
     * last time some QEvent::ActivationChange occurred, negated if no windows on the current active desktop
     * used to determine which instance to reuse, if we have multiple
     */
    qint64 lastActivationChange() const;
};
