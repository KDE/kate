/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "lspsemantichighlighting.h"
#include "lspclientprotocol.h"
#include "lspclientservermanager.h"
#include "semantic_tokens_legend.h"

#include <KTextEditor/MovingInterface>
#include <KTextEditor/View>

SemanticHighlighter::SemanticHighlighter(QSharedPointer<LSPClientServerManager> serverManager, QObject *parent)
    : QObject(parent)
    , m_serverManager(std::move(serverManager))
{
    m_requestTimer.setSingleShot(true);
    m_requestTimer.connect(&m_requestTimer, &QTimer::timeout, this, [this]() {
        doSemanticHighlighting_impl(m_currentView);
    });
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

void SemanticHighlighter::doSemanticHighlighting(KTextEditor::View *view, bool textChanged)
{
    // start the timer
    // We dont send the request directly because then there can be too many requests
    // which leads to too much load on the server and client/server getting out of sync.
    m_currentView = view;
    if (textChanged) {
        m_requestTimer.start(1000);
    } else {
        // This is not a textChange, its either the user scrolled or view changed etc
        m_requestTimer.start(40);
    }
}

void SemanticHighlighter::doSemanticHighlighting_impl(KTextEditor::View *view)
{
    if (!view) {
        return;
    }

    auto server = m_serverManager->findServer(view);
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
        // clang-format off
        connect(doc,
                SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
                this,
                SLOT(remove(KTextEditor::Document*)),
                Qt::UniqueConnection);
        connect(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)), this, SLOT(remove(KTextEditor::Document*)), Qt::UniqueConnection);
        // clang-format on
    }

    if (caps.semanticTokenProvider.range) {
        connect(view, &KTextEditor::View::verticalScrollPositionChanged, this, &SemanticHighlighter::semanticHighlightRange, Qt::UniqueConnection);
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

void SemanticHighlighter::semanticHighlightRange(KTextEditor::View *view, const KTextEditor::Cursor &)
{
    doSemanticHighlighting(view, false);
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
    auto oldRanges = std::move(movingRanges);

    for (size_t i = 0; i < data.size(); i += 5) {
        const uint32_t deltaLine = data[i];
        const uint32_t deltaStart = data[i + 1];
        const uint32_t len = data[i + 2];
        const uint32_t type = data[i + 3];
        // auto mod = data[i + 4];

        currentLine += deltaLine;

        if (deltaLine == 0) {
            start += deltaStart;
        } else {
            start = deltaStart;
        }

        // QString text = doc->line(currentLine);
        // text = text.mid(start, len);

        auto attribute = legend->attributeForTokenType(type);
        if (!attribute) {
            continue;
        }

        KTextEditor::Range r(currentLine, start, currentLine, start + len);
        using MovingRangePtr = std::unique_ptr<KTextEditor::MovingRange>;
        // Check if we have a moving range for 'r' already available
        auto it = std::lower_bound(oldRanges.begin(), oldRanges.end(), r, [](const MovingRangePtr &mr, KTextEditor::Range r) {
            // null range is considered less
            // it can be null because we 'move' the range from oldRanges whenever we find a matching
            return !mr || mr->toRange() < r;
        });

        MovingRangePtr range;
        if (it != oldRanges.end() && (*it) && (*it)->toRange() == r && (*it)->attribute() == attribute.constData()) {
            range = std::move(*it);
        } else {
            range.reset(miface->newMovingRange(r));
            range->setZDepth(-91000.0);
            range->setRange(r);
            range->setAttribute(std::move(attribute));
        }
        movingRanges.push_back(std::move(range));
    }
}
