/*
    SPDX-FileCopyrightText: 2026 Leia <leia@tutamail.com>

    SPDX-License-Identifier: MIT
*/
#pragma once

#include "kateprivate_export.h"

#include <QObject>
#include <QUrl>

class QTabWidget;
class QLabel;

namespace KTextEditor
{
class Document;
class View;
};

class KUrlRequester;

/**
 * Common class for plugins that have JSON based settings (like the LSP client)
 * Provides 2 tabs, one with user editable settings and one with readonly default settings
 */
class KATE_PRIVATE_EXPORT JSONSettings : public QObject
{
    Q_OBJECT
public:
    JSONSettings(QObject *parent, QTabWidget *tabWidget, const QString &defaultConfigPath, const QUrl &userConfigPath);
    ~JSONSettings() override = default;

    QUrl userConfigPath();
    void saveUserConfig();
    void setConfigUrl(QUrl &url);
    void readUserConfig();

Q_SIGNALS:
    void configUrlChanged();
    void configChanged();
    void configSaved();

private:
    void updateConfigTextErrorState();

    KUrlRequester *m_configPathEditor;
    QLabel *m_userConfigError;

    const QUrl m_defaultUserConfigPath;

    KTextEditor::Document *m_userConfigDoc;
    KTextEditor::View *m_userConfigView;
};
