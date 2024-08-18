/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "Formatters.h"
#include "FormattersEnum.h"

#include <QJsonObject>

static AbstractFormatter *formatterForDoc(KTextEditor::Document *doc, const QJsonObject &config)
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

    // NOTE: When adding a new formatter ensure that it is documented in plugins.docbook
    if (is_or_contains("c++") || is("c") || is("objective-c") || is("objective-c++") || is("protobuf")) {
        return new ClangFormat(config, doc);
    } else if (is("dart")) {
        return new DartFormat(config, doc);
    } else if (is("html")) {
        return new PrettierFormat(config, doc);
    } else if (is("javascript") || is("typescript") || is("typescript react (tsx)") || is("javascript react (jsx)") || is("css")) {
        return new PrettierFormat(config, doc);
    } else if (is("json")) {
        const auto configValue = config.value(QStringLiteral("formatterForJson")).toString();
        Formatters f = formatterForName(configValue, Formatters::Prettier);
        if (f == Formatters::Prettier) {
            return new PrettierFormat(config, doc);
        } else if (f == Formatters::ClangFormat) {
            return new ClangFormat(config, doc);
        } else if (f == Formatters::Jq) {
            return jqFmt(config, doc);
        }
        Utils::showMessage(i18n("Unknown formatterForJson: %1", configValue), {}, i18n("Format"), MessageType::Error);
        return jqFmt(config, doc);
    } else if (is("rust")) {
        return rustFormat(config, doc);
    } else if (is("xml")) {
        return new XmlLintFormat(config, doc);
    } else if (is("go")) {
        return goFormat(config, doc);
    } else if (is("zig")) {
        return zigFormat(config, doc);
    } else if (is("cmake")) {
        return cMakeFormat(config, doc);
    } else if (is("python")) {
        const auto configValue = config.value(QStringLiteral("formatterForPython")).toString();
        Formatters f = formatterForName(configValue, Formatters::Ruff);
        if (f == Formatters::Ruff) {
            return ruffFormat(config, doc);
        } else if (f == Formatters::Autopep8) {
            return autoPep8Format(config, doc);
        }
        Utils::showMessage(i18n("Unknown formatterForPython: %1", configValue), {}, i18n("Format"), MessageType::Error);
        return ruffFormat(config, doc);
    } else if (is("d")) {
        return dfmt(config, doc);
    } else if (is("fish")) {
        return fishIndent(config, doc);
    } else if (is("bash")) {
        return shfmt(config, doc);
    } else if (is("nixfmt")) {
        return nixfmt(config, doc);
    } else if (is("qml")) {
        return new QMLFormat(config, doc);
    } else if (is("yaml")) {
        const auto configValue = config.value(QStringLiteral("formatterForYaml")).toString();
        Formatters f = formatterForName(configValue, Formatters::YamlFmt);
        if (f == Formatters::YamlFmt) {
            return yamlfmt(config, doc);
        } else if (f == Formatters::Prettier) {
            return new PrettierFormat(config, doc);
        }
        Utils::showMessage(i18n("Unknown formatterForYaml: %1, falling back to yamlfmt", configValue), {}, i18n("Format"), MessageType::Error);
        return yamlfmt(config, doc);
    } else if (is("opsi-script")) {
        return opsiScriptBeautifier(config, doc);
    } else if (is("odin")) {
        return odinFmt(config, doc);
    }

    static QList<QString> alreadyWarned;
    if (!alreadyWarned.contains(mode)) {
        alreadyWarned.push_back(mode);
        Utils::showMessage(i18n("Failed to run formatter. Unsupported language %1", mode), {}, i18n("Format"), MessageType::Info);
    }

    return nullptr;
}
