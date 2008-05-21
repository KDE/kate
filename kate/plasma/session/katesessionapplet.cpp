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
#include <plasma/widgets/icon.h>
#include <QGraphicsProxyWidget>
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <KIcon>
#include <KToolInvocation>
#include <KDirWatch>
#include <QGraphicsLinearLayout>

KateSessionApplet::KateSessionApplet(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args), m_icon( 0 ), m_proxy(0), m_layout( 0 )
{
    int iconSize = IconSize(KIconLoader::Desktop);
    resize(iconSize, iconSize);
    KDirWatch *dirwatch = new KDirWatch( this );
    QStringList lst = KGlobal::dirs()->findDirs( "data", "kate/sessions/" );
    for ( int i = 0; i < lst.count(); i++ )
    {
        dirwatch->addDir( lst[i] );
    }
    connect( dirwatch, SIGNAL(dirty (const QString &) ), this, SLOT( slotUpdateSessionMenu() ) );
}

KateSessionApplet::~KateSessionApplet()
{
    delete m_widget;
    delete m_icon;
}

void KateSessionApplet::init()
{
    m_widget = new Plasma::Dialog();
    QVBoxLayout *l_layout = new QVBoxLayout();
    l_layout->setSpacing(0);
    l_layout->setMargin(0);

    m_listView= new QTreeView(m_widget);
    m_listView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    m_listView->setRootIsDecorated(false);
    m_listView->setHeaderHidden(true);
    m_listView->setMouseTracking(true);

    m_kateModel = new QStandardItemModel(this);
    m_listView->setModel(m_kateModel);
    m_listView->setMouseTracking(true);

    initSessionFiles();

    connect(m_listView, SIGNAL(clicked (const QModelIndex &)), this, SLOT(slotOnItemClicked(const QModelIndex &)));

    l_layout->addWidget( m_listView );

    m_widget->setLayout( l_layout );
    m_widget->adjustSize();

}

void KateSessionApplet::constraintsEvent(Plasma::Constraints constraints)
{
    // on the panel we don't want a background, and our proxy widget in Planar has one
    setBackgroundHints(NoBackground);

    bool isSizeConstrained = formFactor() != Plasma::Planar && formFactor() != Plasma::MediaCenter;

    if (constraints & Plasma::FormFactorConstraint) {
        if (isSizeConstrained) {
            delete m_layout;
            m_layout = 0;

            if (m_proxy) {
                m_proxy->setWidget(0);
                delete m_proxy;
                m_proxy = 0;
            }

            initSysTray();
        } else {
            delete m_icon;
            m_icon = 0;

            m_widget->setWindowFlags(Qt::Widget);
            m_layout = new QGraphicsLinearLayout();
            m_layout->setContentsMargins(0,0,0,0);
            m_layout->setSpacing(0);
            m_proxy = new QGraphicsProxyWidget(this);
            m_proxy->setWidget(m_widget);
            m_proxy->show();
            setLayout( m_layout );
            //Laurent size fixed until I was able to resize it correctly
            //setMinimumContentSize(m_widget->size());
            //setMaximumContentSize(m_widget->size());

        }
    }

    if (m_icon && constraints & Plasma::SizeConstraint) {
        m_icon->resize(geometry().size());
    }
}

void KateSessionApplet::slotUpdateSessionMenu()
{
    m_kateModel->clear();
    initSessionFiles();
}

void KateSessionApplet::initSessionFiles()
{
    int index = 0;

    QStandardItem *item = new QStandardItem();
    item->setData(i18n("Start Kate (no arguments)"), Qt::DisplayRole);
    item->setData( KIcon( "kate" ), Qt::DecorationRole );
    item->setData( index++, Index );
    m_kateModel->appendRow(item);

    item = new QStandardItem();
    item->setData( i18n("New Kate Session"), Qt::DisplayRole);
    item->setData( KIcon( "document-new" ), Qt::DecorationRole );
    item->setData( index++, Index );
    m_kateModel->appendRow(item);

    item = new QStandardItem();
    item->setData( i18n("New Anonymous Session"), Qt::DisplayRole);
    item->setData( index++, Index );
    item->setData( KIcon( "document-new" ), Qt::DecorationRole );
    m_kateModel->appendRow(item);

    QStringList list = KGlobal::dirs()->findAllResources( "data", "kate/sessions/*.katesession", KStandardDirs::NoDuplicates );
    for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        KConfig _config( *it, KConfig::SimpleConfig );
        KConfigGroup config(&_config, "General" );
        QString name =  config.readEntry( "Name" );
        m_sessions.append( name );
        item = new QStandardItem();
        item->setData(name, Qt::DisplayRole);
        item->setData( index++, Index );
        m_kateModel->appendRow( item);
    }
}

void KateSessionApplet::initSysTray()
{
    if (m_icon) {
        return;
    }

    m_widget->setWindowFlags(Qt::Popup);

    m_icon = new Plasma::Icon(KIcon("kate"), QString(), this);
    connect(m_icon, SIGNAL(clicked()), this, SLOT(slotOpenMenu()));
    m_icon->resize(m_icon->sizeFromIconSize(IconSize(KIconLoader::Small)));
    updateGeometry();
}

void KateSessionApplet::slotOpenMenu()
{
    if (m_widget->isVisible()) {
        m_widget->hide();
    } else {
        m_widget->move(popupPosition(m_widget->sizeHint()));
        m_widget->show();
    }

    m_widget->clearFocus();
}

void KateSessionApplet::slotOnItemClicked(const QModelIndex &index)
{
    if ( m_icon )
        m_widget->hide();
    int id = index.data(Index).toInt();
    QStringList args;
    if ( id > 0 )
        args << "--start";

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
        args << name;
    }

    else if ( id == 2 )
        args << "";

    else if ( id > 2 )
        args << m_sessions[ id-3 ];

    KToolInvocation::kdeinitExec("kate", args);
}



#include "katesessionapplet.moc"
