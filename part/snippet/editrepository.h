/***************************************************************************
 *   Copyright 2010 Milian Wolff <mail@milianw.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef EDITREPOSITORY_H
#define EDITREPOSITORY_H

#include "ui_editrepository.h"

#include <KDialog>

class SnippetRepository;

/**
 * This dialog is used to create/edit snippet repositories and
 * the snippets in them.
 *
 * @author Milian Wolff <mail@milianw.de>
 */
class EditRepository : public KDialog, public Ui::EditRepositoryBase
{
    Q_OBJECT

public:
    /// @p repo set to 0 when you want to create a new repository.
    explicit EditRepository(SnippetRepository* repo, QWidget* parent = 0);
    virtual ~EditRepository();

private:
    SnippetRepository* m_repo;

private slots:
    void save();
    void validate();
    void updateFileTypes();
};

#endif

