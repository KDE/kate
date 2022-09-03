#ifndef KATE_DIFF_PARAMS
#define KATE_DIFF_PARAMS

#include <QMetaType>
#include <QStringList>

struct DiffParams {
    enum Flag { ShowStage = 1, ShowUnstage = 2, ShowDiscard = 4 };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAGS(Flags)

    /**
     * The tab title that will get shown in Kate.
     * If not specified, srcFile/destFile will be
     * used instead
     */
    QString tabTitle;

    /** Source File **/
    QString srcFile;

    /** Destination file **/
    QString destFile;

    /** Working dir i.e., to which
     * src/dest files belong and where
     * the command was run. This should
     * normally be your repo base path
     */
    QString workingDir;

    /**
     * The arguments that were passed to git
     * when this diff was generated
     */
    QStringList arguments;

    /**
     * These flags are used internally for e.g.,
     * for the context menu actions
     */
    Flags flags;

    void clear()
    {
        tabTitle = srcFile = destFile = workingDir = QString();
        arguments.clear();
        flags = {};
    }
};
Q_DECLARE_METATYPE(DiffParams)

#endif
