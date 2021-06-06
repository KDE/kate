/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#include "lspsemantichighlighting.h"
#include "lspclientprotocol.h"
#include "lspclientservermanager.h"
#include "semantic_tokens_legend.h"

#include <KTextEditor/MovingInterface>
#include <KTextEditor/MovingRange>
#include <KTextEditor/View>

SemanticHighlighter::SemanticHighlighter(QObject *parent)
    : QObject(parent)
{
}

static KTextEditor::Range getCurrentViewLinesRange(KTextEditor::View *view)
{
    Q_ASSERT(view);

    auto doc = view->document();
    auto first = view->firstDisplayedLine();
    auto last = view->lastDisplayedLine();
    auto lastLineLen = doc->line(last).size();
    return KTextEditor::Range(first, 0, last, lastLineLen);
}

void SemanticHighlighter::doSemanticHighlighting(KTextEditor::View *view, QSharedPointer<LSPClientServerManager> serverManager)
{
    if (!view) {
        return;
    }

    auto server = serverManager->findServer(view);
    if (!server) {
        return;
    }

    const auto &caps = server->capabilities();
    const bool serverSupportsSemHighlighting = caps.semanticTokenProvider.full || caps.semanticTokenProvider.fullDelta || caps.semanticTokenProvider.range;
    if (!serverSupportsSemHighlighting) {
        return;
    }

    auto doc = view->document();
    if (m_docResultId.count(doc) == 0) {
        connect(doc,
                SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document *)),
                this,
                SLOT(remove(KTextEditor::Document *)),
                Qt::UniqueConnection);
        connect(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document *)), this, SLOT(remove(KTextEditor::Document *)), Qt::UniqueConnection);
    }

    if (caps.semanticTokenProvider.range) {
        if (!m_docSemanticConnectedViews.insert(view).second) {
            // track vertical scrolling for this view
            QPointer<KTextEditor::View> v = view;
            connect(
                view,
                &KTextEditor::View::verticalScrollPositionChanged,
                this,
                [this, v, serverManager]() {
                    doSemanticHighlighting(v, serverManager);
                },
                Qt::UniqueConnection);

            // clean it up from our set after the view is gone
            connect(view, &KTextEditor::View::destroyed, this, [this](QObject *o) {
                auto view = static_cast<KTextEditor::View *>(o);
                m_docSemanticConnectedViews.erase(view);
            });
        }
    }

    //  m_semHighlightingManager.setTypes(server->capabilities().semanticTokenProvider.types);

    QPointer<KTextEditor::View> v = view;
    auto h = [this, v, server](const LSPSemanticTokensDelta &st) {
        if (v && server) {
            const auto legend = &server->capabilities().semanticTokenProvider.legend;
            processTokens(st, v, legend);
        }
    };

    if (caps.semanticTokenProvider.range) {
        server->documentSemanticTokensRange(doc->url(), getCurrentViewLinesRange(view), this, h);
    } else if (caps.semanticTokenProvider.fullDelta) {
        auto prevResultId = previousResultIdForDoc(doc);
        server->documentSemanticTokensFullDelta(doc->url(), prevResultId, this, h);
    } else {
        server->documentSemanticTokensFull(doc->url(), QString(), this, h);
    }
}

QString SemanticHighlighter::previousResultIdForDoc(KTextEditor::Document *doc) const
{
    auto it = m_docResultId.find(doc);
    if (it != m_docResultId.end()) {
        return it->second;
    }
    return QString();
}

void SemanticHighlighter::processTokens(const LSPSemanticTokensDelta &tokens, KTextEditor::View *view, const SemanticTokensLegend *legend)
{
    Q_ASSERT(view);

    for (const auto &semTokenEdit : tokens.edits) {
        update(view->document(), tokens.resultId, semTokenEdit.start, semTokenEdit.deleteCount, semTokenEdit.data);
    }

    if (!tokens.data.empty()) {
        insert(view->document(), tokens.resultId, tokens.data);
    }
    highlight(view, legend);
}

void SemanticHighlighter::remove(KTextEditor::Document *doc)
{
    m_docResultId.erase(doc);
    m_docSemanticInfo.erase(doc);
}

void SemanticHighlighter::insert(KTextEditor::Document *doc, const QString &resultId, const std::vector<uint32_t> &data)
{
    m_docResultId[doc] = resultId;
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

    auto &existingTokens = toks->second.tokens;

    // replace
    if (deleteCount > 0) {
        existingTokens.erase(existingTokens.begin() + start, existingTokens.begin() + start + deleteCount);
    }
    existingTokens.insert(existingTokens.begin() + start, data.begin(), data.end());

    //     Update result Id
    m_docResultId[doc] = resultId;
}

void SemanticHighlighter::highlight(KTextEditor::View *view, const SemanticTokensLegend *legend)
{
    Q_ASSERT(legend);

    if (!view || !legend) {
        return;
    }
    auto doc = view->document();
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

        // QString text = doc->line(currentLine);
        // text = text.mid(start, len);

        KTextEditor::Range r(currentLine, start, currentLine, start + len);

        // Check if we have a moving ranges already available in the cache
        const auto index = i / 5;
        if (index < movingRanges.size()) {
            auto &range = movingRanges[index];
            if (range) {
                range->setRange(r);
                range->setAttribute(legend->attributeForTokenType(type));
                reusedRanges++;
                continue;
            }
        }

        std::unique_ptr<KTextEditor::MovingRange> mr(miface->newMovingRange(r));
        mr->setAttribute(legend->attributeForTokenType(type));
        movingRanges.push_back(std::move(mr));
        newRanges++;

        // std::cout << "Token: " << text.toStdString() << " => " << m_types.at(type).toStdString() << ", Line: {" << currentLine << ", " << deltaLine
        //         << "}\n";
    }

    /**
     * Invalid all extra ranges
     */
    int totalCreatedRanges = reusedRanges + newRanges;
    if (totalCreatedRanges < (int)movingRanges.size()) {
        std::for_each(movingRanges.begin() + totalCreatedRanges, movingRanges.end(), [](const std::unique_ptr<KTextEditor::MovingRange> &mr) {
            mr->setRange(KTextEditor::Range::invalid());
        });
    }
}
