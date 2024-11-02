/*
    SPDX-FileCopyrightText: 2024 Karthik Nishanth <kndevl@outlook.com>
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "KateTextHintManager.h"

#include <QString>

#include <functional>

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
    std::vector<std::pair<ID, Hint>> m_hints;
    QString m_rendered;
};
