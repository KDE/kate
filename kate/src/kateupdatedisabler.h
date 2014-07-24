/* This file is part of the KDE libraries
   Copyright (C) 2014 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_UPDATE_DISABLER
#define KATE_UPDATE_DISABLER

#include <QPointer>
#include <QWidget>

class KateUpdateDisabler
{
public:
    /**
     * Disable updates for given widget.
     * Will auto-enable them on destruction, like a mutex locker releases its lock.
     * @param widget widget to disable updates for
     */
    explicit KateUpdateDisabler (QWidget *widget)
        : m_widget ((widget && widget->updatesEnabled()) ? widget : nullptr)
    {
        if (m_widget)
            m_widget->setUpdatesEnabled(false);
    }
    
    /**
     * Enable updates again on destruction.
     */
    ~KateUpdateDisabler()
    {
        if (m_widget)
            m_widget->setUpdatesEnabled(true);
    }
    
private:
    /**
     * No copying please
     */
    Q_DISABLE_COPY(KateUpdateDisabler)
    
    /**
     * pointer to widget, if not null, enable/disable widgets
     */
    QPointer<QWidget> m_widget;
};

#endif
