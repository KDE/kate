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
#include <QIcon>
#include <QVBoxLayout>

KateSQLConfigPage::KateSQLConfigPage(QWidget *parent)
    : KTextEditor::ConfigPage(parent)
{
    auto *layout = new QVBoxLayout(this);

    m_box = new QCheckBox(i18nc("@option:check", "Save and restore connections in Kate session"), this);

    auto *stylesGroupBox = new QGroupBox(i18nc("@title:group", "Output Customization"), this);
    auto *stylesLayout = new QVBoxLayout(stylesGroupBox);

    m_outputStyleWidget = new OutputStyleWidget(this);

    stylesLayout->addWidget(m_outputStyleWidget);

    layout->addWidget(m_box);
    layout->addWidget(stylesGroupBox, 1);

    setLayout(layout);

    reset();

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(m_box, &QCheckBox::stateChanged, this, &KateSQLConfigPage::changed);
#else
    connect(m_box, &QCheckBox::checkStateChanged, this, &KateSQLConfigPage::changed);
#endif
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

    m_outputStyleWidget->writeConfig();

    config.sync();

    Q_EMIT settingsChanged();
}

void KateSQLConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    m_box->setChecked(config.readEntry(KateSQLConstants::Config::SaveConnections, true));

    m_outputStyleWidget->readConfig();
}

void KateSQLConfigPage::defaults()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);

    config.revertToDefault(KateSQLConstants::Config::SaveConnections);
    config.revertToDefault(KateSQLConstants::Config::OutputCustomizationGroup);
}

#include "moc_katesqlconfigpage.cpp"
