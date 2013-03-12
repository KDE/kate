/*   Kate search plugin
 *
 * Copyright (C) 2011-2012 by Kåre Särs <kare.sars@iki.fi>
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
#include "plugin_search.moc"

#include "htmldelegate.h"

#include <kate/application.h>
#include <ktexteditor/editor.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kate/documentmanager.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/configinterface.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kurlcompletion.h>
#include <klineedit.h>
#include <kcolorscheme.h>

#include <QKeyEvent>
#include <QClipboard>
#include <QMenu>
#include <QTextDocument>
#include <QScrollBar>

static QAction *menuEntry(QMenu *menu,
                          const QString &before, const QString &after, const QString &desc,
                          QString menuBefore = QString(), QString menuAfter = QString());

static QAction *menuEntry(QMenu *menu,
                          const QString &before, const QString &after, const QString &desc,
                          QString menuBefore, QString menuAfter)
{
    if (menuBefore.isEmpty()) menuBefore = before;
    if (menuAfter.isEmpty())  menuAfter = after;

    QAction *const action = menu->addAction(menuBefore + menuAfter + '\t' + desc);
    if (!action) return 0;

    action->setData(QString(before + ' ' + after));
    return action;
}

Results::Results(QWidget *parent): QWidget(parent), matches(0)
{
    setupUi(this);

    tree->setItemDelegate(new SPHtmlDelegate(tree));
    selectAllCB->setText(i18n("Select all 9999 matches"));
    selectAllCB->setFixedWidth(selectAllCB->sizeHint().width());
    selectAllCB->setText(i18n("Select all"));
    buttonContainer->setDisabled(true);

    connect(selectAllCB, SIGNAL(clicked(bool)), this, SLOT(selectAll(bool)));
}

void Results::selectAll(bool)
{
    disconnect(tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(checkCheckedState()));
    Qt::CheckState state = selectAllCB->checkState();
    if (state == Qt::PartiallyChecked) state = Qt::Checked;
    selectAllCB->setCheckState(state);
    for (int i=0; i<tree->topLevelItemCount(); i++) {
        tree->topLevelItem(i)->setCheckState(0, state);
    }
    connect(tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(checkCheckedState()));
}

void Results::checkCheckedState()
{
    Qt::CheckState state;
    for (int i=0; i<tree->topLevelItemCount(); i++) {
        if (i==0) {
            state = tree->topLevelItem(i)->checkState(0);
        }
        else if (state != tree->topLevelItem(i)->checkState(0)) {
            selectAllCB->setCheckState(Qt::PartiallyChecked);
            return;
        }
    }
    selectAllCB->setCheckState(state);
}


K_PLUGIN_FACTORY(KatePluginSearchFactory, registerPlugin<KatePluginSearch>();)
K_EXPORT_PLUGIN(KatePluginSearchFactory(KAboutData("katesearch","katesearch",ki18n("Search & Replace"), "0.1", ki18n("Search & replace in files"))))

KatePluginSearch::KatePluginSearch(QObject* parent, const QList<QVariant>&)
    : Kate::Plugin((Kate::Application*)parent, "kate-search-plugin"),
    m_searchCommand(0)
{
    KGlobal::locale()->insertCatalog("katesearch");

    KTextEditor::CommandInterface* iface =
    qobject_cast<KTextEditor::CommandInterface*>(Kate::application()->editor());
    if (iface) {
        m_searchCommand = new KateSearchCommand(this);
        iface->registerCommand(m_searchCommand);
    }
}

KatePluginSearch::~KatePluginSearch()
{
    KTextEditor::CommandInterface* iface =
    qobject_cast<KTextEditor::CommandInterface*>(Kate::application()->editor());
    if (iface && m_searchCommand) {
        iface->unregisterCommand(m_searchCommand);
    }
}

Kate::PluginView *KatePluginSearch::createView(Kate::MainWindow *mainWindow)
{
    KatePluginSearchView *view = new KatePluginSearchView(mainWindow, application());
    connect(m_searchCommand, SIGNAL(setSearchPlace(int)), view, SLOT(setSearchPlace(int)));
    connect(m_searchCommand, SIGNAL(setCurrentFolder()), view, SLOT(setCurrentFolder()));
    connect(m_searchCommand, SIGNAL(setSearchString(QString)), view, SLOT(setSearchString(QString)));
    connect(m_searchCommand, SIGNAL(startSearch()), view, SLOT(startSearch()));
    connect(m_searchCommand, SIGNAL(newTab()), view, SLOT(addTab()));

    return view;
}


KatePluginSearchView::KatePluginSearchView(Kate::MainWindow *mainWin, Kate::Application* application)
: Kate::PluginView(mainWin),
Kate::XMLGUIClient(KatePluginSearchFactory::componentData()),
m_kateApp(application),
m_curResults(0),
m_searchJustOpened(false),
m_projectPluginView(0)
{
    KAction *a = actionCollection()->addAction("search_in_files");
    a->setText(i18n("Search in Files"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(openSearchView()));

    a = actionCollection()->addAction("go_to_next_match");
    a->setText(i18n("Go to Next Match"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(goToNextMatch()));

    a = actionCollection()->addAction("go_to_prev_match");
    a->setText(i18n("Go to Previous Match"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(goToPreviousMatch()));

    m_toolView = mainWin->createToolView ("kate_plugin_katesearch",
                                          Kate::MainWindow::Bottom,
                                          SmallIcon("edit-find"),
                                          i18n("Search and Replace"));
    QWidget *container = new QWidget(m_toolView);
    m_ui.setupUi(container);
    container->setFocusProxy(m_ui.searchCombo);

    m_ui.resultTabWidget->tabBar()->setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);

    m_ui.displayOptions->setIcon(KIcon("arrow-down-double"));
    m_ui.searchButton->setIcon(KIcon("edit-find"));
    m_ui.stopButton->setIcon(KIcon("process-stop"));
    m_ui.searchPlaceCombo->setItemIcon(0, KIcon("text-plain"));
    m_ui.searchPlaceCombo->setItemIcon(1, KIcon("folder"));
    m_ui.folderUpButton->setIcon(KIcon("go-up"));
    m_ui.currentFolderButton->setIcon(KIcon("view-refresh"));
    m_ui.newTabButton->setIcon(KIcon("tab-new"));

    m_ui.filterCombo->setToolTip(i18n("Comma separated list of file types to search in. example: \"*.cpp,*.h\"\n"
    "NOTE: Put a minus sign ('-') in front of the list to make it into an exclude list. example: \"-*.o,*.obj\""));

    int padWidth = m_ui.folderLabel->sizeHint().width();
    padWidth = qMax(padWidth, m_ui.filterLabel->sizeHint().width());
    m_ui.topLayout->setColumnMinimumWidth(0, padWidth);
    m_ui.topLayout->setAlignment(m_ui.newTabButton, Qt::AlignHCenter);
    m_ui.optionsLayout->setColumnMinimumWidth(0, padWidth);

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
    connect(m_ui.resultTabWidget,  SIGNAL(closeRequest(QWidget*)), this, SLOT(closeTab(QWidget*)));
    connect(m_ui.resultTabWidget,  SIGNAL(currentChanged(int)), this, SLOT(resultTabChanged(int)));
    connect(m_ui.searchButton,     SIGNAL(clicked()), this, SLOT(startSearch()));
    connect(m_ui.searchCombo,      SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.folderRequester,  SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.folderUpButton,   SIGNAL(clicked()), this, SLOT(navigateFolderUp()));
    connect(m_ui.currentFolderButton, SIGNAL(clicked()), this, SLOT(setCurrentFolder()));

    connect(m_ui.filterCombo,      SIGNAL(returnPressed()), this, SLOT(startSearch()));

    connect(m_ui.displayOptions,   SIGNAL(toggled(bool)), this, SLOT(toggleOptions(bool)));
    connect(m_ui.searchPlaceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(searchPlaceChanged()));
    connect(m_ui.searchCombo,      SIGNAL(editTextChanged(QString)), this, SLOT(searchPatternChanged()));
    connect(m_ui.matchCase,        SIGNAL(stateChanged(int)), this, SLOT(searchPatternChanged()));
    connect(m_ui.useRegExp,        SIGNAL(stateChanged(int)), this, SLOT(searchPatternChanged()));
    connect(m_ui.stopButton,       SIGNAL(clicked()), &m_searchOpenFiles, SLOT(cancelSearch()));
    connect(m_ui.stopButton,       SIGNAL(clicked()), &m_searchFolder, SLOT(cancelSearch()));
    connect(m_ui.stopButton,       SIGNAL(clicked()), &m_searchProject, SLOT(cancelSearch()));

    m_ui.displayOptions->setChecked(true);

    connect(&m_searchOpenFiles, SIGNAL(matchFound(QString,int,int,QString,int)),
            this,                 SLOT(matchFound(QString,int,int,QString,int)));
    connect(&m_searchOpenFiles, SIGNAL(searchDone()),  this, SLOT(searchDone()));

    connect(&m_searchFolder, SIGNAL(matchFound(QString,int,int,QString,int)),
            this,              SLOT(matchFound(QString,int,int,QString,int)));
    connect(&m_searchFolder, SIGNAL(searchDone()),  this, SLOT(searchDone()));

    connect(&m_searchProject, SIGNAL(matchFound(QString,int,int,QString,int)),
            this,              SLOT(matchFound(QString,int,int,QString,int)));
    connect(&m_searchProject, SIGNAL(searchDone()),  this, SLOT(searchDone()));

    connect(&m_searchWhileTyping, SIGNAL(matchFound(QString,int,int,QString,int)),
            this,              SLOT(matchFound(QString,int,int,QString,int)));
    connect(&m_searchWhileTyping, SIGNAL(searchDone()),  this, SLOT(searchWhileTypingDone()));

    connect(m_kateApp->documentManager(), SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
            &m_searchOpenFiles, SLOT(cancelSearch()));

    connect(m_kateApp->documentManager(), SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
            &m_replacer, SLOT(cancelReplace()));

    connect(m_kateApp->documentManager(), SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
            this, SLOT(clearDocMarks(KTextEditor::Document*)));

    connect(&m_replacer, SIGNAL(matchReplaced(KTextEditor::Document*,int,int,int)),
            this, SLOT(addMatchMark(KTextEditor::Document*,int,int,int)));

    // Hook into line edit context menus
    m_ui.searchCombo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.searchCombo, SIGNAL(customContextMenuRequested(QPoint)), this,
            SLOT(searchContextMenu(QPoint)));

    connect(mainWindow(), SIGNAL(unhandledShortcutOverride(QEvent*)),
            this, SLOT(handleEsc(QEvent*)));

    // watch for project plugin view creation/deletion
    connect(mainWindow(), SIGNAL(pluginViewCreated (const QString &, Kate::PluginView *))
        , this, SLOT(slotPluginViewCreated (const QString &, Kate::PluginView *)));

    connect(mainWindow(), SIGNAL(pluginViewDeleted (const QString &, Kate::PluginView *))
        , this, SLOT(slotPluginViewDeleted (const QString &, Kate::PluginView *)));

    // update once project plugin state manually
    m_projectPluginView = mainWindow()->pluginView ("kateprojectplugin");
    slotProjectFileNameChanged ();

    m_replacer.setDocumentManager(m_kateApp->documentManager());

    searchPlaceChanged();

    m_toolView->installEventFilter(this);

    mainWindow()->guiFactory()->addClient(this);
}

KatePluginSearchView::~KatePluginSearchView()
{
    clearMarks();

    mainWindow()->guiFactory()->removeClient(this);
    delete m_toolView;
}

void KatePluginSearchView::navigateFolderUp()
{
    // navigate one folder up
    m_ui.folderRequester->setUrl(m_ui.folderRequester->url().upUrl());
}

void KatePluginSearchView::setCurrentFolder()
{
    if (!mainWindow()) {
        return;
    }
    KTextEditor::View* editView = mainWindow()->activeView();
    if (editView && editView->document()) {
        // upUrl as we want the folder not the file
        m_ui.folderRequester->setUrl(editView->document()->url().upUrl());
    }
}

void KatePluginSearchView::openSearchView()
{
    if (!mainWindow()) {
        return;
    }
    if (!m_toolView->isVisible()) {
        mainWindow()->showToolView(m_toolView);
    }
    m_ui.searchCombo->setFocus(Qt::OtherFocusReason);
    m_ui.displayOptions->setChecked(true);

    KTextEditor::View* editView = mainWindow()->activeView();
    if (editView && editView->document()) {
        if (m_ui.folderRequester->text().isEmpty()) {
            // upUrl as we want the folder not the file
            m_ui.folderRequester->setUrl(editView->document()->url().upUrl());
        }
        if (editView->selection()) {
            QString selection = editView->selectionText();
            // remove possible trailing '\n'
            if (selection.endsWith('\n')) {
                selection = selection.left(selection.size() -1);
            }
            if (!selection.isEmpty() && !selection.contains('\n')) {
                m_ui.searchCombo->blockSignals(true);
                m_ui.searchCombo->lineEdit()->setText(selection);
                m_ui.searchCombo->blockSignals(false);
            }
        }
        m_ui.searchCombo->lineEdit()->selectAll();
        m_searchJustOpened = true;
        searchPatternChanged();
    }
}

void KatePluginSearchView::handleEsc(QEvent *e)
{
    if (!mainWindow()) return;

    QKeyEvent *k = static_cast<QKeyEvent *>(e);
    if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {

        if (m_toolView->isVisible()) {
            mainWindow()->hideToolView(m_toolView);
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

void KatePluginSearchView::startSearch()
{
    mainWindow()->showToolView(m_toolView); // in case we are invoked from the command interface

    if (m_ui.searchCombo->currentText().isEmpty()) {
        // return pressed in the folder combo or filter combo
        return;
    }
    m_ui.searchCombo->addToHistory(m_ui.searchCombo->currentText());
    if(m_ui.filterCombo->findText(m_ui.filterCombo->currentText()) == -1) {
        m_ui.filterCombo->insertItem(0, m_ui.filterCombo->currentText());
        m_ui.filterCombo->setCurrentIndex(0);
    }
    m_curResults = qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        kWarning() << "This is a bug";
        return;
    }

    m_ui.newTabButton->setDisabled(true);
    m_ui.searchCombo->setDisabled(true);
    m_ui.searchButton->setDisabled(true);
    m_ui.locationAndStop->setCurrentIndex(1);
    m_ui.displayOptions->setChecked (false);
    m_ui.displayOptions->setDisabled(true);

    QRegExp reg(m_ui.searchCombo->currentText(),
                m_ui.matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                m_ui.useRegExp->isChecked() ? QRegExp::RegExp : QRegExp::FixedString);
    m_curResults->regExp = reg;

    clearMarks();
    m_curResults->tree->clear();
    m_curResults->buttonContainer->setEnabled(false);
    m_curResults->matches = 0;
    m_curResults->selectAllCB->setText(i18n("Select all"));
    m_curResults->selectAllCB->setChecked(true);
    disconnect(m_curResults->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), m_curResults, SLOT(checkCheckedState()));

    m_ui.resultTabWidget->setTabText(m_ui.resultTabWidget->currentIndex(),
                                     m_ui.searchCombo->currentText());

    if (m_ui.searchPlaceCombo->currentIndex() ==  0) {
        m_searchOpenFiles.startSearch(m_kateApp->documentManager()->documents(), reg);
    }
    else if (m_ui.searchPlaceCombo->currentIndex() == 1) {
        m_searchFolder.startSearch(m_ui.folderRequester->text(),
                                   m_ui.recursiveCheckBox->isChecked(),
                                   m_ui.hiddenCheckBox->isChecked(),
                                   m_ui.symLinkCheckBox->isChecked(),
                                   m_ui.binaryCheckBox->isChecked(),
                                   m_ui.filterCombo->currentText(),
                                   reg);
    } else {
        /**
         * init search with file list from current project, if any
         */
        QStringList files;
        if (m_projectPluginView)
            files = m_projectPluginView->property ("projectFiles").toStringList();
        m_searchProject.startSearch(files, reg);
    }
    m_toolView->setCursor(Qt::WaitCursor);

    m_curResults->matches = 0;
}

