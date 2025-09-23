/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

   For the addScrollablePage original
   SPDX-FileCopyrightText: 2003 Benjamin C Meyer <ben+kdelibs at meyerhome dot net>
   SPDX-FileCopyrightText: 2003 Waldo Bastian <bastian@kde.org>
   SPDX-FileCopyrightText: 2004 Michael Brade <brade@kde.org>
   SPDX-FileCopyrightText: 2021 Ahmad Samir <a.samirh78@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateconfigdialog.h"

#include "kateapp.h"
#include "kateconfigplugindialogpage.h"
#include "katedebug.h"
#include "katemainwindow.h"
#include "katepluginmanager.h"
#include "katequickopenmodel.h"
#include "katesessionmanager.h"
#include "ui_sessionconfigwidget.h"

#include <KConfigGroup>
#include <KLocalization>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

KateConfigDialog::KateConfigDialog(KateMainWindow *parent)
    : KPageDialog(parent)
    , m_mainWindow(parent)
{
    setWindowTitle(i18n("Configure"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("configure")));
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Help);

    // we may have a lot of pages on Kate, we want small icons for the list
    if (KateApp::isKate()) {
        setFaceType(KPageDialog::FlatList);
    } else {
        setFaceType(KPageDialog::List);
    }

    const auto listViews = findChildren<QListView *>();
    for (auto *lv : listViews) {
        if (qstrcmp(lv->metaObject()->className(), "KDEPrivate::KPageListView") == 0) {
            m_sideBar = lv;
            break;
        }
    }
    if (!m_sideBar) {
        qWarning("Unable to find config dialog sidebar listview!!");
    }

    // first: add the KTextEditor config pages
    // rational: most people want to alter e.g. the fonts, the colors or some other editor stuff first
    addEditorPages();

    // second: add out own config pages
    // this includes all plugin config pages, added to the bottom
    addBehaviorPage();
    addSessionPage();
    addFeedbackPage();

    // no plugins for KWrite
    if (KateApp::isKate()) {
        addPluginsPage();
        addPluginPages();
    }

    // ensure no stray signals already set this!
    m_dataChanged = false;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);

    // handle dialog actions
    connect(this, &KateConfigDialog::accepted, this, &KateConfigDialog::slotApply);
    connect(buttonBox()->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KateConfigDialog::slotApply);
    connect(buttonBox()->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &KateConfigDialog::slotHelp);
}

