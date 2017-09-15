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

#ifndef KATE_PROJECT_ITEM_H
#define KATE_PROJECT_ITEM_H

#include <QStandardItem>
#include <KTextEditor/ModificationInterface>

namespace KTextEditor
{
class Document;

}

/**
 * Class representing a item inside a project.
 * Items can be: projects, directories, files
 */
class KateProjectItem : public QStandardItem
{

public:
    /**
     * Possible Types
     */
    enum Type {
        Project
        , Directory
        , File
    };

    /**
     * construct new item with given text
     * @param type type for this item
     * @param text text for this item
     */
    KateProjectItem(Type type, const QString &text);

    /**
     * deconstruct project
     */
    ~KateProjectItem() override;

    /**
     * Overwritten data methode for on-demand icon creation and co.
     * @param role role to get data for
     * @return data for role
     */
    QVariant data(int role = Qt::UserRole + 1) const override;

public:
    void slotModifiedChanged(KTextEditor::Document *);
    void slotModifiedOnDisk(KTextEditor::Document *document,
                            bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);

private:
    QIcon *icon() const;

private:
    /**
     * type
     */
    const Type m_type;

    /**
     * cached icon
     */
    mutable QIcon *m_icon;

    /**
     * for document icons
     */
    QString m_emblem;

};

#endif

