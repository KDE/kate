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

    const QString labelForRelationship = labelForRelationshipOption + u' ' + u'(' + expirimentalLabel + u')';

    m_enableEditableRelationalTableCheckBox = new QCheckBox(labelForRelationship, this);

    m_enableRunOutsideSqlFilesCheckBox = new QCheckBox(i18nc("@option:check", "Allow running queries in non-SQL files"), this);

    m_enableAutoQuerySelectionCheckBox = new QCheckBox(i18nc("@option:check", "Enable auto query selection"), this);
    m_treatBlankLineAsBreakCheckBox = new QCheckBox(i18nc("@option:check", "Treat a blank line as a statement break"), this);
    m_alwaysShowQueryPopupCheckBox = new QCheckBox(i18nc("@option:check", "Always show query selector popup"), this);
    m_batchShowOnlyErrorsCheckBox = new QCheckBox(i18nc("@option:check", "Show only errors when running multiple queries"), this);
    m_batchShowOnlyErrorsCheckBox->setToolTip(i18nc("@info:tooltip",
                                                    "When enabled, only failed statements are shown in the output during batch execution. "
                                                    "Disabling this may affect performance when running large files."));

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

    const int indent = KateSQLConstants::ConfigPageStyle::Indent;

    layout->addWidget(m_box);
    layout->addWidget(m_enableEditableTableCheckBox);
    auto *relationalTableRow = new QHBoxLayout();
    relationalTableRow->setContentsMargins(indent, 0, 0, 0);
    relationalTableRow->addWidget(m_enableEditableRelationalTableCheckBox);
    layout->addLayout(relationalTableRow);

    layout->addWidget(m_enableRunOutsideSqlFilesCheckBox);
    layout->addWidget(m_enableAutoQuerySelectionCheckBox);
    auto *blankLineRow = new QHBoxLayout();
    blankLineRow->setContentsMargins(indent, 0, 0, 0);
    blankLineRow->addWidget(m_treatBlankLineAsBreakCheckBox);
    layout->addLayout(blankLineRow);
    auto *queryPopupRow = new QHBoxLayout();
    queryPopupRow->setContentsMargins(indent, 0, 0, 0);
    queryPopupRow->addWidget(m_alwaysShowQueryPopupCheckBox);
    layout->addLayout(queryPopupRow);
    layout->addWidget(m_batchShowOnlyErrorsCheckBox);
    layout->addWidget(stylesGroupBox, 1);

    setLayout(layout);

    reset();

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(m_box, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableEditableTableCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_useSystemDefaultsCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::slotUseSystemDefaultsChanged);
    connect(m_enableEditableRelationalTableCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableAutoQuerySelectionCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_treatBlankLineAsBreakCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableRunOutsideSqlFilesCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_alwaysShowQueryPopupCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
    connect(m_batchShowOnlyErrorsCheckBox, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
#else
    connect(m_box, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableEditableTableCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_useSystemDefaultsCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::slotUseSystemDefaultsChanged);
    connect(m_enableEditableRelationalTableCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableAutoQuerySelectionCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_treatBlankLineAsBreakCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_enableRunOutsideSqlFilesCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_alwaysShowQueryPopupCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
    connect(m_batchShowOnlyErrorsCheckBox, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
#endif
    connect(m_enableAutoQuerySelectionCheckBox, &QCheckBox::toggled, m_treatBlankLineAsBreakCheckBox, &QCheckBox::setEnabled);
    connect(m_enableAutoQuerySelectionCheckBox, &QCheckBox::toggled, m_alwaysShowQueryPopupCheckBox, &QCheckBox::setEnabled);
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
    config.writeEntry(KateSQLConstants::Config::EnableAutoQuerySelection, m_enableAutoQuerySelectionCheckBox->isChecked());
    config.writeEntry(KateSQLConstants::Config::TreatBlankLineAsStatementBreak, m_treatBlankLineAsBreakCheckBox->isChecked());
    config.writeEntry(KateSQLConstants::Config::EnableRunOutsideSqlFiles, m_enableRunOutsideSqlFilesCheckBox->isChecked());
    config.writeEntry(KateSQLConstants::Config::AlwaysShowQueryPopup, m_alwaysShowQueryPopupCheckBox->isChecked());
    config.writeEntry(KateSQLConstants::Config::BatchShowOnlyErrors, m_batchShowOnlyErrorsCheckBox->isChecked());

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

    m_enableAutoQuerySelectionCheckBox->setChecked(
        config.readEntry(KateSQLConstants::Config::EnableAutoQuerySelection, KateSQLConstants::Config::DefaultValues::EnableAutoQuerySelection));

    m_treatBlankLineAsBreakCheckBox->setChecked(
        config.readEntry(KateSQLConstants::Config::TreatBlankLineAsStatementBreak, KateSQLConstants::Config::DefaultValues::TreatBlankLineAsStatementBreak));
    m_treatBlankLineAsBreakCheckBox->setEnabled(m_enableAutoQuerySelectionCheckBox->isChecked());

    m_enableRunOutsideSqlFilesCheckBox->setChecked(
        config.readEntry(KateSQLConstants::Config::EnableRunOutsideSqlFiles, KateSQLConstants::Config::DefaultValues::EnableRunOutsideSqlFiles));

    m_alwaysShowQueryPopupCheckBox->setChecked(
        config.readEntry(KateSQLConstants::Config::AlwaysShowQueryPopup, KateSQLConstants::Config::DefaultValues::AlwaysShowQueryPopup));
    m_alwaysShowQueryPopupCheckBox->setEnabled(m_enableAutoQuerySelectionCheckBox->isChecked());

    m_batchShowOnlyErrorsCheckBox->setChecked(
        config.readEntry(KateSQLConstants::Config::BatchShowOnlyErrors, KateSQLConstants::Config::DefaultValues::BatchShowOnlyErrors));

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

    m_enableAutoQuerySelectionCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::EnableAutoQuerySelection);
    m_treatBlankLineAsBreakCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::TreatBlankLineAsStatementBreak);
    m_treatBlankLineAsBreakCheckBox->setEnabled(KateSQLConstants::Config::DefaultValues::EnableAutoQuerySelection);

    m_enableRunOutsideSqlFilesCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::EnableRunOutsideSqlFiles);

    m_alwaysShowQueryPopupCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::AlwaysShowQueryPopup);
    m_alwaysShowQueryPopupCheckBox->setEnabled(KateSQLConstants::Config::DefaultValues::EnableAutoQuerySelection);

    m_batchShowOnlyErrorsCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::BatchShowOnlyErrors);

    m_useSystemDefaultsCheckBox->blockSignals(true);
    m_useSystemDefaultsCheckBox->setChecked(KateSQLConstants::Config::DefaultValues::UseSystemDefaults);
    m_outputStyleWidget->resetToSystemDefaults();
    m_useSystemDefaultsCheckBox->blockSignals(false);

    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    config.revertToDefault(KateSQLConstants::Config::SaveConnections);
    config.revertToDefault(KateSQLConstants::Config::EnableEditableTable);
    config.revertToDefault(KateSQLConstants::Config::EnableEditableRelationalTable);
    config.revertToDefault(KateSQLConstants::Config::EnableAutoQuerySelection);
    config.revertToDefault(KateSQLConstants::Config::TreatBlankLineAsStatementBreak);
    config.revertToDefault(KateSQLConstants::Config::EnableRunOutsideSqlFiles);
    config.revertToDefault(KateSQLConstants::Config::AlwaysShowQueryPopup);
    config.revertToDefault(KateSQLConstants::Config::BatchShowOnlyErrors);
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
