/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include <KTextEditor/Range>
#include <QString>
#include <QUrl>
#include <qmetatype.h>

enum class DiagnosticSeverity {
    Unknown = 0,
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4,
};

struct SourceLocation {
    QUrl uri;
    KTextEditor::Range range;
};

struct DiagnosticRelatedInformation {
    // empty url / invalid range when absent
    SourceLocation location;
    QString message;
};

struct Diagnostic {
    KTextEditor::Range range;
    DiagnosticSeverity severity;
    QString code;
    QString source;
    QString message;
    QList<DiagnosticRelatedInformation> relatedInformation;
};

struct FileDiagnostics {
    QUrl uri;
    QList<Diagnostic> diagnostics;
};

struct DiagnosticFix {
    QString fixTitle;
    std::function<void()> fixCallback;
};
