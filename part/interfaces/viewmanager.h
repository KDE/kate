/***************************************************************************
                          viewmanager.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
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
 ***************************************************************************/

#ifndef _KATE_VIEWMANAGER_INCLUDE_
#define _KATE_VIEWMANAGER_INCLUDE_

#include <qwidget.h>
#include <kurl.h>

namespace Kate
{

/** An interface to the kate viewmanager.

 */
class ViewManager : public QWidget
{
  Q_OBJECT

  public:
    ViewManager (QWidget *);
    virtual ~ViewManager ();

    /** Returns a pointer to the currently active KateView */
    virtual class View *getActiveView() { return 0L; };

    /** Opens the file pointed to by URL */
    virtual void openURL (KURL) { ; };
};

};

#endif
