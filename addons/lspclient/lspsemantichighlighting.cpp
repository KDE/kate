/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#include "lspsemantichighlighting.h"

#include <KTextEditor/MovingInterface>
#include <KTextEditor/View>

#include "semantic_tokens_legend.h"

void SemanticHighlighter::remove(KTextEditor::Document *doc)
{
    m_docUrlToResultId.remove(doc);

    auto it = m_docSemanticInfo.find(doc);
    if (it == m_docSemanticInfo.end()) {
        return;
    }

    auto &movingRanges = it->movingRanges;
    for (auto mr : movingRanges) {
        delete mr;
    }
    m_docSemanticInfo.remove(doc);
}

void SemanticHighlighter::insert(KTextEditor::Document *doc, const QString &resultId, const std::vector<uint32_t> &data)
{
    m_docUrlToResultId[doc] = resultId;
    TokensData &tokensData = m_docSemanticInfo[doc];
    tokensData.tokens = data;
}

/**
 * Handle semantic tokens edits
 */
void SemanticHighlighter::update(KTextEditor::Document *doc, const QString &resultId, uint32_t start, uint32_t deleteCount, const std::vector<uint32_t> &data)
{
    auto toks = m_docSemanticInfo.find(doc);
    if (toks == m_docSemanticInfo.end()) {
        return;
    }

    auto &existingTokens = toks->tokens;

    // replace
    if (deleteCount > 0) {
        existingTokens.erase(existingTokens.begin() + start, existingTokens.begin() + start + deleteCount);
    }
    existingTokens.insert(existingTokens.begin() + start, data.begin(), data.end());

    //     Update result Id
    m_docUrlToResultId[doc] = resultId;
}

void SemanticHighlighter::highlight(KTextEditor::Document *doc)
{
    Q_ASSERT(m_legend);

    if (!view || !m_legend) {
        return;
    }

    if (!doc) {
        qWarning() << "View doesn't have doc!";
        return;
    }

    auto miface = qobject_cast<KTextEditor::MovingInterface *>(doc);

    TokensData &semanticData = m_docSemanticInfo[doc];
    auto &movingRanges = semanticData.movingRanges;
    auto &data = semanticData.tokens;

    if (data.size() % 5 != 0) {
        qWarning() << "Bad data for doc: " << doc->url() << " skipping";
        return;
    }

    uint32_t currentLine = 0;
    uint32_t start = 0;

    int reusedRanges = 0;
    int newRanges = 0;

    for (size_t i = 0; i < data.size(); i += 5) {
        auto deltaLine = data.at(i);
        auto deltaStart = data.at(i + 1);
        auto len = data.at(i + 2);
        auto type = data.at(i + 3);
        auto mod = data.at(i + 4);
        (void)mod;

        currentLine += deltaLine;

        if (deltaLine == 0) {
            start += deltaStart;
        } else {
            start = deltaStart;
        }

        //         QString text = doc->line(currentLine);
        //         text = text.mid(start, len);

        KTextEditor::Range r(currentLine, start, currentLine, start + len);

        // Check if we have a moving ranges already available in the cache
        const auto index = i / 5;
        if (index < movingRanges.size()) {
            KTextEditor::MovingRange *range = movingRanges[index];
            if (range) {
                range->setRange(r);
                range->setAttribute(m_legend->attrForIndex(type));
                reusedRanges++;
                continue;
            }
        }

        KTextEditor::MovingRange *mr = miface->newMovingRange(r);
        mr->setAttribute(m_legend->attrForIndex(type));
        movingRanges.push_back(mr);
        newRanges++;

        //         std::cout << "Token: " << text.toStdString() << " => " << m_types.at(type).toStdString() << ", Line: {" << currentLine << ", " << deltaLine
        //         << "}\n";
    }

    /**
     * Invalid all extra ranges
     */
    int totalCreatedRanges = reusedRanges + newRanges;
    if (totalCreatedRanges < (int)movingRanges.size()) {
        std::for_each(movingRanges.begin() + totalCreatedRanges, movingRanges.end(), [](KTextEditor::MovingRange *mr) {
            mr->setRange(KTextEditor::Range::invalid());
        });
    }
}
