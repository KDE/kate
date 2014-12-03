/**
  * This file is part of the KDE libraries
  * Copyright (C) 2008 Jakob Petsovits <jpetso@gmx.at>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "autobrace.h"
#include "autobrace_config.h"

#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <ktexteditor/configinterface.h>
#include <kmessagebox.h>
#include <klocalizedstring.h>
#include <iostream>
#include <kconfiggroup.h>

AutoBracePlugin *AutoBracePlugin::plugin = 0;

namespace {

/**
 * Returns next character after specified text range in document.
 * @param document Current document.
 * @param position Text cursor position
 * @return Next character after text position
 */
QChar nextToken(KTextEditor::Document* document, const KTextEditor::Cursor& position)
{
    return document->character(position + KTextEditor::Cursor(0, 1));
}

/**
 * Returns previous character before specified text range in document.
 * @param document Current document.
 * @param position Text cursor position
 * @return Next character after text position
 */
QChar previousToken(KTextEditor::Document* document, const KTextEditor::Cursor& position)
{
    return document->character(position - KTextEditor::Cursor(0, 1));
}

}

K_PLUGIN_FACTORY_DEFINITION(AutoBracePluginFactory,
        registerPlugin<AutoBracePlugin>("ktexteditor_autobrace");
        registerPlugin<AutoBraceConfig>("ktexteditor_autobrace_config");
        )
K_EXPORT_PLUGIN(AutoBracePluginFactory("ktexteditor_autobrace", "ktexteditor_plugins"))

AutoBracePlugin::AutoBracePlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent), m_autoBrackets(true), m_autoQuotations(true)
{
    Q_UNUSED(args);
    plugin = this;

    readConfig();
}

AutoBracePlugin::~AutoBracePlugin()
{
    plugin = 0;
}

void AutoBracePlugin::addView(KTextEditor::View *view)
{
    AutoBracePluginDocument *docplugin;

    // We're not storing the brace inserter by view but by document,
    // which makes signal connection and destruction a bit easier.
    if (m_docplugins.contains(view->document())) {
        docplugin = m_docplugins.value(view->document());
    }
    else {
        // Create Editor plugin and assign options through reference
        docplugin = new AutoBracePluginDocument(view->document(),
                                                m_autoBrackets,
                                                m_autoQuotations);
        m_docplugins.insert(view->document(), docplugin);
    }
    // Shouldn't be necessary in theory, but for removeView() the document
    // might already be destroyed and removed. Also used as refcounter.
    m_documents.insert(view, view->document());
}

void AutoBracePlugin::removeView(KTextEditor::View *view)
{
    if (m_documents.contains(view))
    {
        KTextEditor::Document *document = m_documents.value(view);
        m_documents.remove(view);

        // Only detach from the document if it was the last view pointing to that.
        if (m_documents.keys(document).empty()) {
            AutoBracePluginDocument *docplugin = m_docplugins.value(document);
            m_docplugins.remove(document);
            delete docplugin;
        }
    }
}

void AutoBracePlugin::readConfig()
{
    KConfigGroup cg(KGlobal::config(), "AutoBrace Plugin");
    m_autoBrackets = cg.readEntry("autobrackets", true);
    m_autoQuotations = cg.readEntry("autoquotations", false);
}

void AutoBracePlugin::writeConfig()
{
    KConfigGroup cg(KGlobal::config(), "AutoBrace Plugin");
    cg.writeEntry("autobrackets", m_autoBrackets);
    cg.writeEntry("autoquotations", m_autoQuotations);
}

/// AutoBracePluginDocument

AutoBracePluginDocument::AutoBracePluginDocument(KTextEditor::Document* document, const bool& autoBrackets, const bool& autoQuotations)
  : QObject(document), m_insertionLine(0), m_withSemicolon(false),
    m_lastRange(KTextEditor::Range::invalid()), m_autoBrackets(autoBrackets), m_autoQuotations(autoQuotations)
{
    connect(document, SIGNAL(exclusiveEditStart(KTextEditor::Document*)),
            this, SLOT(disconnectSlots(KTextEditor::Document*)));
    connect(document, SIGNAL(exclusiveEditEnd(KTextEditor::Document*)),
            this, SLOT(connectSlots(KTextEditor::Document*)));

    connectSlots(document);
}

