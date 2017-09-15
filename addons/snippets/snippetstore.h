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

#ifndef __SNIPPETSTORE_H__
#define __SNIPPETSTORE_H__

#include <QStandardItemModel>
#include <KConfigGroup>

class SnippetRepository;
class KateSnippetGlobal;

namespace KTextEditor {
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
    static void init(KateSnippetGlobal* plugin);
    /**
     * Retuns the SnippetStore. Call init() to set it up first.
     */
    static SnippetStore* self();

    ~SnippetStore() override;
    KConfigGroup getConfig();
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    /**
     * Returns the repository for the given @p file if there is any.
     */
    SnippetRepository* repositoryForFile(const QString &file);
private:
    SnippetStore(KateSnippetGlobal* plugin);

    Qt::ItemFlags flags (const QModelIndex & index) const override;

    static SnippetStore* m_self;
    KateSnippetGlobal* m_plugin;
};

#endif

