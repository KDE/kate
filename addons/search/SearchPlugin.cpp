/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SearchPlugin.h"
#include "KateSearchCommand.h"
#include "MatchExportDialog.h"
#include "MatchProxyModel.h"
#include "Results.h"

#include <KTextEditor/Document>
#include <ktexteditor/editor.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/view.h>

#include <KAcceleratorManager>
#include <KActionCollection>
#include <KColorScheme>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KUrlCompletion>

#include <KConfigGroup>
#include <KXMLGUIFactory>

#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>

#include <ktexteditor_utils.h>

static QUrl localFileDirUp(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return url;
    }

    // else go up
    return QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).dir().absolutePath());
}

static QAction *
menuEntry(QMenu *menu, const QString &before, const QString &after, const QString &desc, QString menuBefore = QString(), QString menuAfter = QString());

/**
 * When the action is triggered the cursor will be placed between @p before and @p after.
 */
static QAction *menuEntry(QMenu *menu, const QString &before, const QString &after, const QString &desc, QString menuBefore, QString menuAfter)
{
    if (menuBefore.isEmpty()) {
        menuBefore = before;
    }
    if (menuAfter.isEmpty()) {
        menuAfter = after;
    }

    QAction *const action = menu->addAction(menuBefore + menuAfter + QLatin1Char('\t') + desc);
    if (!action) {
        return nullptr;
    }

    action->setData(QString(before + QLatin1Char(' ') + after));
    return action;
}

/**
 * adds items and separators for special chars in "replace" field
 */
static void addSpecialCharsHelperActionsForReplace(QSet<QAction *> *actionList, QMenu *menu)
{
    QSet<QAction *> &actionPointers = *actionList;
    QString emptyQSTring;

    actionPointers << menuEntry(menu, QStringLiteral("\\n"), emptyQSTring, i18n("Line break"));
    actionPointers << menuEntry(menu, QStringLiteral("\\t"), emptyQSTring, i18n("Tab"));
}

/**
 * adds items and separators for regex in "search" field
 */
static void addRegexHelperActionsForSearch(QSet<QAction *> *actionList, QMenu *menu)
{
    QSet<QAction *> &actionPointers = *actionList;
    QString emptyQSTring;

    actionPointers << menuEntry(menu, QStringLiteral("^"), emptyQSTring, i18n("Beginning of line"));
    actionPointers << menuEntry(menu, QStringLiteral("$"), emptyQSTring, i18n("End of line"));
    menu->addSeparator();
    actionPointers << menuEntry(menu, QStringLiteral("."), emptyQSTring, i18n("Any single character (excluding line breaks)"));
    actionPointers << menuEntry(menu, QStringLiteral("[.]"), emptyQSTring, i18n("Literal dot"));
    menu->addSeparator();
    actionPointers << menuEntry(menu, QStringLiteral("+"), emptyQSTring, i18n("One or more occurrences"));
    actionPointers << menuEntry(menu, QStringLiteral("*"), emptyQSTring, i18n("Zero or more occurrences"));
    actionPointers << menuEntry(menu, QStringLiteral("?"), emptyQSTring, i18n("Zero or one occurrences"));
    actionPointers
        << menuEntry(menu, QStringLiteral("{"), QStringLiteral(",}"), i18n("<a> through <b> occurrences"), QStringLiteral("{a"), QStringLiteral(",b}"));
    menu->addSeparator();
    actionPointers << menuEntry(menu, QStringLiteral("("), QStringLiteral(")"), i18n("Group, capturing"));
    actionPointers << menuEntry(menu, QStringLiteral("|"), emptyQSTring, i18n("Or"));
    actionPointers << menuEntry(menu, QStringLiteral("["), QStringLiteral("]"), i18n("Set of characters"));
    actionPointers << menuEntry(menu, QStringLiteral("[^"), QStringLiteral("]"), i18n("Negative set of characters"));
    actionPointers << menuEntry(menu, QStringLiteral("(?:"), QStringLiteral(")"), i18n("Group, non-capturing"), QStringLiteral("(?:E"));
    actionPointers << menuEntry(menu, QStringLiteral("(?="), QStringLiteral(")"), i18n("Lookahead"), QStringLiteral("(?=E"));
    actionPointers << menuEntry(menu, QStringLiteral("(?!"), QStringLiteral(")"), i18n("Negative lookahead"), QStringLiteral("(?!E"));

    menu->addSeparator();
    actionPointers << menuEntry(menu, QStringLiteral("\\n"), emptyQSTring, i18n("Line break"));
    actionPointers << menuEntry(menu, QStringLiteral("\\t"), emptyQSTring, i18n("Tab"));
    actionPointers << menuEntry(menu, QStringLiteral("\\b"), emptyQSTring, i18n("Word boundary"));
    actionPointers << menuEntry(menu, QStringLiteral("\\B"), emptyQSTring, i18n("Not word boundary"));
    actionPointers << menuEntry(menu, QStringLiteral("\\d"), emptyQSTring, i18n("Digit"));
    actionPointers << menuEntry(menu, QStringLiteral("\\D"), emptyQSTring, i18n("Non-digit"));
    actionPointers << menuEntry(menu, QStringLiteral("\\s"), emptyQSTring, i18n("Whitespace (excluding line breaks)"));
    actionPointers << menuEntry(menu, QStringLiteral("\\S"), emptyQSTring, i18n("Non-whitespace (excluding line breaks)"));
    actionPointers << menuEntry(menu, QStringLiteral("\\w"), emptyQSTring, i18n("Word character (alphanumerics plus '_')"));
    actionPointers << menuEntry(menu, QStringLiteral("\\W"), emptyQSTring, i18n("Non-word character"));
}

/**
 * adds items and separators for regex in "replace" field
 */
void KatePluginSearchView::addRegexHelperActionsForReplace(QSet<QAction *> *actionList, QMenu *menu)
{
    QSet<QAction *> &actionPointers = *actionList;
    QString emptyQSTring;

    menu->addSeparator();
    actionPointers << menuEntry(menu, QStringLiteral("\\0"), emptyQSTring, i18n("Regular expression capture 0 (whole match)"));
    actionPointers << menuEntry(menu, QStringLiteral("\\"), emptyQSTring, i18n("Regular expression capture 1-9"), QStringLiteral("\\#"));
    actionPointers << menuEntry(menu, QStringLiteral("\\{"), QStringLiteral("}"), i18n("Regular expression capture 0-999"), QStringLiteral("\\{#"));
    menu->addSeparator();
    actionPointers << menuEntry(menu, QStringLiteral("\\U\\"), emptyQSTring, i18n("Upper-cased capture 0-9"), QStringLiteral("\\U\\#"));
    actionPointers << menuEntry(menu, QStringLiteral("\\U\\{"), QStringLiteral("}"), i18n("Upper-cased capture 0-999"), QStringLiteral("\\U\\{###"));
    actionPointers << menuEntry(menu, QStringLiteral("\\L\\"), emptyQSTring, i18n("Lower-cased capture 0-9"), QStringLiteral("\\L\\#"));
    actionPointers << menuEntry(menu, QStringLiteral("\\L\\{"), QStringLiteral("}"), i18n("Lower-cased capture 0-999"), QStringLiteral("\\L\\{###"));
}

/**
 * inserts text and sets cursor position
 */
void KatePluginSearchView::regexHelperActOnAction(QAction *resultAction, const QSet<QAction *> &actionList, QLineEdit *lineEdit)
{
    if (resultAction && actionList.contains(resultAction)) {
        const int cursorPos = lineEdit->cursorPosition();
        QStringList beforeAfter = resultAction->data().toString().split(QLatin1Char(' '));
        if (beforeAfter.size() != 2) {
            return;
        }
        lineEdit->insert(beforeAfter[0] + beforeAfter[1]);
        lineEdit->setCursorPosition(cursorPos + beforeAfter[0].size());
        lineEdit->setFocus();
    }
}

K_PLUGIN_FACTORY_WITH_JSON(KatePluginSearchFactory, "katesearch.json", registerPlugin<KatePluginSearch>();)

KatePluginSearch::KatePluginSearch(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
    // ensure we can send over vector of matches via queued connection
    qRegisterMetaType<QList<KateSearchMatch>>();

    m_searchCommand = new KateSearchCommand(this);
}

KatePluginSearch::~KatePluginSearch()
{
    delete m_searchCommand;
}

QObject *KatePluginSearch::createView(KTextEditor::MainWindow *mainWindow)
{
    KatePluginSearchView *view = new KatePluginSearchView(this, mainWindow, KTextEditor::Editor::instance()->application());
    connect(m_searchCommand, &KateSearchCommand::setSearchPlace, view, &KatePluginSearchView::setSearchPlace);
    connect(m_searchCommand, &KateSearchCommand::setCurrentFolder, view, &KatePluginSearchView::setCurrentFolder);
    connect(m_searchCommand, &KateSearchCommand::setSearchString, view, &KatePluginSearchView::setSearchString);
    connect(m_searchCommand, &KateSearchCommand::startSearch, view, &KatePluginSearchView::startSearch);
    connect(m_searchCommand, &KateSearchCommand::setRegexMode, view, &KatePluginSearchView::setRegexMode);
    connect(m_searchCommand, &KateSearchCommand::setCaseInsensitive, view, &KatePluginSearchView::setCaseInsensitive);
    connect(m_searchCommand, &KateSearchCommand::setExpandResults, view, &KatePluginSearchView::setExpandResults);
    connect(m_searchCommand, &KateSearchCommand::newTab, view, &KatePluginSearchView::addTab);

    connect(view, &KatePluginSearchView::searchBusy, m_searchCommand, &KateSearchCommand::setBusy);

    return view;
}

bool ContainerWidget::focusNextPrevChild(bool next)
{
    QWidget *fw = focusWidget();
    bool found = false;
    Q_EMIT nextFocus(fw, &found, next);

    if (found) {
        return true;
    }
    return QWidget::focusNextPrevChild(next);
}

void KatePluginSearchView::nextFocus(QWidget *currentWidget, bool *found, bool next)
{
    *found = false;

    if (!currentWidget) {
        return;
    }

    // we use the object names here because there can be multiple trees (on multiple result tabs)
    if (next) {
        if (currentWidget->objectName() == QLatin1String("treeView") || currentWidget == m_ui.sizeLimitSpinBox) {
            m_ui.searchCombo->setFocus();
            *found = true;
            return;
        }
        if (currentWidget == m_ui.displayOptions) {
            if (m_ui.displayOptions->isChecked()) {
                if (m_ui.searchPlaceCombo->currentIndex() < MatchModel::Folder) {
                    m_ui.searchCombo->setFocus();
                    *found = true;
                    return;
                } else if (m_ui.searchPlaceCombo->currentIndex() == MatchModel::Folder) {
                    m_ui.folderRequester->setFocus();
                    *found = true;
                    return;
                } else {
                    m_ui.filterCombo->setFocus();
                    *found = true;
                    return;
                }
            } else {
                Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
                if (!res) {
                    return;
                }
                res->treeView->setFocus();
                *found = true;
                return;
            }
        }
    } else {
        if (currentWidget == m_ui.searchCombo) {
            if (m_ui.displayOptions->isChecked()) {
                if (m_ui.searchPlaceCombo->currentIndex() < MatchModel::Folder) {
                    m_ui.displayOptions->setFocus();
                    *found = true;
                    return;
                } else if (m_ui.searchPlaceCombo->currentIndex() >= MatchModel::Folder) {
                    m_ui.sizeLimitSpinBox->setFocus();
                    *found = true;
                    return;
                } else {
                    m_ui.excludeCombo->setFocus();
                    *found = true;
                    return;
                }
            } else {
                Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
                if (!res) {
                    return;
                }
                res->treeView->setFocus();
            }
            *found = true;
            return;
        } else {
            if (currentWidget->objectName() == QLatin1String("treeView")) {
                m_ui.displayOptions->setFocus();
                *found = true;
                return;
            }
        }
    }
}

