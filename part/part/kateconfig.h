/* This file is part of the KDE libraries
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_CONFIG_H__
#define __KATE_CONFIG_H__

#include <qcolor.h>

class KateView;
class KateDocument;
class KateRenderer;

class KConfig;

class KateDocumentConfig
{
  public:
    /**
     * only used in KateFactory for the static global fallback !!!
     */
    KateDocumentConfig ();

    /**
     * Construct a DocumentConfig
     */
    KateDocumentConfig (KateDocument *doc);

    /**
     * Cu DocumentConfig
     */
    ~KateDocumentConfig ();

    inline static KateDocumentConfig *global () { return s_global; };

    inline bool isGlobal () { return (this == s_global); };

  public:
    /**
     * Read config from object
     */
    void readConfig (KConfig *config);

    /**
     * Write config to object
     */
    void writeConfig (KConfig *config);

    /**
     * update the related document
     */
    void updateDocument ();

  public:
    int tabWidth ();
    void setTabWidth (int tabWidth);

  private:
    int m_tabWidth;

    bool m_tabWidthSet : 1;

  private:
    KateDocument *m_doc;

    static KateDocumentConfig *s_global;
};

class KateViewConfig
{
  public:
    /**
     * only used in KateFactory for the static global fallback !!!
     */
    KateViewConfig ();

    /**
     * Construct a DocumentConfig
     */
    KateViewConfig (KateView *view);

    /**
     * Cu DocumentConfig
     */
    ~KateViewConfig ();

    inline static KateViewConfig *global () { return s_global; };

    inline bool isGlobal () { return (this == s_global); };

  public:
    /**
     * Read config from object
     */
    void readConfig (KConfig *config);

    /**
     * Write config to object
     */
    void writeConfig (KConfig *config);

    /**
     * update the related document
     */
    void updateView ();

  public:

  private:

  private:
    KateView *m_view;

    static KateViewConfig *s_global;
};

class KateRendererConfig
{
  public:
    /**
     * only used in KateFactory for the static global fallback !!!
     */
    KateRendererConfig ();

    /**
     * Construct a DocumentConfig
     */
    KateRendererConfig (KateRenderer *renderer);

    /**
     * Cu DocumentConfig
     */
    ~KateRendererConfig ();

    inline static KateRendererConfig *global () { return s_global; };

    inline bool isGlobal () { return (this == s_global); };

  public:
    /**
     * Read config from object
     */
    void readConfig (KConfig *config);

    /**
     * Write config to object
     */
    void writeConfig (KConfig *config);

    /**
     * update the related document
     */
    void updateRenderer ();

  public:

  private:

  private:
    KateRenderer *m_renderer;

    static KateRendererConfig *s_global;
};

#endif