void KatePluginSearchView::toggleOptions(bool show)
{
    m_ui.stackedWidget->setCurrentIndex((show) ? 1:0);
}

void KatePluginSearchView::setSearchPlace(int place)
{
    m_ui.searchPlaceCombo->setCurrentIndex(place);
}

void KatePluginSearchView::searchPlaceChanged()
{
    bool needFolderOptions = (m_ui.searchPlaceCombo->currentIndex() == 1);

    if (needFolderOptions)
        m_ui.displayOptions->setChecked(true);

    m_ui.folderOptions->setDisabled(!needFolderOptions);
}

void KatePluginSearchView::searchPatternChanged()
{
    m_ui.searchButton->setDisabled(m_ui.searchCombo->currentText().isEmpty());

    if (m_ui.searchCombo->currentText().length() < 3) return;
    if (!mainWindow()->activeView()) return;

    KTextEditor::Document *doc = mainWindow()->activeView()->document();
    if (!doc) return;

    m_curResults =qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        kWarning() << "This is a bug";
        return;
    }

    QRegExp reg(m_ui.searchCombo->currentText(),
                m_ui.matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                m_ui.useRegExp->isChecked() ? QRegExp::RegExp : QRegExp::FixedString);

    m_curResults->regExp = reg;

    clearMarks();
    m_curResults->tree->clear();
    m_curResults->buttonContainer->setEnabled(false);
    m_curResults->matches = 0;
    m_curResults->selectAllCB->setText(i18n("Select all"));
    m_curResults->selectAllCB->setChecked(true);
    disconnect(m_curResults->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), m_curResults, SLOT(checkCheckedState()));

    m_searchWhileTyping.startSearch(doc, reg);
}

