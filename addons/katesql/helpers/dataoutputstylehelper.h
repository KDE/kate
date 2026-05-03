/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "outputstyle.h"
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
    void updateDefaultStyle();

    bool useSystemLocale() const;
    void setUseSystemLocale(bool useSystemLocale);

    QVariant styleData(const QVariant &value, int role, bool isDirty = false) const;

private:
    QHash<QString, OutputStyle *> m_styles;
    OutputStyle m_defaultStyle;
    bool m_useSystemLocale;
    bool m_useSystemTheme;
};
