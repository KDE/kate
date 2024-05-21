/*
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <ktexteditor/application.h>

#ifdef WITH_KUSERFEEDBACK
#include <KUserFeedback/Provider>
#endif

#ifdef WITH_DBUS
#include "kateappadaptor.h"
#endif

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateprivate_export.h"
#include "katesessionmanager.h"
#include "katestashmanager.h"

#include <KConfig>
#include <QList>

class KAboutData;
class KateSessionManager;
class KateMainWindow;
class KatePluginManager;
class KateDocManager;
class KateAppCommands;
class QCommandLineParser;

/**
 * Kate Application
 * This class represents the core kate application object
 */
class KATE_PRIVATE_EXPORT KateApp : public QObject
{
    Q_OBJECT

    /**
     * constructors & accessor to app object + plugin interface for it
     */
public:
    /**
     * Do the needed setup steps before we create our QApplication.
     * @param detach shall we detach from the terminal?
     */
    static void initPreApplicationCreation(bool detach);

    /**
     * Was the application started inside a terminal?
     */
    static bool isInsideTerminal();

    /**
     *  Are we full featured Kate or the simplified KWrite?
     */
    enum ApplicationMode { ApplicationKate, ApplicationKWrite };

    /**
     * application constructor
     * @param arg command line parser, reference will be stored, needs to survive as long as application object is alive
     * @param mode application mode, either Kate or KWrite
     * @param sessionDir directory used to store the session config files
     */
    KateApp(const QCommandLineParser &arg, const ApplicationMode mode, const QString &sessionsDir);

    /**
     * return current application mode
     */
    static ApplicationMode mode()
    {
        return KateApp::self()->m_mode;
    }

    /**
     * return current application mode
     */
    static bool isKate()
    {
        return mode() == KateApp::ApplicationKate;
    }

    /**
     * return current application mode
     */
    static bool isKWrite()
    {
        return mode() == KateApp::ApplicationKWrite;
    }

    /**
     * Fill in the authors and credits for the application.
     * Use by Kate & KWrite
     */
    static void fillAuthorsAndCredits(KAboutData &aboutData);

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
    KTextEditor::Application *wrapper()
    {
        return &m_wrapper;
    }

#ifdef WITH_KUSERFEEDBACK
    /**
     * Get our global user feedback provider
     * @return user feedback provider
     */
    KUserFeedback::Provider &userFeedbackProvider()
    {
        return m_userFeedbackProvider;
    }
#endif

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
     *
     */
    KateStashManager *stashManager();

    /**
     * window management
     */
public:
    /**
     * create a new main window, use given config if any for restore
     * @param sconfig session config object
     * @param sgroup session group for this window
     * @param userTriggered did the user trigger creation of this new window
     * @return new constructed main window
     */
    static KateMainWindow *newMainWindow(KConfig *sconfig = nullptr, const QString &sgroup = QString(), bool userTriggered = false);

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
     * Has the given document tabs/views inside main windows
     * other then the given one?
     *
     * @param doc document to check
     * @param window window to check
     * @return is this document visible as tab/view/... in any other window?
     */
    bool documentVisibleInOtherWindows(KTextEditor::Document *doc, KateMainWindow *window) const;

    /**
     * some stuff for the DBus API
     */
public:
    /**
     * open url with given encoding
     * used by kate if --use given
     * @param url filename
     * @param encoding encoding name
     * @param isTempFile whether the file is temporary
     * @param activateView activate the view of the opened document?
     * @param c cursor position to set if not invalid
     * @return opened document or nullptr
     */
    KTextEditor::Document *
    openDocUrl(const QUrl &url, const QString &encoding, bool isTempFile, bool activateView = true, KTextEditor::Cursor c = KTextEditor::Cursor::invalid());

    /**
     * position cursor in current active view
     * will clear selection
     * @param line line to set
     * @param column column to set
     * @return success
     */
    bool setCursor(int line, int column);

    /**
     * Checks if --line and/or --column args were provided and
     * returns the cursor for it
     */
    KTextEditor::Cursor cursorFromArgs();

    /**
     * Checks if line or column were provided in query string
     * (e.g. file:///file1?line=3&column=4) and returns the cursor
     * If cursor was not set it will return invalid cursor
     *
     * @param url the url to check
     */
    KTextEditor::Cursor cursorFromQueryString(const QUrl &url);