QTreeWidgetItem * KatePluginSearchView::rootFileItem(const QString &url)
{
    if (!m_curResults) {
        return 0;
    }

    KUrl kurl(url);
    QString path = kurl.isLocalFile() ? kurl.upUrl().path() : kurl.upUrl().url();
    QString name = kurl.fileName();

    for (int i=0; i<m_curResults->tree->topLevelItemCount(); i++) {
        if (m_curResults->tree->topLevelItem(i)->data(0, Qt::UserRole).toString() == url) {
            int matches = m_curResults->tree->topLevelItem(i)->data(1, Qt::UserRole).toInt() + 1;
            QString tmpUrl = QString("%1<b>%2</b>: <b>%3</b>").arg(path).arg(name).arg(matches);
            m_curResults->tree->topLevelItem(i)->setData(0, Qt::DisplayRole, tmpUrl);
            m_curResults->tree->topLevelItem(i)->setData(1, Qt::UserRole, matches);
            return m_curResults->tree->topLevelItem(i);
        }
    }
    // file item not found create a new one
    QString tmpUrl = QString("%1<b>%2</b>: <b>%3</b>").arg(path).arg(name).arg(1);

    QTreeWidgetItem *item = new QTreeWidgetItem(m_curResults->tree, QStringList(tmpUrl));
    item->setData(0, Qt::UserRole, url);
    item->setData(1, Qt::UserRole, 1);
    item->setCheckState (0, Qt::Checked);
    item->setFlags(item->flags() | Qt::ItemIsTristate);
    return item;
}

