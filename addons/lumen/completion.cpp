/*
 * Copyright 2014-2015  David Herberth kde@dav1d.de
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "completion.h"
#include <QtCore/QRegularExpression>
#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/CodeCompletionModelControllerInterface>
#include <klocalizedstring.h>


LumenCompletionModel::LumenCompletionModel(QObject* parent, DCD* dcd)
    : CodeCompletionModel(parent)
{
    m_dcd = dcd;
}

LumenCompletionModel::~LumenCompletionModel()
{

}

bool LumenCompletionModel::shouldStartCompletion(View* view, const QString& insertedText, bool userInsertion, const Cursor& position)
{
    bool complete = KTextEditor::CodeCompletionModelControllerInterface::shouldStartCompletion(
        view, insertedText, userInsertion, position
    );

    complete = complete || insertedText.endsWith(QStringLiteral("(")); // calltip
    complete = complete || insertedText.endsWith(QStringLiteral("import ")); // import

    return complete;
}

void LumenCompletionModel::completionInvoked(View* view, const Range& range, CodeCompletionModel::InvocationType invocationType)
{
    Q_UNUSED(invocationType);
    KTextEditor::Document* document = view->document();

    KTextEditor::Cursor cursor = range.end();
    KTextEditor::Cursor cursorEnd = document->documentEnd();
    KTextEditor::Range range0c = KTextEditor::Range(0, 0, cursor.line(), cursor.column());
    KTextEditor::Range rangece = KTextEditor::Range(cursor.line(), cursor.column(),
                                                    cursorEnd.line(), cursorEnd.column());
    QString text0c = document->text(range0c, false);
    QByteArray utf8 = text0c.toUtf8();
    int offset = utf8.length();
    utf8.append(document->text(rangece, false).toUtf8());

    m_data = m_dcd->complete(utf8, offset);
    setRowCount(m_data.completions.length());

    setHasGroups(false);
}


void LumenCompletionModel::executeCompletionItem(View* view, const Range& word, const QModelIndex& index) const
{
    QModelIndex sibling = index.sibling(index.row(), Name);
    Document *document = view->document();

    document->replaceText(word, data(sibling).toString());

    int crole = data(sibling, CompletionRole).toInt();
    if (crole & Function) {
        KTextEditor::Cursor cursor = view->cursorPosition();
        document->insertText(cursor, QStringLiteral("()"));
        view->setCursorPosition(Cursor(cursor.line(), cursor.column()+1));
    }
}

QVariant LumenCompletionModel::data(const QModelIndex& index, int role) const
{
    DCDCompletionItem item = m_data.completions[index.row()];

    switch (role)
    {
        case Qt::DecorationRole:
        {
            if(index.column() == Icon) {
                return item.icon();
            }
            break;
        }
        case Qt::DisplayRole:
        {
            if(item.type == DCDCompletionItemType::Calltip) {
                QRegularExpression funcRE(QStringLiteral("^\\s*(\\w+)\\s+(\\w+\\s*\\(.*\\))\\s*$"));
                QStringList matches = funcRE.match(item.name).capturedTexts();

                switch(index.column()) {
                    case Prefix: return matches[1];
                    case Name: return matches[2];
                }
            } else {
                if(index.column() == Name) {
                    return item.name;
                }
            }
            break;
        }
        case CompletionRole:
        {
            int p = NoProperty;
            switch (item.type) {
                case DCDCompletionItemType::FunctionName: p |= Function; break;
                case DCDCompletionItemType::VariableName: p |= Variable; break;
                default: break;
            }
            return p;
        }
        case BestMatchesCount:
        {
            return 5;
        }
        case ArgumentHintDepth:
        {
            if(item.type == DCDCompletionItemType::Calltip) {
                return 1;
            }
            break;
        }
        case GroupRole:
        {
            break;
        }
        case IsExpandable:
        {
            // I like the green arrow
            return true;
        }
        case ExpandingWidget:
        {
            // TODO well implementation in DCD is missing
            return QVariant();
        }
    }

    return QVariant();
}
