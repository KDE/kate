/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "lspclientcompletion.h"
#include "lspclientplugin.h"
#include "lspclientprotocol.h"
#include "lspclientutils.h"

#include "lspclient_debug.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QIcon>

#include <algorithm>
#include <utility>

#include <drawing_utils.h>

static KTextEditor::CodeCompletionModel::CompletionProperty kind_property(LSPCompletionItemKind kind)
{
    using CompletionProperty = KTextEditor::CodeCompletionModel::CompletionProperty;
    auto p = CompletionProperty::NoProperty;

    switch (kind) {
    case LSPCompletionItemKind::Method:
    case LSPCompletionItemKind::Function:
    case LSPCompletionItemKind::Constructor:
        p = CompletionProperty::Function;
        break;
    case LSPCompletionItemKind::Variable:
        p = CompletionProperty::Variable;
        break;
    case LSPCompletionItemKind::Class:
    case LSPCompletionItemKind::Interface:
        p = CompletionProperty::Class;
        break;
    case LSPCompletionItemKind::Struct:
        p = CompletionProperty::Class;
        break;
    case LSPCompletionItemKind::Module:
        p = CompletionProperty::Namespace;
        break;
    case LSPCompletionItemKind::Enum:
    case LSPCompletionItemKind::EnumMember:
        p = CompletionProperty::Enum;
        break;
    default:
        break;
    }
    return p;
}

struct LSPClientCompletionItem : public LSPCompletionItem {
    int argumentHintDepth = 0;
    QString prefix;
    QString postfix;
    int start = 0;
    int len = 0;

    LSPClientCompletionItem(const LSPCompletionItem &item)
        : LSPCompletionItem(item)
    {
        // transform for later display
        // sigh, remove (leading) whitespace (looking at clangd here)
        // could skip the [] if empty detail, but it is a handy watermark anyway ;-)
        label = QString(label.simplified() + QLatin1String(" [") + detail.simplified() + QStringLiteral("]"));
    }

    LSPClientCompletionItem(const LSPSignatureInformation &sig, int activeParameter, const QString &_sortText)
    {
        argumentHintDepth = 1;
        documentation = sig.documentation;
        label = sig.label;
        sortText = _sortText;

        // transform into prefix, name, suffix if active
        if (activeParameter >= 0 && activeParameter < sig.parameters.length()) {
            const auto &param = sig.parameters.at(activeParameter);
            if (param.start >= 0 && param.start < label.length() && param.end >= 0 && param.end < label.length() && param.start < param.end) {
                start = param.start;
                len = param.end - param.start;
                prefix = label.mid(0, param.start);
                postfix = label.mid(param.end);
                label = label.mid(param.start, param.end - param.start);
            }
        }
    }
};

/**
 * Helper class that caches the completion icons
 */
class CompletionIcons : public QObject
{
    Q_OBJECT

public:
    CompletionIcons()
        : QObject(KTextEditor::Editor::instance())
        , classIcon(QIcon::fromTheme(QStringLiteral("code-class")))
        , blockIcon(QIcon::fromTheme(QStringLiteral("code-block")))
        , funcIcon(QIcon::fromTheme(QStringLiteral("code-function")))
        , varIcon(QIcon::fromTheme(QStringLiteral("code-variable")))
        , enumIcon(QIcon::fromTheme(QStringLiteral("enum")))
    {
        auto e = KTextEditor::Editor::instance();
        QObject::connect(e, &KTextEditor::Editor::configChanged, this, [this](KTextEditor::Editor *e) {
            colorIcons(e);
        });
        colorIcons(e);
    }

    QIcon iconForKind(LSPCompletionItemKind kind) const
    {
        switch (kind) {
        case LSPCompletionItemKind::Method:
        case LSPCompletionItemKind::Function:
        case LSPCompletionItemKind::Constructor:
            return funcIcon;
        case LSPCompletionItemKind::Variable:
            return varIcon;
        case LSPCompletionItemKind::Class:
        case LSPCompletionItemKind::Interface:
        case LSPCompletionItemKind::Struct:
            return classIcon;
        case LSPCompletionItemKind::Module:
            return blockIcon;
        case LSPCompletionItemKind::Field:
        case LSPCompletionItemKind::Property:
            // align with symbolview
            return varIcon;
        case LSPCompletionItemKind::Enum:
        case LSPCompletionItemKind::EnumMember:
            return enumIcon;
        default:
            break;
        }
        return QIcon();
    }

private:
    void colorIcons(KTextEditor::Editor *e)
    {
        using KSyntaxHighlighting::Theme;
        auto theme = e->theme();
        auto varColor = QColor::fromRgba(theme.textColor(Theme::Variable));
        varIcon = Utils::colorIcon(varIcon, varColor);

        auto typeColor = QColor::fromRgba(theme.textColor(Theme::DataType));
        classIcon = Utils::colorIcon(classIcon, typeColor);

        auto enColor = QColor::fromRgba(theme.textColor(Theme::Constant));
        enumIcon = Utils::colorIcon(enumIcon, enColor);

        auto funcColor = QColor::fromRgba(theme.textColor(Theme::Function));
        funcIcon = Utils::colorIcon(funcIcon, funcColor);

        auto blockColor = QColor::fromRgba(theme.textColor(Theme::Import));
        blockIcon = Utils::colorIcon(blockIcon, blockColor);
    }

private:
    QIcon classIcon;
    QIcon blockIcon;
    QIcon funcIcon;
    QIcon varIcon;
    QIcon enumIcon;
};

