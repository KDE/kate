/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2010 Milian Wolff <mail@milianw.de>
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

#include "snippetstore.h"

#include "katesnippetglobal.h"
#include "snippetrepository.h"

#include <QDir>
#include <QStandardPaths>

#include <KSharedConfig>

#include <ktexteditor/editor.h>
#include <ktexteditor/templateinterface2.h>

Q_DECLARE_METATYPE(KSharedConfig::Ptr)

SnippetStore* SnippetStore::m_self = 0;

SnippetStore::SnippetStore(KateSnippetGlobal* plugin)
    : m_plugin(plugin), m_scriptregistrar(0)
{
    m_self = this;
  //required for setting sessionConfig property
  qRegisterMetaType<KSharedConfig::Ptr>("KSharedConfig::Ptr");

    QStringList files;

    const QStringList dirs =
      QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("ktexteditor_snippets/data"), QStandardPaths::LocateDirectory)
      << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("ktexteditor_snippets/ghns"), QStandardPaths::LocateDirectory);

    foreach (const QString& dir, dirs) {
      const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.xml"));
      foreach (const QString& file, fileNames) {
        files.append(dir + QLatin1Char('/') + file);
      }
    }

    foreach(const QString& file, files ) {
        SnippetRepository* repo = new SnippetRepository(file);
        appendRow(repo);
    }

    m_scriptregistrar = qobject_cast<KTextEditor::TemplateScriptRegistrar *> (KTextEditor::Editor::instance());
}

SnippetStore::~SnippetStore()
{
    invisibleRootItem()->removeRows( 0, invisibleRootItem()->rowCount() );
    m_self = 0;
}

void SnippetStore::init(KateSnippetGlobal* plugin)
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
  /**
   * use KTextEditor::Editor session config object
   */
  return KTextEditor::Editor::instance()->property("sessionConfig").value<KSharedConfig::Ptr>()->group("Snippets");
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
