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

#include "kateconfig.h"

#include "katerenderer.h"
#include "kateview.h"
#include "katedocument.h"
#include "katefactory.h"

#include <kconfig.h>

// $Id$

KateDocumentConfig *KateDocumentConfig::s_global = new KateDocumentConfig ();
KateViewConfig *KateViewConfig::s_global = new KateViewConfig ();
KateRendererConfig *KateRendererConfig::s_global = new KateRendererConfig ();

KateDocumentConfig::KateDocumentConfig ()
 : m_tabWidthSet (true),
   m_doc (0)
{
  // init with defaults from config or really hardcoded ones
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate Document Defaults");
  readConfig (config);
}

KateDocumentConfig::KateDocumentConfig (KateDocument *doc)
 : m_tabWidthSet (false),
   m_doc (doc)
{
}

KateDocumentConfig::~KateDocumentConfig ()
{
}

void KateDocumentConfig::readConfig (KConfig *config)
{
  setTabWidth (config->readNumEntry("Tab Width", 8));
}

void KateDocumentConfig::writeConfig (KConfig *config)
{
  config->writeEntry("Tab Width", tabWidth());

  config->sync ();
}

void KateDocumentConfig::updateDocument ()
{
  if (m_doc)
  {
    m_doc->updateConfig ();
    return;
  }

  if (isGlobal())
  {
    for (uint z=0; z < KateFactory::documents()->count(); z++)
    {
      KateFactory::documents()->at(z)->updateConfig ();
    }
  }
}

int KateDocumentConfig::tabWidth ()
{
  if (m_tabWidthSet || isGlobal())
    return m_tabWidth;

  return s_global->tabWidth();
};

void KateDocumentConfig::setTabWidth (int tabWidth)
{
  if (tabWidth < 1)
    return;

  m_tabWidthSet = true;
  m_tabWidth = tabWidth;

  updateDocument ();
}

KateViewConfig::KateViewConfig ()
 :
   m_view (0)
{
  // init with defaults from config or really hardcoded ones
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate View Defaults");
  readConfig (config);
}

KateViewConfig::KateViewConfig (KateView *view)
 :
   m_view (view)
{
}

KateViewConfig::~KateViewConfig ()
{
}

void KateViewConfig::readConfig (KConfig *config)
{
}

void KateViewConfig::writeConfig (KConfig *config)
{
  config->sync ();
}

void KateViewConfig::updateView ()
{
  if (m_view)
  {
    //m_doc->updateConfig ();
    return;
  }

  if (isGlobal())
  {
    for (uint z=0; z < KateFactory::views()->count(); z++)
    {
      //KateFactory::documents()->at(z)->updateConfig ();
    }
  }
}

KateRendererConfig::KateRendererConfig ()
 :
   m_renderer (0)
{
  // init with defaults from config or really hardcoded ones
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate Renderer Defaults");
  readConfig (config);
}

KateRendererConfig::KateRendererConfig (KateRenderer *renderer)
 :
   m_renderer (renderer)
{
}

KateRendererConfig::~KateRendererConfig ()
{
}

void KateRendererConfig::readConfig (KConfig *config)
{
}

void KateRendererConfig::writeConfig (KConfig *config)
{
  config->sync ();
}

void KateRendererConfig::updateRenderer ()
{
  if (m_renderer)
  {
    //m_doc->updateConfig ();
    return;
  }

  if (isGlobal())
  {
    for (uint z=0; z < KateFactory::views()->count(); z++)
    {
      //KateFactory::documents()->at(z)->updateConfig ();
    }
  }
}