KatePluginSearchView::KatePluginSearchView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWin, KTextEditor::Application *application)
    : QObject(mainWin)
    , m_kateApp(application)
    , m_mainWindow(mainWin)
{
    KXMLGUIClient::setComponentName(QStringLiteral("katesearch"), i18n("Search & Replace"));
    setXMLFile(QStringLiteral("ui.rc"));

    m_toolView = mainWin->createToolView(plugin,
                                         QStringLiteral("kate_plugin_katesearch"),
                                         KTextEditor::MainWindow::Bottom,
                                         QIcon::fromTheme(QStringLiteral("edit-find")),
                                         i18n("Search"));

    ContainerWidget *container = new ContainerWidget(m_toolView);
    QWidget *searchUi = new QWidget(container);
    m_ui.setupUi(searchUi);

    m_tabBar = new QTabBar(container);
    m_tabBar->setAutoHide(true);
    m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
    connect(m_tabBar, &QTabBar::currentChanged, m_ui.resultWidget, &QStackedWidget::setCurrentIndex);
    m_tabBar->setElideMode(Qt::ElideMiddle);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setAutoHide(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
    KAcceleratorManager::setNoAccel(m_tabBar);
    connect(m_tabBar, &QTabBar::tabMoved, this, [this](int from, int to) {
        QWidget *fromWidget = m_ui.resultWidget->widget(from);
        m_ui.resultWidget->removeWidget(fromWidget);
        m_ui.resultWidget->insertWidget(to, fromWidget);
    });

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->addWidget(m_tabBar);
    layout->addWidget(searchUi);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    container->setFocusProxy(m_ui.searchCombo);
    connect(container, &ContainerWidget::nextFocus, this, &KatePluginSearchView::nextFocus);

    QAction *a = actionCollection()->addAction(QStringLiteral("search_in_files"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    a->setText(i18n("Find in Files"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-find")));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &KatePluginSearchView::openSearchView);

    a = actionCollection()->addAction(QStringLiteral("search_in_files_new_tab"));
    a->setText(i18n("Find in Files (in a New Tab)"));
    // first add tab, then open search view, since open search view switches to show the search options
    connect(a, &QAction::triggered, this, &KatePluginSearchView::addTab);
    connect(a, &QAction::triggered, this, &KatePluginSearchView::openSearchView);

    a = actionCollection()->addAction(QStringLiteral("go_to_next_match"));
    a->setText(i18n("Go to Next Match"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::Key_F6));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &KatePluginSearchView::goToNextMatch);

    a = actionCollection()->addAction(QStringLiteral("go_to_prev_match"));
    a->setText(i18n("Go to Previous Match"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::SHIFT | Qt::Key_F6));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &KatePluginSearchView::goToPreviousMatch);

    a = actionCollection()->addAction(QStringLiteral("cut_searched_lines"));
    a->setText(i18n("Cut Matching Lines"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-cut")));
    a->setWhatsThis(i18n("This will cut all highlighted search match lines from the current document to the clipboard"));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &KatePluginSearchView::cutSearchedLines);

    a = actionCollection()->addAction(QStringLiteral("copy_searched_lines"));
    a->setText(i18n("Copy Matching Lines"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    a->setWhatsThis(i18n("This will copy all highlighted search match lines in the current document to the clipboard"));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &KatePluginSearchView::copySearchedLines);

    // Only show the tab bar when there is more than one tab
    KAcceleratorManager::setNoAccel(m_ui.resultWidget);

    // Gnome does not seem to have all icons we want, so we use fall-back icons for those that are missing.
    QIcon dispOptIcon = QIcon::fromTheme(QStringLiteral("games-config-options"), QIcon::fromTheme(QStringLiteral("preferences-system")));
    QIcon matchCaseIcon = QIcon::fromTheme(QStringLiteral("format-text-superscript"), QIcon::fromTheme(QStringLiteral("format-text-bold")));
    QIcon useRegExpIcon = QIcon::fromTheme(QStringLiteral("code-context"), QIcon::fromTheme(QStringLiteral("edit-find-replace")));
    QIcon expandResultsIcon = QIcon::fromTheme(QStringLiteral("view-list-tree"), QIcon::fromTheme(QStringLiteral("format-indent-more")));

    m_ui.gridLayout->setSpacing(searchUi->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));
    m_ui.gridLayout->setContentsMargins(searchUi->style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                        searchUi->style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                        searchUi->style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                        searchUi->style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
    m_ui.displayOptions->setIcon(dispOptIcon);
    m_ui.searchButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-find")));
    m_ui.nextButton->setIcon(QIcon::fromTheme(QStringLiteral("go-down-search")));
    m_ui.stopButton->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    m_ui.matchCase->setIcon(matchCaseIcon);
    m_ui.useRegExp->setIcon(useRegExpIcon);
    m_ui.expandResults->setIcon(expandResultsIcon);
    m_ui.filterBtn->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    m_ui.searchPlaceCombo->setItemIcon(MatchModel::CurrentFile, QIcon::fromTheme(QStringLiteral("text-plain")));
    m_ui.searchPlaceCombo->setItemIcon(MatchModel::OpenFiles, QIcon::fromTheme(QStringLiteral("text-plain")));
    m_ui.searchPlaceCombo->setItemIcon(MatchModel::Folder, QIcon::fromTheme(QStringLiteral("folder")));
    m_ui.folderUpButton->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    m_ui.currentFolderButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_ui.newTabButton->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));

    m_ui.filterCombo->setToolTip(i18n("Comma separated list of file types to search in. Example: \"*.cpp,*.h\"\n"));
    m_ui.excludeCombo->setToolTip(i18n("Comma separated list of files and directories to exclude from the search. Example: \"build*\""));

    m_ui.filterBtn->setToolTip(i18n("Click to filter through results"));
    m_ui.filterBtn->setDisabled(true);
    connect(m_ui.filterBtn, &QToolButton::toggled, this, [this](bool on) {
        if (Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget())) {
            res->setFilterLineVisible(on);
        }
    });

    addTab();

    // get url-requester's combo box and sanely initialize
    KComboBox *cmbUrl = m_ui.folderRequester->comboBox();
    cmbUrl->setDuplicatesEnabled(false);
    cmbUrl->setEditable(true);
    m_ui.folderRequester->setMode(KFile::Directory | KFile::LocalOnly | KFile::ExistingOnly);
    KUrlCompletion *cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
    cmbUrl->setCompletionObject(cmpl);
    cmbUrl->setAutoDeleteCompletionObject(true);

    connect(m_ui.newTabButton, &QToolButton::clicked, this, &KatePluginSearchView::addTab);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &KatePluginSearchView::tabCloseRequested);
    connect(m_tabBar, &QTabBar::currentChanged, this, &KatePluginSearchView::resultTabChanged);

    connect(m_ui.folderUpButton, &QToolButton::clicked, this, &KatePluginSearchView::navigateFolderUp);
    connect(m_ui.currentFolderButton, &QToolButton::clicked, this, &KatePluginSearchView::setCurrentFolder);
    connect(m_ui.expandResults, &QToolButton::clicked, this, &KatePluginSearchView::expandResults);

    connect(m_ui.searchCombo, &QComboBox::editTextChanged, &m_changeTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_ui.matchCase, &QToolButton::toggled, &m_changeTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_ui.matchCase, &QToolButton::toggled, this, [this] {
        Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
        if (res) {
            res->matchCase = m_ui.matchCase->isChecked();
        }
    });

    connect(m_ui.searchCombo->lineEdit(), &QLineEdit::returnPressed, this, &KatePluginSearchView::startSearch);
    // connecting to returnPressed() of the folderRequester doesn't work, I haven't found out why yet. But connecting to the linedit works:
    connect(m_ui.folderRequester->comboBox()->lineEdit(), &QLineEdit::returnPressed, this, &KatePluginSearchView::startSearch);
    connect(m_ui.filterCombo, static_cast<void (KComboBox::*)(const QString &)>(&KComboBox::returnPressed), this, &KatePluginSearchView::startSearch);
    connect(m_ui.excludeCombo, static_cast<void (KComboBox::*)(const QString &)>(&KComboBox::returnPressed), this, &KatePluginSearchView::startSearch);
    connect(m_ui.searchButton, &QPushButton::clicked, this, &KatePluginSearchView::startSearch);

    connect(m_ui.displayOptions, &QToolButton::toggled, this, &KatePluginSearchView::toggleOptions);
    connect(m_ui.searchPlaceCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KatePluginSearchView::searchPlaceChanged);

    connect(m_ui.stopButton, &QPushButton::clicked, this, &KatePluginSearchView::stopClicked);

    connect(m_ui.nextButton, &QToolButton::clicked, this, &KatePluginSearchView::goToNextMatch);

    connect(m_ui.replaceButton, &QPushButton::clicked, this, &KatePluginSearchView::replaceSingleMatch);
    connect(m_ui.replaceCheckedBtn, &QPushButton::clicked, this, &KatePluginSearchView::replaceChecked);
    connect(m_ui.replaceCombo->lineEdit(), &QLineEdit::returnPressed, this, &KatePluginSearchView::replaceChecked);

    m_ui.displayOptions->setChecked(true);

    connect(&m_searchOpenFiles, &SearchOpenFiles::matchesFound, this, &KatePluginSearchView::matchesFound);
    connect(&m_searchOpenFiles, &SearchOpenFiles::searchDone, this, &KatePluginSearchView::searchDone);

    m_diskSearchDoneTimer.setSingleShot(true);
    m_diskSearchDoneTimer.setInterval(10);
    connect(&m_diskSearchDoneTimer, &QTimer::timeout, this, &KatePluginSearchView::searchDone);

    m_updateCheckedStateTimer.setSingleShot(true);
    m_updateCheckedStateTimer.setInterval(10);
    connect(&m_updateCheckedStateTimer, &QTimer::timeout, this, &KatePluginSearchView::updateMatchMarks);

    // queued connect to signals emitted outside of background thread
    connect(&m_folderFilesList, &FolderFilesList::fileListReady, this, &KatePluginSearchView::folderFileListChanged, Qt::QueuedConnection);
    connect(
        &m_folderFilesList,
        &FolderFilesList::searching,
        this,
        [this](const QString &path) {
            Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
            if (res) {
                res->matchModel.setFileListUpdate(path);
            }
        },
        Qt::QueuedConnection);

    connect(m_kateApp, &KTextEditor::Application::documentWillBeDeleted, this, &KatePluginSearchView::clearDocMarksAndRanges);
    connect(m_kateApp, &KTextEditor::Application::documentWillBeDeleted, &m_searchOpenFiles, &SearchOpenFiles::cancelSearch);
    connect(m_kateApp, &KTextEditor::Application::documentWillBeDeleted, this, [this]() {
        Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
        if (res) {
            res->matchModel.cancelReplace();
        }
    });

    m_ui.searchCombo->lineEdit()->setPlaceholderText(i18n("Find"));
    // Hook into line edit context menus
    m_ui.searchCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.searchCombo, &QComboBox::customContextMenuRequested, this, &KatePluginSearchView::searchContextMenu);
    m_ui.searchCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    m_ui.searchCombo->completer()->setCaseSensitivity(Qt::CaseSensitive);
    m_ui.searchCombo->setInsertPolicy(QComboBox::NoInsert);
    m_ui.searchCombo->lineEdit()->setClearButtonEnabled(true);
    m_ui.searchCombo->setMaxCount(25);
    QAction *searchComboActionForInsertRegexButton =
        m_ui.searchCombo->lineEdit()->addAction(QIcon::fromTheme(QStringLiteral("code-context"), QIcon::fromTheme(QStringLiteral("edit-find-replace"))),
                                                QLineEdit::TrailingPosition);
    connect(searchComboActionForInsertRegexButton, &QAction::triggered, this, [this]() {
        QMenu menu;
        QSet<QAction *> actionList;
        addRegexHelperActionsForSearch(&actionList, &menu);
        auto &&action = menu.exec(QCursor::pos());
        regexHelperActOnAction(action, actionList, m_ui.searchCombo->lineEdit());
    });

    m_ui.replaceCombo->lineEdit()->setPlaceholderText(i18n("Replace"));
    // Hook into line edit context menus
    m_ui.replaceCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.replaceCombo, &QComboBox::customContextMenuRequested, this, &KatePluginSearchView::replaceContextMenu);
    m_ui.replaceCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    m_ui.replaceCombo->completer()->setCaseSensitivity(Qt::CaseSensitive);
    m_ui.replaceCombo->setInsertPolicy(QComboBox::NoInsert);
    m_ui.replaceCombo->lineEdit()->setClearButtonEnabled(true);
    m_ui.replaceCombo->setMaxCount(25);
    QAction *replaceComboActionForInsertRegexButton =
        m_ui.replaceCombo->lineEdit()->addAction(QIcon::fromTheme(QStringLiteral("code-context")), QLineEdit::TrailingPosition);
    connect(replaceComboActionForInsertRegexButton, &QAction::triggered, this, [this]() {
        QMenu menu;
        QSet<QAction *> actionList;
        addRegexHelperActionsForReplace(&actionList, &menu);
        auto &&action = menu.exec(QCursor::pos());
        regexHelperActOnAction(action, actionList, m_ui.replaceCombo->lineEdit());
    });
    QAction *replaceComboActionForInsertSpecialButton = m_ui.replaceCombo->lineEdit()->addAction(QIcon::fromTheme(QStringLiteral("insert-text")), //
                                                                                                 QLineEdit::TrailingPosition);
    connect(replaceComboActionForInsertSpecialButton, &QAction::triggered, this, [this]() {
        QMenu menu;
        QSet<QAction *> actionList;
        addSpecialCharsHelperActionsForReplace(&actionList, &menu);
        auto &&action = menu.exec(QCursor::pos());
        regexHelperActOnAction(action, actionList, m_ui.replaceCombo->lineEdit());
    });

    connect(m_ui.useRegExp, &QToolButton::toggled, &m_changeTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    auto onRegexToggleChanged = [this, searchComboActionForInsertRegexButton, replaceComboActionForInsertRegexButton] {
        Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
        if (res) {
            bool useRegExp = m_ui.useRegExp->isChecked();
            res->useRegExp = useRegExp;
            searchComboActionForInsertRegexButton->setVisible(useRegExp);
            replaceComboActionForInsertRegexButton->setVisible(useRegExp);
        }
    };
    connect(m_ui.useRegExp, &QToolButton::toggled, this, onRegexToggleChanged);
    onRegexToggleChanged(); // invoke initially
    m_changeTimer.setInterval(300);
    m_changeTimer.setSingleShot(true);
    connect(&m_changeTimer, &QTimer::timeout, this, &KatePluginSearchView::startSearchWhileTyping);

    m_toolView->setMinimumHeight(container->sizeHint().height());

    connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &KatePluginSearchView::handleEsc);

    // watch for project plugin view creation/deletion
    connect(m_mainWindow, &KTextEditor::MainWindow::pluginViewCreated, this, &KatePluginSearchView::slotPluginViewCreated);

    connect(m_mainWindow, &KTextEditor::MainWindow::pluginViewDeleted, this, &KatePluginSearchView::slotPluginViewDeleted);

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KatePluginSearchView::updateMatchMarks);

    // Connect signals from project plugin to our slots
    m_projectPluginView = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    slotPluginViewCreated(QStringLiteral("kateprojectplugin"), m_projectPluginView);

    searchPlaceChanged();

    m_toolView->installEventFilter(this);

    m_mainWindow->guiFactory()->addClient(this);

    // We dont want our shortcuts available in the whole mainwindow
    // It can conflict with Konsole shortcuts for e.g.,
    actionCollection()->removeAssociatedWidget(m_mainWindow->window());
    if (auto vm = m_mainWindow->window()->findChild<QWidget *>(QStringLiteral("KateViewManager"))) {
        actionCollection()->addAssociatedWidget(vm);
    }
    actionCollection()->addAssociatedWidget(container);

    auto e = KTextEditor::Editor::instance();
    connect(e, &KTextEditor::Editor::configChanged, this, &KatePluginSearchView::updateViewColors);
    updateViewColors();
}

