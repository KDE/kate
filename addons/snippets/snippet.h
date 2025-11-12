/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  SPDX-FileCopyrightText: 2007 Robert Gruber <rgruber@users.sourceforge.net>
 *  SPDX-FileCopyrightText: 2010 Milian Wolff <mail@milianw.de>
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QStandardItem>

class QAction;

namespace KTextEditor
{
class View;
}

/**
 * One object of this class represents a single snippet.
 * Multiple snippets are stored in one repository (XML-file).
 *
 * To access the snippet's name (which should also be used for matching
 * during code completion) use @p QStandardItem::text().
 *
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 * @author Milian Wolff <mail@milianw.de>
 */
class Snippet : public QStandardItem
{
public:
    /**
     * Construct an empty snippet.
     */
    Snippet();
    ~Snippet() override;

    static constexpr inline int SnippetItemType = QStandardItem::UserType + 2;

    int type() const override
    {
        return SnippetItemType;
    }

    static Snippet *fromItem(QStandardItem *item)
    {
        if (item && item->type() == SnippetItemType) {
            return static_cast<Snippet *>(item);
        }
        return nullptr;
    }

    enum SnippetType {
        TextTemplate,
        Script
    };

    /**
     * Returns the type (template or script) of this snippet.
     */
    SnippetType snippetType() const;

    /**
     * Map snippet type to string (for storage).
     */
    static QString typeToString(const SnippetType type);

    /**
     * Map stored string back to snippet type.
     */
    static SnippetType stringToType(const QString &string);

    /**
     * Sets the actual contents of this snippet.
     */
    void setSnippet(const QString &snippet, const QString &description, SnippetType type);

    /**
     * Returns the actual contents of this snippet.
     */
    QString snippet() const;

    /**
     * Returns the description, or - if that is empty - the snippet contents.
     * For use in tooltips.
     */
    QString description() const;

    /**
     * Action to trigger insertion of this snippet.
     */
    QAction *action();

    /**
     * Insert/run this snippet in the given view
     */
    void apply(KTextEditor::View *view, const QString &repoScript) const;

    void registerActionForView(QWidget *view);

    QVariant data(int role = Qt::UserRole + 1) const override;

private:
    /// the actual snippet contents aka \code<fillin>\endcode
    QString m_snippet;
    /// optional description
    QString m_description;
    /// the type of this snippet
    SnippetType m_type = TextTemplate;
    /// the insertion action for this snippet.
    QAction *m_action = nullptr;
};

Q_DECLARE_METATYPE(Snippet *)
