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
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kate/documentmanager.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

K_PLUGIN_FACTORY(KatePluginSearchFactory, registerPlugin<KatePluginSearch>();)
K_EXPORT_PLUGIN(KatePluginSearchFactory(KAboutData("katesearch","katesearch",ki18n("Search in files"), "0.1", ki18n("Find in open files plugin"))))

KatePluginSearch::KatePluginSearch(QObject* parent, const QList<QVariant>&)
    : Kate::Plugin((Kate::Application*)parent, "kate-search-plugin")
{
    KGlobal::locale()->insertCatalog("katesearch");
}

KatePluginSearch::~KatePluginSearch()
{
}

Kate::PluginView *KatePluginSearch::createView(Kate::MainWindow *mainWindow)
{
    return new KatePluginSearchView(mainWindow, application());
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

    m_ui.displayOptions->setIcon(KIcon("list-add"));
    m_ui.searchButton->setIcon(KIcon("edit-find"));
    m_ui.stopButton->setIcon(KIcon("process-stop"));
    
    m_matchCase = new KAction(i18n("Match case"), this);
    m_matchCase->setCheckable(true);
    m_ui.optionsButton->addAction(m_matchCase);
    
    m_useRegExp = new KAction(i18n("Regular Expression"), this);
    m_useRegExp->setCheckable(true);
    m_ui.optionsButton->addAction(m_useRegExp);

    connect(m_ui.searchButton, SIGNAL(clicked()), this, SLOT(startSearch()));
    connect(m_ui.displayOptions, SIGNAL(toggled(bool)), this, SLOT(toggleOptions(bool)));
    connect(m_ui.searchPlaceCombo, SIGNAL(currentIndexChanged (int)), this, SLOT(searchPlaceChanged()));
    connect(m_ui.searchCombo, SIGNAL(editTextChanged(QString)), this, SLOT(searchPatternChanged()));
    connect(m_ui.stopButton, SIGNAL(clicked()), &m_searchOpenFiles, SLOT(cancelSearch()));
    connect(m_ui.stopButton, SIGNAL(clicked()), &m_searchFolder, SLOT(cancelSearch()));
    
    m_ui.displayOptions->setChecked(true);

    connect(m_ui.searchResults, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            SLOT(itemSelected(QTreeWidgetItem *)));

    connect(&m_searchOpenFiles, SIGNAL(matchFound(QString, int, QString)),
            this, SLOT(matchFound(QString, int, QString)));
    connect(&m_searchOpenFiles, SIGNAL(searchDone()),  this, SLOT(searchDone()));

    connect(&m_searchFolder, SIGNAL(matchFound(QString, int, QString)),
            this, SLOT(matchFound(QString, int, QString)));
    connect(&m_searchFolder, SIGNAL(searchDone()),  this, SLOT(searchDone()));
    
    mainWindow()->guiFactory()->addClient(this);
}

KatePluginSearchView::~KatePluginSearchView()
{
    mainWindow()->guiFactory()->removeClient(this);
}

void KatePluginSearchView::toggleSearchView()
{
    if (!mainWindow()) {
        return;
    }
    if (!m_toolView->isVisible()) {
        mainWindow()->showToolView(m_toolView);
        m_ui.searchCombo->setFocus(Qt::OtherFocusReason);
    }
    else {
        mainWindow()->hideToolView(m_toolView);
    }
}

