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

#include <plasmaappletdialog.h>

class QTreeView;
class QGraphicsProxyWidget;
class QStandardItemModel;
class QModelIndex;


class KateSessionApplet : public PlasmaAppletDialog
{
    Q_OBJECT
public:
    KateSessionApplet(QObject *parent, const QVariantList &args);
    ~KateSessionApplet();

    QWidget *widget();

    enum SpecificRoles {
        Index = Qt::UserRole+1
    };

protected slots:
    void slotOnItemClicked(const QModelIndex &index);
    void slotUpdateSessionMenu();

protected:
    void initSessionFiles();
    void initialize();
private:
    QTreeView *m_listView;
    QStandardItemModel *m_kateModel;
    QStringList m_sessions;
};

K_EXPORT_PLASMA_APPLET(katesession, KateSessionApplet )

#endif
