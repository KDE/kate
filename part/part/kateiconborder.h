/***************************************************************************
                          kateiconborder.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KateIconBorder_H_
#define _KateIconBorder_H_

#include "kateglobal.h"

#include <qwidget.h>

class KateView;
class KateViewInternal;

class KateIconBorder : public QWidget
{
Q_OBJECT
public:
    KateIconBorder(KateView *view, class KateViewInternal *internalView);
    ~KateIconBorder();

    void paintLine(int i);

protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);

private:

    KateView *myView;
    class KateViewInternal *myInternalView;
    bool lmbSetsBreakpoints;
};
#endif
