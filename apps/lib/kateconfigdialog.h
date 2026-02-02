/*
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QPointer>

#include <KTextEditor/ConfigPage>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/Plugin>

#include <KPageDialog>

#ifdef WITH_KUSERFEEDBACK
#include <KUserFeedback/FeedbackConfigWidget>
#endif

class QCheckBox;
class QComboBox;
class QSpinBox;
class KateMainWindow;
class QLineEdit;

namespace Ui
{
class SessionConfigWidget;
}

struct PluginPageListItem {
    KTextEditor::Plugin *plugin;
    int idInPlugin;
    KTextEditor::ConfigPage *pluginPage;
    KPageWidgetItem *pageWidgetItem;
};

class KateConfigDialog : public KPageDialog
{
    Q_OBJECT

public:
    // create new dialog
    explicit KateConfigDialog(KateMainWindow *parent);
    ~KateConfigDialog() override;

    /**
     * Reads the value from the given open config. If not present in config yet then
     * the default value 10 is used.
     */
    static int recentFilesMaxCount();

public:
    void addPluginPage(KTextEditor::Plugin *plugin);
    void removePluginPage(KTextEditor::Plugin *plugin);
    void showAppPluginPage(KTextEditor::Plugin *plugin, int id);

protected Q_SLOTS:
    void slotApply();
    void slotChanged();
    static void slotHelp();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void addBehaviorPage();
    void addSessionPage();
    void addPluginsPage();
    void addFeedbackPage();
    void addPluginPages();
    void addEditorPages();

    // add page variant that ensures the page is wrapped into a QScrollArea
    KPageWidgetItem *addScrollablePage(QWidget *page, const QString &itemName);

private:
    KateMainWindow *m_mainWindow;

    bool m_dataChanged = false;

    QComboBox *m_messageTypes = nullptr;
    QSpinBox *m_outputHistoryLimit = nullptr;
    QComboBox *m_mouseBackActions = nullptr;
    QComboBox *m_mouseForwardActions = nullptr;
    QCheckBox *m_sdiMode = nullptr;
    QCheckBox *m_modNotifications = nullptr;
    QCheckBox *m_withDate = nullptr;
    QCheckBox *m_syncSectionSizeWithSidebarTabs = nullptr;
    QCheckBox *m_showTextForLeftRightSidebars = nullptr;
    QSpinBox *m_leftRightSidebarsIconSize = nullptr;
    QComboBox *m_cmbQuickOpenListMode = nullptr;
    QSpinBox *m_tabLimit = nullptr;
    QCheckBox *m_autoHideTabs = nullptr;
    QCheckBox *m_showTabCloseButton = nullptr;
    QCheckBox *m_expandTabs = nullptr;
    QCheckBox *m_tabDoubleClickNewDocument = nullptr;
    QCheckBox *m_tabMiddleClickCloseDocument = nullptr;
    QCheckBox *m_tabsScrollable = nullptr;
    QCheckBox *m_tabsElided = nullptr;
    QComboBox *m_diffStyle = nullptr;
    QCheckBox *m_urlBarShowSymbols = nullptr;
    QCheckBox *m_openNewTabInFrontOfCurrent = nullptr;
    QCheckBox *m_cycleThroughTabs = nullptr;
    QSpinBox *m_diagnosticsLimit = nullptr;
    QCheckBox *m_hintViewEnabled = nullptr;
    QLineEdit *m_pathEdit = nullptr;

    std::unique_ptr<Ui::SessionConfigWidget> sessionConfigUi;

    QHash<KPageWidgetItem *, PluginPageListItem> m_pluginPages;
    std::vector<KTextEditor::ConfigPage *> m_editorPages;

    QPointer<class QListView> m_sideBar;
    QSet<KPageWidgetItem *> m_allPages;

#ifdef WITH_KUSERFEEDBACK
    KUserFeedback::FeedbackConfigWidget *m_userFeedbackWidget = nullptr;
#endif

    class KateConfigPluginPage *m_configPluginPage = nullptr;
};
