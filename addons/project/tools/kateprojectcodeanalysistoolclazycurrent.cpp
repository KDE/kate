#include "kateprojectcodeanalysistoolclazycurrent.h"

#include <KLocalizedString>
#include <KTextEditor/MainWindow>

KateProjectCodeAnalysisToolClazyCurrent::KateProjectCodeAnalysisToolClazyCurrent(QObject *parent)
    : KateProjectCodeAnalysisToolClazy(parent)
{
}

QString KateProjectCodeAnalysisToolClazyCurrent::name() const
{
    return i18n("Clazy - Current File");
}

QString KateProjectCodeAnalysisToolClazyCurrent::description() const
{
    return i18n("clang-tidy is a clang-based C++ “linter” tool");
}

QStringList KateProjectCodeAnalysisToolClazyCurrent::arguments()
{
    if (!m_project || !m_mainWindow || !m_mainWindow->activeView()) {
        return {};
    }

    QString compileCommandsDir = compileCommandsDirectory();

    QStringList args;
    if (!compileCommandsDir.isEmpty()) {
        args << QStringList{QStringLiteral("-p"), compileCommandsDir};
    }
    setActualFilesCount(1);

    const QString file = m_mainWindow->activeView()->document()->url().toLocalFile();
    args.append(file);

    return args;
}
