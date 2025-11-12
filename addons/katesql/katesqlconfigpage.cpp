/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katesqlconfigpage.h"
#include "outputstylewidget.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QCheckBox>
#include <QGroupBox>
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
    return QIcon::fromTheme(QLatin1String("server-database"));
}

void KateSQLConfigPage::apply()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("KateSQLPlugin"));

    config.writeEntry("SaveConnections", m_box->isChecked());

    m_outputStyleWidget->writeConfig();

    config.sync();

    Q_EMIT settingsChanged();
}

void KateSQLConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("KateSQLPlugin"));

    m_box->setChecked(config.readEntry("SaveConnections", true));

    m_outputStyleWidget->readConfig();
}

void KateSQLConfigPage::defaults()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("KateSQLPlugin"));

    config.revertToDefault("SaveConnections");
    config.revertToDefault("OutputCustomization");
}

#include "moc_katesqlconfigpage.cpp"
