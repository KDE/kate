/*
    Copyright (C) 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    Copyright (C) 2019 Christoph Cullmann <cullmann@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
