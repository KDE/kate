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
#include <kiconloader.h>

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

static QUrl localFileDirUp (const QUrl &url)
{
    if (!url.isLocalFile())
        return url;
    
    // else go up
    return QUrl::fromLocalFile (QFileInfo (url.toLocalFile()).dir().absolutePath());
}

static QAction *menuEntry(QMenu *menu,
                          const QString &before, const QString &after, const QString &desc,
                          QString menuBefore = QString(), QString menuAfter = QString());

static QAction *menuEntry(QMenu *menu,
                          const QString &before, const QString &after, const QString &desc,
                          QString menuBefore, QString menuAfter)
{
    if (menuBefore.isEmpty()) menuBefore = before;
    if (menuAfter.isEmpty())  menuAfter = after;

    QAction *const action = menu->addAction(menuBefore + menuAfter + QLatin1Char('\t') + desc);
    if (!action) return 0;

    action->setData(QString(before + QLatin1Char(' ') + after));
    return action;
}

class TreeWidgetItem : public QTreeWidgetItem {
public:
    TreeWidgetItem(QTreeWidget* parent):QTreeWidgetItem(parent){}
    TreeWidgetItem(QTreeWidget* parent, const QStringList &list):QTreeWidgetItem(parent, list){}
    TreeWidgetItem(QTreeWidgetItem* parent, const QStringList &list):QTreeWidgetItem(parent, list){}
private:
    bool operator<(const QTreeWidgetItem &other)const {
        if (childCount() == 0) {
            int line = data(0, ReplaceMatches::LineRole).toInt();
            int column = data(0, ReplaceMatches::ColumnRole).toInt();
            int oLine = other.data(0, ReplaceMatches::LineRole).toInt();
            int oColumn = other.data(0, ReplaceMatches::ColumnRole).toInt();
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
        if (sepCount < oSepCount) return true;
        if (sepCount > oSepCount) return false;
        return data(0, ReplaceMatches::FileUrlRole).toString().toLower() < other.data(0, ReplaceMatches::FileUrlRole).toString().toLower();
    }
};

Results::Results(QWidget *parent): QWidget(parent), matches(0)
{
    setupUi(this);

    tree->setItemDelegate(new SPHtmlDelegate(tree));
}


K_PLUGIN_FACTORY_WITH_JSON (KatePluginSearchFactory, "katesearch.json", registerPlugin<KatePluginSearch>();)

KatePluginSearch::KatePluginSearch(QObject* parent, const QList<QVariant>&)
    : KTextEditor::Plugin (parent),
    m_searchCommand(0)
{
    // FIXME KF5
    //KGlobal::locale()->insertCatalog("katesearch");

    m_searchCommand = new KateSearchCommand(this);
}

KatePluginSearch::~KatePluginSearch()
{
    delete m_searchCommand;
}

QObject *KatePluginSearch::createView(KTextEditor::MainWindow *mainWindow)
{
    KatePluginSearchView *view = new KatePluginSearchView(this, mainWindow, KTextEditor::Editor::instance()->application());
    connect(m_searchCommand, SIGNAL(setSearchPlace(int)), view, SLOT(setSearchPlace(int)));
    connect(m_searchCommand, SIGNAL(setCurrentFolder()), view, SLOT(setCurrentFolder()));
    connect(m_searchCommand, SIGNAL(setSearchString(QString)), view, SLOT(setSearchString(QString)));
    connect(m_searchCommand, SIGNAL(startSearch()), view, SLOT(startSearch()));
    connect(m_searchCommand, SIGNAL(newTab()), view, SLOT(addTab()));

    return view;
}


bool ContainerWidget::focusNextPrevChild (bool next)
{
    QWidget* fw = focusWidget();
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
        if (currentWidget->objectName() == QStringLiteral("tree")) {
            m_ui.newTabButton->setFocus();
            *found = true;
            return;
        }
        if (currentWidget == m_ui.displayOptions) {
            if (m_ui.displayOptions->isChecked()) {
                m_ui.newTabButton->setFocus();
                *found = true;
                return;
            }
            else {
                Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
                if (!res) {
                    return;
                }
                res->tree->setFocus();
                *found = true;
                return;
            }
        }
    }
    else {
        if (currentWidget == m_ui.newTabButton) {
            if (m_ui.displayOptions->isChecked()) {
                m_ui.displayOptions->setFocus();
            }
            else {
                Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
                if (!res) {
                    return;
                }
                res->tree->setFocus();
            }
            *found = true;
            return;
        }
        else {
            if (currentWidget->objectName() == QStringLiteral("tree")) {
                m_ui.displayOptions->setFocus();
                *found = true;
                return;
            }
        }
    }
}

