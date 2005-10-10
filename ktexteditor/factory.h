/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

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

#ifndef KDELIBS_KTEXTEDITOR_FACTORY_H
#define KDELIBS_KTEXTEDITOR_FACTORY_H

// our main baseclass of the KTextEditor::Document
#include <kparts/factory.h>

/**
 * Namespace for the KDE Text Editor Interfaces.
 * These interfaces provide easy access to editor parts for the
 * applications embedding them. At the moment they are at least
 * supported by both the Kate Part and the Yzis Part.
 */
namespace KTextEditor
{

class Editor;

/**
 * @brief Accessor to the Editor implementation.
 *
 * <b>Introduction</b>\n
 *
 * The Factory provides access to the chosen Editor (selected with
 * KTextEditor::EditorChooser). The Editor itself then provides methods
 * to handle documents and config options.
 *
 * To access the Editor use editor().
 *
 * Each KTextEditor implementation must reimplement this factory to allow
 * access to the editor object.
 *
 * @see KParts::Factory, KTextEditor::Editor
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Factory : public KParts::Factory
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a new Factory with @p parent.
     * @param parent parent object
     */
    Factory ( QObject *parent );

    /**
     * Virtual destructor.
     */
    virtual ~Factory ();

    /**
     * Get the global Editor object. The editor part implementation @e must
     * ensure that this object lives as long as any factory or document
     * object exists.
     * @return global KTextEditor::Editor object
     */
    virtual Editor *editor () = 0;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
