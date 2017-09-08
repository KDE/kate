/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#include "kwrite.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/editor.h>

#include <KAboutApplicationDialog>
#include <KActionCollection>
#include <KToggleAction>
#include <KEditToolBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KSqueezedTextLabel>
#include <KXMLGUIFactory>
#include <KConfig>
#include <KConfigGui>

#include <config.h>

#ifdef KActivities_FOUND
#include <KActivities/ResourceInstance>
#endif

#include <QTimer>
#include <QTextCodec>
#include <QMimeData>
#include <QApplication>
#include <QLabel>
#include <QDragEnterEvent>
#include <QMenuBar>
#include <QDir>
#include <QFileDialog>
#include <QFileOpenEvent>

QList<KTextEditor::Document *> KWrite::docList;
QList<KWrite *> KWrite::winList;

KWrite::KWrite(KTextEditor::Document *doc)
    : m_view(nullptr)
    , m_recentFiles(nullptr)
    , m_paShowPath(nullptr)
    , m_paShowMenuBar(nullptr)
    , m_paShowStatusBar(nullptr)
    , m_activityResource(nullptr)
{
    if (!doc) {
        doc = KTextEditor::Editor::instance()->createDocument(nullptr);

        // enable the modified on disk warning dialogs if any
        if (qobject_cast<KTextEditor::ModificationInterface *>(doc)) {
            qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning(true);
        }

        docList.append(doc);
    }

    m_view = doc->createView(this);

    setCentralWidget(m_view);

    setupActions();

    // signals for the statusbar
    connect(m_view->document(), &KTextEditor::Document::modifiedChanged, this, &KWrite::modifiedChanged);
    connect(m_view->document(), SIGNAL(documentNameChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged()));
    connect(m_view->document(), SIGNAL(readWriteChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged()));
    connect(m_view->document(), SIGNAL(documentUrlChanged(KTextEditor::Document*)), this, SLOT(urlChanged()));

    setAcceptDrops(true);
    connect(m_view, SIGNAL(dropEventPass(QDropEvent*)), this, SLOT(slotDropEvent(QDropEvent*)));

    setXMLFile(QStringLiteral("kwriteui.rc"));
    createShellGUI(true);
    guiFactory()->addClient(m_view);

    // FIXME: make sure the config dir exists, any idea how to do it more cleanly?
    QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).mkpath(QStringLiteral("."));

    // call it as last thing, must be sure everything is already set up ;)
    setAutoSaveSettings();

    readConfig();

    winList.append(this);

    documentNameChanged();
    show();

    // give view focus
    m_view->setFocus(Qt::OtherFocusReason);

    /**
     * handle mac os x like file open request via event filter
     */
    qApp->installEventFilter(this);
}

KWrite::~KWrite()
{
    guiFactory()->removeClient(m_view);

    winList.removeAll(this);

    KTextEditor::Document *doc = m_view->document();
    delete m_view;

    // kill document, if last view is closed
    if (doc->views().isEmpty()) {
        docList.removeAll(doc);
        delete doc;
    }

    KSharedConfig::openConfig()->sync();
}

QSize KWrite::sizeHint () const
{
    /**
     * have some useful size hint, else we have mini windows per default
     */
    return (QSize(640, 480).expandedTo(minimumSizeHint()));
}

