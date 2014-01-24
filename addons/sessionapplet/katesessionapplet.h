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

#include <plasma/popupapplet.h>

#include "ui_katesessionConfig.h"

class QTreeView;
class QGraphicsProxyWidget;
class QStandardItemModel;
class QModelIndex;
class KConfigDialog;
class QStringList;
class KateSessionConfigInterface : public QWidget {
    // Wrapper widget class for the configuration interface.
    Q_OBJECT
public:
    KateSessionConfigInterface(const QStringList& all, const QStringList& hidden);
    QStringList hideList() const;
private:
    QStringList m_all;
    Ui::KateSessionConfig m_config;
};


class KateSessionApplet : public Plasma::PopupApplet
{
    Q_OBJECT
public:
    KateSessionApplet(QObject *parent, const QVariantList &args);
    ~KateSessionApplet();

    QWidget *widget();

    enum SpecificRoles {
        Index = Qt::UserRole+1
    };

protected Q_SLOTS:
    void slotOnItemClicked(const QModelIndex &index);
    void slotUpdateSessionMenu();
    void slotSaveConfig();

protected:
    void initSessionFiles();
    void createConfigurationInterface(KConfigDialog *parent);
    void configChanged();
private:
    QTreeView *m_listView;
    QStandardItemModel *m_kateModel;
    QStringList m_sessions;
    QStringList m_fullList;
    KateSessionConfigInterface *m_config;
};

K_EXPORT_PLASMA_APPLET(katesession, KateSessionApplet )

#endif
