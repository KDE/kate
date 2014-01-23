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

#include "katemainwindow.h"

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katesessionmanager.h"
#include "katedebug.h"

#include <ktexteditor/configpage.h>

#include <KIconLoader>
#include <KStandardAction>
#include <KLocalizedString>
#include <KConfigGroup>

#include <QDesktopServices>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
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
    connect(m_modNotifications, SIGNAL(toggled(bool)),
            this, SLOT(slotChanged()));

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
    connect(m_saveMetaInfos, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    vbox->addWidget(m_saveMetaInfos);

    // meta infos days
    QFrame *metaInfos = new QFrame(buttonGroup);
    QHBoxLayout *hlayout = new QHBoxLayout(metaInfos);

    metaInfos->setEnabled(KateApp::self()->documentManager()->getSaveMetaInfos());
    QLabel *label = new QLabel(i18n("&Delete unused meta-information after:"), metaInfos);
    hlayout->addWidget(label);
    m_daysMetaInfos = new QSpinBox(metaInfos);
    m_daysMetaInfos->setMaximum(180);
    m_daysMetaInfos->setSpecialValueText(i18n("(never)"));
    m_daysMetaInfos->setValue(KateApp::self()->documentManager()->getDaysMetaInfos());
    hlayout->addWidget(m_daysMetaInfos);
    label->setBuddy(m_daysMetaInfos);
    connect(m_saveMetaInfos, SIGNAL(toggled(bool)), metaInfos, SLOT(setEnabled(bool)));
    connect(m_daysMetaInfos, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));

    vbox->addWidget(metaInfos);
    buttonGroup->setLayout(vbox);

    layout->addStretch(1); // :-] works correct without autoadd
    //END General page

    //BEGIN Session page
    QFrame *sessionsFrame = new QFrame;
    item = addSubPage(applicationItem, sessionsFrame, i18n("Sessions"));
    item->setHeader(i18n("Session Management"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));

    layout = new QVBoxLayout(sessionsFrame);
    layout->setMargin(0);

    // GROUP with the one below: "Startup"
    buttonGroup = new QGroupBox(i18n("Elements of Sessions"), sessionsFrame);
    vbox = new QVBoxLayout;
    layout->addWidget(buttonGroup);

    // restore view  config
    m_restoreVC = new QCheckBox(buttonGroup);
    m_restoreVC->setText(i18n("Include &window configuration"));
    m_restoreVC->setChecked(cgGeneral.readEntry("Restore Window Configuration", true));
    m_restoreVC->setWhatsThis(i18n("Check this if you want all your views and frames restored each time you open Kate"));
    connect(m_restoreVC, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    vbox->addWidget(m_restoreVC);
    buttonGroup->setLayout(vbox);

    QGroupBox *sessionsStart = new QGroupBox(i18n("Behavior on Application Startup"), sessionsFrame);
    vbox = new QVBoxLayout;
    layout->addWidget(sessionsStart);

    m_startNewSessionRadioButton = new QRadioButton(i18n("&Start new session"), sessionsStart);
    m_loadLastUserSessionRadioButton = new QRadioButton(i18n("&Load last-used session"), sessionsStart);
    m_manuallyChooseSessionRadioButton = new QRadioButton(i18n("&Manually choose a session"), sessionsStart);

    QString sesStart(cgGeneral.readEntry("Startup Session", "manual"));
    if (sesStart == QStringLiteral("new")) {
        m_startNewSessionRadioButton->setChecked(true);
    } else if (sesStart == QStringLiteral("last")) {
        m_loadLastUserSessionRadioButton->setChecked(true);
    } else {
        m_manuallyChooseSessionRadioButton->setChecked(true);
    }

    connect(m_startNewSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(m_loadLastUserSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
    connect(m_manuallyChooseSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

    vbox->addWidget(m_startNewSessionRadioButton);
    vbox->addWidget(m_loadLastUserSessionRadioButton);
    vbox->addWidget(m_manuallyChooseSessionRadioButton);
    sessionsStart->setLayout(vbox);

    layout->addStretch(1); // :-] works correct without autoadd
    //END Session page

    //BEGIN Plugins page
    QFrame *page = new QFrame(this);
    QVBoxLayout *vlayout = new QVBoxLayout(page);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
    vlayout->addWidget(configPluginPage);
    connect(configPluginPage, SIGNAL(changed()), this, SLOT(slotChanged()));

    item = addSubPage(applicationItem, page, i18n("Plugins"));
    item->setHeader(i18n("Plugin Manager"));
    item->setIcon(QIcon::fromTheme(QStringLiteral("preferences-plugin")));

    KatePluginList &pluginList(KateApp::self()->pluginManager()->pluginList());
    foreach(const KatePluginInfo & plugin, pluginList) {
        if (plugin.load
                && qobject_cast<KTextEditor::ConfigPageInterface *>(plugin.plugin)) {
            addPluginPage(plugin.plugin);
        }
    }
    //END Plugins page

// editor widgets from kwrite/kwdialog
    m_editorPage = addPage(new QWidget, i18n("Editor Component"));
    m_editorPage->setIcon(QIcon::fromTheme(QStringLiteral("accessories-text-editor")));
    m_editorPage->setHeader(i18n("Editor Component Options"));

    addEditorPages();

    connect(this, SIGNAL(accepted()), this, SLOT(slotApply()));
    connect(buttonBox()->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(slotApply()));
    connect(buttonBox()->button(QDialogButtonBox::Help), SIGNAL(clicked()), this, SLOT(slotHelp()));
    connect(this, SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)),
            this, SLOT(slotCurrentPageChanged(KPageWidgetItem*,KPageWidgetItem*)));
    m_dataChanged = false;

    resize(minimumSizeHint());
}

KateConfigDialog::~KateConfigDialog()
{}

void KateConfigDialog::addEditorPages()
{

    for (int i = 0; i < KTextEditor::Editor::instance()->configPages(); ++i) {
        KTextEditor::ConfigPage *page = KTextEditor::Editor::instance()->configPage(i, this);
        connect(page, SIGNAL(changed()), this, SLOT(slotChanged()));
        m_editorPages.push_back(page);
        KPageWidgetItem *item = addSubPage(m_editorPage, page, KTextEditor::Editor::instance()->configPageName(i));
        item->setHeader(KTextEditor::Editor::instance()->configPageFullName(i));
        item->setIcon(KTextEditor::Editor::instance()->configPageIcon(i));
    }
}

void KateConfigDialog::addPluginPage(KTextEditor::Plugin *plugin)
{
    if (!qobject_cast<KTextEditor::ConfigPageInterface *>(plugin)) {
        return;
    }

    for (int i = 0; i < qobject_cast<KTextEditor::ConfigPageInterface *>(plugin)->configPages(); i++) {
        QFrame *page = new QFrame();
        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setSpacing(0);
        layout->setMargin(0);

        KPageWidgetItem *item = addSubPage(m_applicationPage, page, qobject_cast<KTextEditor::ConfigPageInterface *>(plugin)->configPageName(i));
        item->setHeader(qobject_cast<KTextEditor::ConfigPageInterface *>(plugin)->configPageFullName(i));
        item->setIcon(qobject_cast<KTextEditor::ConfigPageInterface *>(plugin)->configPageIcon(i));

        PluginPageListItem *info = new PluginPageListItem;
        info->plugin = plugin;
        info->configPageInterface = qobject_cast<KTextEditor::ConfigPageInterface *>(plugin);
        info->pageParent = page;
        info->pluginPage = 0; //info->configPageInterface->configPage (i, page);
        info->idInPlugin = i;
        info->pageWidgetItem = item;
        //connect( info->page, SIGNAL(changed()), this, SLOT(slotChanged()) );
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
    qCDebug(LOG_KATE) << "creating config page";
    info->pluginPage = info->configPageInterface->configPage(info->idInPlugin, info->pageParent);
    info->pageParent->layout()->addWidget(info->pluginPage);
    info->pluginPage->show();
    connect(info->pluginPage, SIGNAL(changed()), this, SLOT(slotChanged()));
}

void KateConfigDialog::removePluginPage(KTextEditor::Plugin *plugin)
{
    if (!qobject_cast<KTextEditor::ConfigPageInterface *>(plugin)) {
        return;
    }

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

        cg.writeEntry("Restore Window Configuration", m_restoreVC->isChecked());

        if (m_startNewSessionRadioButton->isChecked()) {
            cg.writeEntry("Startup Session", "new");
        } else if (m_loadLastUserSessionRadioButton->isChecked()) {
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

    foreach(PluginPageListItem * plugin, m_pluginPages.values()) {
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

    // write back the editor config
    KTextEditor::Editor::instance()->writeConfig(config.data());

    config->sync();

    m_dataChanged = false;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void KateConfigDialog::slotChanged()
{
    m_dataChanged = true;
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
    m_daysMetaInfos->setSuffix(i18ncp("The suffix of 'Delete unused meta-information after'", " day", " days", m_daysMetaInfos->value()));
}

void KateConfigDialog::showAppPluginPage(KTextEditor::ConfigPageInterface *configpageinterface, uint id)
{
    foreach(PluginPageListItem * plugin, m_pluginPages.values()) {
        if ((plugin->configPageInterface == configpageinterface) && (id == plugin->idInPlugin)) {
            setCurrentPage(plugin->pageWidgetItem);
            break;
        }
    }
}

void KateConfigDialog::slotHelp()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("help:/")));
}
