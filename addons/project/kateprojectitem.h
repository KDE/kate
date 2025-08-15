/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KTextEditor/Document>
#include <QStandardItem>

/**
 * Class representing a item inside a project.
 * Items can be: projects, directories, files
 */
class KateProjectItem : public QStandardItem
{
public:
    /**
     * Possible Types
     * We start with 1 to have 0 as invalid value!
     */
    enum Type {
        LinkedProject = QStandardItem::UserType + 1,
        Project = QStandardItem::UserType + 2,
        Directory = QStandardItem::UserType + 3,
        File = QStandardItem::UserType + 4
    };

    /**
     * Our defined roles
     */
    enum Role {
        TypeRole = Qt::UserRole + 42,
        ProjectRole
    };

    /**
     * construct new item with given text
     * @param type type for this item
     * @param text text for this item
     */
    KateProjectItem(Type type, const QString &text, const QString &path);

    /**
     * deconstruct project
     */
    ~KateProjectItem() override;

    int type() const override
    {
        return m_type;
    }

    /**
     * Overwritten data method for on-demand icon creation and co.
     * @param role role to get data for
     * @return data for role
     */
    QVariant data(int role = Qt::UserRole + 1) const override;
    void setData(const QVariant &value, int role) override;

    /**
     * We want case-insensitive sorting and directories first!
     * @param other other element to compare with
     * @return is this element less than?
     */
    bool operator<(const QStandardItem &other) const override;

public:
    void slotModifiedChanged(KTextEditor::Document *);
    void slotModifiedOnDisk(KTextEditor::Document *document, bool isModified, KTextEditor::Document::ModifiedOnDiskReason reason);

private:
    QIcon *icon() const;

private:
    /**
     * type
     */
    const Type m_type;

    // File path this item represents
    QString m_path;

    /**
     * cached icon
     */
    mutable QIcon *m_icon = nullptr;

    /**
     * for document icons
     */
    QString m_emblem;
};