void KatePluginSearchView::addMatchMark(KTextEditor::Document* doc, int line, int column, int matchLen)
{
    if (!doc) return;

    KTextEditor::MovingInterface* miface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    KTextEditor::ConfigInterface* ciface = qobject_cast<KTextEditor::ConfigInterface*>(mainWindow()->activeView());
    KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());
    if (sender() == &m_replacer) {
        QColor replaceColor(Qt::green);
        if (ciface) replaceColor = ciface->configValue("replace-highlight-color").value<QColor>();
        attr->setBackground(replaceColor);
    }
    else {
        QColor searchColor(Qt::yellow);
        if (ciface) searchColor = ciface->configValue("search-highlight-color").value<QColor>();
        attr->setBackground(searchColor);
    }
    KTextEditor::Range range(line, column, line, column+matchLen);
    KTextEditor::MovingRange* mr = miface->newMovingRange(range);
    mr->setAttribute(attr);
    mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
    mr->setAttributeOnlyForViews(true);
    m_matchRanges.append(mr);

    KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
    if (!iface) return;
    iface->setMarkDescription(KTextEditor::MarkInterface::markType32, i18n("SearchHighLight"));
    iface->setMarkPixmap(KTextEditor::MarkInterface::markType32,
                         KIcon().pixmap(0,0));
    iface->addMark(line, KTextEditor::MarkInterface::markType32);

    connect(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            this, SLOT(clearMarks()), Qt::UniqueConnection);
}