void KatePluginSearchView::startSearch()
{
    m_ui.searchButton->setDisabled(true);
    m_ui.stopButton->setDisabled(false);
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
        m_searchFolder.startSearch(m_ui.searchFolder->text(),
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
    searchPlaceChanged();
}

void KatePluginSearchView::searchPlaceChanged()
{
    bool disable = (m_ui.searchPlaceCombo->currentIndex() == 0);
    m_ui.recursiveCheckBox->setDisabled(disable);
    m_ui.hiddenCheckBox->setDisabled(disable);
    m_ui.symLinkCheckBox->setDisabled(disable);
    m_ui.folderLabel->setDisabled(disable);
    m_ui.searchFolder->setDisabled(disable);
    m_ui.filterLabel->setDisabled(disable);
    m_ui.filterCombo->setDisabled(disable);
}

void KatePluginSearchView::searchPatternChanged()
{
    m_ui.searchButton->setDisabled(m_ui.searchCombo->currentText().isEmpty());
}

void KatePluginSearchView::matchFound(const QString &fileName, int line, const QString &lineContent)
{
    QStringList row;
    row << fileName << QString::number(line) << lineContent;
    new QTreeWidgetItem(m_ui.searchResults, row);
}

void KatePluginSearchView::searchDone()
{
    m_ui.searchButton->setDisabled(false);
    m_ui.stopButton->setDisabled(true);
    m_ui.optionsButton->setDisabled(false);
    m_ui.displayOptions->setDisabled(false);
    
    m_ui.searchResults->resizeColumnToContents(0);
    m_ui.searchResults->resizeColumnToContents(1);
    m_ui.searchResults->resizeColumnToContents(2);
}

void KatePluginSearchView::itemSelected(QTreeWidgetItem *item)
{
    // get stuff
    const QString url = item->data(0, Qt::DisplayRole).toString();
    if (url.isEmpty()) return;
    int line = item->data(1, Qt::DisplayRole).toInt();
    
    // open file (if needed, otherwise, this will activate only the right view...)
    mainWindow()->openUrl(KUrl(url));
    
    // any view active?
    if (!mainWindow()->activeView()) {
        return;
    }
    
    // do it ;)
    mainWindow()->activeView()->setCursorPosition(KTextEditor::Cursor(line, 0));
    mainWindow()->activeView()->setFocus();
}

void KatePluginSearchView::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":search-plugin");
    m_ui.searchCombo->clear();
    m_ui.searchCombo->addItem(cg.readEntry("Search", QString()));
    m_matchCase->setChecked(cg.readEntry("MatchCase", false));
    m_useRegExp->setChecked(cg.readEntry("UseRegExp", false));

    m_ui.searchPlaceCombo->setCurrentIndex(cg.readEntry("Place", 0));
    m_ui.recursiveCheckBox->setChecked(cg.readEntry("Recursive", true));
    m_ui.hiddenCheckBox->setChecked(cg.readEntry("HiddenFiles", false));
    m_ui.symLinkCheckBox->setChecked(cg.readEntry("FollowSymLink", false));
    m_ui.searchFolder->setText(cg.readEntry("SearchFolder", QString()));
    m_ui.filterCombo->clear();
    m_ui.filterCombo->addItems(cg.readEntry("Filters", QString()).split(";"));
    m_ui.filterCombo->setCurrentIndex(cg.readEntry("CurrentFilter", 0));
}

void KatePluginSearchView::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":search-plugin");
    cg.writeEntry("Search", m_ui.searchCombo->currentText()); // FIXME
    cg.writeEntry("MatchCase", m_matchCase->isChecked());
    cg.writeEntry("UseRegExp", m_useRegExp->isChecked());
    
    cg.writeEntry("Place", m_ui.searchPlaceCombo->currentIndex());
    cg.writeEntry("Recursive", m_ui.recursiveCheckBox->isChecked());
    cg.writeEntry("HiddenFiles", m_ui.hiddenCheckBox->isChecked());
    cg.writeEntry("FollowSymLink", m_ui.symLinkCheckBox->isChecked());
    cg.writeEntry("SearchFolder", m_ui.searchFolder->text());
    QStringList filterItems;
    for (int i=0; i<qMin(m_ui.filterCombo->count(), 10); i++) {
        filterItems << m_ui.filterCombo->itemText(i);
    }
    cg.writeEntry("Filters", filterItems.join(";"));
    cg.writeEntry("CurrentFilter", m_ui.filterCombo->currentIndex());
}

// kate: space-indent on; indent-width 4; replace-tabs on;

