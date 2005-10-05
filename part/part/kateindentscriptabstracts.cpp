/* This file is part of the KDE libraries
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>

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

#include "kateindentscriptabstracts.h"

#include <kdebug.h>
#include <qstring.h>

//BEGIN KateIndentScriptImplAbstractImpl

KateIndentScriptImplAbstract::KateIndentScriptImplAbstract(const QString& internalName,
        const QString  &filePath, const QString &niceName,
        const QString &copyright, double version):m_refcount(0),m_filePath(filePath),m_niceName(niceName),
            m_copyright(copyright),m_version(version)
{
}

KateIndentScriptImplAbstract::~KateIndentScriptImplAbstract()
{
}

void KateIndentScriptImplAbstract::incRef()
{
  kdDebug(13050)<<"KateIndentScriptImplAbstract::incRef()"<<endl;
  m_refcount++;
}

void KateIndentScriptImplAbstract::decRef()
{
  kdDebug(13050)<<"KateIndentScriptImplAbstract::decRef()"<<endl;
  m_refcount--;
}

//END