KatePluginSearchView::~KatePluginSearchView()
{
    cancelDiskFileSearch();
    clearMarksAndRanges();
    m_mainWindow->guiFactory()->removeClient(this);
    delete m_toolView;
}

QList<int> KatePluginSearchView::getDocumentSearchMarkedLines(KTextEditor::Document *currentDocument)
{
    QList<int> result;
    if (!currentDocument) {
        return result;
    }
    QHash<int, KTextEditor::Mark *> documentMarksHash = currentDocument->marks();
    auto searchMarkType = KTextEditor::Document::SearchMatch;
    for (const int markedLineNumber : documentMarksHash.keys()) {
        auto documentMarkTypeMask = documentMarksHash.value(markedLineNumber)->type;
        if ((searchMarkType & documentMarkTypeMask) != searchMarkType)
            continue;
        result.push_back(markedLineNumber);
    }
    std::sort(result.begin(), result.end());
    return result;
}

void KatePluginSearchView::setClipboardFromDocumentLines(const KTextEditor::Document *currentDocument, const QList<int> lineNumbers)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString text;
    for (int lineNumber : lineNumbers) {
        text += currentDocument->line(lineNumber);
        text += QLatin1String("\n");
    }
    clipboard->setText(text);
}

void KatePluginSearchView::cutSearchedLines()
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    KTextEditor::Document *currentDocument = m_mainWindow->activeView()->document();
    if (!currentDocument) {
        return;
    }

    QList<int> lineNumbers = getDocumentSearchMarkedLines(currentDocument);
    setClipboardFromDocumentLines(currentDocument, lineNumbers);

    // Iterate in descending line number order to remove the search matched lines to complete
    // the "cut" action.
    // Make one transaction for the whole remove to get all removes in one "undo"
    KTextEditor::Document::EditingTransaction transaction(currentDocument);
    for (auto iter = lineNumbers.rbegin(); iter != lineNumbers.rend(); ++iter) {
        int lineNumber = *iter;
        currentDocument->removeLine(lineNumber);
    }
}

void KatePluginSearchView::copySearchedLines()
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    KTextEditor::Document *currentDocument = m_mainWindow->activeView()->document();
    if (!currentDocument) {
        return;
    }

    QList<int> lineNumbers = getDocumentSearchMarkedLines(currentDocument);
    setClipboardFromDocumentLines(currentDocument, lineNumbers);
}

void KatePluginSearchView::navigateFolderUp()
{
    // navigate one folder up
    m_ui.folderRequester->setUrl(localFileDirUp(m_ui.folderRequester->url()));
}

void KatePluginSearchView::setCurrentFolder()
{
    if (!m_mainWindow) {
        return;
    }
    KTextEditor::View *editView = m_mainWindow->activeView();
    if (editView && editView->document()) {
        // upUrl as we want the folder not the file
        m_ui.folderRequester->setUrl(localFileDirUp(editView->document()->url()));
    }
    m_ui.displayOptions->setChecked(true);
}

void KatePluginSearchView::openSearchView()
{
    if (!m_mainWindow) {
        return;
    }
    if (!m_toolView->isVisible()) {
        m_mainWindow->showToolView(m_toolView);
    }
    m_ui.searchCombo->setFocus(Qt::OtherFocusReason);
    if (m_ui.searchPlaceCombo->currentIndex() == MatchModel::Folder) {
        m_ui.displayOptions->setChecked(true);
    }

    KTextEditor::View *editView = m_mainWindow->activeView();
    if (editView && editView->document()) {
        if (m_ui.folderRequester->text().isEmpty()) {
            // upUrl as we want the folder not the file
            m_ui.folderRequester->setUrl(localFileDirUp(editView->document()->url()));
        }
        QString selection;
        if (editView->selection()) {
            selection = editView->selectionText();
            // remove possible trailing '\n'
            if (selection.endsWith(QLatin1Char('\n'))) {
                selection = selection.left(selection.size() - 1);
            }
        }
        if (selection.isEmpty()) {
            selection = editView->document()->wordAt(editView->cursorPosition());
        }

        if (!selection.isEmpty() && !selection.contains(QLatin1Char('\n'))) {
            m_ui.searchCombo->blockSignals(true);
            m_ui.searchCombo->lineEdit()->setText(selection);
            m_ui.searchCombo->blockSignals(false);
        }

        m_ui.searchCombo->lineEdit()->selectAll();
        m_searchJustOpened = true;
        startSearchWhileTyping();
    }
}

void KatePluginSearchView::handleEsc(QEvent *e)
{
    if (!m_mainWindow) {
        return;
    }

    QKeyEvent *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        static ulong lastTimeStamp;
        if (lastTimeStamp == k->timestamp()) {
            // Same as previous... This looks like a bug somewhere...
            return;
        }
        lastTimeStamp = k->timestamp();
        if (!m_matchRanges.isEmpty()) {
            clearMarksAndRanges();
        } else if (m_toolView->isVisible()) {
            m_mainWindow->hideToolView(m_toolView);
        }
        // uncheck all so no new marks are added again when switching views
        Results *curResults = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
        if (curResults) {
            curResults->matchModel.uncheckAll();
        }
    }
}

void KatePluginSearchView::setSearchString(const QString &pattern)
{
    m_ui.searchCombo->lineEdit()->setText(pattern);
}

void KatePluginSearchView::toggleOptions(bool show)
{
    m_ui.stackedWidget->setCurrentIndex((show) ? 1 : 0);
    Results *curResults = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (curResults) {
        curResults->displayFolderOptions = show;
    }
}

void KatePluginSearchView::setSearchPlace(int place)
{
    if (place >= m_ui.searchPlaceCombo->count()) {
        // This probably means the project plugin is not active or no project loaded
        // fallback to search in folder
        qDebug() << place << "is not a valid search place index";
        place = MatchModel::Folder;
    }
    m_ui.searchPlaceCombo->setCurrentIndex(place);
}

QStringList KatePluginSearchView::filterFiles(const QStringList &files) const
{
    QString types = m_ui.filterCombo->currentText();
    QString excludes = m_ui.excludeCombo->currentText();
    if (((types.isEmpty() || types == QLatin1String("*"))) && (excludes.isEmpty())) {
        // shortcut for use all files
        return files;
    }

    if (types.isEmpty()) {
        types = QStringLiteral("*");
    }

    const QStringList tmpTypes = types.split(QLatin1Char(','), Qt::SkipEmptyParts);
    QList<QRegularExpression> typeList;
    for (const auto &type : tmpTypes) {
        typeList << QRegularExpression(QRegularExpression::wildcardToRegularExpression(type.trimmed()));
    }

    const QStringList tmpExcludes = excludes.split(QLatin1Char(','), Qt::SkipEmptyParts);
    QList<QRegularExpression> excludeList;
    for (const auto &exclude : tmpExcludes) {
        excludeList << QRegularExpression(QRegularExpression::wildcardToRegularExpression(exclude.trimmed()));
    }

    QStringList filteredFiles;
    for (const QString &filePath : files) {
        bool isInSubDir = filePath.startsWith(m_resultBaseDir);
        QString nameToCheck = filePath;
        if (isInSubDir) {
            nameToCheck = filePath.mid(m_resultBaseDir.size());
        }

        bool skip = false;
        const QStringList pathSplit = nameToCheck.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        for (const auto &regex : std::as_const(excludeList)) {
            for (const auto &part : pathSplit) {
                QRegularExpressionMatch match = regex.match(part);
                if (match.hasMatch()) {
                    skip = true;
                    break;
                }
            }
        }
        if (skip) {
            continue;
        }

        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();

        for (const auto &regex : std::as_const(typeList)) {
            QRegularExpressionMatch match = regex.match(fileName);
            if (match.hasMatch()) {
                filteredFiles << filePath;
                break;
            }
        }
    }
    return filteredFiles;
}

void KatePluginSearchView::folderFileListChanged()
{
    if (!m_searchingTab) {
        qWarning() << "This is a bug";
        searchDone();
        return;
    }
    QStringList fileList = m_folderFilesList.fileList();

    if (fileList.isEmpty()) {
        searchDone();
        return;
    }

    QList<KTextEditor::Document *> openList;
    const auto documents = m_kateApp->documents();
    for (int i = 0; i < documents.size(); i++) {
        int index = fileList.indexOf(documents[i]->url().toLocalFile());
        if (index != -1) {
            openList << documents[i];
            fileList.removeAt(index);
        }
    }

    // search order is important: Open files starts immediately and should finish
    // earliest after first event loop.
    // The DiskFile might finish immediately
    if (!openList.empty()) {
        m_searchOpenFiles.startSearch(openList, m_searchingTab->regExp);
    }

    startDiskFileSearch(fileList, m_searchingTab->regExp, m_ui.binaryCheckBox->isChecked(), m_ui.sizeLimitSpinBox->value());
}

