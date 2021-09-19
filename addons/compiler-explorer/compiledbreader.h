#ifndef KATE_CMP_DB_READER_H
#define KATE_CMP_DB_READER_H

#include <QHash>
#include <QString>

namespace KTextEditor
{
class MainWindow;
}

class CompileDBReader
{
public:
    //     CompileDBReader(const QString &path);

    /**
     * Locate compile_commands.json file
     */
    static QString locateCompileCommands(KTextEditor::MainWindow *mw, const QString &openedFile);

    /**
     * Get compiler args for file
     */
    static QString argsForFile(const QString &compile_commandsPath, const QString &file);

    /**
     * Filter some args like -o file.o
     *
     * adds the path of @file to compile_commands
     */
    static QString filteredArgsForFile(const QString &compile_commandsPath, const QString &file);

    // private:
    //     void readCompileCommands();
    //
    //     QHash<QString, QString> m_fileToArgs;
};

#endif // KATE_CMP_DB_READER_H
