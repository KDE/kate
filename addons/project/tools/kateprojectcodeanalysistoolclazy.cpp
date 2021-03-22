#include "kateprojectcodeanalysistoolclazy.h"

#include <KLocalizedString>

#include <QDir>
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

static QString buildDirectory(const QVariantMap &projectMap)
{
    const QVariantMap buildMap = projectMap[QStringLiteral("build")].toMap();
    const QString buildDir = buildMap[QStringLiteral("directory")].toString();
    return buildDir;
}

QStringList KateProjectCodeAnalysisToolClazy::arguments()
{
    if (!m_project) {
        return {};
    }

    QString compileCommandsDir = compileCommandsDirectory();

    QStringList args;
    if (!compileCommandsDir.isEmpty()) {
        args = QStringList{QStringLiteral("-p"), compileCommandsDir};
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

QString KateProjectCodeAnalysisToolClazy::compileCommandsDirectory() const
{
    QString buildDir = buildDirectory(m_project->projectMap());
    const QString compCommandsFile = QStringLiteral("compile_commands.json");

    // check for compile_commands.json
    QString compileCommandsDir;
    bool hasCompileComds = false;
    if (QDir::isAbsolutePath(buildDir)) {
        if (!buildDir.endsWith(QLatin1Char('/'))) {
            buildDir.append(QLatin1Char('/'));
        }
        hasCompileComds = QFile::exists(buildDir + compCommandsFile);
        if (hasCompileComds) {
            compileCommandsDir = buildDir;
        }
    } else if (QDir::isRelativePath(buildDir)) {
        if (buildDir.startsWith(QLatin1Char('/'))) {
            buildDir = buildDir.mid(1);
        } else if (buildDir.startsWith(QLatin1String("./"))) {
            buildDir = buildDir.mid(2);
        }
        QString path = m_project->baseDir() + QLatin1Char('/') + buildDir;
        hasCompileComds = QFile::exists(QDir(path).absoluteFilePath(compCommandsFile));
        if (hasCompileComds) {
            compileCommandsDir = path;
        }
    } else {
        hasCompileComds = QFile::exists(m_project->baseDir() + QStringLiteral("/build/compile_commands.json"));
        if (hasCompileComds) {
            compileCommandsDir = m_project->baseDir() + QStringLiteral("/build");
        }
    }
    return compileCommandsDir;
}