void KatePluginSearchView::startDiskFileSearch(const QStringList &fileList, const QRegularExpression &reg, const bool includeBinaryFiles, const int sizeLimit)
{
    if (fileList.isEmpty()) {
        searchDone();
        return;
    }

    // spread work to X threads => default to ideal thread count
    const int threadCount = m_searchDiskFilePool.maxThreadCount();

    // init worklist for these number of threads
    m_worklistForDiskFiles.init(fileList, threadCount);

    // spawn enough runnables, they will pull the files themself from our worklist
    // this must exactly match the count we used to init the worklist above, as this is used to finalize stuff!
    for (int i = 0; i < threadCount; ++i) {
        // new runnable, will pull work from the worklist itself!
        // worklist is used to drive if we need to stop the work, too!
        SearchDiskFiles *runner = new SearchDiskFiles(m_worklistForDiskFiles, reg, includeBinaryFiles, sizeLimit);

        // queued connection for the results, this is emitted by a different thread than the runnable object and this one!
        connect(runner, &SearchDiskFiles::matchesFound, this, &KatePluginSearchView::matchesFound, Qt::QueuedConnection);

        // queued connection for the results, this is emitted by a different thread than the runnable object and this one!
        connect(
            runner,
            &SearchDiskFiles::destroyed,
            this,
            [this]() {
                // signal the worklist one runnable more is done
                m_worklistForDiskFiles.markOnRunnableAsDone();

                // if no longer anything running, signal finished!
                if (!m_worklistForDiskFiles.isRunning()) {
                    if (!m_diskSearchDoneTimer.isActive()) {
                        m_diskSearchDoneTimer.start();
                    }
                }
            },
            Qt::QueuedConnection);

        // launch the runnable
        m_searchDiskFilePool.start(runner);
    }
}

void KatePluginSearchView::cancelDiskFileSearch()
{
    // signal canceling to runnables
    m_worklistForDiskFiles.cancel();

    // wait for finalization
    m_searchDiskFilePool.clear();
    m_searchDiskFilePool.waitForDone();
}

bool KatePluginSearchView::searchingDiskFiles()
{
    return m_worklistForDiskFiles.isRunning() || m_folderFilesList.isRunning();
}

void KatePluginSearchView::searchPlaceChanged()
{
    int searchPlace = m_ui.searchPlaceCombo->currentIndex();
    const bool inFolder = (searchPlace == MatchModel::Folder);

    if (searchPlace < MatchModel::Folder) {
        m_ui.displayOptions->setChecked(false);
        m_ui.displayOptions->setEnabled(false);
    } else {
        m_ui.displayOptions->setEnabled(true);
        if (qobject_cast<QComboBox *>(sender())) {
            // Only display the options if the change was due to a direct
            // index change in the combo-box triggered by the user
            m_ui.displayOptions->setChecked(true);
        }
    }

    m_ui.filterCombo->setEnabled(searchPlace >= MatchModel::Folder);
    m_ui.excludeCombo->setEnabled(searchPlace >= MatchModel::Folder);
    m_ui.folderRequester->setEnabled(inFolder);
    m_ui.folderUpButton->setEnabled(inFolder);
    m_ui.currentFolderButton->setEnabled(inFolder);
    m_ui.recursiveCheckBox->setEnabled(inFolder);
    m_ui.hiddenCheckBox->setEnabled(inFolder);
    m_ui.symLinkCheckBox->setEnabled(inFolder);
    m_ui.binaryCheckBox->setEnabled(inFolder);

    if (inFolder && sender() == m_ui.searchPlaceCombo) {
        setCurrentFolder();
    }

    // ... and the labels:
    m_ui.folderLabel->setEnabled(m_ui.folderRequester->isEnabled());
    m_ui.filterLabel->setEnabled(m_ui.filterCombo->isEnabled());
    m_ui.excludeLabel->setEnabled(m_ui.excludeCombo->isEnabled());

    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (res) {
        res->searchPlaceIndex = searchPlace;
    }
}

void KatePluginSearchView::matchesFound(const QUrl &url, const QList<KateSearchMatch> &searchMatches, KTextEditor::Document *doc)
{
    if (!m_searchingTab) {
        qWarning() << "BUG: A search tab should be set when search results arrive";
        return;
    }

    m_searchingTab->matchModel.addMatches(url, searchMatches, doc);
    m_searchingTab->matches += searchMatches.size();
}

void KatePluginSearchView::stopClicked()
{
    m_folderFilesList.terminateSearch();
    m_searchOpenFiles.cancelSearch();
    cancelDiskFileSearch();
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (res) {
        res->matchModel.cancelReplace();
    }
}

/**
 * update the search widget colors and font. This is done on start of every
 * search so that if the user changes the theme, he can see the new colors
 * on the next search
 */
void KatePluginSearchView::updateViewColors()
{
    auto *e = KTextEditor::Editor::instance();
    const auto theme = e->theme();

    auto search = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight));
    auto replace = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::ReplaceHighlight));
    auto fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));

    if (!m_resultAttr) {
        m_resultAttr = new KTextEditor::Attribute();
    }
    m_resultAttr->clear();
    m_resultAttr->setBackground(search);
    m_resultAttr->setForeground(fg);

    m_replaceHighlightColor = replace;
}

// static QElapsedTimer s_timer;
void KatePluginSearchView::startSearch()
{
    // s_timer.start();
    if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier) {
        // search in new tab
        const QString tmpSearchString = m_ui.searchCombo->currentText();
        const QString tmpReplaceString = m_ui.replaceCombo->currentText();
        addTab();
        m_ui.searchCombo->setCurrentText(tmpSearchString);
        m_ui.replaceCombo->setCurrentText(tmpReplaceString);
    }

    // Forcefully stop any ongoing search or replace
    m_folderFilesList.terminateSearch();
    m_searchOpenFiles.terminateSearch();
    cancelDiskFileSearch();

    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (res) {
        res->matchModel.cancelReplace();
    }

    m_changeTimer.stop(); // make sure not to start a "while you type" search now
    m_mainWindow->showToolView(m_toolView); // in case we are invoked from the command interface
    m_projectSearchPlaceIndex = 0; // now that we started, don't switch back automatically

    if (m_ui.searchCombo->currentText().isEmpty()) {
        // return pressed in the folder combo or filter combo
        clearMarksAndRanges();
        return;
    }

    KTextEditor::View *activeView = m_mainWindow->activeView();
    if ((m_ui.searchPlaceCombo->currentIndex() == MatchModel::CurrentFile && !activeView)
        || (m_ui.searchPlaceCombo->currentIndex() == MatchModel::OpenFiles && m_kateApp->documents().isEmpty())) {
        return;
    }

    m_isSearchAsYouType = false;

    QString currentSearchText = m_ui.searchCombo->currentText();
    m_ui.searchCombo->setItemText(0, QString()); // remove the text from index 0 on enter/search
    int index = m_ui.searchCombo->findText(currentSearchText);
    if (index > 0) {
        m_ui.searchCombo->removeItem(index);
    }
    m_ui.searchCombo->insertItem(1, currentSearchText);
    m_ui.searchCombo->setCurrentIndex(1);

    if (m_ui.filterCombo->findText(m_ui.filterCombo->currentText()) == -1) {
        m_ui.filterCombo->insertItem(0, m_ui.filterCombo->currentText());
        m_ui.filterCombo->setCurrentIndex(0);
    }
    if (m_ui.excludeCombo->findText(m_ui.excludeCombo->currentText()) == -1) {
        m_ui.excludeCombo->insertItem(0, m_ui.excludeCombo->currentText());
        m_ui.excludeCombo->setCurrentIndex(0);
    }
    if (m_ui.folderRequester->comboBox()->findText(m_ui.folderRequester->comboBox()->currentText()) == -1) {
        m_ui.folderRequester->comboBox()->insertItem(0, m_ui.folderRequester->comboBox()->currentText());
        m_ui.folderRequester->comboBox()->setCurrentIndex(0);
    }
    m_searchingTab = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!m_searchingTab) {
        qWarning() << "BUG: Failed to find a search tab";
        return;
    }

    QString pattern = (m_ui.useRegExp->isChecked() ? currentSearchText : QRegularExpression::escape(currentSearchText));
    QRegularExpression::PatternOptions patternOptions = QRegularExpression::UseUnicodePropertiesOption;
    if (!m_ui.matchCase->isChecked()) {
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    }

    if (m_ui.useRegExp->isChecked() && pattern.contains(QLatin1String("\\n"))) {
        patternOptions |= QRegularExpression::MultilineOption;
    }
    QRegularExpression reg(pattern, patternOptions);

    if (!reg.isValid()) {
        m_ui.searchCombo->setToolTip(reg.errorString());
        indicateMatch(MatchType::InvalidRegExp);
        return;
    }
    m_ui.searchCombo->setToolTip(QString());

    Q_EMIT searchBusy(true);

    m_searchingTab->searchStr = currentSearchText;
    m_searchingTab->regExp = reg;
    m_searchingTab->useRegExp = m_ui.useRegExp->isChecked();
    m_searchingTab->matchCase = m_ui.matchCase->isChecked();
    m_searchingTab->searchPlaceIndex = m_ui.searchPlaceCombo->currentIndex();

    m_ui.newTabButton->setDisabled(true);
    m_ui.searchCombo->setDisabled(true);
    m_ui.searchButton->setDisabled(true);
    m_ui.displayOptions->setChecked(false);
    m_ui.displayOptions->setDisabled(true);
    m_ui.replaceCheckedBtn->setDisabled(true);
    m_ui.replaceButton->setDisabled(true);
    m_ui.stopAndNext->setCurrentWidget(m_ui.stopButton);
    m_ui.replaceCombo->setDisabled(true);
    m_ui.searchPlaceCombo->setDisabled(true);
    m_ui.useRegExp->setDisabled(true);
    m_ui.matchCase->setDisabled(true);
    m_ui.expandResults->setDisabled(true);
    m_ui.currentFolderButton->setDisabled(true);

    clearMarksAndRanges();
    m_searchingTab->matches = 0;

    // BUG: 441340 We need to escape the & because it is used for accelerators/shortcut mnemonic by default
    QString tabName = m_ui.searchCombo->currentText();
    tabName.replace(QLatin1Char('&'), QLatin1String("&&"));
    m_tabBar->setTabText(m_ui.resultWidget->currentIndex(), tabName);

    m_toolView->setCursor(Qt::WaitCursor);

    const bool inCurrentProject = m_ui.searchPlaceCombo->currentIndex() == MatchModel::Project;
    const bool inAllOpenProjects = m_ui.searchPlaceCombo->currentIndex() == MatchModel::AllProjects;

    m_searchingTab->matchModel.clear();
    m_searchingTab->matchModel.setSearchPlace(static_cast<MatchModel::SearchPlaces>(m_searchingTab->searchPlaceIndex));
    m_searchingTab->matchModel.setSearchState(MatchModel::Searching);
    m_searchingTab->expandRoot();

    if (m_ui.searchPlaceCombo->currentIndex() == MatchModel::CurrentFile) {
        m_resultBaseDir.clear();
        QList<KTextEditor::Document *> documents;
        if (activeView) {
            documents << activeView->document();
        }

        m_searchOpenFiles.startSearch(documents, reg);
    } else if (m_ui.searchPlaceCombo->currentIndex() == MatchModel::OpenFiles) {
        m_resultBaseDir.clear();
        const QList<KTextEditor::Document *> documents = m_kateApp->documents();
        m_searchOpenFiles.startSearch(documents, reg);
    } else if (m_ui.searchPlaceCombo->currentIndex() == MatchModel::Folder) {
        m_resultBaseDir = m_ui.folderRequester->url().toLocalFile();
        if (!m_resultBaseDir.isEmpty() && !m_resultBaseDir.endsWith(QLatin1Char('/'))) {
            m_resultBaseDir += QLatin1Char('/');
        }
        m_searchingTab->matchModel.setBaseSearchPath(m_resultBaseDir);
        m_folderFilesList.generateList(m_resultBaseDir,
                                       m_ui.recursiveCheckBox->isChecked(),
                                       m_ui.hiddenCheckBox->isChecked(),
                                       m_ui.symLinkCheckBox->isChecked(),
                                       m_ui.filterCombo->currentText(),
                                       m_ui.excludeCombo->currentText());
        // the file list will be ready when the thread returns (connected to folderFileListChanged)
    } else if (inCurrentProject || inAllOpenProjects) {
        /**
         * init search with file list from current project, if any
         */
        m_resultBaseDir.clear();
        QStringList files;
        if (m_projectPluginView) {
            if (inCurrentProject) {
                m_resultBaseDir = m_projectPluginView->property("projectBaseDir").toString();
                m_searchingTab->matchModel.setProjectName(m_projectPluginView->property("projectName").toString());
            } else {
                m_resultBaseDir = m_projectPluginView->property("allProjectsCommonBaseDir").toString();
                m_searchingTab->matchModel.setProjectName(m_projectPluginView->property("projectName").toString());
            }

            if (!m_resultBaseDir.endsWith(QLatin1Char('/'))) {
                m_resultBaseDir += QLatin1Char('/');
            }

            QStringList projectFiles;
            if (inCurrentProject) {
                projectFiles = m_projectPluginView->property("projectFiles").toStringList();
            } else {
                projectFiles = m_projectPluginView->property("allProjectsFiles").toStringList();
            }

            files = filterFiles(projectFiles);
        }
        m_searchingTab->matchModel.setBaseSearchPath(m_resultBaseDir);

        QList<KTextEditor::Document *> openList;
        const auto docs = m_kateApp->documents();
        for (const auto doc : docs) {
            // match project file's list toLocalFile()
            int index = files.indexOf(doc->url().toLocalFile());
            if (index != -1) {
                openList << doc;
                files.removeAt(index);
            }
        }
        // search order is important: Open files starts immediately and should finish
        // earliest after first event loop.
        // The DiskFile might finish immediately
        if (!openList.empty()) {
            m_searchOpenFiles.startSearch(openList, m_searchingTab->regExp);
        }
        // We don't want to search for binary files in the project, so false is used instead of the checkbox
        // which is disabled in this case
        startDiskFileSearch(files, m_searchingTab->regExp, false, m_ui.sizeLimitSpinBox->value());
    } else {
        qDebug() << "Case not handled:" << m_ui.searchPlaceCombo->currentIndex();
        Q_ASSERT_X(false, "KatePluginSearchView::startSearch", "case not handled");
    }
}

