/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2023 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <KTextEditor/ConfigPage>

class KatePluginGDB;

namespace Ui
{
class DebugConfigWidget;
}

class DebugConfigPage : public KTextEditor::ConfigPage
{
public:
    explicit DebugConfigPage(QWidget *parent = nullptr, KatePluginGDB *plugin = nullptr);
    ~DebugConfigPage() override;

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void defaults() override;
    void reset() override;
    void configTextChanged();
    void configUrlChanged();
    void updateHighlighters();

private:
    void readUserConfig(const QString &fileName);
    void updateConfigTextErrorState();

    Ui::DebugConfigWidget *ui;
    KatePluginGDB *m_plugin;
};
