/* This file is part of the KDE libraries
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kateviinsertmode.h"
#include "kateview.h"
#include "kateviewinternal.h"

KateViInsertMode::KateViInsertMode( KateView * view, KateViewInternal * viewInternal )
{
  m_view = view;
  m_viewInternal = viewInternal;
}

KateViInsertMode::~KateViInsertMode()
{
}

/**
 * checks if the key is a valid command
 * @return true if a command was completed and executed, false otherwise
 */
bool KateViInsertMode::handleKeypress( QKeyEvent *e )
{
    if ( e->key() == Qt::Key_Escape ) {
        m_view->viEnterNormalMode();
        return true;
    }

    if ( e->modifiers() == Qt::ControlModifier ) {
        switch( e->key() ) {
        case Qt::Key_C:
            m_view->viEnterNormalMode();
            return true;
        default:
            return false;
        }
    }

    return false;
}
