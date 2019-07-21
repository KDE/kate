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

#ifndef LSPCLIENTHOVER_H
#define LSPCLIENTHOVER_H

#include "lspclientserver.h"
#include "lspclientservermanager.h"

#include <KTextEditor/TextHintInterface>

class LSPClientHover : public QObject, public KTextEditor::TextHintProvider
{
    Q_OBJECT

public:

    // implementation factory method
    static LSPClientHover* new_(QSharedPointer<LSPClientServerManager> manager);

    LSPClientHover()
        : KTextEditor::TextHintProvider()
    {}

    virtual void setServer(QSharedPointer<LSPClientServer> server) = 0;
};

#endif
