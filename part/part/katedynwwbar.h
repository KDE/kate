/* This file is part of the KDE libraries
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   
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

#ifndef _KateDynWWBar_H_
#define _KateDynWWBar_H_

#include <qwidget.h>
#include <qpixmap.h>

class KateDynWWBar : public QWidget
{
  Q_OBJECT      
  
  public:
    KateDynWWBar( class KateViewInternal* internalView, QWidget *parent );
    ~KateDynWWBar() {};

    virtual QSize sizeHint() const;
    
    void updateFont();
    

  private:
    void paintEvent( QPaintEvent* );
    void paintBar (int x, int y, int width, int height);
    
    class KateView *m_view;   
    class KateDocument *m_doc;
    class KateViewInternal *m_viewInternal;
    QPixmap arrow;
};
#endif