KatePluginSearchView::KatePluginSearchView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWin, KTextEditor::Application* application)
: QObject (mainWin),
m_kateApp(application),
m_curResults(0),
m_searchJustOpened(false),
m_switchToProjectModeWhenAvailable(false),
m_searchDiskFilesDone(true),
m_searchOpenFilesDone(true),
m_projectPluginView(0),
m_mainWindow (mainWin)
{
    KXMLGUIClient::setComponentName (QStringLiteral("katesearch"), i18n ("Kate Search & Replace"));
    setXMLFile( QStringLiteral("ui.rc") );
  
    m_toolView = mainWin->createToolView (plugin, QStringLiteral("kate_plugin_katesearch"),
                                          KTextEditor::MainWindow::Bottom,
                                          SmallIcon(QStringLiteral("edit-find")),
                                          i18n("Search and Replace"));

    ContainerWidget *container = new ContainerWidget(m_toolView);
    m_ui.setupUi(container);
    container->setFocusProxy(m_ui.searchCombo);
    connect(container, SIGNAL(nextFocus(QWidget*,bool*,bool)), this, SLOT(nextFocus(QWidget*,bool*,bool)));

    QAction *a = actionCollection()->addAction(QStringLiteral("search_in_files"));
    a->setText(i18n("Search in Files"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(openSearchView()));

    a = actionCollection()->addAction(QStringLiteral("search_in_files_new_tab"));
    a->setText(i18n("Search in Files (in new tab)"));
    // first add tab, then open search view, since open search view switches to show the search options
    connect(a, SIGNAL(triggered(bool)), this, SLOT(addTab()));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(openSearchView()));

    a = actionCollection()->addAction(QStringLiteral("go_to_next_match"));
    a->setText(i18n("Go to Next Match"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(goToNextMatch()));

    a = actionCollection()->addAction(QStringLiteral("go_to_prev_match"));
    a->setText(i18n("Go to Previous Match"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(goToPreviousMatch()));

    m_ui.resultTabWidget->tabBar()->setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
    KAcceleratorManager::setNoAccel(m_ui.resultTabWidget);

    m_ui.displayOptions->setIcon(SmallIcon(QStringLiteral("arrow-down-double")));
    m_ui.searchButton->setIcon(SmallIcon(QStringLiteral("edit-find")));
    m_ui.stopButton->setIcon(SmallIcon(QStringLiteral("process-stop")));
    m_ui.searchPlaceCombo->setItemIcon(0, SmallIcon(QStringLiteral("text-plain")));
    m_ui.searchPlaceCombo->setItemIcon(1, SmallIcon(QStringLiteral("folder")));
    m_ui.folderUpButton->setIcon(SmallIcon(QStringLiteral("go-up")));
    m_ui.currentFolderButton->setIcon(SmallIcon(QStringLiteral("view-refresh")));
    m_ui.newTabButton->setIcon(SmallIcon(QStringLiteral("tab-new")));

    m_ui.filterCombo->setToolTip(i18n("Comma separated list of file types to search in. Example: \"*.cpp,*.h\"\n"));
    m_ui.excludeCombo->setToolTip(i18n("Comma separated list of files and directories to exclude from the search. Example: \"build*\""));

    // the order here is important to get the tabBar hidden for only one tab
    addTab();
    m_ui.resultTabWidget->tabBar()->hide();

    // get url-requester's combo box and sanely initialize
    KComboBox* cmbUrl = m_ui.folderRequester->comboBox();
    cmbUrl->setDuplicatesEnabled(false);
    cmbUrl->setEditable(true);
    m_ui.folderRequester->setMode(KFile::Directory | KFile::LocalOnly);
    KUrlCompletion* cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
    cmbUrl->setCompletionObject(cmpl);
    cmbUrl->setAutoDeleteCompletionObject(true);

    connect(m_ui.newTabButton,     SIGNAL(clicked()), this, SLOT(addTab()));
    connect(m_ui.resultTabWidget,  SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
    connect(m_ui.resultTabWidget,  SIGNAL(currentChanged(int)), this, SLOT(resultTabChanged(int)));

    connect(m_ui.folderUpButton,   SIGNAL(clicked()), this, SLOT(navigateFolderUp()));
    connect(m_ui.currentFolderButton, SIGNAL(clicked()), this, SLOT(setCurrentFolder()));

    connect(m_ui.searchCombo,      SIGNAL(editTextChanged(QString)), &m_changeTimer, SLOT(start()));
    connect(m_ui.matchCase,        SIGNAL(stateChanged(int)), &m_changeTimer, SLOT(start()));
    connect(m_ui.useRegExp,        SIGNAL(stateChanged(int)), &m_changeTimer, SLOT(start()));
    m_changeTimer.setInterval(300);
    m_changeTimer.setSingleShot(true);
    connect(&m_changeTimer, SIGNAL(timeout()), this, SLOT(startSearchWhileTyping()));

    connect(m_ui.searchCombo->lineEdit(), SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.folderRequester,  SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.filterCombo,      SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.excludeCombo,     SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.searchButton,     SIGNAL(clicked()),       this, SLOT(startSearch()));

    connect(m_ui.displayOptions,   SIGNAL(toggled(bool)), this, SLOT(toggleOptions(bool)));
    connect(m_ui.searchPlaceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(searchPlaceChanged()));

    connect(m_ui.stopButton,       SIGNAL(clicked()), &m_searchOpenFiles, SLOT(cancelSearch()));
    connect(m_ui.stopButton,       SIGNAL(clicked()), &m_searchDiskFiles, SLOT(cancelSearch()));
    connect(m_ui.stopButton,       SIGNAL(clicked()), &m_folderFilesList, SLOT(cancelSearch()));
    connect(m_ui.stopButton,       SIGNAL(clicked()), &m_replacer,        SLOT(cancelReplace()));

    connect(m_ui.nextButton,       SIGNAL(clicked()), this, SLOT(goToNextMatch()));

    connect(m_ui.replaceButton,     SIGNAL(clicked(bool)),   this, SLOT(replaceSingleMatch()));
    connect(m_ui.replaceCheckedBtn, SIGNAL(clicked(bool)),   this, SLOT(replaceChecked()));
    connect(m_ui.replaceCombo->lineEdit(), SIGNAL(returnPressed()), this, SLOT(replaceChecked()));



    m_ui.displayOptions->setChecked(true);

    connect(&m_searchOpenFiles, SIGNAL(matchFound(QString,QString,int,int,QString,int)),
            this,                 SLOT(matchFound(QString,QString,int,int,QString,int)));
    connect(&m_searchOpenFiles, SIGNAL(searchDone()),  this, SLOT(searchDone()));
    connect(&m_searchOpenFiles, SIGNAL(searching(QString)), this, SLOT(searching(QString)));

    connect(&m_folderFilesList, SIGNAL(finished()),  this, SLOT(folderFileListChanged()));

    connect(&m_searchDiskFiles, SIGNAL(matchFound(QString,QString,int,int,QString,int)),
            this,                 SLOT(matchFound(QString,QString,int,int,QString,int)));
    connect(&m_searchDiskFiles, SIGNAL(searchDone()),  this, SLOT(searchDone()));
    connect(&m_searchDiskFiles, SIGNAL(searching(QString)), this, SLOT(searching(QString)));

    connect(m_kateApp, SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
            &m_searchOpenFiles, SLOT(cancelSearch()));

    connect(m_kateApp, SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
            &m_replacer, SLOT(cancelReplace()));

    connect(m_kateApp, SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
            this, SLOT(clearDocMarks(KTextEditor::Document*)));

    connect(&m_replacer, SIGNAL(matchReplaced(KTextEditor::Document*,int,int,int)),
            this, SLOT(addMatchMark(KTextEditor::Document*,int,int,int)));

    // Hook into line edit context menus
    m_ui.searchCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.searchCombo, SIGNAL(customContextMenuRequested(QPoint)), this,
            SLOT(searchContextMenu(QPoint)));
    m_ui.searchCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    m_ui.searchCombo->setInsertPolicy(QComboBox::NoInsert);
    m_ui.searchCombo->lineEdit()->setClearButtonEnabled(true);
    m_ui.searchCombo->setMaxCount(25);

    m_ui.replaceCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    m_ui.replaceCombo->setInsertPolicy(QComboBox::NoInsert);
    m_ui.replaceCombo->lineEdit()->setClearButtonEnabled(true);
    m_ui.replaceCombo->setMaxCount(25);

    connect(m_mainWindow, SIGNAL(unhandledShortcutOverride(QEvent*)),
            this, SLOT(handleEsc(QEvent*)));

    // watch for project plugin view creation/deletion
    connect(m_mainWindow, SIGNAL(pluginViewCreated (const QString &, QObject *))
        , this, SLOT(slotPluginViewCreated (const QString &, QObject *)));

    connect(m_mainWindow, SIGNAL(pluginViewDeleted (const QString &, QObject *))
        , this, SLOT(slotPluginViewDeleted (const QString &, QObject *)));

    connect(m_mainWindow, SIGNAL(viewChanged(KTextEditor::View *)), this, SLOT(docViewChanged()));


    // update once project plugin state manually
    m_projectPluginView = m_mainWindow->pluginView (QStringLiteral("kateprojectplugin"));
    slotProjectFileNameChanged ();

    m_replacer.setDocumentManager(m_kateApp);
    connect(&m_replacer, SIGNAL(replaceDone()), this, SLOT(replaceDone()));

    searchPlaceChanged();

    m_toolView->installEventFilter(this);

    m_mainWindow->guiFactory()->addClient(this);
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
    m_ui.folderRequester->setUrl(localFileDirUp (m_ui.folderRequester->url()));
}

void KatePluginSearchView::setCurrentFolder()
{
    if (!m_mainWindow) {
        return;
    }
    KTextEditor::View* editView = m_mainWindow->activeView();
    if (editView && editView->document()) {
        // upUrl as we want the folder not the file
        m_ui.folderRequester->setUrl(localFileDirUp (editView->document()->url()));
    }
}


QString KatePluginSearchView::currentWord(const KTextEditor::Document& document, const KTextEditor::Cursor& cursor ) const
{
    QString textLine = document.line(cursor.line());

    int len = textLine.length();

    if (cursor.column() > len) {       // Probably because of non-wrapping cursor mode.
        return QString();
    }

    int start = cursor.column();
    for(int currPos = cursor.column()-1; currPos >= 0; currPos--) {
        if (textLine.at(currPos).isLetterOrNumber() || (textLine[currPos]==QLatin1Char('_')) || (textLine[currPos]==QLatin1Char('~'))) {
            start = currPos;
        }
        else {
            break;
        }
    }

    int end = cursor.column();
    while (end < len && (textLine.at(end).isLetterOrNumber()
                     || (textLine[end]==QLatin1Char('_')) || (textLine[end]==QLatin1Char('~')))) {
        end++;
    }

    return textLine.mid(start, (end - start));
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
    m_ui.displayOptions->setChecked(true);

    KTextEditor::View* editView = m_mainWindow->activeView();
    if (editView && editView->document()) {
        if (m_ui.folderRequester->text().isEmpty()) {
            // upUrl as we want the folder not the file
            m_ui.folderRequester->setUrl(localFileDirUp (editView->document()->url()));
        }
        QString selection;
        if (editView->selection()) {
            selection = editView->selectionText();
            // remove possible trailing '\n'
            if (selection.endsWith(QLatin1Char('\n'))) {
                selection = selection.left(selection.size() -1);
            }
        }
        if (selection.isEmpty()) {
            selection = currentWord(*editView->document(), editView->cursorPosition());
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
    if (!m_mainWindow) return;

    QKeyEvent *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {

        if (m_toolView->isVisible()) {
            m_mainWindow->hideToolView(m_toolView);
        }
        else {
            clearMarks();
        }
    }
}

void KatePluginSearchView::setSearchString(const QString &pattern)
{
    m_ui.searchCombo->lineEdit()->setText(pattern);
}

void KatePluginSearchView::toggleOptions(bool show)
{
    m_ui.stackedWidget->setCurrentIndex((show) ? 1:0);
}

void KatePluginSearchView::setSearchPlace(int place)
{
    m_ui.searchPlaceCombo->setCurrentIndex(place);
}

QStringList KatePluginSearchView::filterFiles(const QStringList& files) const
{
    QString types = m_ui.filterCombo->currentText();
    QString excludes = m_ui.excludeCombo->currentText();
    if (((types.isEmpty() || types == QStringLiteral("*"))) && (excludes.isEmpty())) {
        // shortcut for use all files
        return files;
    }

    QStringList tmpTypes = types.split(QLatin1Char(','));
    QVector<QRegExp> typeList;
    for (int i=0; i<tmpTypes.size(); i++) {
        QRegExp rx(tmpTypes[i]);
        rx.setPatternSyntax(QRegExp::Wildcard);
        typeList << rx;
    }

    QStringList tmpExcludes = excludes.split(QLatin1Char(','));
    QVector<QRegExp> excludeList;
    for (int i=0; i<tmpExcludes.size(); i++) {
        QRegExp rx(tmpExcludes[i]);
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
        for (int i=0; i<excludeList.size(); i++) {
            if (excludeList[i].exactMatch(nameToCheck)) {
                skip = true;
                break;
            }
        }
        if (skip) {
            continue;
        }


        for (int i=0; i<typeList.size(); i++) {
            if (typeList[i].exactMatch(nameToCheck)) {
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

    QList<KTextEditor::Document*> openList;
    for (int i=0; i<m_kateApp->documents().size(); i++) {
        int index = fileList.indexOf(m_kateApp->documents()[i]->url().toString());
        if (index != -1) {
            openList << m_kateApp->documents()[i];
            fileList.removeAt(index);
        }
    }

    // search order is important: Open files starts immediately and should finish
    // earliest after first event loop.
    // The DiskFile might finish immediately
    if (openList.size() > 0) {
        m_searchOpenFiles.startSearch(openList, m_curResults->regExp);
    }
    else {
        m_searchOpenFilesDone = true;
    }

    m_searchDiskFiles.startSearch(fileList, m_curResults->regExp);
}


void KatePluginSearchView::searchPlaceChanged()
{
    m_ui.displayOptions->setChecked(true);

    const bool inFolder = (m_ui.searchPlaceCombo->currentIndex() == 1);
    const bool inProject = (m_ui.searchPlaceCombo->currentIndex() == 2);

    m_ui.filterCombo->setEnabled(inFolder || inProject);

    m_ui.excludeCombo->setEnabled(inFolder || inProject);
    m_ui.folderRequester->setEnabled(inFolder);
    m_ui.folderUpButton->setEnabled(inFolder);
    m_ui.currentFolderButton->setEnabled(inFolder);
    m_ui.recursiveCheckBox->setEnabled(inFolder);
    m_ui.hiddenCheckBox->setEnabled(inFolder);
    m_ui.symLinkCheckBox->setEnabled(inFolder);
    m_ui.binaryCheckBox->setEnabled(inFolder);

    // ... and the labels:
    m_ui.folderLabel->setEnabled(m_ui.folderRequester->isEnabled());
    m_ui.filterLabel->setEnabled(m_ui.filterCombo->isEnabled());
    m_ui.excludeLabel->setEnabled(m_ui.excludeCombo->isEnabled());
}

void KatePluginSearchView::addHeaderItem()
{
    QTreeWidgetItem *item = new QTreeWidgetItem(m_curResults->tree, QStringList());
    item->setCheckState(0, Qt::Checked);
    item->setFlags(item->flags() | Qt::ItemIsTristate);
    m_curResults->tree->expandItem(item);
}

QTreeWidgetItem * KatePluginSearchView::rootFileItem(const QString &url, const QString &fName)
{
    if (!m_curResults) {
        return 0;
    }

    // FIXME KF5
    QUrl kurl = QUrl::fromUserInput(url);
    QString path = kurl.isLocalFile() ? localFileDirUp (kurl).path() : kurl.url();
    path.replace(m_resultBaseDir, QString());
    QString name = kurl.fileName();
    if (url.isEmpty()) {
        name = fName;
    }

    // make sure we have a root item
    if (m_curResults->tree->topLevelItemCount() == 0) {
        addHeaderItem();
    }
    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);

    if (root->data(0, ReplaceMatches::FileNameRole).toString() == fName) {
        // The root item contains the document name ->
        // this is search as you type, return the root item
        return root;
    }

    for (int i=0; i<root->childCount(); i++) {
        //qDebug() << root->child(i)->data(0, ReplaceMatches::FileNameRole).toString() << fName;
        if ((root->child(i)->data(0, ReplaceMatches::FileUrlRole).toString() == url)&&
            (root->child(i)->data(0, ReplaceMatches::FileNameRole).toString() == fName)) {
            int matches = root->child(i)->data(0, ReplaceMatches::LineRole).toInt() + 1;
            QString tmpUrl = QString::fromLatin1("%1<b>%2</b>: <b>%3</b>").arg(path).arg(name).arg(matches);
            root->child(i)->setData(0, Qt::DisplayRole, tmpUrl);
            root->child(i)->setData(0, ReplaceMatches::LineRole, matches);
            return root->child(i);
        }
    }

    // file item not found create a new one
    QString tmpUrl = QString::fromLatin1("%1<b>%2</b>: <b>%3</b>").arg(path).arg(name).arg(1);

    TreeWidgetItem *item = new TreeWidgetItem(root, QStringList(tmpUrl));
    item->setData(0, ReplaceMatches::FileUrlRole, url);
    item->setData(0, ReplaceMatches::FileNameRole, fName);
    item->setData(0, ReplaceMatches::LineRole, 1);
    item->setCheckState(0, Qt::Checked);
    item->setFlags(item->flags() | Qt::ItemIsTristate);
    return item;
}

void KatePluginSearchView::addMatchMark(KTextEditor::Document* doc, int line, int column, int matchLen)
{
    if (!doc) return;

    KTextEditor::MovingInterface* miface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    KTextEditor::ConfigInterface* ciface = qobject_cast<KTextEditor::ConfigInterface*>(m_mainWindow->activeView());
    KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());

    bool replace = ((sender() == &m_replacer) || (sender() == 0) || (sender() == m_ui.replaceButton));
    if (replace) {
        QColor replaceColor(Qt::green);
        if (ciface) replaceColor = ciface->configValue(QStringLiteral("replace-highlight-color")).value<QColor>();
        attr->setBackground(replaceColor);
    }
    else {
        QColor searchColor(Qt::yellow);
        if (ciface) searchColor = ciface->configValue(QStringLiteral("search-highlight-color")).value<QColor>();
        attr->setBackground(searchColor);
    }
    // calculate end line in case of multi-line match
    int endLine = line;
    int endColumn = column+matchLen;
    while ((endLine < doc->lines()) &&  (endColumn > doc->line(endLine).size())) {
        endColumn -= doc->line(endLine).size();
        endColumn--; // remove one for '\n'
        endLine++;
    }

    KTextEditor::Range range(line, column, endLine, endColumn);

    if (m_curResults && !replace) {
        // special handling for "(?=\\n)" in multi-line search
        QRegExp tmpReg = m_curResults->regExp;
        if (m_curResults->regExp.pattern().endsWith(QStringLiteral("(?=\\n)"))) {
            QString newPatern = tmpReg.pattern();
            newPatern.replace(QStringLiteral("(?=\\n)"), QStringLiteral("$"));
            tmpReg.setPattern(newPatern);
        }

        if (tmpReg.indexIn(doc->text(range)) != 0) {
            qDebug() << doc->text(range) << "Does not match" << m_curResults->regExp.pattern();
            return;
        }
    }

    KTextEditor::MovingRange* mr = miface->newMovingRange(range);
    mr->setAttribute(attr);
    mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
    mr->setAttributeOnlyForViews(true);
    m_matchRanges.append(mr);

    KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
    if (!iface) return;
    iface->setMarkDescription(KTextEditor::MarkInterface::markType32, i18n("SearchHighLight"));
    iface->setMarkPixmap(KTextEditor::MarkInterface::markType32,
                         QIcon().pixmap(0,0));
    iface->addMark(line, KTextEditor::MarkInterface::markType32);

    connect(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            this, SLOT(clearMarks()), Qt::UniqueConnection);
}

void KatePluginSearchView::matchFound(const QString &url, const QString &fName, int line, int column,
                                      const QString &lineContent, int matchLen)
{
    if (!m_curResults) {
        return;
    }

    QString pre = lineContent.left(column).toHtmlEscaped();
    QString match = lineContent.mid(column, matchLen).toHtmlEscaped();
    match.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    QString post = lineContent.mid(column + matchLen).toHtmlEscaped();
    QStringList row;
    row << i18n("Line: <b>%1</b>: %2", line+1, pre+QStringLiteral("<b>")+match+QStringLiteral("</b>")+post);

    TreeWidgetItem *item = new TreeWidgetItem(rootFileItem(url, fName), row);
    item->setData(0, ReplaceMatches::FileUrlRole, url);
    item->setData(0, Qt::ToolTipRole, url);
    item->setData(0, ReplaceMatches::FileNameRole, fName);
    item->setData(0, ReplaceMatches::LineRole, line);
    item->setData(0, ReplaceMatches::ColumnRole, column);
    item->setData(0, ReplaceMatches::MatchLenRole, matchLen);
    item->setData(0, ReplaceMatches::PreMatchRole, pre);
    item->setData(0, ReplaceMatches::MatchRole, match);
    item->setData(0, ReplaceMatches::PostMatchRole, post);
    item->setCheckState (0, Qt::Checked);

    m_curResults->matches++;

    // Add mark if the document is open
    KTextEditor::Document* doc;
    if (url.isEmpty()) {
        doc = m_replacer.findNamed(fName);
    }
    else {
        doc = m_kateApp->findUrl(QUrl::fromUserInput(url));
    }
    addMatchMark(doc, line, column, matchLen);
}

void KatePluginSearchView::clearMarks()
{
    // FIXME: check for ongoing search...
    KTextEditor::MarkInterface* iface;
    foreach (KTextEditor::Document* doc, m_kateApp->documents()) {
        iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        if (iface) {
            const QHash<int, KTextEditor::Mark*> marks = iface->marks();
            QHashIterator<int, KTextEditor::Mark*> i(marks);
            while (i.hasNext()) {
                i.next();
                if (i.value()->type & KTextEditor::MarkInterface::markType32) {
                    iface->removeMark(i.value()->line, KTextEditor::MarkInterface::markType32);
                }
            }
        }
    }
    qDeleteAll(m_matchRanges);
    m_matchRanges.clear();
}

void KatePluginSearchView::clearDocMarks(KTextEditor::Document* doc)
{
    //qDebug() << sender();
    // FIXME: check for ongoing search...
    KTextEditor::MarkInterface* iface;
    iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
    if (iface) {
        const QHash<int, KTextEditor::Mark*> marks = iface->marks();
        QHashIterator<int, KTextEditor::Mark*> i(marks);
        while (i.hasNext()) {
            i.next();
            if (i.value()->type & KTextEditor::MarkInterface::markType32) {
                iface->removeMark(i.value()->line, KTextEditor::MarkInterface::markType32);
            }
        }
    }

    int i = 0;
    while (i<m_matchRanges.size()) {
        if (m_matchRanges.at(i)->document() == doc) {
            //qDebug() << "removing mark in" << doc->url();
            delete m_matchRanges.at(i);
            m_matchRanges.removeAt(i);
        }
        else {
            i++;
        }
    }
}

void KatePluginSearchView::startSearch()
{
    m_changeTimer.stop(); // make sure not to start a "while you type" search now
    m_mainWindow->showToolView(m_toolView); // in case we are invoked from the command interface
    m_switchToProjectModeWhenAvailable = false; // now that we started, don't switch back automatically

    if (m_ui.searchCombo->currentText().isEmpty()) {
        // return pressed in the folder combo or filter combo
        return;
    }

    if (m_ui.searchCombo->findText(m_ui.searchCombo->currentText()) == -1) {
        m_ui.searchCombo->insertItem(1, m_ui.searchCombo->currentText());
        m_ui.searchCombo->setCurrentIndex(1);
    }

    if (m_ui.filterCombo->findText(m_ui.filterCombo->currentText()) == -1) {
        m_ui.filterCombo->insertItem(0, m_ui.filterCombo->currentText());
        m_ui.filterCombo->setCurrentIndex(0);
    }
    if (m_ui.excludeCombo->findText(m_ui.excludeCombo->currentText()) == -1) {
        m_ui.excludeCombo->insertItem(0, m_ui.excludeCombo->currentText());
        m_ui.excludeCombo->setCurrentIndex(0);
    }
    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "This is a bug";
        return;
    }

    m_ui.newTabButton->setDisabled(true);
    m_ui.searchCombo->setDisabled(true);
    m_ui.searchButton->setDisabled(true);
    m_ui.displayOptions->setChecked (false);
    m_ui.displayOptions->setDisabled(true);
    m_ui.replaceCheckedBtn->setDisabled(true);
    m_ui.replaceButton->setDisabled(true);
    m_ui.nextAndStop->setCurrentIndex(1);
    m_ui.replaceCombo->setDisabled(true);


    QRegExp reg(m_ui.searchCombo->currentText(),
                m_ui.matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                m_ui.useRegExp->isChecked() ? QRegExp::RegExp : QRegExp::FixedString);
    m_curResults->regExp = reg;

    clearMarks();
    m_curResults->tree->clear();
    m_curResults->matches = 0;

    m_ui.resultTabWidget->setTabText(m_ui.resultTabWidget->currentIndex(),
                                     m_ui.searchCombo->currentText());

    m_toolView->setCursor(Qt::WaitCursor);
    m_searchDiskFilesDone = false;
    m_searchOpenFilesDone = false;

    if (m_ui.searchPlaceCombo->currentIndex() ==  0) {
        m_searchDiskFilesDone = true;
        m_resultBaseDir.clear();
        const QList<KTextEditor::Document*> documents = m_kateApp->documents();
        addHeaderItem();
        m_searchOpenFiles.startSearch(documents, reg);
    }
    else if (m_ui.searchPlaceCombo->currentIndex() == 1) {
        m_resultBaseDir = m_ui.folderRequester->text();
        addHeaderItem();
        m_folderFilesList.generateList(m_ui.folderRequester->text(),
                                       m_ui.recursiveCheckBox->isChecked(),
                                       m_ui.hiddenCheckBox->isChecked(),
                                       m_ui.symLinkCheckBox->isChecked(),
                                       m_ui.binaryCheckBox->isChecked(),
                                       m_ui.filterCombo->currentText(),
                                       m_ui.excludeCombo->currentText());
        // the file list will be ready when the thread returns (connected to folderFileListChanged)
    }
    else {
        /**
         * init search with file list from current project, if any
         */
        m_resultBaseDir.clear();
        QStringList files;
        QString projectName;
        if (m_projectPluginView) {
            projectName = m_projectPluginView->property ("projectName").toString();
            m_resultBaseDir = m_projectPluginView->property ("projectBaseDir").toString();
            if (!m_resultBaseDir.endsWith(QLatin1Char('/')))
                m_resultBaseDir += QLatin1Char('/');
            QStringList projectFiles = m_projectPluginView->property ("projectFiles").toStringList();
            files = filterFiles(projectFiles);
        }
        addHeaderItem();

        QList<KTextEditor::Document*> openList;
        for (int i=0; i<m_kateApp->documents().size(); i++) {
            int index = files.indexOf(m_kateApp->documents()[i]->url().toString());
            if (index != -1) {
                openList << m_kateApp->documents()[i];
                files.removeAt(index);
            }
        }
        // search order is important: Open files starts immediately and should finish
        // earliest after first event loop.
        // The DiskFile might finish immediately
        if (openList.size() > 0) {
            m_searchOpenFiles.startSearch(openList, m_curResults->regExp);
        }
        else {
            m_searchOpenFilesDone = true;
        }
        m_searchDiskFiles.startSearch(files, reg);
    }
}

void KatePluginSearchView::startSearchWhileTyping()
{
    if (!m_searchDiskFilesDone || !m_searchOpenFilesDone) {
        return;
    }

    m_ui.searchButton->setDisabled(m_ui.searchCombo->currentText().isEmpty());

    if (!m_mainWindow->activeView()) return;

    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    if (!doc) return;

    m_curResults =qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "This is a bug";
        return;
    }

    m_ui.replaceCheckedBtn->setDisabled(true);
    m_ui.replaceButton->setDisabled(true);
    m_ui.nextButton->setDisabled(true);

    QRegExp reg(m_ui.searchCombo->currentText(),
                m_ui.matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                m_ui.useRegExp->isChecked() ? QRegExp::RegExp : QRegExp::FixedString);

    m_curResults->regExp = reg;

    clearMarks();
    m_curResults->tree->clear();
    m_curResults->matches = 0;

    m_resultBaseDir.clear();

    // add header item
    TreeWidgetItem *item = new TreeWidgetItem(m_curResults->tree, QStringList());
    item->setData(0, ReplaceMatches::FileUrlRole, doc->url().toString());
    item->setData(0, ReplaceMatches::FileNameRole, doc->documentName());
    item->setData(0, ReplaceMatches::LineRole, 0);
    item->setCheckState(0, Qt::Checked);
    item->setFlags(item->flags() | Qt::ItemIsTristate);

    if (m_ui.searchCombo->currentText().length() >= 2) {
        m_searchOpenFiles.searchOpenFile(doc, reg, 0);
    }
    searchWhileTypingDone();
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

    m_ui.newTabButton->setDisabled(false);
    m_ui.searchCombo->setDisabled(false);
    m_ui.searchButton->setDisabled(false);
    m_ui.nextAndStop->setCurrentIndex(0);
    m_ui.displayOptions->setDisabled(false);
    m_ui.replaceCombo->setDisabled(false);

    if (!m_curResults) {
        return;
    }

    m_ui.replaceCheckedBtn->setDisabled(m_curResults->matches < 1);
    m_ui.replaceButton->setDisabled(m_curResults->matches < 1);
    m_ui.nextButton->setDisabled(m_curResults->matches < 1);

    m_curResults->tree->sortItems(0, Qt::AscendingOrder);

    m_curResults->tree->expandAll();
    m_curResults->tree->resizeColumnToContents(0);
    if (m_curResults->tree->columnWidth(0) < m_curResults->tree->width()-30) {
        m_curResults->tree->setColumnWidth(0, m_curResults->tree->width()-30);
    }

    // expand the "header item " to display all files and all results if configured
    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);
    m_curResults->tree->expandItem(root);
    if (root && (root->childCount() > 1) && (!m_ui.expandResults->isChecked())) {
        for (int i=0; i<root->childCount(); i++) {
            m_curResults->tree->collapseItem(root->child(i));
        }
    }

    m_curResults->tree->setCurrentItem(root);
    m_curResults->tree->setFocus(Qt::OtherFocusReason);

    if (root) {
        switch (m_ui.searchPlaceCombo->currentIndex())
        {
            case 0:
                root->setData(0, Qt::DisplayRole, i18np("<b><i>One match found in open files</i></b>",
                                                        "<b><i>%1 matches found in open files</i></b>",
                                                        m_curResults->matches));
                break;
            case 1:
                root->setData(0, Qt::DisplayRole, i18np("<b><i>One match found in folder %2</i></b>",
                                                        "<b><i>%1 matches found in folder %2</i></b>",
                                                        m_curResults->matches,
                                                        m_resultBaseDir));
                break;
            case 2:
                QString projectName;
                if (m_projectPluginView) {
                    projectName = m_projectPluginView->property("projectName").toString();
                }
                root->setData(0, Qt::DisplayRole, i18np("<b><i>One match found in project %2 (%3)</i></b>",
                                                        "<b><i>%1 matches found in project %2 (%3)</i></b>",
                                                        m_curResults->matches,
                                                        projectName,
                                                        m_resultBaseDir));
                break;
        }
    }

    indicateMatch(m_curResults->matches > 0);
    m_curResults = 0;
    m_toolView->unsetCursor();
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
    if (m_curResults->tree->columnWidth(0) < m_curResults->tree->width()-30) {
        m_curResults->tree->setColumnWidth(0, m_curResults->tree->width()-30);
    }

    QTreeWidgetItem *root = m_curResults->tree->topLevelItem(0);
    if (root) {
        QTreeWidgetItem *child = root->child(0);
        if (!m_searchJustOpened) {
            itemSelected(child);
        }
        indicateMatch(child);

        root->setData(0, Qt::DisplayRole, i18np("<b><i>One match found</i></b>",
                                                "<b><i>%1 matches found</i></b>",
                                                m_curResults->matches));
    }
    m_curResults = 0;

    m_ui.searchCombo->lineEdit()->setFocus();
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
        }
        else {
            root->setData(0, Qt::DisplayRole, i18n("<b>Searching: %1</b>", file));
        }
    }
}

