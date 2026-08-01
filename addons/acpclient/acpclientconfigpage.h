/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <KTextEditor/ConfigPage>

class JSONSettings;
class ACPClientPlugin;

namespace Ui
{
class ACPConfigWidget;
}

class ACPClientConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit ACPClientConfigPage(ACPClientPlugin *plugin, QWidget *parent = nullptr);
    ~ACPClientConfigPage() override;

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public:
    void apply() override;
    void defaults() override;
    void reset() override;

private:
    void resetUiTo();

    JSONSettings *m_jsonSettings;
    Ui::ACPConfigWidget *ui;
    ACPClientPlugin *m_plugin;
};
