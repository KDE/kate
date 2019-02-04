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

#include <QDir>
#include <QFileInfo>
#include <QDate>
#include <QTime>
#include <QUuid>

using StringFunction = QString (*)(KTextEditor::View* view);
class Variable
{
public:
    Variable(const QString& variable, const QString& description, StringFunction func)
        : m_variable(variable)
        , m_description(description)
        , m_function(func)
    {}

    QString variable() const
    {
        return m_variable;
    }

    QString description() const
    {
        return m_description;
    }

    QString value(KTextEditor::View * view) const
    {
        return m_function(view);
    }

private:
    QString m_variable;
    QString m_description;
    StringFunction m_function;
};

MacroExpander::MacroExpander(KTextEditor::View* view)
    : KWordMacroExpander()
    , m_view(view)
    , m_url(view ? view->document()->url().toLocalFile() : QString())
{
}

bool MacroExpander::expandMacro(const QString& str, QStringList& ret)
{
    KTextEditor::View* view = m_view;
    if (!view)
        return false;

    KTextEditor::Document* doc = view->document();
    const QUrl url = doc->url();

    if (str == QStringLiteral("CurrentDocument:FileBaseName")) // Current document: File base name without path and suffix.
        ret += QFileInfo(m_url).baseName();
    else if (str == QStringLiteral("CurrentDocument:FileExtension")) // Current document: File extension.
        ret += QFileInfo(m_url).completeSuffix();
    else if (str == QStringLiteral("CurrentDocument:FileName")) // Current document: File name without path.
        ret += QFileInfo(m_url).fileName();
    else if (str == QStringLiteral("CurrentDocument:FilePath")) // Current document: Full path including file name
        ret += QFileInfo(m_url).absoluteFilePath();
    else if (str == QStringLiteral("CurrentDocument:Text")) // Current document: Contents of entire file
        ret += doc->text();
    else if (str == QStringLiteral("CurrentDocument:Path")) // Current document: Full path excluding file name
        ret += QFileInfo(m_url).absolutePath();
    else if (str == QStringLiteral("CurrentDocument:NativeFilePath")) // Current document: Full path including file name, with native path separator (backslash on Windows).
        ret += m_url.isEmpty() ? QString() : QDir::toNativeSeparators(QFileInfo(m_url).absoluteFilePath());
    else if (str == QStringLiteral("CurrentDocument:NativePath")) // Current document: Full path excluding file name, with native path separator (backslash on Windows).
        ret += m_url.isEmpty() ? QString() : QDir::toNativeSeparators(QFileInfo(m_url).absolutePath());

    else if (str == QStringLiteral("CurrentDocument:Cursor:Line")) // Line number of the text cursor position in current document (starts with 0).
        ret += QString::number(view->cursorPosition().line());
    else if (str == QStringLiteral("CurrentDocument:Cursor:Column")) // Column number of the text cursor position in current document (starts with 0).
        ret += QString::number(view->cursorPosition().column());
    else if (str == QStringLiteral("CurrentDocument:Cursor:XPos")) // X component in global screen coordinates of the cursor position.
        ret += QString::number(view->mapToGlobal(view->cursorPositionCoordinates()).x());
    else if (str == QStringLiteral("CurrentDocument:Cursor:YPos")) // Y component in global screen coordinates of the cursor position.
        ret += QString::number(view->mapToGlobal(view->cursorPositionCoordinates()).y());
    else if (str == QStringLiteral("CurrentDocument:Selection:Text")) // Current document: Full path excluding file name.
        ret += view->selectionText();
    else if (str == QStringLiteral("CurrentDocument:Selection:StartLine"))
        ret += QString::number(view->selectionRange().start().line());
    else if (str == QStringLiteral("CurrentDocument:Selection:StartColumn"))
        ret += QString::number(view->selectionRange().start().column());
    else if (str == QStringLiteral("CurrentDocument:Selection:EndLine"))
        ret += QString::number(view->selectionRange().end().line());
    else if (str == QStringLiteral("CurrentDocument:Selection:EndColumn"))
        ret += QString::number(view->selectionRange().end().column());
    else if (str == QStringLiteral("CurrentDocument:RowCount"))
        ret += QString::number(view->document()->lines());

    else if (str == QStringLiteral("Date:Locale")) // The current date in current locale format.
        ret += QDate::currentDate().toString(Qt::DefaultLocaleShortDate);
    else if (str == QStringLiteral("Date:ISO")) // The current date (ISO).
        ret += QDate::currentDate().toString(Qt::ISODate);
    else if (str.startsWith(QStringLiteral("Date:"))) // The current date (QDate formatstring).
        ret += QDate::currentDate().toString(QStringView(str.constData() + 5, str.length() - 5));

    else if (str == QStringLiteral("Time:Locale")) // The current time in current locale format.
        ret += QTime::currentTime().toString(Qt::DefaultLocaleShortDate);
    else if (str == QStringLiteral("Time:ISO")) // The current time (ISO).
        ret += QTime::currentTime().toString(Qt::ISODate);
    else if (str.startsWith(QStringLiteral("Time:"))) // The current time (QTime formatstring).
        ret += QTime::currentTime().toString(QStringView(str.constData() + 5, str.length() - 5));

    else if (str.startsWith(QStringLiteral("ENV:"))) // Access environment variables.
        ret += QString::fromLocal8Bit(qgetenv(str.rightRef(str.size() - 4).toLocal8Bit().constData()));
    else if (str.startsWith(QStringLiteral("JS:"))) // Evaluate simple JavaScript statements. The statements may not contain '{' nor '}' characters.
        ; // FIXME
    else if (str == QStringLiteral("UUID")) // Generate a new UUID.
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        ret += QUuid::createUuid().toString(QUuid::WithoutBraces);
#else
        { // LEGACY
            QString uuid = QUuid::createUuid().toString();
            if (uuid.startsWith(QLatin1Char('{')))
                uuid.remove(0, 1);
            if (uuid.endsWith(QLatin1Char('}')))
                uuid.chop(1);
            ret += uuid;
        }
#endif
    else
        return false;

    return true;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
