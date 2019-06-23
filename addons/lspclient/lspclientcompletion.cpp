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
        RETURN_CACHED_ICON("field");
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

static bool compare_match (const LSPCompletionItem & a, const LSPCompletionItem b)
{ return a.sortText < b.sortText; }


class LSPClientCompletionImpl : public LSPClientCompletion
{
    Q_OBJECT

    typedef LSPClientCompletionImpl self_type;

    QSharedPointer<LSPClientServerManager> m_manager;
    QSharedPointer<LSPClientServer> m_server;

    QList<LSPCompletionItem> m_matches;
    LSPClientServer::RequestHandle m_handle;

public:
    LSPClientCompletionImpl(QSharedPointer<LSPClientServerManager> manager)
        : LSPClientCompletion(nullptr), m_manager(manager), m_server(nullptr)
    {
    }

    void setServer(QSharedPointer<LSPClientServer> server) override
    { m_server = server; }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_matches.size()) {
            return QVariant();
        }

        const auto &match = m_matches.at(index.row());

        if (index.column() == KTextEditor::CodeCompletionModel::Name && role == Qt::DisplayRole) {
            // sigh, remove (leading) whitespace (looking at clangd here)
            // TODO better way, but it seems handy this way ??
            return QString(match.label.simplified() + QStringLiteral(" [") + match.detail + QStringLiteral("]"));;
        } else if (index.column() == KTextEditor::CodeCompletionModel::Icon && role == Qt::DecorationRole) {
            return kind_icon(match.kind);
        } else if (role == KTextEditor::CodeCompletionModel::CompletionRole) {
            return kind_property(match.kind);
        } else if (role == KTextEditor::CodeCompletionModel::ArgumentHintDepth) {
            // TODO use when also handling signatureHelp
            return 0; // match.depth;
        } else if (role == KTextEditor::CodeCompletionModel::InheritanceDepth) {
            // (ab)use depth to indicate sort order
            return index.row();
        }

        return QVariant();
    }

    bool shouldStartCompletion(KTextEditor::View *view, const QString &insertedText,
        bool userInsertion, const KTextEditor::Cursor &position) override
    {
        qCInfo(LSPCLIENT) << "should start " << userInsertion;

        if (!userInsertion) {
            return false;
        }

        bool complete = CodeCompletionModelControllerInterface::shouldStartCompletion(view,
            insertedText, userInsertion, position);

        complete = complete || insertedText.endsWith(QStringLiteral("("));
        complete = complete || insertedText.endsWith(QStringLiteral("::"));

        qCInfo(LSPCLIENT) << "should start " << complete;

        return complete;
    }

    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType it) override
    {
        Q_UNUSED(it)

        qCInfo(LSPCLIENT) << "completion invoked" << m_server;

        auto handler = [this] (const QList<LSPCompletionItem> compl) {
            beginResetModel();
            m_matches = compl;
            // sort as requested
            qSort(m_matches.begin(), m_matches.end(), compare_match);
            setRowCount(m_matches.size());
            qCInfo(LSPCLIENT) << "adding completions " << m_matches.size();
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
            m_manager->update(document);
            m_handle = m_server->documentCompletion(document->url(),
                {cursor.line(), cursor.column()}, handler);
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
        endResetModel();
    }
};

LSPClientCompletion*
LSPClientCompletion::new_(QSharedPointer<LSPClientServerManager> manager)
{
    return new LSPClientCompletionImpl(manager);
}

#include "lspclientcompletion.moc"
