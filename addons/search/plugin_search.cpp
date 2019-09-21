/*   Kate search plugin
 *
 * Copyright (C) 2011-2013 by Kåre Särs <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file called COPYING; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "plugin_search.h"

#include "htmldelegate.h"

#include <ktexteditor/application.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/configinterface.h>

#include "kacceleratormanager.h"
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kurlcompletion.h>
#include <klineedit.h>
#include <kcolorscheme.h>

#include <KXMLGUIFactory>
#include <KConfigGroup>

#include <QKeyEvent>
#include <QClipboard>
#include <QMenu>
#include <QMetaObject>
#include <QTextDocument>
#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QComboBox>
#include <QCompleter>

static QUrl localFileDirUp(const QUrl &url)
{
    if (!url.isLocalFile())
        return url;

    // else go up
    return QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).dir().absolutePath());
}

static QAction *menuEntry(QMenu *menu, const QString &before, const QString &after, const QString &desc, QString menuBefore = QString(), QString menuAfter = QString());

/**
 * When the action is triggered the cursor will be placed between @p before and @p after.
 */
static QAction *menuEntry(QMenu *menu, const QString &before, const QString &after, const QString &desc, QString menuBefore, QString menuAfter)
{
    if (menuBefore.isEmpty())
        menuBefore = before;
    if (menuAfter.isEmpty())
        menuAfter = after;

    QAction *const action = menu->addAction(menuBefore + menuAfter + QLatin1Char('\t') + desc);
    if (!action)
        return nullptr;

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
    actionPointers << menuEntry(menu, QStringLiteral("{"), QStringLiteral(",}"), i18n("<a> through <b> occurrences"), QStringLiteral("{a"), QStringLiteral(",b}"));
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
static void addRegexHelperActionsForReplace(QSet<QAction *> *actionList, QMenu *menu)
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
static void regexHelperActOnAction(QAction *resultAction, const QSet<QAction *> &actionList, QLineEdit *lineEdit)
{
    if (resultAction && actionList.contains(resultAction)) {
        const int cursorPos = lineEdit->cursorPosition();
        QStringList beforeAfter = resultAction->data().toString().split(QLatin1Char(' '));
        if (beforeAfter.size() != 2)
            return;
        lineEdit->insert(beforeAfter[0] + beforeAfter[1]);
        lineEdit->setCursorPosition(cursorPos + beforeAfter[0].count());
        lineEdit->setFocus();
    }
}

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget *parent)
        : QTreeWidgetItem(parent)
    {
    }
    TreeWidgetItem(QTreeWidget *parent, const QStringList &list)
        : QTreeWidgetItem(parent, list)
    {
    }
    TreeWidgetItem(QTreeWidgetItem *parent, const QStringList &list)
        : QTreeWidgetItem(parent, list)
    {
    }

private:
    bool operator<(const QTreeWidgetItem &other) const override
    {
        if (childCount() == 0) {
            int line = data(0, ReplaceMatches::StartLineRole).toInt();
            int column = data(0, ReplaceMatches::StartColumnRole).toInt();
            int oLine = other.data(0, ReplaceMatches::StartLineRole).toInt();
            int oColumn = other.data(0, ReplaceMatches::StartColumnRole).toInt();
            if (line < oLine) {
                return true;
            }
            if ((line == oLine) && (column < oColumn)) {
                return true;
            }
            return false;
        }
        int sepCount = data(0, ReplaceMatches::FileUrlRole).toString().count(QDir::separator());
        int oSepCount = other.data(0, ReplaceMatches::FileUrlRole).toString().count(QDir::separator());
        if (sepCount < oSepCount)
            return true;
        if (sepCount > oSepCount)
            return false;
        return data(0, ReplaceMatches::FileUrlRole).toString().toLower() < other.data(0, ReplaceMatches::FileUrlRole).toString().toLower();
    }
};

Results::Results(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    tree->setItemDelegate(new SPHtmlDelegate(tree));
}

K_PLUGIN_FACTORY_WITH_JSON(KatePluginSearchFactory, "katesearch.json", registerPlugin<KatePluginSearch>();)

KatePluginSearch::KatePluginSearch(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
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
    connect(m_searchCommand, SIGNAL(newTab()), view, SLOT(addTab()));
    return view;
}