void KatePluginSearchView::indicateMatch(bool hasMatch) {
    QLineEdit * const lineEdit = m_ui.searchCombo->lineEdit();
    QPalette background(lineEdit->palette());

    if (hasMatch) {
        // Green background for line edit
        KColorScheme::adjustBackground(background, KColorScheme::PositiveBackground);
    }
    else {
        // Reset background of line edit
        background = QPalette();
    }
    // Red background for line edit
    //KColorScheme::adjustBackground(background, KColorScheme::NegativeBackground);
    // Neutral background
    //KColorScheme::adjustBackground(background, KColorScheme::NeutralBackground);

    lineEdit->setPalette(background);
}

void KatePluginSearchView::replaceSingleMatch()
{
    if (m_ui.searchCombo->findText(m_ui.searchCombo->currentText()) == -1) {
        m_ui.searchCombo->insertItem(1, m_ui.searchCombo->currentText());
        m_ui.searchCombo->setCurrentIndex(1);
    }

    if (m_ui.replaceCombo->findText(m_ui.replaceCombo->currentText()) == -1) {
        m_ui.replaceCombo->insertItem(1, m_ui.replaceCombo->currentText());
        m_ui.replaceCombo->setCurrentIndex(1);
    }

    // check if the cursor is at the current item if not jump there
    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        return;
    }
    QTreeWidgetItem *item = res->tree->currentItem();
    if (!item || !item->parent()) {
        // nothing was selected
        goToNextMatch();
        return;
    }

    if (!m_mainWindow->activeView() || !m_mainWindow->activeView()->cursorPosition().isValid()) {
        itemSelected(item);
        return;
    }

    int dLine = m_mainWindow->activeView()->cursorPosition().line();
    int dColumn = m_mainWindow->activeView()->cursorPosition().column();

    int iLine = item->data(0, ReplaceMatches::LineRole).toInt();
    int iColumn = item->data(0, ReplaceMatches::ColumnRole).toInt();

    if ((dLine != iLine) || (dColumn != iColumn)) {
        itemSelected(item);
        return;
    }

    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    // Find the corresponding range
    int i;
    for (i=0; i<m_matchRanges.size(); i++) {
        if (m_matchRanges[i]->document() != doc) continue;
        if (m_matchRanges[i]->start().line() != iLine) continue;
        if (m_matchRanges[i]->start().column() != iColumn) continue;
        break;
    }

    if (i >=m_matchRanges.size()) {
        goToNextMatch();
        return;
    }

    if (!res->regExp.exactMatch(doc->text(m_matchRanges[i]->toRange()))) {
        qDebug() << doc->text(m_matchRanges[i]->toRange()) << "Does not match" << res->regExp.pattern();
        goToNextMatch();
        return;
    }

    QString replaceText = m_ui.replaceCombo->currentText();
    replaceText.replace(QStringLiteral("\\\\"), QStringLiteral("¤Search&Replace¤"));
    for (int j=1; j<=res->regExp.captureCount(); j++) {
        replaceText.replace(QString(QStringLiteral("\\%1")).arg(j), res->regExp.cap(j));
    }
    replaceText.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    replaceText.replace(QStringLiteral("¤Search&Replace¤"), QStringLiteral("\\\\"));

    doc->replaceText(m_matchRanges[i]->toRange(), replaceText);
    addMatchMark(doc, dLine, dColumn, replaceText.size());

    replaceText.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    QString html = item->data(0, ReplaceMatches::PreMatchRole).toString();
    html += QStringLiteral("<i><s>") + item->data(0, ReplaceMatches::MatchRole).toString() + QStringLiteral("</s></i> ");
    html += QStringLiteral("<b>") + replaceText + QStringLiteral("</b>");
    html += item->data(0, ReplaceMatches::PostMatchRole).toString();
    item->setData(0, Qt::DisplayRole, i18n("Line: <b>%1</b>: %2",m_matchRanges[i]->start().line()+1, html));

    // now update the rest of the tree items for this file (they are sorted in ascending order
    i++;
    for (; i<m_matchRanges.size(); i++) {
        if (m_matchRanges[i]->document() != doc) continue;
        item = res->tree->itemBelow(item);
        if (!item) break;
        if (item->data(0, ReplaceMatches::FileUrlRole).toString() != doc->url().toString()) break;
        iLine = item->data(0, ReplaceMatches::LineRole).toInt();
        iColumn = item->data(0, ReplaceMatches::ColumnRole).toInt();
        if ((m_matchRanges[i]->start().line() == iLine) && (m_matchRanges[i]->start().column() == iColumn)) {
            break;
        }
        item->setData(0, ReplaceMatches::LineRole, m_matchRanges[i]->start().line());
        item->setData(0, ReplaceMatches::ColumnRole, m_matchRanges[i]->start().column());
    }
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

    m_curResults =qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        qWarning() << "Results not found";
        return;
    }

    m_ui.nextAndStop->setCurrentIndex(1);
    m_ui.displayOptions->setChecked(false);

    m_curResults->replace = m_ui.replaceCombo->currentText();

    m_replacer.replaceChecked(m_curResults->tree,
                              m_curResults->regExp,
                              m_curResults->replace);
}

