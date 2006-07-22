/* This file is part of the KDE project
   Copyright (C) 2006 Matt Broadstone (mbroadst@gmail.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_CONFIGINTERFACE_H
#define KDELIBS_KTEXTEDITOR_CONFIGINTERFACE_H

#include <kdelibs_export.h>

namespace KTextEditor
{

class Document;


/**
 * \brief Config Interface for the View.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section config_intro Introduction
 *
 * The ConfigInterface provides methods to access to low level config 
 * information for a given View. Examples of this config data can be
 * displaying the icon bar, showing line numbers, etc. This generally
 * allows access to settings that otherwise are only accesible during
 * runtime.
 *
 * \section config_access Accessing the Interface
 *
 * The ConfigInterface is supposed to be an extension interface for a View,
 * i.e. the View inherits the interface \e provided that the
 * KTextEditor library in use implements the interface. Use qobject_cast to access
 * the interface:
 * \code
 *   // view is of type KTextEditor::View*
 *   KTextEditor::ConfigInterface *iface =
 *       qobject_cast<KTextEditor::ConfigInterface*>( view );
 *
 *   if( iface ) {
 *   
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * \endcode
 *
 * \section config_accessing Accessing Data
 *
 * Once you access the interface you can query what config variables are
 * provided by calling configKeys(). Once you have these keys, you can 
 * check to see if the variable you want access to is provided in the keys
 * and then use setConfigValue() to change the data, or simply configValue()
 * to check the current setting. Currently only LineNumbers, IconBar, and
 * DynamicWordWrap are implemented. 
 *
 * \see KTextEditor::View
 * \author Matt Broadstone \<mbroadst@gmail.com\>
 */
class KTEXTEDITOR_EXPORT ConfigInterface
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~ConfigInterface () {}

  public:
    virtual QStringList configKeys() const = 0;
    virtual QVariant configValue(const QString &key) = 0;
    virtual void setConfigValue(const QString &key, const QVariant &value) = 0; 
};

}

Q_DECLARE_INTERFACE(KTextEditor::ConfigInterface, "org.kde.KTextEditor.ConfigInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
