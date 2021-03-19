#include "kateprojectcodeanalysistoolclazy.h"

#include <KLocalizedString>
#include <QRegularExpression>

KateProjectCodeAnalysisToolClazy::KateProjectCodeAnalysisToolClazy(QObject *parent)
    : KateProjectCodeAnalysisTool(parent)
{
}

QString KateProjectCodeAnalysisToolClazy::name() const
{
    return i18n("Clazy (Qt/C++)");
}

QString KateProjectCodeAnalysisToolClazy::description() const
{
    return i18n("Clazy is a static analysis tool for Qt/C++ code");
}

QString KateProjectCodeAnalysisToolClazy::fileExtensions() const
{
    return QStringLiteral("cpp|cxx|cc|c++|tpp|txx");
}

QStringList KateProjectCodeAnalysisToolClazy::filter(const QStringList &files) const
{
    // c++ files
    return files.filter(
        QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
}

QString KateProjectCodeAnalysisToolClazy::path() const
{
    return QStringLiteral("clazy-standalone");
}

QStringList KateProjectCodeAnalysisToolClazy::arguments()
{
    if (!m_project) {
        return {};
    }

    // check for compile_commands.json
    bool hasCompileComds = QFile::exists(m_project->baseDir() + QStringLiteral("/build/compile_commands.json"));

    QStringList args;
    if (hasCompileComds) {
        args = QStringList{QStringLiteral("-p"), m_project->baseDir() + QStringLiteral("/build")};
    }

    auto &&fileList = filter(m_project->files());
    setActualFilesCount(fileList.size());
    QString files = fileList.join(QLatin1Char(' '));

    return args << fileList;
}

QString KateProjectCodeAnalysisToolClazy::notInstalledMessage() const
{
    return i18n("Please install 'clazy'.");
}

QStringList KateProjectCodeAnalysisToolClazy::parseLine(const QString &line) const
{
    //"/path/kate/kate/kateapp.cpp:529:10: warning: Missing reference in range-for with non trivial type (QJsonValue) [-Wclazy-range-loop]"
    int idxColon = line.indexOf(QLatin1Char(':'));
    if (idxColon < 0) {
        return {};
    }
    QString file = line.mid(0, idxColon);
    idxColon++;
    int nextColon = line.indexOf(QLatin1Char(':'), idxColon);
    QString lineNo = line.mid(idxColon, nextColon - idxColon);

    int spaceIdx = line.indexOf(QLatin1Char(' '), nextColon);
    if (spaceIdx < 0) {
        return {};
    }

    idxColon = line.indexOf(QLatin1Char(':'), spaceIdx);
    if (idxColon < 0) {
        return {};
    }

    QString severity = line.mid(spaceIdx + 1, idxColon - (spaceIdx + 1));

    idxColon++;

    QString msg = line.mid(idxColon);

    return {file, lineNo, severity, msg};
}

QString KateProjectCodeAnalysisToolClazy::stdinMessages()
{
    return QString();
}
