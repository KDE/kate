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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
               
// $Id$

#include "katebrowserextension.h"
#include "katebrowserextension.moc"

#include "katedocument.h"

KateBrowserExtension::KateBrowserExtension( KateDocument* doc )
: KParts::BrowserExtension( doc, "katepartbrowserextension" ),
  m_doc (doc)
{
  connect( doc, SIGNAL( selectionChanged() ),
           this, SLOT( slotSelectionChanged() ) );
  emit enableAction( "print", true );
}

void KateBrowserExtension::copy()
{
  m_doc->copy();
}

void KateBrowserExtension::print()
{
  m_doc->printDialog();
}

void KateBrowserExtension::slotSelectionChanged()
{
  emit enableAction( "copy", m_doc->hasSelection() );
}
