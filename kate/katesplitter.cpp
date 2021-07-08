/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katesplitter.h"
#include <QDebug>
#include <QEvent>

class SplitterHandle : public QSplitterHandle
{
public:
    SplitterHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent)
    {
    }

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        QList<int> _sizes = splitter()->sizes();

        if (_sizes.size() == 2) {
            const int halfSize = (_sizes[0] + _sizes[1]) / 2;
            _sizes[0] = halfSize;
            _sizes[1] = halfSize;
            splitter()->setSizes(_sizes);
        } else {
            QSplitterHandle::mouseDoubleClickEvent(event);
        }
    }
};

KateSplitter::KateSplitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent)
{
}

KateSplitter::KateSplitter(QWidget *parent)
    : QSplitter(parent)
{
}

QSplitterHandle *KateSplitter::createHandle()
{
    return new SplitterHandle(orientation(), this);
}
