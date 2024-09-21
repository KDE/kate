#include "hintstate.h"

#include <algorithm>

HintState::HintState() = default;

void HintState::upsert(HintState::ID instanceId, const QString &text, TextHintMarkupKind kind)
{
    m_hints[instanceId] = Hint{text, kind};
}

void HintState::clear()
{
    m_hints.clear();
}

void HintState::remove(HintState::ID instanceId)
{
    m_hints.erase(instanceId);
}

void HintState::render(const std::function<void(const Hint &)> &callback)
{
    const auto nonPlainText = std::any_of(m_hints.begin(), m_hints.end(), [](const auto &entry) {
        const auto &kind = entry.second.m_kind;
        return kind == TextHintMarkupKind::MarkDown || kind == TextHintMarkupKind::None;
    });

    const auto renderMode = nonPlainText ? TextHintMarkupKind::MarkDown : TextHintMarkupKind::PlainText;
    QString contents;

    if (renderMode == TextHintMarkupKind::PlainText) {
        for (const auto &[_, hint] : m_hints) {
            contents += hint.m_text;
            contents += QStringLiteral("\n");
        }
    } else {
        for (const auto &[_, hint] : m_hints) {
            const auto &[text, kind] = hint;
            if (kind == TextHintMarkupKind::PlainText) {
                // Render plaintext as-is
                contents += QStringLiteral("<p>");
                contents += text.toHtmlEscaped();
                contents += QStringLiteral("</p>\n");
            } else {
                contents += text;
            }
            contents += QStringLiteral("\n----\n");
        }
    }

    if (m_rendered != contents) {
        callback(Hint{contents, renderMode});
        m_rendered = std::move(contents);
    }
}
