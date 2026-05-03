/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "../helpers/dataoutputstylehelper.h"
#include <QObject>
#include <QVariant>

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

    void setStyleHelper(DataOutputStyleHelper *helper)
    {
        m_styleHelper = helper;
    };

protected:
    DataOutputStyleHelper *m_styleHelper = nullptr;

    /**
     * Applies style helper transformations to the given display value.
     *
     * Shared by both editable model implementations to consolidate
     * the duplicated data() override logic.
     *
     * @param displayValue The raw display value from the base SQL model.
     * @param role The Qt item data role.
     * @param isDirty Whether the cell has unsaved changes.
     * @return The styled variant if the style helper produced one, otherwise an invalid QVariant.
     */
    QVariant applyStyle(const QVariant &displayValue, int role, bool isDirty) const
    {
        if (m_styleHelper) {
            return m_styleHelper->styleData(displayValue, role, isDirty);
        }
        return QVariant();
    }
};

Q_DECLARE_INTERFACE(DataOutputModelInterface, "org.kde.kate.DataOutputModelInterface")
