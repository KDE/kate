/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2007 Mirko Stocker <me@misto.ch>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateconfigplugindialogpage.h"

#include "kateapp.h"
#include "kateconfigdialog.h"
#include "katepluginmanager.h"

#include <KLocalizedString>

#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

class KatePluginListItem : public QStandardItem
{
public:
    KatePluginListItem(bool checked, KatePluginInfo *info);

    KatePluginInfo *info() const
    {
        return mInfo;
    }

private:
    KatePluginInfo *mInfo;
};

KatePluginListItem::KatePluginListItem(bool checked, KatePluginInfo *info)
    : mInfo(info)
{
    setCheckable(true);
    setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

KateConfigPluginPage::KateConfigPluginPage(QWidget *parent, KateConfigDialog *dialog)
    : QFrame(parent)
    , myDialog(dialog)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QTreeView *listView = new QTreeView(this);
    layout->addWidget(listView);
    listView->setRootIsDecorated(false);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView->setAllColumnsShowFocus(true);
    listView->setDragDropMode(QAbstractItemView::NoDragDrop);
    listView->setWhatsThis(
        i18n("Here you can see all available Kate plugins. Those with a check mark are loaded, and will be loaded again the next time Kate is started."));

    KatePluginList &pluginList(KateApp::self()->pluginManager()->pluginList());
    auto pluginModel = new QStandardItemModel(this);
    pluginModel->setHorizontalHeaderLabels({i18n("Name"), i18n("Description")});
    for (auto &pluginInfo : pluginList) {
        auto item = new KatePluginListItem(pluginInfo.load, &pluginInfo);
        item->setText(pluginInfo.metaData.name());
        pluginModel->appendRow({item, new QStandardItem(pluginInfo.metaData.description())});
        m_pluginItems.push_back(item);
    }
    connect(pluginModel, &QStandardItemModel::itemChanged, this, &KateConfigPluginPage::changed);

    /**
     * attach our persistent model to the view with filter in-between
     */
    auto m = listView->selectionModel();
    auto sortModel = new QSortFilterProxyModel(this);
    sortModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setSourceModel(pluginModel);
    listView->setModel(sortModel);
    delete m;

    listView->resizeColumnToContents(0);
    listView->setSortingEnabled(true);
    listView->sortByColumn(0, Qt::AscendingOrder);
}

void KateConfigPluginPage::slotApply()
{
    for (auto item : m_pluginItems) {
        if (item->checkState() == Qt::Checked) {
            loadPlugin(item);
        } else {
            unloadPlugin(item);
        }
    }
}

void KateConfigPluginPage::loadPlugin(KatePluginListItem *item)
{
    if (item->info()->load) {
        return;
    }

    const bool ok = KateApp::self()->pluginManager()->loadPlugin(item->info());
    if (!ok) {
        return;
    }
    KateApp::self()->pluginManager()->enablePluginGUI(item->info());
    myDialog->addPluginPage(item->info()->plugin);

    item->setCheckState(Qt::Checked);
}

void KateConfigPluginPage::unloadPlugin(KatePluginListItem *item)
{
    if (!item->info()->load) {
        return;
    }

    myDialog->removePluginPage(item->info()->plugin);
    KateApp::self()->pluginManager()->unloadPlugin(item->info());

    item->setCheckState(Qt::Unchecked);
}

#include "moc_kateconfigplugindialogpage.cpp"
