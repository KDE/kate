/* This file is part of the KDE libraries
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_GLOBAL_H__
#define __KATE_GLOBAL_H__

#include "katescript.h"

#include <ktexteditor/editor.h>

#include <kservice.h>
#include <kcomponentdata.h>
#include <kaboutdata.h>
#include <ktexteditor/commandinterface.h>
#include <ktexteditor/containerinterface.h>
#include <QtCore/QList>

/**
 * katepart version must be a string in double quotes, format: "x.x"
 * it is used for the version in the aboutdata + hl stuff
 */
#define KATEPART_VERSION "3.0"

class KateCmd;
class KateModeManager;
class KateSchemaManager;
class KateDocumentConfig;
class KateViewConfig;
class KateRendererConfig;
class KateDocument;
class KateRenderer;
class KateView;
class KateScriptManager;
class KDirWatch;
class KateHlManager;
class KateCmd;
class KatePartPluginManager;
class KateViGlobal;

namespace Kate {
  class Command;
}

/**
 * KateGlobal
 * One instance of this class is hold alive during
 * a kate part session, as long as any factory, document
 * or view stay around, here is the place to put things
 * which are needed and shared by all this objects ;)
 */
class KateGlobal : public KTextEditor::Editor, public KTextEditor::CommandInterface, public KTextEditor::ContainerInterface
{
  Q_OBJECT
  Q_INTERFACES(KTextEditor::CommandInterface)
  Q_INTERFACES(KTextEditor::ContainerInterface)

  private:
    /**
     * Default constructor, private, as singleton
     */
    KateGlobal ();

  public:
    /**
     * Destructor
     */
    ~KateGlobal ();

    /**
     * Create a new document object
     * @param parent parent object
     * @return created KTextEditor::Document
     */
    KTextEditor::Document *createDocument ( QObject *parent );

    /**
     * Returns a list of all documents of this editor.
     * @return list of all existing documents
     */
    const QList<KTextEditor::Document*> &documents ();

  /**
   * General Information about this editor
   */
  public:
    /**
     * return the about data
     * @return about data of this editor part
     */
    const KAboutData* aboutData() const { return &m_aboutData; }

   /**
   * Configuration management
   */
  public:
    /**
     * Read editor configuration from given config object
     * @param config config object
     */
    void readConfig (KConfig *config = 0);

    /**
     * Write editor configuration to given config object
     * @param config config object
     */
    void writeConfig (KConfig *config = 0);

    /**
     * Does this editor support a config dialog
     * @return does this editor have a config dialog?
     */
    bool configDialogSupported () const;

    /**
     * Shows a config dialog for the part, changes will be applied
     * to the editor, but not saved anywhere automagically, call
     * writeConfig to save them
    */
    void configDialog (QWidget *parent);

    /**
     * Number of available config pages
     * If the editor returns a number < 1, it doesn't support this
     * and the embedding app should use the configDialog () instead
     * @return number of config pages
     */
    int configPages () const;

    /**
     * returns config page with the given number,
     * config pages from 0 to configPages()-1 are available
     * if configPages() > 0
     */
    KTextEditor::ConfigPage *configPage (int number, QWidget *parent);

    QString configPageName (int number) const;

    QString configPageFullName (int number) const;

    KIcon configPageIcon (int number) const;

  /**
   * Kate Part Internal stuff ;)
   */
  public:
    /**
     * singleton accessor
     * @return instance of the factory
     */
    static KateGlobal *self ();

    /**
     * increment reference counter
     */
    static void incRef () { ++s_ref; }

    /**
     * decrement reference counter
     */
    static void decRef () { if (s_ref > 0) --s_ref; if (s_ref == 0) { delete s_self; s_self = 0L; } }

    /**
     * public accessor to the instance
     * @return instance
     */
    const KComponentData &componentData() { return m_componentData; }

    /**
     * register document at the factory
     * this allows us to loop over all docs for example on config changes
     * @param doc document to register
     */
    void registerDocument ( KateDocument *doc );

    /**
     * unregister document at the factory
     * @param doc document to register
     */
    void deregisterDocument ( KateDocument *doc );

    /**
     * register view at the factory
     * this allows us to loop over all views for example on config changes
     * @param view view to register
     */
    void registerView ( KateView *view );

