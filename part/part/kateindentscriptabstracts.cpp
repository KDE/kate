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
#include <klocale.h>

//BEGIN KateIndentScriptImplAbstractImpl

KateIndentScriptImplAbstract::KateIndentScriptImplAbstract(KateIndentScriptManagerAbstract *manager, const QString& internalName,
        const QString  &filePath, const QString &niceName,
        const QString &license, bool hasCopyright, double version):m_refcount(0),m_manager(manager),m_internalName(internalName),m_filePath(filePath),m_niceName(niceName),
            m_license(license),m_hasCopyright(hasCopyright),m_version(version)
{
}

KateIndentScriptImplAbstract::~KateIndentScriptImplAbstract()
{
}

void KateIndentScriptImplAbstract::incRef()
{
  kDebug(13050)<<"KateIndentScriptImplAbstract::incRef()"<<endl;
  m_refcount++;
}

void KateIndentScriptImplAbstract::decRef()
{
  kDebug(13050)<<"KateIndentScriptImplAbstract::decRef()"<<endl;
  m_refcount--;
}


QString KateIndentScriptImplAbstract::internalName() { return m_internalName;}
QString KateIndentScriptImplAbstract::filePath() { return m_filePath;}
QString KateIndentScriptImplAbstract::niceName() { return m_niceName;}
QString KateIndentScriptImplAbstract::license()  { if (!m_hasCopyright) return i18n("tainted, no copyright notice. license(%1)").arg(m_license); else return m_license;}
QString KateIndentScriptImplAbstract::copyright() { if (!m_hasCopyright) return i18n("Script has no copyright notice"); else return m_manager->copyright(this);}
double KateIndentScriptImplAbstract::version() { return m_version;}



//END
