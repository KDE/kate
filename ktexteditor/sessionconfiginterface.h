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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_SESSIONCONFIGINTERFACE_H
#define KDELIBS_KTEXTEDITOR_SESSIONCONFIGINTERFACE_H

#include <kdelibs_export.h>

class KConfig;

namespace KTextEditor
{

/**
 * Session config interface extension for the Document and the View.
 *
 * <b>Introduction</b>\n
 *
 * This is an interface to session-specific configuration of the
 * Document, Plugin and PluginViewInterface classes.
 *
 * <b>Accessing the SessionConfigInterface</b>\n
 *
 * The SessionConfigInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface @e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * @code
 *   // doc is of type KTextEditor::Document*
 *   KTextEditor::SessionConfigInterface *iface =
 *       qobject_cast<KTextEditor::SessionConfigInterface*>( doc );
 *
 *   if( iface ) {
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * @endcode

@todo dh: document functions and this interface, not clear yet.

 * @see KTextEditor::Document, KTextEditor::View
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT SessionConfigInterface
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~SessionConfigInterface() {}

  //
  // SLOTS !!!
  //
  public:
    /**
     * Read session config settings of only this document/view/plugin
     * In case of the document, that means for example it should reload the file,
     * restore all marks, ...
     * @param config read the session settings from this KConfig
     */
    virtual void readSessionConfig (KConfig *config) = 0;

    /**
     * Write session settings to the @p config.
     *
     * of only this document/view/plugin
     * In case of the document, that means for example it should reload the file,
     * restore all marks, ...
     * @param config write the session settings to this KConfig
     */
    virtual void writeSessionConfig (KConfig *config) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SessionConfigInterface, "org.kde.KTextEditor.SessionConfigInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
