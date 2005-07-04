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

/**
 * This interface is designed to provide access to "document variables",
 * for example variables defined in files like "kate: variable value;"
 * or the emacs style "-*- variable: value -*-".
 *
 * The purpose is to allow KTE plugins to use variables.
 * A document implementing this interface should return values for variable
 * that it does not otherwise know how to use, since they could be of interrest
 * to plugins. A document implementing this interface must emit the variableChanged()
 * signal whenever a variable is set that it will return a value for.
 *
 * @short KTextEditor interface to Document Variables
 */
class KTEXTEDITOR_EXPORT VariableInterface
{
  public:
    VariableInterface();
    virtual ~VariableInterface();

    unsigned int variableInterfaceNumber();

    /**
    * @return the value of the variable @p name, or an empty string if the
    * variable is not set or has no value.
    */
    virtual QString variable( const QString &name ) const = 0;

    //
    // signals!!
    //
  public:
    /**
    * Signal: emitted when a variable is set
    */
    virtual void variableChanged( const QString &variable, const QString &value ) = 0;

  private:
    static unsigned int globalVariableInterfaceNumber;
    unsigned int myVariableInterfaceNumber;
};


KTEXTEDITOR_EXPORT VariableInterface *variableInterface( class Document * );
} // namespace KTextEditor
#endif //_KTEXTEDITOR_VARIABLE_INTERFACE_H_
