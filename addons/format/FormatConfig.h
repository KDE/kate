/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KLocalizedString>
#include <KTextEditor/ConfigPage>
#include <QIcon>

class FormatConfigPage final : public KTextEditor::ConfigPage
{
    Q_OBJECT
public:
    explicit FormatConfigPage(class FormatPlugin *plugin, QWidget *parent = nullptr);

    QString name() const override
    {
        return i18n("Formatting");
    }

    QString fullName() const override
    {
        return i18n("Formatting Settings");
    }

    QIcon icon() const override
    {
        return QIcon::fromTheme(QStringLiteral("format-indent-less"));
    }

    void apply() override;
    void reset() override;

    void defaults() override
    {
    }

private:
    class FormatPlugin *const m_plugin;
    class QTabWidget *const m_tabWidget;
    class QPlainTextEdit *m_defaultConfigEdit;
    class UserConfigEdit *m_userConfigEdit;
};
