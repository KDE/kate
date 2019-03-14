/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "katemacroexpander.h"

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KLocalizedString>

#include <QDir>
#include <QFileInfo>
#include <QDate>
#include <QTime>
#include <QUuid>

#include <algorithm>

MacroExpander::MacroExpander(KTextEditor::View* view)
    : KWordMacroExpander()
    , m_view(view)
    , m_url(view ? view->document()->url().toLocalFile() : QString())
{
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:FileBaseName"), i18n("Current document: File base name without path and suffix."), [](const QStringView&, KTextEditor::View* view) {
        const auto url = view ? view->document()->url().toLocalFile() : QString();
        return QFileInfo(url).baseName();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:FileExtension"), i18n("Current document: File extension."), [](const QStringView&, KTextEditor::View* view) {
        const auto url = view ? view->document()->url().toLocalFile() : QString();
        return QFileInfo(url).completeSuffix();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:FileName"), i18n("Current document: File name without path."), [](const QStringView&, KTextEditor::View* view) {
        const auto url = view ? view->document()->url().toLocalFile() : QString();
        return QFileInfo(url).fileName();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:FilePath"), i18n("Current document: Full path including file name."), [](const QStringView&, KTextEditor::View* view) {
        const auto url = view ? view->document()->url().toLocalFile() : QString();
        return QFileInfo(url).absoluteFilePath();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Text"), i18n("Current document: Contents of entire file."), [](const QStringView&, KTextEditor::View* view) {
        return view ? view->document()->text() : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Path"), i18n("Current document: Full path excluding file name."), [](const QStringView&, KTextEditor::View* view) {
        const auto url = view ? view->document()->url().toLocalFile() : QString();
        return QFileInfo(url).absolutePath();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:NativeFilePath"), i18n("Current document: Full path including file name, with native path separator (backslash on Windows)."), [](const QStringView&, KTextEditor::View* view) {
        const auto url = view ? view->document()->url().toLocalFile() : QString();
        return url.isEmpty() ? QString() : QDir::toNativeSeparators(QFileInfo(url).absoluteFilePath());
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:NativePath"), i18n("Current document: Full path excluding file name, with native path separator (backslash on Windows)."), [](const QStringView&, KTextEditor::View* view) {
        const auto url = view ? view->document()->url().toLocalFile() : QString();
        return url.isEmpty() ? QString() : QDir::toNativeSeparators(QFileInfo(url).absolutePath());
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Cursor:Line"), i18n("Line number of the text cursor position in current document (starts with 0)."), [](const QStringView&, KTextEditor::View* view) {
        return view ? QString::number(view->cursorPosition().line()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Cursor:Column"), i18n("Column number of the text cursor position in current document (starts with 0)."), [](const QStringView&, KTextEditor::View* view) {
        return view ? QString::number(view->cursorPosition().column()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Cursor:XPos"), i18n("X component in global screen coordinates of the cursor position."), [](const QStringView&, KTextEditor::View* view) {
        return view ? QString::number(view->mapToGlobal(view->cursorPositionCoordinates()).x()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Cursor:YPos"), i18n("Y component in global screen coordinates of the cursor position."), [](const QStringView&, KTextEditor::View* view) {
        return view ? QString::number(view->mapToGlobal(view->cursorPositionCoordinates()).y()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Selection:Text"), i18n("Selection of current document."), [](const QStringView&, KTextEditor::View* view) {
        return (view && view->selection()) ? view->selectionText() : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Selection:StartLine"), i18n("Start line of selected text of current document."), [](const QStringView&, KTextEditor::View* view) {
        return (view && view->selection()) ? QString::number(view->selectionRange().start().line()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Selection:StartColumn"), i18n("Start column of selected text of current document."), [](const QStringView&, KTextEditor::View* view) {
        return (view && view->selection()) ? QString::number(view->selectionRange().start().column()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Selection:EndLine"), i18n("End line of selected text of current document."), [](const QStringView&, KTextEditor::View* view) {
        return (view && view->selection()) ? QString::number(view->selectionRange().end().line()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:Selection:EndColumn"), i18n("End column of selected text of current document."), [](const QStringView&, KTextEditor::View* view) {
        return (view && view->selection()) ? QString::number(view->selectionRange().end().column()) : QString();
    }));
    registerExactMatch(Variable(QStringLiteral("CurrentDocument:RowCount"), i18n("Number of rows of current document."), [](const QStringView&, KTextEditor::View* view) {
        return view ? QString::number(view->document()->lines()) : QString();
    }));

    registerExactMatch(Variable(QStringLiteral("Date:Locale"), i18n("The current date in current locale format."), [](const QStringView&, KTextEditor::View*) {
        return QDate::currentDate().toString(Qt::DefaultLocaleShortDate);
    }));
    registerExactMatch(Variable(QStringLiteral("Date:ISO"), i18n("The current date (ISO)."), [](const QStringView&, KTextEditor::View*) {
        return QDate::currentDate().toString(Qt::ISODate);
    }));
    registerPrefix(Variable(QStringLiteral("Date:"), i18n("The current date (QDate formatstring)."), [](const QStringView& str, KTextEditor::View*) {
        return QDate::currentDate().toString(str.right(str.length() - 5));
    }));

    registerExactMatch(Variable(QStringLiteral("Time:Locale"), i18n("The current time in current locale format."), [](const QStringView&, KTextEditor::View*) {
        return QTime::currentTime().toString(Qt::DefaultLocaleShortDate);
    }));
    registerExactMatch(Variable(QStringLiteral("Time:ISO"), i18n("The current time (ISO)."), [](const QStringView&, KTextEditor::View*) {
        return QTime::currentTime().toString(Qt::ISODate);
    }));
    registerPrefix(Variable(QStringLiteral("Time:"), i18n("The current time (QTime formatstring)."), [](const QStringView& str, KTextEditor::View*) {
        return QTime::currentTime().toString(str.right(str.length() - 5));
    }));

    registerPrefix(Variable(QStringLiteral("ENV:"), i18n("Access environment variables."), [](const QStringView& str, KTextEditor::View*) {
        return QString::fromLocal8Bit(qgetenv(str.right(str.size() - 4).toLocal8Bit().constData()));
    }));
    registerPrefix(Variable(QStringLiteral("JS:"), i18n("Evaluate simple JavaScript statements. The statements may not contain '{' nor '}' characters."), [](const QStringView& str, KTextEditor::View*) {
        // FIXME
        Q_UNUSED(str)
        return QString();
    }));

    registerExactMatch(Variable(QStringLiteral("UUID"), i18n("Generate a new UUID."), [](const QStringView&, KTextEditor::View*) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
#else
        // LEGACY
        QString uuid = QUuid::createUuid().toString();
        if (uuid.startsWith(QLatin1Char('{')))
            uuid.remove(0, 1);
        if (uuid.endsWith(QLatin1Char('}')))
            uuid.chop(1);
        return uuid;
#endif
    }));

}

void MacroExpander::registerExactMatch(const Variable & variable)
{
    m_exactMatches.push_back(variable);
}

void MacroExpander::registerPrefix(const Variable & variable)
{
    m_prefixMatches.push_back(variable);
}

bool MacroExpander::expandMacro(const QString& str, QStringList& ret)
{
    KTextEditor::View* view = m_view;
    if (!view)
        return false;

    // first try exact matches
    const auto it = std::find_if(m_exactMatches.cbegin(), m_exactMatches.cend(), [&str](const Variable& var) {
        return str == var.variable();
    });
    if (it != m_exactMatches.cend()) {
        ret += it->evaluate(str, view);
        return true;
    }

    // try prefix matching
    const int colonIndex = str.indexOf(QLatin1Char(':'));
    if (colonIndex >= 0) {
        const QString prefix = str.left(colonIndex + 1);
        const auto itPrefix = std::find_if(m_prefixMatches.cbegin(), m_prefixMatches.cend(), [&prefix](const Variable& var) {
            return prefix == var.variable();
        });
        if (itPrefix != m_prefixMatches.cend()) {
            ret += itPrefix->evaluate(str, view);
            return true;
        }
    }
    return false;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
