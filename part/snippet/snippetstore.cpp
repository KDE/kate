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

#include "snippetstore.h"

#include "snippetplugin.h"
#include "snippetrepository.h"

#include <interfaces/icore.h>
#include <interfaces/isession.h>

#include <KStandardDirs>
#include <KDebug>

#include <ktexteditor/editor.h>
#include <interfaces/ipartcontroller.h>
#include <ktexteditor/templateinterface2.h>

SnippetStore* SnippetStore::m_self = 0;

SnippetStore::SnippetStore(SnippetPlugin* plugin)
    : m_plugin(plugin), m_scriptregistrar(0)
{
    m_self = this;

    const QStringList list = KGlobal::dirs()->findAllResources("data",
        "ktexteditor_snippets/data/*.xml", KStandardDirs::NoDuplicates)
                        << KGlobal::dirs()->findAllResources("data",
        "ktexteditor_snippets/ghns/*.xml", KStandardDirs::NoDuplicates);

    foreach(const QString& file, list ) {
        SnippetRepository* repo = new SnippetRepository(file);
        appendRow(repo);
    }

    m_scriptregistrar = qobject_cast<KTextEditor::TemplateScriptRegistrar*>(KDevelop::ICore::self()->partController()->editorPart());
}

SnippetStore::~SnippetStore()
{
    invisibleRootItem()->removeRows( 0, invisibleRootItem()->rowCount() );
    m_self = 0;
}

void SnippetStore::init(SnippetPlugin* plugin)
{
    Q_ASSERT(!SnippetStore::self());
    new SnippetStore(plugin);
}

SnippetStore* SnippetStore::self()
{
    return m_self;
}

Qt::ItemFlags SnippetStore::flags(const QModelIndex & index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    if ( !index.parent().isValid() ) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

KConfigGroup SnippetStore::getConfig()
{
    return m_plugin->core()->activeSession()->config()->group("Snippets");
}

bool SnippetStore::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ( role == Qt::EditRole && value.toString().isEmpty() ) {
        // don't allow empty names
        return false;
    }
    const bool success = QStandardItemModel::setData(index, value, role);
    if ( !success || role != Qt::EditRole ) {
        return success;
    }

    // when we edited something, save the repository

    QStandardItem* repoItem = 0;
    if ( index.parent().isValid() ) {
        repoItem = itemFromIndex(index.parent());
    } else {
        repoItem = itemFromIndex(index);
    }

    SnippetRepository* repo = dynamic_cast<SnippetRepository*>(repoItem);
    if ( repo ) {
        repo->save();
    }
    return true;
}

SnippetRepository* SnippetStore::repositoryForFile(const QString& file)
{
    for ( int i = 0; i < rowCount(); ++i ) {
        if ( SnippetRepository* repo = dynamic_cast<SnippetRepository*>(item(i)) ) {
            if ( repo->file() == file ) {
                return repo;
            }
        }
    }
    return 0;
}

void SnippetStore::unregisterScript(KTextEditor::TemplateScript* script)
{
    if ( m_scriptregistrar ) {
        m_scriptregistrar->unregisterTemplateScript(script);
    }
}

KTextEditor::TemplateScript* SnippetStore::registerScript(const QString& script)
{
    if ( m_scriptregistrar ) {
        return m_scriptregistrar->registerTemplateScript(this, script);
    }
    return 0;
}


#include "snippetstore.moc"