void KatePluginSearchView::startSearchWhileTyping()
{
    if (searchingDiskFiles() || m_searchOpenFiles.searching()) {
        return;
    }

    // If the user has selected to disable search as you type, stop here
    int searchPlace = m_ui.searchPlaceCombo->currentIndex();
    if (!m_searchAsYouType.value(static_cast<MatchModel::SearchPlaces>(searchPlace), true)) {
        return;
    }

    QString currentSearchText = m_ui.searchCombo->currentText();

    m_ui.searchButton->setDisabled(currentSearchText.isEmpty());

    if (!m_mainWindow->activeView()) {
        return;
    }

    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    if (!doc) {
        return;
    }

    m_searchingTab = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!m_searchingTab) {
        qWarning() << "BUG: Failed to find a search tab";
        return;
    }

    // check if we typed something or just changed combobox index
    // changing index should not trigger a search-as-you-type
    if (m_ui.searchCombo->currentIndex() > 0 && currentSearchText == m_ui.searchCombo->itemText(m_ui.searchCombo->currentIndex())) {
        return;
    }

    // Now we should have a true typed text change
    m_isSearchAsYouType = true;
    clearMarksAndRanges();

    QString pattern = (m_ui.useRegExp->isChecked() ? currentSearchText : QRegularExpression::escape(currentSearchText));
    QRegularExpression::PatternOptions patternOptions = QRegularExpression::UseUnicodePropertiesOption;
    if (!m_ui.matchCase->isChecked()) {
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    }
    if (m_searchingTab->useRegExp && pattern.contains(QLatin1String("\\n"))) {
        patternOptions |= QRegularExpression::MultilineOption;
    }
    QRegularExpression reg(pattern, patternOptions);

    // If we have an invalid regex it could be caused by the user not having finished the query,
    // consequently we should ignore it skip the search but don't display an error message'
    if (!reg.isValid()) {
        indicateMatch(MatchType::InvalidRegExp);
        return;
    }
    m_ui.searchCombo->setToolTip(QString());

    Q_EMIT searchBusy(true);

    m_searchingTab->regExp = reg;
    m_searchingTab->useRegExp = m_ui.useRegExp->isChecked();

    m_ui.replaceCheckedBtn->setDisabled(true);
    m_ui.replaceButton->setDisabled(true);
    m_ui.nextButton->setDisabled(true);

    int cursorPosition = m_ui.searchCombo->lineEdit()->cursorPosition();
    bool hasSelected = m_ui.searchCombo->lineEdit()->hasSelectedText();
    m_ui.searchCombo->blockSignals(true);
    if (m_ui.searchCombo->count() == 0) {
        m_ui.searchCombo->insertItem(0, currentSearchText);
    } else {
        m_ui.searchCombo->setItemText(0, currentSearchText);
    }
    m_ui.searchCombo->setCurrentIndex(0);
    m_ui.searchCombo->lineEdit()->setCursorPosition(cursorPosition);
    if (hasSelected) {
        // This restores the select all from invoking openSearchView
        // This selects too much if we have a partial selection and toggle match-case/regexp
        m_ui.searchCombo->lineEdit()->selectAll();
    }
    m_ui.searchCombo->blockSignals(false);

    // Prepare for the new search content
    m_resultBaseDir.clear();
    m_searchingTab->matches = 0;

    m_searchingTab->matchModel.clear();
    m_searchingTab->matchModel.setSearchPlace(MatchModel::CurrentFile);
    m_searchingTab->matchModel.setSearchState(MatchModel::Searching);
    m_searchingTab->expandRoot();

    // Do the search
    int searchStoppedAt = m_searchOpenFiles.searchOpenFile(doc, reg, 0);
    searchWhileTypingDone();

    if (searchStoppedAt != 0) {
        delete m_infoMessage;
        const QString msg = i18n("Searching while you type was interrupted. It would have taken too long.");
        m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Warning);
        m_infoMessage->setPosition(KTextEditor::Message::TopInView);
        m_infoMessage->setAutoHide(3000);
        m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
        m_infoMessage->setView(m_mainWindow->activeView());
        m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
    }

    // We need to escape the & because it is used for accelerators/shortcut mnemonic by default
    QString tabName = m_ui.searchCombo->currentText();
    tabName.replace(QLatin1Char('&'), QLatin1String("&&"));
    m_tabBar->setTabText(m_ui.resultWidget->currentIndex(), tabName);
}

void KatePluginSearchView::searchDone()
{
    m_changeTimer.stop(); // avoid "while you type" search directly after

    if (searchingDiskFiles() || m_searchOpenFiles.searching()) {
        return;
    }

    QWidget *fw = QApplication::focusWidget();
    // NOTE: we take the focus widget here before the enabling/disabling
    // moves the focus around.
    m_ui.newTabButton->setDisabled(false);
    m_ui.searchCombo->setDisabled(false);
    m_ui.searchButton->setDisabled(false);
    m_ui.stopAndNext->setCurrentWidget(m_ui.nextButton);
    m_ui.displayOptions->setDisabled(false);
    m_ui.replaceCombo->setDisabled(false);
    m_ui.searchPlaceCombo->setDisabled(false);
    m_ui.useRegExp->setDisabled(false);
    m_ui.matchCase->setDisabled(false);
    m_ui.expandResults->setDisabled(false);
    m_ui.currentFolderButton->setDisabled(m_ui.searchPlaceCombo->currentIndex() != MatchModel::Folder);

    Q_EMIT searchBusy(false);

    if (!m_searchingTab) {
        return;
    }

    m_ui.replaceCheckedBtn->setDisabled(m_searchingTab->matches < 1);
    m_ui.replaceButton->setDisabled(m_searchingTab->matches < 1);
    m_ui.nextButton->setDisabled(m_searchingTab->matches < 1);
    m_ui.filterBtn->setDisabled(m_searchingTab->matches <= 1);

    // Set search to done. This sorts the model and collapses all items in the view
    m_searchingTab->matchModel.setSearchState(MatchModel::SearchDone);

    // expand the "header item " to display all files and all results if configured
    expandResults();

    m_searchingTab->treeView->resizeColumnToContents(0);

    indicateMatch(m_searchingTab->matches > 0 ? MatchType::HasMatch : MatchType::NoMatch);

    m_toolView->unsetCursor();

    if (fw == m_ui.stopButton) {
        m_ui.searchCombo->setFocus();
    }

    m_searchJustOpened = false;
    m_searchingTab->searchStr = m_ui.searchCombo->currentText();
    m_searchingTab = nullptr;
    updateMatchMarks();

    // qDebug() << "done:" << s_timer.elapsed();
}

void KatePluginSearchView::searchWhileTypingDone()
{
    Q_EMIT searchBusy(false);

    if (!m_searchingTab) {
        return;
    }

    bool popupVisible = m_ui.searchCombo->lineEdit()->completer()->popup()->isVisible();

    m_ui.replaceCheckedBtn->setDisabled(m_searchingTab->matches < 1);
    m_ui.replaceButton->setDisabled(m_searchingTab->matches < 1);
    m_ui.nextButton->setDisabled(m_searchingTab->matches < 1);
    m_ui.filterBtn->setDisabled(m_searchingTab->matches <= 1);

    m_searchingTab->treeView->expandAll();
    m_searchingTab->treeView->resizeColumnToContents(0);
    if (m_searchingTab->treeView->columnWidth(0) < m_searchingTab->treeView->width() - 30) {
        m_searchingTab->treeView->setColumnWidth(0, m_searchingTab->treeView->width() - 30);
    }

    // Set search to done. This sorts the model and collapses all items in the view
    m_searchingTab->matchModel.setSearchState(MatchModel::SearchDone);

    // expand the "header item " to display all files and all results if configured
    expandResults();

    indicateMatch(m_searchingTab->matches > 0 ? MatchType::HasMatch : MatchType::NoMatch);

    if (popupVisible) {
        m_ui.searchCombo->lineEdit()->completer()->complete();
    }
    if (!m_searchJustOpened && m_ui.displayOptions->isEnabled()) {
        m_ui.displayOptions->setChecked(false);
    }

    m_searchJustOpened = false;
    m_searchingTab->searchStr = m_ui.searchCombo->currentText();

    m_searchingTab = nullptr;

    updateMatchMarks();
}

void KatePluginSearchView::indicateMatch(MatchType matchType)
{
    QLineEdit *const lineEdit = m_ui.searchCombo->lineEdit();
    QPalette background(lineEdit->palette());

    if (matchType == MatchType::HasMatch) {
        // Green background for line edit
        KColorScheme::adjustBackground(background, KColorScheme::PositiveBackground);
    } else if (matchType == MatchType::InvalidRegExp) {
        // Red background in case an error occured
        KColorScheme::adjustBackground(background, KColorScheme::NegativeBackground);
    } else {
        // Reset background of line edit
        background = QPalette();
    }
    // Red background for line edit
    // KColorScheme::adjustBackground(background, KColorScheme::NegativeBackground);
    // Neutral background
    // KColorScheme::adjustBackground(background, KColorScheme::NeutralBackground);

    lineEdit->setPalette(background);
}

void KatePluginSearchView::replaceSingleMatch()
{
    // Save the search text
    if (m_ui.searchCombo->findText(m_ui.searchCombo->currentText()) == -1) {
        m_ui.searchCombo->insertItem(1, m_ui.searchCombo->currentText());
        m_ui.searchCombo->setCurrentIndex(1);
    }

    // Save the replace text
    if (m_ui.replaceCombo->findText(m_ui.replaceCombo->currentText()) == -1) {
        m_ui.replaceCombo->insertItem(1, m_ui.replaceCombo->currentText());
        m_ui.replaceCombo->setCurrentIndex(1);
    }

    // Check if the cursor is at the current item if not jump there
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res) {
        return; // Security measure
    }

    QModelIndex itemIndex = res->treeView->currentIndex();
    if (!itemIndex.isValid() || !res->isMatch(itemIndex)) {
        goToNextMatch();
        // If no item was selected "Replace" is similar to just pressing "Next"
        // We do not want to replace a string we do not see with plain "Replace"
        return;
    }

    if (!m_mainWindow->activeView() || !m_mainWindow->activeView()->cursorPosition().isValid()) {
        itemSelected(itemIndex); // Correct any bad cursor positions
        return;
    }

    KTextEditor::Range matchRange = res->matchRange(itemIndex);

    if (m_mainWindow->activeView()->cursorPosition() != matchRange.start()) {
        itemSelected(itemIndex);
        return;
    }

    Q_EMIT searchBusy(true);

    KTextEditor::Document *doc = m_mainWindow->activeView()->document();

    // The document might have been edited after the search.
    // Sync the ranges before attempting the replace
    syncModelRanges(res);

    res->replaceSingleMatch(doc, itemIndex, res->regExp, m_ui.replaceCombo->currentText());

    goToNextMatch();
}

