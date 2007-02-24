/*
   This file is part of KDE
   Copyright (C) 2006 Christian Ehrlicher <ch.ehrlicher@gmx.de>

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

#ifndef _KDEBASE_EXPORT_H
#define _KDEBASE_EXPORT_H

#include <kdemacros.h>

#ifdef MAKE_KATEINTERFACES_LIB
# define KATEINTERFACES_EXPORT KDE_EXPORT
#else
# if defined _WIN32 || defined _WIN64
#  define KATEINTERFACES_EXPORT KDE_IMPORT
# else
#  define KATEINTERFACES_EXPORT KDE_EXPORT
# endif
#endif

#endif // _KDEBASE_EXPORT_H

