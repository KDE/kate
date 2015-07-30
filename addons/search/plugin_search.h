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

#ifndef _PLUGIN_SEARCH_H_
#define _PLUGIN_SEARCH_H_

#include <ktexteditor/mainwindow.h>
#include <KTextEditor/Plugin>
#include <ktexteditor/application.h>
#include <KTextEditor/Command>
#include <ktexteditor/sessionconfiginterface.h>
#include <QAction>

#include <QTreeWidget>
#include <QTimer>

#include <KXMLGUIClient>

#include "ui_search.h"
#include "ui_results.h"

#include "search_open_files.h"
#include "SearchDiskFiles.h"
#include "FolderFilesList.h"
#include "replace_matches.h"

class KateSearchCommand;
namespace KTextEditor{
    class MovingRange;
}

class Results: public QWidget, public Ui::Results
{
    Q_OBJECT
public:
    Results(QWidget *parent = 0);
    int     matches;
    QRegularExpression regExp;
    bool    fixedString;
    QString replace;
};

// This class keeps the focus inside the S&R plugin when pressing tab/shift+tab by overriding focusNextPrevChild()
class ContainerWidget:public QWidget
{
    Q_OBJECT
public:
    ContainerWidget(QWidget *parent): QWidget(parent) {}

Q_SIGNALS:
    void nextFocus(QWidget *currentWidget, bool *found, bool next);

protected:
    virtual bool focusNextPrevChild (bool next);
};


class KatePluginSearch : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginSearch(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~KatePluginSearch();

    QObject *createView(KTextEditor::MainWindow *mainWindow);

private:
    KateSearchCommand* m_searchCommand;
};



class KatePluginSearchView : public QObject, public KXMLGUIClient, public KTextEditor::SessionConfigInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::SessionConfigInterface)

public:
    enum SearchPlaces {
        CurrentFile,
        OpenFiles,
        Folder,
        Project,
        AllProjects
    };

    KatePluginSearchView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWindow, KTextEditor::Application* application);
    ~KatePluginSearchView();

    void readSessionConfig (const KConfigGroup& config);
    void writeSessionConfig (KConfigGroup& config);

public Q_SLOTS:
    void startSearch();
    void setSearchString(const QString &pattern);
    void navigateFolderUp();
    void setCurrentFolder();
    void setSearchPlace(int place);
    void goToNextMatch();
    void goToPreviousMatch();

private Q_SLOTS:
    void openSearchView();
    void handleEsc(QEvent *e);
    void nextFocus(QWidget *currentWidget, bool *found, bool next);

    void addTab();
    void tabCloseRequested(int index);
    void toggleOptions(bool show);

    void searchContextMenu(const QPoint& pos);

    void searchPlaceChanged();
    void startSearchWhileTyping();

    void folderFileListChanged();

    void matchFound(const QString &url, const QString &fileName, int line, int column,
                    const QString &lineContent, int matchLen);

    void addMatchMark(KTextEditor::Document* doc, int line, int column, int len);

    void searchDone();
    void searchWhileTypingDone();
    void indicateMatch(bool hasMatch);

    void searching(const QString &file);

    void itemSelected(QTreeWidgetItem *item);

    void clearMarks();
    void clearDocMarks(KTextEditor::Document* doc);

    void replaceSingleMatch();
    void replaceChecked();

    void replaceDone();

    void docViewChanged();


    void resultTabChanged(int index);

    /**
     * keep track if the project plugin is alive and if the project file did change
     */
    void slotPluginViewCreated (const QString &name, QObject *pluginView);
    void slotPluginViewDeleted (const QString &name, QObject *pluginView);
    void slotProjectFileNameChanged ();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void addHeaderItem();

private:
    QTreeWidgetItem *rootFileItem(const QString &url, const QString &fName);
    QStringList filterFiles(const QStringList& files) const;
    QString currentWord(const KTextEditor::Document& document, const KTextEditor::Cursor& cursor) const;

    Ui::SearchDialog                   m_ui;
    QWidget                           *m_toolView;
    KTextEditor::Application          *m_kateApp;
    SearchOpenFiles                    m_searchOpenFiles;
    FolderFilesList                    m_folderFilesList;
    SearchDiskFiles                    m_searchDiskFiles;
    ReplaceMatches                     m_replacer;
    QAction                           *m_matchCase;
    QAction                           *m_useRegExp;
    Results                           *m_curResults;
    bool                               m_searchJustOpened;
    bool                               m_switchToProjectModeWhenAvailable;
    bool                               m_searchDiskFilesDone;
    bool                               m_searchOpenFilesDone;
    QString                            m_resultBaseDir;
    QList<KTextEditor::MovingRange*>   m_matchRanges;
    QTimer                             m_changeTimer;

    /**
     * current project plugin view, if any
     */
    QObject *m_projectPluginView;

    /**
     * our main window
     */
    KTextEditor::MainWindow *m_mainWindow;
};

class KateSearchCommand : public KTextEditor::Command
{
    Q_OBJECT
public:
    KateSearchCommand(QObject *parent);

Q_SIGNALS:
    void setSearchPlace(int place);
    void setCurrentFolder();
    void setSearchString(const QString &pattern);
    void startSearch();
    void newTab();

    //
    // KTextEditor::Command
    //
public:
    bool exec (KTextEditor::View *view, const QString &cmd, QString &msg,
                      const KTextEditor::Range &range = KTextEditor::Range::invalid());
    bool help (KTextEditor::View *view, const QString &cmd, QString &msg);
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;