void KWrite::setupActions()
{
    m_closeAction = actionCollection()->addAction(KStandardAction::Close, QStringLiteral("file_close"), this, SLOT(slotFlush()));
    m_closeAction->setIcon(QIcon::fromTheme(QStringLiteral("document-close")));
    m_closeAction->setWhatsThis(i18n("Use this command to close the current document"));
    m_closeAction->setDisabled(true);

    // setup File menu
    actionCollection()->addAction(KStandardAction::New, QStringLiteral("file_new"), this, SLOT(slotNew()))
    ->setWhatsThis(i18n("Use this command to create a new document"));
    actionCollection()->addAction(KStandardAction::Open, QStringLiteral("file_open"), this, SLOT(slotOpen()))
    ->setWhatsThis(i18n("Use this command to open an existing document for editing"));

    m_recentFiles = KStandardAction::openRecent(this, SLOT(slotOpen(QUrl)), this);
    actionCollection()->addAction(m_recentFiles->objectName(), m_recentFiles);
    m_recentFiles->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

    QAction *a = actionCollection()->addAction(QStringLiteral("view_new_view"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    a->setText(i18n("&New Window"));
    connect(a, SIGNAL(triggered()), this, SLOT(newView()));
    a->setWhatsThis(i18n("Create another view containing the current document"));

    actionCollection()->addAction(KStandardAction::Quit, this, SLOT(close()))
    ->setWhatsThis(i18n("Close the current document view"));

    // setup Settings menu
    setStandardToolBarMenuEnabled(true);

    m_paShowMenuBar = KStandardAction::showMenubar(this, SLOT(toggleMenuBar()), actionCollection());

    m_paShowStatusBar = KStandardAction::showStatusbar(this, SLOT(toggleStatusBar()), this);
    actionCollection()->addAction(m_paShowStatusBar->objectName(), m_paShowStatusBar);
    m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));

    m_paShowPath = new KToggleAction(i18n("Sho&w Path in Titlebar"), this);
    actionCollection()->addAction(QStringLiteral("set_showPath"), m_paShowPath);
    connect(m_paShowPath, SIGNAL(triggered()), this, SLOT(documentNameChanged()));
    m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));

    a = actionCollection()->addAction(KStandardAction::KeyBindings, this, SLOT(editKeys()));
    a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

    a = actionCollection()->addAction(KStandardAction::ConfigureToolbars, QStringLiteral("options_configure_toolbars"),
                                      this, SLOT(editToolbars()));
    a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

    a = actionCollection()->addAction(QStringLiteral("help_about_editor"));
    a->setText(i18n("&About Editor Component"));
    connect(a, SIGNAL(triggered()), this, SLOT(aboutEditor()));
}

// load on url
void KWrite::loadURL(const QUrl &url)
{
    m_view->document()->openUrl(url);

#ifdef KActivities_FOUND
    if (!m_activityResource) {
        m_activityResource = new KActivities::ResourceInstance(winId(), this);
    }
    m_activityResource->setUri(m_view->document()->url());
#endif

    m_closeAction->setEnabled(true);
}

// is closing the window wanted by user ?
bool KWrite::queryClose()
{
    if (m_view->document()->views().count() > 1) {
        return true;
    }

    if (m_view->document()->queryClose()) {
        writeConfig();

        return true;
    }

    return false;
}

void KWrite::slotFlush()
{
    if (m_view->document()->closeUrl()) {
        m_closeAction->setDisabled(true);
    }
}

void KWrite::modifiedChanged()
{
    documentNameChanged();
    m_closeAction->setEnabled(true);
}

void KWrite::slotNew()
{
    new KWrite();
}

void KWrite::slotOpen()
{
    const QList<QUrl> urls = QFileDialog::getOpenFileUrls(this, i18n("Open File"), m_view->document()->url());
    Q_FOREACH(QUrl url, urls) {
        slotOpen(url);
    }
}

void KWrite::slotOpen(const QUrl &url)
{
    if (url.isEmpty()) {
        return;
    }

    if (m_view->document()->isModified() || !m_view->document()->url().isEmpty()) {
        KWrite *t = new KWrite();
        t->loadURL(url);
    } else {
        loadURL(url);
    }
}

void KWrite::urlChanged()
{
    if (! m_view->document()->url().isEmpty()) {
        m_recentFiles->addUrl(m_view->document()->url());
    }

    // update caption
    documentNameChanged();
}

void KWrite::newView()
{
    new KWrite(m_view->document());
}

