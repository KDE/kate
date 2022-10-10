/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "welcomeview.h"

#include "kateapp.h"
#include "kateviewmanager.h"
#include "ktexteditor_utils.h"
#include "recentfilesmodel.h"
#include "savedsessionsmodel.h"

#include <KAboutData>
#include <KConfigGroup>
#include <KIconLoader>
#include <KRecentFilesAction>
#include <KSharedConfig>
#include <KIO/OpenFileManagerWindowJob>

#include <QClipboard>
#include <QDir>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMenu>

class Placeholder : public QLabel
{
public:
    explicit Placeholder(QWidget *parent = nullptr)
        : QLabel(parent)
    {
        setAlignment(Qt::AlignCenter);
        setMargin(20);
        setWordWrap(true);
        // To match the size of a level 2 Heading/KTitleWidget
        QFont placeholderFont = font();
        placeholderFont.setPointSize(qRound(placeholderFont.pointSize() * 1.3));
        setFont(placeholderFont);
        // Match opacity of QML placeholder label component
        QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect;
        opacityEffect->setOpacity(0.5);
        setGraphicsEffect(opacityEffect);
    }
};

WelcomeView::WelcomeView(KateViewManager *viewManager, QWidget *parent)
    : QScrollArea(parent)
    , m_viewManager(viewManager)
{
    setupUi(this);

    const KAboutData aboutData = KAboutData::applicationData();
    labelTitle->setText(i18n("Welcome to %1", aboutData.displayName()));
    labelDescription->setText(aboutData.shortDescription());
    labelIcon->setPixmap(aboutData.programLogo().value<QIcon>().pixmap(KIconLoader::SizeEnormous));

    Placeholder *placeholderRecentFiles = new Placeholder;
    placeholderRecentFiles->setText(i18n("No recent files"));

    QVBoxLayout *layoutPlaceholderRecentFiles = new QVBoxLayout;
    layoutPlaceholderRecentFiles->addWidget(placeholderRecentFiles);
    listViewRecentFiles->setLayout(layoutPlaceholderRecentFiles);

    m_recentFilesModel = new RecentFilesModel(this);
    connect(m_recentFilesModel, &RecentFilesModel::modelReset, this, [this, placeholderRecentFiles]() {
        const bool noRecentFiles = m_recentFilesModel->rowCount() == 0;
        buttonClearRecentFiles->setDisabled(noRecentFiles);
        placeholderRecentFiles->setVisible(noRecentFiles);
    });

    KRecentFilesAction *recentFilesAction = m_viewManager->mainWindow()->recentFilesAction();
    m_recentFilesModel->refresh(recentFilesAction->urls());
    connect(recentFilesAction, &KRecentFilesAction::recentListCleared, this, [this, recentFilesAction]() {
        m_recentFilesModel->refresh(recentFilesAction->urls());
    });

    listViewRecentFiles->setModel(m_recentFilesModel);
    connect(listViewRecentFiles, &QListView::customContextMenuRequested,
            this, &WelcomeView::onRecentFilesContextMenuRequested);
    connect(listViewRecentFiles, &QListView::activated, this, [this](const QModelIndex &index) {
        if (index.isValid()) {
            const QUrl url = m_recentFilesModel->url(index);
            Q_ASSERT(url.isValid());
            if (url.isLocalFile()) {
                QDir dir = {url.path()};
                if (dir.exists()) {
                    Utils::openDirectoryOrProject(m_viewManager->mainWindow(), dir);
                    return;
                }
            }

            m_viewManager->openUrl(url);
        }
    });

    connect(buttonNewFile, &QPushButton::clicked, m_viewManager, &KateViewManager::slotDocumentNew);
    connect(buttonOpenFile, &QPushButton::clicked, m_viewManager, &KateViewManager::slotDocumentOpen);
    connect(buttonClearRecentFiles, &QPushButton::clicked, this, [recentFilesAction]() {
        recentFilesAction->clear();
    });

    onPluginViewChanged();

    const KTextEditor::MainWindow *mainWindow = m_viewManager->mainWindow()->wrapper();
    connect(mainWindow, &KTextEditor::MainWindow::pluginViewCreated,
            this, &WelcomeView::onPluginViewChanged);
    connect(mainWindow, &KTextEditor::MainWindow::pluginViewDeleted,
            this, &WelcomeView::onPluginViewChanged);

    if (KateApp::isKWrite()) {
        widgetSavedSessions->hide();
    } else {
        Placeholder *placeholderSavedSessions = new Placeholder;
        placeholderSavedSessions->setText(i18n("No saved sessions"));

        QVBoxLayout *layoutPlaceholderSavedSessions = new QVBoxLayout;
        layoutPlaceholderSavedSessions->addWidget(placeholderSavedSessions);
        listViewSavedSessions->setLayout(layoutPlaceholderSavedSessions);

        m_savedSessionsModel = new SavedSessionsModel(this);
        connect(m_savedSessionsModel, &SavedSessionsModel::modelReset, this, [this, placeholderSavedSessions]() {
            const bool noSavedSession = m_savedSessionsModel->rowCount() == 0;
            placeholderSavedSessions->setVisible(noSavedSession);
        });

        KateSessionManager *sessionManager = KateApp::self()->sessionManager();
        m_savedSessionsModel->refresh(sessionManager->sessionList());
        connect(sessionManager, &KateSessionManager::sessionListChanged, this, [this, sessionManager]() {
            m_savedSessionsModel->refresh(sessionManager->sessionList());
        });

        listViewSavedSessions->setModel(m_savedSessionsModel);
        connect(listViewSavedSessions, &QListView::activated, this, [sessionManager](const QModelIndex &index) {
            if (index.isValid()) {
                const QString sessionName = index.data().toString();
                Q_ASSERT(!sessionName.isEmpty());
                sessionManager->activateSession(sessionName);
            }
        });

        connect(buttonNewSession, &QPushButton::clicked, sessionManager, &KateSessionManager::sessionNew);
        connect(buttonManageSessions, &QPushButton::clicked, this, []() {
            KateSessionManager::sessionManage();
        });
    }

    // properly layout the buttons
    QVector<QPushButton*> buttons {
        buttonNewFile,
        buttonOpenFile,
        buttonOpenFolder
    };
    if (KateApp::isKate()) {
        buttons << buttonNewSession << buttonManageSessions;
    }
    const int maxWidth = std::accumulate(buttons.cbegin(), buttons.cend(), 0, [](int maxWidth, const QPushButton *button) {
        return std::max(maxWidth, button->sizeHint().width());
    });
    for (QPushButton *button : std::as_const(buttons)) {
        button->setFixedWidth(maxWidth);
    }

    static const char showForNewWindowKey[] = "Open untitled document for new window";
    KConfigGroup configGroup = KSharedConfig::openConfig()->group("General");
    checkBoxShowForNewWindow->setChecked(!configGroup.readEntry(showForNewWindowKey, false));
    connect(checkBoxShowForNewWindow, &QCheckBox::toggled, this, [configGroup](bool checked) mutable {
        configGroup.writeEntry(showForNewWindowKey, !checked);
    });

    connect(KateApp::self(), &KateApp::configurationChanged, this, [this, configGroup]() {
        checkBoxShowForNewWindow->setChecked(!configGroup.readEntry(showForNewWindowKey, false));
    });
}

