/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#ifndef SEMANTICTOKENMAP_H
#define SEMANTICTOKENMAP_H

#include <QObject>

#include <vector>

#include <KTextEditor/Attribute>

namespace KTextEditor {
class Editor;
}

class SemanticHighlighting : public QObject
{
    Q_OBJECT

public:
    explicit SemanticHighlighting(QObject* parent = nullptr);

    enum TokenType : quint16
    {
        variableOtherCpp = 0,
        variableOtherLocalCpp,
        variableParameterCpp,
        entityNameFunctionCpp,
        entityNameFunctionMethodCpp,
        entityNameFunctionMethodStaticCpp,
        variableOtherFieldCpp,
        variableOtherFieldStaticCpp,
        entityNameTypeClassCpp,
        entityNameTypeEnumCpp,
        variableOtherEnummemberCpp,
        entityNameTypeTypedefCpp,
        entityNameTypeDependentCpp,
        entityNameOtherDependentCpp,
        entityNameNamespaceCpp,
        entityNameTypeTemplateCpp,
        entityNameTypeConceptCpp,
        storageTypePrimitiveCpp,
        entityNameFunctionPreprocessorCpp,
        metaDisabled
    };

    Q_SLOT void themeChange(KTextEditor::Editor* e);

    KTextEditor::Attribute::Ptr attrForScope(quint16 scopeidx) const
    {
        if (scopeidx > m_scopes.size())
            return {};
        return sharedAttrs.at(scopeidx);
    }

    void scopesToAttrVector(const QVector<QString>& scopes);

    void refresh();

    void clear()
    {
        m_scopes.clear();
        sharedAttrs.clear();
    }

private:
    std::vector<TokenType> m_scopes;
    std::vector<KTextEditor::Attribute::Ptr> sharedAttrs;
    KTextEditor::Attribute::Ptr fixedAttrs[6];
};

#endif // SEMANTICTOKENMAP_H
