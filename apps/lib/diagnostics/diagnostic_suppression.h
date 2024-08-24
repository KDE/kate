/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include "ktexteditor_utils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardItem>

#include <KLocalizedString>
#include <KTextEditor/Document>

// helper data that holds diagnostics suppressions
class DiagnosticSuppression
{
    struct Suppression {
        QRegularExpression diag, code;
    };
    std::vector<Suppression> m_suppressions;

public:
    // construct from configuration
    DiagnosticSuppression(const QUrl &docUrl, const std::vector<QJsonObject> &serverConfigs, const std::vector<QString> &sessionSuppressions)
    {
        // check regexp and report
        auto checkRegExp = [](const QRegularExpression &regExp) {
            auto valid = regExp.isValid();
            if (!valid) {
                auto error = regExp.errorString();
                auto offset = regExp.patternErrorOffset();
                auto msg = i18nc("@info", "Error in regular expression: %1\noffset %2: %3", regExp.pattern(), offset, error);
                Utils::showMessage(msg, {}, QStringLiteral("LSP Client"), MessageType::Error);
            }
            return valid;
        };

        const auto localPath = docUrl.toLocalFile();
        for (const auto &serverConfig : serverConfigs) {
            const auto supps = serverConfig.value(QStringLiteral("suppressions")).toObject();
            for (const auto &entry : supps) {
                // should be (array) tuple (last element optional)
                // [url regexp, message regexp, code regexp]
                const auto patterns = entry.toArray();
                if (patterns.size() >= 2) {
                    const auto urlRegExp = QRegularExpression(patterns.at(0).toString());
                    if (urlRegExp.isValid() && urlRegExp.match(localPath).hasMatch()) {
                        QRegularExpression diagRegExp, codeRegExp;
                        diagRegExp = QRegularExpression(patterns.at(1).toString());
                        if (patterns.size() >= 3) {
                            codeRegExp = QRegularExpression(patterns.at(2).toString());
                        }
                        if (checkRegExp(diagRegExp) && checkRegExp(codeRegExp)) {
                            m_suppressions.push_back({diagRegExp, codeRegExp});
                        }
                    }
                }
            }
        }

        // also consider session suppressions
        for (const auto &entry : sessionSuppressions) {
            auto pattern = QRegularExpression::escape(entry);
            m_suppressions.push_back({QRegularExpression(pattern), {}});
        }
    }

    bool match(const QStandardItem &item, KTextEditor::Document *doc) const
    {
        for (const auto &s : m_suppressions) {
            if (s.diag.match(item.text()).hasMatch()) {
                // retrieve and check code text if we need to match the content as well
                if (doc && !s.code.pattern().isEmpty()) {
                    auto range = item.data(/*RangeData::RangeRole*/).value<KTextEditor::Range>();
                    auto code = doc->text(range);
                    if (!s.code.match(code).hasMatch()) {
                        continue;
                    }
                }
                return true;
            }
        }
        return false;
    }
};
