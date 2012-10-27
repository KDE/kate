/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __SNIPPETPLUGIN_H__
#define __SNIPPETPLUGIN_H__

#include <interfaces/iplugin.h>
#include <interfaces/contextmenuextension.h>
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
class SnippetPlugin : public KDevelop::IPlugin
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
    virtual KDevelop::ContextMenuExtension contextMenuExtension(KDevelop::Context* context);

    // KDevelop::IPlugin methods
    virtual void unload();

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

