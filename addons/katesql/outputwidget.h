/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Joseph Wenninger <jowenn@kde.org>
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

#ifndef _KATE_SQL_OUTPUT_VIEW_
#define _KATE_SQL_OUTPUT_VIEW_

#include <QTabWidget>

class TextOutputWidget;
class DataOutputWidget;

class KateSQLOutputWidget : public QTabWidget
{
  Q_OBJECT

  public:
    
    KateSQLOutputWidget (QWidget *parent);

    ~KateSQLOutputWidget () override;
    
    TextOutputWidget *textOutputWidget() const { return m_textOutputWidget; }
    DataOutputWidget *dataOutputWidget() const { return m_dataOutputWidget; }
  private:

    TextOutputWidget *m_textOutputWidget;
    DataOutputWidget *m_dataOutputWidget;

  
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
