/***************************************************************************
 *   Copyright (C) 2008 by Montel Laurent <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#include "katesessionapplet.h"
#include <QStyleOptionGraphicsItem>
#include <QTreeView>
#include <QVBoxLayout>
#include <QGraphicsGridLayout>
#include <KStandardDirs>
#include <KIconLoader>
#include <KInputDialog>
#include <KMessageBox>
#include <KStandardGuiItem>
#include <QGraphicsProxyWidget>
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <KIcon>
#include <KToolInvocation>
#include <KDirWatch>
#include <QGraphicsLinearLayout>
#include <KGlobalSettings>
#include <KUrl>
#include <KStringHandler>
#include <QFile>
#include <KConfigDialog>


bool katesessions_compare_sessions(const QString &s1, const QString &s2) {
    return KStringHandler::naturalCompare(s1,s2)==-1;
}


KateSessionApplet::KateSessionApplet(QObject *parent, const QVariantList &args)
    : Plasma::PopupApplet(parent, args), m_listView( 0 ), m_config(0)
{
    KDirWatch *dirwatch = new KDirWatch( this );
    QStringList lst = KGlobal::dirs()->findDirs( "data", "kate/sessions/" );
    for ( int i = 0; i < lst.count(); i++ )
    {
        dirwatch->addDir( lst[i] );
    }
    connect( dirwatch, SIGNAL(dirty(QString)), this, SLOT(slotUpdateSessionMenu()) );
    setPopupIcon( "kate" );
    setHasConfigurationInterface(true);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
}

KateSessionApplet::~KateSessionApplet()
{
    delete m_listView;
}

QWidget *KateSessionApplet::widget()
{
    if ( !m_listView )
    {
        m_listView= new QTreeView();
        m_listView->setAttribute(Qt::WA_NoSystemBackground);
        m_listView->setEditTriggers( QAbstractItemView::NoEditTriggers );
        m_listView->setRootIsDecorated(false);
        m_listView->setHeaderHidden(true);
        m_listView->setMouseTracking(true);

        m_kateModel = new QStandardItemModel(this);
        m_listView->setModel(m_kateModel);
        m_listView->setMouseTracking(true);

        initSessionFiles();

        connect(m_listView, SIGNAL(activated(QModelIndex)),
            this, SLOT(slotOnItemClicked(QModelIndex)));
    }
    return m_listView;
}


void KateSessionApplet::slotUpdateSessionMenu()
{
   m_kateModel->clear();
   m_sessions.clear();
   m_fullList.clear();
   initSessionFiles();
}

void KateSessionApplet::initSessionFiles()
{
    // Obtain list of items previously configured as hidden
    const QStringList hideList = config().readEntry("hideList", QStringList());

    // Construct a full list of items (m_fullList) so we can display them
    // in the config dialog, but leave out the hidden stuff for m_kateModel
    // that is actually displayed
    int index=0;
    QStandardItem *item = new QStandardItem();
    item->setData(i18n("Start Kate (no arguments)"), Qt::DisplayRole);
    item->setData( KIcon( "kate" ), Qt::DecorationRole );
    item->setData( index++, Index );
    m_fullList << item->data(Qt::DisplayRole).toString();
    if (!hideList.contains(item->data(Qt::DisplayRole).toString())) {
        m_kateModel->appendRow(item);
    }

    item = new QStandardItem();
    item->setData( i18n("New Kate Session"), Qt::DisplayRole);
    item->setData( KIcon( "document-new" ), Qt::DecorationRole );
    item->setData( index++, Index );
    m_fullList << item->data(Qt::DisplayRole).toString();
    if (!hideList.contains(item->data(Qt::DisplayRole).toString())) {
        m_kateModel->appendRow(item);
    }

    item = new QStandardItem();
    item->setData( i18n("New Anonymous Session"), Qt::DisplayRole);
    item->setData( index++, Index );
    item->setData( KIcon( "document-new" ), Qt::DecorationRole );
    m_fullList << item->data(Qt::DisplayRole).toString();
    if (!hideList.contains(item->data(Qt::DisplayRole).toString())) {
        m_kateModel->appendRow(item);
    }

    const QStringList list = KGlobal::dirs()->findAllResources( "data", "kate/sessions/*.katesession", KStandardDirs::NoDuplicates );
    KUrl url;
    for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        url.setPath(*it);
        QString name=url.fileName();
        name = QUrl::fromPercentEncoding(QFile::encodeName(url.fileName()));
        name.chop(12);///.katesession==12
/*        KConfig _config( *it, KConfig::SimpleConfig );
        KConfigGroup config(&_config, "General" );
        QString name =  config.readEntry( "Name" );*/
        m_sessions.append( name );
    }
    qSort(m_sessions.begin(),m_sessions.end(),katesessions_compare_sessions);
    for(QStringList::ConstIterator it=m_sessions.constBegin();it!=m_sessions.constEnd();++it)
    {
        m_fullList << *it;
        if (!hideList.contains(*it)) {
            item = new QStandardItem();
            item->setData(*it, Qt::DisplayRole);
            item->setData( index++, Index );
            m_kateModel->appendRow( item);
        }
    }
}