void KateConfigDialog::addBehaviorPage()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));

    auto *generalFrame = new QFrame(this);
    KPageWidgetItem *item = addScrollablePage(generalFrame, i18n("Behavior"));
    m_allPages.insert(item);
    item->setHeader(i18n("Behavior Options"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-system-windows-behavior")));

    auto *layout = new QVBoxLayout(generalFrame);

    // PATH setting
    if (KateApp::isKate()) {
        auto *buttonGroup = new QGroupBox(i18n("PATH"), generalFrame);
        auto *vbox = new QVBoxLayout(buttonGroup);
        layout->addWidget(buttonGroup);

        auto label = new QLabel(buttonGroup);
        label->setText(
            i18n("List of %1 separated directories where Kate will look for programs to run. This list will be prepended to your PATH environment variable. "
                 "E.g '/home/USER/myapps/bin/:/home/USER/.local/bin'",
                 QDir::listSeparator()));
        label->setWordWrap(true);

        m_pathEdit = new QLineEdit(buttonGroup);
        m_pathEdit->setObjectName(QStringLiteral("katePATHedit"));
        m_pathEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        m_pathEdit->setText(cgGeneral.readEntry("Kate PATH", QString()));
        connect(m_pathEdit, &QLineEdit::textChanged, this, &KateConfigDialog::slotChanged);

        vbox->addWidget(label);
        vbox->addWidget(m_pathEdit);
    }

    // GROUP with the one below: "Behavior"
    auto *buttonGroup = new QGroupBox(i18n("&Behavior"), generalFrame);
    auto *vbox = new QVBoxLayout;
    layout->addWidget(buttonGroup);

    // shall we try to behave like some SDI application
    m_sdiMode = new QCheckBox(i18n("Open each document in its own window"), buttonGroup);
    m_sdiMode->setChecked(cgGeneral.readEntry("SDI Mode", false));
    m_sdiMode->setToolTip(
        i18n("If enabled, each document will be opened in its own window. "
             "If not enabled, each document will be opened in a new tab in the current window."));
    connect(m_sdiMode, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_sdiMode);

    QHBoxLayout *hlayout = nullptr;
    QLabel *label = nullptr;

    if (KateApp::isKate()) {
        hlayout = new QHBoxLayout;
        label = new QLabel(i18n("&Switch to output view upon message type:"), buttonGroup);
        hlayout->addWidget(label);
        m_messageTypes = new QComboBox(buttonGroup);
        hlayout->addWidget(m_messageTypes);
        label->setBuddy(m_messageTypes);
        m_messageTypes->addItems({i18n("Never"), i18n("Error"), i18n("Warning"), i18n("Info"), i18n("Log")});
        m_messageTypes->setCurrentIndex(cgGeneral.readEntry("Show output view for message type", 1));
        connect(m_messageTypes, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);
        vbox->addLayout(hlayout);

        hlayout = new QHBoxLayout;
        label = new QLabel(i18n("&Limit output view history:"), buttonGroup);
        hlayout->addWidget(label);
        m_outputHistoryLimit = new QSpinBox(buttonGroup);
        hlayout->addWidget(m_outputHistoryLimit);
        label->setBuddy(m_outputHistoryLimit);
        m_outputHistoryLimit->setRange(-1, 10000);
        m_outputHistoryLimit->setSpecialValueText(i18n("Unlimited"));
        m_outputHistoryLimit->setValue(cgGeneral.readEntry("Output History Limit", 100));
        connect(m_outputHistoryLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);
        vbox->addLayout(hlayout);
    }

    // modified files notification
    m_modNotifications = new QCheckBox(i18n("Use a separate &dialog for handling externally modified files"), buttonGroup);
    m_modNotifications->setChecked(m_mainWindow->modNotificationEnabled());
    m_modNotifications->setToolTip(
        i18n("If enabled, a modal dialog will be used to show all of the modified files. "
             "If not enabled, you will be individually asked what to do for each modified file "
             "only when that file's view receives focus."));
    connect(m_modNotifications, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    m_withDate = new QCheckBox(i18n("Output timestamps with date"), buttonGroup);
    m_withDate->setChecked(cgGeneral.readEntry("Output With Date", false));
    m_withDate->setToolTip(i18n("When enabled, the output line timestamp headers will be prefixed with date."));
    connect(m_withDate, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    vbox->addWidget(m_withDate);
    vbox->addWidget(m_modNotifications);
    buttonGroup->setLayout(vbox);

    if (KateApp::isKate()) {
        auto *buttonGroup = new QGroupBox(i18n("&Sidebars"), generalFrame);
        layout->addWidget(buttonGroup);
        auto *vbox = new QVBoxLayout;
        m_syncSectionSizeWithSidebarTabs = new QCheckBox(i18n("Sync section size with tab positions"), buttonGroup);
        m_syncSectionSizeWithSidebarTabs->setChecked(cgGeneral.readEntry("Sync section size with tab positions", false));
        m_syncSectionSizeWithSidebarTabs->setToolTip(
            i18n("When enabled the section size will be determined by the position of the tabs.\n"
                 "This option does not affect the bottom sidebar."));
        connect(m_syncSectionSizeWithSidebarTabs, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
        vbox->addWidget(m_syncSectionSizeWithSidebarTabs);

        m_showTextForLeftRightSidebars = new QCheckBox(i18n("Show text for left and right sidebar buttons"), buttonGroup);
        m_showTextForLeftRightSidebars->setChecked(cgGeneral.readEntry("Show text for left and right sidebar", false));
        connect(m_showTextForLeftRightSidebars, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
        vbox->addWidget(m_showTextForLeftRightSidebars);

        label = new QLabel(i18n("Icon size for left and right sidebar buttons:"), buttonGroup);
        m_leftRightSidebarsIconSize = new QSpinBox(buttonGroup);
        m_leftRightSidebarsIconSize->setMinimum(16);
        m_leftRightSidebarsIconSize->setMaximum(48);
        m_leftRightSidebarsIconSize->setValue(cgGeneral.readEntry("Icon size for left and right sidebar buttons", 32));
        label->setBuddy(m_leftRightSidebarsIconSize);
        connect(m_leftRightSidebarsIconSize, &QSpinBox::textChanged, this, &KateConfigDialog::slotChanged);
        hlayout = new QHBoxLayout;
        hlayout->addWidget(label);
        hlayout->addWidget(m_leftRightSidebarsIconSize);
        vbox->addLayout(hlayout);

        connect(m_showTextForLeftRightSidebars, &QCheckBox::toggled, this, [l = QPointer(label), this](bool v) {
            m_leftRightSidebarsIconSize->setEnabled(!v);
            l->setEnabled(!v);
        });
        buttonGroup->setLayout(vbox);
    }

    // tabbar => we allow to configure some limit on number of tabs to show
    buttonGroup = new QGroupBox(i18n("&Tabs"), generalFrame);
    vbox = new QVBoxLayout;
    buttonGroup->setLayout(vbox);
    hlayout = new QHBoxLayout;
    label = new QLabel(i18n("&Limit number of tabs:"), buttonGroup);
    hlayout->addWidget(label);
    m_tabLimit = new QSpinBox(buttonGroup);
    hlayout->addWidget(m_tabLimit);
    label->setBuddy(m_tabLimit);
    m_tabLimit->setRange(0, 256);
    m_tabLimit->setSpecialValueText(i18n("Unlimited"));
    m_tabLimit->setValue(cgGeneral.readEntry("Tabbar Tab Limit", 0));
    connect(m_tabLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);
    vbox->addLayout(hlayout);
    label =
        new QLabel(i18n("A high limit can increase the window size, please enable 'Allow tab scrolling' to prevent it. Unlimited tabs are always scrollable."));
    label->setWordWrap(true);
    vbox->addWidget(label);

    m_autoHideTabs = new QCheckBox(i18n("&Auto hide tabs"), buttonGroup);
    m_autoHideTabs->setChecked(cgGeneral.readEntry("Auto Hide Tabs", KateApp::isKWrite()));
    m_autoHideTabs->setToolTip(i18n("When checked tabs will be hidden if only one document is open."));
    connect(m_autoHideTabs, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_autoHideTabs);

    m_showTabCloseButton = new QCheckBox(i18n("&Show close button"), buttonGroup);
    m_showTabCloseButton->setChecked(cgGeneral.readEntry("Show Tabs Close Button", true));
    m_showTabCloseButton->setToolTip(i18n("When checked each tab will display a close button."));
    connect(m_showTabCloseButton, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_showTabCloseButton);

    m_expandTabs = new QCheckBox(i18n("&Expand tabs"), buttonGroup);
    m_expandTabs->setChecked(cgGeneral.readEntry("Expand Tabs", false));
    m_expandTabs->setToolTip(i18n("When checked tabs take as much size as possible."));
    connect(m_expandTabs, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_expandTabs);

    m_tabDoubleClickNewDocument = new QCheckBox(i18n("&Double click opens a new document"), buttonGroup);
    m_tabDoubleClickNewDocument->setChecked(cgGeneral.readEntry("Tab Double Click New Document", true));
    m_tabDoubleClickNewDocument->setToolTip(i18n("When checked double click opens a new document."));
    connect(m_tabDoubleClickNewDocument, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabDoubleClickNewDocument);

    m_tabMiddleClickCloseDocument = new QCheckBox(i18n("&Middle click closes a document"), buttonGroup);
    m_tabMiddleClickCloseDocument->setChecked(cgGeneral.readEntry("Tab Middle Click Close Document", true));
    m_tabMiddleClickCloseDocument->setToolTip(i18n("When checked middle click closes a document."));
    connect(m_tabMiddleClickCloseDocument, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabMiddleClickCloseDocument);

    m_tabsScrollable = new QCheckBox(i18n("Allow tab scrolling"), this);
    m_tabsScrollable->setChecked(cgGeneral.readEntry("Allow Tab Scrolling", true));
    m_tabsScrollable->setToolTip(i18n("When checked this will allow scrolling in tab bar when number of tabs is large."));
    connect(m_tabsScrollable, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabsScrollable);

    m_tabsElided = new QCheckBox(i18n("Elide tab text"), this);
    m_tabsElided->setChecked(cgGeneral.readEntry("Elide Tab Text", false));
    m_tabsElided->setToolTip(i18n("When checked tab text might be elided if it is too long."));
    connect(m_tabsElided, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_tabsElided);

    m_openNewTabInFrontOfCurrent = new QCheckBox(i18n("Open new tabs to the right of the current tab"), this);
    m_openNewTabInFrontOfCurrent->setChecked(cgGeneral.readEntry("Open New Tab To The Right Of Current", false));
    m_openNewTabInFrontOfCurrent->setToolTip(i18n("If unchecked the new tab will open at the end."));
    connect(m_openNewTabInFrontOfCurrent, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_openNewTabInFrontOfCurrent);

    m_cycleThroughTabs = new QCheckBox(
        i18nc("'Next Tab'/'Previous Tab' is an existing action name. Please use the same text here as it is used in that action's translation",
              "Allow cycling through tabs. If unchecked, the action 'Next Tab'/'Previous Tab' will not go to first/last tab after reaching the end"),
        this);
    m_cycleThroughTabs->setChecked(cgGeneral.readEntry("Cycle To First Tab", true));
    connect(m_cycleThroughTabs, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);
    vbox->addWidget(m_cycleThroughTabs);

    layout->addWidget(buttonGroup);

    buttonGroup = new QGroupBox(i18n("&Mouse"), generalFrame);
    vbox = new QVBoxLayout;
    layout->addWidget(buttonGroup);

    hlayout = new QHBoxLayout;
    label = new QLabel(i18n("&Backward button pressed:"), buttonGroup);
    hlayout->addWidget(label);
    m_mouseBackActions = new QComboBox(buttonGroup);
    hlayout->addWidget(m_mouseBackActions);
    label->setBuddy(m_mouseBackActions);
    m_mouseBackActions->addItems({i18n("Previous tab"), i18n("History back")});
    m_mouseBackActions->setCurrentIndex(cgGeneral.readEntry("Mouse back button action", 0));
    connect(m_mouseBackActions, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);
    vbox->addLayout(hlayout);

    hlayout = new QHBoxLayout;
    label = new QLabel(i18n("&Forward button pressed:"), buttonGroup);
    hlayout->addWidget(label);
    m_mouseForwardActions = new QComboBox(buttonGroup);
    hlayout->addWidget(m_mouseForwardActions);
    label->setBuddy(m_mouseForwardActions);
    m_mouseForwardActions->addItems({i18n("Next tab"), i18n("History forward")});
    m_mouseForwardActions->setCurrentIndex(cgGeneral.readEntry("Mouse forward button action", 0));
    connect(m_mouseForwardActions, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);
    vbox->addLayout(hlayout);

    buttonGroup->setLayout(vbox);

    /** DIFF **/
    buttonGroup = new QGroupBox(i18n("Diff"), generalFrame);
    vbox = new QVBoxLayout(buttonGroup);
    hlayout = new QHBoxLayout;
    vbox->addLayout(hlayout);
    layout->addWidget(buttonGroup);
    m_diffStyle = new QComboBox(buttonGroup);
    m_diffStyle->addItem(i18n("Side By Side"));
    m_diffStyle->addItem(i18n("Unified"));
    m_diffStyle->addItem(i18n("Raw"));
    label = new QLabel(i18n("Diff Style:"), buttonGroup);
    label->setBuddy(m_diffStyle);
    hlayout->addWidget(label);
    hlayout->addWidget(m_diffStyle);
    m_diffStyle->setCurrentIndex(cgGeneral.readEntry("Diff Show Style", 0));
    connect(m_diffStyle, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &KateConfigDialog::slotChanged);

    buttonGroup = new QGroupBox(i18n("Navigation Bar"), generalFrame);
    vbox = new QVBoxLayout(buttonGroup);
    hlayout = new QHBoxLayout;
    vbox->addLayout(hlayout);
    layout->addWidget(buttonGroup);
    m_urlBarShowSymbols = new QCheckBox(i18n("Show current symbol in navigation bar"), buttonGroup);
    hlayout->addWidget(m_urlBarShowSymbols);
    m_urlBarShowSymbols->setChecked(cgGeneral.readEntry("Show Symbol In Navigation Bar", true));
    connect(m_urlBarShowSymbols, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    buttonGroup = new QGroupBox(i18n("Diagnostics"), generalFrame);
    vbox = new QVBoxLayout(buttonGroup);
    hlayout = new QHBoxLayout;
    vbox->addLayout(hlayout);
    layout->addWidget(buttonGroup);
    label = new QLabel(i18n("Diagnostics limit:"), buttonGroup);
    m_diagnosticsLimit = new QSpinBox(buttonGroup);
    m_diagnosticsLimit->setRange(-1, 50000);
    m_diagnosticsLimit->setSpecialValueText(i18n("Unlimited"));
    m_diagnosticsLimit->setValue(cgGeneral.readEntry("Diagnostics Limit", 12000));
    m_diagnosticsLimit->setToolTip(i18n("Max number of diagnostics allowed in the Diagnostics toolview."));
    label->setBuddy(m_diagnosticsLimit);
    hlayout->addWidget(label);
    hlayout->addWidget(m_diagnosticsLimit);
    connect(m_diagnosticsLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);

    buttonGroup = new QGroupBox(i18n("Context"), generalFrame);
    vbox = new QVBoxLayout(buttonGroup);
    hlayout = new QHBoxLayout;
    vbox->addLayout(hlayout);
    layout->addWidget(buttonGroup);
    m_hintViewEnabled = new QCheckBox(i18n("Enable tool view"), buttonGroup);
    m_hintViewEnabled->setToolTip(
        i18n("A tool view to display context associated with the current "
             "cursor position (LSP hints, diagnostics)"));
    hlayout->addWidget(m_hintViewEnabled);
    m_hintViewEnabled->setChecked(cgGeneral.readEntry("Enable Context ToolView", false));
    connect(m_hintViewEnabled, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    layout->addStretch(1); // :-] works correct without autoadd
}

void KateConfigDialog::addSessionPage()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));

    auto *sessionsPage = new QWidget(this);
    KPageWidgetItem *item = addScrollablePage(sessionsPage, i18n("Session"));
    m_allPages.insert(item);
    item->setHeader(i18n("Session Management"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));

    sessionConfigUi = new Ui::SessionConfigWidget;
    sessionConfigUi->setupUi(sessionsPage);

    // save meta infos
    sessionConfigUi->saveMetaInfos->setChecked(KateApp::self()->documentManager()->getSaveMetaInfos());
    connect(sessionConfigUi->saveMetaInfos, &QGroupBox::toggled, this, &KateConfigDialog::slotChanged);

    // meta infos days
    sessionConfigUi->daysMetaInfos->setMaximum(180);
    sessionConfigUi->daysMetaInfos->setSpecialValueText(i18nc("The special case of 'Delete unused meta-information after'", "(never)"));
    KLocalization::setupSpinBoxFormatString(sessionConfigUi->daysMetaInfos,
                                            ki18ncp("The suffix of 'Delete unused meta-information after'", "%v day", "%v days"));
    sessionConfigUi->daysMetaInfos->setValue(KateApp::self()->documentManager()->getDaysMetaInfos());
    connect(sessionConfigUi->daysMetaInfos, &QSpinBox::valueChanged, this, &KateConfigDialog::slotChanged);

    // restore view  config
    sessionConfigUi->restoreVC->setChecked(cgGeneral.readEntry("Restore Window Configuration", true));
    connect(sessionConfigUi->restoreVC, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    sessionConfigUi->spinBoxRecentFilesCount->setValue(recentFilesMaxCount());
    connect(sessionConfigUi->spinBoxRecentFilesCount, &QSpinBox::valueChanged, this, &KateConfigDialog::slotChanged);

    QString sesStart(cgGeneral.readEntry("Startup Session", "manual"));
    if (sesStart == QLatin1String("new")) {
        sessionConfigUi->startNewSessionRadioButton->setChecked(true);
    } else if (sesStart == QLatin1String("last")) {
        sessionConfigUi->loadLastUserSessionRadioButton->setChecked(true);
    } else {
        sessionConfigUi->manuallyChooseSessionRadioButton->setChecked(true);
    }

    connect(sessionConfigUi->startNewSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi->loadLastUserSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi->manuallyChooseSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);

    // New main windows open always a new document if none there
    sessionConfigUi->showWelcomeViewForNewWindow->setChecked(cgGeneral.readEntry("Show welcome view for new window", true));
    connect(sessionConfigUi->showWelcomeViewForNewWindow, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    // When a window is closed, close all documents only visible in that window, too
    sessionConfigUi->winClosesDocuments->setChecked(cgGeneral.readEntry("Close documents with window", true));
    sessionConfigUi->winClosesDocuments->setToolTip(i18n("When a window is closed the documents opened only in this window are closed as well."));
    connect(sessionConfigUi->winClosesDocuments, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    // Closing last file closes Kate
    sessionConfigUi->modCloseAfterLast->setChecked(m_mainWindow->modCloseAfterLast());
    connect(sessionConfigUi->modCloseAfterLast, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    // stash unsave changes
    sessionConfigUi->stashNewUnsavedFiles->setChecked(KateApp::self()->stashManager()->stashNewUnsavedFiles);
    sessionConfigUi->stashUnsavedFilesChanges->setChecked(KateApp::self()->stashManager()->stashUnsavedChanges);
    connect(sessionConfigUi->stashNewUnsavedFiles, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi->stashUnsavedFilesChanges, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);

    // simplify the session page for KWrite
    if (KateApp::isKWrite()) {
        sessionConfigUi->gbAppStartup->hide();
        sessionConfigUi->restoreVC->hide();
        sessionConfigUi->label_4->hide();
        sessionConfigUi->stashNewUnsavedFiles->hide();
        sessionConfigUi->stashUnsavedFilesChanges->hide();
        sessionConfigUi->label->hide();
    }
}

void KateConfigDialog::addPluginsPage()
{
    auto *page = new QFrame(this);
    auto *vlayout = new QVBoxLayout(page);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    m_configPluginPage = new KateConfigPluginPage(page, this);
    vlayout->addWidget(m_configPluginPage);
    connect(m_configPluginPage, &KateConfigPluginPage::changed, this, &KateConfigDialog::slotChanged);

    KPageWidgetItem *item = addScrollablePage(page, i18n("Plugins"));
    m_allPages.insert(item);
    item->setHeader(i18n("Plugin Manager"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-plugin")));
}

void KateConfigDialog::addFeedbackPage()
{
#ifdef WITH_KUSERFEEDBACK
    // KUserFeedback Config
    auto page = new QFrame(this);
    auto vlayout = new QVBoxLayout(page);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    m_userFeedbackWidget = new KUserFeedback::FeedbackConfigWidget(page);
    m_userFeedbackWidget->setFeedbackProvider(KateApp::self()->userFeedbackProvider());
    connect(m_userFeedbackWidget, &KUserFeedback::FeedbackConfigWidget::configurationChanged, this, &KateConfigDialog::slotChanged);
    vlayout->addWidget(m_userFeedbackWidget);

    KPageWidgetItem *item = addScrollablePage(page, i18n("User Feedback"));
    m_allPages.insert(item);
    item->setHeader(i18n("User Feedback"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-locale")));
#endif
}

void KateConfigDialog::addPluginPages()
{
    const KatePluginList &pluginList(KateApp::self()->pluginManager()->pluginList());
    for (const KatePluginInfo &plugin : pluginList) {
        if (plugin.plugin) {
            addPluginPage(plugin.plugin);
        }
    }
}

void KateConfigDialog::addEditorPages()
{
    for (int i = 0; i < KTextEditor::Editor::instance()->configPages(); ++i) {
        KTextEditor::ConfigPage *page = KTextEditor::Editor::instance()->configPage(i, this);
        connect(page, &KTextEditor::ConfigPage::changed, this, &KateConfigDialog::slotChanged);
        m_editorPages.push_back(page);
        KPageWidgetItem *item = addScrollablePage(page, page->name());
        item->setHeader(page->fullName());
        item->setIcon(page->icon());
        m_allPages.insert(item);
    }
}

void KateConfigDialog::addPluginPage(KTextEditor::Plugin *plugin)
{
    for (int i = 0; i < plugin->configPages(); i++) {
        KTextEditor::ConfigPage *cp = plugin->configPage(i, this);
        KPageWidgetItem *item = addScrollablePage(cp, cp->name());
        item->setHeader(cp->fullName());
        item->setIcon(cp->icon());

        PluginPageListItem info;
        info.plugin = plugin;
        info.pluginPage = cp;
        info.idInPlugin = i;
        info.pageWidgetItem = item;
        connect(info.pluginPage, &KTextEditor::ConfigPage::changed, this, &KateConfigDialog::slotChanged);
        m_pluginPages.insert(item, info);
        m_allPages.insert(item);
    }
}

void KateConfigDialog::removePluginPage(KTextEditor::Plugin *plugin)
{
    std::vector<KPageWidgetItem *> remove;
    for (QHash<KPageWidgetItem *, PluginPageListItem>::const_iterator it = m_pluginPages.constBegin(); it != m_pluginPages.constEnd(); ++it) {
        const PluginPageListItem &pli = it.value();

        if (pli.plugin == plugin) {
            remove.push_back(it.key());
        }
    }

    qCDebug(LOG_KATE, "%zu", remove.size());
    while (!remove.empty()) {
        KPageWidgetItem *wItem = remove.back();
        remove.pop_back();
        PluginPageListItem item = m_pluginPages.take(wItem);
        m_allPages.remove(wItem);
        delete item.pluginPage;
        removePage(wItem);
    }
}

void KateConfigDialog::slotApply()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    // if data changed apply the kate app stuff
    if (m_dataChanged) {
        // apply plugin load state changes
        if (m_configPluginPage) {
            m_configPluginPage->slotApply();
        }

        KConfigGroup cg(config, QStringLiteral("General"));

        cg.writeEntry("SDI Mode", m_sdiMode->isChecked());

        // only there for kate
        if (m_syncSectionSizeWithSidebarTabs) {
            cg.writeEntry("Sync section size with tab positions", m_syncSectionSizeWithSidebarTabs->isChecked());
        }
        if (m_showTextForLeftRightSidebars) {
            cg.writeEntry("Show text for left and right sidebar", m_showTextForLeftRightSidebars->isChecked());
        }
        if (m_leftRightSidebarsIconSize) {
            cg.writeEntry("Icon size for left and right sidebar buttons", m_leftRightSidebarsIconSize->value());
        }

        cg.writeEntry("Restore Window Configuration", sessionConfigUi->restoreVC->isChecked());

        cg.writeEntry("Recent File List Entry Count", sessionConfigUi->spinBoxRecentFilesCount->value());

        if (sessionConfigUi->startNewSessionRadioButton->isChecked()) {
            cg.writeEntry("Startup Session", "new");
        } else if (sessionConfigUi->loadLastUserSessionRadioButton->isChecked()) {
            cg.writeEntry("Startup Session", "last");
        } else {
            cg.writeEntry("Startup Session", "manual");
        }

        cg.writeEntry("Save Meta Infos", sessionConfigUi->saveMetaInfos->isChecked());
        KateApp::self()->documentManager()->setSaveMetaInfos(sessionConfigUi->saveMetaInfos->isChecked());

        cg.writeEntry("Days Meta Infos", sessionConfigUi->daysMetaInfos->value());
        KateApp::self()->documentManager()->setDaysMetaInfos(sessionConfigUi->daysMetaInfos->value());

        cg.writeEntry("Show welcome view for new window", sessionConfigUi->showWelcomeViewForNewWindow->isChecked());

        cg.writeEntry("Close documents with window", sessionConfigUi->winClosesDocuments->isChecked());

        cg.writeEntry("Close After Last", sessionConfigUi->modCloseAfterLast->isChecked());
        m_mainWindow->setModCloseAfterLast(sessionConfigUi->modCloseAfterLast->isChecked());

        if (m_messageTypes && m_outputHistoryLimit) {
            cg.writeEntry("Show output view for message type", m_messageTypes->currentIndex());
            cg.writeEntry("Output History Limit", m_outputHistoryLimit->value());
        }

        cg.writeEntry("Mouse back button action", m_mouseBackActions->currentIndex());
        cg.writeEntry("Mouse forward button action", m_mouseForwardActions->currentIndex());

        cg.writeEntry("Stash unsaved file changes", sessionConfigUi->stashUnsavedFilesChanges->isChecked());
        KateApp::self()->stashManager()->stashUnsavedChanges = sessionConfigUi->stashUnsavedFilesChanges->isChecked();
        cg.writeEntry("Stash new unsaved files", sessionConfigUi->stashNewUnsavedFiles->isChecked());
        KateApp::self()->stashManager()->stashNewUnsavedFiles = sessionConfigUi->stashNewUnsavedFiles->isChecked();

        cg.writeEntry("Modified Notification", m_modNotifications->isChecked());
        m_mainWindow->setModNotificationEnabled(m_modNotifications->isChecked());

        cg.writeEntry("Tabbar Tab Limit", m_tabLimit->value());

        cg.writeEntry("Auto Hide Tabs", m_autoHideTabs->isChecked());

        cg.writeEntry("Show Tabs Close Button", m_showTabCloseButton->isChecked());

        cg.writeEntry("Expand Tabs", m_expandTabs->isChecked());

        cg.writeEntry("Tab Double Click New Document", m_tabDoubleClickNewDocument->isChecked());
        cg.writeEntry("Tab Middle Click Close Document", m_tabMiddleClickCloseDocument->isChecked());

        cg.writeEntry("Allow Tab Scrolling", m_tabsScrollable->isChecked());
        cg.writeEntry("Elide Tab Text", m_tabsElided->isChecked());
        cg.writeEntry("Open New Tab To The Right Of Current", m_openNewTabInFrontOfCurrent->isChecked());
        cg.writeEntry("Cycle To First Tab", m_cycleThroughTabs->isChecked());

        cg.writeEntry("Diff Show Style", m_diffStyle->currentIndex());

        cg.writeEntry("Show Symbol In Navigation Bar", m_urlBarShowSymbols->isChecked());

        cg.writeEntry("Diagnostics Limit", m_diagnosticsLimit->value());
        cg.writeEntry("Output With Date", m_withDate->isChecked());

        cg.writeEntry("Enable Context ToolView", m_hintViewEnabled->isChecked());

        // patch document modified warn state
        const QList<KTextEditor::Document *> &docs = KateApp::self()->documentManager()->documentList();
        for (KTextEditor::Document *doc : docs) {
            doc->setModifiedOnDiskWarning(!m_modNotifications->isChecked());
        }

        m_mainWindow->saveOptions();

        // save plugin config !!
        KateSessionManager *sessionmanager = KateApp::self()->sessionManager();
        KConfig *sessionConfig = sessionmanager->activeSession()->config();
        KateApp::self()->pluginManager()->writeConfig(sessionConfig);

#ifdef WITH_KUSERFEEDBACK
        // set current active mode + write back the config for future starts
        KateApp::self()->userFeedbackProvider()->setTelemetryMode(m_userFeedbackWidget->telemetryMode());
        KateApp::self()->userFeedbackProvider()->setSurveyInterval(m_userFeedbackWidget->surveyInterval());
#endif

        if (KateApp::isKate()) {
            const QString existingPATH = cg.readEntry("Kate PATH", QString());
            const QString newPATH = m_pathEdit->text();
            const QChar seperator = QDir::listSeparator();
            // check if it changed
            if (existingPATH != newPATH) {
                // validate entries, check they exist
                const QStringList paths = newPATH.split(seperator, Qt::SkipEmptyParts);
                QStringList validatedPaths;
                for (const auto &p : paths) {
                    if (!QFileInfo::exists(p)) {
                        QMessageBox::warning(this, i18n("Bad PATH value"), i18n("The path '%1' doesn't exist", p));
                    } else {
                        validatedPaths.append(p);
                    }
                }

                // Remove existing user set path
                QString oldPATH = QString::fromUtf8(qgetenv("PATH"));
                if (oldPATH.startsWith(existingPATH)) {
                    oldPATH.remove(0, existingPATH.length());
                    if (oldPATH.startsWith(seperator)) {
                        oldPATH.remove(0, 1);
                    }
                }

                // prepend to PATH
                const QString p = validatedPaths.join(seperator);
                const QString newPATH = p + seperator + oldPATH;
                // putenv the new PATH
                qputenv("PATH", newPATH.toUtf8());

                // write out the setting
                cg.writeEntry("Kate PATH", p);
            }
        }
    }

    for (const PluginPageListItem &plugin : std::as_const(m_pluginPages)) {
        if (plugin.pluginPage) {
            plugin.pluginPage->apply();
        }
    }

    // apply ktexteditor pages
    for (KTextEditor::ConfigPage *page : m_editorPages) {
        page->apply();
    }

    config->sync();

    // emit config change
    if (m_dataChanged) {
        KateApp::self()->configurationChanged();
    }

    m_dataChanged = false;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void KateConfigDialog::slotChanged()
{
    m_dataChanged = true;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void KateConfigDialog::showAppPluginPage(KTextEditor::Plugin *p, int id)
{
    for (const PluginPageListItem &plugin : std::as_const(m_pluginPages)) {
        if ((plugin.plugin == p) && (id == plugin.idInPlugin)) {
            setCurrentPage(plugin.pageWidgetItem);
            break;
        }
    }
}

void KateConfigDialog::slotHelp()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("help:/")));
}

int KateConfigDialog::recentFilesMaxCount()
{
    int maxItems = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General")).readEntry("Recent File List Entry Count", 10);
    return maxItems;
}

void KateConfigDialog::closeEvent(QCloseEvent *event)
{
    if (!m_dataChanged) {
        event->accept();
        return;
    }

    const auto response = KMessageBox::warningTwoActionsCancel(this,
                                                               i18n("You have unsaved changes. Do you want to apply the changes or discard them?"),
                                                               i18n("Warning"),
                                                               KStandardGuiItem::save(),
                                                               KStandardGuiItem::discard(),
                                                               KStandardGuiItem::cancel());
    switch (response) {
    case KMessageBox::PrimaryAction:
        slotApply();
        Q_FALLTHROUGH();
    case KMessageBox::SecondaryAction:
        event->accept();
        break;
    case KMessageBox::Cancel:
        event->ignore();
        break;
    default:
        break;
    }
}

KPageWidgetItem *KateConfigDialog::addScrollablePage(QWidget *page, const QString &itemName)
{
    // inspired by KPageWidgetItem *KConfigDialogPrivate::addPageInternal(QWidget *page, const QString &itemName, const QString &pixmapName, const QString
    // &header)
    auto *frame = new QWidget(this);
    auto *boxLayout = new QVBoxLayout(frame);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(frame);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    scroll->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    boxLayout->addWidget(scroll);
    return addPage(frame, itemName);
}

#include "moc_kateconfigdialog.cpp"
