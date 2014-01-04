/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef __KATE_SNIPPETS_H__
#define __KATE_SNIPPETS_H__

#include <KTextEditor/ApplicationPlugin>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>

class KateSnippetsPluginView;

class KateSnippetsPlugin: public KTextEditor::ApplicationPlugin
{
    Q_OBJECT
    
  friend class KateSnippetsPluginView;
  
  public:
    explicit KateSnippetsPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateSnippetsPlugin();

    QObject *createView (KTextEditor::MainWindow *mainWindow);
  
  private:
    QList<KateSnippetsPluginView*> mViews;
};

class KateSnippetsPluginView : public QObject
{
    Q_OBJECT

  public:
    /**
      * Constructor.
      */
    KateSnippetsPluginView (KateSnippetsPlugin* plugin, KTextEditor::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateSnippetsPluginView ();

    void readConfig();

  private:
    KateSnippetsPlugin *m_plugin;
    QWidget *m_toolView;
    QWidget *m_snippets;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
