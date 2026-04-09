/*
SPDX-FileCopyrightText: 2010 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QChar>
#include <QCollator>
#include <QDir>
#include <QLatin1String>
#include <QLatin1StringView>
#include <QString>

// edge-cases, which are more convinient too keep here outside of the KateSQLConstants namespace
inline constexpr QChar FieldIsRequired(u'*');
enum KWalletConnectionResponse {
    Success = 0,
    Error = -1,
    RejectedByUser = -2,
};

/**
 * Shared constants for the Kate SQL plugin, grouped by subsystem.
 */
namespace KateSQLConstants
{
/**
 * Configuration constants: KConfig group names and entry keys.
 */
namespace Config
{
/// KConfig group names.
inline constexpr QLatin1String PluginGroup("KateSQLPlugin");
inline constexpr QLatin1String OutputCustomizationGroup("OutputCustomization");

/// Plugin-level KConfig entry keys.
const auto SaveConnections("SaveConnections");
const auto LastUsed("LastUsed");

/**
 * Keys identifying data-type style contexts.
 * Used as hash keys in DataOutputStyleHelper and as config sub-group
 * names under the OutputCustomization group.
 */
namespace TypesToStyle
{
inline constexpr QLatin1String Text("text");
inline constexpr QLatin1String Number("number");
inline constexpr QLatin1String Null("null");
inline constexpr QLatin1String Blob("blob");
inline constexpr QLatin1String DateTime("datetime");
inline constexpr QLatin1String Bool("bool");
}

/**
 * Entry keys stored inside each style context's config sub-group
 * (e.g. font, colors).
 */
namespace Style
{
const auto Font("font");
const auto ForegroundColor("foregroundColor");
const auto BackgroundColor("backgroundColor");
}

}

/**
 * Connection attribute keys and driver identifiers.
 * Used as wizard field names, KConfig entry keys, and KKeychain JSON keys.
 */
namespace Connection
{
const auto Driver("driver");
const auto Database("database");
const auto Hostname("hostname");
const auto Username("username");
const auto Password("password");
const auto Options("options");
const auto Port("port");

const auto StdOptions("stdOptions");
const auto SqliteOptions("sqliteOptions");
const auto Path("path");
const auto ConnectionName("connectionName");

/// SQL driver identifiers.
inline constexpr QLatin1String QSQLite("QSQLITE");
}

/**
 * Export wizard and data export constants.
 */
namespace Export
{
/// Field names registered with QWizard::registerField() in ExportWizard pages
/// and read back via QWizard::field() in DataOutputWidget::slotExport().
namespace Fields
{
inline constexpr QLatin1String OutputDocument("outDocument");
inline constexpr QLatin1String OutputClipboard("outClipboard");
inline constexpr QLatin1String OutputFile("outFile");
inline constexpr QLatin1String OutputFileUrl("outFileUrl");

inline constexpr QLatin1String ExportColumnNames("exportColumnNames");
inline constexpr QLatin1String ExportLineNumbers("exportLineNumbers");
inline constexpr QLatin1String CheckQuoteStrings("checkQuoteStrings");
inline constexpr QLatin1String CheckQuoteNumbers("checkQuoteNumbers");
inline constexpr QLatin1String QuoteStringsChar("quoteStringsChar");
inline constexpr QLatin1String QuoteNumbersChar("quoteNumbersChar");
inline constexpr QLatin1String FieldDelimiter("fieldDelimiter");
}

/**
 * Escape sequences used to represent special delimiter characters in the
 * export wizard's field delimiter input. These are the literal strings
 * the user types, which are later replaced with the actual characters
 * in DataOutputWidget::slotExport().
 */
namespace Delimiters
{
inline constexpr QLatin1String Tab("\\t");
inline constexpr QLatin1String CarriageReturn("\\r");
inline constexpr QLatin1String Newline("\\n");
}

/**
 * Default values for data export operations.
 *
 * Two sets of defaults are provided:
 * - Copy-paste defaults: used for quick copy/paste via Ctrl+C/V
 * - Wizard defaults: used as initial values in the ExportWizard dialog
 */
namespace DefaultValues
{
inline constexpr bool IsExportingColumnNames(false);
inline constexpr bool IsExportingLineNumbers(false);
inline constexpr bool IsQuotingStrings(false);
inline constexpr bool IsQuotingNumbers(false);

inline constexpr QChar NoQuotingChar(u'\0');

inline constexpr QChar QuoteStringCharForCopyPaste(u'"');
inline constexpr QChar QuoteNumbersCharForCopyPaste(NoQuotingChar);
inline constexpr QChar FieldDelimiterForCopyPaste(u'\t');
inline constexpr QChar LineDelimiterForCopyPaste(u'\n');

inline constexpr QLatin1String QuoteStringCharForWizard("\"");
inline constexpr QLatin1String QuoteNumbersCharForWizard("\"");
inline constexpr QLatin1String FieldDelimiterForWizard(Delimiters::Tab);
}
}

} // namespace KateSQL
