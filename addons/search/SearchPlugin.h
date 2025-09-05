/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KTextEditor/Command>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <ktexteditor/application.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/sessionconfiginterface.h>

#include <QAction>
#include <QPointer>
#include <QThreadPool>
#include <QTimer>
#include <QTreeView>

#include <KXMLGUIClient>

#include "ui_results.h"
#include "ui_search.h"

#include "FolderFilesList.h"
#include "MatchModel.h"
#include "Results.h"
#include "SearchDiskFiles.h"
#include "SearchOpenFiles.h"

class KateSearchCommand;
class Results;
class QPoint;
namespace KTextEditor
{
class MovingRange;
class MovingInterface;
}

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
public:
    explicit KatePluginSearch(QObject *parent);
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
    KatePluginSearchView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWindow, KTextEditor::Application *application);
    ~KatePluginSearchView() override;

    void readSessionConfig(const KConfigGroup &config) override;
    void writeSessionConfig(KConfigGroup &config) override;

    static void addRegexHelperActionsForReplace(QSet<QAction *> *actionList, QMenu *menu);
    static void regexHelperActOnAction(QAction *resultAction, const QSet<QAction *> &actionList, QLineEdit *lineEdit);

public:
    void stopClicked();
    void startSearch();
    void setSearchString(const QString &pattern);
    void navigateFolderUp();
    void setCurrentFolder();
    void setSearchPlace(int place);
    void goToNextMatch();
    void goToPreviousMatch();
    void setRegexMode(bool enabled);
    void setCaseInsensitive(bool enabled);
    void setExpandResults(bool enabled);
    void addTab();

private:
    enum CopyResultType {
        AllExpanded,
        All
    };
    enum class MatchType {
        NoMatch,
        HasMatch,
        InvalidRegExp
    };

private Q_SLOTS:
    void slotProjectFileNameChanged();

private:
    void openSearchView();
    void handleEsc(QEvent *e);
    void nextFocus(QWidget *currentWidget, bool *found, bool next);

    void tabCloseRequested(int index);
    void toggleOptions(bool show);
    void detachTabToMainWindow(Results *);

    void searchContextMenu(const QPoint &pos);
    void replaceContextMenu(const QPoint &pos);

    void searchPlaceChanged();
    void startSearchWhileTyping();

    void folderFileListChanged();

    void matchesFound(const QUrl &url, const QList<KateSearchMatch> &searchMatches, KTextEditor::Document *doc);

    void addRangeAndMark(KTextEditor::Document *doc, const KateSearchMatch &match, KTextEditor::Attribute::Ptr attr, const QRegularExpression &regexp);

    void searchDone();
    void searchWhileTypingDone();

    void indicateMatch(MatchType matchType);

    void itemSelected(const QModelIndex &item);

    void clearMarksAndRanges();
    void clearDocMarksAndRanges(KTextEditor::Document *doc);

    void replaceSingleMatch();
    void replaceChecked();

    void replaceDone();

    void updateCheckState(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void updateMatchMarks();

    void syncModelRanges(Results *resultsTab);

    void resultTabChanged(int index);

    void expandResults();

    /**
     * keep track if the project plugin is alive and if the project file did change
     */
    void slotPluginViewCreated(const QString &name, QObject *pluginView);
    void slotPluginViewDeleted(const QString &name, QObject *pluginView);

    void copySearchToClipboard(CopyResultType type);
    void showExportMatchesDialog();
    void customResMenuRequested(const QPoint &pos);

    void cutSearchedLines();
    void copySearchedLines();

Q_SIGNALS:
    void searchBusy(bool busy);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QList<int> getDocumentSearchMarkedLines(KTextEditor::Document *currentDocument);
    void setClipboardFromDocumentLines(const KTextEditor::Document *currentDocument, const QList<int> &lineNumbers);

    QStringList filterFiles(const QStringList &fileList) const;
    void startDiskFileSearch(const QStringList &fileList, const QRegularExpression &reg, const bool includeBinaryFiles, const int sizeLimit);
    void cancelDiskFileSearch();
    bool searchingDiskFiles();

    void updateViewColors();

    void onResize(const QSize &size);

    Ui::SearchDialog m_ui{};
    QWidget *m_toolView;
    KTextEditor::Application *m_kateApp;
    SearchOpenFiles m_searchOpenFiles;
    FolderFilesList m_folderFilesList;

    /**
     * worklist for runnables, must survive thread pool below!
     */
    SearchDiskFilesWorkList m_worklistForDiskFiles;

    /**
     * threadpool for multi-threaded disk search
     */
    QThreadPool m_searchDiskFilePool;

    QTimer m_diskSearchDoneTimer;
    QTimer m_updateCheckedStateTimer;
    QPointer<Results> m_searchingTab = nullptr;
    QPointer<Results> m_currentTab = nullptr;
    QTabBar *m_tabBar = nullptr;
    bool m_searchJustOpened = false;
    int m_projectSearchPlaceIndex = 0;
    bool m_isSearchAsYouType = false;
    bool m_isVerticalLayout = false;
    QString m_resultBaseDir;
    QList<KTextEditor::MovingRange *> m_matchRanges;
    QTimer m_changeTimer;
    QPointer<KTextEditor::Message> m_infoMessage;
    QColor m_replaceHighlightColor;
    KTextEditor::Attribute::Ptr m_resultAttr;

    QHash<MatchModel::SearchPlaces, bool> m_searchAsYouType;

    /**
     * current project plugin view, if any
     */
    QObject *m_projectPluginView = nullptr;

    /**
     * our main window
     */
    KTextEditor::MainWindow *m_mainWindow;
};

// kate: space-indent on; indent-width 4; replace-tabs on;