static bool compare_match(const LSPCompletionItem &a, const LSPCompletionItem &b)
{
    return a.sortText < b.sortText;
}

class LSPClientCompletionImpl : public LSPClientCompletion
{
    Q_OBJECT

    typedef LSPClientCompletionImpl self_type;

    std::shared_ptr<LSPClientServerManager> m_manager;
    std::shared_ptr<LSPClientServer> m_server;
    bool m_selectedDocumentation = false;
    bool m_signatureHelp = true;
    bool m_complParens = true;
    bool m_autoImport = true;

    QList<QChar> m_triggersCompletion;
    QList<QChar> m_triggersSignature;
    bool m_triggerSignature = false;
    bool m_triggerCompletion = false;

    QList<LSPClientCompletionItem> m_matches;
    LSPClientServer::RequestHandle m_handle, m_handleSig;

public:
    LSPClientCompletionImpl(std::shared_ptr<LSPClientServerManager> manager)
        : LSPClientCompletion(nullptr)
        , m_manager(std::move(manager))
        , m_server(nullptr)
    {
    }

    void setServer(std::shared_ptr<LSPClientServer> server) override
    {
        m_server = server;
        if (m_server) {
            const auto &caps = m_server->capabilities();
            m_triggersCompletion = caps.completionProvider.triggerCharacters;
            m_triggersSignature = caps.signatureHelpProvider.triggerCharacters;
        } else {
            m_triggersCompletion.clear();
            m_triggersSignature.clear();
        }
    }

    void setSelectedDocumentation(bool s) override
    {
        m_selectedDocumentation = s;
    }

    void setSignatureHelp(bool s) override
    {
        m_signatureHelp = s;
    }

    void setCompleteParens(bool s) override
    {
        m_complParens = s;
    }

    void setAutoImport(bool s) override
    {
        m_autoImport = s;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_matches.size()) {
            return QVariant();
        }

        const auto &match = m_matches.at(index.row());
        static CompletionIcons *icons = new CompletionIcons;

        if (role == Qt::DisplayRole) {
            if (index.column() == KTextEditor::CodeCompletionModel::Name) {
                return match.label;
            } else if (index.column() == KTextEditor::CodeCompletionModel::Prefix) {
                return match.prefix;
            } else if (index.column() == KTextEditor::CodeCompletionModel::Postfix) {
                return match.postfix;
            }
        } else if (role == Qt::DecorationRole && index.column() == KTextEditor::CodeCompletionModel::Icon) {
            return icons->iconForKind(match.kind);
        } else if (role == KTextEditor::CodeCompletionModel::CompletionRole) {
            return kind_property(match.kind);
        } else if (role == KTextEditor::CodeCompletionModel::ArgumentHintDepth) {
            return match.argumentHintDepth;
        } else if (role == KTextEditor::CodeCompletionModel::InheritanceDepth) {
            // (ab)use depth to indicate sort order
            return index.row();
        } else if (role == KTextEditor::CodeCompletionModel::IsExpandable) {
            return !match.documentation.value.isEmpty();
        } else if (role == KTextEditor::CodeCompletionModel::ExpandingWidget && !match.documentation.value.isEmpty()) {
            // probably plaintext, but let's show markdown as-is for now
            // FIXME better presentation of markdown
            return match.documentation.value;
        } else if (role == KTextEditor::CodeCompletionModel::ItemSelected && !match.argumentHintDepth && !match.documentation.value.isEmpty()
                   && m_selectedDocumentation) {
            return match.documentation.value;
        } else if (role == KTextEditor::CodeCompletionModel::CustomHighlight && match.argumentHintDepth > 0) {
            if (index.column() != Name || match.len == 0)
                return {};
            QTextCharFormat boldFormat;
            boldFormat.setFontWeight(QFont::Bold);
            const QVariantList highlighting{
                QVariant(0),
                QVariant(match.len),
                boldFormat,
            };
            return highlighting;
        } else if (role == CodeCompletionModel::HighlightingMethod && match.argumentHintDepth > 0) {
            return QVariant(HighlightMethod::CustomHighlighting);
        }