bool ContainerWidget::focusNextPrevChild(bool next)
{
    QWidget *fw = focusWidget();
    bool found = false;
    emit nextFocus(fw, &found, next);

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

    // we use the object names here because there can be multiple replaceButtons (on multiple result tabs)
    if (next) {
        if (currentWidget->objectName() == QLatin1String("tree") || currentWidget == m_ui.binaryCheckBox) {
            m_ui.newTabButton->setFocus();
            *found = true;
            return;
        }
        if (currentWidget == m_ui.displayOptions) {
            if (m_ui.displayOptions->isChecked()) {
                m_ui.folderRequester->setFocus();
                *found = true;
                return;
            } else {
                Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
                if (!res) {
                    return;
                }
                res->tree->setFocus();
                *found = true;
                return;
            }
        }
    } else {
        if (currentWidget == m_ui.newTabButton) {
            if (m_ui.displayOptions->isChecked()) {
                m_ui.binaryCheckBox->setFocus();
            } else {
                Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
                if (!res) {
                    return;
                }
                res->tree->setFocus();
            }
            *found = true;
            return;
        } else {
            if (currentWidget->objectName() == QLatin1String("tree")) {
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
    , m_curResults(nullptr)
    , m_searchJustOpened(false)
    , m_switchToProjectModeWhenAvailable(false)
    , m_searchDiskFilesDone(true)
    , m_searchOpenFilesDone(true)
    , m_isSearchAsYouType(false)
    , m_isLeftRight(false)
    , m_projectPluginView(nullptr)
    , m_mainWindow(mainWin)
{
    KXMLGUIClient::setComponentName(QStringLiteral("katesearch"), i18n("Kate Search & Replace"));
    setXMLFile(QStringLiteral("ui.rc"));

    m_toolView = mainWin->createToolView(plugin, QStringLiteral("kate_plugin_katesearch"), KTextEditor::MainWindow::Bottom, QIcon::fromTheme(QStringLiteral("edit-find")), i18n("Search and Replace"));

    ContainerWidget *container = new ContainerWidget(m_toolView);
    m_ui.setupUi(container);
    container->setFocusProxy(m_ui.searchCombo);
    connect(container, &ContainerWidget::nextFocus, this, &KatePluginSearchView::nextFocus);

    QAction *a = actionCollection()->addAction(QStringLiteral("search_in_files"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_F));
    a->setText(i18n("Search in Files"));
    connect(a, &QAction::triggered, this, &KatePluginSearchView::openSearchView);

    a = actionCollection()->addAction(QStringLiteral("search_in_files_new_tab"));
    a->setText(i18n("Search in Files (in new tab)"));
    // first add tab, then open search view, since open search view switches to show the search options
    connect(a, &QAction::triggered, this, &KatePluginSearchView::addTab);
    connect(a, &QAction::triggered, this, &KatePluginSearchView::openSearchView);

    a = actionCollection()->addAction(QStringLiteral("go_to_next_match"));
    a->setText(i18n("Go to Next Match"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::Key_F6));
    connect(a, &QAction::triggered, this, &KatePluginSearchView::goToNextMatch);

    a = actionCollection()->addAction(QStringLiteral("go_to_prev_match"));
    a->setText(i18n("Go to Previous Match"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::SHIFT + Qt::Key_F6));
    connect(a, &QAction::triggered, this, &KatePluginSearchView::goToPreviousMatch);

    m_ui.resultTabWidget->tabBar()->setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
    KAcceleratorManager::setNoAccel(m_ui.resultTabWidget);

    // Gnome does not seem to have all icons we want, so we use fall-back icons for those that are missing.
    QIcon dispOptIcon = QIcon::fromTheme(QStringLiteral("games-config-options"), QIcon::fromTheme(QStringLiteral("preferences-system")));
    QIcon matchCaseIcon = QIcon::fromTheme(QStringLiteral("format-text-superscript"), QIcon::fromTheme(QStringLiteral("format-text-bold")));
    QIcon useRegExpIcon = QIcon::fromTheme(QStringLiteral("code-context"), QIcon::fromTheme(QStringLiteral("edit-find-replace")));
    QIcon expandResultsIcon = QIcon::fromTheme(QStringLiteral("view-list-tree"), QIcon::fromTheme(QStringLiteral("format-indent-more")));

    m_ui.displayOptions->setIcon(dispOptIcon);
    m_ui.searchButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-find")));
    m_ui.nextButton->setIcon(QIcon::fromTheme(QStringLiteral("go-down-search")));
    m_ui.stopButton->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    m_ui.matchCase->setIcon(matchCaseIcon);
    m_ui.useRegExp->setIcon(useRegExpIcon);
    m_ui.expandResults->setIcon(expandResultsIcon);
    m_ui.searchPlaceCombo->setItemIcon(CurrentFile, QIcon::fromTheme(QStringLiteral("text-plain")));
    m_ui.searchPlaceCombo->setItemIcon(OpenFiles, QIcon::fromTheme(QStringLiteral("text-plain")));
    m_ui.searchPlaceCombo->setItemIcon(Folder, QIcon::fromTheme(QStringLiteral("folder")));
    m_ui.folderUpButton->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    m_ui.currentFolderButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_ui.newTabButton->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));

    m_ui.filterCombo->setToolTip(i18n("Comma separated list of file types to search in. Example: \"*.cpp,*.h\"\n"));
    m_ui.excludeCombo->setToolTip(i18n("Comma separated list of files and directories to exclude from the search. Example: \"build*\""));

    // the order here is important to get the tabBar hidden for only one tab
    addTab();
    m_ui.resultTabWidget->tabBar()->hide();

    // get url-requester's combo box and sanely initialize
    KComboBox *cmbUrl = m_ui.folderRequester->comboBox();
    cmbUrl->setDuplicatesEnabled(false);
    cmbUrl->setEditable(true);
    m_ui.folderRequester->setMode(KFile::Directory | KFile::LocalOnly);
    KUrlCompletion *cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
    cmbUrl->setCompletionObject(cmpl);
    cmbUrl->setAutoDeleteCompletionObject(true);

    connect(m_ui.newTabButton, &QToolButton::clicked, this, &KatePluginSearchView::addTab);
    connect(m_ui.resultTabWidget, &QTabWidget::tabCloseRequested, this, &KatePluginSearchView::tabCloseRequested);
    connect(m_ui.resultTabWidget, &QTabWidget::currentChanged, this, &KatePluginSearchView::resultTabChanged);

    connect(m_ui.folderUpButton, &QToolButton::clicked, this, &KatePluginSearchView::navigateFolderUp);
    connect(m_ui.currentFolderButton, &QToolButton::clicked, this, &KatePluginSearchView::setCurrentFolder);
    connect(m_ui.expandResults, &QToolButton::clicked, this, &KatePluginSearchView::expandResults);

    connect(m_ui.searchCombo, &QComboBox::editTextChanged, &m_changeTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_ui.matchCase, &QToolButton::toggled, &m_changeTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_ui.matchCase, &QToolButton::toggled, this, [=] {
        Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
        if (res) {
            res->matchCase = m_ui.matchCase->isChecked();
        }
    });

    connect(m_ui.searchCombo->lineEdit(), &QLineEdit::returnPressed, this, &KatePluginSearchView::startSearch);
    // connecting to returnPressed() of the folderRequester doesn't work, I haven't found out why yet. But connecting to the linedit works:
    connect(m_ui.folderRequester->comboBox()->lineEdit(), &QLineEdit::returnPressed, this, &KatePluginSearchView::startSearch);
    connect(m_ui.filterCombo, static_cast<void (KComboBox::*)()>(&KComboBox::returnPressed), this, &KatePluginSearchView::startSearch);
    connect(m_ui.excludeCombo, static_cast<void (KComboBox::*)()>(&KComboBox::returnPressed), this, &KatePluginSearchView::startSearch);
    connect(m_ui.searchButton, &QPushButton::clicked, this, &KatePluginSearchView::startSearch);

    connect(m_ui.displayOptions, &QToolButton::toggled, this, &KatePluginSearchView::toggleOptions);
    connect(m_ui.searchPlaceCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KatePluginSearchView::searchPlaceChanged);
    connect(m_ui.searchPlaceCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_ui.searchPlaceCombo->currentIndex() == Folder) {
            m_ui.displayOptions->setChecked(true);
        }
    });

    connect(m_ui.stopButton, &QPushButton::clicked, &m_searchOpenFiles, &SearchOpenFiles::cancelSearch);
    connect(m_ui.stopButton, &QPushButton::clicked, &m_searchDiskFiles, &SearchDiskFiles::cancelSearch);
    connect(m_ui.stopButton, &QPushButton::clicked, &m_folderFilesList, &FolderFilesList::cancelSearch);
    connect(m_ui.stopButton, &QPushButton::clicked, &m_replacer, &ReplaceMatches::cancelReplace);

    connect(m_ui.nextButton, &QToolButton::clicked, this, &KatePluginSearchView::goToNextMatch);

    connect(m_ui.replaceButton, &QPushButton::clicked, this, &KatePluginSearchView::replaceSingleMatch);
    connect(m_ui.replaceCheckedBtn, &QPushButton::clicked, this, &KatePluginSearchView::replaceChecked);
    connect(m_ui.replaceCombo->lineEdit(), &QLineEdit::returnPressed, this, &KatePluginSearchView::replaceChecked);

    m_ui.displayOptions->setChecked(true);

    connect(&m_searchOpenFiles, &SearchOpenFiles::matchFound, this, &KatePluginSearchView::matchFound);
    connect(&m_searchOpenFiles, &SearchOpenFiles::searchDone, this, &KatePluginSearchView::searchDone);
    connect(&m_searchOpenFiles, static_cast<void (SearchOpenFiles::*)(const QString &)>(&SearchOpenFiles::searching), this, &KatePluginSearchView::searching);

    connect(&m_folderFilesList, &FolderFilesList::finished, this, &KatePluginSearchView::folderFileListChanged);
    connect(&m_folderFilesList, &FolderFilesList::searching, this, &KatePluginSearchView::searching);

    connect(&m_searchDiskFiles, &SearchDiskFiles::matchFound, this, &KatePluginSearchView::matchFound);
    connect(&m_searchDiskFiles, &SearchDiskFiles::searchDone, this, &KatePluginSearchView::searchDone);
    connect(&m_searchDiskFiles, static_cast<void (SearchDiskFiles::*)(const QString &)>(&SearchDiskFiles::searching), this, &KatePluginSearchView::searching);

    connect(m_kateApp, &KTextEditor::Application::documentWillBeDeleted, &m_searchOpenFiles, &SearchOpenFiles::cancelSearch);

    connect(m_kateApp, &KTextEditor::Application::documentWillBeDeleted, &m_replacer, &ReplaceMatches::cancelReplace);

    connect(m_kateApp, &KTextEditor::Application::documentWillBeDeleted, this, &KatePluginSearchView::clearDocMarks);

    connect(&m_replacer, &ReplaceMatches::replaceStatus, this, &KatePluginSearchView::replaceStatus);

    // Hook into line edit context menus
    m_ui.searchCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.searchCombo, &QComboBox::customContextMenuRequested, this, &KatePluginSearchView::searchContextMenu);
    m_ui.searchCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    m_ui.searchCombo->completer()->setCaseSensitivity(Qt::CaseSensitive);
    m_ui.searchCombo->setInsertPolicy(QComboBox::NoInsert);
    m_ui.searchCombo->lineEdit()->setClearButtonEnabled(true);
    m_ui.searchCombo->setMaxCount(25);
    QAction *searchComboActionForInsertRegexButton = m_ui.searchCombo->lineEdit()->addAction(QIcon::fromTheme(QStringLiteral("code-context"), QIcon::fromTheme(QStringLiteral("edit-find-replace"))), QLineEdit::TrailingPosition);
    connect(searchComboActionForInsertRegexButton, &QAction::triggered, this, [this]() {
        QMenu menu;
        QSet<QAction *> actionList;
        addRegexHelperActionsForSearch(&actionList, &menu);
        auto &&action = menu.exec(QCursor::pos());
        regexHelperActOnAction(action, actionList, m_ui.searchCombo->lineEdit());
    });

    m_ui.replaceCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.replaceCombo, &QComboBox::customContextMenuRequested, this, &KatePluginSearchView::replaceContextMenu);
    m_ui.replaceCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    m_ui.replaceCombo->completer()->setCaseSensitivity(Qt::CaseSensitive);
    m_ui.replaceCombo->setInsertPolicy(QComboBox::NoInsert);
    m_ui.replaceCombo->lineEdit()->setClearButtonEnabled(true);
    m_ui.replaceCombo->setMaxCount(25);
    QAction *replaceComboActionForInsertRegexButton = m_ui.replaceCombo->lineEdit()->addAction(QIcon::fromTheme(QStringLiteral("code-context")), QLineEdit::TrailingPosition);
    connect(replaceComboActionForInsertRegexButton, &QAction::triggered, this, [this]() {
        QMenu menu;
        QSet<QAction *> actionList;
        addRegexHelperActionsForReplace(&actionList, &menu);
        auto &&action = menu.exec(QCursor::pos());
        regexHelperActOnAction(action, actionList, m_ui.replaceCombo->lineEdit());
    });
    QAction *replaceComboActionForInsertSpecialButton = m_ui.replaceCombo->lineEdit()->addAction(QIcon::fromTheme(QStringLiteral("insert-text")), QLineEdit::TrailingPosition);
    connect(replaceComboActionForInsertSpecialButton, &QAction::triggered, this, [this]() {
        QMenu menu;
        QSet<QAction *> actionList;
        addSpecialCharsHelperActionsForReplace(&actionList, &menu);
        auto &&action = menu.exec(QCursor::pos());
        regexHelperActOnAction(action, actionList, m_ui.replaceCombo->lineEdit());
    });

    connect(m_ui.useRegExp, &QToolButton::toggled, &m_changeTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    auto onRegexToggleChanged = [=] {
        Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
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

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KatePluginSearchView::docViewChanged);

    // Connect signals from project plugin to our slots
    m_projectPluginView = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
    slotPluginViewCreated(QStringLiteral("kateprojectplugin"), m_projectPluginView);

    m_replacer.setDocumentManager(m_kateApp);
    connect(&m_replacer, &ReplaceMatches::replaceDone, this, &KatePluginSearchView::replaceDone);

    searchPlaceChanged();

    m_toolView->installEventFilter(this);

    m_mainWindow->guiFactory()->addClient(this);

    m_updateSumaryTimer.setInterval(1);
    m_updateSumaryTimer.setSingleShot(true);
    connect(&m_updateSumaryTimer, &QTimer::timeout, this, &KatePluginSearchView::updateResultsRootItem);
}

