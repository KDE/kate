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
* This is an interface to session-specific configuration of the
* Document, Plugin and PluginViewInterface classes.
*/
class KTEXTEDITOR_EXPORT SessionConfigInterface
{
  public:
    virtual ~SessionConfigInterface() {}

  //
  // slots !!!
  //
  public:
    /**
     * Read/Write session config of only this document/view/plugin
     * In case of the document, that means for example it should reload the file,
     * restore all marks, ...
    */
    virtual void readSessionConfig (KConfig *) = 0;
    virtual void writeSessionConfig (KConfig *) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SessionConfigInterface, "org.kde.KTextEditor.SessionConfigInterface")

#endif
