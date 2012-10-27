/***************************************************************************
 *   Copyright 2010 Milian Wolff <mail@milianw.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef EDITSNIPPET_H
#define EDITSNIPPET_H

#include <KDialog>

namespace KTextEditor
{
class Document;
class View;
}

class SnippetRepository;
class Snippet;

class QStandardItemModel;

namespace Ui {
class EditSnippetBase;
}

/**
 * This dialog is used to create/edit snippets in a given repository.
 *
 * @author Milian Wolff <mail@milianw.de>
 */
class EditSnippet : public KDialog
{
    Q_OBJECT

public:
    /// @p snippet set to 0 when you want to create a new snippet.
    explicit EditSnippet(SnippetRepository* repo, Snippet* snippet, QWidget* parent = 0);
    virtual ~EditSnippet();

    void setSnippetText(const QString& text);

    virtual void reject();

private:
    Ui::EditSnippetBase* m_ui;
    SnippetRepository* m_repo;
    Snippet* m_snippet;
    KTextEditor::View* m_snippetView;
    KTextEditor::View* m_scriptsView;
    bool m_topBoxModified;

private slots:
    void save();
    void validate();
    void topBoxModified();

    void slotSnippetDocumentation();
    void slotScriptDocumentation();
};

#endif