KatePluginSearchView::~KatePluginSearchView()
{
    clearMarks();

    m_mainWindow->guiFactory()->removeClient(this);
    delete m_toolView;
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
    if (m_ui.searchPlaceCombo->currentIndex() == Folder) {
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
    if (!m_mainWindow)
        return;

    QKeyEvent *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
        static ulong lastTimeStamp;
        if (lastTimeStamp == k->timestamp()) {
            // Same as previous... This looks like a bug somewhere...
            return;
        }
        lastTimeStamp = k->timestamp();
        if (!m_matchRanges.isEmpty()) {
            clearMarks();
        } else if (m_toolView->isVisible()) {
            m_mainWindow->hideToolView(m_toolView);
        }

        // Remove check marks
        Results *curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
        if (!curResults) {
            qWarning() << "This is a bug";
            return;
        }
        QTreeWidgetItemIterator it(curResults->tree);
        while (*it) {
            (*it)->setCheckState(0, Qt::Unchecked);
            ++it;
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
}

void KatePluginSearchView::setSearchPlace(int place)
{
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

    QStringList tmpTypes = types.split(QLatin1Char(','));
    QVector<QRegExp> typeList(tmpTypes.size());
    for (int i = 0; i < tmpTypes.size(); i++) {
        QRegExp rx(tmpTypes[i].trimmed());
        rx.setPatternSyntax(QRegExp::Wildcard);
        typeList << rx;
    }

    QStringList tmpExcludes = excludes.split(QLatin1Char(','));
    QVector<QRegExp> excludeList(tmpExcludes.size());
    for (int i = 0; i < tmpExcludes.size(); i++) {
        QRegExp rx(tmpExcludes[i].trimmed());
        rx.setPatternSyntax(QRegExp::Wildcard);
        excludeList << rx;
    }

    QStringList filteredFiles;
    foreach (QString fileName, files) {
        bool isInSubDir = fileName.startsWith(m_resultBaseDir);
        QString nameToCheck = fileName;
        if (isInSubDir) {
            nameToCheck = fileName.mid(m_resultBaseDir.size());
        }

        bool skip = false;
        for (const auto &regex : qAsConst(excludeList)) {
            if (regex.exactMatch(nameToCheck)) {
                skip = true;
                break;
            }
        }
        if (skip) {
            continue;
        }

        for (const auto &regex : qAsConst(typeList)) {
            if (regex.exactMatch(nameToCheck)) {
                filteredFiles << fileName;
                break;
            }
        }
    }
    return filteredFiles;
}

void KatePluginSearchView::folderFileListChanged()
{
    m_searchDiskFilesDone = false;
    m_searchOpenFilesDone = false;

    if (!m_curResults) {
        qWarning() << "This is a bug";
        m_searchDiskFilesDone = true;
        m_searchOpenFilesDone = true;
        searchDone();
        return;
    }
    QStringList fileList = m_folderFilesList.fileList();

    QList<KTextEditor::Document *> openList;
    for (int i = 0; i < m_kateApp->documents().size(); i++) {
        int index = fileList.indexOf(m_kateApp->documents()[i]->url().toLocalFile());
        if (index != -1) {
            openList << m_kateApp->documents()[i];
            fileList.removeAt(index);
        }
    }

    // search order is important: Open files starts immediately and should finish
    // earliest after first event loop.
    // The DiskFile might finish immediately
    if (!openList.empty()) {
        m_searchOpenFiles.startSearch(openList, m_curResults->regExp);
    } else {
        m_searchOpenFilesDone = true;
    }

    m_searchDiskFiles.startSearch(fileList, m_curResults->regExp);
}

void KatePluginSearchView::searchPlaceChanged()
{
    int searchPlace = m_ui.searchPlaceCombo->currentIndex();
    const bool inFolder = (searchPlace == Folder);

    m_ui.filterCombo->setEnabled(searchPlace >= Folder);
    m_ui.excludeCombo->setEnabled(searchPlace >= Folder);
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

    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (res) {
        res->searchPlaceIndex = searchPlace;
    }
}

void KatePluginSearchView::addHeaderItem()
{
    QTreeWidgetItem *item = new QTreeWidgetItem(m_curResults->tree, QStringList());
    item->setCheckState(0, Qt::Checked);
    item->setFlags(item->flags() | Qt::ItemIsAutoTristate);
    m_curResults->tree->expandItem(item);
}

QTreeWidgetItem *KatePluginSearchView::rootFileItem(const QString &url, const QString &fName)
{
    if (!m_curResults) {
        return nullptr;
    }

    QUrl fullUrl = QUrl::fromUserInput(url);
    QString path = fullUrl.isLocalFile() ? localFileDirUp(fullUrl).path() : fullUrl.url();
    if (!path.isEmpty() && !path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    path.remove(m_resultBaseDir);
    QString name = fullUrl.fileName();
    if (url.isEmpty()) {
        name = fName;
    }

    // make sure we have a root item
    if (m_curResults->tree->topLevelItemCount() == 0) {
        addHeaderItem();
    }
    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);

    if (m_isSearchAsYouType) {
        return root;
    }

    for (int i = 0; i < root->childCount(); i++) {
        // qDebug() << root->child(i)->data(0, ReplaceMatches::FileNameRole).toString() << fName;
        if ((root->child(i)->data(0, ReplaceMatches::FileUrlRole).toString() == url) && (root->child(i)->data(0, ReplaceMatches::FileNameRole).toString() == fName)) {
            int matches = root->child(i)->data(0, ReplaceMatches::StartLineRole).toInt() + 1;
            QString tmpUrl = QStringLiteral("%1<b>%2</b>: <b>%3</b>").arg(path, name).arg(matches);
            root->child(i)->setData(0, Qt::DisplayRole, tmpUrl);
            root->child(i)->setData(0, ReplaceMatches::StartLineRole, matches);
            return root->child(i);
        }
    }

    // file item not found create a new one
    QString tmpUrl = QStringLiteral("%1<b>%2</b>: <b>%3</b>").arg(path, name).arg(1);

    TreeWidgetItem *item = new TreeWidgetItem(root, QStringList(tmpUrl));
    item->setData(0, ReplaceMatches::FileUrlRole, url);
    item->setData(0, ReplaceMatches::FileNameRole, fName);
    item->setData(0, ReplaceMatches::StartLineRole, 1);
    item->setCheckState(0, Qt::Checked);
    item->setFlags(item->flags() | Qt::ItemIsAutoTristate);
    return item;
}

void KatePluginSearchView::addMatchMark(KTextEditor::Document *doc, QTreeWidgetItem *item)
{
    if (!doc || !item) {
        return;
    }

    KTextEditor::View *activeView = m_mainWindow->activeView();
    KTextEditor::MovingInterface *miface = qobject_cast<KTextEditor::MovingInterface *>(doc);
    KTextEditor::ConfigInterface *ciface = qobject_cast<KTextEditor::ConfigInterface *>(activeView);
    KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());

    int line = item->data(0, ReplaceMatches::StartLineRole).toInt();
    int column = item->data(0, ReplaceMatches::StartColumnRole).toInt();
    int endLine = item->data(0, ReplaceMatches::EndLineRole).toInt();
    int endColumn = item->data(0, ReplaceMatches::EndColumnRole).toInt();
    bool isReplaced = item->data(0, ReplaceMatches::ReplacedRole).toBool();

    if (isReplaced) {
        QColor replaceColor(Qt::green);
        if (ciface)
            replaceColor = ciface->configValue(QStringLiteral("replace-highlight-color")).value<QColor>();
        attr->setBackground(replaceColor);

        if (activeView) {
            attr->setForeground(activeView->defaultStyleAttribute(KTextEditor::dsNormal)->foreground().color());
        }
    } else {
        QColor searchColor(Qt::yellow);
        if (ciface)
            searchColor = ciface->configValue(QStringLiteral("search-highlight-color")).value<QColor>();
        attr->setBackground(searchColor);

        if (activeView) {
            attr->setForeground(activeView->defaultStyleAttribute(KTextEditor::dsNormal)->foreground().color());
        }
    }

    KTextEditor::Range range(line, column, endLine, endColumn);

    // Check that the match still matches
    if (m_curResults) {
        if (!isReplaced) {
            // special handling for "(?=\\n)" in multi-line search
            QRegularExpression tmpReg = m_curResults->regExp;
            if (m_curResults->regExp.pattern().endsWith(QLatin1String("(?=\\n)"))) {
                QString newPatern = tmpReg.pattern();
                newPatern.replace(QStringLiteral("(?=\\n)"), QStringLiteral("$"));
                tmpReg.setPattern(newPatern);
            }

            // Check that the match still matches ;)
            if (tmpReg.match(doc->text(range)).capturedStart() != 0) {
                qDebug() << doc->text(range) << "Does not match" << m_curResults->regExp.pattern();
                return;
            }
        } else {
            if (doc->text(range) != item->data(0, ReplaceMatches::ReplacedTextRole).toString()) {
                qDebug() << doc->text(range) << "Does not match" << item->data(0, ReplaceMatches::ReplacedTextRole).toString();
                return;
            }
        }
    }

    // Highlight the match
    KTextEditor::MovingRange *mr = miface->newMovingRange(range);
    mr->setAttribute(attr);
    mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
    mr->setAttributeOnlyForViews(true);
    m_matchRanges.append(mr);

    // Add a match mark
    KTextEditor::MarkInterface *iface = qobject_cast<KTextEditor::MarkInterface *>(doc);
    if (!iface)
        return;
    iface->setMarkDescription(KTextEditor::MarkInterface::markType32, i18n("SearchHighLight"));
    iface->setMarkPixmap(KTextEditor::MarkInterface::markType32, QIcon().pixmap(0, 0));
    iface->addMark(line, KTextEditor::MarkInterface::markType32);

    connect(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document *)), this, SLOT(clearMarks()), Qt::UniqueConnection);
}