AutoBracePluginDocument::~AutoBracePluginDocument()
{
    disconnect(parent() /* == document */, 0, this, 0);
}

/**
 * (Re-)setups slots for AutoBracePluginDocument.
 * @param document Current document.
 */
void AutoBracePluginDocument::connectSlots(KTextEditor::Document *document)
{
    connect(document, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
            this, SLOT(slotTextInserted(KTextEditor::Document*,KTextEditor::Range)));
    connect(document, SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range)),
            this, SLOT(slotTextRemoved(KTextEditor::Document*,KTextEditor::Range)));
}

void AutoBracePluginDocument::disconnectSlots(KTextEditor::Document* document)
{
    disconnect(document, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
               this, SLOT(slotTextInserted(KTextEditor::Document*,KTextEditor::Range)));
    disconnect(document, SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range)),
               this, SLOT(slotTextRemoved(KTextEditor::Document*,KTextEditor::Range)));
    disconnect(document, SIGNAL(textChanged(KTextEditor::Document*)),
               this, SLOT(slotTextChanged(KTextEditor::Document*)));
}

/**
 * Connected to KTextEditor::Document::textChanged() once slotTextInserted()
 * found a line with an opening brace. This takes care of inserting the new
 * line with its closing counterpart.
 */
void AutoBracePluginDocument::slotTextChanged(KTextEditor::Document *document) {
    // Disconnect from all signals as we insert stuff by ourselves.
    // Prevent infinite recursion.
    disconnectSlots(document);

    // Make really sure that we want to insert the brace, paste guard and all.
    if (m_insertionLine != 0
        && m_insertionLine == document->activeView()->cursorPosition().line()
        && document->line(m_insertionLine).trimmed().isEmpty())
    {
        KTextEditor::View *view = document->activeView();
        document->startEditing();

        // If the document's View is a KateView then it's able to indent.
        // We hereby ignore the indenter and always indent correctly. (Sorry!)
        if (view->inherits("KateView")) {
            // Correctly indent the empty line. Magic!
            KTextEditor::Range lineRange(
                m_insertionLine, 0,
                m_insertionLine, document->lineLength(m_insertionLine)
            );
            document->replaceText(lineRange, m_indentation);

            connect(this, SIGNAL(indent()), view, SLOT(indent()));
            emit indent();
            disconnect(this, SIGNAL(indent()), view, SLOT(indent()));
        }
        // The line with the closing brace. (Inserted via insertLine() in order
        // to avoid space removal by potential indenters.)
        document->insertLine(m_insertionLine + 1, m_indentation + '}' + (m_withSemicolon ? ";" : ""));

        document->endEditing();
        view->setCursorPosition(document->endOfLine(m_insertionLine));
    }
    m_insertionLine = 0;

    // Re-enable the textInserted() slot again.
    connectSlots(document);
}

/**
 * Connected to KTextEditor::Documet::textRemoved. Allows to delete
 * an automatically inserted closing bracket if the opening counterpart
 * has been removed.
 */
void AutoBracePluginDocument::slotTextRemoved(KTextEditor::Document* document, const KTextEditor::Range& range)
{
    // If last range equals the deleted text range (last range
    // is last inserted bracket), we also delete the associated closing bracket.
    if (m_lastRange == range) {
        // avoid endless recursion
        disconnectSlots(document);

        // Delete the character at the same range because the opening
        // bracket has already been removed so the closing bracket
        // should now have been shifted to that same position
        if (range.isValid()) {
            document->removeText(range);
        }

        connectSlots(document);
    }
}

/**
 * Connected to KTextEditor::Document::textInserted(), which is emitted on all
 * insertion changes. Line text and line breaks are emitted separately by
 * KatePart, and pasted text gets the same treatment as manually entered text.
 * Because of that, we discard paste operations by only remembering the
 * insertion status for the last line that was entered.
 */
