/*
    SPDX-FileCopyrightText: 2026 Gary Wang <opensource@blumia.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <memory>
#include <unordered_map>

#include <KTextEditor/InlineNoteProvider>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>

class ShowNonPrintableInlineNoteProvider : public KTextEditor::InlineNoteProvider
{
public:
    explicit ShowNonPrintableInlineNoteProvider(KTextEditor::Document *doc);
    ~ShowNonPrintableInlineNoteProvider() override;

    void setEnabled(bool enabled);
    bool isEnabled() const
    {
        return m_enabled;
    }

    void updateNotes(int startLine = -1, int endLine = -1);

    QList<int> inlineNotes(int line) const override;
    QSize inlineNoteSize(const KTextEditor::InlineNote &note) const override;
    void paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection) const override;

private:
    QPointer<KTextEditor::Document> m_doc;
    int m_startChangedLines = -1;
    int m_endChangedLines = -1;
    int m_previousNumLines = -1;
    bool m_enabled = false;

    struct NoteData {
        int column;
        QString text;
    };

    mutable QHash<int, QList<NoteData>> m_notes;
};

class KateShowNonPrintablePluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    KateShowNonPrintablePluginView(class KateShowNonPrintablePlugin *plugin, KTextEditor::MainWindow *mainWindow);
    ~KateShowNonPrintablePluginView() override;

private:
    KateShowNonPrintablePlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    class QAction *m_toggleAction;
};

class KateShowNonPrintablePlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit KateShowNonPrintablePlugin(QObject *parent);
    ~KateShowNonPrintablePlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    void setEnabled(bool enabled);
    bool isEnabled() const
    {
        return m_enabled;
    }

Q_SIGNALS:
    void enabledChanged(bool enabled);

private:
    void addDocument(KTextEditor::Document *doc);

    std::unordered_map<KTextEditor::Document *, std::unique_ptr<ShowNonPrintableInlineNoteProvider>> m_inlineNoteProviders;
    bool m_enabled = false;
};
