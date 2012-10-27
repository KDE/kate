/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *   Copyright 2010 Milian Wolff <mail@milianw.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __SNIPPETSTORE_H__
#define __SNIPPETSTORE_H__

#include <QStandardItemModel>
#include <KConfigGroup>

class SnippetRepository;
class SnippetPlugin;

namespace KTextEditor {
class TemplateScriptRegistrar;
class TemplateScript;
}

/**
 * This class is implemented as singelton.
 * It represents the model containing all snippet repositories with their snippets.
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 * @author Milian Wolff <mail@milianw.de>
 */
class SnippetStore : public QStandardItemModel
{
    Q_OBJECT

public:
    /**
     * Initialize the SnippetStore.
     */
    static void init(SnippetPlugin* plugin);
    /**
     * Retuns the SnippetStore. Call init() to set it up first.
     */
    static SnippetStore* self();

    virtual ~SnippetStore();
    KConfigGroup getConfig();
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    /**
     * Returns the repository for the given @p file if there is any.
     */
    SnippetRepository* repositoryForFile(const QString &file);

    /**
     * Register @p script to make it available in snippets.
     * 
     * @return token identifying the script
     * 
     * @since KDE 4.5
     */
    KTextEditor::TemplateScript* registerScript(const QString& script);

    /**
     * Unregister script identified by @p token.
     *
     * @since KDE 4.5
     */
    void unregisterScript(KTextEditor::TemplateScript* token);

private:
    SnippetStore(SnippetPlugin* plugin);

    virtual Qt::ItemFlags flags (const QModelIndex & index) const;

    static SnippetStore* m_self;
    SnippetPlugin* m_plugin;
    KTextEditor::TemplateScriptRegistrar* m_scriptregistrar;
};

#endif

