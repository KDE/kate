/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

class OutputStyleWidget;
class QCheckBox;
class QPushButton;

#include <ktexteditor/configpage.h>

/// TODO: add options to change datetime and numbers format

class KateSQLConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit KateSQLConfigPage(QWidget *parent = nullptr);
    ~KateSQLConfigPage() override;

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reset() override;
    void defaults() override;

private Q_SLOTS:
    void slotUseSystemDefaultsChanged();
    void slotResetToSystemDefaults();

private:
    QCheckBox *m_box;
    QCheckBox *m_enableEditableTableCheckBox;
    QCheckBox *m_useSystemDefaultsCheckBox;
    QPushButton *m_resetToDefaultsButton;
    OutputStyleWidget *m_outputStyleWidget;

Q_SIGNALS:
    void settingsChanged();
};
