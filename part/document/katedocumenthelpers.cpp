/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
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

#include "katedocumenthelpers.h"
#include "katedocumenthelpers.moc"

#include "katedocument.h"
#include "kateview.h"

#include <kmenu.h>
#include <klocale.h>

namespace KTextEditor { class View; }

KateBrowserExtension::KateBrowserExtension( KateDocument* doc )
: KParts::BrowserExtension( doc ),
  m_doc (doc)
{
  setObjectName( "katepartbrowserextension" );
  emit enableAction( "print", true );
}

void KateBrowserExtension::print()
{
  m_doc->printDialog();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