void KateSessionApplet::slotOnItemClicked(const QModelIndex &index)
{
    hidePopup();
    int id = index.data(Index).toInt();
    QStringList args;

    // If a new session is requested we try to ask for a name.
    if ( id == 1 )
    {
        bool ok = false;
        QString name = KInputDialog::getText( i18n("Session Name"),
                                              i18n("Please enter a name for the new session"),
                                              QString(),
                                              &ok );
        if ( ! ok )
            return;

        if ( name.isEmpty() && KMessageBox::questionYesNo( 0,
                                                           i18n("An unnamed session will not be saved automatically. "
                                                                "Do you want to create such a session?"),
                                                           i18n("Create anonymous session?"),
                                                           KStandardGuiItem::yes(), KStandardGuiItem::cancel(),
                                                           "kate_session_button_create_anonymous" ) == KMessageBox::No )
            return;

        if ( m_sessions.contains( name ) &&
             KMessageBox::warningYesNo( 0,
                                        i18n("You already have a session named %1. Do you want to open that session?", name ),
                                        i18n("Session exists") ) == KMessageBox::No )
            return;
        if (name.isEmpty())
          args <<"-startanon";
        else
          args <<"-n"<<"--start"<< name;
    }

    else if ( id == 2 )
        args << "--startanon";

    else if ( id > 2 )
        args <<"-n"<< "--start"<<m_sessions[ id-3 ];

    KToolInvocation::kdeinitExec("kate", args);
}

void KateSessionApplet::createConfigurationInterface(KConfigDialog *parent)
{
    const QStringList hideList = config().readEntry("hideList", QStringList());
    m_config = new KateSessionConfigInterface(m_fullList,
                                              hideList);
    parent->addPage(m_config, i18n("Sessions"),
                    "preferences-desktop-notification",
                    i18n("Sessions to show"));
    connect(parent, SIGNAL(applyClicked()), this, SLOT(slotSaveConfig()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(slotSaveConfig()));
}

void KateSessionApplet::slotSaveConfig()
{
    config().writeEntry("hideList", m_config->hideList());
}

void KateSessionApplet::configChanged()
{
    // refresh menu from config
    slotUpdateSessionMenu();
}

KateSessionConfigInterface::KateSessionConfigInterface(const QStringList& all, const QStringList& hidden)
{
    m_all = all;
    m_config.setupUi(this);
    for (int i=0; i < m_all.size(); i++) {
        QListWidgetItem *item = new QListWidgetItem(m_all[i]);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (hidden.contains(item->text())) {
            item->setCheckState(Qt::Unchecked);
            m_config.itemList->addItem(item);
        } else {
            item->setCheckState(Qt::Checked);
            m_config.itemList->addItem(item);
        }
    }
}

QStringList KateSessionConfigInterface::hideList() const
{
    QStringList hideList;
    const int numberOfItem = m_config.itemList->count();
    for (int i=0; i< numberOfItem; ++i) {
        QListWidgetItem *item = m_config.itemList->item(i);
        if (item->checkState() == Qt::Unchecked)
            hideList << m_config.itemList->item(i)->text();
    }
    return hideList;
}

#include "katesessionapplet.moc"
