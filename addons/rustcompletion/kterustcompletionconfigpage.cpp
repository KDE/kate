/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "kterustcompletionconfigpage.h"
#include "kterustcompletionplugin.h"

#include <QGroupBox>
#include <QLineEdit>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KUrlRequester>

KTERustCompletionConfigPage::KTERustCompletionConfigPage(QWidget *parent, KTERustCompletionPlugin *plugin)
    :  KTextEditor::ConfigPage(parent)
    , m_changed(false)
    , m_plugin(plugin)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    QVBoxLayout *vbox = new QVBoxLayout;
    QGroupBox *group = new QGroupBox(i18n("Racer command"), this);
    m_racerCmd = new QLineEdit(this);
    vbox->addWidget(m_racerCmd);
    group->setLayout(vbox);
    layout->addWidget(group);

    vbox = new QVBoxLayout;
    group = new QGroupBox(i18n("Rust source tree location"), this);
    m_rustSrcPath = new KUrlRequester(this);
    m_rustSrcPath->setMode(KFile::Directory | KFile::LocalOnly);
    vbox->addWidget(m_rustSrcPath);
    group->setLayout(vbox);
    layout->addWidget(group);

    layout->insertStretch(-1, 10);

    reset();

    connect(m_racerCmd, &QLineEdit::textChanged, this, &KTERustCompletionConfigPage::changedInternal);
    connect(m_rustSrcPath, &KUrlRequester::textChanged, this, &KTERustCompletionConfigPage::changedInternal);
    connect(m_rustSrcPath, &KUrlRequester::urlSelected, this, &KTERustCompletionConfigPage::changedInternal);
}

QString KTERustCompletionConfigPage::name() const
{
    return QString(i18n("Rust code completion"));
}

QString KTERustCompletionConfigPage::fullName() const
{
    return QString(i18n("Rust code completion"));
}

QIcon KTERustCompletionConfigPage::icon() const
{
    return QIcon::fromTheme(QLatin1String("text-field"));
}

void KTERustCompletionConfigPage::apply()
{
    if (!m_changed) {
        return;
    }

    m_changed = false;

    m_plugin->setRacerCmd(m_racerCmd->text());
    m_plugin->setRustSrcPath(m_rustSrcPath->url());
}

void KTERustCompletionConfigPage::reset()
{
    m_racerCmd->setText(m_plugin->racerCmd());
    m_rustSrcPath->setUrl(m_plugin->rustSrcPath());
    m_changed = false;
}

void KTERustCompletionConfigPage::defaults()
{
    reset();
}

void KTERustCompletionConfigPage::changedInternal()
{
    m_changed = true;

    emit changed();
}
