/***************************************************************************
                          docmanager.h -  description
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

#ifndef _KATE_DOCMANAGER_INCLUDE_
#define _KATE_DOCMANAGER_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{
/** This interface provides access to the Kate Document Manager.
*/
class DocManager : public QObject
{
  Q_OBJECT

  public:
    DocManager ();
    virtual ~DocManager ();

  public:
    /** Returns a pointer to the document indexed by n in the managers internal list.
    */
    virtual class Document *getNthDoc (uint ) { return 0L; };
    /** Returns a pointer to the currently active document or NULL if no document is open.
    */
    virtual class Document *getCurrentDoc () { return 0L; };
    /** Returns a pointer to the first document in the managers internal list or NULL if the list is empty.
    */
    virtual class Document *getFirstDoc () { return 0L; };
    /** Returns a pointer to the next document in the managers internal list or NULL if no such doc exists.
    */
    virtual class Document *getNextDoc () { return 0L; };
    /** Returns a pointer to the document with the given ID or NULL if no such document exists.
    */
    virtual class Document *getDocWithID (uint ) { return 0L; };

    /** Returns the ID of the document located at url if such a document is known by the manager.
     */
    virtual int findDoc (KURL ) { return 0L; };
    /** Returns true if the document located at url is open, otherwise false.
     */
    virtual bool isOpen (KURL ) { return 0L; };

    /** returns the number of documents managed by this manager.
    */
    virtual uint docCount () { return 0L; };
};

};

#endif
