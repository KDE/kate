/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "FormatPlugin.h"
#include "Formatters.h"

static AbstractFormatter *formatterForDoc(KTextEditor::Document *doc, FormatPlugin *plugin)
{
    if (!doc) {
        qWarning() << "Unexpected null doc";
        return nullptr;
    }
    const auto mode = doc->highlightingMode().toLower();
    auto is = [mode](const char *s) {
        return mode == QLatin1String(s);
    };
    auto is_or_contains = [mode](const char *s) {
        return mode == QLatin1String(s) || mode.contains(QLatin1String(s));
    };

    if (is_or_contains("c++") || is("c") || is("objective-c") || is("objective-c++") || is("protobuf")) {
        return new ClangFormat(plugin->formatterConfig(), doc);
    } else if (is("dart")) {
        return new DartFormat(plugin->formatterConfig(), doc);
    } else if (is("html")) {
        return new PrettierFormat(plugin->formatterConfig(), doc);
    } else if (is("javascript") || is("typescript") || is("typescript react (tsx)") || is("javascript react (jsx)") || is("css")) {
        return new PrettierFormat(plugin->formatterConfig(), doc);
    } else if (is("json")) {
        if (plugin->formatterForJson == Formatters::Prettier) {
            return new PrettierFormat(plugin->formatterConfig(), doc);
        } else if (plugin->formatterForJson == Formatters::ClangFormat) {
            return new ClangFormat(plugin->formatterConfig(), doc);
        } else if (plugin->formatterForJson == Formatters::Jq) {
            return new JsonJqFormat(plugin->formatterConfig(), doc);
        }
        qWarning() << "Unexpected formatterForJson value: " << (int)plugin->formatterForJson;
        return new JsonJqFormat(plugin->formatterConfig(), doc);
    } else if (is("rust")) {
        return new RustFormat(plugin->formatterConfig(), doc);
    } else if (is("xml")) {
        return new XmlLintFormat(plugin->formatterConfig(), doc);
    } else if (is("go")) {
        return new GoFormat(plugin->formatterConfig(), doc);
    } else if (is("zig")) {
        return new ZigFormat(plugin->formatterConfig(), doc);
    }

    Utils::showMessage(i18n("Failed to run formatter. Unsupporter language %1", mode), {}, i18n("Format"), MessageType::Info);

    return nullptr;
}
