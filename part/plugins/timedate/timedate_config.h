/**
  * This file is part of the KDE libraries
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef TIMEDATE_CONFIG_H
#define TIMEDATE_CONFIG_H

#include <kcmodule.h>

class KLineEdit;

class TimeDateConfig
    : public KCModule
{
    Q_OBJECT

public:
    explicit TimeDateConfig(QWidget *parent = 0, const QVariantList &args = QVariantList());
    virtual ~TimeDateConfig();

    virtual void save();
    virtual void load();
    virtual void defaults();

private Q_SLOTS:
    void slotChanged();

private:
    KLineEdit *format;
};

#endif // TIMEDATE_CONFIG_H
