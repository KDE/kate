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

#include <QVariant>
#include <QPointer>

#include <KTextEditor/View>

class SnippetCompletionModel;
class Snippet;

/**
 * This is the main class of KDevelop's snippet plugin.
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 */
class KateSnippetGlobal : public QObject
{
    Q_OBJECT

public:
    KateSnippetGlobal(QObject *parent, const QVariantList &args = QVariantList() );
    ~KateSnippetGlobal () override;

    /**
     * Inserts the given @p snippet into the currently active view.
     * If the current active view is not inherited from KTextEditor::View
     * nothing will happen.
     */
    void insertSnippet(Snippet* snippet);

    static KateSnippetGlobal* self() { return s_self; }

    /**
     * Code completion model.
     * @return code completion model for snippets
     */
    SnippetCompletionModel *completionModel () { return m_model.data(); }

public Q_SLOTS:
    /**
     * Create snippet for given view, e.g. by using the selection
     * @param view view to create snippet for
     */
    void createSnippet (KTextEditor::View *view);

    void insertSnippetFromActionData();

private:
    static KateSnippetGlobal *s_self;
    QScopedPointer<SnippetCompletionModel> m_model;
    QPointer<KTextEditor::View> m_activeViewForDialog;
};

#endif

