/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __ktexteditor_view_h__
#define __ktexteditor_view_h__

#include <qwidget.h>
#include <kxmlguiclient.h>

namespace KTextEditor
{

/**
 * The View class encapsulates a single view into the document.
 */

class View : public QWidget, public KXMLGUIClient
{
  friend class PrivateView;

  Q_OBJECT

  public:
    /**
    * Create a new view to the given document. The document must be non-null.
    */
    View ( class Document *, QWidget *parent, const char *name = 0 );
    virtual ~View ();

    unsigned int viewNumber () const;
    
    QCString viewDCOPSuffix () const;

    /**
    * Acessor to the parent Document.
    */
    virtual class Document *document () const = 0;
    
  private:
    class PrivateView *d;
    static unsigned int globalViewNumber;
    unsigned int myViewNumber;
};

};

#endif