static const int contextLen = 70;

void KatePluginSearchView::matchFound(const QString &url, const QString &fName, const QString &lineContent, int matchLen, int startLine, int startColumn, int endLine, int endColumn)
{
    if (!m_curResults) {
        return;
    }
    int preLen = contextLen;
    int preStart = startColumn - preLen;
    if (preStart < 0) {
        preLen += preStart;
        preStart = 0;
    }
    QString pre;
    if (preLen == contextLen) {
        pre = QStringLiteral("...");
    }
    pre += lineContent.mid(preStart, preLen).toHtmlEscaped();
    QString match = lineContent.mid(startColumn, matchLen).toHtmlEscaped();
    match.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    QString post = lineContent.mid(startColumn + matchLen, contextLen);
    if (post.size() >= contextLen) {
        post += QStringLiteral("...");
    }
    post = post.toHtmlEscaped();
    QStringList row;
    row << i18n("Line: <b>%1</b> Column: <b>%2</b>: %3", startLine + 1, startColumn + 1, pre + QStringLiteral("<b>") + match + QStringLiteral("</b>") + post);

    TreeWidgetItem *item = new TreeWidgetItem(rootFileItem(url, fName), row);
    item->setData(0, ReplaceMatches::FileUrlRole, url);
    item->setData(0, Qt::ToolTipRole, url);
    item->setData(0, ReplaceMatches::FileNameRole, fName);
    item->setData(0, ReplaceMatches::StartLineRole, startLine);
    item->setData(0, ReplaceMatches::StartColumnRole, startColumn);
    item->setData(0, ReplaceMatches::MatchLenRole, matchLen);
    item->setData(0, ReplaceMatches::PreMatchRole, pre);
    item->setData(0, ReplaceMatches::MatchRole, match);
    item->setData(0, ReplaceMatches::PostMatchRole, post);
    item->setData(0, ReplaceMatches::EndLineRole, endLine);
    item->setData(0, ReplaceMatches::EndColumnRole, endColumn);
    item->setCheckState(0, Qt::Checked);

    m_curResults->matches++;
}

void KatePluginSearchView::clearMarks()
{
    foreach (KTextEditor::Document *doc, m_kateApp->documents()) {
        clearDocMarks(doc);
    }
    qDeleteAll(m_matchRanges);
    m_matchRanges.clear();
}

void KatePluginSearchView::clearDocMarks(KTextEditor::Document *doc)
{
    KTextEditor::MarkInterface *iface;
    iface = qobject_cast<KTextEditor::MarkInterface *>(doc);
    if (iface) {
        const QHash<int, KTextEditor::Mark *> marks = iface->marks();
        QHashIterator<int, KTextEditor::Mark *> i(marks);
        while (i.hasNext()) {
            i.next();
            if (i.value()->type & KTextEditor::MarkInterface::markType32) {
                iface->removeMark(i.value()->line, KTextEditor::MarkInterface::markType32);
            }
        }
    }

    int i = 0;
    while (i < m_matchRanges.size()) {
        if (m_matchRanges.at(i)->document() == doc) {
            delete m_matchRanges.at(i);
            m_matchRanges.removeAt(i);
        } else {
            i++;
        }
    }

    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "This is a bug";
        return;
    }
}

void KatePluginSearchView::startSearch()
{
    m_changeTimer.stop();                       // make sure not to start a "while you type" search now
    m_mainWindow->showToolView(m_toolView);     // in case we are invoked from the command interface
    m_switchToProjectModeWhenAvailable = false; // now that we started, don't switch back automatically

    if (m_ui.searchCombo->currentText().isEmpty()) {
        // return pressed in the folder combo or filter combo
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
    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "This is a bug";
        return;
    }

    QRegularExpression::PatternOptions patternOptions = (m_ui.matchCase->isChecked() ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
    QString pattern = (m_ui.useRegExp->isChecked() ? currentSearchText : QRegularExpression::escape(currentSearchText));
    QRegularExpression reg(pattern, patternOptions);

    if (!reg.isValid()) {
        // qDebug() << "invalid regexp";
        indicateMatch(false);
        return;
    }

    m_curResults->regExp = reg;
    m_curResults->useRegExp = m_ui.useRegExp->isChecked();
    m_curResults->matchCase = m_ui.matchCase->isChecked();
    m_curResults->searchPlaceIndex = m_ui.searchPlaceCombo->currentIndex();

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

    clearMarks();
    m_curResults->tree->clear();
    m_curResults->tree->setCurrentItem(nullptr);
    m_curResults->matches = 0;
    disconnect(m_curResults->tree, &QTreeWidget::itemChanged, &m_updateSumaryTimer, nullptr);

    m_ui.resultTabWidget->setTabText(m_ui.resultTabWidget->currentIndex(), m_ui.searchCombo->currentText());

    m_toolView->setCursor(Qt::WaitCursor);
    m_searchDiskFilesDone = false;
    m_searchOpenFilesDone = false;

    const bool inCurrentProject = m_ui.searchPlaceCombo->currentIndex() == Project;
    const bool inAllOpenProjects = m_ui.searchPlaceCombo->currentIndex() == AllProjects;

    if (m_ui.searchPlaceCombo->currentIndex() == CurrentFile) {
        m_searchDiskFilesDone = true;
        m_resultBaseDir.clear();
        QList<KTextEditor::Document *> documents;
        documents << m_mainWindow->activeView()->document();
        addHeaderItem();
        m_searchOpenFiles.startSearch(documents, reg);
    } else if (m_ui.searchPlaceCombo->currentIndex() == OpenFiles) {
        m_searchDiskFilesDone = true;
        m_resultBaseDir.clear();
        const QList<KTextEditor::Document *> documents = m_kateApp->documents();
        addHeaderItem();
        m_searchOpenFiles.startSearch(documents, reg);
    } else if (m_ui.searchPlaceCombo->currentIndex() == Folder) {
        m_resultBaseDir = m_ui.folderRequester->url().path();
        if (!m_resultBaseDir.isEmpty() && !m_resultBaseDir.endsWith(QLatin1Char('/')))
            m_resultBaseDir += QLatin1Char('/');
        addHeaderItem();
        m_folderFilesList.generateList(m_ui.folderRequester->text(),
                                       m_ui.recursiveCheckBox->isChecked(),
                                       m_ui.hiddenCheckBox->isChecked(),
                                       m_ui.symLinkCheckBox->isChecked(),
                                       m_ui.binaryCheckBox->isChecked(),
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
            } else {
                m_resultBaseDir = m_projectPluginView->property("allProjectsCommonBaseDir").toString();
            }

            if (!m_resultBaseDir.endsWith(QLatin1Char('/')))
                m_resultBaseDir += QLatin1Char('/');

            QStringList projectFiles;
            if (inCurrentProject) {
                projectFiles = m_projectPluginView->property("projectFiles").toStringList();
            } else {
                projectFiles = m_projectPluginView->property("allProjectsFiles").toStringList();
            }

            files = filterFiles(projectFiles);
        }
        addHeaderItem();

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
            m_searchOpenFiles.startSearch(openList, m_curResults->regExp);
        } else {
            m_searchOpenFilesDone = true;
        }
        m_searchDiskFiles.startSearch(files, reg);
    } else {
        Q_ASSERT_X(false, "KatePluginSearchView::startSearch", "case not handled");
    }
}