void KatePluginSearchView::matchFound(const QString &url, int line, int column,
                                      const QString &lineContent, int matchLen)
{
    if (!m_curResults) {
        return;
    }
    QString pre = Qt::escape(lineContent.left(column));
    QString match = Qt::escape(lineContent.mid(column, matchLen));
    QString post = Qt::escape(lineContent.mid(column + matchLen));
    QStringList row;
    row << i18n("Line: <b>%1</b>: %2", line+1, pre+"<b>"+match+"</b>"+post);

    QTreeWidgetItem *item = new QTreeWidgetItem(rootFileItem(url), row);
    item->setData(0, Qt::UserRole, url);
    item->setData(0, Qt::ToolTipRole, url);
    item->setData(1, Qt::UserRole, line);
    item->setData(2, Qt::UserRole, column);
    item->setData(3, Qt::UserRole, matchLen);
    item->setData(1, Qt::ToolTipRole, pre);
    item->setData(2, Qt::ToolTipRole, match);
    item->setData(3, Qt::ToolTipRole, post);
    item->setCheckState (0, Qt::Checked);

    m_curResults->matches++;
    m_curResults->selectAllCB->setText(i18np("Select %1 match",
                                            "Select all %1 matches",
                                            m_curResults->matches));

    // Add mark if the document is open
    KTextEditor::Document* doc = m_kateApp->documentManager()->findUrl(url);
    addMatchMark(doc, line, column, matchLen);
}

void KatePluginSearchView::clearMarks()
{
    // FIXME: check for ongoing search...
    KTextEditor::MarkInterface* iface;
    foreach (KTextEditor::Document* doc, m_kateApp->documentManager()->documents()) {
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
    //kDebug() << sender();
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
            //kDebug() << "removing mark in" << doc->url();
            delete m_matchRanges.at(i);
            m_matchRanges.removeAt(i);
        }
        else {
            i++;
        }
    }
}


void KatePluginSearchView::searchDone()
{
    m_ui.newTabButton->setDisabled(false);
    m_ui.searchCombo->setDisabled(false);
    m_ui.searchButton->setDisabled(false);
    m_ui.locationAndStop->setCurrentIndex(0);
    m_ui.displayOptions->setDisabled(false);

    if (!m_curResults) {
        return;
    }
    if (m_curResults->tree->topLevelItemCount() > 0) {
        m_curResults->tree->setCurrentItem(m_curResults->tree->topLevelItem(0));
        m_curResults->setFocus(Qt::OtherFocusReason);
    }
    m_curResults->tree->expandAll();
    m_curResults->tree->resizeColumnToContents(0);
    if (m_curResults->tree->verticalScrollBar()->maximum() >
        3*m_curResults->tree->verticalScrollBar()->pageStep())
    {
        m_curResults->tree->collapseAll();
    }
    m_curResults->buttonContainer->setEnabled(true);

    connect(m_curResults->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), m_curResults, SLOT(checkCheckedState()));

    indicateMatch(m_curResults->tree->topLevelItemCount() > 0);
    m_curResults = 0;
    m_toolView->unsetCursor();
    m_searchJustOpened = false;
}

