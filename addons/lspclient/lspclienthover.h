/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

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
    static LSPClientHover *new_(QSharedPointer<LSPClientServerManager> manager);

    LSPClientHover()
        : KTextEditor::TextHintProvider()
    {
    }

    virtual void setServer(QSharedPointer<LSPClientServer> server) = 0;
};

#endif