void KatePluginSearchView::startSearchWhileTyping()
{
    if (!m_searchDiskFilesDone || !m_searchOpenFilesDone) {
        return;
    }

    m_isSearchAsYouType = true;

    QString currentSearchText = m_ui.searchCombo->currentText();

    m_ui.searchButton->setDisabled(currentSearchText.isEmpty());

    // Do not clear the search results if you press up by mistake
    if (currentSearchText.isEmpty())
        return;

    if (!m_mainWindow->activeView())
        return;

    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    if (!doc)
        return;

    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "This is a bug";
        return;
    }

    // check if we typed something or just changed combobox index
    // changing index should not trigger a search-as-you-type
    if (m_ui.searchCombo->currentIndex() > 0 && currentSearchText == m_ui.searchCombo->itemText(m_ui.searchCombo->currentIndex())) {
        return;
    }

    // Now we should have a true typed text change

    QRegularExpression::PatternOptions patternOptions = (m_ui.matchCase->isChecked() ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
    QString pattern = (m_ui.useRegExp->isChecked() ? currentSearchText : QRegularExpression::escape(currentSearchText));
    QRegularExpression reg(pattern, patternOptions);
    if (!reg.isValid()) {
        // qDebug() << "invalid regexp";
        indicateMatch(false);
        return;
    }

    disconnect(m_curResults->tree, &QTreeWidget::itemChanged, &m_updateSumaryTimer, nullptr);

    m_curResults->regExp = reg;
    m_curResults->useRegExp = m_ui.useRegExp->isChecked();

    m_ui.replaceCheckedBtn->setDisabled(true);
    m_ui.replaceButton->setDisabled(true);
    m_ui.nextButton->setDisabled(true);

    int cursorPosition = m_ui.searchCombo->lineEdit()->cursorPosition();
    bool hasSelected = m_ui.searchCombo->lineEdit()->hasSelectedText();
    m_ui.searchCombo->blockSignals(true);
    m_ui.searchCombo->setItemText(0, currentSearchText);
    m_ui.searchCombo->setCurrentIndex(0);
    m_ui.searchCombo->lineEdit()->setCursorPosition(cursorPosition);
    if (hasSelected) {
        // This restores the select all from invoking openSearchView
        // This selects too much if we have a partial selection and toggle match-case/regexp
        m_ui.searchCombo->lineEdit()->selectAll();
    }
    m_ui.searchCombo->blockSignals(false);

    // Prepare for the new search content
    clearMarks();
    m_resultBaseDir.clear();
    m_curResults->tree->clear();
    m_curResults->tree->setCurrentItem(nullptr);
    m_curResults->matches = 0;

    // Add the search-as-you-type header item
    TreeWidgetItem *item = new TreeWidgetItem(m_curResults->tree, QStringList());
    item->setData(0, ReplaceMatches::FileUrlRole, doc->url().toString());
    item->setData(0, ReplaceMatches::FileNameRole, doc->documentName());
    item->setData(0, ReplaceMatches::StartLineRole, 0);
    item->setCheckState(0, Qt::Checked);
    item->setFlags(item->flags() | Qt::ItemIsAutoTristate);

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
}

void KatePluginSearchView::searchDone()
{
    m_changeTimer.stop(); // avoid "while you type" search directly after

    if (sender() == &m_searchDiskFiles) {
        m_searchDiskFilesDone = true;
    }
    if (sender() == &m_searchOpenFiles) {
        m_searchOpenFilesDone = true;
    }

    if (!m_searchDiskFilesDone || !m_searchOpenFilesDone) {
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
    m_ui.currentFolderButton->setDisabled(false);

    if (!m_curResults) {
        return;
    }

    m_ui.replaceCheckedBtn->setDisabled(m_curResults->matches < 1);
    m_ui.replaceButton->setDisabled(m_curResults->matches < 1);
    m_ui.nextButton->setDisabled(m_curResults->matches < 1);

    m_curResults->tree->sortItems(0, Qt::AscendingOrder);

    m_curResults->tree->expandAll();
    m_curResults->tree->resizeColumnToContents(0);
    if (m_curResults->tree->columnWidth(0) < m_curResults->tree->width() - 30) {
        m_curResults->tree->setColumnWidth(0, m_curResults->tree->width() - 30);
    }

    // expand the "header item " to display all files and all results if configured
    expandResults();

    updateResultsRootItem();
    connect(m_curResults->tree, &QTreeWidget::itemChanged, &m_updateSumaryTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    indicateMatch(m_curResults->matches > 0);
    m_curResults = nullptr;
    m_toolView->unsetCursor();

    if (fw == m_ui.stopButton) {
        m_ui.searchCombo->setFocus();
    }

    m_searchJustOpened = false;
}

void KatePluginSearchView::searchWhileTypingDone()
{
    if (!m_curResults) {
        return;
    }

    bool popupVisible = m_ui.searchCombo->lineEdit()->completer()->popup()->isVisible();

    m_ui.replaceCheckedBtn->setDisabled(m_curResults->matches < 1);
    m_ui.replaceButton->setDisabled(m_curResults->matches < 1);
    m_ui.nextButton->setDisabled(m_curResults->matches < 1);

    m_curResults->tree->expandAll();
    m_curResults->tree->resizeColumnToContents(0);
    if (m_curResults->tree->columnWidth(0) < m_curResults->tree->width() - 30) {
        m_curResults->tree->setColumnWidth(0, m_curResults->tree->width() - 30);
    }

    QWidget *focusObject = nullptr;
    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);
    if (root) {
        QTreeWidgetItem *child = root->child(0);
        if (!m_searchJustOpened) {
            focusObject = qobject_cast<QWidget *>(QGuiApplication::focusObject());
        }
        indicateMatch(child);

        updateResultsRootItem();
        connect(m_curResults->tree, &QTreeWidget::itemChanged, &m_updateSumaryTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    }

    m_curResults = nullptr;

    if (focusObject) {
        focusObject->setFocus();
    }
    if (popupVisible) {
        m_ui.searchCombo->lineEdit()->completer()->complete();
    }
    m_searchJustOpened = false;
}

void KatePluginSearchView::searching(const QString &file)
{
    if (!m_curResults) {
        return;
    }

    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);
    if (root) {
        if (file.size() > 70) {
            root->setData(0, Qt::DisplayRole, i18n("<b>Searching: ...%1</b>", file.right(70)));
        } else {
            root->setData(0, Qt::DisplayRole, i18n("<b>Searching: %1</b>", file));
        }
    }
}

void KatePluginSearchView::indicateMatch(bool hasMatch)
{
    QLineEdit *const lineEdit = m_ui.searchCombo->lineEdit();
    QPalette background(lineEdit->palette());

    if (hasMatch) {
        // Green background for line edit
        KColorScheme::adjustBackground(background, KColorScheme::PositiveBackground);
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
    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        return; // Security measure
    }
    QTreeWidgetItem *item = res->tree->currentItem();
    if (!item || !item->parent()) {
        // Nothing was selected
        goToNextMatch();
        return;
    }

    if (!m_mainWindow->activeView() || !m_mainWindow->activeView()->cursorPosition().isValid()) {
        itemSelected(item); // Correct any bad cursor positions
        return;
    }

    int cursorLine = m_mainWindow->activeView()->cursorPosition().line();
    int cursorColumn = m_mainWindow->activeView()->cursorPosition().column();

    int startLine = item->data(0, ReplaceMatches::StartLineRole).toInt();
    int startColumn = item->data(0, ReplaceMatches::StartColumnRole).toInt();

    if ((cursorLine != startLine) || (cursorColumn != startColumn)) {
        itemSelected(item);
        return;
    }

    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    // Find the corresponding range
    int i;
    for (i = 0; i < m_matchRanges.size(); i++) {
        if (m_matchRanges[i]->document() != doc)
            continue;
        if (m_matchRanges[i]->start().line() != startLine)
            continue;
        if (m_matchRanges[i]->start().column() != startColumn)
            continue;
        break;
    }

    if (i >= m_matchRanges.size()) {
        goToNextMatch();
        return;
    }

    m_replacer.replaceSingleMatch(doc, item, res->regExp, m_ui.replaceCombo->currentText());

    goToNextMatch();
}

void KatePluginSearchView::replaceChecked()
{
    if (m_ui.searchCombo->findText(m_ui.searchCombo->currentText()) == -1) {
        m_ui.searchCombo->insertItem(1, m_ui.searchCombo->currentText());
        m_ui.searchCombo->setCurrentIndex(1);
    }

    if (m_ui.replaceCombo->findText(m_ui.replaceCombo->currentText()) == -1) {
        m_ui.replaceCombo->insertItem(1, m_ui.replaceCombo->currentText());
        m_ui.replaceCombo->setCurrentIndex(1);
    }

    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "Results not found";
        return;
    }

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

    m_curResults->replaceStr = m_ui.replaceCombo->currentText();

    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);

    if (root) {
        m_curResults->treeRootText = root->data(0, Qt::DisplayRole).toString();
    }
    m_replacer.replaceChecked(m_curResults->tree, m_curResults->regExp, m_curResults->replaceStr);
}

