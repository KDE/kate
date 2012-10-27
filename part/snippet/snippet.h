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

#ifndef __SNIPPET_H__
#define __SNIPPET_H__

#include <QStandardItem>

class SnippetRepository;
class KAction;

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
    ~Snippet();

    /**
     * Returns the actual contents of this snippet.
     */
    QString snippet() const;
    /**
     * Sets the actual contents of this snippet.
     */
    void setSnippet(const QString& snippet);

    /**
     * Returns the display prefix of this snippet.
     */
    QString prefix() const;
    /**
     * Sets the display prefix of this snippet.
     */
    void setPrefix(const QString& prefix);

    /**
     * Returns the display postfix of this snippet.
     */
    QString postfix() const;
    /**
     * Sets the display postfix of this snippet.
     */
    void setPostfix(const QString& postfix);

    /**
     * Returns the display arguments of this snippet.
     */
    QString arguments() const;
    /**
     * Sets the display arguments of this snippet.
     */
    void setArguments(const QString& arguments);

    /**
     * Action to trigger insertion of this snippet.
     */
    KAction* action();

    virtual QVariant data(int role = Qt::UserRole + 1) const;

private:
    /// the actual snippet contents aka \code<fillin>\endcode
    QString m_snippet;
    /// the display postfix aka \code<displaypostfix>\endcode
    QString m_postfix;
    /// the display prefix aka \code<displayprefix>\endcode
    QString m_prefix;
    /// the display arguments aka \code<displayarguments>\endcode
    QString m_arguments;
    /// the insertion action for this snippet.
    KAction* m_action;
};

Q_DECLARE_METATYPE ( Snippet* )

#endif
