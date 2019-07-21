/***************************************************************************
 *   Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>              *
 *   Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>          *
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

#include "lspclienthover.h"
#include "lspclientplugin.h"

#include "lspclient_debug.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QToolTip>

class LSPClientHoverImpl : public LSPClientHover
{
    Q_OBJECT

    typedef LSPClientHoverImpl self_type;

    QSharedPointer<LSPClientServerManager> m_manager;
    QSharedPointer<LSPClientServer> m_server;

    LSPClientServer::RequestHandle m_handle;

public:
    LSPClientHoverImpl(QSharedPointer<LSPClientServerManager> manager)
        : LSPClientHover(), m_manager(manager), m_server(nullptr)
    {
    }

    void setServer(QSharedPointer<LSPClientServer> server) override
    {
        m_server = server;
    }

    /**
     * This function is called whenever the users hovers over text such
     * that the text hint delay passes. Then, textHint() is called
     * for each registered TextHintProvider.
     *
     * Return the text hint (possibly Qt richtext) for @p view at @p position.
     *
     * If you do not have any contents to show, just return an empty QString().
     *
     * \param view the view that requests the text hint
     * \param position text cursor under the mouse position
     * \return text tool tip to be displayed, may be Qt richtext
     */
    QString textHint(KTextEditor::View *view,
                             const KTextEditor::Cursor &position) override
    {
        // hack: delayed handling of tooltip on our own, the API is too dumb for a-sync feedback ;=)
        if (m_server) {
            QPointer<KTextEditor::View> v(view);
            auto h = [this,v,position] (const LSPHover & info)
            {
                if (!v || info.contents.value.isEmpty()) {
                    return;
                }

                auto localCoordinates = v->cursorToCoordinate(position);
                QToolTip::showText(v->mapToGlobal(localCoordinates), info.contents.value);
            };

            m_handle.cancel() = m_server->documentHover(view->document()->url(), position, this, h);
        }

        return QString();
    }

};

LSPClientHover*
LSPClientHover::new_(QSharedPointer<LSPClientServerManager> manager)
{
    return new LSPClientHoverImpl(manager);
}

#include "lspclienthover.moc"
