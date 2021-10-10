#include "compiledbreader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QVariant>

#include <KTextEditor/MainWindow>

#include "gitprocess.h"

#include <optional>

// TODO: copied from kate project plugin, move to shared/
std::optional<QString> getDotGitPath(const QString &repo)
{
    /* This call is intentionally blocking because we need git path for everything else */
    QProcess git;
    setupGitProcess(git, repo, {QStringLiteral("rev-parse"), QStringLiteral("--absolute-git-dir")});
    git.start(QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return std::nullopt;
        }
        QString dotGitPath = QString::fromUtf8(git.readAllStandardOutput());
        if (dotGitPath.endsWith(QLatin1String("\n"))) {
            dotGitPath.remove(QLatin1String(".git\n"));
        } else {
            dotGitPath.remove(QLatin1String(".git"));
        }
        return dotGitPath;
    }
    return std::nullopt;
}

QString CompileDBReader::locateCompileCommands(KTextEditor::MainWindow *mw, const QString &openedFile)
{
    Q_ASSERT(mw);

    // Check using project
    QObject *project = mw->pluginView(QStringLiteral("kateprojectplugin"));
    if (project) {
        QString baseDir = project->property("projectBaseDir").toString();
        if (baseDir.endsWith(QLatin1Char('/'))) {
            baseDir.chop(1);
        }

        if (QFile::exists(baseDir + QStringLiteral("/compile_commands.json"))) {
            return baseDir + QStringLiteral("/compile_commands.json");
        }
    }

    // Check using git
    // For now it only checks for compile_commands in the .git/../ directory
    QFileInfo fi(openedFile);
    if (fi.exists()) {
        QString base = fi.absolutePath();
        auto basePathOptional = getDotGitPath(base);
        if (basePathOptional.has_value()) {
            auto basePath = basePathOptional.value();
            if (basePath.endsWith(QLatin1Char('/'))) {
                basePath.chop(1);
            }
            if (QFile::exists(basePath + QStringLiteral("/compile_commands.json"))) {
                return basePath + QStringLiteral("/compile_commands.json");
            }
        }
    }

    qWarning() << "Compile DB not found for file: " << openedFile;

    return QString();
}

QString CompileDBReader::argsForFile(const QString &compile_commandsPath, const QString &file)
{
    QFile f(compile_commandsPath);
    if (!f.open(QFile::ReadOnly)) {
        // TODO: Use Output view to report error
        qWarning() << "Failed to load compile_commands: " << f.errorString();
        return {};
    }

    QJsonParseError error;
    QJsonDocument cmdCmds = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to read compile_commands: " << error.errorString();
        return {};
    }

    if (!cmdCmds.isArray()) {
        qWarning() << "Invalid compile_commands, root element is not an array";
        return {};
    }

    QJsonArray commandsArray = cmdCmds.array();

    for (const auto &cmdJV : commandsArray) {
        auto compileCommand = cmdJV.toObject();
        auto cmpCmdFile = compileCommand.value(QStringLiteral("file")).toString();

        QFileInfo fi(cmpCmdFile);
        if (fi.isRelative()) {
            QString dir = QDir::cleanPath(compileCommand.value(QStringLiteral("directory")).toString());
            //             QString file = QDir::cleanPath(dir + QStringLiteral("/") + cmpCmdFile);
        } else {
            if (fi.canonicalFilePath() == file) {
                return compileCommand.value(QStringLiteral("command")).toString();
            }
        }
    }

    qWarning() << "compile_command for " << file << " not found";
    return {};
}

static void addCurrentFilePathToCompileCommands(const QString &currentCompiler, QStringList &commands, const QString &fileBasePath)
{
    // For these compilers we include the current file path to
    // the compiler commands
    QStringList compilers = {
        QStringLiteral("c++"),
        QStringLiteral("g++"),
        QStringLiteral("gcc"),
        QStringLiteral("clang"),
        QStringLiteral("clang++"),
    };

    for (const auto &c : compilers) {
        if (currentCompiler.contains(c)) {
            commands << QStringLiteral("-I") + fileBasePath;
        }
    }
}

/*
 * Remove args like "-include xyz.h"
 * CompilerExplorer doesn't like them
 */
static void removeIncludeArgument(QStringList &commands)
{
    QStringList toRemove;
    for (int i = 0; i < commands.size(); ++i) {
        if (commands.at(i) == QStringLiteral("-include")) {
            if (i + 1 < commands.size()) {
                toRemove << commands.at(i);
                toRemove << commands.at(i + 1);
                ++i;
            }
        }
    }

    for (const auto &rem : qAsConst(toRemove)) {
        commands.removeAll(rem);
    }
}

QString CompileDBReader::filteredArgsForFile(const QString &compile_commandsPath, const QString &file)
{
    QString args = argsForFile(compile_commandsPath, file);

    QFileInfo fi(file);
    QString fileBasePath = fi.canonicalPath();

    QStringList argsList = args.split(QLatin1Char(' '));
    QString currentCompiler = argsList.takeFirst(); // First is the compiler, drop it
    QStringList finalArgs;
    finalArgs.reserve(argsList.size() - 2);

    for (auto &&arg : argsList) {
        if (arg == QStringLiteral("-o"))
            continue;
        if (arg.endsWith(QStringLiteral(".o")))
            continue;
        if (arg == QStringLiteral("-c"))
            continue;
        if (file == arg || file.contains(arg))
            continue;

        finalArgs << arg;
    }

    removeIncludeArgument(finalArgs);

    addCurrentFilePathToCompileCommands(currentCompiler, finalArgs, fileBasePath);

    return finalArgs.join(QLatin1Char(' '));
}
