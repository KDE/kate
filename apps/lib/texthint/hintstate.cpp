/*
    SPDX-FileCopyrightText: 2024 Karthik Nishanth <kndevl@outlook.com>
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "hintstate.h"

#include <algorithm>

HintState::HintState() = default;

void HintState::upsert(HintState::ID instanceId, const QString &text, TextHintMarkupKind kind)
{
    for (auto &[id, hint] : m_hints) {
        if (id == instanceId) {
            hint = Hint{.m_text = text, .m_kind = kind};
            return;
        }
    }
    m_hints.emplace_back(instanceId, Hint{.m_text = text, .m_kind = kind});
}

void HintState::clear()
{
    m_hints.clear();
}

void HintState::remove(HintState::ID instanceId)
{
    m_hints.erase(std::remove_if(m_hints.begin(),
                                 m_hints.end(),
                                 [instanceId](const std::pair<ID, Hint> &idToHint) {
                                     return idToHint.first == instanceId;
                                 }),
                  m_hints.end());
}

void HintState::render(const std::function<void(const Hint &)> &callback)
{
    const bool nonPlainText = std::any_of(m_hints.begin(), m_hints.end(), [](const auto &entry) {
        const auto &kind = entry.second.m_kind;
        return kind == TextHintMarkupKind::MarkDown || kind == TextHintMarkupKind::None;
    });

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
            const auto &[text, kind] = hint;
            if (kind == TextHintMarkupKind::PlainText) {
                // Render plaintext as-is
                contents += QStringLiteral("<p>");
                contents += text.toHtmlEscaped();
                contents += QStringLiteral("</p>\n");
            } else {
                contents += text;
            }
        }
    }

    if (m_rendered != contents) {
        callback(Hint{.m_text = contents, .m_kind = renderMode});
        m_rendered = std::move(contents);
    }
}
