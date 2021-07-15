/*
    SPDX-FileCopyrightText: 2021 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2021 Michal Humpula <kde@hudrydum.cz>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katesplitter.h"
#include <QDebug>
#include <QEvent>

class SplitterHandle : public QSplitterHandle
{
public:
    SplitterHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent)
        , m_mouseReleaseWasReceived(false)
    {
    }

protected:
    bool event(QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            m_mouseReleaseWasReceived = false;
            break;
        case QEvent::MouseButtonRelease:
            if (m_mouseReleaseWasReceived) {
                resetSplitterSizes();
            }
            m_mouseReleaseWasReceived = !m_mouseReleaseWasReceived;
            break;
        case QEvent::MouseButtonDblClick:
            m_mouseReleaseWasReceived = false;
            resetSplitterSizes();
            break;
        default:
            break;
        }

        return QSplitterHandle::event(event);
    }

private:
    void resetSplitterSizes()
    {
        QList<int> splitterSizes = splitter()->sizes();
        std::fill(splitterSizes.begin(), splitterSizes.end(), 0);
        splitter()->setSizes(splitterSizes);
    }

    // Sometimes QSplitterHandle doesn't receive MouseButtonDblClick event.
    // We can detect that MouseButtonDblClick event should have been
    // received if we receive two MouseButtonRelease events in a row.
    bool m_mouseReleaseWasReceived;
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
