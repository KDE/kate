/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QComboBox>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

class KateProjectPluginView;
class KateProjectCodeAnalysisTool;
class KMessageWidget;
class KateProject;
class QStandardItemModel;
class DiagnosticsProvider;
class QSortFilterProxyModel;
class QLabel;

namespace KTextEditor
{
class Document;
}

/**
 * View for Code Analysis.
 * cppcheck and perhaps later more...
 */
class KateProjectInfoViewCodeAnalysis : public QWidget
{
public:
    /**
     * construct project info view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectInfoViewCodeAnalysis(KateProjectPluginView *pluginView, KateProject *project);

    /**
     * deconstruct info view
     */
    ~KateProjectInfoViewCodeAnalysis() override;

    /**
     * our project.
     * @return project
     */
    KateProject *project() const
    {
        return m_project;
    }

private:
    /**
     * Called if the tool is changed (currently via Combobox)
     */
    void slotToolSelectionChanged(int);

    /**
     * Called if start/stop button is clicked.
     */
    void slotStartStopClicked();

    /**
     * More checker output is available
     */
    void slotReadyRead();

    /**
     * Analysis finished
     * @param exitCode analyzer process exit code
     * @param exitStatus analyzer process exit status
     */
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    /**
     * our plugin view
     */
    KateProjectPluginView *m_pluginView;

    /**
     * our project
     */
    KateProject *m_project;

    /**
     * start/stop analysis button
     */
    QPushButton *m_startStopAnalysis;

    /**
     * running analyzer process
     */
    QProcess *m_analyzer;

    /**
     * currently selected tool
     */
    KateProjectCodeAnalysisTool *m_analysisTool;

    /**
     * UI element to select the tool
     */
    QComboBox *m_toolSelector;

    /**
     * Contains a rich text to explain what the current tool does
     */
    QLabel *m_toolInfoLabel;

    DiagnosticsProvider *const m_diagnosticProvider;

    /**
     * Output read in the slotReadyRead
     * will be cleared after process finishes
     */
    QByteArray m_errOutput;
};