void KatePluginSearchView::replaceStatus(const QUrl &url, int replacedInFile, int matchesInFile)
{
    if (!m_curResults) {
        qDebug() << "m_curResults == nullptr";
        return;
    }
    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);
    if (root) {
        QString file = url.toString(QUrl::PreferLocalFile);
        if (file.size() > 70) {
            root->setData(0, Qt::DisplayRole, i18n("<b>Processed %1 of %2 matches in: ...%3</b>", replacedInFile, matchesInFile, file.right(70)));
        } else {
            root->setData(0, Qt::DisplayRole, i18n("<b>Processed %1 of %2 matches in: %3</b>", replacedInFile, matchesInFile, file));
        }
    }
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

    if (!m_curResults) {
        qDebug() << "m_curResults == nullptr";
        return;
    }
    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);
    if (root) {
        root->setData(0, Qt::DisplayRole, m_curResults->treeRootText);
    }
}

void KatePluginSearchView::docViewChanged()
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        qDebug() << "No res";
        return;
    }

    m_curResults = res;

    // add the marks if it is not already open
    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    if (doc && res->tree->topLevelItemCount() > 0) {
        // There is always one root item with match count
        // and X children with files or matches in case of search while typing
        QTreeWidgetItem *rootItem = res->tree->topLevelItem(0);
        QTreeWidgetItem *fileItem = nullptr;
        for (int i = 0; i < rootItem->childCount(); i++) {
            QString url = rootItem->child(i)->data(0, ReplaceMatches::FileUrlRole).toString();
            QString fName = rootItem->child(i)->data(0, ReplaceMatches::FileNameRole).toString();
            if (url == doc->url().toString() && fName == doc->documentName()) {
                fileItem = rootItem->child(i);
                break;
            }
        }
        if (fileItem) {
            clearDocMarks(doc);

            if (m_isSearchAsYouType) {
                fileItem = fileItem->parent();
            }

            for (int i = 0; i < fileItem->childCount(); i++) {
                if (fileItem->child(i)->checkState(0) == Qt::Unchecked) {
                    continue;
                }
                addMatchMark(doc, fileItem->child(i));
            }
        }
        // Re-add the highlighting on document reload
        connect(doc, &KTextEditor::Document::reloaded, this, &KatePluginSearchView::docViewChanged, Qt::UniqueConnection);
    }
}

void KatePluginSearchView::expandResults()
{
    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "Results not found";
        return;
    }

    if (m_ui.expandResults->isChecked()) {
        m_curResults->tree->expandAll();
    } else {
        QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);
        m_curResults->tree->expandItem(root);
        if (root && (root->childCount() > 1)) {
            for (int i = 0; i < root->childCount(); i++) {
                m_curResults->tree->collapseItem(root->child(i));
            }
        }
    }
}

void KatePluginSearchView::updateResultsRootItem()
{
    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        return;
    }

    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);

    if (!root) {
        // nothing to update
        return;
    }
    int checkedItemCount = 0;
    if (m_curResults->matches > 0) {
        for (QTreeWidgetItemIterator it(m_curResults->tree, QTreeWidgetItemIterator::Checked | QTreeWidgetItemIterator::NoChildren); *it; ++it) {
            checkedItemCount++;
        }
    }

    QString checkedStr = i18np("One checked", "%1 checked", checkedItemCount);

    int searchPlace = m_ui.searchPlaceCombo->currentIndex();
    if (m_isSearchAsYouType) {
        searchPlace = CurrentFile;
    }

    switch (searchPlace) {
        case CurrentFile:
            root->setData(0, Qt::DisplayRole, i18np("<b><i>One match (%2) found in file</i></b>", "<b><i>%1 matches (%2) found in file</i></b>", m_curResults->matches, checkedStr));
            break;
        case OpenFiles:
            root->setData(0, Qt::DisplayRole, i18np("<b><i>One match (%2) found in open files</i></b>", "<b><i>%1 matches (%2) found in open files</i></b>", m_curResults->matches, checkedStr));
            break;
        case Folder:
            root->setData(0, Qt::DisplayRole, i18np("<b><i>One match (%3) found in folder %2</i></b>", "<b><i>%1 matches (%3) found in folder %2</i></b>", m_curResults->matches, m_resultBaseDir, checkedStr));
            break;
        case Project: {
            QString projectName;
            if (m_projectPluginView) {
                projectName = m_projectPluginView->property("projectName").toString();
            }
            root->setData(0, Qt::DisplayRole, i18np("<b><i>One match (%4) found in project %2 (%3)</i></b>", "<b><i>%1 matches (%4) found in project %2 (%3)</i></b>", m_curResults->matches, projectName, m_resultBaseDir, checkedStr));
            break;
        }
        case AllProjects: // "in Open Projects"
            root->setData(
                0,
                Qt::DisplayRole,
                i18np("<b><i>One match (%3) found in all open projects (common parent: %2)</i></b>", "<b><i>%1 matches (%3) found in all open projects (common parent: %2)</i></b>", m_curResults->matches, m_resultBaseDir, checkedStr));
            break;
    }

    docViewChanged();
}

void KatePluginSearchView::itemSelected(QTreeWidgetItem *item)
{
    if (!item)
        return;

    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        return;
    }

    while (item->data(0, ReplaceMatches::StartColumnRole).toString().isEmpty()) {
        item->treeWidget()->expandItem(item);
        item = item->child(0);
        if (!item)
            return;
    }
    item->treeWidget()->setCurrentItem(item);

    // get stuff
    int toLine = item->data(0, ReplaceMatches::StartLineRole).toInt();
    int toColumn = item->data(0, ReplaceMatches::StartColumnRole).toInt();

    KTextEditor::Document *doc;
    QString url = item->data(0, ReplaceMatches::FileUrlRole).toString();
    if (!url.isEmpty()) {
        doc = m_kateApp->findUrl(QUrl::fromUserInput(url));
    } else {
        doc = m_replacer.findNamed(item->data(0, ReplaceMatches::FileNameRole).toString());
    }

    // add the marks to the document if it is not already open
    if (!doc) {
        doc = m_kateApp->openUrl(QUrl::fromUserInput(url));
    }
    if (!doc)
        return;

    // open the right view...
    m_mainWindow->activateView(doc);

    // any view active?
    if (!m_mainWindow->activeView()) {
        return;
    }

    // set the cursor to the correct position
    m_mainWindow->activeView()->setCursorPosition(KTextEditor::Cursor(toLine, toColumn));
    m_mainWindow->activeView()->setFocus();
}