    /**
     * unregister view at the factory
     * @param view view to unregister
     */
    void deregisterView ( KateView *view );

    /**
     * return a list of all registered docs
     * @return all known documents
     */
    QList<KateDocument*> &kateDocuments () { return m_documents; }

    /**
     * return a list of all registered views
     * @return all known views
     */
    QList<KateView*> &views () { return m_views; }

    /**
     * global plugin manager
     * @return kate part plugin manager
     */
    KatePartPluginManager *pluginManager () { return m_pluginManager; }

    /**
     * global dirwatch
     * @return dirwatch instance
     */
    KDirWatch *dirWatch () { return m_dirWatch; }

    /**
     * global mode manager
     * used to manage the modes centrally
     * @return mode manager
     */
    KateModeManager *modeManager () { return m_modeManager; }

    /**
     * manager for the katepart schemas
     * @return schema manager
     */
    KateSchemaManager *schemaManager () { return m_schemaManager; }

    /**
     * fallback document config
     * @return default config for all documents
     */
    KateDocumentConfig *documentConfig () { return m_documentConfig; }

    /**
     * fallback view config
     * @return default config for all views
     */
    KateViewConfig *viewConfig () { return m_viewConfig; }

    /**
     * fallback renderer config
     * @return default config for all renderers
     */
    KateRendererConfig *rendererConfig () { return m_rendererConfig; }

    /**
     * Global script collection
     */
    KateScriptManager *scriptManager () { return m_scriptManager; }

    /**
     * hl manager
     * @return hl manager
     */
    KateHlManager *hlManager () { return m_hlManager; }

    /**
     * command manager
     * @return command manager
     */
    KateCmd *cmdManager () { return m_cmdManager; }

    /**
     * vi input mode global
     * @return vi input mode global
     */
    KateViGlobal *viInputModeGlobal () { return m_viInputModeGlobal; }

    /**
     * register given command
     * this works global, for all documents
     * @param cmd command to register
     * @return success
     */
    bool registerCommand (KTextEditor::Command *cmd);

    /**
     * unregister given command
     * this works global, for all documents
     * @param cmd command to unregister
     * @return success
     */
    bool unregisterCommand (KTextEditor::Command *cmd);

    /**
     * query for command
     * @param cmd name of command to query for
     * @return found command or 0
     */
    KTextEditor::Command *queryCommand (const QString &cmd) const;

    /**
     * Get a list of all registered commands.
     * \return list of all commands
     */
    QList<KTextEditor::Command*> commands() const;

    /**
     * Get a list of available commandline strings.
     * \return commandline strings
     */
    QStringList commandList() const;


    /**
     * Get the currently associated Container object
     * \return container object
     */
    QObject * container();

    /**
     * Set the associated container object
     */
    void setContainer( QObject * container );

  private:
    /**
     * instance of this factory
     */
    static KateGlobal *s_self;

    /**
     * reference counter
     */
    static int s_ref;

    /**
     * about data (authors and more)
     */
    KAboutData m_aboutData;

    /**
     * our kinstance
     */
    KComponentData m_componentData;

    /**
     * registered docs
     */
    QList<KateDocument*> m_documents;

    /**
     * registered views
     */
    QList<KateView*> m_views;

    /**
     * global dirwatch object
     */
    KDirWatch *m_dirWatch;

    /**
     * mode manager
     */
    KateModeManager *m_modeManager;

    /**
     * schema manager
     */
    KateSchemaManager *m_schemaManager;

    /**
     * at start found plugins
     */
    KatePartPluginManager *m_pluginManager;

    /**
     * fallback document config
     */
    KateDocumentConfig *m_documentConfig;

    /**
     * fallback view config
     */
    KateViewConfig *m_viewConfig;

    /**
     * fallback renderer config
     */
    KateRendererConfig *m_rendererConfig;

    /**
     * internal commands
     */
    QList<KTextEditor::Command *> m_cmds;

    /**
     * script manager
     */
    KateScriptManager *m_scriptManager;

    /**
     * hl manager
     */
    KateHlManager *m_hlManager;

    /**
     * command manager
     */
    KateCmd *m_cmdManager;

    /**
     * vi input mode global
     */
    KateViGlobal *m_viInputModeGlobal;

    QList<KTextEditor::Document*> m_docs;

    /**
     * container interface
     */
    QPointer<QObject> m_container;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
