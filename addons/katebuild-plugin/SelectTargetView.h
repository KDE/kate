/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   Copyright 2014 Kåre Särs <kare.sars@iki.fi>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef SelectTargetView_H
#define SelectTargetView_H

#include "ui_SelectTargetUi.h"
#include <QAbstractItemModel>

class TargetFilterProxyModel;

class SelectTargetView : public QDialog, public Ui::SelectTargetUi
{
    Q_OBJECT
public:
    SelectTargetView(QAbstractItemModel *model, QWidget *parent = nullptr);
    const QModelIndex currentIndex() const;

    void setCurrentIndex(const QModelIndex &index);

public Q_SLOTS:
    void setFilter(const QString &filter);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    TargetFilterProxyModel *m_proxyModel;
};

#endif