void KatePluginSearchView::replaceChecked()
{
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res) {
        qWarning() << "BUG: Results tab not found";
        return;
    }

    // Sync the current documents ranges with the model in case it has been edited
    syncModelRanges(res);

    // Clear match marks and ranges
    // we MUST do this because after we are done replacing, our current moving ranges
    // destroy the replace ranges and we don't get the highlights for replace for the
    // current open doc
    clearMarksAndRanges();

    if (m_ui.searchCombo->findText(m_ui.searchCombo->currentText()) == -1) {
        m_ui.searchCombo->insertItem(1, m_ui.searchCombo->currentText());
        m_ui.searchCombo->setCurrentIndex(1);
    }

    if (m_ui.replaceCombo->findText(m_ui.replaceCombo->currentText()) == -1) {
        m_ui.replaceCombo->insertItem(1, m_ui.replaceCombo->currentText());
        m_ui.replaceCombo->setCurrentIndex(1);
    }

    Q_EMIT searchBusy(true);

    m_ui.stopAndNext->setCurrentWidget(m_ui.stopButton);
    m_ui.displayOptions->setChecked(false);
    m_ui.displayOptions->setDisabled(true);
    m_ui.newTabButton->setDisabled(true);
    m_ui.searchCombo->setDisabled(true);
    m_ui.searchButton->setDisabled(true);
    m_ui.replaceCheckedBtn->setDisabled(true);
    m_ui.replaceButton->setDisabled(true);
    m_ui.replaceCombo->setDisabled(true);
    m_ui.searchPlaceCombo->setDisabled(true);
    m_ui.useRegExp->setDisabled(true);
    m_ui.matchCase->setDisabled(true);
    m_ui.expandResults->setDisabled(true);
    m_ui.currentFolderButton->setDisabled(true);

    res->replaceStr = m_ui.replaceCombo->currentText();

    res->matchModel.replaceChecked(res->regExp, res->replaceStr);
}

void KatePluginSearchView::replaceDone()
{
    m_ui.stopAndNext->setCurrentWidget(m_ui.nextButton);
    m_ui.replaceCombo->setDisabled(false);
    m_ui.newTabButton->setDisabled(false);
    m_ui.searchCombo->setDisabled(false);
    m_ui.searchButton->setDisabled(false);
    m_ui.replaceCheckedBtn->setDisabled(false);
    m_ui.replaceButton->setDisabled(false);
    m_ui.displayOptions->setDisabled(false);
    m_ui.searchPlaceCombo->setDisabled(false);
    m_ui.useRegExp->setDisabled(false);
    m_ui.matchCase->setDisabled(false);
    m_ui.expandResults->setDisabled(false);
    m_ui.currentFolderButton->setDisabled(false);
    updateMatchMarks();

    Q_EMIT searchBusy(false);
}

/** Remove all moving ranges and document marks belonging to Search & Replace */
void KatePluginSearchView::clearMarksAndRanges()
{
    // If we have a MovingRange we have a corresponding MatchMark
    while (!m_matchRanges.isEmpty()) {
        clearDocMarksAndRanges(m_matchRanges.first()->document());
    }
}

void KatePluginSearchView::clearDocMarksAndRanges(KTextEditor::Document *doc)
{
    if (doc) {
        const QHash<int, KTextEditor::Mark *> marks = doc->marks();
        QHashIterator<int, KTextEditor::Mark *> i(marks);
        while (i.hasNext()) {
            i.next();
            if (i.value()->type & KTextEditor::Document::markType32) {
                doc->removeMark(i.value()->line, KTextEditor::Document::markType32);
            }
        }
    }

    m_matchRanges.erase(std::remove_if(m_matchRanges.begin(),
                                       m_matchRanges.end(),
                                       [doc](KTextEditor::MovingRange *r) {
                                           if (r->document() == doc) {
                                               delete r;
                                               return true;
                                           }
                                           return false;
                                       }),
                        m_matchRanges.end());
}

void KatePluginSearchView::addRangeAndMark(KTextEditor::Document *doc,
                                           const KateSearchMatch &match,
                                           KTextEditor::Attribute::Ptr attr,
                                           const QRegularExpression &regExp)
{
    if (!doc || !match.checked) {
        return;
    }

    bool isReplaced = !match.replaceText.isEmpty();

    // Check that the match still matches
    if (!isReplaced) {
        auto regMatch = MatchModel::rangeTextMatches(doc->text(match.range), regExp);
        if (regMatch.capturedStart() != 0) {
            // qDebug() << doc->text(match.range) << "Does not match" << regExp.pattern();
            return;
        }
    } else {
        if (doc->text(match.range) != match.replaceText) {
            // qDebug() << doc->text(match.range) << "Does not match" << match.replaceText;
            return;
        }
    }

    // Highlight the match
    if (isReplaced) {
        attr->setBackground(m_replaceHighlightColor);
    }

    KTextEditor::MovingRange *mr = doc->newMovingRange(match.range);
    mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
    mr->setAttribute(attr);
    mr->setAttributeOnlyForViews(true);
    m_matchRanges.append(mr);

    // Add a match mark
    static const auto description = i18n("Search Match");
    doc->setMarkDescription(KTextEditor::Document::markType32, description);
    doc->setMarkIcon(KTextEditor::Document::markType32, QIcon());
    doc->addMark(match.range.start().line(), KTextEditor::Document::markType32);
}

void KatePluginSearchView::updateCheckState(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);

    // check tailored to the way signal is raised by the model
    // keep the check simple in case each one is one of many
    if (roles.size() == 0 || roles.size() > 1 || roles[0] != Qt::CheckStateRole) {
        return;
    }
    // more updates might follow, let's batch those
    if (!m_updateCheckedStateTimer.isActive()) {
        m_updateCheckedStateTimer.start();
    }
}

void KatePluginSearchView::updateMatchMarks()
{
    // We only keep marks & ranges for one document at a time so clear the rest
    // This will also update the model ranges corresponding to the cleared ranges.
    clearMarksAndRanges();

    if (!m_mainWindow->activeView()) {
        return;
    }

    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res || res->isEmpty()) {
        return;
    }

    // add the marks if it is not already open
    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    if (!doc) {
        return;
    }

    connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &KatePluginSearchView::clearMarksAndRanges, Qt::UniqueConnection);
    // Re-add the highlighting on document reload
    connect(doc, &KTextEditor::Document::reloaded, this, &KatePluginSearchView::updateMatchMarks, Qt::UniqueConnection);
    // Re-do highlight upon check mark update
    connect(&res->matchModel, &QAbstractItemModel::dataChanged, this, &KatePluginSearchView::updateCheckState, Qt::UniqueConnection);

    // Add match marks for all matches in the file
    const QList<KateSearchMatch> &fileMatches = res->matchModel.fileMatches(doc);
    for (const KateSearchMatch &match : fileMatches) {
        addRangeAndMark(doc, match, m_resultAttr, res->regExp);
    }
}

void KatePluginSearchView::syncModelRanges(Results *resultsTab)
{
    if (!resultsTab || resultsTab->isEmpty()) {
        return;
    }
    // NOTE: We assume there are only ranges for one document in the ranges at a time...
    resultsTab->matchModel.updateMatchRanges(m_matchRanges);
}

void KatePluginSearchView::expandResults()
{
    Results *currentTab = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!currentTab) {
        qWarning() << "Results not found";
        return;
    }

    // we expand recursively if we either are told so or we have just one toplevel match item
    auto *model = currentTab->treeView->model();
    QModelIndex rootItem = model->index(0, 0);
    if (m_ui.expandResults->isChecked() || model->rowCount(rootItem) == 1) {
        currentTab->treeView->expandAll();
    } else {
        // first collapse all and then expand the root, much faster than collapsing all children manually
        currentTab->treeView->collapseAll();
        currentTab->treeView->expand(rootItem);
    }
}

void KatePluginSearchView::itemSelected(const QModelIndex &item)
{
    Results *currentTab = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!currentTab) {
        qDebug() << "No result widget available";
        return;
    }

    // Sync the current document matches with the model before jumping
    syncModelRanges(currentTab);

    // open any children to go to the first match in the file
    QModelIndex matchItem = item;
    if (item.model() == currentTab->model()) {
        while (currentTab->model()->hasChildren(matchItem)) {
            matchItem = currentTab->model()->index(0, 0, matchItem);
        }
        currentTab->treeView->setCurrentIndex(matchItem);
    }

    // get stuff
    int toLine = matchItem.data(MatchModel::StartLineRole).toInt();
    int toColumn = matchItem.data(MatchModel::StartColumnRole).toInt();
    QUrl url = matchItem.data(MatchModel::FileUrlRole).toUrl();

    // If this url is invalid, it could be that we are searching an unsaved file
    // use doc ptr in that case.
    KTextEditor::Document *doc = nullptr;
    if (url.isValid()) {
        doc = m_kateApp->findUrl(url);
        // add the marks to the document if it is not already open
        if (!doc) {
            doc = m_kateApp->openUrl(url);
        }
    } else {
        doc = matchItem.data(MatchModel::DocumentRole).value<KTextEditor::Document *>();
        if (!doc) {
            // maybe the doc was closed
            return;
        }
    }

    if (!doc) {
        qWarning() << "Could not open" << url;
        Q_ASSERT(false); // If we get here we have a bug
        return;
    }

    // open the right view...
    m_mainWindow->activateView(doc);

    // any view active?
    if (!m_mainWindow->activeView()) {
        qDebug() << "Could not activate view for:" << url;
        Q_ASSERT(false);
        return;
    }

    // set the cursor to the correct position
    m_mainWindow->activeView()->setCursorPosition(KTextEditor::Cursor(toLine, toColumn));
    m_mainWindow->activeView()->setFocus();
}

void KatePluginSearchView::goToNextMatch()
{
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res) {
        return;
    }

    m_ui.displayOptions->setChecked(false);

    QModelIndex currentIndex = res->treeView->currentIndex();
    bool focusInView = m_mainWindow->activeView() && m_mainWindow->activeView()->hasFocus();

    if (!currentIndex.isValid() && focusInView) {
        // no item has been visited && focus is not in searchCombo (probably in the view) ->
        // jump to the closest match after current cursor position
        auto *doc = m_mainWindow->activeView()->document();

        // check if current file is in the file list
        currentIndex = res->firstFileMatch(doc);
        if (currentIndex.isValid()) {
            // We have the index of the first match in the file
            // expand the file item
            res->treeView->expand(currentIndex.parent());

            // check if we can get the next match after the
            currentIndex = res->closestMatchAfter(doc, m_mainWindow->activeView()->cursorPosition());
            if (currentIndex.isValid()) {
                itemSelected(currentIndex);
                delete m_infoMessage;
                const QString msg = i18n("Next from cursor");
                m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
                m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
                m_infoMessage->setAutoHide(2000);
                m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
                m_infoMessage->setView(m_mainWindow->activeView());
                m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
                return;
            }
        }
    }

    if (!currentIndex.isValid()) {
        currentIndex = res->firstMatch();
        if (currentIndex.isValid()) {
            itemSelected(currentIndex);
            delete m_infoMessage;
            const QString msg = i18n("Starting from first match");
            m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
            m_infoMessage->setPosition(KTextEditor::Message::TopInView);
            m_infoMessage->setAutoHide(2000);
            m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
            m_infoMessage->setView(m_mainWindow->activeView());
            m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
            return;
        }
    }
    if (!currentIndex.isValid()) {
        // no matches to activate
        return;
    }

    // we had an active item go to next
    currentIndex = res->nextMatch(currentIndex);
    itemSelected(currentIndex);
    if (currentIndex == res->firstMatch()) {
        delete m_infoMessage;
        const QString msg = i18n("Continuing from first match");
        m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
        m_infoMessage->setPosition(KTextEditor::Message::TopInView);
        m_infoMessage->setAutoHide(2000);
        m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
        m_infoMessage->setView(m_mainWindow->activeView());
        m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
    }
}

