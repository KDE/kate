/*   Kate search plugin
 *
 * SPDX-FileCopyrightText: 2011-2020 Kåre Särs <kare.sars@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

#ifndef _PLUGIN_SEARCH_H_
#define _PLUGIN_SEARCH_H_

#include <KTextEditor/Command>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <QAction>
#include <ktexteditor/application.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/sessionconfiginterface.h>

#include <QTimer>
#include <QTreeWidget>

#include <KXMLGUIClient>

#include "ui_results.h"
#include "ui_search.h"

#include "FolderFilesList.h"
#include "SearchDiskFiles.h"
#include "replace_matches.h"
#include "search_open_files.h"

class KateSearchCommand;
class QPoint;
namespace KTextEditor
{
class MovingRange;
}

class Results : public QWidget, public Ui::Results
{
    Q_OBJECT
public:
    Results(QWidget *parent = nullptr);
    int matches = 0;
    QRegularExpression regExp;
    bool useRegExp = false;
    bool matchCase = false;
    QString replaceStr;
    int searchPlaceIndex = 0;
    QString treeRootText;
};

// This class keeps the focus inside the S&R plugin when pressing tab/shift+tab by overriding focusNextPrevChild()
class ContainerWidget : public QWidget
{
    Q_OBJECT
public:
    ContainerWidget(QWidget *parent)
        : QWidget(parent)
    {
    }

Q_SIGNALS:
    void nextFocus(QWidget *currentWidget, bool *found, bool next);

protected:
    bool focusNextPrevChild(bool next) override;
};

class KatePluginSearch : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginSearch(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KatePluginSearch() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    KateSearchCommand *m_searchCommand = nullptr;
};

class KatePluginSearchView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)

public:
    enum SearchPlaces { CurrentFile, OpenFiles, Folder, Project, AllProjects };

    KatePluginSearchView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWindow, KTextEditor::Application *application);
    ~KatePluginSearchView() override;

    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;

public Q_SLOTS:
    void stopClicked();
    void startSearch();
    void setSearchString(const QString &pattern);
    void navigateFolderUp();
    void setCurrentFolder();
    void setSearchPlace(int place);
    void goToNextMatch();
    void goToPreviousMatch();

private:
    enum CopyResultType { AllExpanded, All };

private Q_SLOTS:
    void openSearchView();
    void handleEsc(QEvent *e);
    void nextFocus(QWidget *currentWidget, bool *found, bool next);

    void addTab();
    void tabCloseRequested(int index);
    void toggleOptions(bool show);

    void searchContextMenu(const QPoint &pos);
    void replaceContextMenu(const QPoint &pos);

    void searchPlaceChanged();
    void startSearchWhileTyping();

    void folderFileListChanged();

    void matchFound(const QString &url, const QString &fileName, const QString &lineContent, int matchLen, int startLine, int startColumn, int endLine, int endColumn);

    void addMatchMark(KTextEditor::Document *doc, QTreeWidgetItem *item);

    void searchDone();
    void searchWhileTypingDone();
    void indicateMatch(bool hasMatch);

    void searching(const QString &file);

    void itemSelected(QTreeWidgetItem *item);

    void clearMarks();
    void clearDocMarks(KTextEditor::Document *doc);

    void replaceSingleMatch();
    void replaceChecked();

    void replaceStatus(const QUrl &url, int replacedInFile, int matchesInFile);
    void replaceDone();

    void docViewChanged();

    void resultTabChanged(int index);

    void expandResults();

    void updateResultsRootItem();

    /**
     * keep track if the project plugin is alive and if the project file did change
     */
    void slotPluginViewCreated(const QString &name, QObject *pluginView);
    void slotPluginViewDeleted(const QString &name, QObject *pluginView);
    void slotProjectFileNameChanged();

    void copySearchToClipboard(CopyResultType type);
    void customResMenuRequested(const QPoint &pos);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void addHeaderItem();

private:
    QTreeWidgetItem *rootFileItem(const QString &url, const QString &fName);
    QStringList filterFiles(const QStringList &files) const;

    void onResize(const QSize &size);

    Ui::SearchDialog m_ui {};
    QWidget *m_toolView;
    KTextEditor::Application *m_kateApp;
    SearchOpenFiles m_searchOpenFiles;
    FolderFilesList m_folderFilesList;
    SearchDiskFiles m_searchDiskFiles;
    ReplaceMatches m_replacer;
    QAction *m_matchCase = nullptr;
    QAction *m_useRegExp = nullptr;
    Results *m_curResults = nullptr;
    bool m_searchJustOpened = false;
    int m_projectSearchPlaceIndex = 0;
    bool m_searchDiskFilesDone = true;
    bool m_searchOpenFilesDone = true;
    bool m_isSearchAsYouType = false;
    bool m_isVerticalLayout = false;
    bool m_blockDiskMatchFound = false;
    QString m_resultBaseDir;
    QList<KTextEditor::MovingRange *> m_matchRanges;
    QTimer m_changeTimer;
    QTimer m_updateSumaryTimer;
    QPointer<KTextEditor::Message> m_infoMessage;
    QString m_searchBackgroundColor;
    QString m_foregroundColor;
    QString m_lineNumberColor;

    /**
     * current project plugin view, if any
     */
    QObject *m_projectPluginView = nullptr;

    /**
     * our main window
     */
    KTextEditor::MainWindow *m_mainWindow;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
