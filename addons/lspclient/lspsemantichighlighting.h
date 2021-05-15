#pragma once

#include <QHash>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVector>

#include <KTextEditor/Attribute>
#include <KTextEditor/MovingRange>
#include <KTextEditor/View>

class SemanticTokensLegend;

class SemanticHighlighter
{
public:
    ~SemanticHighlighter()
    {
        for (auto &info : m_docSemanticInfo) {
            qDeleteAll(info.movingRanges.begin(), info.movingRanges.end());
        }
    }

    /**
     * Does the actual highlighting
     */
    void highlight(const QUrl &url);

    /**
     * Insert tokens @p data for doc with @p url
     */
    void insert(const QUrl &url, const QString &resultId, const std::vector<uint32_t> &data);

    /**
     * Unregister a doc from highlighter and remove all its moving ranges and tokens
     */
    void remove(const QUrl &url);

    /**
     * This function is more or less useless for Kate? Maybe because MovingRange already handles this for us
     *
     * An update or SemanticTokensEdit only arrives if you entered a new line or something trivial. If you insert new characters you
     * get a full new vector with new data which has to be replaced with the old and everything rehighlighted. This is the behaviour of
     * clangd, not sure about others.
     */
    void update(const QUrl &url, const QString &resultId, uint32_t start, uint32_t deleteCount, const std::vector<uint32_t> &data);

    QString resultIdForDoc(const QUrl &url) const
    {
        return m_docUrlToResultId.value(url);
    }

    void setCurrentView(KTextEditor::View *v)
    {
        view = v;
    }

    void setLegend(const SemanticTokensLegend *legend)
    {
        m_legend = legend;
    }

private:
    /**
     * A simple struct which holds the tokens recieved by server +
     * moving ranges that were created to highlight those tokens
     */
    struct TokensData {
        std::vector<uint32_t> tokens;
        std::vector<KTextEditor::MovingRange *> movingRanges;
    };

    /**
     * The current active view
     */
    QPointer<KTextEditor::View> view;

    /**
     * token types specified in server caps. Uncomment for debuggin
     */
    //     QVector<QString> m_types;

    /**
     * Doc => result-id mapping
     */
    QHash<QUrl, QString> m_docUrlToResultId;

    /**
     * semantic token and moving range mapping for doc
     */
    QHash<QUrl, TokensData> m_docSemanticInfo;

    /**
     * Legend which is basically used to fetch color for a type
     */
    const SemanticTokensLegend *m_legend = nullptr;
};
