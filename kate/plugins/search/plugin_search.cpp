/*   Kate search plugin
 * 
 * Copyright (C) 2011 by Kåre Särs <kare.sars@iki.fi>
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

#include <kate/application.h>
#include <ktexteditor/editor.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kate/documentmanager.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kurlcompletion.h>
#include <QLineEdit>
#include <QKeyEvent>
#include <QClipboard>

K_PLUGIN_FACTORY(KatePluginSearchFactory, registerPlugin<KatePluginSearch>();)
K_EXPORT_PLUGIN(KatePluginSearchFactory(KAboutData("katesearch","katesearch",ki18n("Search in files"), "0.1", ki18n("Find in open files plugin"))))

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
    
    return view;
}


KatePluginSearchView::KatePluginSearchView(Kate::MainWindow *mainWin, Kate::Application* application)
: Kate::PluginView(mainWin),
Kate::XMLGUIClient(KatePluginSearchFactory::componentData()),
m_kateApp(application)
{
    KAction *a = actionCollection()->addAction("search_in_files");
    a->setText(i18n("Search in Files"));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(toggleSearchView()));

    m_toolView = mainWin->createToolView ("kate_plugin_katesearch",
                                          Kate::MainWindow::Bottom,
                                          SmallIcon("edit-find"),
                                          i18n("Search in files"));
    QWidget *container = new QWidget(m_toolView);
    m_ui.setupUi(container);

    m_ui.displayOptions->setIcon(KIcon("arrow-down-double"));
    m_ui.searchButton->setIcon(KIcon("edit-find"));
    m_ui.stopButton->setIcon(KIcon("process-stop"));
    m_ui.optionsButton->setIcon(KIcon("configure"));
    m_ui.searchPlaceCombo->setItemIcon(0, KIcon("text-plain"));
    m_ui.searchPlaceCombo->setItemIcon(1, KIcon("folder"));
    m_ui.currentFolderButton->setIcon(KIcon("view-refresh"));
    m_ui.hideButton->setIcon(KIcon("dialog-close"));

    int padWidth = m_ui.folderLabel->width();
    padWidth = qMax(padWidth, m_ui.filterLabel->width());
    m_ui.gridLayout->setColumnMinimumWidth(0, padWidth);
    m_ui.gridLayout->setAlignment(m_ui.hideButton, Qt::AlignHCenter);

    // get url-requester's combo box and sanely initialize
    KComboBox* cmbUrl = m_ui.folderRequester->comboBox();
    cmbUrl->setDuplicatesEnabled(false);
    cmbUrl->setEditable(true);
    m_ui.folderRequester->setMode(KFile::Directory | KFile::LocalOnly);
    KUrlCompletion* cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
    cmbUrl->setCompletionObject(cmpl);
    cmbUrl->setAutoDeleteCompletionObject(true);

    m_matchCase = new KAction(i18n("Match case"), this);
    m_matchCase->setCheckable(true);
    m_ui.optionsButton->addAction(m_matchCase);

    m_useRegExp = new KAction(i18n("Regular Expression"), this);
    m_useRegExp->setCheckable(true);
    m_ui.optionsButton->addAction(m_useRegExp);

    connect(m_ui.hideButton, SIGNAL(clicked()), this, SLOT(toggleSearchView()));
    connect(m_ui.searchButton, SIGNAL(clicked()), this, SLOT(startSearch()));
    connect(m_ui.searchCombo, SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.folderRequester, SIGNAL(returnPressed()), this, SLOT(startSearch()));
    connect(m_ui.currentFolderButton, SIGNAL(clicked()), this, SLOT(setCurrentFolder()));

    connect(m_ui.filterCombo, SIGNAL(returnPressed()), this, SLOT(startSearch()));

    connect(m_ui.displayOptions, SIGNAL(toggled(bool)), this, SLOT(toggleOptions(bool)));
    connect(m_ui.searchPlaceCombo, SIGNAL(currentIndexChanged (int)), this, SLOT(searchPlaceChanged()));
    connect(m_ui.searchCombo, SIGNAL(editTextChanged(QString)), this, SLOT(searchPatternChanged()));
    connect(m_ui.stopButton, SIGNAL(clicked()), &m_searchOpenFiles, SLOT(cancelSearch()));
    connect(m_ui.stopButton, SIGNAL(clicked()), &m_searchFolder, SLOT(cancelSearch()));

    m_ui.displayOptions->setChecked(true);

    connect(m_ui.searchResults, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
            SLOT(itemSelected(QTreeWidgetItem *)));

    connect(&m_searchOpenFiles, SIGNAL(matchFound(QString, int, int, QString)),
            this, SLOT(matchFound(QString, int, int, QString)));
    connect(&m_searchOpenFiles, SIGNAL(searchDone()),  this, SLOT(searchDone()));

    connect(&m_searchFolder, SIGNAL(matchFound(QString, int, int, QString)),
            this, SLOT(matchFound(QString, int, int, QString)));
    connect(&m_searchFolder, SIGNAL(searchDone()),  this, SLOT(searchDone()));

    connect(m_kateApp->documentManager(), SIGNAL(documentWillBeDeleted (KTextEditor::Document *)),
            &m_searchOpenFiles, SLOT(cancelSearch()));

    searchPlaceChanged();

    m_ui.searchResults->installEventFilter(this);
    m_toolView->installEventFilter(this);

    mainWindow()->guiFactory()->addClient(this);
}

KatePluginSearchView::~KatePluginSearchView()
{
    mainWindow()->guiFactory()->removeClient(this);
    delete m_toolView;
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

void KatePluginSearchView::toggleSearchView()
{
    if (!mainWindow()) {
        return;
    }
    if (!m_toolView->isVisible()) {
        mainWindow()->showToolView(m_toolView);
        m_ui.searchCombo->setFocus(Qt::OtherFocusReason);
        if (m_ui.folderRequester->text().isEmpty()) {
            KTextEditor::View* editView = mainWindow()->activeView();
            if (editView && editView->document()) {
                // upUrl as we want the folder not the file
                m_ui.folderRequester->setUrl(editView->document()->url().upUrl());
            }
        }
    }
    else {
        mainWindow()->hideToolView(m_toolView);
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
    m_ui.searchButton->setDisabled(true);
    m_ui.locationAndStop->setCurrentIndex(1);
    m_ui.optionsButton->setDisabled(true);
    m_ui.displayOptions->setChecked (false);
    m_ui.displayOptions->setDisabled(true);

    QRegExp reg(m_ui.searchCombo->currentText(),
                m_matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                m_useRegExp->isChecked() ? QRegExp::RegExp : QRegExp::FixedString);

    m_ui.searchResults->clear();
    if (m_ui.searchPlaceCombo->currentIndex() ==  0) {
        m_searchOpenFiles.startSearch(m_kateApp->documentManager()->documents(), reg);
    }
    else {
        m_searchFolder.startSearch(m_ui.folderRequester->text(),
                                   m_ui.recursiveCheckBox->isChecked(),
                                   m_ui.hiddenCheckBox->isChecked(),
                                   m_ui.symLinkCheckBox->isChecked(),
                                   m_ui.filterCombo->currentText(),
                                   reg);
    }
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
    bool disable = (m_ui.searchPlaceCombo->currentIndex() == 0);
    if (!disable) {
        m_ui.displayOptions->setChecked(true);
    }
    m_ui.recursiveCheckBox->setDisabled(disable);
    m_ui.hiddenCheckBox->setDisabled(disable);
    m_ui.symLinkCheckBox->setDisabled(disable);
    m_ui.folderLabel->setDisabled(disable);
    m_ui.folderRequester->setDisabled(disable);
    m_ui.filterLabel->setDisabled(disable);
    m_ui.filterCombo->setDisabled(disable);
}

void KatePluginSearchView::searchPatternChanged()
{
    m_ui.searchButton->setDisabled(m_ui.searchCombo->currentText().isEmpty());
}

void KatePluginSearchView::matchFound(const QString &url, int line, int column, const QString &lineContent)
{
    QStringList row;
    row << QFileInfo(url).fileName() << QString::number(line +1) << lineContent;
    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui.searchResults, row);
    item->setData(0, Qt::UserRole, url);
    item->setData(0, Qt::ToolTipRole, url);
    item->setData(1, Qt::UserRole, column);
}

void KatePluginSearchView::searchDone()
{
    m_ui.searchButton->setDisabled(false);
    m_ui.locationAndStop->setCurrentIndex(0);
    m_ui.optionsButton->setDisabled(false);
    m_ui.displayOptions->setDisabled(false);

    m_ui.searchResults->resizeColumnToContents(0);
    m_ui.searchResults->resizeColumnToContents(1);
    m_ui.searchResults->resizeColumnToContents(2);
    if (m_ui.searchResults->topLevelItemCount() > 0) {
        m_ui.searchResults->setCurrentItem(m_ui.searchResults-> topLevelItem(0));
        m_ui.searchResults->setFocus(Qt::OtherFocusReason);
    }
}

void KatePluginSearchView::itemSelected(QTreeWidgetItem *item)
{
    // get stuff
    const QString url = item->data(0, Qt::UserRole).toString();
    if (url.isEmpty()) return;
    int line = item->data(1, Qt::DisplayRole).toInt() - 1;
    int column = item->data(1, Qt::UserRole).toInt();

    // open file (if needed, otherwise, this will activate only the right view...)
    mainWindow()->openUrl(KUrl(url));

    // any view active?
    if (!mainWindow()->activeView()) {
        return;
    }

    // do it ;)
    mainWindow()->activeView()->setCursorPosition(KTextEditor::Cursor(line, column));
    //mainWindow()->activeView()->setFocus();
}

void KatePluginSearchView::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":search-plugin");
    m_ui.searchCombo->clearHistory();
    m_ui.searchCombo->setHistoryItems(cg.readEntry("Search", QStringList()), true);
    m_matchCase->setChecked(cg.readEntry("MatchCase", false));
    m_useRegExp->setChecked(cg.readEntry("UseRegExp", false));

    m_ui.searchPlaceCombo->setCurrentIndex(cg.readEntry("Place", 0));
    m_ui.recursiveCheckBox->setChecked(cg.readEntry("Recursive", true));
    m_ui.hiddenCheckBox->setChecked(cg.readEntry("HiddenFiles", false));
    m_ui.symLinkCheckBox->setChecked(cg.readEntry("FollowSymLink", false));
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
    cg.writeEntry("MatchCase", m_matchCase->isChecked());
    cg.writeEntry("UseRegExp", m_useRegExp->isChecked());

    cg.writeEntry("Place", m_ui.searchPlaceCombo->currentIndex());
    cg.writeEntry("Recursive", m_ui.recursiveCheckBox->isChecked());
    cg.writeEntry("HiddenFiles", m_ui.hiddenCheckBox->isChecked());
    cg.writeEntry("FollowSymLink", m_ui.symLinkCheckBox->isChecked());
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

bool KatePluginSearchView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (obj == m_ui.searchResults) {
            if (ke->matches(QKeySequence::Copy)) {
                // user pressed ctrl+c -> copy full URL to the clipboard
                QVariant variant = m_ui.searchResults->currentItem()->data(0, Qt::UserRole);
                QApplication::clipboard()->setText(variant.toString());
                event->accept();
                return true;
            }
            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
                if (m_ui.searchResults->currentItem()) {
                    itemSelected(m_ui.searchResults->currentItem());
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

KateSearchCommand::KateSearchCommand(QObject *parent)
: QObject(parent), KTextEditor::Command()
{
}

const QStringList& KateSearchCommand::cmds()
{
    static QStringList sl = QStringList() << "grep" << "search";
    return sl;
}

bool KateSearchCommand::exec (KTextEditor::View* /*view*/, const QString& cmd, QString& /*msg*/)
{
    //create a list of args
    QStringList args(cmd.split(' ', QString::KeepEmptyParts));
    QString command = args.takeFirst();
    QString searchText = args.join(QString(' '));

    if (command == "grep") {
        emit setSearchPlace(1);
        emit setCurrentFolder();
    }
    else if (command == "search") {
        emit setSearchPlace(0);
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
    else if (cmd.startsWith("search")) {
        msg = i18n("Usage: search [pattern to search for in open files]");
    }
    return true;
}


// kate: space-indent on; indent-width 4; replace-tabs on;
