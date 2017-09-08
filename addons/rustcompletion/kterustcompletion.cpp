/***************************************************************************
 *   Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>              *
 *   Copyright (C) 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>          *
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
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

#include "kterustcompletion.h"
#include "kterustcompletionplugin.h"

#include <QProcessEnvironment>
#include <QTemporaryFile>
#include <QTextStream>

#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/View>

KTERustCompletion::KTERustCompletion(KTERustCompletionPlugin *plugin)
    : KTextEditor::CodeCompletionModel(nullptr)
    , m_plugin(plugin)
{
}

KTERustCompletion::~KTERustCompletion()
{
}

QVariant KTERustCompletion::data(const QModelIndex &index, int role) const
{

    if (!index.isValid() || index.row() >= m_matches.size()) {
        return QVariant();
    }

    const CompletionMatch &match = m_matches.at(index.row());

    if (index.column() == KTextEditor::CodeCompletionModel::Name && role == Qt::DisplayRole) {
        return match.text;
    } else if (index.column() == KTextEditor::CodeCompletionModel::Icon && role == Qt::DecorationRole) {
        return match.icon;
    } else if (role == KTextEditor::CodeCompletionModel::CompletionRole) {
        return (int)match.type;
    } else if (role == KTextEditor::CodeCompletionModel::ArgumentHintDepth) {
        return match.depth;
    } else if (role == KTextEditor::CodeCompletionModel::MatchQuality) {
        if (index.row() < 10) {
            return 10 - index.row();
        } else {
            return 0;
        }
    } else if (role == KTextEditor::CodeCompletionModel::BestMatchesCount) {
        return (index.row() < 10) ? 1 : 0;
    }

    return QVariant();
}

bool KTERustCompletion::shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position)
{
    if (!userInsertion) {
        return false;
    }

    if (insertedText.isEmpty()) {
        return false;
    }

    bool complete = CodeCompletionModelControllerInterface::shouldStartCompletion(view,
        insertedText, userInsertion, position);

    complete = complete || insertedText.endsWith(QStringLiteral("("));
    complete = complete || insertedText.endsWith(QStringLiteral("."));
    complete = complete || insertedText.endsWith(QStringLiteral("::"));

    return complete;
}

void KTERustCompletion::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType it)
{
    Q_UNUSED(it)

    beginResetModel();

    m_matches = getMatches(view->document(), Complete, range.end());

    setRowCount(m_matches.size());
    setHasGroups(false);

    endResetModel();
}

void KTERustCompletion::aborted(KTextEditor::View *view)
{
    Q_UNUSED(view);

    beginResetModel();

    m_matches.clear();

    endResetModel();
}

QList<CompletionMatch> KTERustCompletion::getMatches(const KTextEditor::Document *document, MatchAction action, const KTextEditor::Cursor &position)
{
    QList<CompletionMatch> matches;

    if (!m_plugin->configOk()) {
        return matches;
    }

    QTemporaryFile file;

    if (file.open()) {
        {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << document->text();
        }

        QProcess proc;

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
        env.insert(QStringLiteral("RUST_SRC_PATH"), m_plugin->rustSrcPath().toLocalFile());
        proc.setProcessEnvironment(env);

        QStringList args;
        args << (action == Complete ? QStringLiteral("complete") : QStringLiteral("find-definition"));
        args << QString::number(position.line() + 1);
        args << QString::number(position.column());
        args << file.fileName();

        proc.start(m_plugin->racerCmd(), args, QIODevice::ReadOnly);

        if (proc.waitForFinished(1000)) {
            const QString &output = QString::fromUtf8(proc.readAllStandardOutput());

            QStringList lines(output.split(QStringLiteral("\n"), QString::SkipEmptyParts));

            foreach(QString line, lines) {
                if (line.startsWith(QStringLiteral("MATCH "))) {
                    line = line.mid(6);

                    CompletionMatch match;
                    match.text = line.section(QStringLiteral(","), 0, 0);

                    const QString &type = line.section(QStringLiteral(","), 4, 4);
                    match.depth = (type == QStringLiteral("StructField")) ? 1 : 0;
                    addType(match, type);

                    QString path = line.section(QStringLiteral(","), 3, 3);

                    if (path == file.fileName()) {
                        match.url = document->url();
                    } else {
                        match.url = QUrl::fromLocalFile(path);
                    }

                    bool ok = false;

                    int row = line.section(QStringLiteral(","), 1, 1).toInt(&ok);
                    if (ok) match.line = row - 1;

                    int col = line.section(QStringLiteral(","), 2, 2).toInt(&ok);
                    if (ok) match.col = col;

                    matches.append(match);

                    if (action == FindDefinition) {
                        break;
                    }
                }
            }
        }
    }

    return matches;
}

void KTERustCompletion::addType(CompletionMatch &match, const QString &type)
{
    if (type == QStringLiteral("Function")) {
        match.type = CodeCompletionModel::Function;
        match.icon = QIcon::fromTheme(QStringLiteral("code-function"));
    } else if (type == QStringLiteral("Struct")) {
        match.type = CodeCompletionModel::Struct;
        match.icon = QIcon::fromTheme(QStringLiteral("code-class"));
    } else if (type == QStringLiteral("StructField")) {
        match.icon = QIcon::fromTheme(QStringLiteral("field"));
    } else if (type == QStringLiteral("Trait")) {
        match.type = CodeCompletionModel::Class;
        match.icon = QIcon::fromTheme(QStringLiteral("code-class"));
    } else if (type == QStringLiteral("Module")) {
        match.type = CodeCompletionModel::Namespace;
        match.icon = QIcon::fromTheme(QStringLiteral("field"));
    } else if (type == QStringLiteral("Crate")) {
        match.type = CodeCompletionModel::Namespace;
        match.icon = QIcon::fromTheme(QStringLiteral("field"));
    } else if (type == QStringLiteral("Let")) {
        match.type = CodeCompletionModel::Variable;
        match.icon = QIcon::fromTheme(QStringLiteral("code-variable"));
    } else if (type == QStringLiteral("Enum")) {
        match.type = CodeCompletionModel::Enum;
        match.icon = QIcon::fromTheme(QStringLiteral("icon"));
    }
}
