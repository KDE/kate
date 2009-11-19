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

#include <ktexteditor/ktexteditor_export.h>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace KTextEditor
{
/**
 * \brief Config interface extension for the Document and View.
 *
 * \ingroup kte_group_view_extensions
 * \ingroup kte_group_doc_extensions
 *
 * \section config_intro Introduction
 *
 * The ConfigInterface provides methods to access and modify the low level
 * config information for a given Document or View. Examples of this config data can be
 * displaying the icon bar, showing line numbers, etc. This generally allows
 * access to settings that otherwise are only accessible during runtime.
 *
 * \section config_access Accessing the Interface
 *
 * The ConfigInterface is supposed to be an extension interface for a Document or View,
 * i.e. the Document or View inherits the interface \e provided that the
 * KTextEditor library in use implements the interface. Use qobject_cast to access
 * the interface:
 * \code
 * // ptr is of type KTextEditor::Document* or KTextEditor::View*
 * KTextEditor::ConfigInterface *iface =
 *     qobject_cast<KTextEditor::ConfigInterface*>( ptr );
 *
 * if( iface ) {
 *
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \section config_data Accessing Data
 *
 * A list of available config variables (or keys) can be optained by calling
 * configKeys(). For all available keys configValue() returns the corresponding
 * value as QVariant. A value for a given key can be set by calling
 * setConfigValue(). Right now, when using KatePart as editor component,
 * KTextEditor::View has support for the following tuples:
 *  - line-numbers [bool], show/hide line numbers
 *  - icon-bar [bool], show/hide icon bar
 *  - dynamic-word-wrap [bool], enable/disable dynamic word wrap
 *  - background-color [QColor], read/set the default background color
 *  - selection-color [QColor], read/set the default color for selections
 *
 * KTextEditor::Document has support for the following:
 *  - auto-brackets [bool], enable/disable automatic bracket completion
 *  - backup-on-save-local [bool], enable/disable backup when saving local files
 *  - backup-on-save-remote [bool], enable/disable backup when saving remote files
 *  - backup-on-save-suffix [string], set the suffix for file backups, e.g. "~"
 *  - backup-on-save-prefix [string], set the prefix for file backups, e.g. "."
 *
 * Either interface should emit the \p configChanged signal when appropriate.
 * TODO: Add to interface in KDE 5.
 *
 * For instance, if you want to enable dynamic word wrap of a KTextEditor::View
 * simply call
 * \code
 * iface->setConfigValue("dynamic-word-wrap", true);
 * \endcode
 *
 * \see KTextEditor::View, KTextEditor::Document
 * \author Matt Broadstone \<mbroadst@gmail.com\>
 */
class KTEXTEDITOR_EXPORT ConfigInterface
{
  public:
    ConfigInterface ();

    /**
     * Virtual destructor.
     */
    virtual ~ConfigInterface ();

  public:
    /**
     * Get a list of all available keys.
     */
    virtual QStringList configKeys() const = 0;
    /**
     * Get a value for the \p key.
     */
    virtual QVariant configValue(const QString &key) = 0;
    /**
     * Set a the \p key's value to \p value.
     */
    virtual void setConfigValue(const QString &key, const QVariant &value) = 0;

  private:
    class ConfigInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::ConfigInterface, "org.kde.KTextEditor.ConfigInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
