/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include <KConfigGroup>
#include <QList>
#include <QSet>
#include <QString>

class SessionDiagnosticSuppressions
{
    // file -> suppression
    // (empty file matches any file)
    QHash<QString, QSet<QString>> m_suppressions;
    const QString ENTRY_PREFIX{QStringLiteral("File_")};

public:
    void readSessionConfig(const KConfigGroup &cg)
    {
        const auto groups = cg.keyList();
        for (const auto &fkey : groups) {
            if (fkey.startsWith(ENTRY_PREFIX)) {
                QString fname = fkey.mid(ENTRY_PREFIX.size());
                QStringList entries = cg.readEntry(fkey, QStringList());
                if (entries.size()) {
                    m_suppressions[fname] = {entries.begin(), entries.end()};
                }
            }
        }
    }

    void writeSessionConfig(KConfigGroup &cg)
    {
        // clear existing entries
        cg.deleteGroup(QLatin1String());
        for (auto it = m_suppressions.begin(); it != m_suppressions.end(); ++it) {
            QStringList entries = it.value().values();
            if (entries.size()) {
                cg.writeEntry(ENTRY_PREFIX + it.key(), entries);
            }
        }
    }

    void add(const QString &file, const QString &diagnostic)
    {
        m_suppressions[file].insert(diagnostic);
    }

    void remove(const QString &file, const QString &diagnostic)
    {
        auto it = m_suppressions.find(file);
        if (it != m_suppressions.end()) {
            it->remove(diagnostic);
        }
    }

    bool hasSuppression(const QString &file, const QString &diagnostic)
    {
        auto it = m_suppressions.find(file);
        if (it != m_suppressions.end()) {
            return it->contains(diagnostic);
        } else {
            return false;
        }
    }

    std::vector<QString> getSuppressions(const QString &file)
    {
        std::vector<QString> result;

        for (const auto &entry : {QString(), file}) {
            auto it = m_suppressions.find(entry);
            if (it != m_suppressions.end()) {
                const QSet<QString> ds = it.value();
                for (const QString &d : ds) {
                    result.push_back(d);
                }
            }
        }
        return result;
    }
};
