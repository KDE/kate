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
#include <QListWidget>
#include <QVBoxLayout>
#include <QGraphicsGridLayout>
#include <KStandardDirs>
#include <KIconLoader>
#include <plasma/layouts/boxlayout.h>
#include <plasma/widgets/icon.h>
#include <QGraphicsProxyWidget>
#include <QGraphicsGridLayout>
#include <QListWidgetItem>
#include <KIcon>

KateSessionApplet::KateSessionApplet(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args), m_icon( 0 ), m_proxy(0), m_layout( 0 )
{
    int iconSize = IconSize(KIconLoader::Desktop);
    resize(iconSize, iconSize);
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
    m_listView= new QListWidget(m_widget);
    m_listView->setMouseTracking(true);
    initSessionFiles();
    connect(m_listView, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(slotOnItemClicked(QListWidgetItem*)));
    l_layout->addWidget( m_listView );
    m_widget->setLayout( l_layout );
    m_widget->adjustSize();

}

void KateSessionApplet::constraintsUpdated(Plasma::Constraints constraints)
{
    // on the panel we don't want a background, and our proxy widget in Planar has one
    setDrawStandardBackground(false);

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
            m_layout = new QGraphicsGridLayout;
            m_widget->setWindowFlags(Qt::Widget);
            m_layout->setSpacing(0);
            m_proxy = new QGraphicsProxyWidget(this);
            m_proxy->setWidget(m_widget);
            m_proxy->show();
            m_layout->addItem( m_proxy, 0, 0, 1, 2);
            m_proxy->resize(contentSize());
        }
    }

    if (m_icon && constraints & Plasma::SizeConstraint) {
        m_icon->resize(contentSize());
    }
}

void KateSessionApplet::paintInterface(QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &rect)
{
    Applet::paintInterface(p,option,rect);
}

Qt::Orientations KateSessionApplet::expandingDirections() const
{
    if (formFactor() == Plasma::Horizontal) {
        return Qt::Vertical;
    } else {
        return Qt::Horizontal;
    }
}

QSizeF KateSessionApplet::contentSizeHint() const
{
    QSizeF sizeHint = contentSize();
    switch (formFactor()) {
    case Plasma::Vertical:
        sizeHint.setHeight(sizeHint.width());
        break;
    case Plasma::Horizontal:
        sizeHint.setWidth(sizeHint.height());
        break;
    default:
        break;
    }

    return sizeHint;
}

void KateSessionApplet::initSessionFiles()
{


    // no session - exec 'kate'
    //insertItem( KIcon("kate"), i18n("Start Kate (no arguments)"), id++ );

    // new session - prompt for a name and  exec 'kate --start NAME'
    //insertItem( KIcon("new"), i18n("New Kate Session"), id++ );

    // new anonymous session, 'kate --start ""'
    //insertItem( KIcon("new"), i18n("New Anonymous Session"), id++ );

    QStringList list = KGlobal::dirs()->findAllResources( "data", "kate/sessions/*.katesession", KStandardDirs::NoDuplicates );
    for (QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        KConfig _config( *it, KConfig::SimpleConfig );
        KConfigGroup config(&_config, "General" );
        m_listView->addItem( config.readEntry( "Name" ) );
        //sessions.append( config.readEntry( "Name" ) );
    }
}

void KateSessionApplet::initSysTray()
{
    if (m_icon) {
        return;
    }

    m_widget->setWindowFlags(Qt::Popup);

    m_icon = new Plasma::Icon(KIcon("kate"), QString(), this);
    setMinimumContentSize(m_icon->sizeFromIconSize(IconSize(KIconLoader::Small)));
    connect(m_icon, SIGNAL(clicked()), this, SLOT(slotOpenMenu()));
    m_icon->resize(contentSize());
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

void KateSessionApplet::slotOnItemClicked( QListWidgetItem *)
{
}

#if 0
void KateSessionMenu::slotExec( int id )
{
  if ( id < 0 )
    return;

  QStringList args;
  if ( id > 0 )
    args << "--start";

  // If a new session is requested we try to ask for a name.
  if ( id == 1 )
  {
    bool ok (false);
    QString name = KInputDialog::getText( i18n("Session Name"),
                                          i18n("Please enter a name for the new session"),
                                          QString(),
                                          &ok, 0, new Validator( m_parent ) );
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
    else
      // mark the menu as dirty so that it gets rebuild at next display
      // to show the new session
      setInitialized( false );

    args << name;
  }

  else if ( id == 2 )
    args << "";

  else if ( id > 2 )
    args << m_sessions[ id-3 ];

   KToolInvocation::kdeinitExec("kate", args);
}

#endif


#include "katesessionapplet.moc"
