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
#include <kglobalsettings.h>
#include <kdebug.h>

// $Id$

KateDocumentConfig *KateDocumentConfig::s_global = 0;
KateViewConfig *KateViewConfig::s_global = 0;
KateRendererConfig *KateRendererConfig::s_global = 0;

KateDocumentConfig::KateDocumentConfig ()
 : m_tabWidthSet (true),
   m_doc (0)
{
  s_global = this;

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

KateDocumentConfig *KateDocumentConfig::global ()
{
  if (!s_global)
    s_global = new KateDocumentConfig ();

  return s_global;
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
  s_global = this;

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

KateViewConfig *KateViewConfig::global ()
{
  if (!s_global)
    s_global = new KateViewConfig ();

  return s_global;
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
   m_viewFont (new FontStruct ()),
   m_printFont (new FontStruct ()),
   m_viewFontSet (true),
   m_printFontSet (true),
   m_renderer (0)
{
  s_global = this;

  kdDebug () << "STATIC OBJECT THERE" << endl;

  // init with defaults from config or really hardcoded ones
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate Renderer Defaults");
  readConfig (config);
}

KateRendererConfig::KateRendererConfig (KateRenderer *renderer)
 : m_viewFont (0),
   m_printFont (0),
   m_viewFontSet (false),
   m_printFontSet (false),
   m_renderer (renderer)
{
  kdDebug () << "DNY OBJECT THERE" << endl;
}

KateRendererConfig::~KateRendererConfig ()
{
}

KateRendererConfig *KateRendererConfig::global ()
{
  if (!s_global)
    s_global = new KateRendererConfig ();

  return s_global;
};

void KateRendererConfig::readConfig (KConfig *config)
{
  QFont f (KGlobalSettings::fixedFont());

  setFont(KateRendererConfig::ViewFont, config->readFontEntry("View Font", &f));
  setFont(KateRendererConfig::PrintFont, config->readFontEntry("Printer Font", &f));

}

void KateRendererConfig::writeConfig (KConfig *config)
{
  config->writeEntry("View Font", *font(KateRendererConfig::ViewFont));
  config->writeEntry("Printer Font", *font(KateRendererConfig::PrintFont));

  config->sync ();
}

void KateRendererConfig::updateRenderer ()
{
  if (m_renderer)
  {
    m_renderer->updateConfig ();
    return;
  }

  if (isGlobal())
  {
    for (uint z=0; z < KateFactory::views()->count(); z++)
    {
      KateFactory::views()->at(z)->renderer()->updateConfig ();
    }
  }
}

void KateRendererConfig::setFont(int whichFont, QFont font)
{
  if (whichFont == ViewFont) {
    if (!m_viewFontSet)
    {
      m_viewFontSet = true;
      m_viewFont = new FontStruct ();
    }

     m_viewFont->setFont(font);

  } else {
    if (!m_printFontSet)
    {
      m_printFontSet = true;
      m_printFont = new FontStruct ();
    }

    m_printFont->setFont(font);
  }

  updateRenderer ();
}

const FontStruct *KateRendererConfig::fontStruct (int whichFont)
{
  if (whichFont == ViewFont)
  {
    if (m_viewFontSet || isGlobal())
      return m_viewFont;

    return s_global->fontStruct (whichFont);
  }
  else
  {
    if (m_printFontSet || isGlobal())
      return m_printFont;

    return s_global->fontStruct (whichFont);
  }
}

const QFont *KateRendererConfig::font(int whichFont)
{
  return &(fontStruct (whichFont)->myFont);
}

const QFontMetrics *KateRendererConfig::fontMetrics(int whichFont)
{
  return &(fontStruct (whichFont)->myFontMetrics);
}