void KatePluginSearchView::replaceDone()
{
    m_ui.nextAndStop->setCurrentIndex(0);
    m_ui.replaceCombo->setDisabled(false);
}

void KatePluginSearchView::docViewChanged()
{
    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        return;
    }

    m_curResults = res;

    if (!m_mainWindow->activeView()) {
        return;
    }

    // add the marks if it is not already open
    KTextEditor::Document *doc = m_mainWindow->activeView()->document();
    if (doc) {
        QTreeWidgetItem *rootItem = 0;
        for (int i=0; i<res->tree->topLevelItemCount(); i++) {
            QString url = res->tree->topLevelItem(i)->data(0, ReplaceMatches::FileUrlRole).toString();
            QString fName = res->tree->topLevelItem(i)->data(0, ReplaceMatches::FileNameRole).toString();
            if (url == doc->url().toString() && fName == doc->documentName()) {
                rootItem = res->tree->topLevelItem(i);
                break;
            }
        }
        if (rootItem) {

            int line;
            int column;
            int len;
            QTreeWidgetItem *item;
            for (int i=0; i<rootItem->childCount(); i++) {
                item = rootItem->child(i);
                line = item->data(0, ReplaceMatches::LineRole).toInt();
                column = item->data(0, ReplaceMatches::ColumnRole).toInt();
                len = item->data(0, ReplaceMatches::MatchLenRole).toInt();
                addMatchMark(doc, line, column, len);
            }
        }
    }
}

