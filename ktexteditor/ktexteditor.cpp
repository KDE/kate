/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "document.h"
#include "view.h"

#include "document.moc"
#include "view.moc"

#include "editinterface.h"
#include "selectioninterface.h"
#include "cursorinterface.h"
#include "undointerface.h"
#include "clipboardinterface.h"

using namespace KTextEditor;

View::View( Document *, QWidget *parent, const char *name ) : QWidget( parent, name )
{
}

View::~View()
{
}

Document::Document( QObject *parent, const char *name ) : KParts::ReadWritePart( parent, name )
{
}

Document::~Document()
{
}