void AutoBracePluginDocument::slotTextInserted(KTextEditor::Document *document,
                                               const KTextEditor::Range& range)
{
    if (!range.onSingleLine() || range.columnWidth() != 1) {
        return;
    }
    const KTextEditor::Cursor position = range.start();
    const QChar insertedToken = document->character(position);

    // Fill brackets map matching opening and closing brackets.
    QMap<QChar,QChar> brackets;
    brackets['('] = ')';
    brackets['['] = ']';

    // latex wants {, too
    if (document->mode() == "LaTeX")
        brackets['{'] = '}';

    // List of Tokens after which an automatic bracket expanion
    // is allowed.
    const static QVector<QChar> allowedNextToken = QVector<QChar>()
        << ']' << ')' << ',' << '.' << ';' << '\n' << '\t' << ' ' << QChar();

    // An insertion operation cancels any last position removal
    // operation
    m_lastRange = KTextEditor::Range::invalid();

    // Make sure to handle only:
    // 1.) New lines after { (brace openers)
    // 2.) Opening braces like '(' and '['
    // 3.) Quotation marks like " and '

    // Handle brace openers
    if (insertedToken == '\n') {
        // Remember this position as insertion candidate.
        // We don't directly insert this here because of KatePart specifics:
        // a) Setting the cursor position crashes at this point, and
        // b) textChanged() only gets called once per edit operation, so we can
        //    ignore the same braces when they're being inserted via paste.
        if (isInsertionCandidate(document, position.line())) {
            m_insertionLine = position.line();
            connect(document, SIGNAL(textChanged(KTextEditor::Document*)),
                    this, SLOT(slotTextChanged(KTextEditor::Document*)));
        }
        else {
            m_insertionLine = 0;
        }
    }
    // Opening brackets (defined in ctor)
    else if (m_autoBrackets && brackets.contains(insertedToken)) {
        // Only insert auto closing brackets if current text position
        // is followed by one of the allowed next tokens.
        if (allowedNextToken.contains(nextToken(document, position))) {
            insertAutoBracket(document, range, brackets[insertedToken]);
        }

    }
    // Check whether closing brackets are allowed.
    // If a brace is not allowed remove it
    // and set the cursor to the position after that text position.
    // Bracket tests bases on this simple idea: A bracket can only be inserted
    // if it is NOT followed by the same bracket. This results in overwriting closing brackets.
    else if (m_autoBrackets && brackets.values().contains(insertedToken)) {
        if (nextToken(document, position) == insertedToken) {
            KTextEditor::Cursor saved = range.end();
            document->removeText(range);
            document->activeView()->setCursorPosition(saved);
        }
    }
    // Insert auto-quotation marks (if enabled). Same idea as with brackets
    // applies here: double quotation marks are eaten up and only inserted if not
    // followed by the same quoation mark. Additionally automatic quotation marks
    // are inserted only if NOT followed by a back slash (escaping character).
    else if (m_autoQuotations && (insertedToken == '\"' || insertedToken == '\'') && previousToken(document, position) != '\\') {
        const QChar next = nextToken(document, position);
        // Eat it if already there
        if (next == insertedToken) {
            KTextEditor::Cursor saved = range.end();
            document->removeText(range);
            document->activeView()->setCursorPosition(saved);
        }
        // Quotation marks only inserted if followed by one of the allowed
        // next tokens and the number of marks in the insertion line is even
        // (excluding the already inserted mark)
        else if (allowedNextToken.contains(next)
            && (document->line(position.line()).count(insertedToken) % 2) ) {
            insertAutoBracket(document, range, insertedToken);
        }
    }
}

/**
 * Automatically inserts closing bracket. Cursor
 * is placed in between the brackets.
 * @param document Current document.
 * @param range Inserted text range (by text-inserted slot)
 * @param brace Brace to insert
 */
