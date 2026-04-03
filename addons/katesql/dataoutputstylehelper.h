/*
SPDX-FileCopyrightText: 2010 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QHash>
#include <QMetaType>
#include <QVariant>

struct OutputStyle;

class DataOutputStyleHelper
{
public:
    DataOutputStyleHelper();
    ~DataOutputStyleHelper();

    void readConfig();

    bool useSystemLocale() const;
    void setUseSystemLocale(bool useSystemLocale);

    QVariant styleData(const QVariant &value, int role) const;

private:
    QHash<QString, OutputStyle *> m_styles;
    bool m_useSystemLocale;
};