    /**
     * @return true if --line or --column command line args were provided
     */
    bool hasCursorInArgs();

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
    QList<KTextEditor::MainWindow *> mainWindows();

    /**
     * Accessor to the active main window.
     * \return a pointer to the active mainwindow
     */
    KTextEditor::MainWindow *activeMainWindow();

    /**
     * Get a list of all documents that are managed by the application.
     * This might contain less documents than the editor has in his documents () list.
     * @return all documents the application manages
     */
    QList<KTextEditor::Document *> documents()
    {
        return m_docManager.documentList();
    }

    /**
     * Get the document with the URL \p url.
     * if multiple documents match the searched url, return the first found one...
     * \param url the document's URL
     * \return the document with the given \p url or NULL, if none found
     */
    KTextEditor::Document *findUrl(const QUrl &url)
    {
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
    KTextEditor::Document *openUrl(const QUrl &url, const QString &encoding = QString())
    {
        return m_docManager.openUrl(url, encoding);
    }

    /**
     * Close the given \p document. If the document is modified, user will be asked if he wants that.
     * \param document the document to be closed
     * \return \e true on success, otherwise \e false
     */
    bool closeDocument(KTextEditor::Document *document)
    {
        return closeDocuments({document});
    }

    /**
     * Close a list of documents. If any of them are modified, user will be asked if he wants that.
     * Use this, if you want to close multiple documents at once, as the application might
     * be able to group the "do you really want that" dialogs into one.
     * \param documents list of documents to be closed
     * \return \e true on success, otherwise \e false
     */
    bool closeDocuments(const QList<KTextEditor::Document *> &documents);

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
     * last time some QEvent::ActivationChange occured, negated if no windows on the current active desktop
     * used to determine which instance to reuse, if we have multiple
     */
    qint64 lastActivationChange() const;

    /**
     * A message is received from an external instance, if we use QtSingleApplication
     *
     * \p instanceId the instance id we got the message from
     * \p message is a serialized message (at the moment just the file list separated by ';')
     */
    void remoteMessageReceived(quint32 instanceId, QByteArray message);

    /**
     * activate this kate instance
     */
    void activate(const QString &token = QString());

Q_SIGNALS:
    /**
     * Emitted when the configuration got changed via the global config dialog.
     * Allows widgets to listen on this and adapt, the KSharedConfig::openConfig()
     * is already updated.
     */
    void configurationChanged();

protected:
    /**
     * Event filter for QApplication to handle mac os like file open
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    /**
     * Handle destroy of documents created via openDocUrl.
     * Needed to e.g. emit token signal and cleanup temporary files.
     */
    void openDocUrlDocumentDestroyed(QObject *document);

private:
    /**
     * kate's command line args
     */
    const QCommandLineParser &m_args;

    /**
     * application mode, kate or kwrite
     */
    const ApplicationMode m_mode;

    /**
     * files opened via --tempfile, must be deleted on program exit
     * need to be constructed before document manager as the destructor of that might
     * access it via the destroyed signal of the document
     */
    QHash<QObject *, QStringList> m_tempFilesToDelete;

    /**
     * known main windows
     */
    QList<KateMainWindow *> m_mainWindows;

    /**
     * Wrapper of application for KTextEditor
     */
    KTextEditor::Application m_wrapper;

#ifdef WITH_DBUS
    /**
     * dbus interface, must survive longer than m_docManager
     * e.g. the destroyed signal of the document might access this
     */
    KateAppAdaptor m_adaptor;
#endif

    /**
     * document manager
     */
    KateDocManager m_docManager;

    /**
     * plugin manager
     */
    KatePluginManager m_pluginManager;

    /**
     * session manager
     */
    KateSessionManager m_sessionManager;

    KateStashManager m_stashManager;

#ifdef WITH_KUSERFEEDBACK
    /**
     * user feedback provider
     */
    KUserFeedback::Provider m_userFeedbackProvider;
#endif

    /**
     * last time some QEvent::ActivationChange occured
     * used to determine which instance to reuse, if we have multiple
     */
    qint64 m_lastActivationChange;
};
