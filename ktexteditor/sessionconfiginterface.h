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
 * Session config interface extension for the Document, View and Plugin.
 *
 * <b>Introduction</b>\n
 *
 * The SessionConfigInterface is an extension for Documents, Views and Plugins
 * to add support for session-specific configuration settings.
 * readSessionConfig() is called whenever session-specific settings are to be
 * read from the given KConfig* and writeSessionConfig() whenever they are to
 * be written, for example when a session changed or was closed.
 *
 * <b>Accessing the SessionConfigInterface</b>\n
 *
 * The SessionConfigInterface is supposed to be an extension interface for a
 * Document, a View or a Plugin, i.e. the Document/View/Plugin inherits the
 * interface @e provided that it implements the interface. Use qobject_cast to
 * access the interface:
 * @code
 *   // object is of type KTextEditor::Document* or View* or Plugin*
 *   KTextEditor::SessionConfigInterface *iface =
 *       qobject_cast<KTextEditor::SessionConfigInterface*>( object );
 *
 *   if( iface ) {
 *       // interface is supported
 *       // do stuff
 *   }
 * @endcode
 *
 * @see KTextEditor::Document, KTextEditor::View, KTextEditor::Plugin
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
     * Read session settings from the given @p config.
     *
     * That means for example
     *  - a Document should reload the file, restore all marks etc...
     *  - a View should scroll to the last position and restore the cursor
     *    position etc...
     *  - a Plugin should restore session specific settings
     *
     * @param config read the session settings from this KConfig
     * @see writeSessionConfig()
     */
    virtual void readSessionConfig (KConfig *config) = 0;

    /**
     * Write session settings to the @p config.
     * See readSessionConfig() for more details.
     *
     * @param config write the session settings to this KConfig
     * @see readSessionConfig()
     */
    virtual void writeSessionConfig (KConfig *config) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SessionConfigInterface, "org.kde.KTextEditor.SessionConfigInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
