/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>
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
 *  Boston, MA 02110-1301, USA.
 */

#ifndef __KATE_GLOBAL_H__
#define __KATE_GLOBAL_H__

#include <ktexteditor/ktexteditor_export.h>
#include "katescript.h"

#include <ktexteditor/editor.h>

#include <KService>
#include <KAboutData>
#include <ktexteditor/application.h>
#include <ktexteditor/commandinterface.h>
#include <ktexteditor/templateinterface2.h>

#include <QList>
#include <QPointer>

class KateCmd;
class KateModeManager;
class KateSchemaManager;
class KateGlobalConfig;
class KateDocumentConfig;
class KateViewConfig;
class KateRendererConfig;
class KateDocument;
class KateView;
class KateScriptManager;
class KDirWatch;
class KateHlManager;
class KatePartPluginManager;
class KateSpellCheckManager;
class KateViGlobal;
class KateWordCompletionModel;

Q_DECLARE_METATYPE(KSharedConfig::Ptr)

/**
 * KateGlobal
 * One instance of this class is hold alive during
 * a kate part session, as long as any factory, document
 * or view stay around, here is the place to put things
 * which are needed and shared by all this objects ;)
 */
class KTEXTEDITOR_EXPORT KateGlobal : public KTextEditor::Editor, public KTextEditor::CommandInterface, public KTextEditor::TemplateScriptRegistrar
{
  Q_OBJECT
  Q_INTERFACES(KTextEditor::CommandInterface)
  Q_INTERFACES(KTextEditor::TemplateScriptRegistrar)

  public:
    /**
     * property to tell the editor to use a given session config for session related
     * configuration instead of KSharedConfig::openConfig().
     * MUST be set directly after first creation of the editor as otherwise
     * some parts might not pick this up.
     */
    Q_PROPERTY (KSharedConfig::Ptr sessionConfig READ sessionConfig WRITE setSessionConfig)

    /**
     * Get session config, defaults to KSharedConfig::openConfig()
     * @return session config
     */
    KSharedConfig::Ptr sessionConfig ()
    {
      return m_sessionConfig;
    }

    /**
     * Set session config
     * @param sessionConfig new session config
     */
    void setSessionConfig (KSharedConfig::Ptr sessionConfig)
    {
      m_sessionConfig = sessionConfig;
    }

  // unit testing
  public:
    /**
     * Calling this function internally sets a flag such that unitTestMode()
     * returns \p true.
     */
    static void enableUnitTestMode();
    /**
     * Returns \p true, if the unit test mode was enabled through a call of
     * enableUnitTestMode(), otherwise \p false.
     */
    static bool unitTestMode();

  // for setDefaultEncoding
  friend class KateDocumentConfig;

  private:
    /**
     * Default constructor, private, as singleton
     * @param staticInstance pointer to fill with content of this
     */
    KateGlobal (QPointer<KateGlobal> &staticInstance);

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
     * Set the global application object.
     * This will allow the editor component to access
     * the hosting application.
     * @param application application object
     */
    void setApplication (KTextEditor::Application *application)
    {
      m_application = application;
    }
    
    /**
     * Current hosting application, if any set.
     * @return current application object or nullptr
     */
    KTextEditor::Application *application () const
    {
      return m_application;
    }

  /**
   * General Information about this editor
   */
  public:
    /**
     * return the about data
     * @return about data of this editor part
     */
    const KAboutData &aboutData() const { return m_aboutData; }

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

    QIcon configPageIcon (int number) const;

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
     * spell check manager
     * @return spell check manager
     */
    KateSpellCheckManager *spellCheckManager () { return m_spellCheckManager; }

    /**
     * global instance of the simple word completion mode
     * @return global instance of the simple word completion mode
     */
    KateWordCompletionModel *wordCompletionModel () { return m_wordCompletionModel; }

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
     *  TemplateScriptRegistrar interface
     */
    KTextEditor::TemplateScript* registerTemplateScript (QObject* owner, const QString& script);
    void unregisterTemplateScript(KTextEditor::TemplateScript* templateScript);

    /**
     * Copy text to clipboard an remember it in the history
     * @param text text to copy to clipboard, does nothing if empty!
     */
    void copyToClipboard (const QString &text);
    
    /**
     * Clipboard history, filled with text we ever copied
     * to clipboard via copyToClipboard.
     */
    const QStringList &clipboardHistory () const
    {
        return m_clipboardHistory;
    }
    
  Q_SIGNALS:
    /**
     * Emitted if the history of clipboard changes via copyToClipboard
     */
    void clipboardHistoryChanged ();

  protected:
    bool eventFilter(QObject *, QEvent *);

  private Q_SLOTS:
    void updateColorPalette();

  private:
    /**
     * about data (authors and more)
     */
    KAboutData m_aboutData;

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
     * global config
     */
    KateGlobalConfig *m_globalConfig;

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

    /**
     * spell check manager
     */
    KateSpellCheckManager *m_spellCheckManager;

    QList<KTextEditor::Document*> m_docs;

    /**
     * global instance of the simple word completion mode
     */
    KateWordCompletionModel *m_wordCompletionModel;

    /**
     * session config
     */
    KSharedConfig::Ptr m_sessionConfig;
    
    /**
     * clipboard history
     */
    QStringList m_clipboardHistory;
    
    /**
     * access to application
     */
    QPointer<KTextEditor::Application> m_application;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