        return QVariant();
    }

    bool shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position) override
    {
        qCInfo(LSPCLIENT) << "should start " << userInsertion << insertedText;

        if (!userInsertion || !m_server || insertedText.isEmpty()) {
            if (!insertedText.isEmpty() && m_triggersSignature.contains(insertedText.back())) {
                m_triggerSignature = true;
                return true;
            }
            return false;
        }

        // covers most already ...
        bool complete = CodeCompletionModelControllerInterface::shouldStartCompletion(view, insertedText, userInsertion, position);
        QChar lastChar = insertedText.at(insertedText.size() - 1);

        m_triggerSignature = false;
        complete = complete || m_triggersCompletion.contains(lastChar);
        m_triggerCompletion = complete;
        if (m_triggersSignature.contains(lastChar)) {
            complete = true;
            m_triggerSignature = true;
        }
        return complete;
    }

    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType it) override
    {
        Q_UNUSED(it)

        qCInfo(LSPCLIENT) << "completion invoked" << m_server.get();

        const bool userInvocation = it == UserInvocation;
        if (userInvocation && range.isEmpty() && m_signatureHelp) {
            // If this is a user invocation (ctrl-space), check the last non-space char for sig help trigger
            QChar c;
            int i = range.start().column() - 1;
            int ln = range.start().line();
            for (; i >= 0; --i) {
                c = view->document()->characterAt(KTextEditor::Cursor(ln, i));
                if (!c.isSpace()) {
                    break;
                }
            }
            m_triggerSignature = m_triggersSignature.contains(c);
        }

        // maybe use WaitForReset ??
        // but more complex and already looks good anyway
        auto handler = [this](const QList<LSPCompletionItem> &completion) {
            beginResetModel();
            qCInfo(LSPCLIENT) << "adding completions " << completion.size();
            // purge all existing completion items
            m_matches.erase(std::remove_if(m_matches.begin(),
                                           m_matches.end(),
                                           [](const LSPClientCompletionItem &ci) {
                                               return ci.argumentHintDepth == 0;
                                           }),
                            m_matches.end());
            for (const auto &item : completion) {
                m_matches.push_back(item);
            }
            std::stable_sort(m_matches.begin(), m_matches.end(), compare_match);
            setRowCount(m_matches.size());
            endResetModel();
        };

        auto sigHandler = [this](const LSPSignatureHelp &sig) {
            beginResetModel();
            qCInfo(LSPCLIENT) << "adding signatures " << sig.signatures.size();
            int index = 0;
            m_matches.erase(std::remove_if(m_matches.begin(),
                                           m_matches.end(),
                                           [](const LSPClientCompletionItem &ci) {
                                               return ci.argumentHintDepth == 1;
                                           }),
                            m_matches.end());
            for (const auto &item : sig.signatures) {
                int sortIndex = 10 + index;
                int active = -1;
                if (index == sig.activeSignature) {
                    sortIndex = 0;
                    active = sig.activeParameter;
                }
                // trick active first, others after that
                m_matches.push_back({item, active, QString(QStringLiteral("%1").arg(sortIndex, 3, 10))});
                ++index;
            }
            std::stable_sort(m_matches.begin(), m_matches.end(), compare_match);
            setRowCount(m_matches.size());
            endResetModel();
        };

        beginResetModel();
        m_matches.clear();
        auto document = view->document();
        if (m_server && document) {
            // the default range is determined based on a reasonable identifier (word)
            // which is generally fine and nice, but let's pass actual cursor position
            // (which may be within this typical range)
            auto position = view->cursorPosition();
            auto cursor = qMax(range.start(), qMin(range.end(), position));
            m_manager->update(document, false);

            if (m_triggerCompletion || userInvocation) {
                m_handle = m_server->documentCompletion(document->url(), {cursor.line(), cursor.column()}, this, handler);
            }

            if (m_signatureHelp && m_triggerSignature) {
                m_handleSig = m_server->signatureHelp(document->url(), {cursor.line(), cursor.column()}, this, sigHandler);
            }
        }
        setRowCount(m_matches.size());
        endResetModel();
    }

    /**
     * @brief return next char *after* the range
     */
    static QChar peekNextChar(KTextEditor::Document *doc, const KTextEditor::Range &range)
    {
        return doc->characterAt(KTextEditor::Cursor(range.end().line(), range.end().column()));
    }

    // parses lsp snippets
    // returns the column where cursor should be after completion and the text to insert
    std::pair<int, QString> stripSnippetMarkers(const QString &snip) const
    {
#define C(c) QLatin1Char(c)
        QString ret;
        ret.reserve(snip.size());
        int bracket = 0;
        int lastSnippetMarkerPos = -1;
        for (auto i = snip.begin(), end = snip.end(); i != end; ++i) {
            const bool prevSlash = i > snip.begin() && *(i - 1) == C('\\');
            if (!prevSlash && *i == C('$') && i + 1 != end && *(i + 1) == C('{')) {
                if (i + 2 != end && (i + 2)->isDigit()) {
                    // its ${1:
                    auto j = i + 2;
                    // eat through digits
                    while (j->isDigit()) {
                        ++j;
                    }
                    if (*j == C(':')) {
                        bracket++;
                        // skip forward
                        i = j;
                    }
                } else {
                    // simple "${"
                    ++i;
                    bracket++;
                }
            } else if (!prevSlash && *i == C('$') && i + 1 != end && (i + 1)->isDigit()) { // $0, $1 => we dont support multiple cursor pos
                ++i;
                // eat through the digits
                while (i->isDigit()) {
                    ++i;
                }
                --i; // one step back to the last valid char
            } else if (bracket > 0 && *i == C('}')) {
                bracket--;
                if (bracket == 0 && lastSnippetMarkerPos == -1) {
                    lastSnippetMarkerPos = ret.size();
                }
            } else if (bracket == 0) { // if we are in "real text", add it
                ret += *i;
            }
        }

#undef C
        return {lastSnippetMarkerPos, ret};
    }

    void executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const override
    {
        if (index.row() >= m_matches.size()) {
            return;
        }

        QChar next = peekNextChar(view->document(), word);
        const auto item = m_matches.at(index.row());
        QString matching = m_matches.at(index.row()).insertText;
        // if there is already a '"' or >, remove it, this happens with #include "xx.h"
        if ((next == QLatin1Char('"') && matching.endsWith(QLatin1Char('"'))) || (next == QLatin1Char('>') && matching.endsWith(QLatin1Char('>')))) {
            matching.chop(1);
        }

        // If the server sent a CompletionItem.textEdit.range and that range's start
        // is different than what we have, perfer the server. This leads to better
        // completion because the server might be supplying items for a bigger range than
        // just the current word under cursor.
        const auto textEditRange = item.textEdit.range;
        auto rangeToReplace = word;
        if (textEditRange.isValid()
            && textEditRange.start() < word.start()
            // only do this if the text to insert is the same as TextEdit.newText
            && m_matches.at(index.row()).insertText == m_matches.at(index.row()).textEdit.newText) {
            rangeToReplace.setStart(textEditRange.start());
        }

        // NOTE: view->setCursorPosition() will invalidate the matches, so we save the
        // additionalTextEdits before setting cursor-possition
        const auto additionalTextEdits = m_matches.at(index.row()).additionalTextEdits;
        if (m_complParens) {
            const auto [col, textToInsert] = stripSnippetMarkers(matching);
            qCInfo(LSPCLIENT) << "original text: " << matching << ", snippet markers removed; " << textToInsert;
            view->document()->replaceText(rangeToReplace, textToInsert);
            // if the text is same, don't do any work
            if (col >= 0 && textToInsert != matching) {
                KTextEditor::Cursor p{rangeToReplace.start()};
                int column = p.column();
                int count = 0;
                // can be multiline text
                for (auto c : textToInsert) {
                    if (count == col) {
                        break;
                    }
                    if (c == QLatin1Char('\n')) {
                        p.setLine(p.line() + 1);
                        // line changed reset column
                        column = 0;
                        count++;
                        continue;
                    }
                    count++;
                    column++;
                }
                p.setColumn(column);
                view->setCursorPosition(p);
            }
        } else {
            view->document()->replaceText(rangeToReplace, matching);
        }

        if (m_autoImport) {
            // re-use util to apply edits
            // (which takes care to use moving range, etc)
            if (!additionalTextEdits.isEmpty()) {
                applyEdits(view->document(), nullptr, additionalTextEdits);
            } else if (!item.data.isNull() && m_server->capabilities().completionProvider.resolveProvider) {
                QPointer<KTextEditor::Document> doc = view->document();
                auto h = [doc](const LSPCompletionItem &c) {
                    if (doc && !c.additionalTextEdits.isEmpty()) {
                        applyEdits(doc, nullptr, c.additionalTextEdits);
                    }
                };
                m_server->documentCompletionResolve(item, this, h);
            }
        }
    }

    void aborted(KTextEditor::View *view) override
    {
        Q_UNUSED(view);
        beginResetModel();
        m_matches.clear();
        m_handle.cancel();
        m_handleSig.cancel();
        m_triggerSignature = false;
        endResetModel();
    }
};

LSPClientCompletion *LSPClientCompletion::new_(std::shared_ptr<LSPClientServerManager> manager)
{
    return new LSPClientCompletionImpl(std::move(manager));
}

#include "lspclientcompletion.moc"
#include "moc_lspclientcompletion.cpp"
