/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katedocumenthelpers.h"
#include "katedocumenthelpers.moc"

#include "katedocument.h"
#include "kateview.h"

#include <kpopupmenu.h>
#include <klocale.h>

namespace KTextEditor { class View; }

KateBrowserExtension::KateBrowserExtension( KateDocument* doc )
: KParts::BrowserExtension( doc ),
  m_doc (doc)
{
  setObjectName( "katepartbrowserextension" );
  connect( doc, SIGNAL( activeViewSelectionChanged(KTextEditor::View*) ),
           this, SLOT( slotSelectionChanged() ) );
  emit enableAction( "print", true );
}

void KateBrowserExtension::copy()
{
  if (m_doc->activeView())
    m_doc->activeKateView()->copy();
}

void KateBrowserExtension::print()
{
  m_doc->printDialog();
}

void KateBrowserExtension::slotSelectionChanged()
{
  if (m_doc->activeView())
    emit enableAction( "copy", m_doc->activeKateView()->selection() );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
