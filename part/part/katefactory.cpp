/* This file is part of the KDE libraries
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>

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

#include "kateglobal.h"
#include "katedocument.h"

#include <ktexteditor/factory.h>

/**
 * wrapper factory to be sure nobody external deletes our kateglobal object
 * each instance will just increment the reference counter of our internal
 * super private global instance ;)
 */
class KateFactory : public KTextEditor::Factory
{
  public:
    /**
     * constructor, increments reference count of KateGlobal
     * @param parent parent object
     * @param name name of factory
     */
    KateFactory ( QObject *parent = 0 )
      : KTextEditor::Factory (parent)
    {
      KateGlobal::incRef ();
    }

    /**
     * destructor, decrements reference count of KateGlobal
     */
    virtual ~KateFactory ()
    {
      KateGlobal::decRef ();
    }

    KTextEditor::Editor *editor () { return KateGlobal::self(); }

    /**
     * reimplemented create object method
     * @param parentWidget parent widget
     * @param parent QObject parent
     * @param args additional arguments
     * @return constructed part object
     */
    KParts::Part *createPartObject ( QWidget *parentWidget, QObject *parent, const char *_classname, const QStringList & )
    {
      QByteArray classname( _classname );

      // match both :: and /

      // default to the kparts::* behavior of having one single widget() if the user don't requested a pure document
      bool bWantSingleView = !(( classname == "KTextEditor/Document" ) || ( classname == "KTextEditor::Document" ));

      // does user want browserview?
      bool bWantBrowserView = ( classname == "Browser/View" ) || ( classname == "Browser::View" );

      // should we be readonly?
      bool bWantReadOnly = (bWantBrowserView || ( classname == "KParts/ReadOnlyPart" ) || ( classname == "KParts::ReadOnlyPart" ));

      KParts::ReadWritePart *part = new KateDocument (bWantSingleView, bWantBrowserView, bWantReadOnly, parentWidget, parent);
      part->setReadWrite( !bWantReadOnly );

      return part;
    }
};

K_EXPORT_COMPONENT_FACTORY( katepart, KateFactory )

// kate: space-indent on; indent-width 2; replace-tabs on;
