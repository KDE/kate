/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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

#ifndef __SNIPPETPLUGIN_H__
#define __SNIPPETPLUGIN_H__

#include <QtCore/QVariant>

class SnippetCompletionModel;

namespace KTextEditor
{
class Document;
class View;
}

namespace KParts
{
class Part;
}

class Snippet;
class KAction;
class QMenu;

/**
 * This is the main class of KDevelop's snippet plugin.
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 */
class SnippetPlugin : public QObject
{
    Q_OBJECT

public:
    SnippetPlugin(QObject *parent, const QVariantList &args = QVariantList() );
    virtual ~SnippetPlugin();

    /**
     * Inserts the given @p snippet into the currently active view.
     * If the current active view is not inherited from KTextEditor::View
     * nothing will happen.
     */
    void insertSnippet(Snippet* snippet);
    
#if 0
  virtual KDevelop::ContextMenuExtension contextMenuExtension(KDevelop::Context* context);

    // KDevelop::IPlugin methods
    virtual void unload();
#endif

    static SnippetPlugin* self();;

private slots:
    void viewCreated( KTextEditor::Document*, KTextEditor::View* view );
    void documentLoaded(KParts::Part*);
public slots:
    void createSnippetFromSelection();
    void insertSnippetFromActionData();

private:
    class SnippetViewFactory *m_factory;
    class SnippetCompletionModel* m_model;
    static SnippetPlugin* m_self;
};

#endif