void KatePluginSearchView::searchWhileTypingDone()
{
    if (!m_curResults) {
        return;
    }

    m_curResults->tree->resizeColumnToContents(0);
    m_curResults->buttonContainer->setEnabled(true);

    connect(m_curResults->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), m_curResults, SLOT(checkCheckedState()));

    if (!m_searchJustOpened && (m_curResults->tree->topLevelItemCount() > 0)) {
        itemSelected(m_curResults->tree->topLevelItem(0));
    }
    indicateMatch(m_curResults->tree->topLevelItemCount() > 0);
    m_curResults = 0;
    m_ui.searchCombo->lineEdit()->setFocus();
    m_searchJustOpened = false;
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

void KatePluginSearchView::itemSelected(QTreeWidgetItem *item)
{
    if (!item) return;

    if ((item->parent()==0) && (item->child(0))) {
        item->treeWidget()->expandItem(item);
        item = item->child(0);
        item->treeWidget()->setCurrentItem(item);
    }

    // get stuff
    const QString url = item->data(0, Qt::UserRole).toString();
    if (url.isEmpty()) return;
    int toLine = item->data(1, Qt::UserRole).toInt();
    int toColumn = item->data(2, Qt::UserRole).toInt();

    // add the marks to the document if it is not already open
    KTextEditor::Document* doc = m_kateApp->documentManager()->findUrl(url);
    if (!doc) {
        doc = m_kateApp->documentManager()->openUrl(url);
        if (doc) {
            int line;
            int column;
            int len;
            QTreeWidgetItem *rootItem = (item->parent()==0) ? item : item->parent();
            for (int i=0; i<rootItem->childCount(); i++) {
                item = rootItem->child(i);
                line = item->data(1, Qt::UserRole).toInt();
                column = item->data(2, Qt::UserRole).toInt();
                len = item->data(3, Qt::UserRole).toInt();
                addMatchMark(doc, line, column, len);
            }
        }
    }
    // open the right view...
    mainWindow()->openUrl(url);

    // any view active?
    if (!mainWindow()->activeView()) {
        return;
    }

    // set the cursor to the correct position
    mainWindow()->activeView()->setCursorPosition(KTextEditor::Cursor(toLine, toColumn));
    mainWindow()->activeView()->setFocus();
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

    if (curr->parent() == 0) {
        res->tree->expandItem(curr);
    }
    curr = res->tree->itemBelow(curr);
    if (!curr) {
        curr = res->tree->topLevelItem(0);
    }
    if (curr->parent() == 0) {
        res->tree->expandItem(curr);
        curr = res->tree->itemBelow(curr);
    }
    if (!curr) return;

    res->tree->setCurrentItem(curr);
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
    if (!curr) {
        // select the last child of the last top-level item
        curr = res->tree->topLevelItem(res->tree->topLevelItemCount()-1);
        curr = curr->child(curr->childCount()-1);
        if (!curr) return;
        res->tree->setCurrentItem(curr);
        itemSelected(curr);
        return;
    }

    curr = res->tree->itemAbove(curr);
    if (!curr) {
        // current was the first top-level item
        res->tree->setCurrentItem(curr);
        goToPreviousMatch();
        return;
    }

    if (curr->parent() == 0) {
        // this is a top-level item -> go to the item above
        curr = res->tree->itemAbove(curr);
        if (!curr) {
            res->tree->setCurrentItem(curr);
            goToPreviousMatch();
            return;
        }
    }

    if (curr->parent() == 0) {
        // still a top-level item -> expand and take the last
        res->tree->expandItem(curr);
        curr = curr->child(curr->childCount()-1);
        if (!curr) return;
    }
    res->tree->setCurrentItem(curr);
    itemSelected(curr);
}

void KatePluginSearchView::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":search-plugin");
    m_ui.searchCombo->clearHistory();
    m_ui.searchCombo->setHistoryItems(cg.readEntry("Search", QStringList()), true);
    m_ui.matchCase->setChecked(cg.readEntry("MatchCase", false));
    m_ui.useRegExp->setChecked(cg.readEntry("UseRegExp", false));

    m_ui.searchPlaceCombo->setCurrentIndex(cg.readEntry("Place", 0));
    m_ui.recursiveCheckBox->setChecked(cg.readEntry("Recursive", true));
    m_ui.hiddenCheckBox->setChecked(cg.readEntry("HiddenFiles", false));
    m_ui.symLinkCheckBox->setChecked(cg.readEntry("FollowSymLink", false));
    m_ui.binaryCheckBox->setChecked(cg.readEntry("BinaryFiles", false));
    m_ui.folderRequester->comboBox()->clear();
    m_ui.folderRequester->comboBox()->addItems(cg.readEntry("SearchFolders", QStringList()));
    m_ui.folderRequester->setText(cg.readEntry("SearchFolder", QString()));
    m_ui.filterCombo->clear();
    m_ui.filterCombo->addItems(cg.readEntry("Filters", QStringList()));
    m_ui.filterCombo->setCurrentIndex(cg.readEntry("CurrentFilter", 0));
}

