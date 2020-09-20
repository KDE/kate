/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Joseph Wenninger
   SPDX-FileCopyrightText: 2009 Erlend Hamberg <ehamberg@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef PLUGIN_KATEOPENHEADER_H
#define PLUGIN_KATEOPENHEADER_H

#include <KPluginFactory>
#include <KTextEditor/Command>
#include <KXMLGUIClient>
#include <QObject>
#include <QUrl>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/plugin.h>

class PluginKateOpenHeader : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit PluginKateOpenHeader(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~PluginKateOpenHeader() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

public Q_SLOTS:
    void slotOpenHeader();
    void tryOpen(const QUrl &url, const QStringList &extensions);
    bool tryOpenInternal(const QUrl &url, const QStringList &extensions);

private:
    bool fileExists(const QUrl &url);
    void setFileName(QUrl *url, const QString &_txt);
};

class PluginViewKateOpenHeader : public KTextEditor::Command, public KXMLGUIClient
{
    Q_OBJECT
public:
    PluginViewKateOpenHeader(PluginKateOpenHeader *plugin, KTextEditor::MainWindow *mainwindow);
    ~PluginViewKateOpenHeader() override;

    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;

private:
    PluginKateOpenHeader *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
};

#endif // PLUGIN_KATEOPENHEADER_H