void KWrite::toggleMenuBar(bool showMessage)
{
    if (m_paShowMenuBar->isChecked()) {
        menuBar()->show();
        removeMenuBarActionFromContextMenu();
    } else {
        if (showMessage) {
            const QString accel = m_paShowMenuBar->shortcut().toString();
            KMessageBox::information(this, i18n("This will hide the menu bar completely."
                                                " You can show it again by typing %1.", accel),
                                     i18n("Hide menu bar"),
                                     QLatin1String("HideMenuBarWarning"));
        }
        menuBar()->hide();
        addMenuBarActionToContextMenu();
    }
}

void KWrite::addMenuBarActionToContextMenu()
{
    m_view->contextMenu()->addAction(m_paShowMenuBar);
}

void KWrite::removeMenuBarActionFromContextMenu()
{
    m_view->contextMenu()->removeAction(m_paShowMenuBar);
}

void KWrite::toggleStatusBar()
{
    m_view->setStatusBarEnabled(m_paShowStatusBar->isChecked());
}

void KWrite::editKeys()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    dlg.addCollection(actionCollection());
    if (m_view) {
        dlg.addCollection(m_view->actionCollection());
    }
    dlg.configure();
}

void KWrite::editToolbars()
{
    KConfigGroup cfg = KSharedConfig::openConfig()->group("MainWindow");
    saveMainWindowSettings(cfg);
    KEditToolBar dlg(guiFactory(), this);

    connect(&dlg, SIGNAL(newToolBarConfig()), this, SLOT(slotNewToolbarConfig()));
    dlg.exec();
}

void KWrite::slotNewToolbarConfig()
{
    applyMainWindowSettings(KSharedConfig::openConfig()->group("MainWindow"));
}

void KWrite::dragEnterEvent(QDragEnterEvent *event)
{
    const QList<QUrl> uriList = event->mimeData()->urls();
    event->setAccepted(! uriList.isEmpty());
}

void KWrite::dropEvent(QDropEvent *event)
{
    slotDropEvent(event);
}

void KWrite::slotDropEvent(QDropEvent *event)
{
    const QList<QUrl> textlist = event->mimeData()->urls();

    foreach(const QUrl & url, textlist)
    slotOpen(url);
}

void KWrite::slotEnableActions(bool enable)
{
    QList<QAction *> actions = actionCollection()->actions();
    QList<QAction *>::ConstIterator it = actions.constBegin();
    QList<QAction *>::ConstIterator end = actions.constEnd();

    for (; it != end; ++it) {
        (*it)->setEnabled(enable);
    }

    actions = m_view->actionCollection()->actions();
    it = actions.constBegin();
    end = actions.constEnd();

    for (; it != end; ++it) {
        (*it)->setEnabled(enable);
    }
}

//common config
void KWrite::readConfig(KSharedConfigPtr config)
{
    KConfigGroup cfg(config, "General Options");

    m_paShowMenuBar->setChecked(cfg.readEntry("ShowMenuBar", true));
    m_paShowStatusBar->setChecked(cfg.readEntry("ShowStatusBar", true));
    m_paShowPath->setChecked(cfg.readEntry("ShowPath", false));

    m_recentFiles->loadEntries(config->group("Recent Files"));

    // update visibility of menu bar and status bar
    toggleMenuBar(false);
    m_view->setStatusBarEnabled(m_paShowStatusBar->isChecked());
}

void KWrite::writeConfig(KSharedConfigPtr config)
{
    KConfigGroup generalOptions(config, "General Options");

    generalOptions.writeEntry("ShowMenuBar", m_paShowMenuBar->isChecked());
    generalOptions.writeEntry("ShowStatusBar", m_paShowStatusBar->isChecked());
    generalOptions.writeEntry("ShowPath", m_paShowPath->isChecked());

    m_recentFiles->saveEntries(KConfigGroup(config, "Recent Files"));

    config->sync();
}

//config file
void KWrite::readConfig()
{
    readConfig(KSharedConfig::openConfig());
}

void KWrite::writeConfig()
{
    writeConfig(KSharedConfig::openConfig());
}

// session management
void KWrite::restore(KConfig *config, int n)
{
    readPropertiesInternal(config, n);
}

