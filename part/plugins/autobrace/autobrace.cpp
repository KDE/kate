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

#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <ktexteditor/configinterface.h>
#include <kmessagebox.h>
#include <klocalizedstring.h>

AutoBracePlugin *AutoBracePlugin::plugin = 0;

K_PLUGIN_FACTORY_DEFINITION(AutoBracePluginFactory,
        registerPlugin<AutoBracePlugin>("ktexteditor_autobrace");
        )
K_EXPORT_PLUGIN(AutoBracePluginFactory("ktexteditor_autobrace", "ktexteditor_plugins"))

AutoBracePlugin::AutoBracePlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent)
{
    Q_UNUSED(args);
    plugin = this;
}

AutoBracePlugin::~AutoBracePlugin()
{
    plugin = 0;
}

void AutoBracePlugin::addView(KTextEditor::View *view)
{
    if ( KTextEditor::ConfigInterface* confIface = qobject_cast< KTextEditor::ConfigInterface* >(view->document()) ) {
        QVariant brackets = confIface->configValue("auto-brackets");
        if ( brackets.isValid() && brackets.canConvert(QVariant::Bool) && brackets.toBool() ) {
            confIface->setConfigValue("auto-brackets", false);
            KMessageBox::information(view, i18n("The autobrace plugin supersedes the Kate-internal \"Auto Brackets\" feature.\n"
                                                "The setting was automatically disabled for this document."),
                                           i18n("Auto brackets feature disabled"), "AutoBraceSupersedesInformation");
        }
    }

    AutoBracePluginDocument *docplugin;

    // We're not storing the brace inserter by view but by document,
    // which makes signal connection and destruction a bit easier.
    if (m_docplugins.contains(view->document())) {
        docplugin = m_docplugins.value(view->document());
    }
    else {
        docplugin = new AutoBracePluginDocument(view->document());
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

AutoBracePluginDocument::AutoBracePluginDocument(KTextEditor::Document *document)
  : QObject(document), m_insertionLine(0), m_withSemicolon(false)
{
    connect(document, SIGNAL(textInserted(KTextEditor::Document*, const KTextEditor::Range&)),
            this, SLOT(slotTextInserted(KTextEditor::Document*, const KTextEditor::Range&)));
}

AutoBracePluginDocument::~AutoBracePluginDocument()
{
    disconnect(parent() /* == document */, 0, this, 0);
}

/**
 * Connected to KTextEditor::Document::textChanged() once slotTextInserted()
 * found a line with an opening brace. This takes care of inserting the new
 * line with its closing counterpart.
 */
void AutoBracePluginDocument::slotTextChanged(KTextEditor::Document *document) {
    // Disconnect from all signals as we insert stuff by ourselves.
    // In other words, this is in order to prevent infinite recursion.
    disconnect(document, 0, this, 0);

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
        document->insertLine(m_insertionLine + 1, m_indentation + "}" + (m_withSemicolon ? ";" : ""));

        document->endEditing();
        view->setCursorPosition(document->endOfLine(m_insertionLine));
    }
    m_insertionLine = 0;

    // Re-enable the textInserted() slot again.
    connect(document, SIGNAL(textInserted(KTextEditor::Document*, const KTextEditor::Range&)),
            this, SLOT(slotTextInserted(KTextEditor::Document*, const KTextEditor::Range&)));
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
    // Make sure we don't even try to handle other events than brace openers.
    if (document->text(range) != "\n") {
        return;
    }

    // Remember this position as insertion candidate.
    // We don't directly insert this here because of KatePart specifics:
    // a) Setting the cursor position crashes at this point, and
    // b) textChanged() only gets called once per edit operation, so we can
    //    ignore the same braces when they're being inserted via paste.
    if (isInsertionCandidate(document, range.start().line())) {
        m_insertionLine = range.end().line();
        connect(document, SIGNAL(textChanged(KTextEditor::Document*)),
                this, SLOT(slotTextChanged(KTextEditor::Document*)));
    }
    else {
        m_insertionLine = 0;
    }
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
    QStringList tokens;
    if ( line.contains("class") || line.contains("interface") || line.contains("struct") ) {
        tokens << "private" << "public" << "protected";
        if ( document->mode() == "C++" ) {
            tokens << "signals" << "Q_SIGNALS";
        } else {
            // PHP and potentially others
            tokens << "function";
        }
    }
    if ( line.contains("namespace", Qt::CaseInsensitive) ) {
        // C++ specific
        tokens << "class" << "struct";
    }
    QString forbiddenTokens = tokens.isEmpty() ? "" : "(?!" + tokens.join("|") + ")";

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
        if ( document->mode() == "C++" && document->line(openingBraceLine).indexOf(QRegExp("(?:class|struct)\\s+[^\\s]+\\s*\\{\\s*$")) != -1 ) {
            m_withSemicolon = true;
        } else {
            m_withSemicolon = false;
        }
    }
    return isCandidate;
}

#include "autobrace.moc"
