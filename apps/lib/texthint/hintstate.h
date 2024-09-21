#pragma once

#include "KateTextHintManager.h"

#include <QString>

#include <functional>
#include <tuple>

class HintState
{
public:
    using ID = size_t;
    struct Hint {
        QString m_text;
        TextHintMarkupKind m_kind;
    };

    HintState();
    void upsert(ID instanceId, const QString &text, TextHintMarkupKind kind);
    void remove(ID instanceId);
    void clear();
    void render(const std::function<void(const Hint &)> &callback);

private:
    std::map<ID, Hint> m_hints;
    QString m_rendered;
};
