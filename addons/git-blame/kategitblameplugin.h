/*
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "gitblameparser.h"
#include "gitblametooltip.h"

#include <KTextEditor/InlineNoteProvider>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>

#include <QProcess>

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QLocale>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariant>

enum class KateGitBlameMode {
    None,
    SingleLine,
    AllLines,
    Count = AllLines
};

class KateGitBlamePluginView;
class GitBlameTooltip;

class GitBlameInlineNoteProvider : public KTextEditor::InlineNoteProvider
{
public:
    explicit GitBlameInlineNoteProvider(KateGitBlamePluginView *view);
    ~GitBlameInlineNoteProvider() override;

    QList<int> inlineNotes(int line) const override;
    QSize inlineNoteSize(const KTextEditor::InlineNote &note) const override;
    void paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection) const override;

    void inlineNoteActivated(const KTextEditor::InlineNote &note, Qt::MouseButtons buttons, const QPoint &globalPos) override;
    void cycleMode();
    void setMode(KateGitBlameMode mode);

private:
    KateGitBlamePluginView *m_pluginView;
    QLocale m_locale;
    KateGitBlameMode m_mode;
};

class KateGitBlamePlugin : public KTextEditor::Plugin
{
public:
    explicit KateGitBlamePlugin(QObject *parent);

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class KateGitBlamePluginView : public QObject, public KXMLGUIClient
{
public:
    KateGitBlamePluginView(KateGitBlamePlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~KateGitBlamePluginView() override;

    QPointer<KTextEditor::View> activeView() const;
    QPointer<KTextEditor::Document> activeDocument() const;

    bool hasBlameInfo() const;
    const CommitInfo &blameInfo(int lineNr);

    void showCommitInfo(const QString &hash, KTextEditor::View *view);

    void setToolTipIgnoreKeySequence(const QKeySequence &sequence);

    void showCommitTreeView(const QUrl &url);

private:
    enum Command {
        RevParse,
        Config,
        Blame,
        IgnoreRevsFile
    };
    Q_ENUM(Command)

    void sendMessage(const QString &text, bool error);

    void startGitBlameForActiveView();

    void startBlameProcess(const QUrl &url);
    void commandFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void parseGitBlameStdOutput();

    void startShowProcess(const QUrl &url, const QString &hash);
    void showFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError e);

    KTextEditor::MainWindow *m_mainWindow;

    GitBlameInlineNoteProvider m_inlineNoteProvider;

    QProcess m_blameInfoProc;
    QProcess m_showProc;

    QPointer<KTextEditor::View> m_lastView;
    int m_lineOffset{0};
    GitBlameTooltip m_tooltip;
    QString m_showHash;
    class CommitDiffTreeView *m_commitFilesView;
    QPointer<KTextEditor::View> m_diffView;
    QTimer m_startBlameTimer;
    QString m_parentPath;
    Command m_currentCommand;
    QString m_root;
    QString m_ignoreRevsFile;
    QString m_absoluteFilePath;

    KateGitBlameParser m_parser;
};
