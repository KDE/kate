/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "outputstyle.h"

#include <QTreeWidget>
#include <qtmetamacros.h>

class OutputStyleWidget : public QTreeWidget
{
    Q_OBJECT

    enum ColumnsOrder {
        TypeLabel = 0,
        BoldCheckBox = 1,
        ItalicCheckBox = 2,
        UnderlineCheckBox = 3,
        StrikeOutCheckBox = 4,
        ForegroundColorButton = 5,
        BackgroundColorButton = 6,
    };
    Q_ENUM(ColumnsOrder)

public:
    explicit OutputStyleWidget(QWidget *parent = nullptr);
    ~OutputStyleWidget() override;

    QTreeWidgetItem *addContext(const QString &key, const QString &name);

    bool useSystemDefaults() const;
    void setUseSystemDefaults(bool useSystemDefaults);

public Q_SLOTS:
    void readConfig();
    void writeConfig();
    void resetToSystemDefaults();
    void updateDefaultStyle();

protected Q_SLOTS:
    void slotChanged();
    void updatePreviews();

    void readConfig(QTreeWidgetItem *item);
    void writeConfig(QTreeWidgetItem *item);

private:
    void setTableToCurrentDefaults();
    void setItemsEnabled(bool enabled);

    OutputStyle m_defaultStyle;
    bool m_useSystemDefaults;

Q_SIGNALS:
    void changed();
};
