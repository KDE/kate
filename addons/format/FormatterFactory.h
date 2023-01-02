/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef FORMATTER_FACTORY_H
#define FORMATTER_FACTORY_H

#include "FormatPlugin.h"
#include "Formatters.h"

static AbstractFormatter *formatterForDoc(KTextEditor::Document *doc, FormatPlugin *plugin)
{
    if (!doc) {
        qWarning() << "Unexpected null doc";
        return nullptr;
    }

    const auto mode = doc->highlightingMode().toLower();
    if (mode.contains(QStringLiteral("c++")) || mode == QStringLiteral("c")) {
        return new ClangFormat(doc);
    } else if (mode == QStringLiteral("dart")) {
        return new DartFormat(doc);
    } else if (mode == QStringLiteral("javascript") || mode == QStringLiteral("typescript") || mode == QStringLiteral("typescript react (tsx)")
               || mode == QStringLiteral("javascript react (jsx)") || mode == QStringLiteral("css")) {
        return new PrettierFormat(doc);
    } else if (mode == QStringLiteral("json")) {
        if (plugin->formatterForJson == Formatters::Prettier) {
            return new PrettierFormat(doc);
        } else if (plugin->formatterForJson == Formatters::ClangFormat) {
            return new ClangFormat(doc);
        } else if (plugin->formatterForJson == Formatters::Jq) {
            return new JsonJqFormat(doc);
        }
        qWarning() << "Unexpected formatterForJson value: " << (int)plugin->formatterForJson;
        return new JsonJqFormat(doc);
    } else if (mode == QStringLiteral("rust")) {
        return new RustFormat(doc);
    } else if (mode == QStringLiteral("xml")) {
        return new XmlLintFormat(doc);
    } else if (mode == QStringLiteral("go")) {
        return new GoFormat(doc);
    }

    Utils::showMessage(i18n("Failed to run formatter. Unsupporter language %1", mode), {}, i18n("Format"), i18n("Info"));

    return nullptr;
}

#endif