void KatePluginSearchView::goToNextMatch()
{
    bool wrapFromFirst = false;
    bool startFromFirst = false;
    bool startFromCursor = false;
    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        return;
    }
    QTreeWidgetItem *curr = res->tree->currentItem();

    bool focusInView = m_mainWindow->activeView() && m_mainWindow->activeView()->hasFocus();

    if (!curr && focusInView) {
        // no item has been visited && focus is not in searchCombo (probably in the view) ->
        // jump to the closest match after current cursor position

        // check if current file is in the file list
        curr = res->tree->topLevelItem(0);
        while (curr && curr->data(0, ReplaceMatches::FileUrlRole).toString() != m_mainWindow->activeView()->document()->url().toString()) {
            curr = res->tree->itemBelow(curr);
        }
        // now we are either in this file or !curr
        if (curr) {
            QTreeWidgetItem *fileBefore = curr;
            res->tree->expandItem(curr);

            int lineNr = 0;
            int columnNr = 0;
            if (m_mainWindow->activeView()->cursorPosition().isValid()) {
                lineNr = m_mainWindow->activeView()->cursorPosition().line();
                columnNr = m_mainWindow->activeView()->cursorPosition().column();
            }

            if (!curr->data(0, ReplaceMatches::StartColumnRole).isValid()) {
                curr = res->tree->itemBelow(curr);
            };

            while (curr && curr->data(0, ReplaceMatches::StartLineRole).toInt() <= lineNr && curr->data(0, ReplaceMatches::FileUrlRole).toString() == m_mainWindow->activeView()->document()->url().toString()) {
                if (curr->data(0, ReplaceMatches::StartLineRole).toInt() == lineNr && curr->data(0, ReplaceMatches::StartColumnRole).toInt() >= columnNr - curr->data(0, ReplaceMatches::MatchLenRole).toInt()) {
                    break;
                }
                fileBefore = curr;
                curr = res->tree->itemBelow(curr);
            }
            curr = fileBefore;
            startFromCursor = true;
        }
    }
    if (!curr) {
        curr = res->tree->topLevelItem(0);
        startFromFirst = true;
    }
    if (!curr)
        return;

    if (!curr->data(0, ReplaceMatches::StartColumnRole).toString().isEmpty()) {
        curr = res->tree->itemBelow(curr);
        if (!curr) {
            wrapFromFirst = true;
            curr = res->tree->topLevelItem(0);
        }
    }

    itemSelected(curr);

    if (startFromFirst) {
        delete m_infoMessage;
        const QString msg = i18n("Starting from first match");
        m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
        m_infoMessage->setPosition(KTextEditor::Message::TopInView);
        m_infoMessage->setAutoHide(2000);
        m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
        m_infoMessage->setView(m_mainWindow->activeView());
        m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
    } else if (startFromCursor) {
        delete m_infoMessage;
        const QString msg = i18n("Next from cursor");
        m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
        m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
        m_infoMessage->setAutoHide(2000);
        m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
        m_infoMessage->setView(m_mainWindow->activeView());
        m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
    } else if (wrapFromFirst) {
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
    bool fromLast = false;
    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        return;
    }
    if (res->tree->topLevelItemCount() == 0) {
        return;
    }
    QTreeWidgetItem *curr = res->tree->currentItem();

    if (!curr) {
        // no item has been visited -> jump to the closest match before current cursor position
        // check if current file is in the file
        curr = res->tree->topLevelItem(0);
        while (curr && curr->data(0, ReplaceMatches::FileUrlRole).toString() != m_mainWindow->activeView()->document()->url().toString()) {
            curr = res->tree->itemBelow(curr);
        }
        // now we are either in this file or !curr
        if (curr) {
            res->tree->expandItem(curr);

            int lineNr = 0;
            int columnNr = 0;
            if (m_mainWindow->activeView()->cursorPosition().isValid()) {
                lineNr = m_mainWindow->activeView()->cursorPosition().line();
                columnNr = m_mainWindow->activeView()->cursorPosition().column() - 1;
            }

            if (!curr->data(0, ReplaceMatches::StartColumnRole).isValid()) {
                curr = res->tree->itemBelow(curr);
            };

            while (curr && curr->data(0, ReplaceMatches::StartLineRole).toInt() <= lineNr && curr->data(0, ReplaceMatches::FileUrlRole).toString() == m_mainWindow->activeView()->document()->url().toString()) {
                if (curr->data(0, ReplaceMatches::StartLineRole).toInt() == lineNr && curr->data(0, ReplaceMatches::StartColumnRole).toInt() > columnNr) {
                    break;
                }
                curr = res->tree->itemBelow(curr);
            }
        }
    }

    QTreeWidgetItem *startChild = curr;

    // go to the item above. (curr == null is not a problem)
    curr = res->tree->itemAbove(curr);

    // expand the items above if needed
    if (curr && curr->data(0, ReplaceMatches::StartColumnRole).toString().isEmpty()) {
        res->tree->expandItem(curr); // probably this file item
        curr = res->tree->itemAbove(curr);
        if (curr && curr->data(0, ReplaceMatches::StartColumnRole).toString().isEmpty()) {
            res->tree->expandItem(curr); // probably file above if this is reached
        }
        curr = res->tree->itemAbove(startChild);
    }

    // skip file name items and the root item
    while (curr && curr->data(0, ReplaceMatches::StartColumnRole).toString().isEmpty()) {
        curr = res->tree->itemAbove(curr);
    }

    if (!curr) {
        // select the last child of the last next-to-top-level item
        QTreeWidgetItem *root = res->tree->topLevelItem(0);

        // select the last "root item"
        if (!root || (root->childCount() < 1))
            return;
        root = root->child(root->childCount() - 1);

        // select the last match of the "root item"
        if (!root || (root->childCount() < 1))
            return;
        curr = root->child(root->childCount() - 1);

        fromLast = true;
    }

    itemSelected(curr);
    if (fromLast) {
        delete m_infoMessage;
        const QString msg = i18n("Continuing from last match");
        m_infoMessage = new KTextEditor::Message(msg, KTextEditor::Message::Information);
        m_infoMessage->setPosition(KTextEditor::Message::BottomInView);
        m_infoMessage->setAutoHide(2000);
        m_infoMessage->setAutoHideMode(KTextEditor::Message::Immediate);
        m_infoMessage->setView(m_mainWindow->activeView());
        m_mainWindow->activeView()->document()->postMessage(m_infoMessage);
    }
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
        searchPlaceIndex = Folder; // for the case we happen to read -1 as Place
    }
    if ((searchPlaceIndex == Project) && (searchPlaceIndex >= m_ui.searchPlaceCombo->count())) {
        // handle the case that project mode was selected, but not yet available
        m_switchToProjectModeWhenAvailable = true;
        searchPlaceIndex = Folder;
    }
    m_ui.searchPlaceCombo->setCurrentIndex(searchPlaceIndex);

    m_ui.recursiveCheckBox->setChecked(cg.readEntry("Recursive", true));
    m_ui.hiddenCheckBox->setChecked(cg.readEntry("HiddenFiles", false));
    m_ui.symLinkCheckBox->setChecked(cg.readEntry("FollowSymLink", false));
    m_ui.binaryCheckBox->setChecked(cg.readEntry("BinaryFiles", false));
    m_ui.folderRequester->comboBox()->clear();
    m_ui.folderRequester->comboBox()->addItems(cg.readEntry("SearchDiskFiless", QStringList()));
    m_ui.folderRequester->setText(cg.readEntry("SearchDiskFiles", QString()));
    m_ui.filterCombo->clear();
    m_ui.filterCombo->addItems(cg.readEntry("Filters", QStringList()));
    m_ui.filterCombo->setCurrentIndex(cg.readEntry("CurrentFilter", -1));
    m_ui.excludeCombo->clear();
    m_ui.excludeCombo->addItems(cg.readEntry("ExcludeFilters", QStringList()));
    m_ui.excludeCombo->setCurrentIndex(cg.readEntry("CurrentExcludeFilter", -1));
    m_ui.displayOptions->setChecked(searchPlaceIndex == Folder);
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
}

void KatePluginSearchView::addTab()
{
    if ((sender() != m_ui.newTabButton) && (m_ui.resultTabWidget->count() > 0) && m_ui.resultTabWidget->tabText(m_ui.resultTabWidget->currentIndex()).isEmpty()) {
        return;
    }

    Results *res = new Results();

    res->tree->setRootIsDecorated(false);

    connect(res->tree, &QTreeWidget::itemDoubleClicked, this, &KatePluginSearchView::itemSelected, Qt::UniqueConnection);

    res->searchPlaceIndex = m_ui.searchPlaceCombo->currentIndex();
    res->useRegExp = m_ui.useRegExp->isChecked();
    res->matchCase = m_ui.matchCase->isChecked();
    m_ui.resultTabWidget->addTab(res, QString());
    m_ui.resultTabWidget->setCurrentIndex(m_ui.resultTabWidget->count() - 1);
    m_ui.stackedWidget->setCurrentIndex(0);
    m_ui.resultTabWidget->tabBar()->show();
    m_ui.displayOptions->setChecked(false);

    res->tree->installEventFilter(this);
}

void KatePluginSearchView::tabCloseRequested(int index)
{
    Results *tmp = qobject_cast<Results *>(m_ui.resultTabWidget->widget(index));
    if (m_curResults == tmp) {
        m_searchOpenFiles.cancelSearch();
        m_searchDiskFiles.cancelSearch();
    }
    if (m_ui.resultTabWidget->count() > 1) {
        delete tmp; // remove the tab
        m_curResults = nullptr;
    }
    if (m_ui.resultTabWidget->count() == 1) {
        m_ui.resultTabWidget->tabBar()->hide();
    }
}

void KatePluginSearchView::resultTabChanged(int index)
{
    if (index < 0) {
        return;
    }

    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->widget(index));
    if (!res) {
        qDebug() << "No res found";
        return;
    }

    m_ui.searchCombo->blockSignals(true);
    m_ui.matchCase->blockSignals(true);
    m_ui.useRegExp->blockSignals(true);
    m_ui.searchPlaceCombo->blockSignals(true);
    m_ui.searchCombo->lineEdit()->setText(m_ui.resultTabWidget->tabText(index));
    m_ui.useRegExp->setChecked(res->useRegExp);
    m_ui.matchCase->setChecked(res->matchCase);
    m_ui.searchPlaceCombo->setCurrentIndex(res->searchPlaceIndex);
    m_ui.searchCombo->blockSignals(false);
    m_ui.matchCase->blockSignals(false);
    m_ui.useRegExp->blockSignals(false);
    m_ui.searchPlaceCombo->blockSignals(false);
    searchPlaceChanged();
}

