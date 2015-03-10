/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#include "outputwidget.h"
#include "dataoutputwidget.h"
#include "textoutputwidget.h"
#include "klocalizedstring.h"

KateSQLOutputWidget::KateSQLOutputWidget (QWidget *parent)
  : QTabWidget (parent)
  
{
  addTab (m_textOutputWidget=new TextOutputWidget (this),
          QIcon::fromTheme(QLatin1String("view-list-text")), i18nc("@title:window", "SQL Text Output"));
  addTab (m_dataOutputWidget=new DataOutputWidget(this),
          QIcon::fromTheme(QLatin1String("view-form-table")),i18nc("@title:window", "SQL Data Output"));

}

KateSQLOutputWidget::~KateSQLOutputWidget ()
{
}

// kate: space-indent on; indent-width 2; replace-tabs on;

