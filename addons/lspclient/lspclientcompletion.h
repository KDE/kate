/***************************************************************************
 *   Copyright (C) 2019 by Mark Nauwelaerts <mark.nauwelaerts@gmail.com>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef LSPCLIENTCOMPLETION_H
#define LSPCLIENTCOMPLETION_H

#include "lspclientserver.h"
#include "lspclientservermanager.h"

#include <KTextEditor/CodeCompletionModel>
#include <KTextEditor/CodeCompletionModelControllerInterface>

class LSPClientCompletion : public KTextEditor::CodeCompletionModel, public KTextEditor::CodeCompletionModelControllerInterface
{
    Q_OBJECT

    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)

public:

    // implementation factory method
    static LSPClientCompletion* new_(QSharedPointer<LSPClientServerManager> manager);

    LSPClientCompletion(QObject * parent)
        : KTextEditor::CodeCompletionModel(parent)
    {}

    virtual void setServer(QSharedPointer<LSPClientServer> server) = 0;
    virtual void setSelectedDocumentation(bool) = 0;
};

#endif
