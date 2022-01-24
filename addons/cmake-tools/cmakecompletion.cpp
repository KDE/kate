/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "cmakecompletion.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QProcess>
#include <QStandardPaths>

struct CMakeComplData {
    std::vector<QByteArray> m_commands;
    std::vector<QByteArray> m_vars;
    std::vector<QByteArray> m_props;
};

static QByteArray runCMake(const QString &arg)
{
    // only use cmake from PATH
    static const auto cmakeExecutable = QStandardPaths::findExecutable(QStringLiteral("cmake"));
    if (cmakeExecutable.isEmpty()) {
        return {};
    }

    QProcess p;
    p.start(cmakeExecutable, {arg});
    if (p.waitForStarted() && p.waitForFinished()) {
        if (p.exitCode() == 0 && p.exitStatus() == QProcess::NormalExit) {
            return p.readAllStandardOutput();
        }
    }
    return {};
}

std::vector<QByteArray> parseList(const QByteArray &ba, int reserve)
{
    std::vector<QByteArray> ret;
    ret.reserve(reserve);
    int start = 0;
    int next = ba.indexOf('\n', start);

    while (next > 0) {
        ret.push_back(ba.mid(start, next - start));
        start = next + 1;
        next = ba.indexOf('\n', start);
    }
    return ret;
}

static CMakeComplData fetchData()
{
    CMakeComplData ret;

    auto cmds = runCMake(QStringLiteral("--help-command-list"));
    auto vars = runCMake(QStringLiteral("--help-variable-list"));
    auto props = runCMake(QStringLiteral("--help-property-list"));

    // The numbers are from counting the number of props/vars/cmds
    // from the output of cmake --help-*
    ret.m_commands = parseList(cmds, 125);
    ret.m_vars = parseList(vars, 627);
    ret.m_props = parseList(props, 497);

    return ret;
}

bool CMakeCompletion::isCMakeFile(const QUrl &url)
{
    auto urlString = url.fileName();
    return urlString == QStringLiteral("CMakeLists.txt") || urlString.endsWith(QStringLiteral(".cmake"));
}

static void append(std::vector<CMakeCompletion::Completion> &out, std::vector<QByteArray> &&in, CMakeCompletion::Completion::Kind kind)
{
    for (auto &&s : in) {
        out.push_back({kind, std::move(s)});
    }
}

CMakeCompletion::CMakeCompletion(QObject *parent)
    : KTextEditor::CodeCompletionModel(parent)
{
}

void CMakeCompletion::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &, InvocationType)
{
    if (!m_hasData && isCMakeFile(view->document()->url())) {
        CMakeComplData data = fetchData();
        append(m_matches, std::move(data.m_commands), Completion::Compl_COMMAND);
        append(m_matches, std::move(data.m_vars), Completion::Compl_VARIABLE);
        append(m_matches, std::move(data.m_props), Completion::Compl_PROPERTY);
        setRowCount(m_matches.size());
        m_hasData = true;
    }
}

bool CMakeCompletion::shouldStartCompletion(KTextEditor::View *view, const QString &insertedText, bool userInsertion, const KTextEditor::Cursor &position)
{
    if (!userInsertion) {
        return false;
    }
    if (insertedText.isEmpty()) {
        return false;
    }
    // Dont invoke for comments, wont handle everything of course
    if (view->document()->line(position.line()).startsWith(QLatin1Char('#'))) {
        return false;
    }

    return isCMakeFile(view->document()->url());
}

int CMakeCompletion::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_matches.size();
}

static QIcon getIcon(CMakeCompletion::Completion::Kind type)
{
    using Kind = CMakeCompletion::Completion::Kind;
    if (type == Kind::Compl_PROPERTY) {
        static const QIcon icon(QIcon::fromTheme(QStringLiteral("code-block")));
        return icon;
    } else if (type == Kind::Compl_COMMAND) {
        static const QIcon icon(QIcon::fromTheme(QStringLiteral("code-function")));
        return icon;
    } else if (type == Kind::Compl_VARIABLE) {
        static const QIcon icon(QIcon::fromTheme(QStringLiteral("code-variable")));
        return icon;
    } else {
        Q_UNREACHABLE();
        return {};
    }
}

QVariant CMakeCompletion::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role != Qt::DisplayRole && role != Qt::DecorationRole)
        return {};

    if (index.column() == KTextEditor::CodeCompletionModel::Name && role == Qt::DisplayRole) {
        return m_matches.at(index.row()).text;
    } else if (role == Qt::DecorationRole && index.column() == KTextEditor::CodeCompletionModel::Icon) {
        return getIcon(m_matches.at(index.row()).kind);
    }

    return {};
}
