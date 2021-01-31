/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#include "lspsemantichighlighting.h"

#include <QColor>
#include <QString>
#include <QVector>

#include <KTextEditor/Editor>

#include <KSyntaxHighlighting/Theme>

#if KTEXTEDITOR_VERSION >= QT_VERSION_CHECK(5, 79, 0)

SemanticHighlighting::SemanticHighlighting(QObject *parent)
    : QObject(parent)
{
    themeChange(KTextEditor::Editor::instance());
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, &SemanticHighlighting::themeChange);
}

void SemanticHighlighting::themeChange(KTextEditor::Editor *e)
{
    if (!e) {
        return;
    }

    using Style = KSyntaxHighlighting::Theme::TextStyle;

    const auto theme = e->theme();

    QColor f = QColor::fromRgba(theme.textColor(Style::Function));
    QColor fs = QColor::fromRgba(theme.selectedTextColor(Style::Function));
    if (!fixedAttrs[0]) {
        fixedAttrs[0] = new KTextEditor::Attribute();
    }
    fixedAttrs[0]->setForeground(f);
    fixedAttrs[0]->setSelectedForeground(fs);
    fixedAttrs[0]->setFontBold(theme.isBold(Style::Function));
    fixedAttrs[0]->setFontItalic(theme.isItalic(Style::Function));

    if (!fixedAttrs[1]) {
        fixedAttrs[1] = new KTextEditor::Attribute();
    }
    {
        // This is only for function parameter which are not
        // directly supported by themes so we load some hard
        // coded values here for some of the themes we have
        // and for others we just read the "Variable" text-style.
        QColor v;
        QColor vs;
        bool italic = false;
        static const char MonokaiVP[] = "#fd971f";
        static const char DraculaVP[] = "#ffb86c";
        static const char AyuDarkLightVP[] = "#a37acc";
        static const char AyuMirageVP[] = "#d4bfff";
        if (theme.name() == QStringLiteral("Monokai")) {
            v = QColor(MonokaiVP);
            vs = v;
            italic = true;
        } else if (theme.name() == QStringLiteral("Dracula")) {
            v = QColor(DraculaVP);
            vs = v;
            italic = true;
        } else if (theme.name() == QStringLiteral("ayu Light") || theme.name() == QStringLiteral("ayu Dark")) {
            v = QColor(AyuDarkLightVP);
            vs = v;
        } else if (theme.name() == QStringLiteral("ayu Mirage")) {
            v = QColor(AyuMirageVP);
            vs = v;
        } else {
            v = QColor::fromRgba(theme.textColor(Style::Variable));
            italic = theme.isItalic(Style::Variable);
            vs = QColor::fromRgba(theme.selectedTextColor(Style::Variable));
        }

        fixedAttrs[1]->setForeground(v);
        fixedAttrs[1]->setSelectedForeground(vs);
        fixedAttrs[1]->setFontBold(theme.isBold(Style::Variable));
        fixedAttrs[1]->setFontItalic(italic);
    }

    QColor c = QColor::fromRgba(theme.textColor(Style::Constant));
    QColor cs = QColor::fromRgba(theme.selectedTextColor(Style::Constant));
    if (!fixedAttrs[2]) {
        fixedAttrs[2] = new KTextEditor::Attribute();
    }
    fixedAttrs[2]->setForeground(c);
    fixedAttrs[2]->setSelectedForeground(cs);
    fixedAttrs[2]->setFontBold(theme.isBold(Style::Constant));
    fixedAttrs[2]->setFontItalic(theme.isItalic(Style::Constant));

    QColor k = QColor::fromRgba(theme.textColor(Style::Keyword));
    QColor ks = QColor::fromRgba(theme.selectedTextColor(Style::Keyword));
    if (!fixedAttrs[3]) {
        fixedAttrs[3] = new KTextEditor::Attribute();
    }
    fixedAttrs[3]->setForeground(k);
    fixedAttrs[3]->setSelectedForeground(ks);
    fixedAttrs[3]->setFontBold(theme.isBold(Style::Keyword));
    fixedAttrs[3]->setFontItalic(theme.isItalic(Style::Keyword));

    QColor cm = QColor::fromRgba(theme.textColor(Style::Comment));
    QColor cms = QColor::fromRgba(theme.selectedTextColor(Style::Comment));
    if (!fixedAttrs[4]) {
        fixedAttrs[4] = new KTextEditor::Attribute();
    }
    fixedAttrs[4]->setForeground(cm);
    fixedAttrs[4]->setSelectedForeground(cms);
    fixedAttrs[4]->setFontBold(theme.isBold(Style::Comment));
    fixedAttrs[4]->setFontItalic(theme.isItalic(Style::Comment));

    QColor p = QColor::fromRgba(theme.textColor(Style::Preprocessor));
    QColor ps = QColor::fromRgba(theme.selectedTextColor(Style::Preprocessor));
    if (!fixedAttrs[5]) {
        fixedAttrs[5] = new KTextEditor::Attribute();
    }
    fixedAttrs[5]->setForeground(p);
    fixedAttrs[5]->setSelectedForeground(ps);
    fixedAttrs[5]->setFontBold(theme.isBold(Style::Preprocessor));
    fixedAttrs[5]->setFontItalic(theme.isItalic(Style::Preprocessor));
}

