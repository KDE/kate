/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include <KTextEditor/Range>
#include <QString>
#include <QUrl>

enum class DiagnosticSeverity {
    Unknown = 0,
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4,
};
Q_DECLARE_METATYPE(DiagnosticSeverity)

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
Q_DECLARE_METATYPE(Diagnostic)

struct FileDiagnostics {
    QUrl uri;
    QVector<Diagnostic> diagnostics;
};
Q_DECLARE_METATYPE(FileDiagnostics)

struct DiagnosticFix {
    QString fixTitle;
    std::function<void()> fixCallback;
};
Q_DECLARE_METATYPE(DiagnosticFix)