void AutoBracePluginDocument::insertAutoBracket(KTextEditor::Document *document,
                                                const KTextEditor::Range& range,
                                                const QChar& brace) {
    // Disconnect Slots to avoid check for redundant closing brackets
    disconnectSlots(document);

    // Save range to allow following remove operation to
    // detect the corresponding closing bracket
    m_lastRange = range;

    KTextEditor::Cursor saved = range.end();
    // Depending on brace, insert corresponding closing brace.
    document->insertText(range.end(), brace);
    document->activeView()->setCursorPosition(saved);

    // Re-Enable insertion slot.
    connectSlots(document);
}

bool AutoBracePluginDocument::isInsertionCandidate(KTextEditor::Document *document, int openingBraceLine) {
    QString line = document->line(openingBraceLine);
    if (line.isEmpty() || !line.endsWith('{')) {
        return false;
    }

    // Get the indentation prefix.
    QRegExp rx("^(\\s+)");
    QString indentation = (rx.indexIn(line) == -1) ? "" : rx.cap(1);

    // Determine whether to insert a brace or not, depending on the indentation
    // of the upcoming (non-empty) line.
    bool isCandidate = true;
    QString indentationLength = QString::number(indentation.length());
    QString indentationLengthMinusOne = QString::number(indentation.length() - 1);

    ///TODO: make configurable
    // these tokens must not start a line that is used to get the correct indendation width
    QStringList forbiddenTokenList;
    if ( line.contains("class") || line.contains("interface") || line.contains("struct") ) {
        forbiddenTokenList  << "private" << "public" << "protected";
        if ( document->mode() == "C++" ) {
            forbiddenTokenList  << "signals" << "Q_SIGNALS";
        } else {
            // PHP and potentially others
            forbiddenTokenList  << "function";
        }
    }
    if ( (document->mode() == "C++" || document->mode() == "C") && line.contains("namespace", Qt::CaseInsensitive) ) {
        // C++ specific
        forbiddenTokenList  << "class" << "struct";
    }
    const QString forbiddenTokens = forbiddenTokenList.isEmpty() ? QLatin1String("") : QString(QLatin1String("(?!") + forbiddenTokenList.join(QLatin1String("|")) + QLatin1Char(')'));

    for (int i = openingBraceLine + 1; i < document->lines(); ++i)
    {
        line = document->line(i);
        if (line.trimmed().isEmpty()) {
            continue; // Empty lines are not a reliable source of information.
        }

        if (indentation.length() == 0) {
            // Inserting a brace is ok if there is a line (not starting with a
            // brace) without indentation.
            rx.setPattern("^(?=[^\\}\\s])"
                // But it's not OK if the line starts with one of our forbidden tokens.
                + forbiddenTokens
            );
        }
        else {
            rx.setPattern("^(?:"
                // Inserting a brace is ok if there is a closing brace with
                // less indentation than the opener line.
                "[\\s]{0," + indentationLengthMinusOne + "}\\}"
                "|"
                // Inserting a brace is ok if there is a line (not starting with a
                // brace) with less or similar indentation as the original line.
                "[\\s]{0," + indentationLength + "}(?=[^\\}\\s])"
                // But it's not OK if the line starts with one of our forbidden tokens.
                + forbiddenTokens +
                ")"
            );
        }

        if (rx.indexIn(line) == -1) {
            // There is already a brace, or the line is indented more than the
            // opener line (which means we expect a brace somewhere further down),
            // or we found a forbidden token.
            // So don't insert the brace, and just indent the line.
            isCandidate = false;
        }
        // Quit the loop - a non-empty line always leads to a definitive decision.
        break;
    }

    if (isCandidate) {
        m_indentation = indentation;
        // in C++ automatically add a semicolon after the closing brace when we create a new class/struct
        if ( (document->mode() == "C++" || document->mode() == "C")
                && document->line(openingBraceLine).indexOf(QRegExp("(?:class|struct|enum)\\s+[^\\s]+(\\s*[:,](\\s*((public|protected|private)\\s+)?[^\\s]+))*\\s*\\{\\s*$")) != -1 )
        {
            m_withSemicolon = true;
        } else {
            m_withSemicolon = false;
        }
    }
    return isCandidate;
}

#include "autobrace.moc"
