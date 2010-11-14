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

#ifndef _HIGHLIGHT_SELECTION_PLUGIN_H_
#define _HIGHLIGHT_SELECTION_PLUGIN_H_

#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>

// #include <kxmlguiclient.h>
#include <QtCore/QObject>
#include <kjob.h>
#include <kurl.h>
#include <kio/job.h>

namespace KTextEditor {
  class MovingRange;
}

class HighlightSelectionPlugin : public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    explicit HighlightSelectionPlugin( QObject *parent = 0,
                      const QVariantList &args = QVariantList() );
    virtual ~HighlightSelectionPlugin();

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);


  private:
    QList<class HighlightSelectionPluginView*> m_views;
};

class HighlightSelectionPluginView
  : public QObject
//   , public KXMLGUIClient
{
  Q_OBJECT
  public:
    explicit HighlightSelectionPluginView(KTextEditor::View *view);
    ~HighlightSelectionPluginView();

    void createHighlights();
    KTextEditor::View* view() const;

  public Q_SLOTS:
    void selectionChanged();
    void clearHighlights();

  private:
    KTextEditor::View* m_view;
    QString m_currentText;

    QList<KTextEditor::MovingRange*> m_ranges;
};

#endif // _HIGHLIGHT_SELECTION_PLUGIN_H_

// kate: space-indent on; indent-width 2; replace-tabs on;
