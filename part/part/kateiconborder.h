/* This file is part of the KDE libraries
   Copyright (C) 2001 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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

#ifndef _KateIconBorder_H_
#define _KateIconBorder_H_

#include "kateglobal.h"

#include <qwidget.h>

class KateView;
class KateViewInternal;
class QPopupMenu;

class KateIconBorder : public QWidget
{
Q_OBJECT
public:
    KateIconBorder(KateView *view, class KateViewInternal *internalView);
    ~KateIconBorder();

    enum Status { None=0, Icons=1, LineNumbers=2, FoldingMarkers=4 };
    void paintLine(int i,int pos);
    int width();

protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);

private:

    KateView *myView;
    class KateViewInternal *myInternalView;
    bool lmbSetsBreakpoints;
    int iconPaneWidth;
    int cachedLNWidth;
    uint linesAtLastCheck; // only calculate width if number of lines has changed

    unsigned int oldEditableMarks;
    QPopupMenu *markMenu;

    void createMarkMenu();

signals:
    void toggleRegionVisibility(unsigned int);
};
#endif
