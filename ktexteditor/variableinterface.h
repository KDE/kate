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
    the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
    Boston, MA 02110-1301, USA.

    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/
#ifndef _KTEXTEDITOR_VARIABLE_INTERFACE_H_
#define _KTEXTEDITOR_VARIABLE_INTERFACE_H_

#include <kdelibs_export.h>

class QString;

namespace KTextEditor {

class Document;

/**
 * This interface is designed to provide access to "document variables",
 * for example variables defined in files like "kate: variable value;"
 * or the emacs style "-*- variable: value -*-".
 *
 * The purpose is to allow KTextEditor plugins to use variables.
 * A document implementing this interface should return values for variable
 * that it does not otherwise know how to use, since they could be of
 * interrest to plugins. A document implementing this interface must emit the
 * signal @p variableChanged() whenever a variable is set that it will return
 * a value for.
 *
 * @short KTextEditor interface to Document Variables
 */
class KTEXTEDITOR_EXPORT VariableInterface
{
  public:
    /**
     * virtual destructor
     */
    virtual ~VariableInterface() {}

    /**
     * Get the value of the variable @e name.
     * @return the value or an empty string if the variable is not set or has
     *         no value.
     */
    virtual QString variable( const QString &name ) const = 0;

    //
    // signals!!
    //
  public:
    /**
     * The @e document emits this signal whenever the @e value of the
     * @e variable changes, this includes when a variable initially is set.
     * @param document document that emitted the signal
     * @param variable variable that changed
     * @param value new value for @e variable
     */
    virtual void variableChanged( Document* document, const QString &variable, const QString &value ) = 0;
};


} // namespace KTextEditor

Q_DECLARE_INTERFACE(KTextEditor::VariableInterface, "org.kde.KTextEditor.VariableInterface")

#endif //_KTEXTEDITOR_VARIABLE_INTERFACE_H_

// kate: space-indent on; indent-width 2; replace-tabs on;
