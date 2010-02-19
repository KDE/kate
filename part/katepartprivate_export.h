/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2007 David Faure <faure@kde.org>
 *  Copyright (C) 2009 by Bernhard Beschow <bbeschow@cs.tu-berlin.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATEPARTPRIVATE_EXPORT_H
#define KATEPARTPRIVATE_EXPORT_H

/* needed for KDE_EXPORT and KDE_IMPORT macros */
#include <kdemacros.h>

/* Classes from the kate part, which are exported only for unit tests */
#if defined(COMPILING_KATEPART)
  /* We are compling the kate part, so don't export symbols */
  #define KATEPART_TESTS_EXPORT
#elif defined(COMPILING_KATEPARTTESTLIB)
  /* We build the test library, so export symbols */
  #define KATEPART_TESTS_EXPORT KDE_EXPORT
#else
  /* We are using this library */
  #define KATEPART_TESTS_EXPORT KDE_IMPORT
#endif

#endif  // KATEPARTPRIVATE_EXPORT_H
