/*
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

    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
    Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)
*/
#ifndef _KTEXTEDITOR_VARIABLE_INTERFACE_H_
#define _KTEXTEDITOR_VARIABLE_INTERFACE_H_

#include <ktexteditor/ktexteditor_export.h>


#include <QtCore/QObject>

class QString;

namespace KTextEditor {

class Document;

/**
 * \brief Variable/Modeline extension interface for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section variface_intro Introduction
 *
 * The VariableInterface is designed to provide access to so called
 * "document variables" (also called modelines), for example variables
 * defined in files like "kate: variable value;" or the emacs style
 * "-*- variable: value -*-".
 *
 * The idea is to allow KTextEditor plugins and applications to use document
 * variables. A document implementing this interface should return values
 * for variables that it does not otherwise know how to use, since they
 * could be of interest for plugins. A Document implementing this interface
 * must emit the signal variableChanged() whenever a variable/value pair was
 * set, changed or removed.
 *
 * \note Implementations should check the document variables whenever the
 *       document was saved or loaded.
 *
 * \section variface_access Accessing the VariableInterface
 *
 * The VariableInterface is an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // doc is of type KTextEditor::Document*
 * KTextEditor::VariableInterface *iface =
 *     qobject_cast<KTextEditor::VariableInterface*>( doc );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Document, KTextEditor::Plugin
 * \author Anders Lund \<anders@alweb.dk\>
 */
class KTEXTEDITOR_EXPORT VariableInterface
{
  public:
    VariableInterface();

    /**
     * Virtual destructor.
     */
    virtual ~VariableInterface();

    /**
     * Get the value of the variable \p name.
     * \return the value or an empty string if the variable is not set or has
     *         no value.
     */
    virtual QString variable( const QString &name ) const = 0;

  //
  // signals!!
  //
  public:
    /**
     * The \p document emits this signal whenever the \p value of the
     * \p variable changed, this includes when a variable was initially set.
     * \param document document that emitted the signal
     * \param variable variable that changed
     * \param value new value for \e variable
     * \see variable()
     */
    virtual void variableChanged( Document* document, const QString &variable, const QString &value ) = 0;

  private:
    class VariableInterfacePrivate* const d;
};


} // namespace KTextEditor

Q_DECLARE_INTERFACE(KTextEditor::VariableInterface, "org.kde.KTextEditor.VariableInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