void KatePluginSearchView::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":search-plugin");
    cg.writeEntry("Search", m_ui.searchCombo->historyItems());
    cg.writeEntry("MatchCase", m_ui.matchCase->isChecked());
    cg.writeEntry("UseRegExp", m_ui.useRegExp->isChecked());

    cg.writeEntry("Place", m_ui.searchPlaceCombo->currentIndex());
    cg.writeEntry("Recursive", m_ui.recursiveCheckBox->isChecked());
    cg.writeEntry("HiddenFiles", m_ui.hiddenCheckBox->isChecked());
    cg.writeEntry("FollowSymLink", m_ui.symLinkCheckBox->isChecked());
    cg.writeEntry("BinaryFiles", m_ui.binaryCheckBox->isChecked());
    QStringList folders;
    for (int i=0; i<qMin(m_ui.folderRequester->comboBox()->count(), 10); i++) {
        folders << m_ui.folderRequester->comboBox()->itemText(i);
    }
    cg.writeEntry("SearchFolders", folders);
    cg.writeEntry("SearchFolder", m_ui.folderRequester->text());
    QStringList filterItems;
    for (int i=0; i<qMin(m_ui.filterCombo->count(), 10); i++) {
        filterItems << m_ui.filterCombo->itemText(i);
    }
    cg.writeEntry("Filters", filterItems);
    cg.writeEntry("CurrentFilter", m_ui.filterCombo->currentIndex());
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

    connect(res->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this,      SLOT  (itemSelected(QTreeWidgetItem*)), Qt::QueuedConnection);

    connect(res->replaceButton, SIGNAL(clicked(bool)), this, SLOT(replaceChecked()));
    connect(res->replaceCombo,  SIGNAL(returnPressed()), this, SLOT(replaceChecked()));
    connect(&m_replacer,        SIGNAL(replaceDone()), this, SLOT(replaceDone()));

    m_ui.resultTabWidget->addTab(res, "");
    m_ui.resultTabWidget->setCurrentIndex(m_ui.resultTabWidget->count()-1);
    m_ui.stackedWidget->setCurrentIndex(0);
    m_ui.resultTabWidget->tabBar()->show();

    res->tree->installEventFilter(this);
}

