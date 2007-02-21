/* This file is part of the KDE project
   Copyright (C) 2003 Ian Reinhart Geiser <geiseri@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katemainwindowadaptor.h"
#include "katemainwindowadaptor.moc"
#include "katemainwindow.h"

KateMainWindowAdaptor::KateMainWindowAdaptor (KateMainWindow *w)
    : QDBusAbstractAdaptor( w ), m_w( w )
//((QString("KateMainWindow#%1").arg(w->mainWindowNumber())).toLatin1()), m_w (w)
{
}
// kate: space-indent on; indent-width 2; replace-tabs on;

