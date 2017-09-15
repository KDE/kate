/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_APP_H__
#define __KATE_APP_H__

#include <ktexteditor/application.h>

#include "kateprivate_export.h"
#include "katemainwindow.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "katesessionmanager.h"
#include "kateappadaptor.h"

#include <KConfig>
#include <QList>

class KateSessionManager;
class KateMainWindow;
class KatePluginManager;
class KateDocManager;
class KateAppCommands;
class KateAppAdaptor;
class QCommandLineParser;

/**
 * Kate Application
 * This class represents the core kate application object
 */
class KATE_TESTS_EXPORT KateApp : public QObject
{
    Q_OBJECT

    /**
     * constructors & accessor to app object + plugin interface for it
     */
public:
    /**
     * application constructor
     */
    KateApp(const QCommandLineParser &args);

    /**
     * get kate inited
     * @return false, if application should exit
     */
    bool init();

    /**
     * application destructor
     */
    ~KateApp() override;

    /**
     * static accessor to avoid casting ;)
     * @return app instance
     */
    static KateApp *self();

    /**
     * KTextEditor::Application wrapper
     * @return KTextEditor::Application wrapper.
     */
    KTextEditor::Application *wrapper() {
        return &m_wrapper;
    }

    /**
     * kate init
     */
private:
    /**
     * restore a old kate session
     */
    void restoreKate();

    /**
     * try to start kate
     * @return success, if false, kate should exit
     */
    bool startupKate();

    /**
     * kate shutdown
     */
public:
    /**
     * shutdown kate application
     * @param win mainwindow which is used for dialogs
     */
    void shutdownKate(KateMainWindow *win);

    /**
     * other accessors for global unique instances
     */
public:
    /**
     * accessor to plugin manager
     * @return plugin manager instance
     */
    KatePluginManager *pluginManager();

    /**
     * accessor to document manager
     * @return document manager instance
     */
    KateDocManager *documentManager();

    /**
     * accessor to session manager
     * @return session manager instance
     */
    KateSessionManager *sessionManager();

    /**
     * window management
     */
public:
    /**
     * create a new main window, use given config if any for restore
     * @param sconfig session config object
     * @param sgroup session group for this window
     * @return new constructed main window
     */
    KateMainWindow *newMainWindow(KConfig *sconfig = nullptr, const QString &sgroup = QString());

    /**
     * add the mainwindow given
     * should be called in mainwindow constructor
     * @param mainWindow window to remove
     */
    void addMainWindow(KateMainWindow *mainWindow);

    /**
     * removes the mainwindow given, DOES NOT DELETE IT
     * should be called in mainwindow destructor
     * @param mainWindow window to remove
     */
    void removeMainWindow(KateMainWindow *mainWindow);

    /**
     * give back current active main window
     * can only be 0 at app start or exit
     * @return current active main window
     */
    KateMainWindow *activeKateMainWindow();

    /**
     * give back number of existing main windows
     * @return number of main windows
     */
    int mainWindowsCount() const;

    /**
     * give back the window you want
     * @param n window index
     * @return requested main window
     */
    KateMainWindow *mainWindow(int n);

    int mainWindowID(KateMainWindow *window);

    /**
     * some stuff for the dcop API
     */
public:
    /**
     * open url with given encoding
     * used by kate if --use given
     * @param url filename
     * @param encoding encoding name
     * @return success
     */
    bool openUrl(const QUrl &url, const QString &encoding, bool isTempFile);

    KTextEditor::Document *openDocUrl(const QUrl &url, const QString &encoding, bool isTempFile);

    void emitDocumentClosed(const QString &token);

    /**
     * position cursor in current active view
     * will clear selection
     * @param line line to set
     * @param column column to set
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

    //
    // KTextEditor::Application interface, called by wrappers via invokeMethod
    //
public Q_SLOTS:
    /**
     * Get a list of all main windows.
     * @return all main windows
     */
    QList<KTextEditor::MainWindow *> mainWindows() {
        // assemble right list
        QList<KTextEditor::MainWindow *> windows;
        for (int i = 0; i < m_mainWindows.size(); ++i) {
            windows.push_back(m_mainWindows[i]->wrapper());
        }
        return windows;
    }

    /**
     * Accessor to the active main window.
     * \return a pointer to the active mainwindow
     */
    KTextEditor::MainWindow *activeMainWindow() {
        // either return wrapper or nullptr
        if (KateMainWindow *a = activeKateMainWindow()) {
            return a->wrapper();
        }
        return nullptr;
    }

    /**
     * Get a list of all documents that are managed by the application.
     * This might contain less documents than the editor has in his documents () list.
     * @return all documents the application manages
     */
    QList<KTextEditor::Document *> documents() {
        return m_docManager.documentList();
    }

    /**
     * Get the document with the URL \p url.
     * if multiple documents match the searched url, return the first found one...
     * \param url the document's URL
     * \return the document with the given \p url or NULL, if none found
     */
    KTextEditor::Document *findUrl(const QUrl &url) {
        return m_docManager.findDocument(url);
    }

    /**
     * Open the document \p url with the given \p encoding.
     * if the url is empty, a new empty document will be created
     * \param url the document's url
     * \param encoding the preferred encoding. If encoding is QString() the
     *        encoding will be guessed or the default encoding will be used.
     * \return a pointer to the created document
     */
    KTextEditor::Document *openUrl(const QUrl &url, const QString &encoding = QString()) {
        return m_docManager.openUrl(url, encoding);
    }

    /**
     * Close the given \p document. If the document is modified, user will be asked if he wants that.
     * \param document the document to be closed
     * \return \e true on success, otherwise \e false
     */
    bool closeDocument(KTextEditor::Document *document) {
        return m_docManager.closeDocument(document);
    }

    /**
     * Close a list of documents. If any of them are modified, user will be asked if he wants that.
     * Use this, if you want to close multiple documents at once, as the application might
     * be able to group the "do you really want that" dialogs into one.
     * \param documents list of documents to be closed
     * \return \e true on success, otherwise \e false
     */
    bool closeDocuments(const QList<KTextEditor::Document *> &documents) {
        return m_docManager.closeDocumentList(documents);
    }

    /**
     * Get a plugin for the plugin with with identifier \p name.
     * \param name the plugin's name
     * \return pointer to the plugin if a plugin with \p name is loaded, otherwise nullptr
     */
    KTextEditor::Plugin *plugin(const QString &name);

    /**
     * Ask app to quit. The app might interact with the user and decide that
     * quitting is not possible and return false.
     *
     * \return true if the app could quit
     */
    bool quit()
    {
        shutdownKate(activeKateMainWindow());
        return true;
    }

    /**
     * A message is received from an external instance, if we use QtSingleApplication
     *
     * \p message is a serialized message (at the moment just the file list separated by ';')
     * \p socket is the QLocalSocket used for the communication
     */
    void remoteMessageReceived(const QString &message, QObject *socket);

protected:
    /**
     * Event filter for QApplication to handle mac os like file open
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    /**
     * kate's command line args
     */
    const QCommandLineParser &m_args;

    /**
     * known main windows
     */
    QList<KateMainWindow *> m_mainWindows;

    /**
     * Wrapper of application for KTextEditor
     */
    KTextEditor::Application m_wrapper;

    /**
     * document manager
     */
    KateDocManager m_docManager;

    /**
     * dbus interface
     */
    KateAppAdaptor m_adaptor;

    /**
     * plugin manager
     */
    KatePluginManager m_pluginManager;

    /**
     * session manager
     */
    KateSessionManager m_sessionManager;
};

#endif
