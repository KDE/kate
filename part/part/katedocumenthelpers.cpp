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

#include "katedocumenthelpers.h"
#include "katedocumenthelpers.moc"

#include "katedocument.h"
#include "kateview.h"

#include <kpopupmenu.h>
#include <klocale.h>

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
  if (m_doc->activeView())
    m_doc->activeView()->copy();
}

void KateBrowserExtension::print()
{
  m_doc->printDialog();
}

void KateBrowserExtension::slotSelectionChanged()
{
  if (m_doc->activeView())
    emit enableAction( "copy", m_doc->activeView()->hasSelection() );
}

KateExportAction::KateExportAction(const QString& text, QObject* parent, const char* name)
        : Kate::ActionMenu(text, parent, name)
{
  filter.clear();
  filter<<QString("kate_html_export");
  popupMenu()->insertItem (i18n("&HTML..."),0);
  connect(popupMenu(),SIGNAL(activated(int)),this,SLOT(filterChoosen(int)));
  m_doc=0L;
}

void KateExportAction::updateMenu (Kate::Document *doc)
{
  m_doc = doc;
}

void KateExportAction::filterChoosen(int id)
{
  Kate::Document *doc = m_doc;

  if (!doc)
    return;

  doc->exportAs(*filter.at(id));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
