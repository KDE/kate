/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katesqlconfigpage.h"
#include "katesqlconstants.h"
#include "outputstylewidget.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QVBoxLayout>
#include <qlatin1stringview.h>

KateSQLConfigPage::KateSQLConfigPage(QWidget *parent)
    : KTextEditor::ConfigPage(parent)
{
    auto *layout = new QVBoxLayout(this);

    m_box = new QCheckBox(i18nc("@option:check", "Save and restore connections in Kate session"), this);
    m_enableEditableTableCheckBox = new QCheckBox(i18nc("@option:check", "Enable editable table for Browse Data"), this);

    const auto labelForRelationshipOption = i18nc("@option:check", "Enable editable relational tables");
    const auto expirimentalLabel = i18nc("@option:check", "Experimental");

    const auto labelForRelationship = labelForRelationshipOption + u' ' + u'(' + expirimentalLabel + u')';

    m_enableEditableRelationalTableCheckBox = new QCheckBox(labelForRelationship, this);

    auto *stylesGroupBox = new QGroupBox(i18nc("@title:group", "Output Customization"), this);
    auto *stylesLayout = new QVBoxLayout(stylesGroupBox);

    m_outputStyleWidget = new OutputStyleWidget(this);
    stylesLayout->addWidget(m_outputStyleWidget);

    auto *bottomRow = new QHBoxLayout();
    m_useSystemDefaultsCheckBox = new QCheckBox(i18nc("@option:check", "Use system defaults"), this);
    m_resetToDefaultsButton = new QPushButton(i18nc("@action:button", "Reset to System Defaults"), this);
    bottomRow->addWidget(m_useSystemDefaultsCheckBox);
    bottomRow->addStretch();
    bottomRow->addWidget(m_resetToDefaultsButton);
    stylesLayout->addLayout(bottomRow);

    layout->addWidget(m_box);
    layout->addWidget(m_enableEditableTableCheckBox);
    layout->addWidget(m_enableEditableRelationalTableCheckBox);
    layout->addWidget(stylesGroupBox, 1);

    setLayout(layout);

    reset();

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(m_box, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableEditableTableCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_useSystemDefaultsCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::slotUseSystemDefaultsChanged);
    connect(m_enableEditableRelationalTableCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
#else
    connect(m_box, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableEditableTableCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_useSystemDefaultsCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::slotUseSystemDefaultsChanged);
    connect(m_enableEditableRelationalTableCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
#endif
    connect(m_enableEditableTableCheckBox, &QCheckBox::toggled, m_enableEditableRelationalTableCheckBox, &QCheckBox::setEnabled);
    connect(m_resetToDefaultsButton, &QPushButton::clicked, this, &KateSQLConfigPage::slotResetToSystemDefaults);
    connect(m_outputStyleWidget, &OutputStyleWidget::changed, this, &KateSQLConfigPage::changed);
}

KateSQLConfigPage::~KateSQLConfigPage() = default;

QString KateSQLConfigPage::name() const
{
    return i18nc("@title", "SQL");
}

QString KateSQLConfigPage::fullName() const
{
    return i18nc("@title:window", "SQL ConfigPage Settings");
}

QIcon KateSQLConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("server-database")); // TODO better Icon from QIcon::ThemeIcon::...
}

void KateSQLConfigPage::apply()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    config.writeEntry(KateSQLConstants::Config::SaveConnections, m_box->isChecked());
    config.writeEntry(KateSQLConstants::Config::EnableEditableTable, m_enableEditableTableCheckBox->isChecked());
    config.writeEntry(KateSQLConstants::Config::EnableEditableRelationalTable, m_enableEditableRelationalTableCheckBox->isChecked());

    m_outputStyleWidget->writeConfig();

    config.sync();

    Q_EMIT settingsChanged();
}

void KateSQLConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    m_box->setChecked(config.readEntry(KateSQLConstants::Config::SaveConnections, KateSQLConstants::Config::DefaultValues::SaveConnections));
    m_enableEditableTableCheckBox->setChecked(
        config.readEntry(KateSQLConstants::Config::EnableEditableTable, KateSQLConstants::Config::DefaultValues::EnableEditableTable));
    m_enableEditableRelationalTableCheckBox->setChecked(
        config.readEntry(KateSQLConstants::Config::EnableEditableRelationalTable, KateSQLConstants::Config::DefaultValues::EnableEditableRelationTable));
    m_enableEditableRelationalTableCheckBox->setEnabled(m_enableEditableTableCheckBox->isChecked());

    m_useSystemDefaultsCheckBox->blockSignals(true);
    m_outputStyleWidget->readConfig();
    m_useSystemDefaultsCheckBox->setChecked(m_outputStyleWidget->useSystemDefaults());
    m_useSystemDefaultsCheckBox->blockSignals(false);
}

void KateSQLConfigPage::defaults()
{
    m_box->setChecked(KateSQLConstants::Config::DefaultValues::SaveConnections);
    m_enableEditableTableCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::EnableEditableTable);
    m_enableEditableRelationalTableCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::EnableEditableRelationTable);
    m_enableEditableRelationalTableCheckBox->setEnabled(KateSQLConstants::Config::DefaultValues::EnableEditableTable);

    m_useSystemDefaultsCheckBox->blockSignals(true);
    m_useSystemDefaultsCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::UseSystemDefaults);
    m_outputStyleWidget->resetToSystemDefaults();
    m_useSystemDefaultsCheckBox->blockSignals(false);

    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    config.revertToDefault(KateSQLConstants::Config::SaveConnections);
    config.revertToDefault(KateSQLConstants::Config::EnableEditableTable);
    config.revertToDefault(KateSQLConstants::Config::EnableEditableRelationalTable);
    config.revertToDefault(KateSQLConstants::Config::UseSystemDefaults);
    config.revertToDefault(KateSQLConstants::Config::OutputCustomizationGroup);
}

void KateSQLConfigPage::slotUseSystemDefaultsChanged()
{
    m_outputStyleWidget->setUseSystemDefaults(m_useSystemDefaultsCheckBox->isChecked());
}

void KateSQLConfigPage::slotResetToSystemDefaults()
{
    m_outputStyleWidget->resetToSystemDefaults();
}

#include "moc_katesqlconfigpage.cpp"
