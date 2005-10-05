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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_DOCUMENT_HELPERS__
#define __KATE_DOCUMENT_HELPERS__

#include <kparts/browserextension.h>

#include <qstringlist.h>
#include <qpointer.h>

class KateDocument;

/**
 * Interface for embedding KateDocument into a browser
 */
class KateBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT

  public:
    /**
     * Constructor
     * @param doc parent document
     */
    KateBrowserExtension( KateDocument* doc );

  public slots:
    /**
     * copy text to clipboard
     */
    void copy();

    /**
     * selection has changed
     */
    void slotSelectionChanged();

    /**
     * print the current file
     */
    void print();

  private:
    /**
     * parent document
     */
    KateDocument* m_doc;
};

#endif

