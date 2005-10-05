/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATESMARTCURSORNOTIFIER_H
#define KATESMARTCURSORNOTIFIER_H

#include <ktexteditor/cursor.h>

/**
Kate implementation of the smart cursor notifier.  Basically allows emission of signals.

	@author Hamish Rodda <rodda@kde.org>
*/
class KateSmartCursorNotifier : public KTextEditor::SmartCursorNotifier
{
  friend class KateSmartCursor;

  public:
    KateSmartCursorNotifier();
    virtual ~KateSmartCursorNotifier();
};

#endif