void KatePluginSearchView::itemSelected(QTreeWidgetItem *item)
{
    if (!item) return;

    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        return;
    }

    while (item->data(0, ReplaceMatches::ColumnRole).toString().isEmpty()) {
        item->treeWidget()->expandItem(item);
        item = item->child(0);
        if (!item) return;
    }
    item->treeWidget()->setCurrentItem(item);

    // get stuff
    int toLine = item->data(0, ReplaceMatches::LineRole).toInt();
    int toColumn = item->data(0, ReplaceMatches::ColumnRole).toInt();

    KTextEditor::Document* doc;
    QString url = item->data(0, ReplaceMatches::FileUrlRole).toString();
    if (!url.isEmpty()) {
        doc = m_kateApp->findUrl(QUrl::fromUserInput(url));
    }
    else {
        doc = m_replacer.findNamed(item->data(0, ReplaceMatches::FileNameRole).toString());
    }

    // add the marks to the document if it is not already open
    if (!doc) {
        doc = m_kateApp->openUrl(QUrl::fromUserInput(url));
        if (doc) {
            int line;
            int column;
            int len;
            QTreeWidgetItem *rootItem = (item->parent()==0) ? item : item->parent();
            for (int i=0; i<rootItem->childCount(); i++) {
                item = rootItem->child(i);
                line = item->data(0, ReplaceMatches::LineRole).toInt();
                column = item->data(0, ReplaceMatches::ColumnRole).toInt();
                len = item->data(0, ReplaceMatches::MatchLenRole).toInt();
                addMatchMark(doc, line, column, len);
            }
        }
    }
    if (!doc) return;

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
    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        return;
    }
    QTreeWidgetItem *curr = res->tree->currentItem();
    if (!curr) {
        curr = res->tree->topLevelItem(0);
    }
    if (!curr) return;

    if (!curr->data(0, ReplaceMatches::ColumnRole).toString().isEmpty()) {
        curr = res->tree->itemBelow(curr);
        if (!curr) {
            curr = res->tree->topLevelItem(0);
        }
    }

    itemSelected(curr);
}

