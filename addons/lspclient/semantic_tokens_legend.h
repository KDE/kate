#ifndef SEMANTIC_TOKENS_LEGEND
#define SEMANTIC_TOKENS_LEGEND

#include <QObject>

#include <KTextEditor/Attribute>

namespace KTextEditor
{
class Editor;
}

class SemanticTokensLegend : public QObject
{
    enum TokenType {
        Type,
        Class,
        Enum,
        Interface,
        Struct,
        TypeParameter,
        Parameter,
        Variable,
        Property,
        EnumMember,
        Event,
        Function,
        Method,
        Macro,
        Keyword,
        Modifier,
        Comment,
        String,
        Number,
        Regexp,
        Operator,
        Namespace
    };

public:
    explicit SemanticTokensLegend(QObject *parent = nullptr);

    /**
     * Called from LSP Server when capabilities are recieved
     */
    void initialize(const std::vector<QString> &types);

    KTextEditor::Attribute::Ptr attrForIndex(int idx) const
    {
        if (idx >= totalTokenTypes) {
            return {};
        }
        return sharedAttrs.at(idx);
    }

private:
    Q_SLOT void themeChange(KTextEditor::Editor *e);
    void refresh(const std::vector<TokenType> &m_tokenTypes);

    int totalTokenTypes;
    std::vector<KTextEditor::Attribute::Ptr> sharedAttrs;
    KTextEditor::Attribute::Ptr fixedAttrs[7];
};

#endif
