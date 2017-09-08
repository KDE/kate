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

#ifndef EDITREPOSITORY_H
#define EDITREPOSITORY_H

#include "ui_editrepository.h"

#include <QDialog>

class SnippetRepository;

/**
 * This dialog is used to create/edit snippet repositories and
 * the snippets in them.
 *
 * @author Milian Wolff <mail@milianw.de>
 */
class EditRepository : public QDialog, public Ui::EditRepositoryBase
{
    Q_OBJECT

public:
    /// @p repo set to 0 when you want to create a new repository.
    explicit EditRepository(SnippetRepository* repo, QWidget* parent = nullptr);

private:
    SnippetRepository* m_repo;

private Q_SLOTS:
    void save();
    void validate();
    void updateFileTypes();
};

#endif