void KatePluginSearchView::onResize(const QSize &size)
{
    bool vertical = size.width() < size.height();

    if (!m_isLeftRight && vertical) {
        m_isLeftRight = true;

        m_ui.gridLayout->addWidget(m_ui.searchCombo, 0, 1, 1, 8);
        m_ui.gridLayout->addWidget(m_ui.findLabel, 0, 0);
        m_ui.gridLayout->addWidget(m_ui.searchButton, 1, 0, 1, 2);
        m_ui.gridLayout->addWidget(m_ui.stopAndNext, 1, 2);
        m_ui.gridLayout->addWidget(m_ui.searchPlaceCombo, 1, 3, 1, 3);
        m_ui.gridLayout->addWidget(m_ui.displayOptions, 1, 6);
        m_ui.gridLayout->addWidget(m_ui.matchCase, 1, 7);
        m_ui.gridLayout->addWidget(m_ui.useRegExp, 1, 8);

        m_ui.gridLayout->addWidget(m_ui.replaceCombo, 2, 1, 1, 8);
        m_ui.gridLayout->addWidget(m_ui.replaceLabel, 2, 0);
        m_ui.gridLayout->addWidget(m_ui.replaceButton, 3, 0, 1, 2);
        m_ui.gridLayout->addWidget(m_ui.replaceCheckedBtn, 3, 2);
        m_ui.gridLayout->addWidget(m_ui.expandResults, 3, 7);
        m_ui.gridLayout->addWidget(m_ui.newTabButton, 3, 8);

        m_ui.gridLayout->setColumnStretch(4, 2);
        m_ui.gridLayout->setColumnStretch(2, 0);
    } else if (m_isLeftRight && !vertical) {
        m_isLeftRight = false;
        m_ui.gridLayout->addWidget(m_ui.searchCombo, 0, 2);
        m_ui.gridLayout->addWidget(m_ui.findLabel, 0, 1);
        m_ui.gridLayout->addWidget(m_ui.searchButton, 0, 3);
        m_ui.gridLayout->addWidget(m_ui.stopAndNext, 0, 4);
        m_ui.gridLayout->addWidget(m_ui.searchPlaceCombo, 0, 5, 1, 4);
        m_ui.gridLayout->addWidget(m_ui.matchCase, 1, 5);
        m_ui.gridLayout->addWidget(m_ui.useRegExp, 1, 6);

        m_ui.gridLayout->addWidget(m_ui.replaceCombo, 1, 2);
        m_ui.gridLayout->addWidget(m_ui.replaceLabel, 1, 1);
        m_ui.gridLayout->addWidget(m_ui.replaceButton, 1, 3);
        m_ui.gridLayout->addWidget(m_ui.replaceCheckedBtn, 1, 4);
        m_ui.gridLayout->addWidget(m_ui.expandResults, 1, 8);
        m_ui.gridLayout->addWidget(m_ui.newTabButton, 0, 0);
        m_ui.gridLayout->addWidget(m_ui.displayOptions, 1, 0);

        m_ui.gridLayout->setColumnStretch(4, 0);
        m_ui.gridLayout->setColumnStretch(2, 2);

        m_ui.findLabel->setAlignment(Qt::AlignRight);
        m_ui.replaceLabel->setAlignment(Qt::AlignRight);
    }
}

bool KatePluginSearchView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        QTreeWidget *tree = qobject_cast<QTreeWidget *>(obj);
        if (tree) {
            if (ke->matches(QKeySequence::Copy)) {
                // user pressed ctrl+c -> copy full URL to the clipboard
                QVariant variant = tree->currentItem()->data(0, ReplaceMatches::FileUrlRole);
                QApplication::clipboard()->setText(variant.toString());
                event->accept();
                return true;
            }
            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
                if (tree->currentItem()) {
                    itemSelected(tree->currentItem());
                    event->accept();
                    return true;
                }
            }
        }
        // NOTE: Qt::Key_Escape is handled by handleEsc
    }
    if (event->type() == QEvent::Resize) {
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
    if (!contextMenu)
        return;

    if (m_ui.useRegExp->isChecked()) {
        QMenu *menu = contextMenu->addMenu(i18n("Add..."));
        if (!menu)
            return;

        menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));

        addRegexHelperActionsForSearch(&actionPointers, menu);
    }

    // Show menu and act
    QAction *const result = contextMenu->exec(m_ui.searchCombo->mapToGlobal(pos));
    regexHelperActOnAction(result, actionPointers, m_ui.searchCombo->lineEdit());
}

void KatePluginSearchView::replaceContextMenu(const QPoint &pos)
{
    QMenu *const contextMenu = m_ui.replaceCombo->lineEdit()->createStandardContextMenu();
    if (!contextMenu)
        return;

    QMenu *menu = contextMenu->addMenu(i18n("Add..."));
    if (!menu)
        return;
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
    QString projectFileName;
    if (m_projectPluginView) {
        projectFileName = m_projectPluginView->property("projectFileName").toString();
    }

    // have project, enable gui for it
    if (!projectFileName.isEmpty()) {
        if (m_ui.searchPlaceCombo->count() <= Project) {
            // add "in Project"
            m_ui.searchPlaceCombo->addItem(QIcon::fromTheme(QStringLiteral("project-open")), i18n("In Current Project"));
            if (m_switchToProjectModeWhenAvailable) {
                // switch to search "in Project"
                m_switchToProjectModeWhenAvailable = false;
                setSearchPlace(Project);
            }

            // add "in Open Projects"
            m_ui.searchPlaceCombo->addItem(QIcon::fromTheme(QStringLiteral("project-open")), i18n("In All Open Projects"));
        }
    }

    // else: disable gui for it
    else {
        if (m_ui.searchPlaceCombo->count() >= Project) {
            // switch to search "in Open files", if "in Project" is active
            if (m_ui.searchPlaceCombo->currentIndex() >= Project) {
                setSearchPlace(OpenFiles);
            }

            // remove "in Project" and "in all projects"
            while (m_ui.searchPlaceCombo->count() > Project) {
                m_ui.searchPlaceCombo->removeItem(m_ui.searchPlaceCombo->count() - 1);
            }
        }
    }
}

KateSearchCommand::KateSearchCommand(QObject *parent)
    : KTextEditor::Command(QStringList() << QStringLiteral("grep") << QStringLiteral("newGrep") << QStringLiteral("search") << QStringLiteral("newSearch") << QStringLiteral("pgrep") << QStringLiteral("newPGrep"), parent)
{
}

bool KateSearchCommand::exec(KTextEditor::View * /*view*/, const QString &cmd, QString & /*msg*/, const KTextEditor::Range &)
{
    // create a list of args
    QStringList args(cmd.split(QLatin1Char(' '), QString::KeepEmptyParts));
    QString command = args.takeFirst();
    QString searchText = args.join(QLatin1Char(' '));

    if (command == QLatin1String("grep") || command == QLatin1String("newGrep")) {
        emit setSearchPlace(KatePluginSearchView::Folder);
        emit setCurrentFolder();
        if (command == QLatin1String("newGrep"))
            emit newTab();
    }

    else if (command == QLatin1String("search") || command == QLatin1String("newSearch")) {
        emit setSearchPlace(KatePluginSearchView::OpenFiles);
        if (command == QLatin1String("newSearch"))
            emit newTab();
    }

    else if (command == QLatin1String("pgrep") || command == QLatin1String("newPGrep")) {
        emit setSearchPlace(KatePluginSearchView::Project);
        if (command == QLatin1String("newPGrep"))
            emit newTab();
    }

    emit setSearchString(searchText);
    emit startSearch();

    return true;
}

bool KateSearchCommand::help(KTextEditor::View * /*view*/, const QString &cmd, QString &msg)
{
    if (cmd.startsWith(QLatin1String("grep"))) {
        msg = i18n("Usage: grep [pattern to search for in folder]");
    } else if (cmd.startsWith(QLatin1String("newGrep"))) {
        msg = i18n("Usage: newGrep [pattern to search for in folder]");
    }

    else if (cmd.startsWith(QLatin1String("search"))) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    } else if (cmd.startsWith(QLatin1String("newSearch"))) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    }

    else if (cmd.startsWith(QLatin1String("pgrep"))) {
        msg = i18n("Usage: pgrep [pattern to search for in current project]");
    } else if (cmd.startsWith(QLatin1String("newPGrep"))) {
        msg = i18n("Usage: newPGrep [pattern to search for in current project]");
    }

    return true;
}

#include "plugin_search.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
