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

namespace KTextEditor
{
class Document;
}

class DocumentOnSaveTracker : public QObject
{
    Q_OBJECT
public:
    DocumentOnSaveTracker(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_timer.setInterval(300);
        m_timer.callOnTimeout(this, [this] {
            Q_EMIT saved(m_doc);
        });
        m_timer.setSingleShot(true);
    }
    void setDocument(KTextEditor::Document *doc);

Q_SIGNALS:
    void saved(KTextEditor::Document *);

private:
    QPointer<KTextEditor::Document> m_doc;
    QTimer m_timer;
};

/**
 * View for Code Analysis.
 * cppcheck and perhaps later more...
 */
class KateProjectInfoViewCodeAnalysis : public QWidget
{
    Q_OBJECT

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

private Q_SLOTS:
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

    void onSaved(KTextEditor::Document *doc);

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

    QSortFilterProxyModel *m_proxyModel;

    /**
     * contains a rich text to explain what the current tool does
     */
    QString m_toolInfoText;

    DiagnosticsProvider *const m_diagnosticProvider;

    DocumentOnSaveTracker m_onSaveTracker;

    enum {
        None = 0,
        OnSave,
        UserClickedButton,
    } m_invocationType;
};
