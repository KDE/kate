/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include "diagnostic_suppression.h"
#include "diagnostic_types.h"

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
    bool enabled = true;
    std::unique_ptr<DiagnosticSuppression> diagnosticSuppression;
    int type() const override
    {
        return DiagnosticItem_File;
    }
};

struct DiagnosticFixItem : public QStandardItem {
    DiagnosticFix fix;
    int type() const override
    {
        return DiagnosticItem_Fix;
    }
};
