/*
    SPDX-FileCopyrightText: 2024 Karthik Nishanth <kndevl@outlook.com>
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "hintstate.h"

#include <algorithm>
#include <qlist.h>

HintState::HintState() = default;

void HintState::upsert(HintState::ID instanceId, const QString &text, TextHintMarkupKind kind, const QList<HintAction> &actions)
{
    for (auto &[id, hint] : m_hints) {
        if (id == instanceId) {
            hint = Hint{.m_text = text, .m_kind = kind, .m_actions = actions};
            return;
        }
    }
    m_hints.push_back(HintWithId{instanceId, Hint{.m_text = text, .m_kind = kind, .m_actions = actions}});
}

void HintState::clear()
{
    m_hints.clear();
}

void HintState::remove(HintState::ID instanceId)
{
    std::erase_if(m_hints, [instanceId](const HintWithId &idToHint) {
        return idToHint.id == instanceId;
    });
}

void HintState::render(const std::function<void(const Hint &)> &callback)
{
    const bool nonPlainText = std::any_of(m_hints.begin(), m_hints.end(), [](const auto &entry) {
        const auto &kind = entry.hint.m_kind;
        return kind == TextHintMarkupKind::MarkDown || kind == TextHintMarkupKind::None || entry.hint.m_actions.size() > 0;
    });

    QList<HintAction> totalActions;
    const auto renderMode = nonPlainText ? TextHintMarkupKind::MarkDown : TextHintMarkupKind::PlainText;
    QString contents;

    if (renderMode == TextHintMarkupKind::PlainText) {
        for (const auto &[_, hint] : m_hints) {
            if (!contents.isEmpty()) {
                contents += QStringLiteral("\n");
            }
            contents += hint.m_text;
        }
    } else {
        bool first = true;
        for (const auto &[_, hint] : m_hints) {
            if (!first) {
                contents += QStringLiteral("\n----\n");
            }
            first = false;
            const auto &[text, kind, actions] = hint;
            if (kind == TextHintMarkupKind::PlainText) {
                // Render plaintext as-is
                contents += QStringLiteral("<div>");
                contents += text.toHtmlEscaped();
                contents += QStringLiteral("</div>\n");
            } else {
                contents += text;
            }

            if (actions.size() > 0) {
                contents += QStringLiteral(
                    "\n<table style='margin-top:5px;' width='100%' bgcolor='#100001'>"
                    "<tr><td align='right'>");
                for (int i = 0; i < actions.size(); ++i) {
                    contents += QStringLiteral("&nbsp;<a href='action:%1'>%2</a>").arg(totalActions.size() + i).arg(actions[i].m_text);
                }
                contents += QStringLiteral(
                    "</td></tr>"
                    "</table>\n\n");
                totalActions += actions;
            }
        }
    }

    if (m_rendered != contents) {
        callback(Hint{.m_text = contents, .m_kind = renderMode, .m_actions = totalActions});
        m_rendered = std::move(contents);
    }
}