/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QJsonArray>
#include <QJsonObject>

namespace json
{
// local helper;
// recursively merge top json top onto bottom json
inline QJsonObject merge(const QJsonObject &bottom, const QJsonObject &top)
{
    QJsonObject result;
    for (auto item = top.begin(); item != top.end(); item++) {
        const auto &key = item.key();
        if (item.value().isObject()) {
            result.insert(key, merge(bottom.value(key).toObject(), item.value().toObject()));
        } else {
            result.insert(key, item.value());
        }
    }
    // parts only in bottom
    for (auto item = bottom.begin(); item != bottom.end(); item++) {
        if (!result.contains(item.key())) {
            result.insert(item.key(), item.value());
        }
    }
    return result;
}

inline void find(const QJsonValue &value, std::function<bool(const QJsonObject &)> check, QJsonObject &current)
{
    if (value.isArray()) {
        const auto av = value.toArray();
        for (const auto &e : av) {
            find(e, check, current);
        }
    } else if (value.isObject()) {
        auto ob = value.toObject();
        if (check(ob) && ob.size() > current.size()) {
            current = ob;
        }
    }
}

inline QJsonObject find(const QJsonValue &value, std::function<bool(const QJsonObject &)> check)
{
    QJsonObject current;
    find(value, check, current);
    return current;
}
}
