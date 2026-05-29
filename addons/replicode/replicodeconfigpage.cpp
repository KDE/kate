/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "replicodeconfigpage.h"
#include "replicodeconfig.h"
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KUrlRequester>

#include <KLocalizedString>
#include <QGridLayout>
#include <QLabel>
#include <QStyle>

ReplicodeConfigPage::ReplicodeConfigPage(QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_config(new ReplicodeConfig(this))
{
    auto *gridlayout = new QGridLayout;
    setLayout(gridlayout);
    gridlayout->setContentsMargins(0, 0, 0, 0);

    auto *hLayout = new QHBoxLayout;

    hLayout->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                0);

    gridlayout->addLayout(hLayout, 0, 0);
    hLayout->addWidget(new QLabel(i18n("Path to replicode executor:")));

    m_requester = new KUrlRequester;
    m_requester->setMode(KFile::File | KFile::ExistingOnly);
    hLayout->addWidget(m_requester);

    gridlayout->addWidget(m_config, 1, 0, 1, 1);

    reset();

    connect(m_requester, &KUrlRequester::textChanged, this, &ReplicodeConfigPage::changed);
}

QString ReplicodeConfigPage::name() const
{
    return i18n("Replicode");
}

QString ReplicodeConfigPage::fullName() const
{
    return i18n("Replicode configuration");
}

QIcon ReplicodeConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("code-block"));
}

void ReplicodeConfigPage::apply()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Replicode"));
    config.writeEntry("replicodePath", m_requester->text());
    m_config->save();
}

void ReplicodeConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Replicode"));
    m_requester->setText(config.readEntry<QString>("replicodePath", QString()));
    m_config->load();
}

void ReplicodeConfigPage::defaults()
{
    m_requester->setText(QString());
    m_config->reset();
}