void KatePluginSearchView::closeTab(QWidget *widget)
{
    Results *tmp = qobject_cast<Results *>(widget);
    if (m_curResults == tmp) {
        m_searchOpenFiles.cancelSearch();
        m_searchFolder.cancelSearch();
        m_searchProject.cancelSearch();
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

    m_ui.searchCombo->lineEdit()->setText(m_ui.resultTabWidget->tabText(index));
    m_ui.matchCase->setChecked(res->regExp.caseSensitivity() == Qt::CaseSensitive);
    m_ui.useRegExp->setChecked(res->regExp.patternSyntax() != QRegExp::FixedString);
}


bool KatePluginSearchView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        QTreeWidget *tree = qobject_cast<QTreeWidget *>(obj);
        if (tree) {
            if (ke->matches(QKeySequence::Copy)) {
                // user pressed ctrl+c -> copy full URL to the clipboard
                QVariant variant = tree->currentItem()->data(0, Qt::UserRole);
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
            mainWindow()->hideToolView(m_toolView);
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

        menu->setIcon(KIcon("list-add"));

        actionPointers << menuEntry(menu, "^", "", i18n("Beginning of line"));
        actionPointers << menuEntry(menu, "$", "", i18n("End of line"));
        menu->addSeparator();
        actionPointers << menuEntry(menu, ".", "", i18n("Any single character (excluding line breaks)"));
        menu->addSeparator();
        actionPointers << menuEntry(menu, "+", "", i18n("One or more occurrences"));
        actionPointers << menuEntry(menu, "*", "", i18n("Zero or more occurrences"));
        actionPointers << menuEntry(menu, "?", "", i18n("Zero or one occurrences"));
        actionPointers << menuEntry(menu, "{", ",}", i18n("<a> through <b> occurrences"), "{a", ",b}");
        menu->addSeparator();
        actionPointers << menuEntry(menu, "(", ")", i18n("Group, capturing"));
        actionPointers << menuEntry(menu, "|", "", i18n("Or"));
        actionPointers << menuEntry(menu, "[", "]", i18n("Set of characters"));
        actionPointers << menuEntry(menu, "[^", "]", i18n("Negative set of characters"));
        actionPointers << menuEntry(menu, "(?:", ")", i18n("Group, non-capturing"), "(?:E");
        actionPointers << menuEntry(menu, "(?=", ")", i18n("Lookahead"), "(?=E");
        actionPointers << menuEntry(menu, "(?!", ")", i18n("Negative lookahead"), "(?!E");

        menu->addSeparator();
        actionPointers << menuEntry(menu, "\\n", "", i18n("Line break"));
        actionPointers << menuEntry(menu, "\\t", "", i18n("Tab"));
        actionPointers << menuEntry(menu, "\\b", "", i18n("Word boundary"));
        actionPointers << menuEntry(menu, "\\B", "", i18n("Not word boundary"));
        actionPointers << menuEntry(menu, "\\d", "", i18n("Digit"));
        actionPointers << menuEntry(menu, "\\D", "", i18n("Non-digit"));
        actionPointers << menuEntry(menu, "\\s", "", i18n("Whitespace (excluding line breaks)"));
        actionPointers << menuEntry(menu, "\\S", "", i18n("Non-whitespace (excluding line breaks)"));
        actionPointers << menuEntry(menu, "\\w", "", i18n("Word character (alphanumerics plus '_')"));
        actionPointers << menuEntry(menu, "\\W", "", i18n("Non-word character"));
    }
    // Show menu
    QAction * const result = contextMenu->exec(m_ui.searchCombo->mapToGlobal(pos));

    // Act on action
    if (result && actionPointers.contains(result)) {
        QLineEdit * lineEdit = m_ui.searchCombo->lineEdit();
        const int cursorPos = lineEdit->cursorPosition();
        QStringList beforeAfter = result->data().toString().split(' ');
        if (beforeAfter.size() != 2) return;
        lineEdit->insert(beforeAfter[0] + beforeAfter[1]);
        lineEdit->setCursorPosition(cursorPos + beforeAfter[0].count());
        lineEdit->setFocus();
    }
}

void KatePluginSearchView::replaceChecked()
{
    m_curResults =qobject_cast<Results *>(m_ui.resultTabWidget->currentWidget());
    if (!m_curResults) {
        kWarning() << "Results not found";
        return;
    }

    if(m_curResults->replaceCombo->findText(m_curResults->replaceCombo->currentText()) == -1) {
        m_curResults->replaceCombo->insertItem(0, m_curResults->replaceCombo->currentText());
        m_curResults->replaceCombo->setCurrentIndex(0);
    }

    m_replacer.replaceChecked(m_curResults->tree,
                              m_curResults->regExp,
                              m_curResults->replaceCombo->currentText());
}

void KatePluginSearchView::replaceDone()
{
    if (m_curResults) {
        m_curResults->buttonStack->setCurrentIndex(0);
        m_curResults->replaceCombo->setDisabled(false);
        m_curResults = 0;
    }
}

void KatePluginSearchView::slotPluginViewCreated (const QString &name, Kate::PluginView *pluginView)
{
    // add view
    if (name == "kateprojectplugin") {
        m_projectPluginView = pluginView;
        slotProjectFileNameChanged ();
        connect (pluginView, SIGNAL(projectFileNameChanged()), this, SLOT(slotProjectFileNameChanged()));
    }
}

void KatePluginSearchView::slotPluginViewDeleted (const QString &name, Kate::PluginView *)
{
    // remove view
    if (name == "kateprojectplugin") {
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
            m_ui.searchPlaceCombo->addItem (SmallIcon("project-open"), i18n("in Project"));

            // switch to search "in Project"
            setSearchPlace (2);
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
: QObject(parent), KTextEditor::Command()
{
}

const QStringList& KateSearchCommand::cmds()
{
    static QStringList sl = QStringList() << "grep" << "newGrep"
        << "search" << "newSearch"
        << "pgrep" << "newPGrep";
    return sl;
}

bool KateSearchCommand::exec (KTextEditor::View* /*view*/, const QString& cmd, QString& /*msg*/)
{
    //create a list of args
    QStringList args(cmd.split(' ', QString::KeepEmptyParts));
    QString command = args.takeFirst();
    QString searchText = args.join(QString(' '));

    if (command == "grep" || command == "newGrep") {
        emit setSearchPlace(1);
        emit setCurrentFolder();
        if (command == "newGrep")
            emit newTab();
    }
    
    else if (command == "search" || command == "newSearch") {
        emit setSearchPlace(0);
        if (command == "newSearch")
            emit newTab();
    }
    
    else if (command == "pgrep" || command == "newPGrep") {
        emit setSearchPlace(2);
        if (command == "newPGrep")
            emit newTab();
    }
    
    emit setSearchString(searchText);
    emit startSearch();

    return true;
}

bool KateSearchCommand::help (KTextEditor::View */*view*/, const QString &cmd, QString & msg)
{
    if (cmd.startsWith("grep")) {
        msg = i18n("Usage: grep [pattern to search for in folder]");
    }
    else if (cmd.startsWith("newGrep")) {
        msg = i18n("Usage: newGrep [pattern to search for in folder]");
    }

    else if (cmd.startsWith("search")) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    }
    else if (cmd.startsWith("newSearch")) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    }
    
    else if (cmd.startsWith("pgrep")) {
        msg = i18n("Usage: pgrep [pattern to search for in current project]");
    }
    else if (cmd.startsWith("newPGrep")) {
        msg = i18n("Usage: newPGrep [pattern to search for in current project]");
    }
    
    return true;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
