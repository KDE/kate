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

#ifndef _KATESESSIONAPPLET_H_
#define _KATESESSIONAPPLET_H_

#include <plasma/applet.h>
#include <plasma/dialog.h>

class QPainter;
class QTreeView;
class QStyleOptionGraphicsItem;
class QGraphicsProxyWidget;
class QGraphicsGridLayout;
class QStandardItemModel;
class QModelIndex;

namespace Plasma
{
    class Icon;
}

class KateSessionApplet : public Plasma::Applet
{
    Q_OBJECT
public:
    KateSessionApplet(QObject *parent, const QVariantList &args);
    ~KateSessionApplet();

    void paintInterface(QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &rect);

    void init();

    void constraintsUpdated(Plasma::Constraints constraints);
    Qt::Orientations expandingDirections() const;
    QSizeF contentSizeHint() const;
    void initSysTray();

    enum SpecificRoles {
        Index = Qt::UserRole+1
    };

protected slots:
    void slotOpenMenu();
    void slotOnItemClicked(const QModelIndex &index);
    void slotUpdateSessionMenu();

protected:
    void initSessionFiles();
private:
    Plasma::Dialog *m_widget;
    QTreeView *m_listView;
    Plasma::Icon *m_icon;
    QGraphicsProxyWidget * m_proxy;
    QGraphicsGridLayout* m_layout;
    QStandardItemModel *m_kateModel;
    QStringList m_sessions;
};

K_EXPORT_PLASMA_APPLET(sessionapplet, KateSessionApplet )

#endif
