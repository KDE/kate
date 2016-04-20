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

#include "kateconfigplugindialogpage.h"

#include "kateapp.h"
#include "katepluginmanager.h"
#include "kateconfigdialog.h"

#include <KLocalizedString>

#include <QVBoxLayout>

class KatePluginListItem : public QTreeWidgetItem
{
public:
    KatePluginListItem(bool checked, KatePluginInfo *info);

    KatePluginInfo *info() const {
        return mInfo;
    }

protected:
    void stateChange(bool);

private:
    KatePluginInfo *mInfo;
};

KatePluginListItem::KatePluginListItem(bool checked, KatePluginInfo *info)
    : QTreeWidgetItem()
    , mInfo(info)
{
    setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
}

KatePluginListView::KatePluginListView(QWidget *parent)
    : QTreeWidget(parent)
{
    setRootIsDecorated(false);
    connect(this, &KatePluginListView::itemChanged, this, &KatePluginListView::stateChanged);
}

void KatePluginListView::stateChanged(QTreeWidgetItem *item)
{
    emit stateChange(static_cast<KatePluginListItem *>(item), item->checkState(0) == Qt::Checked);
}

KateConfigPluginPage::KateConfigPluginPage(QWidget *parent, KateConfigDialog *dialog)
    : QFrame(parent)
    , myDialog(dialog)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);

    KatePluginListView *listView = new KatePluginListView(this);
    layout->addWidget(listView);

    QStringList headers;
    headers << i18n("Name") << i18n("Description");
    listView->setHeaderLabels(headers);
    listView->setWhatsThis(i18n("Here you can see all available Kate plugins. Those with a check mark are loaded, and will be loaded again the next time Kate is started."));

    KatePluginList &pluginList(KateApp::self()->pluginManager()->pluginList());
    for (KatePluginList::iterator it = pluginList.begin(); it != pluginList.end(); ++it) {
        QTreeWidgetItem *item = new KatePluginListItem(it->load, &(*it));
        item->setText(0, it->metaData.name());
        item->setText(1, it->metaData.description());
        listView->addTopLevelItem(item);
    }

    listView->resizeColumnToContents(0);
    listView->sortByColumn(0, Qt::AscendingOrder);
    connect(listView, &KatePluginListView::stateChange, this, &KateConfigPluginPage::stateChange);
}

void KateConfigPluginPage::stateChange(KatePluginListItem *item, bool b)
{
    if (b) {
        loadPlugin(item);
    } else {
        unloadPlugin(item);
    }

    emit changed();
}

void KateConfigPluginPage::loadPlugin(KatePluginListItem *item)
{
    const bool ok = KateApp::self()->pluginManager()->loadPlugin(item->info());
    if (!ok) {
        return;
    }
    KateApp::self()->pluginManager()->enablePluginGUI(item->info());
    myDialog->addPluginPage(item->info()->plugin);

    item->setCheckState(0, Qt::Checked);
}

void KateConfigPluginPage::unloadPlugin(KatePluginListItem *item)
{
    myDialog->removePluginPage(item->info()->plugin);
    KateApp::self()->pluginManager()->unloadPlugin(item->info());

    item->setCheckState(0, Qt::Unchecked);
}

