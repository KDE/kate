/*
    SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __kate_configdialog_h__
#define __kate_configdialog_h__

#include <QPointer>

#include <KTextEditor/ConfigPage>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/Plugin>

#include <KPageDialog>

#ifdef WITH_KUSERFEEDBACK
#include <KUserFeedback/FeedbackConfigWidget>
#endif

#include "ui_sessionconfigwidget.h"

class QCheckBox;
class QComboBox;
class QSpinBox;
class KateMainWindow;
class KPluralHandlingSpinBox;

struct PluginPageListItem {
    KTextEditor::Plugin *plugin;
    int idInPlugin;
    KTextEditor::ConfigPage *pluginPage;
    KPageWidgetItem *pageWidgetItem;
};

class KateConfigDialog : public KPageDialog
{
    Q_OBJECT

private:
    // No manual construction. We will always have one
    // dialog around
    KateConfigDialog(KateMainWindow *parent);

public: // static
    /**
     * Reads the value from the given open config. If not present in config yet then
     * the default value 10 is used.
     */
    static int recentFilesMaxCount();

    // Returns the dialog
    static KateConfigDialog *widget(KateMainWindow *mw);

    KateMainWindow *mainwWindow()
    {
        return m_mainWindow;
    }

    Q_INVOKABLE bool shouldClose()
    {
        return true;
    }

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
    bool event(QEvent *event) override;

private:
    void addBehaviorPage();
    void addSessionPage();
    void addPluginsPage();
    void addFeedbackPage();
    void addPluginPages();
    void addEditorPages();

    // add page variant that ensures the page is wrapped into a QScrollArea
    KPageWidgetItem *addScrollablePage(QWidget *page, const QString &itemName);

    void onSearchTextChanged();

private:
    KateMainWindow *m_mainWindow;

    bool m_dataChanged = false;

    QLineEdit *const m_searchLineEdit;
    QTimer *const m_searchTimer;

    QComboBox *m_messageTypes = nullptr;
    QSpinBox *m_outputHistoryLimit = nullptr;
    QComboBox *m_mouseBackActions = nullptr;
    QComboBox *m_mouseForwardActions = nullptr;
    QCheckBox *m_modNotifications;
    QCheckBox *m_syncSectionSizeWithSidebarTabs = nullptr;
    QCheckBox *m_showTextForLeftRightSidebars = nullptr;
    QSpinBox *m_leftRightSidebarsIconSize = nullptr;
    QComboBox *m_cmbQuickOpenListMode;
    QSpinBox *m_tabLimit;
    QCheckBox *m_autoHideTabs;
    QCheckBox *m_showTabCloseButton;
    QCheckBox *m_expandTabs;
    QCheckBox *m_tabDoubleClickNewDocument;
    QCheckBox *m_tabMiddleClickCloseDocument;
    QCheckBox *m_tabsScrollable = nullptr;
    QCheckBox *m_tabsElided = nullptr;
    QComboBox *m_diffStyle = nullptr;
    QCheckBox *m_urlBarShowSymbols = nullptr;

    Ui::SessionConfigWidget sessionConfigUi;

    QHash<KPageWidgetItem *, PluginPageListItem> m_pluginPages;
    QList<KTextEditor::ConfigPage *> m_editorPages;

    QPointer<class QListView> m_sideBar;
    QSet<KPageWidgetItem *> m_allPages;
    QVector<QWidget *> m_searchMatchOverlays;

#ifdef WITH_KUSERFEEDBACK
    KUserFeedback::FeedbackConfigWidget *m_userFeedbackWidget = nullptr;
#endif

Q_SIGNALS:
    void saved();
};

#endif
