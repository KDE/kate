#include "lspsemantichighlighting.h"

#include <KTextEditor/MovingInterface>
#include <KTextEditor/View>

#include "semantic_tokens_legend.h"

void SemanticHighlighter::remove(const QUrl &url)
{
    m_docUrlToResultId.remove(url);
    auto &data = m_docSemanticInfo[url];
    auto &movingRanges = data.movingRanges;
    for (auto mr : movingRanges) {
        delete mr;
    }
    m_docUrlToResultId.remove(url);
}

void SemanticHighlighter::insert(const QUrl &url, const QString &resultId, const std::vector<uint32_t> &data)
{
    m_docUrlToResultId[url] = resultId;
    TokensData &tokensData = m_docSemanticInfo[url];
    tokensData.tokens = data;
}

/**
 * Handle semantic tokens edits
 */
void SemanticHighlighter::update(const QUrl &url, const QString &resultId, uint32_t start, uint32_t deleteCount, const std::vector<uint32_t> &data)
{
    auto toks = m_docSemanticInfo.find(url);
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
    m_docUrlToResultId[url] = resultId;
}

void SemanticHighlighter::highlight(const QUrl &url)
{
    Q_ASSERT(m_legend);

    if (!view || !m_legend) {
        return;
    }

    auto doc = view->document();
    if (!doc) {
        qWarning() << "View doesn't have doc!";
        return;
    }

    auto miface = qobject_cast<KTextEditor::MovingInterface *>(doc);

    TokensData &semanticData = m_docSemanticInfo[url];
    auto &movingRanges = semanticData.movingRanges;
    auto &data = semanticData.tokens;

    if (data.size() % 5 != 0) {
        qWarning() << "Bad data for doc: " << url << " skipping";
        return;
    }

    int currentLine = 0;
    int start = 0;

    int reusedRanges = 0;
    int newRanges = 0;

    for (size_t i = 0; i < data.size(); i += 5) {
        int deltaLine = data.at(i);
        int deltaStart = data.at(i + 1);
        int len = data.at(i + 2);
        int type = data.at(i + 3);
        int mod = data.at(i + 4);
        (void)mod;

        currentLine += deltaLine;

        if (deltaLine == 0) {
            start += deltaStart;
        } else {
            start = deltaStart;
        }

        QString text = doc->line(currentLine);
        text = text.mid(start, len);

        KTextEditor::Range r(currentLine, start, currentLine, start + len);

        // Check if we have a moving ranges already available in the cache
        const auto index = i / 5;
        if (index < movingRanges.size()) {
            auto &range = movingRanges[index];
            range->setRange(r);
            range->setAttribute(m_legend->attrForIndex(type));
            reusedRanges++;
            continue;
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
