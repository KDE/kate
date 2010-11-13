/* This file is part of the KDE libraries
   Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

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

#include "hlselectionplugin.h"
#include "hlselectionplugin.moc"

#include <ktexteditor/document.h>
#include <ktexteditor/attribute.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>

#include <assert.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kfiledialog.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klocale.h>
#include <kaboutdata.h>


K_PLUGIN_FACTORY( HighlightSelectionPluginFactory, registerPlugin<HighlightSelectionPlugin>(); )
K_EXPORT_PLUGIN( HighlightSelectionPluginFactory( KAboutData( "ktexteditor_insertfile", "ktexteditor_plugins", ki18n("Highlight Selection"), "1.0", ki18n("Highlight Selection"), KAboutData::License_LGPL_V2 ) ) )

//BEGIN HighlightSelectionPlugin
HighlightSelectionPlugin::HighlightSelectionPlugin( QObject *parent, const QVariantList& )
  : KTextEditor::Plugin ( parent )
{
}

HighlightSelectionPlugin::~HighlightSelectionPlugin()
{
}

void HighlightSelectionPlugin::addView(KTextEditor::View *view)
{
  HighlightSelectionPluginView *nview = new HighlightSelectionPluginView (view);
  m_views.append (nview);
}

void HighlightSelectionPlugin::removeView(KTextEditor::View *view)
{
  foreach (HighlightSelectionPluginView *pluginView, m_views) {
    if (pluginView->view() == view) {
      m_views.removeAll(pluginView);
      delete pluginView;
      break;
    }
  }
}
//END HighlightSelectionPlugin

//BEGIN HighlightSelectionPluginView
HighlightSelectionPluginView::HighlightSelectionPluginView( KTextEditor::View *view)
  : QObject( view )
//   , KXMLGUIClient( view ) // XMLGUI stuff not needed right now
{
  setObjectName("highlight-selection-plugin");

  m_view = view;

  // we don't need any XMLGUI stuff, so comment out
//  setComponentData( HighlightSelectionPluginFactory::componentData() );
//  setXMLFile( "ktexteditor_hlselectionui.rc" );

  connect(view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(selectionChanged()));
  connect(view->document(), SIGNAL(aboutToReload(KTextEditor::Document*)), this, SLOT(clearHighlights()));
}

HighlightSelectionPluginView::~HighlightSelectionPluginView()
{
  clearHighlights();
}

KTextEditor::View* HighlightSelectionPluginView::view() const
{
  return m_view;
}

void HighlightSelectionPluginView::clearHighlights()
{
  qDeleteAll(m_ranges);
  m_ranges.clear();
  m_currentText.clear();
}

void HighlightSelectionPluginView::selectionChanged()
{
  QString text;
  // if text of selection is still the same, abort
  if (m_view->selection() && m_view->selectionRange().onSingleLine()) {
    text = m_view->selectionText();
    if (text == m_currentText) {
      return;
    }
  }

  // text changed: remove all highlights + create new ones
  // (do not call clearHighlights(), since this also resets the m_currentText
  qDeleteAll(m_ranges);
  m_ranges.clear();
  
  // do not highlight strings with leading and trailing spaces
  if (!text.isEmpty() && (text.at(0).isSpace() || text.at(text.length()-1).isSpace())) {
    return; 
  }

  m_currentText = text;
  if (!m_currentText.isEmpty()) {
    createHighlights();
  }
}

void HighlightSelectionPluginView::createHighlights()
{
  m_currentText = m_view->selectionText();

  KTextEditor::SearchInterface* siface =
    qobject_cast<KTextEditor::SearchInterface*>(m_view->document());

  if (!siface) {
    return;
  }

  KTextEditor::MovingInterface* miface =
    qobject_cast<KTextEditor::MovingInterface*>(m_view->document());

  KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());
  attr->setFontBold(true);
  attr->setBackground(Qt::yellow);

  KTextEditor::Cursor start(0, 0);
  KTextEditor::Range searchRange;

  QVector<KTextEditor::Range> matches;

  do {
    searchRange.setRange(start, m_view->document()->documentEnd());

    matches = siface->searchText(searchRange, m_currentText, KTextEditor::Search::WholeWords);

    if (matches.first().isValid()) {
      KTextEditor::MovingRange* mr = miface->newMovingRange(matches.first());
      mr->setAttribute(attr);
      mr->setView(m_view);
      mr->setAttributeOnlyForViews(true);
      m_ranges.append(mr);
      start = matches.first().end();
    }
  } while (matches.first().isValid());
}
//END HighlightSelectionPluginView

// kate: space-indent on; indent-width 2; replace-tabs on;