void KatePluginSearchView::goToPreviousMatch()
{
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res) {
        return;
    }

    m_ui.displayOptions->setChecked(false);

    QModelIndex currentIndex = res->treeView->currentIndex();
    bool focusInView = m_mainWindow->activeView() && m_mainWindow->activeView()->hasFocus();

    if (!currentIndex.isValid() && focusInView) {
        // no item has been visited && focus is not in the view ->
        // jump to the closest match before current cursor position
        auto *doc = m_mainWindow->activeView()->document();

        // check if current file is in the file list
        currentIndex = res->firstFileMatch(doc);
        if (currentIndex.isValid()) {
            // We have the index of the first match in the file
            // expand the file item
            res->treeView->expand(currentIndex.parent());

            // check if we can get the next match after the
            currentIndex = res->closestMatchBefore(doc, m_mainWindow->activeView()->cursorPosition());
            if (currentIndex.isValid()) {
                itemSelected(currentIndex);
                delete m_infoMessage;
                const QString msg = i18n("Next from cursor");
                m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
                m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
                m_infoMessage->setAutoHide(2000);
                m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
                m_infoMessage->setView(m_mainWindow->activeView());
                m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
                return;
            }
        }
    }

    if (!currentIndex.isValid()) {
        currentIndex = res->lastMatch();
        if (currentIndex.isValid()) {
            itemSelected(currentIndex);
            delete m_infoMessage;
            const QString msg = i18n("Starting from last match");
            m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
            m_infoMessage->setPosition(KTextEditor::Message::TopInView);
            m_infoMessage->setAutoHide(2000);
            m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
            m_infoMessage->setView(m_mainWindow->activeView());
            m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
            return;
        }
    }
    if (!currentIndex.isValid()) {
        // no matches to activate
        return;
    }

    // we had an active item go to next
    currentIndex = res->prevMatch(currentIndex);
    itemSelected(currentIndex);
    if (currentIndex == res->lastMatch()) {
        delete m_infoMessage;
        const QString msg = i18n("Continuing from last match");
        m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
        m_infoMessage->setPosition(KTextEditor::Message::TopInView);
        m_infoMessage->setAutoHide(2000);
        m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
        m_infoMessage->setView(m_mainWindow->activeView());
        m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
    }
}

void KatePluginSearchView::setRegexMode(bool enabled)
{
    m_ui.useRegExp->setChecked(enabled);
}

void KatePluginSearchView::setCaseInsensitive(bool enabled)
{
    m_ui.matchCase->setChecked(enabled);
}

void KatePluginSearchView::setExpandResults(bool enabled)
{
    m_ui.expandResults->setChecked(enabled);
}

void KatePluginSearchView::readSessionConfig(const KConfigGroup &cg)
{
    m_ui.searchCombo->clear();
    m_ui.searchCombo->addItem(QString()); // Add empty Item
    m_ui.searchCombo->addItems(cg.readEntry("Search", QStringList()));
    m_ui.replaceCombo->clear();
    m_ui.replaceCombo->addItem(QString()); // Add empty Item
    m_ui.replaceCombo->addItems(cg.readEntry("Replaces", QStringList()));
    m_ui.matchCase->setChecked(cg.readEntry("MatchCase", false));
    m_ui.useRegExp->setChecked(cg.readEntry("UseRegExp", false));
    m_ui.expandResults->setChecked(cg.readEntry("ExpandSearchResults", false));

    int searchPlaceIndex = cg.readEntry("Place", 1);
    if (searchPlaceIndex < 0) {
        searchPlaceIndex = MatchModel::Folder; // for the case we happen to read -1 as Place
    }
    if ((searchPlaceIndex >= MatchModel::Project) && (searchPlaceIndex >= m_ui.searchPlaceCombo->count())) {
        // handle the case that project mode was selected, but not yet available
        m_projectSearchPlaceIndex = searchPlaceIndex;
        searchPlaceIndex = MatchModel::Folder;
    }
    m_ui.searchPlaceCombo->setCurrentIndex(searchPlaceIndex);

    m_ui.recursiveCheckBox->setChecked(cg.readEntry("Recursive", true));
    m_ui.hiddenCheckBox->setChecked(cg.readEntry("HiddenFiles", false));
    m_ui.symLinkCheckBox->setChecked(cg.readEntry("FollowSymLink", false));
    m_ui.binaryCheckBox->setChecked(cg.readEntry("BinaryFiles", false));
    m_ui.sizeLimitSpinBox->setValue(cg.readEntry("SizeLimit", 128));
    m_ui.folderRequester->comboBox()->clear();
    m_ui.folderRequester->comboBox()->addItems(cg.readEntry("SearchDiskFiless", QStringList()));
    m_ui.folderRequester->setText(cg.readEntry("SearchDiskFiles", QString()));
    m_ui.filterCombo->clear();
    m_ui.filterCombo->addItems(cg.readEntry("Filters", QStringList()));
    m_ui.filterCombo->setCurrentIndex(cg.readEntry("CurrentFilter", -1));
    m_ui.excludeCombo->clear();
    m_ui.excludeCombo->addItems(cg.readEntry("ExcludeFilters", QStringList()));
    m_ui.excludeCombo->setCurrentIndex(cg.readEntry("CurrentExcludeFilter", -1));
    m_ui.displayOptions->setChecked(searchPlaceIndex == MatchModel::Folder);

    // Search as you type
    m_searchAsYouType.insert(MatchModel::CurrentFile, cg.readEntry("SearchAsYouTypeCurrentFile", true));
    m_searchAsYouType.insert(MatchModel::OpenFiles, cg.readEntry("SearchAsYouTypeOpenFiles", true));
    m_searchAsYouType.insert(MatchModel::Folder, cg.readEntry("SearchAsYouTypeFolder", true));
    m_searchAsYouType.insert(MatchModel::Project, cg.readEntry("SearchAsYouTypeProject", true));
    m_searchAsYouType.insert(MatchModel::AllProjects, cg.readEntry("SearchAsYouTypeAllProjects", true));
}

void KatePluginSearchView::writeSessionConfig(KConfigGroup &cg)
{
    QStringList searchHistoy;
    for (int i = 1; i < m_ui.searchCombo->count(); i++) {
        searchHistoy << m_ui.searchCombo->itemText(i);
    }
    cg.writeEntry("Search", searchHistoy);
    QStringList replaceHistoy;
    for (int i = 1; i < m_ui.replaceCombo->count(); i++) {
        replaceHistoy << m_ui.replaceCombo->itemText(i);
    }
    cg.writeEntry("Replaces", replaceHistoy);

    cg.writeEntry("MatchCase", m_ui.matchCase->isChecked());
    cg.writeEntry("UseRegExp", m_ui.useRegExp->isChecked());
    cg.writeEntry("ExpandSearchResults", m_ui.expandResults->isChecked());

    cg.writeEntry("Place", m_ui.searchPlaceCombo->currentIndex());
    cg.writeEntry("Recursive", m_ui.recursiveCheckBox->isChecked());
    cg.writeEntry("HiddenFiles", m_ui.hiddenCheckBox->isChecked());
    cg.writeEntry("FollowSymLink", m_ui.symLinkCheckBox->isChecked());
    cg.writeEntry("BinaryFiles", m_ui.binaryCheckBox->isChecked());
    cg.writeEntry("SizeLimit", m_ui.sizeLimitSpinBox->value());
    QStringList folders;
    for (int i = 0; i < qMin(m_ui.folderRequester->comboBox()->count(), 10); i++) {
        folders << m_ui.folderRequester->comboBox()->itemText(i);
    }
    cg.writeEntry("SearchDiskFiless", folders);
    cg.writeEntry("SearchDiskFiles", m_ui.folderRequester->text());
    QStringList filterItems;
    for (int i = 0; i < qMin(m_ui.filterCombo->count(), 10); i++) {
        filterItems << m_ui.filterCombo->itemText(i);
    }
    cg.writeEntry("Filters", filterItems);
    cg.writeEntry("CurrentFilter", m_ui.filterCombo->findText(m_ui.filterCombo->currentText()));

    QStringList excludeFilterItems;
    for (int i = 0; i < qMin(m_ui.excludeCombo->count(), 10); i++) {
        excludeFilterItems << m_ui.excludeCombo->itemText(i);
    }
    cg.writeEntry("ExcludeFilters", excludeFilterItems);
    cg.writeEntry("CurrentExcludeFilter", m_ui.excludeCombo->findText(m_ui.excludeCombo->currentText()));

    // Search as you type
    cg.writeEntry("SearchAsYouTypeCurrentFile", m_searchAsYouType.value(MatchModel::CurrentFile, true));
    cg.writeEntry("SearchAsYouTypeOpenFiles", m_searchAsYouType.value(MatchModel::OpenFiles, true));
    cg.writeEntry("SearchAsYouTypeFolder", m_searchAsYouType.value(MatchModel::Folder, true));
    cg.writeEntry("SearchAsYouTypeProject", m_searchAsYouType.value(MatchModel::Project, true));
    cg.writeEntry("SearchAsYouTypeAllProjects", m_searchAsYouType.value(MatchModel::AllProjects, true));
}

void KatePluginSearchView::addTab()
{
    Results *res = new Results();

    res->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    res->treeView->setRootIsDecorated(false);
    connect(res->treeView, &QTreeView::doubleClicked, this, &KatePluginSearchView::itemSelected, Qt::UniqueConnection);
    connect(res->treeView, &QTreeView::customContextMenuRequested, this, &KatePluginSearchView::customResMenuRequested, Qt::UniqueConnection);
    connect(res, &Results::requestDetachToMainWindow, this, &KatePluginSearchView::detachTabToMainWindow, Qt::UniqueConnection);
    res->matchModel.setDocumentManager(m_kateApp);
    connect(&res->matchModel, &MatchModel::replaceDone, this, &KatePluginSearchView::replaceDone);

    res->searchPlaceIndex = m_ui.searchPlaceCombo->currentIndex();
    res->useRegExp = m_ui.useRegExp->isChecked();
    res->matchCase = m_ui.matchCase->isChecked();
    m_ui.resultWidget->addWidget(res);
    m_tabBar->addTab(QString());
    m_tabBar->setCurrentIndex(m_tabBar->count() - 1);
    m_ui.stackedWidget->setCurrentIndex(0);

    // Don't show folder options widget for every new tab
    if (m_tabBar->count() == 1) {
        const bool showFolderOpts = res->searchPlaceIndex < MatchModel::Folder;
        m_ui.displayOptions->setChecked(showFolderOpts);
        res->displayFolderOptions = showFolderOpts;
    }

    res->treeView->installEventFilter(this);
}

void KatePluginSearchView::detachTabToMainWindow(Results *res)
{
    if (!res) {
        return;
    }

    int i = m_tabBar->currentIndex();
    res->setWindowIcon(QIcon::fromTheme(QStringLiteral("edit-find")));
    res->setWindowTitle(i18n("Search: %1", m_tabBar->tabText(i)));
    m_mainWindow->addWidget(res);
    res->isDetachedToMainWindow = true;
    m_tabBar->removeTab(i);
    addTab();
}

void KatePluginSearchView::tabCloseRequested(int index)
{
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->widget(index));
    if (!res) {
        qWarning() << "BUG: Result tab not found";
        return;
    }

    if (m_searchingTab == res) {
        m_searchOpenFiles.cancelSearch();
        cancelDiskFileSearch();
        m_folderFilesList.terminateSearch();
        m_searchingTab = nullptr;
    }

    res->matchModel.cancelReplace();

    if (m_ui.resultWidget->count() > 1) {
        m_tabBar->blockSignals(true);
        m_tabBar->removeTab(index);
        m_ui.resultWidget->removeWidget(res);
        m_tabBar->blockSignals(false);
        delete res;
    }

    // focus the tab after or the first one if it is the last
    if (index >= m_ui.resultWidget->count()) {
        index = m_ui.resultWidget->count() - 1;
    }
    m_tabBar->setCurrentIndex(index); // this will change also the resultWidget index
    resultTabChanged(index); // but the index might not change so make sure we update

    updateMatchMarks();
}

