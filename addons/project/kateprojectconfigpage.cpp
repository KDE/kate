/* This file is part of the KDE project

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateprojectconfigpage.h"
#include "kateprojectplugin.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <KLocalizedString>

KateProjectConfigPage::KateProjectConfigPage(QWidget *parent, KateProjectPlugin *plugin)
    :  KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
    , m_changed(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    QVBoxLayout *vbox = new QVBoxLayout;
    QGroupBox *group = new QGroupBox(i18nc("Groupbox title", "Autoload Repositories"), this);
    group->setWhatsThis(i18n(
        "Project plugin is able to autoload repository working copies when "
        "there is no .kateproject file defined yet."));

    m_cbAutoGit = new QCheckBox(i18n("&Git"), this);
    vbox->addWidget(m_cbAutoGit);

    m_cbAutoSubversion = new QCheckBox(i18n("&Subversion"), this);
    vbox->addWidget(m_cbAutoSubversion);
    m_cbAutoMercurial = new QCheckBox(i18n("&Mercurial"), this);
    vbox->addWidget(m_cbAutoMercurial);

    vbox->addStretch(1);
    group->setLayout(vbox);

    layout->addWidget(group);
    layout->insertStretch(-1, 10);

    reset();

    connect(m_cbAutoGit, &QCheckBox::stateChanged, this, &KateProjectConfigPage::slotMyChanged);
    connect(m_cbAutoSubversion, &QCheckBox::stateChanged, this, &KateProjectConfigPage::slotMyChanged);
    connect(m_cbAutoMercurial, &QCheckBox::stateChanged, this, &KateProjectConfigPage::slotMyChanged);
}

QString KateProjectConfigPage::name() const
{
    return i18n("Projects");
}

QString KateProjectConfigPage::fullName() const
{
    return i18nc("Groupbox title", "Projects Properties");
}

QIcon KateProjectConfigPage::icon() const
{
    return QIcon::fromTheme(QLatin1String("view-list-tree"));
}

void KateProjectConfigPage::apply()
{
    if (!m_changed) {
        return;
    }

    m_changed = false;

    m_plugin->setAutoRepository(
        m_cbAutoGit->checkState() == Qt::Checked,
        m_cbAutoSubversion->checkState() == Qt::Checked,
        m_cbAutoMercurial->checkState() == Qt::Checked);
}

void KateProjectConfigPage::reset()
{
    m_cbAutoGit->setCheckState(m_plugin->autoGit() ? Qt::Checked : Qt::Unchecked);
    m_cbAutoSubversion->setCheckState(m_plugin->autoSubversion() ? Qt::Checked : Qt::Unchecked);
    m_cbAutoMercurial->setCheckState(m_plugin->autoMercurial() ? Qt::Checked : Qt::Unchecked);
    m_changed = false;
}

void KateProjectConfigPage::defaults()
{
    reset();
}

void KateProjectConfigPage::slotMyChanged()
{
    m_changed = true;
    emit changed();
}
