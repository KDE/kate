/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef QUICKOPENLINEEDIT_H
#define QUICKOPENLINEEDIT_H

#include <QLineEdit>
#include <memory>

#include "katequickopenmodel.h"

enum FilterMode : uint8_t {
    FilterByName = 0x01, /* By File Name */
    FilterByPath = 0x02 /* By File Path */
};
Q_DECLARE_FLAGS(FilterModes, FilterMode)
Q_FLAGS(FilterModes)

class QuickOpenLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit QuickOpenLineEdit(QWidget *parent);

    FilterModes filterMode() const
    {
        return m_mode;
    }

    KateQuickOpenModelList listMode() const
    {
        return m_listMode;
    }

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setupMenu();

private:
    std::unique_ptr<QMenu> menu;
    FilterModes m_mode;
    KateQuickOpenModelList m_listMode;

Q_SIGNALS:
    void filterModeChanged(FilterModes mode);
    void listModeChanged(KateQuickOpenModelList mode);
};

#endif // QUICKOPENLINEEDIT_H