void KWrite::readProperties(const KConfigGroup &config)
{
    readConfig();

    m_view->readSessionConfig(KConfigGroup(&config, QStringLiteral("General Options")));
}

void KWrite::saveProperties(KConfigGroup &config)
{
    writeConfig();

    config.writeEntry("DocumentNumber", docList.indexOf(m_view->document()) + 1);

    KConfigGroup cg(&config, QStringLiteral("General Options"));
    m_view->writeSessionConfig(cg);
}

void KWrite::saveGlobalProperties(KConfig *config) //save documents
{
    config->group("Number").writeEntry("NumberOfDocuments", docList.count());

    for (int z = 1; z <= docList.count(); z++) {
        QString buf = QString::fromLatin1("Document %1").arg(z);
        KConfigGroup cg(config, buf);
        KTextEditor::Document *doc = docList.at(z - 1);
        doc->writeSessionConfig(cg);
    }

    for (int z = 1; z <= winList.count(); z++) {
        QString buf = QString::fromLatin1("Window %1").arg(z);
        KConfigGroup cg(config, buf);
        cg.writeEntry("DocumentNumber", docList.indexOf(winList.at(z - 1)->view()->document()) + 1);
    }
}

//restore session
void KWrite::restore()
{
    KConfig *config = KConfigGui::sessionConfig();

    if (!config) {
        return;
    }

    int docs, windows;
    QString buf;
    KTextEditor::Document *doc;
    KWrite *t;

    KConfigGroup numberConfig(config, "Number");
    docs = numberConfig.readEntry("NumberOfDocuments", 0);
    windows = numberConfig.readEntry("NumberOfWindows", 0);

    for (int z = 1; z <= docs; z++) {
        buf = QString::fromLatin1("Document %1").arg(z);
        KConfigGroup cg(config, buf);
        doc = KTextEditor::Editor::instance()->createDocument(nullptr);
        doc->readSessionConfig(cg);
        docList.append(doc);
    }

    for (int z = 1; z <= windows; z++) {
        buf = QString::fromLatin1("Window %1").arg(z);
        KConfigGroup cg(config, buf);
        t = new KWrite(docList.at(cg.readEntry("DocumentNumber", 0) - 1));
        t->restore(config, z);
    }
}

void KWrite::aboutEditor()
{
    KAboutApplicationDialog dlg(KTextEditor::Editor::instance()->aboutData(), this);
    dlg.exec();
}

void KWrite::documentNameChanged()
{
    QString readOnlyCaption;
    if (!m_view->document()->isReadWrite()) {
        readOnlyCaption = i18n(" [read only]");
    }

    if (m_view->document()->url().isEmpty()) {
        setCaption(i18n("Untitled") + readOnlyCaption + QStringLiteral(" [*]"), m_view->document()->isModified());
        return;
    }

    QString c;

    if (m_paShowPath->isChecked()) {
        c = m_view->document()->url().toString(QUrl::PreferLocalFile);

        const QString homePath = QDir::homePath();
        if (c.startsWith(homePath)) {
            c = QStringLiteral("~") + c.right(c.length() - homePath.length());
        }

        //File name shouldn't be too long - Maciek
        if (c.length() > 64) {
            c = QStringLiteral("...") + c.right(64);
        }
    } else {
        c = m_view->document()->url().fileName();

        //File name shouldn't be too long - Maciek
        if (c.length() > 64) {
            c = c.left(64) + QStringLiteral("...");
        }
    }

    setCaption(c + readOnlyCaption + QStringLiteral(" [*]"), m_view->document()->isModified());
}

bool KWrite::eventFilter(QObject *obj, QEvent *event)
{
    /**
     * handle mac os like file open
     */
    if (event->type() == QEvent::FileOpen) {
        /**
         * try to open and activate the new document, like we would do for stuff
         * opened via file dialog
         */
        QFileOpenEvent *foe = static_cast<QFileOpenEvent*>(event);
        slotOpen(foe->url());
        return true;
    }
    
    /**
     * else: pass over to default implementation
     */
    return KParts::MainWindow::eventFilter(obj, event);
}