void SemanticHighlighting::scopesToAttrVector(const QVector<QString> &scopes)
{
    m_scopes.clear();
    m_scopes.resize(scopes.size());

    int i = 0;
    for (const auto &scope : scopes) {
        if (scope == QStringLiteral("variable.other.cpp")) {
            m_scopes[i] = TokenType::variableOtherCpp;
        } else if (scope == QStringLiteral("variable.other.local.cpp")) {
            m_scopes[i] = TokenType::variableOtherLocalCpp;
        } else if (scope == QStringLiteral("variable.parameter.cpp")) {
            m_scopes[i] = TokenType::variableParameterCpp;
        } else if (scope == QStringLiteral("entity.name.function.cpp")) {
            m_scopes[i] = TokenType::entityNameFunctionCpp;
        } else if (scope == QStringLiteral("entity.name.function.method.cpp")) {
            m_scopes[i] = TokenType::entityNameFunctionMethodCpp;
        } else if (scope == QStringLiteral("entity.name.function.method.static.cpp")) {
            m_scopes[i] = TokenType::entityNameFunctionMethodStaticCpp;
        } else if (scope == QStringLiteral("variable.other.field.cpp")) {
            m_scopes[i] = TokenType::variableOtherFieldCpp;
        } else if (scope == QStringLiteral("variable.other.field.static.cpp")) {
            m_scopes[i] = TokenType::variableOtherFieldStaticCpp;
        } else if (scope == QStringLiteral("entity.name.type.class.cpp")) {
            m_scopes[i] = TokenType::entityNameTypeClassCpp;
        } else if (scope == QStringLiteral("entity.name.type.enum.cpp")) {
            m_scopes[i] = TokenType::entityNameTypeEnumCpp;
        } else if (scope == QStringLiteral("variable.other.enummember.cpp")) {
            m_scopes[i] = TokenType::variableOtherEnummemberCpp;
        } else if (scope == QStringLiteral("entity.name.type.typedef.cpp")) {
            m_scopes[i] = TokenType::entityNameTypeTypedefCpp;
        } else if (scope == QStringLiteral("entity.name.type.dependent.cpp")) {
            m_scopes[i] = TokenType::entityNameTypeDependentCpp;
        } else if (scope == QStringLiteral("entity.name.other.dependent.cpp")) {
            m_scopes[i] = TokenType::entityNameOtherDependentCpp;
        } else if (scope == QStringLiteral("entity.name.namespace.cpp")) {
            m_scopes[i] = TokenType::entityNameNamespaceCpp;
        } else if (scope == QStringLiteral("entity.name.type.template.cpp")) {
            m_scopes[i] = TokenType::entityNameTypeTemplateCpp;
        } else if (scope == QStringLiteral("entity.name.type.concept.cpp")) {
            m_scopes[i] = TokenType::entityNameTypeConceptCpp;
        } else if (scope == QStringLiteral("storage.type.primitive.cpp")) {
            m_scopes[i] = TokenType::storageTypePrimitiveCpp;
        } else if (scope == QStringLiteral("entity.name.function.preprocessor.cpp")) {
            m_scopes[i] = TokenType::entityNameFunctionPreprocessorCpp;
        } else if (scope == QStringLiteral("meta.disabled")) {
            m_scopes[i] = TokenType::metaDisabled;
        }
        i++;
    }

    // load attributes
    refresh();
}

void SemanticHighlighting::refresh()
{
    sharedAttrs.clear();
    sharedAttrs.resize(m_scopes.size());
    for (size_t i = 0; i < m_scopes.size(); ++i) {
        switch (m_scopes.at(i)) {
        case variableParameterCpp:
            sharedAttrs[i] = fixedAttrs[1];
            break;
        case entityNameFunctionCpp:
        case entityNameFunctionMethodCpp:
        case entityNameFunctionMethodStaticCpp:
            sharedAttrs[i] = fixedAttrs[0];
            break;
        case entityNameFunctionPreprocessorCpp:
            sharedAttrs[i] = fixedAttrs[5];
            break;
        case entityNameTypeClassCpp:
        case entityNameTypeEnumCpp:
        case entityNameNamespaceCpp:
            sharedAttrs[i] = fixedAttrs[3];
            break;
        case variableOtherEnummemberCpp:
            sharedAttrs[i] = fixedAttrs[2];
            break;
        case metaDisabled:
            sharedAttrs[i] = fixedAttrs[4];
            break;
        default:
            // unused
            //            case variableOtherCpp:
            //            case variableOtherLocalCpp:
            //            case variableOtherFieldCpp:
            //            case variableOtherFieldStaticCpp:
            //            case entityNameTypeTypedefCpp:
            //            case entityNameTypeDependentCpp:
            //            case entityNameOtherDependentCpp:
            //            case entityNameTypeTemplateCpp:
            //            case entityNameTypeConceptCpp:
            //            case storageTypePrimitiveCpp:
            sharedAttrs[i] = {};
        }
    }
}

#endif