void KatePluginSearchView::resultTabChanged(int index)
{
    if (index < 0) {
        return;
    }

    Results *res = qobject_cast<Results *>(m_ui.resultWidget->widget(index));
    if (!res) {
        // qDebug() << "No res found";
        return;
    }

    // Synchronize the old tab's moving-ranges and matches
    if (m_currentTab) {
        syncModelRanges(m_currentTab);
    }
    m_currentTab = res;

    // Restore display folder option state
    m_ui.displayOptions->setChecked(res->displayFolderOptions);

    m_ui.searchCombo->blockSignals(true);
    m_ui.matchCase->blockSignals(true);
    m_ui.useRegExp->blockSignals(true);
    m_ui.searchPlaceCombo->blockSignals(true);

    m_ui.searchCombo->lineEdit()->setText(res->searchStr);
    m_ui.useRegExp->setChecked(res->useRegExp);
    m_ui.matchCase->setChecked(res->matchCase);
    m_ui.searchPlaceCombo->setCurrentIndex(res->searchPlaceIndex);

    m_ui.searchCombo->blockSignals(false);
    m_ui.matchCase->blockSignals(false);
    m_ui.useRegExp->blockSignals(false);
    m_ui.searchPlaceCombo->blockSignals(false);
    searchPlaceChanged();
    updateMatchMarks();
}

void KatePluginSearchView::onResize(const QSize &size)
{
    bool vertical = size.width() < size.height();

    if (!m_isVerticalLayout && vertical) {
        // Change the layout to vertical (left/right)
        m_isVerticalLayout = true;

        // Search row 1
        m_ui.gridLayout->addWidget(m_ui.searchCombo, 0, 0, 1, 5);
        // Search row 2
        m_ui.gridLayout->addWidget(m_ui.searchButton, 1, 0);
        m_ui.gridLayout->addWidget(m_ui.stopAndNext, 1, 1);
        m_ui.gridLayout->addWidget(m_ui.searchPlaceLayoutW, 1, 2, 1, 3);

        // Replace row 1
        m_ui.gridLayout->addWidget(m_ui.replaceCombo, 2, 0, 1, 5);

        // Replace row 2
        m_ui.gridLayout->addWidget(m_ui.replaceButton, 3, 0);
        m_ui.gridLayout->addWidget(m_ui.replaceCheckedBtn, 3, 1);
        m_ui.gridLayout->addWidget(m_ui.searchOptionsLayoutW, 3, 2);
        m_ui.gridLayout->addWidget(m_ui.newTabButton, 3, 3);
        m_ui.gridLayout->addWidget(m_ui.displayOptions, 3, 4);

        m_ui.gridLayout->setColumnStretch(0, 0);
        m_ui.gridLayout->setColumnStretch(2, 4);

    } else if (m_isVerticalLayout && !vertical) {
        // Change layout to horizontal (top/bottom)
        m_isVerticalLayout = false;
        // Top row
        m_ui.gridLayout->addWidget(m_ui.searchCombo, 0, 0);
        m_ui.gridLayout->addWidget(m_ui.searchButton, 0, 1);
        m_ui.gridLayout->addWidget(m_ui.stopAndNext, 0, 2);
        m_ui.gridLayout->addWidget(m_ui.searchPlaceLayoutW, 0, 3, 1, 3);

        // Second row
        m_ui.gridLayout->addWidget(m_ui.replaceCombo, 1, 0);
        m_ui.gridLayout->addWidget(m_ui.replaceButton, 1, 1);
        m_ui.gridLayout->addWidget(m_ui.replaceCheckedBtn, 1, 2);
        m_ui.gridLayout->addWidget(m_ui.searchOptionsLayoutW, 1, 3);
        m_ui.gridLayout->addWidget(m_ui.newTabButton, 1, 4);
        m_ui.gridLayout->addWidget(m_ui.displayOptions, 1, 5);

        m_ui.gridLayout->setColumnStretch(0, 4);
        m_ui.gridLayout->setColumnStretch(2, 0);
    }
}

void KatePluginSearchView::customResMenuRequested(const QPoint &pos)
{
    QPointer<Results> res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res) {
        return;
    }
    QTreeView *tree = qobject_cast<QTreeView *>(sender());
    if (tree == nullptr) {
        return;
    }
    QMenu *menu = new QMenu(tree);

    QAction *copyAll = new QAction(i18n("Copy all"), tree);
    copyAll->setShortcut(QKeySequence::Copy);
    copyAll->setShortcutVisibleInContextMenu(true);
    menu->addAction(copyAll);

    QAction *copyExpanded = new QAction(i18n("Copy expanded"), tree);
    menu->addAction(copyExpanded);

    QAction *exportMatches = new QAction(i18n("Export matches"), tree);
    if (res->useRegExp) {
        menu->addAction(exportMatches);
    }

    QAction *openAsEditorTab = new QAction(i18n("Open as Editor Tab"), tree);
    connect(openAsEditorTab, &QAction::triggered, this, [this, res] {
        if (res) {
            detachTabToMainWindow(res);
        }
    });
    menu->addAction(openAsEditorTab);

    QAction *clear = menu->addAction(i18n("Clear"));

    menu->popup(tree->viewport()->mapToGlobal(pos));

    connect(copyAll, &QAction::triggered, this, [this](bool) {
        copySearchToClipboard(All);
    });
    connect(copyExpanded, &QAction::triggered, this, [this](bool) {
        copySearchToClipboard(AllExpanded);
    });
    connect(exportMatches, &QAction::triggered, this, [this](bool) {
        showExportMatchesDialog();
    });
    connect(clear, &QAction::triggered, this, [this] {
        if (Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget())) {
            res->matchModel.clear();
        }
        clearMarksAndRanges();
    });
}

void KatePluginSearchView::copySearchToClipboard(CopyResultType copyType)
{
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res) {
        return;
    }
    if (res->model()->rowCount() == 0) {
        return;
    }

    QString clipboard;
    auto *model = res->model();

    QModelIndex rootIndex = model->index(0, 0);

    clipboard = rootIndex.data(MatchModel::PlainTextRole).toString();

    int fileCount = model->rowCount(rootIndex);
    for (int i = 0; i < fileCount; ++i) {
        QModelIndex fileIndex = model->index(i, 0, rootIndex);
        if (res->treeView->isExpanded(fileIndex) || copyType == All) {
            clipboard += QLatin1String("\n") + fileIndex.data(MatchModel::PlainTextRole).toString();
            int matchCount = model->rowCount(fileIndex);
            for (int j = 0; j < matchCount; ++j) {
                QModelIndex matchIndex = model->index(j, 0, fileIndex);
                clipboard += QLatin1String("\n") + matchIndex.data(MatchModel::PlainTextRole).toString();
            }
        }
    }
    clipboard += QLatin1String("\n");
    QApplication::clipboard()->setText(clipboard);
}

void KatePluginSearchView::showExportMatchesDialog()
{
    Results *res = qobject_cast<Results *>(m_ui.resultWidget->currentWidget());
    if (!res) {
        return;
    }
    MatchExportDialog matchExportDialog(m_mainWindow->window(), res->model(), &res->regExp);
    matchExportDialog.exec();
}

bool KatePluginSearchView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        // Ignore copy in ShortcutOverride and handle it in the KeyPress event
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->matches(QKeySequence::Copy)) {
            event->accept();
            return true;
        }
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QTreeView *treeView = qobject_cast<QTreeView *>(obj);
        if (treeView) {
            if (ke->matches(QKeySequence::Copy)) {
                copySearchToClipboard(All);
                event->accept();
                return true;
            }
            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
                if (treeView->currentIndex().isValid()) {
                    itemSelected(treeView->currentIndex());
                    event->accept();
                    return true;
                }
            }
        }
        // NOTE: Qt::Key_Escape is handled by handleEsc
    } else if (event->type() == QEvent::Resize) {
        QResizeEvent *re = static_cast<QResizeEvent *>(event);
        if (obj == m_toolView) {
            onResize(re->size());
        }
    }
    return QObject::eventFilter(obj, event);
}

void KatePluginSearchView::searchContextMenu(const QPoint &pos)
{
    QSet<QAction *> actionPointers;

    QMenu *const contextMenu = m_ui.searchCombo->lineEdit()->createStandardContextMenu();
    if (!contextMenu) {
        return;
    }

    if (m_ui.useRegExp->isChecked()) {
        QMenu *menu = contextMenu->addMenu(i18n("Add..."));
        if (!menu) {
            return;
        }

        menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));

        addRegexHelperActionsForSearch(&actionPointers, menu);
    }

    // add option to disable/enable search as you type
    QAction *a = contextMenu->addAction(QStringLiteral("search_as_you_type"));
    a->setText(i18n("Search As You Type"));
    a->setCheckable(true);
    int searchPlace = m_ui.searchPlaceCombo->currentIndex();
    bool enabled = m_searchAsYouType.value(static_cast<MatchModel::SearchPlaces>(searchPlace), true);
    a->setChecked(enabled);
    connect(a, &QAction::triggered, this, [this](bool checked) {
        int searchPlace = m_ui.searchPlaceCombo->currentIndex();
        m_searchAsYouType[static_cast<MatchModel::SearchPlaces>(searchPlace)] = checked;
    });

    // Show menu and act
    QAction *const result = contextMenu->exec(m_ui.searchCombo->mapToGlobal(pos));
    regexHelperActOnAction(result, actionPointers, m_ui.searchCombo->lineEdit());
}

void KatePluginSearchView::replaceContextMenu(const QPoint &pos)
{
    QMenu *const contextMenu = m_ui.replaceCombo->lineEdit()->createStandardContextMenu();
    if (!contextMenu) {
        return;
    }

    QMenu *menu = contextMenu->addMenu(i18n("Add..."));
    if (!menu) {
        return;
    }
    menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));

    QSet<QAction *> actionPointers;
    addSpecialCharsHelperActionsForReplace(&actionPointers, menu);

    if (m_ui.useRegExp->isChecked()) {
        addRegexHelperActionsForReplace(&actionPointers, menu);
    }

    // Show menu and act
    QAction *const result = contextMenu->exec(m_ui.replaceCombo->mapToGlobal(pos));
    regexHelperActOnAction(result, actionPointers, m_ui.replaceCombo->lineEdit());
}

void KatePluginSearchView::slotPluginViewCreated(const QString &name, QObject *pluginView)
{
    // add view
    if (pluginView && name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = pluginView;
        slotProjectFileNameChanged();
        connect(pluginView, SIGNAL(projectFileNameChanged()), this, SLOT(slotProjectFileNameChanged()));
    }
}

void KatePluginSearchView::slotPluginViewDeleted(const QString &name, QObject *)
{
    // remove view
    if (name == QLatin1String("kateprojectplugin")) {
        m_projectPluginView = nullptr;
        slotProjectFileNameChanged();
    }
}

void KatePluginSearchView::slotProjectFileNameChanged()
{
    // query new project file name
    QString projectName;
    if (m_projectPluginView) {
        projectName = m_projectPluginView->property("projectName").toString();
    }

    // have project, enable gui for it
    if (!projectName.isEmpty()) {
        if (m_ui.searchPlaceCombo->count() <= MatchModel::Project) {
            // add "in Project"
            m_ui.searchPlaceCombo->addItem(QIcon::fromTheme(QStringLiteral("project-open")), i18n("In Current Project"));
            // add "in Open Projects"
            m_ui.searchPlaceCombo->addItem(QIcon::fromTheme(QStringLiteral("project-open")), i18n("In All Open Projects"));
            if (m_projectSearchPlaceIndex >= MatchModel::Project) {
                // switch to search "in (all) Project"
                setSearchPlace(m_projectSearchPlaceIndex);
                m_projectSearchPlaceIndex = 0;
            }
        }
    }

    // else: disable gui for it
    else {
        if (m_ui.searchPlaceCombo->count() >= MatchModel::Project) {
            // switch to search "in Open files", if "in Project" is active
            int searchPlaceIndex = m_ui.searchPlaceCombo->currentIndex();
            if (searchPlaceIndex >= MatchModel::Project) {
                m_projectSearchPlaceIndex = searchPlaceIndex;
                setSearchPlace(MatchModel::OpenFiles);
            }

            // remove "in Project" and "in all projects"
            while (m_ui.searchPlaceCombo->count() > MatchModel::Project) {
                m_ui.searchPlaceCombo->removeItem(m_ui.searchPlaceCombo->count() - 1);
            }
        }
    }
}

#include "SearchPlugin.moc"
#include "moc_SearchPlugin.cpp"

// kate: space-indent on; indent-width 4; replace-tabs on;