void KatePluginSearchView::goToPreviousMatch()
{
    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!res) {
        return;
    }
    if (res->tree->topLevelItemCount() == 0) {
        return;
    }
    QTreeWidgetItem *curr = res->tree->currentItem();

    // go to the item above. (curr == null is not a problem)
    curr = res->tree->itemAbove(curr);

    // skip file name items and the root item
    while (curr && curr->data(0, ReplaceMatches::ColumnRole).toString().isEmpty()) {
        curr = res->tree->itemAbove(curr);
    }

    if (!curr) {
        // select the last child of the last next-to-top-level item
        QTreeWidgetItem *root = res->tree->topLevelItem(0);

        // select the last "root item"
        if (!root || (root->childCount() < 1)) return;
        root = root->child(root->childCount()-1);

        // select the last match of the "root item"
        if (!root || (root->childCount() < 1)) return;
        curr = root->child(root->childCount()-1);
    }

    itemSelected(curr);
}

void KatePluginSearchView::readSessionConfig(const KConfigGroup &cg)
{
    m_ui.searchCombo->clear();
    m_ui.searchCombo->addItem(QStringLiteral()); // Add empty Item
    m_ui.searchCombo->addItems(cg.readEntry("Search", QStringList()));
    m_ui.replaceCombo->clear();
    m_ui.replaceCombo->addItem(QStringLiteral()); // Add empty Item
    m_ui.replaceCombo->addItems(cg.readEntry("Replaces", QStringList()));
    m_ui.matchCase->setChecked(cg.readEntry("MatchCase", false));
    m_ui.useRegExp->setChecked(cg.readEntry("UseRegExp", false));
    m_ui.expandResults->setChecked(cg.readEntry("ExpandSearchResults", false));

    int searchPlaceIndex = cg.readEntry("Place", 1);
    if (searchPlaceIndex < 0) {
        searchPlaceIndex = 1; // for the case we happen to read -1 as Place
    }
    if ((searchPlaceIndex == 2) && (searchPlaceIndex >= m_ui.searchPlaceCombo->count())) {
        // handle the case that project mode was selected, butnot yet available
        m_switchToProjectModeWhenAvailable = true;
        searchPlaceIndex = 1;
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
    m_ui.filterCombo->setCurrentIndex(cg.readEntry("CurrentFilter", 0));
    m_ui.excludeCombo->clear();
    m_ui.excludeCombo->addItems(cg.readEntry("ExcludeFilters", QStringList()));
    m_ui.excludeCombo->setCurrentIndex(cg.readEntry("CurrentExcludeFilter", 0));
}

void KatePluginSearchView::writeSessionConfig(KConfigGroup &cg)
{
    QStringList searchHistoy;
    for (int i=1; i<m_ui.searchCombo->count(); i++) {
        searchHistoy << m_ui.searchCombo->itemText(i);
    }
    cg.writeEntry("Search", searchHistoy);
    QStringList replaceHistoy;
    for (int i=1; i<m_ui.replaceCombo->count(); i++) {
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
    for (int i=0; i<qMin(m_ui.folderRequester->comboBox()->count(), 10); i++) {
        folders << m_ui.folderRequester->comboBox()->itemText(i);
    }
    cg.writeEntry("SearchDiskFiless", folders);
    cg.writeEntry("SearchDiskFiles", m_ui.folderRequester->text());
    QStringList filterItems;
    for (int i=0; i<qMin(m_ui.filterCombo->count(), 10); i++) {
        filterItems << m_ui.filterCombo->itemText(i);
    }
    cg.writeEntry("Filters", filterItems);
    cg.writeEntry("CurrentFilter", m_ui.filterCombo->currentIndex());

    QStringList excludeFilterItems;
    for (int i=0; i<qMin(m_ui.excludeCombo->count(), 10); i++) {
        excludeFilterItems << m_ui.excludeCombo->itemText(i);
    }
    cg.writeEntry("ExcludeFilters", excludeFilterItems);
    cg.writeEntry("CurrentExcludeFilter", m_ui.excludeCombo->currentIndex());
}

void KatePluginSearchView::addTab()
{
    if ((sender() != m_ui.newTabButton) &&
        (m_ui.resultTabWidget->count() >  0) &&
        m_ui.resultTabWidget->tabText(m_ui.resultTabWidget->currentIndex()).isEmpty())
    {
        return;
    }

    Results *res = new Results();

    res->tree->setRootIsDecorated(false);

    connect(res->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this,      SLOT  (itemSelected(QTreeWidgetItem*)), Qt::QueuedConnection);

    m_ui.resultTabWidget->addTab(res, QString());
    m_ui.resultTabWidget->setCurrentIndex(m_ui.resultTabWidget->count()-1);
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
        m_curResults = 0;
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
    // empty tab -> nothing to set.
    if (m_ui.resultTabWidget->tabText(index).isEmpty()) {
        return;
    }

    Results *res = qobject_cast<Results *>(m_ui.resultTabWidget->widget(index));
    if (!res) {
        return;
    }
    // check if the text has been modified
    int i;
    for (i=0; i<m_ui.resultTabWidget->count(); i++) {
        if ((m_ui.resultTabWidget->tabText(i) == m_ui.searchCombo->currentText()) &&
            !m_ui.resultTabWidget->tabText(i).isEmpty())
        {
            break;
        }
    }
    if (i == m_ui.resultTabWidget->count()) {
        // the text does not match a tab -> do not update the search
        return;
    }

    m_ui.searchCombo->blockSignals(true);
    m_ui.matchCase->blockSignals(true);
    m_ui.useRegExp->blockSignals(true);
    m_ui.searchCombo->lineEdit()->setText(m_ui.resultTabWidget->tabText(index));
    m_ui.matchCase->setChecked(res->regExp.caseSensitivity() == Qt::CaseSensitive);
    m_ui.useRegExp->setChecked(res->regExp.patternSyntax() != QRegExp::FixedString);
    m_ui.searchCombo->blockSignals(false);
    m_ui.matchCase->blockSignals(false);
    m_ui.useRegExp->blockSignals(false);
}


bool KatePluginSearchView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
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
        if ((obj == m_toolView) && (ke->key() == Qt::Key_Escape)) {
            m_mainWindow->hideToolView(m_toolView);
            event->accept();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void KatePluginSearchView::searchContextMenu(const QPoint& pos)
{
    QSet<QAction *> actionPointers;

    QMenu* const contextMenu = m_ui.searchCombo->lineEdit()->createStandardContextMenu();
    if (!contextMenu) return;

    if (m_ui.useRegExp->isChecked()) {
        QMenu* menu = contextMenu->addMenu(i18n("Add..."));
        if (!menu) return;

        menu->setIcon(SmallIcon(QStringLiteral("list-add")));

        actionPointers << menuEntry(menu, QStringLiteral("^"), QStringLiteral(""), i18n("Beginning of line"));
        actionPointers << menuEntry(menu, QStringLiteral("$"), QStringLiteral(""), i18n("End of line"));
        menu->addSeparator();
        actionPointers << menuEntry(menu, QStringLiteral("."), QStringLiteral(""), i18n("Any single character (excluding line breaks)"));
        menu->addSeparator();
        actionPointers << menuEntry(menu, QStringLiteral("+"), QStringLiteral(""), i18n("One or more occurrences"));
        actionPointers << menuEntry(menu, QStringLiteral("*"), QStringLiteral(""), i18n("Zero or more occurrences"));
        actionPointers << menuEntry(menu, QStringLiteral("?"), QStringLiteral(""), i18n("Zero or one occurrences"));
        actionPointers << menuEntry(menu, QStringLiteral("{"), QStringLiteral(",}"), i18n("<a> through <b> occurrences"), QStringLiteral("{a"), QStringLiteral(",b}"));
        menu->addSeparator();
        actionPointers << menuEntry(menu, QStringLiteral("("), QStringLiteral(")"), i18n("Group, capturing"));
        actionPointers << menuEntry(menu, QStringLiteral("|"), QStringLiteral(""), i18n("Or"));
        actionPointers << menuEntry(menu, QStringLiteral("["), QStringLiteral("]"), i18n("Set of characters"));
        actionPointers << menuEntry(menu, QStringLiteral("[^"), QStringLiteral("]"), i18n("Negative set of characters"));
        actionPointers << menuEntry(menu, QStringLiteral("(?:"), QStringLiteral(")"), i18n("Group, non-capturing"), QStringLiteral("(?:E"));
        actionPointers << menuEntry(menu, QStringLiteral("(?="), QStringLiteral(")"), i18n("Lookahead"), QStringLiteral("(?=E"));
        actionPointers << menuEntry(menu, QStringLiteral("(?!"), QStringLiteral(")"), i18n("Negative lookahead"), QStringLiteral("(?!E"));

        menu->addSeparator();
        actionPointers << menuEntry(menu, QStringLiteral("\\n"), QStringLiteral(""), i18n("Line break"));
        actionPointers << menuEntry(menu, QStringLiteral("\\t"), QStringLiteral(""), i18n("Tab"));
        actionPointers << menuEntry(menu, QStringLiteral("\\b"), QStringLiteral(""), i18n("Word boundary"));
        actionPointers << menuEntry(menu, QStringLiteral("\\B"), QStringLiteral(""), i18n("Not word boundary"));
        actionPointers << menuEntry(menu, QStringLiteral("\\d"), QStringLiteral(""), i18n("Digit"));
        actionPointers << menuEntry(menu, QStringLiteral("\\D"), QStringLiteral(""), i18n("Non-digit"));
        actionPointers << menuEntry(menu, QStringLiteral("\\s"), QStringLiteral(""), i18n("Whitespace (excluding line breaks)"));
        actionPointers << menuEntry(menu, QStringLiteral("\\S"), QStringLiteral(""), i18n("Non-whitespace (excluding line breaks)"));
        actionPointers << menuEntry(menu, QStringLiteral("\\w"), QStringLiteral(""), i18n("Word character (alphanumerics plus '_')"));
        actionPointers << menuEntry(menu, QStringLiteral("\\W"), QStringLiteral(""), i18n("Non-word character"));
    }
    // Show menu
    QAction * const result = contextMenu->exec(m_ui.searchCombo->mapToGlobal(pos));

    // Act on action
    if (result && actionPointers.contains(result)) {
        QLineEdit * lineEdit = m_ui.searchCombo->lineEdit();
        const int cursorPos = lineEdit->cursorPosition();
        QStringList beforeAfter = result->data().toString().split(QLatin1Char(' '));
        if (beforeAfter.size() != 2) return;
        lineEdit->insert(beforeAfter[0] + beforeAfter[1]);
        lineEdit->setCursorPosition(cursorPos + beforeAfter[0].count());
        lineEdit->setFocus();
    }
}

void KatePluginSearchView::slotPluginViewCreated (const QString &name, QObject *pluginView)
{
    // add view
    if (name == QStringLiteral("kateprojectplugin")) {
        m_projectPluginView = pluginView;
        slotProjectFileNameChanged ();
        connect (pluginView, SIGNAL(projectFileNameChanged()), this, SLOT(slotProjectFileNameChanged()));
    }
}

void KatePluginSearchView::slotPluginViewDeleted (const QString &name, QObject *)
{
    // remove view
    if (name == QStringLiteral("kateprojectplugin")) {
        m_projectPluginView = 0;
        slotProjectFileNameChanged ();
    }
}

void KatePluginSearchView::slotProjectFileNameChanged ()
{
    // query new project file name
    QString projectFileName;
    if (m_projectPluginView)
        projectFileName = m_projectPluginView->property("projectFileName").toString();

    // have project, enable gui for it
    if (!projectFileName.isEmpty()) {
        if (m_ui.searchPlaceCombo->count() < 3) {
            // add "in Project"
            m_ui.searchPlaceCombo->addItem (SmallIcon(QStringLiteral("project-open")), i18n("in Project"));
            if (m_switchToProjectModeWhenAvailable) {
                // switch to search "in Project"
                m_switchToProjectModeWhenAvailable = false;
                setSearchPlace (2);
            }
        }
    }

    // else: disable gui for it
    else {
        if (m_ui.searchPlaceCombo->count() > 2) {
            // switch to search "in Open files", if "in Project" is active
            if (m_ui.searchPlaceCombo->currentIndex () == 2)
                setSearchPlace (0);

            // remove "in Project"
            m_ui.searchPlaceCombo->removeItem (2);
        }
    }
}

KateSearchCommand::KateSearchCommand(QObject *parent)
: KTextEditor::Command(QStringList() << QStringLiteral("grep") << QStringLiteral("newGrep")
        << QStringLiteral("search") << QStringLiteral("newSearch")
        << QStringLiteral("pgrep") << QStringLiteral("newPGrep"), parent)
{
}

bool KateSearchCommand::exec (KTextEditor::View* /*view*/, const QString& cmd, QString& /*msg*/, const KTextEditor::Range &)
{
    //create a list of args
    QStringList args(cmd.split(QLatin1Char(' '), QString::KeepEmptyParts));
    QString command = args.takeFirst();
    QString searchText = args.join(QLatin1Char(' '));

    if (command == QStringLiteral("grep") || command == QStringLiteral("newGrep")) {
        emit setSearchPlace(1);
        emit setCurrentFolder();
        if (command == QStringLiteral("newGrep"))
            emit newTab();
    }
    
    else if (command == QStringLiteral("search") || command == QStringLiteral("newSearch")) {
        emit setSearchPlace(0);
        if (command == QStringLiteral("newSearch"))
            emit newTab();
    }
    
    else if (command == QStringLiteral("pgrep") || command == QStringLiteral("newPGrep")) {
        emit setSearchPlace(2);
        if (command == QStringLiteral("newPGrep"))
            emit newTab();
    }
    
    emit setSearchString(searchText);
    emit startSearch();

    return true;
}

bool KateSearchCommand::help (KTextEditor::View */*view*/, const QString &cmd, QString & msg)
{
    if (cmd.startsWith(QStringLiteral("grep"))) {
        msg = i18n("Usage: grep [pattern to search for in folder]");
    }
    else if (cmd.startsWith(QStringLiteral("newGrep"))) {
        msg = i18n("Usage: newGrep [pattern to search for in folder]");
    }

    else if (cmd.startsWith(QStringLiteral("search"))) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    }
    else if (cmd.startsWith(QStringLiteral("newSearch"))) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    }
    
    else if (cmd.startsWith(QStringLiteral("pgrep"))) {
        msg = i18n("Usage: pgrep [pattern to search for in current project]");
    }
    else if (cmd.startsWith(QStringLiteral("newPGrep"))) {
        msg = i18n("Usage: newPGrep [pattern to search for in current project]");
    }
    
    return true;
}

#include "plugin_search.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
