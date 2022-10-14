/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "welcomeview.h"

#include "kateapp.h"
#include "kateviewmanager.h"
#include "ktexteditor_utils.h"
#include "recentitemsmodel.h"
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

    m_placeholderRecentFiles = new Placeholder;
    m_placeholderRecentFiles->setText(i18n("No recent files"));

    QVBoxLayout *layoutPlaceholderRecentFiles = new QVBoxLayout;
    layoutPlaceholderRecentFiles->addWidget(m_placeholderRecentFiles);
    listViewRecentFiles->setLayout(layoutPlaceholderRecentFiles);

    m_recentItemsModel = new RecentItemsModel(this);
    connect(m_recentItemsModel, &RecentItemsModel::modelReset, this, [this]() {
        const bool noRecentFiles = m_recentItemsModel->rowCount() == 0;
        buttonClearRecentFiles->setDisabled(noRecentFiles);
        m_placeholderRecentFiles->setVisible(noRecentFiles);
    });

    KRecentFilesAction *recentFilesAction = m_viewManager->mainWindow()->recentFilesAction();
    m_recentItemsModel->refresh(recentFilesAction->urls());
    connect(recentFilesAction, &KRecentFilesAction::recentListCleared, this, [this, recentFilesAction]() {
        m_recentItemsModel->refresh(recentFilesAction->urls());
    });

    listViewRecentFiles->setModel(m_recentItemsModel);
    connect(listViewRecentFiles, &QListView::customContextMenuRequested,
            this, &WelcomeView::onRecentFilesContextMenuRequested);
    connect(listViewRecentFiles, &QListView::activated, this, [this](const QModelIndex &index) {
        if (index.isValid()) {
            const QUrl url = m_recentItemsModel->url(index);
            Q_ASSERT(url.isValid());
            Utils::openUrlOrProject(m_viewManager, url);
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
        m_placeholderSavedSessions = new Placeholder;
        m_placeholderSavedSessions->setText(i18n("No saved sessions"));

        QVBoxLayout *layoutPlaceholderSavedSessions = new QVBoxLayout;
        layoutPlaceholderSavedSessions->addWidget(m_placeholderSavedSessions);
        listViewSavedSessions->setLayout(layoutPlaceholderSavedSessions);

        m_savedSessionsModel = new SavedSessionsModel(this);
        connect(m_savedSessionsModel, &SavedSessionsModel::modelReset, this, [this]() {
            const bool noSavedSession = m_savedSessionsModel->rowCount() == 0;
            m_placeholderSavedSessions->setVisible(noSavedSession);
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

    static const char showForNewWindowKey[] = "Open untitled document for new window";
    KConfigGroup configGroup = KSharedConfig::openConfig()->group("General");
    checkBoxShowForNewWindow->setChecked(!configGroup.readEntry(showForNewWindowKey, false));
    connect(checkBoxShowForNewWindow, &QCheckBox::toggled, this, [configGroup](bool checked) mutable {
        configGroup.writeEntry(showForNewWindowKey, !checked);
    });

    connect(KateApp::self(), &KateApp::configurationChanged, this, [this, configGroup]() {
        checkBoxShowForNewWindow->setChecked(!configGroup.readEntry(showForNewWindowKey, false));
    });

    updateFonts();
    updateButtons();
}

bool WelcomeView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FontChange:
        updateFonts();
        updateButtons();
        break;
    case QEvent::Resize:
        if (updateLayout()) {
            return true;
        }
        break;
    default:
        break;
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
            labelRecentItems->setText(i18n("Recent Documents and Projects"));
        } else {
            buttonOpenFolder->hide();
            labelRecentItems->setText(i18n("Recent Documents"));
        }
    }
}

void WelcomeView::onRecentFilesContextMenuRequested(const QPoint &pos)
{
    const QModelIndex index = listViewRecentFiles->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const QUrl url = m_recentItemsModel->url(index);
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
        m_recentItemsModel->refresh(recentFilesAction->urls());
    });
    contextMenu.addAction(action);

    contextMenu.exec(listViewRecentFiles->mapToGlobal(pos));
}

void WelcomeView::updateButtons()
{
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
}

void WelcomeView::updateFonts()
{
    QFont titleFont = font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setWeight(QFont::Bold);
    labelTitle->setFont(titleFont);

    QFont panelTitleFont = font();
    panelTitleFont.setPointSize(panelTitleFont.pointSize() + 2);
    labelRecentItems->setFont(panelTitleFont);
    labelSavedSessions->setFont(panelTitleFont);

    QFont placeholderFont = font();
    placeholderFont.setPointSize(qRound(placeholderFont.pointSize() * 1.3));
    m_placeholderRecentFiles->setFont(placeholderFont);
    if (m_placeholderSavedSessions) {
        m_placeholderSavedSessions->setFont(placeholderFont);
    }
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
