/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "welcomeview.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "recentitemsmodel.h"
#include "savedsessionsmodel.h"

#include <KAboutData>
#include <KConfigGroup>
#include <KIO/OpenFileManagerWindowJob>
#include <KIconLoader>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KSharedConfig>
#include <QTimer>

#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
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
        auto *opacityEffect = new QGraphicsOpacityEffect;
        opacityEffect->setOpacity(0.5);
        setGraphicsEffect(opacityEffect);
    }
};

WelcomeView::WelcomeView(KateViewManager *viewManager, QWidget *parent)
    : QScrollArea(parent)
    , m_viewManager(viewManager)
{
    setupUi(this);

    listViewRecentItems->setSelectionMode(QAbstractItemView::MultiSelection);

    const KAboutData aboutData = KAboutData::applicationData();
    labelTitle->setText(i18n("Welcome to %1", aboutData.displayName()));
    labelDescription->setText(aboutData.shortDescription());
    labelIcon->setPixmap(aboutData.programLogo().value<QIcon>().pixmap(KIconLoader::SizeEnormous));

    labelRecentItems->setText(KateApp::isKate() ? i18n("Recent Documents and Projects") : i18n("Recent Documents"));

    m_placeholderRecentItems = new Placeholder;
    m_placeholderRecentItems->setText(KateApp::isKate() ? i18n("No recent documents or projects") : i18n("No recent documents"));

    auto *layoutPlaceholderRecentItems = new QVBoxLayout;
    layoutPlaceholderRecentItems->addWidget(m_placeholderRecentItems);
    listViewRecentItems->setLayout(layoutPlaceholderRecentItems);

    m_recentItemsModel = new RecentItemsModel(this);
    connect(m_recentItemsModel, &RecentItemsModel::modelReset, this, [this]() {
        const bool noRecentItems = m_recentItemsModel->rowCount() == 0;
        buttonClearRecentItems->setDisabled(noRecentItems);
        m_placeholderRecentItems->setVisible(noRecentItems);
    });

    KRecentFilesAction *recentFilesAction = m_viewManager->mainWindow()->recentFilesAction();
    m_recentItemsModel->refresh(recentFilesAction->urls());
    recentFilesAction->menu()->installEventFilter(this);

    listViewRecentItems->setModel(m_recentItemsModel);
    connect(listViewRecentItems, &QListView::customContextMenuRequested, this, &WelcomeView::onRecentItemsContextMenuRequested);
    connect(listViewRecentItems, &QListView::activated, this, [this](const QModelIndex &index) {
        if (index.isValid()) {
            const QUrl url = m_recentItemsModel->url(index);

            // Check with QFile if file exists, display error message if it doesn't
            m_viewManager->openRecent(url);
        }
    });

    connect(buttonNewFile, &QPushButton::clicked, m_viewManager, &KateViewManager::slotDocumentNew);
    connect(buttonOpenFile, &QPushButton::clicked, m_viewManager, &KateViewManager::slotDocumentOpen);
    connect(buttonClearRecentItems, &QPushButton::clicked, this, [recentFilesAction]() {
        recentFilesAction->clear();
    });

    connect(labelHomepage, qOverload<>(&KUrlLabel::leftClickedUrl), this, [aboutData]() {
        QDesktopServices::openUrl(QUrl(aboutData.homepage()));
    });
    connect(labelContribute, qOverload<>(&KUrlLabel::leftClickedUrl), this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://kate-editor.org/join-us")));
    });
    connect(labelHandbook, qOverload<>(&KUrlLabel::leftClickedUrl), this, [this]() {
        m_viewManager->mainWindow()->appHelpActivated();
    });

    onPluginViewChanged();

    const KTextEditor::MainWindow *mainWindow = m_viewManager->mainWindow()->wrapper();
    connect(mainWindow, &KTextEditor::MainWindow::pluginViewCreated, this, &WelcomeView::onPluginViewChanged);
    connect(mainWindow, &KTextEditor::MainWindow::pluginViewDeleted, this, &WelcomeView::onPluginViewChanged);

    if (KateApp::isKWrite()) {
        widgetSavedSessions->hide();
    } else {
        m_placeholderSavedSessions = new Placeholder;
        m_placeholderSavedSessions->setText(i18n("No saved sessions"));

        auto *layoutPlaceholderSavedSessions = new QVBoxLayout;
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

    static const char showForNewWindowKey[] = "Show welcome view for new window";
    KConfigGroup configGroup = KSharedConfig::openConfig()->group(QStringLiteral("General"));
    checkBoxShowForNewWindow->setChecked(configGroup.readEntry(showForNewWindowKey, true));
    connect(checkBoxShowForNewWindow, &QCheckBox::toggled, this, [configGroup](bool checked) mutable {
        configGroup.writeEntry(showForNewWindowKey, checked);
    });

    connect(KateApp::self(), &KateApp::configurationChanged, this, [this, configGroup]() {
        checkBoxShowForNewWindow->setChecked(configGroup.readEntry(showForNewWindowKey, true));
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

bool WelcomeView::eventFilter(QObject *watched, QEvent *event)
{
    KRecentFilesAction *recentFilesAction = m_viewManager->mainWindow()->recentFilesAction();
    if (watched == recentFilesAction->menu()) {
        switch (event->type()) {
        case QEvent::ActionAdded:
        case QEvent::ActionRemoved:
            // since the KRecentFilesAction doesn't notify about adding or
            // deleting items, we should use this dirty trick to find out
            // the KRecentFilesAction has changed
            QTimer::singleShot(0, this, [this, recentFilesAction]() {
                m_recentItemsModel->refresh(recentFilesAction->urls());
            });
            break;
        default:
            break;
        }
    }

    return QScrollArea::eventFilter(watched, event);
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

void WelcomeView::onRecentItemsContextMenuRequested(const QPoint &pos)
{
    const QModelIndex index = listViewRecentItems->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const QUrl url = m_recentItemsModel->url(index);
    Q_ASSERT(url.isValid());

    QMenu contextMenu(listViewRecentItems);

    const auto selectedIndexes = listViewRecentItems->selectionModel()->selectedIndexes();
    auto allSelectedAreFiles = [this, selectedIndexes] {
        return std::all_of(selectedIndexes.begin(), selectedIndexes.end(), [model = m_recentItemsModel](const QModelIndex &index) {
            const QUrl url = model->url(index);
            return !url.isLocalFile() || QFileInfo(url.toLocalFile()).isFile();
        });
    };
    if (selectedIndexes.size() > 1 && allSelectedAreFiles()) {
        auto *action = new QAction(i18n("Open Selected Files..."), this);
        action->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
        connect(action, &QAction::triggered, this, [this, selectedIndexes]() {
            for (const auto &index : selectedIndexes) {
                const auto url = m_recentItemsModel->url(index);
                if (url.isValid()) {
                    m_viewManager->openUrl(url);
                }
            }
        });
        contextMenu.addAction(action);
    }

    auto *action = new QAction(i18n("Copy &Location"), this);
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy-path")));
    connect(action, &QAction::triggered, this, [url]() {
        qApp->clipboard()->setText(url.toString(QUrl::PreferLocalFile));
    });
    contextMenu.addAction(action);

    action = new QAction(i18n("&Open Containing Folder"), this);
    action->setEnabled(url.isLocalFile());
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    connect(action, &QAction::triggered, this, [url]() {
        KIO::highlightInFileManager({url});
    });
    contextMenu.addAction(action);

    action = new QAction(i18n("&Remove"), this);
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    connect(action, &QAction::triggered, this, [this, url, selectedIndexes]() {
        KRecentFilesAction *recentFilesAction = m_viewManager->mainWindow()->recentFilesAction();
        if (selectedIndexes.empty()) {
            if (url.isValid()) {
                recentFilesAction->removeUrl(url);
            }
        } else {
            for (const auto &index : selectedIndexes) {
                const auto url = m_recentItemsModel->url(index);
                if (url.isValid()) {
                    recentFilesAction->removeUrl(url);
                }
            }
        }
    });
    contextMenu.addAction(action);

    contextMenu.exec(listViewRecentItems->mapToGlobal(pos));
}

void WelcomeView::updateButtons()
{
    std::vector<QPushButton *> buttons{buttonNewFile, buttonOpenFile, buttonOpenFolder};
    if (KateApp::isKate()) {
        buttons.push_back(buttonNewSession);
        buttons.push_back(buttonManageSessions);
    }

    int maxWidth = 0;
    for (const auto *button : buttons) {
        maxWidth = std::max(maxWidth, button->sizeHint().width());
    }

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
    labelHelp->setFont(panelTitleFont);

    QFont placeholderFont = font();
    placeholderFont.setPointSize(qRound(placeholderFont.pointSize() * 1.3));
    m_placeholderRecentItems->setFont(placeholderFont);
    if (m_placeholderSavedSessions) {
        m_placeholderSavedSessions->setFont(placeholderFont);
    }
}

bool WelcomeView::updateLayout()
{
    // Align labelHelp with labelRecentItems
    labelHelp->setMinimumHeight(labelRecentItems->height());

    bool result = false;

    // show/hide widgetHeader depending on the view height
    if (widgetHeader->isVisible()) {
        if (height() <= frameContent->height()) {
            widgetHeader->hide();
            result = true;
        }
    } else {
        const int implicitHeight = frameContent->height() + widgetHeader->height() + layoutContent->spacing();
        if (height() > implicitHeight) {
            widgetHeader->show();
            result = true;
        }
    }

    // show/hide widgetHelp depending on the view height
    if (widgetHelp->isVisible()) {
        if (width() <= frameContent->width()) {
            widgetHelp->hide();
            result = true;
        }
    } else {
        const int implicitWidth = frameContent->width() + widgetHelp->width() + layoutPanels->horizontalSpacing();
        if (width() > implicitWidth) {
            widgetHelp->show();
            return true;
        }
    }

    return result;
}

#include "moc_welcomeview.cpp"
