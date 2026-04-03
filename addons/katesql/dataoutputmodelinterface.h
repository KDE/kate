/*
SPDX-FileCopyrightText: 2010 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QObject>

class DataOutputModelInterface
{
public:
    virtual ~DataOutputModelInterface() = default;

    virtual QObject *asQObject() = 0;

    virtual void readConfig() = 0;
    virtual bool useSystemLocale() const = 0;
    virtual void setUseSystemLocale(bool useSystemLocale) = 0;
    virtual void clear() = 0;
    virtual void refresh() = 0;
};

Q_DECLARE_INTERFACE(DataOutputModelInterface, "org.kde.kate.DataOutputModelInterface")
