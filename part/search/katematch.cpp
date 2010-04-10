/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katematch.h"

#include "kateregexpsearch.h"
#include "katedocument.h"


KateMatch::KateMatch(KateDocument *document, KTextEditor::Search::SearchOptions options)
    : m_document(document)
    , m_options(options)
{
    m_resultRanges.append(KTextEditor::Range::invalid());
}


KTextEditor::Range KateMatch::searchText(const KTextEditor::Range &range, const QString &pattern)
{
    m_resultRanges = m_document->searchText(range, pattern, m_options);

    return m_resultRanges[0];
}


KTextEditor::Range KateMatch::replace(const QString &replacement, bool blockMode, int replacementCounter)
{
    // Placeholders depending on search mode
    const bool usePlaceholders = m_options.testFlag(KTextEditor::Search::Regex) ||
                                 m_options.testFlag(KTextEditor::Search::EscapeSequences);

    const QString finalReplacement = usePlaceholders ? buildReplacement(replacement, blockMode, replacementCounter)
                                                     : replacement;

    // Track replacement operation
    KTextEditor::SmartRange *const afterReplace = m_document->newSmartRange(range());
    afterReplace->setInsertBehavior(KTextEditor::SmartRange::ExpandLeft | KTextEditor::SmartRange::ExpandRight);

    blockMode = blockMode && !range().onSingleLine();
    m_document->replaceText(range(), finalReplacement, blockMode);

    const KTextEditor::Range result = *afterReplace;
    delete afterReplace;

    return result;
}


KTextEditor::Range KateMatch::range() const
{
    if (m_resultRanges.size() > 0)
        return m_resultRanges[0];

    return KTextEditor::Range::invalid();
}


bool KateMatch::isEmpty() const
{
    return range().isEmpty();
}


bool KateMatch::isValid() const
{
    return range().isValid();
}


QString KateMatch::buildReplacement(const QString &replacement, bool blockMode, int replacementCounter, QStringList *resultRangesReplacement) {
    const bool use_ranges = !resultRangesReplacement;
    const int MIN_REF_INDEX = 0;
    const int MAX_REF_INDEX = use_ranges ? m_resultRanges.count() - 1 : resultRangesReplacement->count() - 1;

    QList<ReplacementPart> parts;
    const bool REPLACEMENT_GOODIES = true;
    KateRegExpSearch::escapePlaintext(replacement, &parts, REPLACEMENT_GOODIES);

    QString output;
    ReplacementPart::Type caseConversion = ReplacementPart::KeepCase;
    foreach (const ReplacementPart &curPart, parts) {
        switch (curPart.type) {
        case ReplacementPart::Reference:
            if ((curPart.index < MIN_REF_INDEX) || (curPart.index > MAX_REF_INDEX)) {
                // Insert just the number to be consistent with QRegExp ("\c" becomes "c")
                output.append(QString::number(curPart.index));
            } else {
                QString content;
                bool doref=false;
                if (use_ranges) {
                    const KTextEditor::Range captureRange = m_resultRanges[curPart.index];
                    if (captureRange.isValid()) {
                        // Copy capture content
                        content = m_document->text(captureRange, blockMode);
                        doref=true;
                    }
                } else {
                    content=resultRangesReplacement->at (curPart.index);
                    doref=true;
                }

                if (doref) {
                    switch (caseConversion) {
                    case ReplacementPart::UpperCase:
                        // Copy as uppercase
                        output.append(content.toUpper());
                        break;

                    case ReplacementPart::UpperCaseFirst:
                        if (content.length()>0) {
                            output.append(content.at(0).toUpper());
                            output.append(content.mid(1));
                            caseConversion=ReplacementPart::KeepCase;
                        }
                        break;

                    case ReplacementPart::LowerCase:
                        // Copy as lowercase
                        output.append(content.toLower());
                        break;

                    case ReplacementPart::LowerCaseFirst:
                        if (content.length()>0) {
                            output.append(content.at(0).toLower());
                            output.append(content.mid(1));
                            caseConversion=ReplacementPart::KeepCase;
                        }
                        break;

                    case ReplacementPart::KeepCase: // FALLTHROUGH
                    default:
                        // Copy unmodified
                        output.append(content);
                        break;

                    }
                }
            }
            break;

        case ReplacementPart::UpperCase: // FALLTHROUGH
        case ReplacementPart::UpperCaseFirst: // FALLTHROUGH
        case ReplacementPart::LowerCase: // FALLTHROUGH
        case ReplacementPart::LowerCaseFirst: // FALLTHROUGH
        case ReplacementPart::KeepCase:
            caseConversion = curPart.type;
            break;

        case ReplacementPart::Counter:
            {
                // Zero padded counter value
                const int minWidth = curPart.index;
                const int number = replacementCounter;
                output.append(QString("%1").arg(number, minWidth, 10, QLatin1Char('0')));
            }
            break;

        case ReplacementPart::Text: // FALLTHROUGH
        default:
            switch (caseConversion) {
            case ReplacementPart::UpperCase:
                // Copy as uppercase
                output.append(curPart.text.toUpper());
                break;

            case ReplacementPart::UpperCaseFirst:
                if (curPart.text.length()>0) {
                    output.append(curPart.text.at(0).toUpper());
                    output.append(curPart.text.mid(1));
                    caseConversion=ReplacementPart::KeepCase;
                }
                break;

            case ReplacementPart::LowerCase:
                // Copy as lowercase
                output.append(curPart.text.toLower());
                break;

            case ReplacementPart::LowerCaseFirst:
                if (curPart.text.length()>0) {
                    output.append(curPart.text.at(0).toUpper());
                    output.append(curPart.text.mid(1));
                    caseConversion=ReplacementPart::KeepCase;
                }
                break;

            case ReplacementPart::KeepCase: // FALLTHROUGH
            default:
                // Copy unmodified
                output.append(curPart.text);
                break;

            }
            break;

        }
    }

    return output;
}


// kate: space-indent on; indent-width 4; replace-tabs on;
