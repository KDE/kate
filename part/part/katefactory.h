/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

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

class KateCmd;
class KateFileTypeManager;
class KateSchemaManager;
class KateDocumentConfig;
class KateViewConfig;
class KateRendererConfig;

class KDirWatch;

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
     * @param doc document to register
     */
    void registerDocument ( class KateDocument *doc );
    void deregisterDocument ( class KateDocument *doc );

    void registerView ( class KateView *view );
    void deregisterView ( class KateView *view );

    void registerRenderer ( class KateRenderer  *renderer );
    void deregisterRenderer ( class KateRenderer  *renderer );

    inline QPtrList<class KateDocument> *documents () { return &m_documents; };

    inline QPtrList<class KateView> *views () { return &m_views; };

    inline QPtrList<class KateRenderer> *renderers () { return &m_renderers; };
    
    inline const KTrader::OfferList &plugins () { return m_plugins; };

    inline KDirWatch *dirWatch () { return m_dirWatch; };

    inline KateFileTypeManager *fileTypeManager () { return m_fileTypeManager; };

    inline KateSchemaManager *schemaManager () { return m_schemaManager; };

    inline KateDocumentConfig *documentConfig () { return m_documentConfig; }
    inline KateViewConfig *viewConfig () { return m_viewConfig; }
    inline KateRendererConfig *rendererConfig () { return m_rendererConfig; }

  private:
    static KateFactory *s_self;
    
    KAboutData m_aboutData;
    KInstance m_instance;
    
    QPtrList<class KateDocument> m_documents;
    QPtrList<class KateView> m_views;
    QPtrList<class KateRenderer> m_renderers;
    
    KDirWatch *m_dirWatch;  
  
    KateFileTypeManager *m_fileTypeManager;
    KateSchemaManager *m_schemaManager;

    KTrader::OfferList m_plugins;

    KateDocumentConfig *m_documentConfig;
    KateViewConfig *m_viewConfig;
    KateRendererConfig *m_rendererConfig;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
