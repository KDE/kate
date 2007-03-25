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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_FACTORY_H
#define KDELIBS_KTEXTEDITOR_FACTORY_H

#include <ktexteditor/ktexteditor_export.h>
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
 * \brief Accessor to the Editor implementation.
 *
 * Topics:
 *  - \ref factory_intro
 *  - \ref factory_example
 *  - \ref factory_notes
 *
 * \section factory_intro Introduction
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
 * \section factory_example Creating an Editor Part
 * To get a kate part the following code snippet can be used:
 * \code
 * KLibFactory* factory = KLibLoader::self()->factory("katepart");
 * KTextEditor::Factory* kte_factory =
 *     qobject_cast<KTextEditor::Factory*>(factory);
 *
 * if(kte_factory) {
 *     // valid editor factory, it is possible to access the editor now
 *     KTextEditor::Editor* editor = kte_factory->editor();
 * } else {
 *     // error
 * }
 * \endcode
 * If another editor part is desired substitue the string "katepart" with the
 * corresponding library name.
 *
 * However, if you are only interested in getting the editor part (which is
 * usually the case) a simple call of
 * \code
 * KTextEditor::Editor* editor = KTextEditor::editor("katepart");
 * \endcode
 * is enough.
 *
 * \section factory_notes Notes
 * It is recommend to use the EditorChooser to get the used editor part. This
 * way the user can choose the editor implementation. The Factory itself is
 * not needed to get the Editor with the help of the EditorChooser.
 *
 * \see KParts::Factory, KTextEditor::Editor
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Factory : public KParts::Factory
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a new Factory with \p parent.
     * \param parent parent object
     */
    Factory ( QObject *parent );

    /**
     * Virtual destructor.
     */
    virtual ~Factory ();

    /**
     * Get the global Editor object. The editor part implementation \e must
     * ensure that this object lives as long as any factory or document
     * object exists.
     * \return global KTextEditor::Editor object
     */
    virtual Editor *editor () = 0;

  private:
    class FactoryPrivate* const d;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
