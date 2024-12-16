/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include "diagnostic_suppression.h"
#include "diagnostic_types.h"
#include "diagnosticview.h"

#include <QStandardItem>

namespace DiagnosticModelRole
{
enum {
    // preserve UserRole for generic use where needed
    FileUrlRole = Qt::UserRole + 1,
    RangeRole,
    KindRole,
    ProviderRole,
};
}

enum {
    DiagnosticItem_File = QStandardItem::UserType + 1,
    DiagnosticItem_Diag,
    DiagnosticItem_Fix,
};

// custom item subclass that captures additional attributes;
// a bit more convenient than the variant/role way
struct DiagnosticItem : public QStandardItem {
    Diagnostic m_diagnostic;

    explicit DiagnosticItem(const Diagnostic &d)
        : m_diagnostic(d)
    {
    }

    int type() const override
    {
        return DiagnosticItem_Diag;
    }

    bool hasFixes() const
    {
        if (!hasChildren()) {
            return false;
        }
        for (int i = 0; i < rowCount(); ++i) {
            if (child(i)->type() == DiagnosticItem_Fix) {
                return true;
            }
        }
        return false;
    }
};

// likewise; a custom item for document level model item
struct DocumentDiagnosticItem : public QStandardItem {
private:
    QList<DiagnosticsProvider *> m_providers;

public:
    bool enabled = true;
    std::unique_ptr<DiagnosticSuppression> diagnosticSuppression;
    int type() const override
    {
        return DiagnosticItem_File;
    }

    const QList<DiagnosticsProvider *> &providers() const
    {
        return m_providers;
    }
    void addProvider(DiagnosticsProvider *p)
    {
        if (!m_providers.contains(p)) {
            m_providers.push_back(p);
        }
    }

    /**
     * Remove items for given provider. If p == null, remove all items of the file
     * except the items whose provider has m_persistentDiagnostics.
     */
    int removeItemsForProvider(DiagnosticsProvider *p)
    {
        int removedCount = 0;
        if (m_providers.size() == 1) {
            if (p == nullptr && m_providers.back()->persistentDiagnostics()) {
                // If there is only 1 provider and it's diagnostics are persistent, we have nothing to do here
                return removedCount;
            } else if (m_providers.contains(p)) {
                m_providers.clear();
                removedCount = rowCount();
                setRowCount(0);
                return removedCount;
            } else {
                // if we don't have any diagnostics from this provider, we have nothing to do
                return removedCount;
            }
        }

        QVarLengthArray<DiagnosticsProvider *, 3> removedProviders;
        auto removeProvider = [&removedProviders](DiagnosticsProvider *p) {
            if (!removedProviders.contains(p)) {
                removedProviders.append(p);
            }
        };
        // We have more than one diagnostic provider for this file
        if (p == nullptr) {
            // Remove all diagnostics where provider->persistentDiagnostics() == false
            int start = -1;
            int count = 0;
            for (int i = 0; i < rowCount(); ++i) {
                auto item = child(i);
                auto itemProvider = item->data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>();
                if (!itemProvider->persistentDiagnostics()) {
                    if (start == -1) {
                        start = i;
                    }
                    count++;
                    removeProvider(itemProvider);
                } else {
                    if (start > -1 && count != 0) {
                        removedCount += count;
                        removeRows(start, count);
                        i = start - 1;
                        start = -1;
                        count = 0;
                    }
                }
            }
            if (start > -1 && count != 0) {
                removedCount += count;
                removeRows(start, count);
            }
        } else {
            // remove all diagnostics from the given provider
            int start = -1;
            int count = 0;
            for (int i = 0; i < rowCount(); ++i) {
                auto item = child(i);
                auto itemProvider = item->data(DiagnosticModelRole::ProviderRole).value<DiagnosticsProvider *>();
                if (itemProvider == p) {
                    if (start == -1) {
                        start = i;
                    }
                    removeProvider(itemProvider);
                    count++;
                } else {
                    if (start > -1 && count != 0) {
                        removedCount += count;
                        removeRows(start, count);
                        i = start - 1;
                        start = -1;
                        count = 0;
                    }
                }
            }
            if (start > -1 && count != 0) {
                removedCount += count;
                removeRows(start, count);
            }
        }

        // remove the providers for which we don't have diagnostics
        for (DiagnosticsProvider *p : removedProviders) {
            m_providers.removeOne(p);
        }
        return removedCount;
    }
};

struct DiagnosticFixItem : public QStandardItem {
    DiagnosticFix fix;
    int type() const override
    {
        return DiagnosticItem_Fix;
    }
};
