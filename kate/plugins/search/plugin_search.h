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

#ifndef _PLUGIN_SEARCH_H_
#define _PLUGIN_SEARCH_H_

#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/commandinterface.h>
#include <kaction.h>

#include <QTreeWidget>

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
    QRegExp regExp;

public Q_SLOTS:
    void selectAll(bool checked);
    void checkCheckedState();
};

class KatePluginSearch : public Kate::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginSearch(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~KatePluginSearch();

    Kate::PluginView *createView(Kate::MainWindow *mainWindow);

private:
    KateSearchCommand* m_searchCommand;
};



class KatePluginSearchView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    KatePluginSearchView(Kate::MainWindow *mainWindow, Kate::Application* application);
    ~KatePluginSearchView();

    virtual void readSessionConfig(KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig(KConfigBase* config, const QString& groupPrefix);

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
    void addTab();
    void closeTab(QWidget *widget);
    void toggleOptions(bool show);

    void searchContextMenu(const QPoint& pos);

    void searchPlaceChanged();
    void searchPatternChanged();

    void folderFileListChanged();

    void matchFound(const QString &fileName, int line, int column,
                    const QString &lineContent, int matchLen);

    void addMatchMark(KTextEditor::Document* doc, int line, int column, int len);

    void searchDone();
    void searchWhileTypingDone();
    void indicateMatch(bool hasMatch);

    void itemSelected(QTreeWidgetItem *item);

    void clearMarks();
    void clearDocMarks(KTextEditor::Document* doc);

    void replaceChecked();

    void replaceDone();

    void resultTabChanged(int index);

    /**
     * keep track if the project plugin is alive and if the project file did change
     */
    void slotPluginViewCreated (const QString &name, Kate::PluginView *pluginView);
    void slotPluginViewDeleted (const QString &name, Kate::PluginView *pluginView);
    void slotProjectFileNameChanged ();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void addHeaderItem(const QString& text);

private:
    QTreeWidgetItem *rootFileItem(const QString &url);
    QStringList filterFiles(const QStringList& files) const;

    Ui::SearchDialog                   m_ui;
    QWidget                           *m_toolView;
    Kate::Application                 *m_kateApp;
    SearchOpenFiles                    m_searchOpenFiles;
    FolderFilesList                    m_folderFilesList;
    SearchDiskFiles                    m_searchDiskFiles;
    ReplaceMatches                     m_replacer;
    KAction                           *m_matchCase;
    KAction                           *m_useRegExp;
    Results                           *m_curResults;
    bool                               m_searchJustOpened;
    bool                               m_switchToProjectModeWhenAvailable;
    QString                            m_resultBaseDir;
    QList<KTextEditor::MovingRange*>   m_matchRanges;

    /**
     * current project plugin view, if any
     */
    Kate::PluginView *m_projectPluginView;
};

class KateSearchCommand : public QObject, public KTextEditor::Command
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
    const QStringList &cmds ();
    bool exec (KTextEditor::View *view, const QString &cmd, QString &msg);
    bool help (KTextEditor::View *view, const QString &cmd, QString &msg);
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;

