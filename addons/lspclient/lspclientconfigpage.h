/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <KTextEditor/ConfigPage>

class JSONSettings;
class LSPClientPlugin;
struct LSPClientPluginOptions;

namespace KTextEditor
{
class Document;
class View;
}

namespace Ui
{
class LspConfigWidget;
}

class LSPClientConfigPage : public KTextEditor::ConfigPage
{
public:
    explicit LSPClientConfigPage(QWidget *parent = nullptr, LSPClientPlugin *plugin = nullptr);
    ~LSPClientConfigPage() override;

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public:
    void apply() override;
    void defaults() override;
    void reset() override;
    void showContextMenuAllowedBlocked(const QPoint &pos);

private:
    JSONSettings *m_jsonSettings;

    void resetUiTo(const LSPClientPluginOptions &options);

    Ui::LspConfigWidget *ui;
    LSPClientPlugin *m_plugin;
};
