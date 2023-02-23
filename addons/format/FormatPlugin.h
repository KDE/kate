/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "FormattersEnum.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

#include <QPointer>

struct PatchLine;

class FormatPlugin final : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit FormatPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override
    {
        return 1;
    }

    KTextEditor::ConfigPage *configPage(int number, QWidget *parent) override;
    void readConfig();
    void readFormatterConfig();

    QJsonObject formatterConfig() const;
    QString userConfigPath() const;

    bool formatOnSave = false;

    Formatters formatterForJson = Formatters::Prettier;

    Q_SIGNAL void configChanged();

private:
    const QJsonDocument m_defaultConfig;
    QJsonObject m_formatterConfig;
};

class FormatPluginView final : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit FormatPluginView(FormatPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~FormatPluginView();

private:
    void format();
    void runFormatOnSave();
    void onActiveViewChanged(KTextEditor::View *);
    void onFormattedTextReceived(class AbstractFormatter *, KTextEditor::Document *doc, const QByteArray &, int offset);
    void onFormattedPatchReceived(KTextEditor::Document *doc, const std::vector<PatchLine> &, bool setCursor = false);
    void saveDocument(KTextEditor::Document *doc);
    bool formatOnSave() const
    {
        return m_plugin->formatOnSave;
    }
    void onConfigChanged();

    QPointer<KTextEditor::Document> m_activeDoc;
    QByteArray m_lastChecksum;
    FormatPlugin *const m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    bool m_triggeredOnSave = false;
};