bool WelcomeView::event(QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        if (updateLayout()) {
            return true;
        }
    }

    return QScrollArea::event(event);
}

void WelcomeView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);

    updateLayout();
}

void WelcomeView::onPluginViewChanged(const QString &pluginName)
{
    static const QString projectPluginName = QStringLiteral("kateprojectplugin");
    if (pluginName.isEmpty() || pluginName == projectPluginName) {
        QObject *projectPluginView = m_viewManager->mainWindow()->pluginView(projectPluginName);
        if (projectPluginView) {
            connect(buttonOpenFolder, SIGNAL(clicked()), projectPluginView, SLOT(openDirectoryOrProject()));
            buttonOpenFolder->show();
        } else {
            buttonOpenFolder->hide();
        }
    }
}

void WelcomeView::onRecentFilesContextMenuRequested(const QPoint &pos)
{
    const QModelIndex index = listViewRecentFiles->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const QUrl url = m_recentFilesModel->url(index);
    Q_ASSERT(url.isValid());

    QMenu contextMenu;

    QAction *action = new QAction(i18n("Copy &Location"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy-path")));
    connect(action, &QAction::triggered, this, [url]() {
        qApp->clipboard()->setText(url.toString(QUrl::PreferLocalFile));
    });
    contextMenu.addAction(action);

    action = new QAction(i18n("&Open Containing Folder"));
    action->setEnabled(url.isLocalFile());
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    connect(action, &QAction::triggered, this, [url]() {
        KIO::highlightInFileManager({ url });
    });
    contextMenu.addAction(action);

    action = new QAction(i18n("&Remove"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    connect(action, &QAction::triggered, this, [this, url]() {
        KRecentFilesAction *recentFilesAction = m_viewManager->mainWindow()->recentFilesAction();
        recentFilesAction->removeUrl(url);
        m_recentFilesModel->refresh(recentFilesAction->urls());
    });
    contextMenu.addAction(action);

    contextMenu.exec(listViewRecentFiles->mapToGlobal(pos));
}

bool WelcomeView::updateLayout()
{
    // show/hide widgetHeader depending on the view height
    if (widgetHeader->isVisible()) {
        if (height() <= frameContent->height()) {
            widgetHeader->hide();
            return true;
        }
    } else {
        if (height() > frameContent->height() + widgetHeader->height() + layoutContent->spacing()) {
            widgetHeader->show();
            return true;
        }
    }

    return false;
}
