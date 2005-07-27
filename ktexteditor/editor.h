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
     * Editor Constructor
     * @param parent parent object
     */
    Editor ( QObject *parent );

    /**
     * virtual destructor
     */
    virtual ~Editor ();

  /**
   * Stuff to create and manage the documents
   */
  public:
    /**
     * Create a new document object
     * @param parent parent object
     * @return created KTextEditor::Document
     */
    virtual Document *createDocument ( QObject *parent ) = 0;

    /**
     * Returns a list of all documents of this editor.
     * @return list of all existing documents
     */
    virtual const QList<Document*> &documents () = 0;

  /**
   * General Information about this editor
   */
  public:
    /**
     * return the about data
     * @return about data of this editor part
     */
    virtual const KAboutData *aboutData () const = 0;

  /**
   * Configuration management
   */
  public:
    /**
     * Read editor configuration from it's standard config
     */
    virtual void readConfig () = 0;

    /**
     * Write editor configuration to it's standard config
     */
    virtual void writeConfig () = 0;

    /**
     * Read editor configuration from given config object
     * @param config config object
     */
    virtual void readConfig (KConfig *config) = 0;

    /**
     * Write editor configuration to given config object
     * @param config config object
     */
    virtual void writeConfig (KConfig *config) = 0;

    /**
     * Does this editor support a config dialog
     * @return does this editor have a config dialog?
     */
    virtual bool configDialogSupported () const = 0;

    /**
     * Shows a config dialog for the part, changes will be applied
     * to the editor, but not saved anywhere automagically, call
     * writeConfig to save them
     * @param parent parent widget
     */
    virtual void configDialog (QWidget *parent) = 0;

    /**
     * Number of available config pages
     * If the editor returns a number < 1, it doesn't support this
     * and the embedding app should use the configDialog () instead
     * @return number of config pages
     */
    virtual int configPages () const = 0;

    /**
     * returns config page with the given number,
     * config pages from 0 to configPages()-1 are available
     * if configPages() > 0
     * @param number index of config page
     * @param parent parent widget for config page
     * @return created config page
     */
    virtual ConfigPage *configPage (int number, QWidget *parent) = 0;

    /**
     * Retrieve name for config page
     * @param number index of config page
     * @return name of given page index
     */
    virtual QString configPageName (int number) const = 0;

    /**
     * Retrieve full name for config page
     * @param number index of config page
     * @return full name of given page index
     */
    virtual QString configPageFullName (int number) const = 0;

    /**
     * Retrieve pixmap for config page
     * @param number index of config page
     * @param size size of pixmap
     * @return pixmap of given page index
     */
    virtual QPixmap configPagePixmap (int number, int size = KIcon::SizeSmall) const = 0;

  signals:
    /**
     * emit this after successfull document creation in createDocument
     * @param editor editor which created the new document
     * @param document the newly created document instance
     */
    void documentCreated (Editor *editor, Document *document);
};

KTEXTEDITOR_EXPORT Editor *editor ( const char *libname );

}

#endif
