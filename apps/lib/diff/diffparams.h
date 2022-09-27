/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_DIFF_PARAMS
#define KATE_DIFF_PARAMS

#include <QMetaType>
#include <QStringList>

struct DiffParams {
    enum Flag {
        /** show stage action in context menu **/
        ShowStage = 1,
        /** show unstage action in context menu **/
        ShowUnstage = 2,
        /** show discard action in context menu **/
        ShowDiscard = 4,
        /** show filename with diff. Appears right before hunk heading **/
        ShowFileName = 8,
        /** show commit info with diff **/
        ShowCommitInfo = 16,
    };
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
