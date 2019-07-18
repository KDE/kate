/***************************************************************************
 *   Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>              *
 *   Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>          *
 *   Copyright (C) 2019 by Mark Nauwelaerts <mark.nauwelaerts@gmail.com>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "lspclientcompletion.h"
#include "lspclientplugin.h"

#include "lspclient_debug.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QIcon>
#include <QUrl>

#include <algorithm>

#define RETURN_CACHED_ICON(name) \
{ \
    static QIcon icon(QIcon::fromTheme(QStringLiteral(name))); \
    return icon; \
}

static QIcon
kind_icon(LSPCompletionItemKind kind)
{
    switch (kind)
    {
    case LSPCompletionItemKind::Method:
    case LSPCompletionItemKind::Function:
    case LSPCompletionItemKind::Constructor:
        RETURN_CACHED_ICON("code-function")
    case LSPCompletionItemKind::Variable:
        RETURN_CACHED_ICON("code-variable")
    case LSPCompletionItemKind::Class:
    case LSPCompletionItemKind::Interface:
    case LSPCompletionItemKind::Struct:
        RETURN_CACHED_ICON("code-class");
    case LSPCompletionItemKind::Module:
        RETURN_CACHED_ICON("code-block");
    case LSPCompletionItemKind::Field:
    case LSPCompletionItemKind::Property:
        // align with symbolview
        RETURN_CACHED_ICON("code-variable");
    case LSPCompletionItemKind::Enum:
    case LSPCompletionItemKind::EnumMember:
        RETURN_CACHED_ICON("enum");
    default:
        break;
    }
    return QIcon();
}

static KTextEditor::CodeCompletionModel::CompletionProperty
kind_property(LSPCompletionItemKind kind)
{
    using CompletionProperty = KTextEditor::CodeCompletionModel::CompletionProperty;
    auto p = CompletionProperty::NoProperty;

    switch (kind)
    {
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
        p =CompletionProperty::Namespace;
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

struct LSPClientCompletionItem : public LSPCompletionItem
{
    int argumentHintDepth = 0;
    QString prefix;
    QString postfix;

    LSPClientCompletionItem(const LSPCompletionItem & item)
        : LSPCompletionItem(item)
    {
        // transform for later display
        // sigh, remove (leading) whitespace (looking at clangd here)
        // could skip the [] if empty detail, but it is a handy watermark anyway ;-)
        label = QString(label.simplified() + QStringLiteral(" [") +
                        detail.simplified() + QStringLiteral("]"));
    }

    LSPClientCompletionItem(const LSPSignatureInformation & sig,
        int activeParameter, const QString & _sortText)
    {
        argumentHintDepth = 1;
        documentation = sig.documentation;
        label = sig.label;
        sortText = _sortText;
        // transform into prefix, name, suffix if active
        if (activeParameter >= 0 && activeParameter < sig.parameters.length()) {
            const auto& param = sig.parameters.at(activeParameter);
            if (param.start >= 0 && param.start < label.length() &&
                    param.end >= 0 && param.end < label.length() &&
                    param.start < param.end) {
                prefix = label.mid(0, param.start);
                postfix = label.mid(param.end);
                label = label.mid(param.start, param.end - param.start);
            }
        }
    }
};


static bool compare_match (const LSPCompletionItem & a, const LSPCompletionItem b)
{ return a.sortText < b.sortText; }


class LSPClientCompletionImpl : public LSPClientCompletion
{
    Q_OBJECT

    typedef LSPClientCompletionImpl self_type;

    QSharedPointer<LSPClientServerManager> m_manager;
    QSharedPointer<LSPClientServer> m_server;
    bool m_selectedDocumentation = false;

    QVector<QChar> m_triggersCompletion;
    QVector<QChar> m_triggersSignature;
    bool m_triggerSignature = false;

    QList<LSPClientCompletionItem> m_matches;
    LSPClientServer::RequestHandle m_handle, m_handleSig;

public:
    LSPClientCompletionImpl(QSharedPointer<LSPClientServerManager> manager)
        : LSPClientCompletion(nullptr), m_manager(manager), m_server(nullptr)
    {
    }

    void setServer(QSharedPointer<LSPClientServer> server) override
    {
        m_server = server;
        if (m_server) {
            const auto& caps = m_server->capabilities();
            m_triggersCompletion = caps.completionProvider.triggerCharacters;
            m_triggersSignature = caps.signatureHelpProvider.triggerCharacters;
        } else {
            m_triggersCompletion.clear();
            m_triggersSignature.clear();
        }
    }

    virtual void setSelectedDocumentation(bool s) override
    { m_selectedDocumentation = s; }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_matches.size()) {
            return QVariant();
        }

        const auto &match = m_matches.at(index.row());

        if (role == Qt::DisplayRole) {
            if (index.column() == KTextEditor::CodeCompletionModel::Name) {
                return match.label;
            } else if (index.column() == KTextEditor::CodeCompletionModel::Prefix) {
                return match.prefix;
            } else if (index.column() == KTextEditor::CodeCompletionModel::Postfix) {
                return match.postfix;
            }
        } else if (role == Qt::DecorationRole && index.column() == KTextEditor::CodeCompletionModel::Icon) {
            return kind_icon(match.kind);
        } else if (role == KTextEditor::CodeCompletionModel::CompletionRole) {
            return kind_property(match.kind);
        } else if (role == KTextEditor::CodeCompletionModel::ArgumentHintDepth) {
            return match.argumentHintDepth;
        } else if (role == KTextEditor::CodeCompletionModel::InheritanceDepth) {
            // (ab)use depth to indicate sort order
            return index.row();
        } else if (role == KTextEditor::CodeCompletionModel::IsExpandable) {
            return !match.documentation.value.isEmpty();
        } else if (role == KTextEditor::CodeCompletionModel::ExpandingWidget &&
                   !match.documentation.value.isEmpty()) {
            // probably plaintext, but let's show markdown as-is for now
            // FIXME better presentation of markdown
            return match.documentation.value;
        } else if (role == KTextEditor::CodeCompletionModel::ItemSelected &&
                   !match.argumentHintDepth && !match.documentation.value.isEmpty() &&
                   m_selectedDocumentation) {
            return match.documentation.value;
        }

        return QVariant();
    }

    bool shouldStartCompletion(KTextEditor::View *view, const QString &insertedText,
        bool userInsertion, const KTextEditor::Cursor &position) override
    {
        qCInfo(LSPCLIENT) << "should start " << userInsertion << insertedText;

        if (!userInsertion || !m_server || insertedText.isEmpty()) {
            return false;
        }

        // covers most already ...
        bool complete = CodeCompletionModelControllerInterface::shouldStartCompletion(view,
            insertedText, userInsertion, position);
        QChar lastChar = insertedText.at(insertedText.count() - 1);

        m_triggerSignature = false;
        complete = complete  || m_triggersCompletion.contains(lastChar);
        if (m_triggersSignature.contains(lastChar)) {
            complete = true;
            m_triggerSignature = true;
        }

        return complete;
    }

    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType it) override
    {
        Q_UNUSED(it)

        qCInfo(LSPCLIENT) << "completion invoked" << m_server;

        // maybe use WaitForReset ??
        // but more complex and already looks good anyway
        auto handler = [this] (const QList<LSPCompletionItem> & compl) {
            beginResetModel();
            qCInfo(LSPCLIENT) << "adding completions " << compl.size();
            for (const auto & item : compl)
                m_matches.push_back(item);
            std::stable_sort(m_matches.begin(), m_matches.end(), compare_match);
            setRowCount(m_matches.size());
            endResetModel();
        };

        auto sigHandler = [this] (const LSPSignatureHelp & sig) {
            beginResetModel();
            qCInfo(LSPCLIENT) << "adding signatures " << sig.signatures.size();
            int index = 0;
            for (const auto & item : sig.signatures) {
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
            if (!m_triggerSignature) {
                m_handle = m_server->documentCompletion(document->url(),
                    {cursor.line(), cursor.column()}, this, handler);
            }
            m_handleSig = m_server->signatureHelp(document->url(),
                {cursor.line(), cursor.column()}, this, sigHandler);
        }
        setRowCount(m_matches.size());
        endResetModel();
    }

    void executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const override
    {
        if (index.row() < m_matches.size())
            view->document()->replaceText(word, m_matches.at(index.row()).insertText);
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

LSPClientCompletion*
LSPClientCompletion::new_(QSharedPointer<LSPClientServerManager> manager)
{
    return new LSPClientCompletionImpl(manager);
}

#include "lspclientcompletion.moc"
