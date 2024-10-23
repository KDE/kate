/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KBusyIndicatorWidget>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QToolButton>

class BusyWrapperWidget : public QStackedWidget
{
public:
    BusyWrapperWidget(QToolButton *button, QWidget *parent)
        : QStackedWidget(parent)
        , m_tb(button)
        , m_busyHolder(new QWidget(this))
        , m_busy(new KBusyIndicatorWidget(this))
    {
        m_busy->setMaximumSize(m_busy->minimumSizeHint());
        auto l = new QHBoxLayout(m_busyHolder);
        l->setContentsMargins({});
        l->addWidget(m_busy);

        layout()->setContentsMargins({});
        addWidget(m_tb);
        addWidget(m_busyHolder);
    }

    void setBusy(bool isBusy)
    {
        QWidget *w = isBusy == false ? static_cast<QWidget *>(m_tb) : m_busyHolder;
        if (w != currentWidget()) {
            setCurrentWidget(w);
        }
    }

private:
    QToolButton *m_tb;
    QWidget *m_busyHolder;
    KBusyIndicatorWidget *m_busy;
};
