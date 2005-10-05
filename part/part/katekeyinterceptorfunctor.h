/* This file is part of the KDE libraries
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

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
#ifndef _KATE_KEY_INTERCEPTOR_FUNCTOR_H
#define _KATE_KEY_INTERCEPTOR_FUNCTOR_H
#include <kshortcut.h>
class KateKeyInterceptorFunctor {
  public:
  virtual ~KateKeyInterceptorFunctor(){}
  virtual bool operator()(KKey key)=0;
};
#endif
