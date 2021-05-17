/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#ifndef LSP_SEMANTIC_HIGHLIGHTING_H
#define LSP_SEMANTIC_HIGHLIGHTING_H

#include <QHash>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVector>

#include <KTextEditor/Attribute>
#include <KTextEditor/MovingRange>
#include <KTextEditor/View>

class SemanticTokensLegend;
struct LSPSemanticTokensDelta;

class SemanticHighlighter
{
public:
    QString resultIdForDoc(KTextEditor::Document *doc) const
    {
        auto it = m_docResultId.find(doc);
        if (it != m_docResultId.end()) {
            return it->second;
        }
        return QString();
    }

    /**
     * Unregister a doc from highlighter and remove all its associated moving ranges and tokens
     */
    void remove(KTextEditor::Document *doc);

    void processTokens(const LSPSemanticTokensDelta &tokens, KTextEditor::View *view, const SemanticTokensLegend *legend);

private:
    /**
     * Does the actual highlighting
     */
    void highlight(KTextEditor::View *view, const SemanticTokensLegend *legend);

    /**
     * Insert tokens @p data for doc with @p url
     */
    void insert(KTextEditor::Document *doc, const QString &resultId, const std::vector<uint32_t> &data);

    /**
     * This function is more or less useless for Kate? Maybe because MovingRange already handles this for us
     *
     * An update or SemanticTokensEdit only arrives if you entered a new line or something trivial. If you insert new characters you
     * get a full new vector with new data which has to be replaced with the old and everything rehighlighted. This is the behaviour of
     * clangd, not sure about others.
     */
    void update(KTextEditor::Document *doc, const QString &resultId, uint32_t start, uint32_t deleteCount, const std::vector<uint32_t> &data);

    /**
     * A simple struct which holds the tokens recieved by server +
     * moving ranges that were created to highlight those tokens
     */
    struct TokensData {
        std::vector<uint32_t> tokens;
        std::vector<std::unique_ptr<KTextEditor::MovingRange>> movingRanges;
    };

    /**
     * token types specified in server caps. Uncomment for debuggin
     */
    //     QVector<QString> m_types;

    /**
     * Doc => result-id mapping
     */
    std::unordered_map<KTextEditor::Document *, QString> m_docResultId;

    /**
     * semantic token and moving range mapping for doc
     */
    std::unordered_map<KTextEditor::Document *, TokensData> m_docSemanticInfo;
};
#endif
