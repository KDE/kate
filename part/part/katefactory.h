/* This file is part of the KDE libraries
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KATE_FACTORY_H__
#define __KATE_FACTORY_H__

#include <kparts/factory.h>

#include <ktrader.h>
#include <kinstance.h>
#include <kaboutdata.h>

// katepart version must be a string in double quotes, format: "x.x"
#define KATEPART_VERSION "2.4"

class KateCmd;
class KateFileTypeManager;
class KateSchemaManager;
class KateDocumentConfig;
class KateViewConfig;
class KateRendererConfig;
class KateDocument;
class KateRenderer;
class KateView;
class KateJScript;

class KDirWatch;
class KVMAllocator;

namespace Kate {
  class Command;
}

class KateFactory
{
  private:
    /**
     * Default constructor, private, as singleton
     */
    KateFactory ();

  public:
    /**
     * Destructor
     */
    ~KateFactory ();

    /**
     * singleton accessor
     * @return instance of the factory
     */
    static KateFactory *self ();

    /**
     * reimplemented create object method
     * @param parentWidget parent widget
     * @param widgetName widget name
     * @param parent QObject parent
     * @param name object name
     * @param args additional arguments
     * @return constructed part object
     */
    KParts::Part *createPartObject ( QWidget *parentWidget, const char *widgetName,
                                     QObject *parent, const char *name, const char *classname,
                                     const QStringList &args );

    /**
     * public accessor to the instance
     * @return instance
     */
    inline KInstance *instance () { return &m_instance; };

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
     * register renderer at the factory
     * this allows us to loop over all views for example on config changes
     * @param renderer renderer to register
     */
    void registerRenderer ( KateRenderer  *renderer );

    /**
     * unregister renderer at the factory
     * @param renderer renderer to unregister
     */
    void deregisterRenderer ( KateRenderer  *renderer );

    /**
     * return a list of all registered docs
     * @return all known documents
     */
    inline QPtrList<KateDocument> *documents () { return &m_documents; };

    /**
     * return a list of all registered views
     * @return all known views
     */
    inline QPtrList<KateView> *views () { return &m_views; };

    /**
     * return a list of all registered renderers
     * @return all known renderers
     */
    inline QPtrList<KateRenderer> *renderers () { return &m_renderers; };

    /**
     * on start detected plugins
     * @return list of all at launch detected ktexteditor::plugins
     */
    inline const KTrader::OfferList &plugins () { return m_plugins; };

    /**
     * global dirwatch
     * @return dirwatch instance
     */
    inline KDirWatch *dirWatch () { return m_dirWatch; };

    /**
     * global filetype manager
     * used to manage the file types centrally
     * @return filetype manager
     */
    inline KateFileTypeManager *fileTypeManager () { return m_fileTypeManager; };

    /**
     * manager for the katepart schemas
     * @return schema manager
     */
    inline KateSchemaManager *schemaManager () { return m_schemaManager; };

    /**
     * fallback document config
     * @return default config for all documents
     */
    inline KateDocumentConfig *documentConfig () { return m_documentConfig; }

    /**
     * fallback view config
     * @return default config for all views
     */
    inline KateViewConfig *viewConfig () { return m_viewConfig; }

    /**
     * fallback renderer config
     * @return default config for all renderers
     */
    inline KateRendererConfig *rendererConfig () { return m_rendererConfig; }

    /**
     * Global allocator for swapping
     * @return allocator
     */
    inline KVMAllocator *vm () { return m_vm; }

    /**
     * global interpreter, for nice js stuff
     */
    KateJScript *jscript ();

  private:
    /**
     * instance of this factory
     */
    static KateFactory *s_self;

    /**
     * about data (authors and more)
     */
    KAboutData m_aboutData;

    /**
     * our kinstance
     */
    KInstance m_instance;

    /**
     * registered docs
     */
    QPtrList<KateDocument> m_documents;

    /**
     * registered views
     */
    QPtrList<KateView> m_views;

    /**
     * registered renderers
     */
    QPtrList<KateRenderer> m_renderers;

    /**
     * global dirwatch object
     */
    KDirWatch *m_dirWatch;

    /**
     * filetype manager
     */
    KateFileTypeManager *m_fileTypeManager;

    /**
     * schema manager
     */
    KateSchemaManager *m_schemaManager;

    /**
     * at start found plugins
     */
    KTrader::OfferList m_plugins;

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
     * vm allocator
     */
    KVMAllocator *m_vm;

    /**
     * internal commands
     */
    QValueList<Kate::Command *> m_cmds;

    /**
     * js interpreter
     */
    KateJScript *m_jscript;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
