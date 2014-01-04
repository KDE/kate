/* This file is part of the KDE libraries
 *  Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.*/

#ifndef KDELIBS_KTEXTEDITOR_EDITOR_H
#define KDELIBS_KTEXTEDITOR_EDITOR_H

#include <ktexteditor/configpageinterface.h>

#include <QObject>
#include <QIcon>

class KAboutData;
class KConfig;

namespace KTextEditor
{

class Application;
class Document;

/**
 * \brief Accessor interface for Editor part.
 *
 * Topics:
 *  - \ref editor_intro
 *  - \ref editor_config
 *  - \ref editor_notes
 *  - \ref editor_extensions
 *
 * \section editor_intro Introduction
 *
 * The Editor part can be accessed via the KTextEditor::Factory or the
 * KTextEditor::EditorChooser and provides
 * general information and configuration methods for the Editor
 * implementation, for example KAboutData by using aboutData().
 *
 * The Editor implementation has a list of all opened documents. Get this
 * list with documents(). To create a new Document call createDocument(). The
 * signal documentCreated() is emitted whenever the Editor created a new
 * document.
 *
 * \section editor_config Editor Configuration
 *
 * The config dialog can be shown with configDialog().
 * Instead of using the config dialog, the config
 * pages can be embedded into the application's config dialog. To do this,
 * configPages() returns the number of
 * config pages the Editor implementation provides and configPage() returns
 * the requested page. Further, a config page has a short descriptive name,
 * get it with configPageName(). You can get more detailed name by using
 * configPageFullName(). Also every config page has a pixmap, get it with
 * configPagePixmap(). The configuration can be saved and loaded with
 * readConfig() and writeConfig().
 *
 * \note We recommend to embedd the config pages into the main application's
 *       config dialog instead of using a separate config dialog, if the config
 *       dialog does not look cluttered then. This way, all settings are grouped
 *       together in one place.
 *
 * \section editor_notes Implementation Notes
 *
 * Usually only one instance of the Editor exists. The Kate Part
 * implementation internally uses a static accessor to make sure that only one
 * Kate Part Editor object exists. So several factories still use the same
 * Editor.
 *
 * readConfig() and writeConfig() should be forwarded to all loaded plugins
 * as well, see KTextEditor::Plugin::readConfig() and
 * KTextEditor::Plugin::writeConfig().
 *
 * \section editor_extensions Editor Extension Interfaces
 *
 * There is only a single extension interface for the Editor: the
 * CommandInterface. With the CommandInterface it is possible to add and
 * remove new command line commands which are valid for all documents. Common
 * use cases are for example commands like \e find or setting document
 * variables. For further details read the detailed descriptions in the class
 * KTextEditor::CommandInterface.
 *
 * \see KTextEditor::Factory, KTextEditor::Document, KTextEditor::ConfigPage
 *      KTextEditor::Plugin, KTextEditor::CommandInterface
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Editor : public QObject, public ConfigPageInterface
{
  Q_OBJECT

  protected:
    /**
     * Constructor.
     *
     * Create the Editor object with \p parent.
     * \param parent parent object
     */
    Editor ( QObject *parent );

    /**
     * Virtual destructor.
     */
    virtual ~Editor ();

  public:
    /**
     * Accessor to get the Editor instance.
     *
     * That Editor instance can be qobject-cast to specific extensions.
     * If the result of the cast is not NULL, that extension is supported.
     * 
     * This object will stay alive until QCoreApplication terminates.
     * You shall not delete it yourself.
     * There is only ONE instance of this per process.
     *
     * \return Editor controller, after initial construction, will live until QCoreApplication is terminating.
     */
    static Editor *instance ();
    
  public:
    /**
     * Set the global application object.
     * This will allow the editor component to access
     * the hosting application.
     * @param application application object
     */
    virtual void setApplication (KTextEditor::Application *application) = 0;
    
    /**
     * Current hosting application, if any set.
     * @return current application object or nullptr
     */
    virtual KTextEditor::Application *application () const = 0;

  /*
   * Methods to create and manage the documents.
   */
  public:
    /**
     * Create a new document object with \p parent.
     * \param parent parent object
     * \return new KTextEditor::Document object
     * \see documents()
     */
    virtual Document *createDocument ( QObject *parent ) = 0;

    /**
     * Get a list of all documents of this editor.
     * \return list of all existing documents
     * \see createDocument()
     */
    virtual const QList<Document*> &documents () = 0;

  /*
   * General Information about this editor.
   */
  public:
    /**
     * Get the about data of this Editor part.
     * \return about data
     */
    virtual const KAboutData &aboutData () const = 0;

    /**
     * Get the current default encoding for this Editor part.
     * \return default encoding
     * \since 4.5
     */
    const QString &defaultEncoding () const;

  protected:
    /**
     * Set the current default encoding for this Editor part.
     * Editor part implementation should call this internally on creation and config changes.
     * \param defaultEncoding new default encoding
     * \since 4.5
     */
    void setDefaultEncoding (const QString &defaultEncoding);

  /*
   * Configuration management.
   */
  public:
    /**
     * Read editor configuration from KConfig \p config.
     *
     * \note Implementation Notes: If \p config is NULL you should use
     *       kapp->config() as a fallback solution. Additionally the
     *       readConfig() call should be forwarded to every loaded plugin.
     * \param config config object
     * \see writeConfig()
     */
    virtual void readConfig (KConfig *config = 0) = 0;

    /**
     * Write editor configuration to KConfig \p config.
     *
     * \note Implementation Notes: If \p config is NULL you should use
     *       kapp->config() as a fallback solution. Additionally the
     *       writeConfig() call should be forwarded to every loaded plugin.
     * \param config config object
     * \see readConfig()
     */
    virtual void writeConfig (KConfig *config = 0) = 0;

    /**
     * Show the editor's config dialog, changes will be applied to the
     * editor, but not saved anywhere automagically, call \p writeConfig()
     * to save them.
     *
     * \note Instead of using the config dialog, the config pages can be
     *       embedded into your own config dialog by using configPages() and
     *       configPage() inherited by ConfigPageInterface.
     * \param parent parent widget
     */
    virtual void configDialog (QWidget *parent) = 0;

  Q_SIGNALS:
    /**
     * The \p editor emits this signal whenever a \p document was successfully
     * created.
     * \param editor editor which created the new document
     * \param document the newly created document instance
     * \see createDocument()
     */
    void documentCreated (KTextEditor::Editor *editor,
                          KTextEditor::Document *document);

  private:
    class EditorPrivate* const d;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
