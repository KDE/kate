/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "lspclienthover.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include "lspclientserver.h"
#include "lspclientservermanager.h"
#include "texthint/KateTextHintManager.h"

static TextHintMarkupKind toKateMarkupKind(LSPMarkupKind kind)
{
    switch (kind) {
    case LSPMarkupKind::None:
    case LSPMarkupKind::PlainText:
        return TextHintMarkupKind::PlainText;
    case LSPMarkupKind::MarkDown:
        return TextHintMarkupKind::MarkDown;
    }
    qWarning() << Q_FUNC_INFO << "Unknown markup kind" << (int)kind;
    return TextHintMarkupKind::PlainText;
}

class LSPClientHoverImpl : public LSPClientHover
{
    Q_OBJECT

    typedef LSPClientHoverImpl self_type;

    std::shared_ptr<LSPClientServerManager> m_manager;
    std::shared_ptr<LSPClientServer> m_server;

    LSPClientServer::RequestHandle m_handle;
    KateTextHintProvider *m_textHintProvider;

public:
    explicit LSPClientHoverImpl(std::shared_ptr<LSPClientServerManager> manager, KateTextHintProvider *provider)
        : m_manager(std::move(manager))
        , m_server(nullptr)
        , m_textHintProvider(provider)
    {
    }

    void setServer(std::shared_ptr<LSPClientServer> server) override
    {
        m_server = server;
    }

    QString showTextHint(KTextEditor::View *view, const KTextEditor::Cursor &position, bool manual) override
    {
        if (!position.isValid()) {
            return {};
        }

        // hack: delayed handling of tooltip on our own, the API is too dumb for a-sync feedback ;=)
        if (m_server) {
            QPointer<KTextEditor::View> v(view);
            auto h = [v, position, manual, this](const LSPHover &info) {
                if (!v || info.contents.isEmpty()) {
                    return;
                }

                // combine contents elements to one string
                LSPMarkupKind kind = LSPMarkupKind::PlainText;
                QString finalTooltip;
                for (auto &element : info.contents) {
                    kind = element.kind;
                    if (!finalTooltip.isEmpty()) {
                        finalTooltip.append(QLatin1Char('\n'));
                    }
                    finalTooltip.append(element.value);
                }

                // make sure there is no selection, otherwise we interrupt
                if (!v->selection()) {
                    if (manual) {
                        Q_EMIT m_textHintProvider->showTextHint(finalTooltip, toKateMarkupKind(kind), position);
                    } else {
                        Q_EMIT m_textHintProvider->textHintAvailable(finalTooltip, toKateMarkupKind(kind), position);
                    }
                }
            };

            if (view && view->document()) {
                auto doc = view->document();
                if (doc->wordAt(position).isEmpty() || view->selection()) {
                    return {};
                }
                m_handle.cancel() = m_server->documentHover(view->document()->url(), position, this, h);
            }
        }

        return QString();
    }
};

LSPClientHover *LSPClientHover::new_(std::shared_ptr<LSPClientServerManager> manager, class KateTextHintProvider *provider)
{
    return new LSPClientHoverImpl(std::move(manager), provider);
}

#include "lspclienthover.moc"
#include "moc_lspclienthover.cpp"
