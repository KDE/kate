/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef QUICKOPENLINEEDIT_H
#define QUICKOPENLINEEDIT_H

#include <QLineEdit>
#include <QAbstractButton>
#include <QIcon>

#include "katequickopenmodel.h"

class SwitchModeButton : public QAbstractButton
{

public:
    explicit SwitchModeButton(QWidget *parent);

    QSize sizeHint() const override
    {
        return m_icon.actualSize(QSize(24, 24));
    }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QIcon m_icon;
};

enum FilterMode : uint8_t
{
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

    FilterModes filterMode()
    {
        return m_mode;
    }

    void updateViewGeometry();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    SwitchModeButton* m_button;
    QMenu* m_menu;
    FilterModes m_mode = (FilterMode)(FilterMode::FilterByName | FilterMode::FilterByPath);

private Q_SLOTS:
    void openMenu();

Q_SIGNALS:
    void filterModeChanged(FilterModes mode);
    void listModeChanged(KateQuickOpenModelList mode);
};

#endif // QUICKOPENLINEEDIT_H
