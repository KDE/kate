/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "FormatConfig.h"

#include "FormatPlugin.h"
#include "FormattersEnum.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KSharedConfig>

FormatConfigPage::FormatConfigPage(class FormatPlugin *plugin, QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
    , m_cbFormatOnSave(new QCheckBox(this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins({});

    auto gb = new QGroupBox(i18n("General"), this);
    mainLayout->addWidget(gb);
    { // General
        auto layout = new QVBoxLayout(gb);
        m_cbFormatOnSave->setText(i18n("Format on save"));
        connect(m_cbFormatOnSave, &QCheckBox::stateChanged, this, &KTextEditor::ConfigPage::changed);
        layout->addWidget(m_cbFormatOnSave);
    }

    gb = new QGroupBox(i18n("Formatter Preference"), this);
    mainLayout->addWidget(gb);
    {
        auto layout = new QVBoxLayout(gb);
        auto addRow = [layout, this](auto label, auto combo) {
            auto hlayout = new QHBoxLayout;
            connect(combo, &QComboBox::currentIndexChanged, this, &KTextEditor::ConfigPage::changed);
            layout->addLayout(hlayout);
            hlayout->setContentsMargins({});
            hlayout->addWidget(label);
            hlayout->addWidget(combo);
            hlayout->addStretch();
        };

        auto lbl = new QLabel(i18n("Formatter for Json"), this);
        m_cmbJson = new QComboBox(this);
        addRow(lbl, m_cmbJson);
        m_cmbJson->addItem(i18n("Clang Format"), (int)Formatters::ClangFormat);
        m_cmbJson->addItem(i18n("Prettier"), (int)Formatters::Prettier);
        m_cmbJson->addItem(i18n("Jq"), (int)Formatters::Jq);
    }

    mainLayout->addStretch();

    reset();
}

void FormatConfigPage::apply()
{
    bool changed = false;

    KConfigGroup cg(KSharedConfig::openConfig(), "Formatting");
    if (m_plugin->formatOnSave != m_cbFormatOnSave->isChecked()) {
        m_plugin->formatOnSave = m_cbFormatOnSave->isChecked();
        cg.writeEntry("FormatOnSave", m_plugin->formatOnSave);
        changed = true;
    }

    if ((int)m_plugin->formatterForJson != m_cmbJson->currentData().value<int>()) {
        m_plugin->formatterForJson = (Formatters)m_cmbJson->currentData().value<int>();
        cg.writeEntry("FormatterForJson", (int)m_plugin->formatterForJson);
        changed = true;
    }

    if (changed) {
        Q_EMIT m_plugin->configChanged();
    }
}

static void setCurrentIndexForCombo(QComboBox *cmb, int formatter)
{
    for (int i = 0; i < cmb->count(); ++i) {
        if (cmb->itemData(i).value<int>() == formatter) {
            cmb->setCurrentIndex(i);
            return;
        }
    }
    qWarning() << "Invalid formatter for combo" << formatter;
}

void FormatConfigPage::reset()
{
    m_cbFormatOnSave->setChecked(m_plugin->formatOnSave);
    setCurrentIndexForCombo(m_cmbJson, (int)m_plugin->formatterForJson);
}
