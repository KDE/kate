/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateconfigdialog.h"

#include "ui_sessionconfigwidget.h"

#include "katemainwindow.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katesessionmanager.h"
#include "katedebug.h"

#include <KTextEditor/ConfigPage>

#include <KStandardAction>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KPluralHandlingSpinBox>

#include <QDesktopServices>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

KateConfigDialog::KateConfigDialog(KateMainWindow *parent, KTextEditor::View *view)
    : KPageDialog(parent)
    , m_mainWindow(parent)
    , m_view(view)
{
    setFaceType(Tree);
    setWindowTitle(i18n("Configure"));
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    setObjectName(QStringLiteral("configdialog"));

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cgGeneral = KConfigGroup(config, "General");

    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);

    KPageWidgetItem *applicationItem = addPage(new QWidget, i18n("Application"));
    applicationItem->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));
    applicationItem->setHeader(i18n("Application Options"));
    applicationItem->setCheckable(false);
    applicationItem->setEnabled(false);
    m_applicationPage = applicationItem;

    //BEGIN General page
    QFrame *generalFrame = new QFrame;
    KPageWidgetItem *item = addSubPage(applicationItem, generalFrame, i18n("General"));
    item->setHeader(i18n("General Options"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("go-home")));
    setCurrentPage(item);

    QVBoxLayout *layout = new QVBoxLayout(generalFrame);
    layout->setMargin(0);

    // GROUP with the one below: "Behavior"
    QGroupBox *buttonGroup = new QGroupBox(i18n("&Behavior"), generalFrame);
    QVBoxLayout *vbox = new QVBoxLayout;
    layout->addWidget(buttonGroup);

    // modified files notification
    m_modNotifications = new QCheckBox(
        i18n("Wa&rn about files modified by foreign processes"), buttonGroup);
    m_modNotifications->setChecked(parent->modNotificationEnabled());
    m_modNotifications->setWhatsThis(i18n(
                                         "If enabled, when Kate receives focus you will be asked what to do with "
                                         "files that have been modified on the hard disk. If not enabled, you will "
                                         "be asked what to do with a file that has been modified on the hard disk only "
                                         "when that file is tried to be saved."));
    connect(m_modNotifications, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    vbox->addWidget(m_modNotifications);
    buttonGroup->setLayout(vbox);

    // GROUP with the one below: "Meta-information"
    buttonGroup = new QGroupBox(i18n("Meta-Information"), generalFrame);
    vbox = new QVBoxLayout;
    layout->addWidget(buttonGroup);

    // save meta infos
    m_saveMetaInfos = new QCheckBox(buttonGroup);
    m_saveMetaInfos->setText(i18n("Keep &meta-information past sessions"));
    m_saveMetaInfos->setChecked(KateApp::self()->documentManager()->getSaveMetaInfos());
    m_saveMetaInfos->setWhatsThis(i18n(
                                      "Check this if you want document configuration like for example "
                                      "bookmarks to be saved past editor sessions. The configuration will be "
                                      "restored if the document has not changed when reopened."));
    connect(m_saveMetaInfos, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    vbox->addWidget(m_saveMetaInfos);

    // meta infos days
    QFrame *metaInfos = new QFrame(buttonGroup);
    QHBoxLayout *hlayout = new QHBoxLayout(metaInfos);

    metaInfos->setEnabled(KateApp::self()->documentManager()->getSaveMetaInfos());
    QLabel *label = new QLabel(i18n("&Delete unused meta-information after:"), metaInfos);
    hlayout->addWidget(label);
    m_daysMetaInfos = new KPluralHandlingSpinBox(metaInfos);
    m_daysMetaInfos->setMaximum(180);
    m_daysMetaInfos->setSpecialValueText(i18nc("The special case of 'Delete unused meta-information after'", "(never)"));
    m_daysMetaInfos->setSuffix(ki18ncp("The suffix of 'Delete unused meta-information after'", " day", " days"));
    m_daysMetaInfos->setValue(KateApp::self()->documentManager()->getDaysMetaInfos());
    hlayout->addWidget(m_daysMetaInfos);
    label->setBuddy(m_daysMetaInfos);
    connect(m_saveMetaInfos, &QCheckBox::toggled, metaInfos, &QFrame::setEnabled);
    connect(m_daysMetaInfos, static_cast<void (KPluralHandlingSpinBox::*)(int)>(&KPluralHandlingSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);

    vbox->addWidget(metaInfos);
    buttonGroup->setLayout(vbox);

    layout->addStretch(1); // :-] works correct without autoadd
    //END General page

    //BEGIN Session page
    QWidget *sessionsPage = new QWidget();
    item = addSubPage(applicationItem, sessionsPage, i18n("Sessions"));
    item->setHeader(i18n("Session Management"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));

    sessionConfigUi = new Ui::SessionConfigWidget();
    sessionConfigUi->setupUi(sessionsPage);

    // restore view  config
    sessionConfigUi->restoreVC->setChecked( cgGeneral.readEntry("Restore Window Configuration", true) );
    connect(sessionConfigUi->restoreVC, &QCheckBox::toggled, this, &KateConfigDialog::slotChanged);

    sessionConfigUi->spinBoxRecentFilesCount->setValue(recentFilesMaxCount());
    connect(sessionConfigUi->spinBoxRecentFilesCount, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KateConfigDialog::slotChanged);

    QString sesStart (cgGeneral.readEntry ("Startup Session", "manual"));
    if (sesStart == QStringLiteral("new"))
        sessionConfigUi->startNewSessionRadioButton->setChecked (true);
    else if (sesStart == QStringLiteral("last"))
        sessionConfigUi->loadLastUserSessionRadioButton->setChecked (true);
    else
        sessionConfigUi->manuallyChooseSessionRadioButton->setChecked (true);

    connect(sessionConfigUi->startNewSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi->loadLastUserSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    connect(sessionConfigUi->manuallyChooseSessionRadioButton, &QRadioButton::toggled, this, &KateConfigDialog::slotChanged);
    //END Session page

    //BEGIN Plugins page
    QFrame *page = new QFrame(this);
    QVBoxLayout *vlayout = new QVBoxLayout(page);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
    vlayout->addWidget(configPluginPage);
    connect(configPluginPage, &KateConfigPluginPage::changed, this, &KateConfigDialog::slotChanged);

    item = addSubPage(applicationItem, page, i18n("Plugins"));
    item->setHeader(i18n("Plugin Manager"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-plugin")));

    KatePluginList &pluginList(KateApp::self()->pluginManager()->pluginList());
    foreach(const KatePluginInfo & plugin, pluginList) {
        if (plugin.load) {
            addPluginPage(plugin.plugin);
        }
    }
    //END Plugins page

// editor widgets from kwrite/kwdialog
    m_editorPage = addPage(new QWidget, i18n("Editor Component"));
    m_editorPage->setIcon(QIcon::fromTheme(QStringLiteral("accessories-text-editor")));
    m_editorPage->setHeader(i18n("Editor Component Options"));
    m_editorPage->setCheckable(false);
    m_editorPage->setEnabled(false);

    addEditorPages();

    connect(this, &KateConfigDialog::accepted, this, &KateConfigDialog::slotApply);
    connect(buttonBox()->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KateConfigDialog::slotApply);
    connect(buttonBox()->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &KateConfigDialog::slotHelp);
    connect(this, &KateConfigDialog::currentPageChanged, this, &KateConfigDialog::slotCurrentPageChanged);
    m_dataChanged = false;

    resize(minimumSizeHint());
}

KateConfigDialog::~KateConfigDialog()
{
    delete sessionConfigUi;
}

void KateConfigDialog::addEditorPages()
{

    for (int i = 0; i < KTextEditor::Editor::instance()->configPages(); ++i) {
        KTextEditor::ConfigPage *page = KTextEditor::Editor::instance()->configPage(i, this);
        connect(page, &KTextEditor::ConfigPage::changed, this, &KateConfigDialog::slotChanged);
        m_editorPages.push_back(page);
        KPageWidgetItem *item = addSubPage(m_editorPage, page, page->name());
        item->setHeader(page->fullName());
        item->setIcon(page->icon());
    }
}

void KateConfigDialog::addPluginPage(KTextEditor::Plugin *plugin)
{
    for (int i = 0; i < plugin->configPages(); i++) {
        QFrame *page = new QFrame();
        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setSpacing(0);
        layout->setMargin(0);

        KTextEditor::ConfigPage *cp = plugin->configPage(i, page);
        page->layout()->addWidget(cp);

        KPageWidgetItem *item = addSubPage(m_applicationPage, page, cp->name());
        item->setHeader(cp->fullName());
        item->setIcon(cp->icon());

        PluginPageListItem *info = new PluginPageListItem;
        info->plugin = plugin;
        info->pageParent = page;
        info->pluginPage = cp;
        info->idInPlugin = i;
        info->pageWidgetItem = item;
        connect(info->pluginPage, &KTextEditor::ConfigPage::changed, this, &KateConfigDialog::slotChanged);
        m_pluginPages.insert(item, info);
    }
}

void KateConfigDialog::slotCurrentPageChanged(KPageWidgetItem *current, KPageWidgetItem * /*before*/)
{
    PluginPageListItem *info = m_pluginPages[current];
    if (!info) {
        return;
    }
    if (info->pluginPage) {
        return;
    }
    qCDebug(LOG_KATE) << "creating config page (shouldnt get here)";
    info->pluginPage = info->plugin->configPage(info->idInPlugin, info->pageParent);
    info->pageParent->layout()->addWidget(info->pluginPage);
    info->pluginPage->show();
    connect(info->pluginPage, &KTextEditor::ConfigPage::changed, this, &KateConfigDialog::slotChanged);
}

void KateConfigDialog::removePluginPage(KTextEditor::Plugin *plugin)
{
    QList<KPageWidgetItem *> remove;
    for (QHash<KPageWidgetItem *, PluginPageListItem *>::const_iterator it = m_pluginPages.constBegin(); it != m_pluginPages.constEnd(); ++it) {
        PluginPageListItem *pli = it.value();
        if (!pli) {
            continue;
        }
        if (pli->plugin == plugin) {
            remove.append(it.key());
        }
    }

    qCDebug(LOG_KATE) << remove.count();
    while (!remove.isEmpty()) {
        KPageWidgetItem *wItem = remove.takeLast();
        PluginPageListItem *pItem = m_pluginPages.take(wItem);
        delete pItem->pluginPage;
        delete pItem->pageParent;
        removePage(wItem);
        delete pItem;
    }
}

void KateConfigDialog::slotApply()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    // if data changed apply the kate app stuff
    if (m_dataChanged) {
        KConfigGroup cg = KConfigGroup(config, "General");

        cg.writeEntry("Restore Window Configuration", sessionConfigUi->restoreVC->isChecked());

        cg.writeEntry("Recent File List Entry Count", sessionConfigUi->spinBoxRecentFilesCount->value());

        if (sessionConfigUi->startNewSessionRadioButton->isChecked()) {
            cg.writeEntry("Startup Session", "new");
        } else if (sessionConfigUi->loadLastUserSessionRadioButton->isChecked()) {
            cg.writeEntry("Startup Session", "last");
        } else {
            cg.writeEntry("Startup Session", "manual");
        }

        cg.writeEntry("Save Meta Infos", m_saveMetaInfos->isChecked());
        KateApp::self()->documentManager()->setSaveMetaInfos(m_saveMetaInfos->isChecked());

        cg.writeEntry("Days Meta Infos", m_daysMetaInfos->value());
        KateApp::self()->documentManager()->setDaysMetaInfos(m_daysMetaInfos->value());

        cg.writeEntry("Modified Notification", m_modNotifications->isChecked());
        m_mainWindow->setModNotificationEnabled(m_modNotifications->isChecked());

        // patch document modified warn state
        const QList<KTextEditor::Document *> &docs = KateApp::self()->documentManager()->documentList();
        foreach(KTextEditor::Document * doc, docs)
        if (qobject_cast<KTextEditor::ModificationInterface *>(doc)) {
            qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning(!m_modNotifications->isChecked());
        }

        m_mainWindow->saveOptions();

        // save plugin config !!
        KateSessionManager *sessionmanager = KateApp::self()->sessionManager();
        KConfig *sessionConfig = sessionmanager->activeSession()->config();
        KateApp::self()->pluginManager()->writeConfig(sessionConfig);
    }

    foreach(PluginPageListItem * plugin, m_pluginPages) {
        if (!plugin) {
            continue;
        }
        if (plugin->pluginPage) {
            plugin->pluginPage->apply();
        }
    }

    // apply ktexteditor pages
    foreach(KTextEditor::ConfigPage * page, m_editorPages)
    page->apply();

    config->sync();

    m_dataChanged = false;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void KateConfigDialog::slotChanged()
{
    m_dataChanged = true;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void KateConfigDialog::showAppPluginPage(KTextEditor::Plugin *p, uint id)
{
    foreach(PluginPageListItem * plugin, m_pluginPages) {
        if ((plugin->plugin == p) && (id == plugin->idInPlugin)) {
            setCurrentPage(plugin->pageWidgetItem);
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
    int maxItems = KConfigGroup(KSharedConfig::openConfig(), "General").readEntry("Recent File List Entry Count", 10);
    return maxItems;
}

