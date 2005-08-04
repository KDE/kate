/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __ktexteditor_editor_h__
#define __ktexteditor_editor_h__

// our main baseclass of the KTextEditor::Editor
#include <QObject>

// pixmap of config page
#include <QPixmap>

// icon size enum
#include <kicontheme.h>

class KAboutData;
class KConfig;

namespace KTextEditor
{

class Document;
class ConfigPage;

/**
 * This class represents the Editor of the editor
 * Each KTextEditor part must reimplement this Editor,
 * to allow easy access to the editor object
 */
class KTEXTEDITOR_EXPORT Editor : public QObject
{
  Q_OBJECT

  public:
    /**
     * Editor Constructor.
     * @param parent parent object
     */
    Editor ( QObject *parent );

    /**
     * virtual destructor
     */
    virtual ~Editor ();

  /**
   * Methods to create and manage the documents.
   */
  public:
    /**
     * Create a new document object.
     * @param parent parent object
     * @return new KTextEditor::Document object
     */
    virtual Document *createDocument ( QObject *parent ) = 0;

    /**
     * Get a list of all documents of this editor.
     * @return list of all existing documents
     */
    virtual const QList<Document*> &documents () = 0;

  /**
   * General Information about this editor.
   */
  public:
    /**
     * Get the about data.
     * @return about data of this editor part
     */
    virtual const KAboutData *aboutData () const = 0;

  /**
   * Configuration management.
   */
  public:
    /**
     * Read editor configuration from its standard config.
     */
    virtual void readConfig () = 0;

    /**
     * Write editor configuration to its standard config.
     */
    virtual void writeConfig () = 0;

    /**
     * Read editor configuration from KConfig @e config.
     * @param config config object
     */
    virtual void readConfig (KConfig *config) = 0;

    /**
     * Write editor configuration to KConfig @e config.
     * @param config config object
     */
    virtual void writeConfig (KConfig *config) = 0;

    /**
     * Check, whether this editor has a configuration dialog.
     * @return @e true, if the editor has a configuration dialog,
     *         otherwise @e false
     */
    virtual bool configDialogSupported () const = 0;

    /**
     * Show the editor's config dialog, changes will be applied to the
     * editor, but not saved anywhere automagically, call @p writeConfig()
     * to save them.
     * @param parent parent widget
     */
    virtual void configDialog (QWidget *parent) = 0;

    /**
     * Get the number of available config pages.
     * If the editor returns a number < 1, it does not support config pages
     * and the embedding application should use @p configDialog() instead.
     * @return number of config pages
     */
    virtual int configPages () const = 0;

    /**
     * Get the config page with the @e number, config pages from 0 to
     * @p configPages()-1 are available if @p configPages() > 0.
     * @param number index of config page
     * @param parent parent widget for config page
     * @return created config page or NULL, if the number is out of bounds
     */
    virtual ConfigPage *configPage (int number, QWidget *parent) = 0;

    /**
     * Get a readable name for the config page @e number. The name should be
     * translated.
     * @param number index of config page
     * @return name of given page index
     */
    virtual QString configPageName (int number) const = 0;

    /**
     * Get a readable full name for the config page @e number. The name
     * should be translated.
     *
     * Example: If the name is "Filetypes", the full name could be
     * "Filetype Specific Settings". For "Shortcuts" the full name would be
     * something like "Shortcut Configuration".
     * @param number index of config page
     * @return full name of given page index
     */
    virtual QString configPageFullName (int number) const = 0;

    /**
     * Get a pixmap with @e size for the config page @e number.
     * @param number index of config page
     * @param size size of pixmap
     * @return pixmap of given page index
     */
    virtual QPixmap configPagePixmap (int number, int size = KIcon::SizeSmall) const = 0;

  signals:
    /**
     * The @e editor emits this signal whenever a @e document was successfully created.
     * @param editor editor which created the new document
     * @param document the newly created document instance
     * @see createDocument()
     */
    void documentCreated (Editor *editor, Document *document);
};

KTEXTEDITOR_EXPORT Editor *editor ( const char *libname );

}

#endif
