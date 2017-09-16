/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2007 Anders Lund <anders@alweb.dk>
   Copyright (C) 2009, Abhishek Patil <abhishekworld@gmail.com>
   Copyright (C) 2010, Thomas Fjellstrom <thomas@fjellstrom.ca>

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

/*
Config stuff plan:
-----------------

main default config is stored in KSharedConfig::openConfig()+":filetree"
when main config is set, it needs to tell view's to delete
existing customized settings, and use the global ones (somehow)
(maybe some kind of "customized" flag?)

view needs to pull default settings from the main plugin config

*/

#include "katefiletreeconfigpage.h"
#include "katefiletreeplugin.h"
#include "katefiletreedebug.h"
#include "katefiletreemodel.h"
#include "katefiletreeproxymodel.h"

#include <QGroupBox>
#include <KColorButton>
#include <QLabel>
#include <KComboBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <KLocalizedString>

KateFileTreeConfigPage::KateFileTreeConfigPage(QWidget *parent, KateFileTreePlugin *fl)
    :  KTextEditor::ConfigPage(parent),
       m_plug(fl),
       m_changed(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    gbEnableShading = new QGroupBox(i18n("Background Shading"), this);
    gbEnableShading->setCheckable(true);
    layout->addWidget(gbEnableShading);

    QGridLayout *lo = new QGridLayout(gbEnableShading);

    kcbViewShade = new KColorButton(gbEnableShading);
    lViewShade = new QLabel(i18n("&Viewed documents' shade:"), gbEnableShading);
    lViewShade->setBuddy(kcbViewShade);
    lo->addWidget(lViewShade, 2, 0);
    lo->addWidget(kcbViewShade, 2, 1);

    kcbEditShade = new KColorButton(gbEnableShading);
    lEditShade = new QLabel(i18n("&Modified documents' shade:"), gbEnableShading);
    lEditShade->setBuddy(kcbEditShade);
    lo->addWidget(lEditShade, 3, 0);
    lo->addWidget(kcbEditShade, 3, 1);

    // sorting
    QHBoxLayout *lo2 = new QHBoxLayout;
    layout->addLayout(lo2);
    lSort = new QLabel(i18n("&Sort by:"), this);
    lo2->addWidget(lSort);
    cmbSort = new KComboBox(this);
    lo2->addWidget(cmbSort);
    lSort->setBuddy(cmbSort);
    cmbSort->addItem(i18n("Opening Order"), (int)KateFileTreeModel::OpeningOrderRole);
    cmbSort->addItem(i18n("Document Name"), (int)Qt::DisplayRole);
    cmbSort->addItem(i18n("Url"), (int)KateFileTreeModel::PathRole);

    // view mode
    QHBoxLayout *lo3 = new QHBoxLayout;
    layout->addLayout(lo3);
    lMode = new QLabel(i18n("&View Mode:"), this);
    lo3->addWidget(lMode);
    cmbMode = new KComboBox(this);
    lo3->addWidget(cmbMode);
    lMode->setBuddy(cmbMode);
    cmbMode->addItem(i18n("Tree View"), QVariant(false));
    cmbMode->addItem(i18n("List View"), QVariant(true));

    // Show Full Path on Roots?
    QHBoxLayout *lo4 = new QHBoxLayout;
    layout->addLayout(lo4);
    cbShowFullPath = new QCheckBox(i18n("&Show Full Path"), this);
    lo4->addWidget(cbShowFullPath);

    layout->insertStretch(-1, 10);

    gbEnableShading->setWhatsThis(i18n(
                                      "When background shading is enabled, documents that have been viewed "
                                      "or edited within the current session will have a shaded background. "
                                      "The most recent documents have the strongest shade."));
    kcbViewShade->setWhatsThis(i18n(
                                   "Set the color for shading viewed documents."));
    kcbEditShade->setWhatsThis(i18n(
                                   "Set the color for modified documents. This color is blended into "
                                   "the color for viewed files. The most recently edited documents get "
                                   "most of this color."));

    cbShowFullPath->setWhatsThis(i18n(
                                     "When enabled, in tree mode, top level folders will show up with their full path "
                                     "rather than just the last folder name."));

//   cmbSort->setWhatsThis( i18n(
//       "Set the sorting method for the documents.") );

    reset();

    connect(gbEnableShading, &QGroupBox::toggled, this, &KateFileTreeConfigPage::slotMyChanged);
    connect(kcbViewShade, &KColorButton::changed, this, &KateFileTreeConfigPage::slotMyChanged);
    connect(kcbEditShade, &KColorButton::changed, this, &KateFileTreeConfigPage::slotMyChanged);
    connect(cmbSort, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated),
            this, &KateFileTreeConfigPage::slotMyChanged);
    connect(cmbMode, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated),
            this, &KateFileTreeConfigPage::slotMyChanged);
    connect(cbShowFullPath, &QCheckBox::stateChanged, this, &KateFileTreeConfigPage::slotMyChanged);
}

QString KateFileTreeConfigPage::name() const
{
    return QString(i18n("Documents"));
}

QString KateFileTreeConfigPage::fullName() const
{
    return QString(i18n("Configure Documents"));
}

QIcon KateFileTreeConfigPage::icon() const
{
    return QIcon::fromTheme(QLatin1String("view-list-tree"));
}

void KateFileTreeConfigPage::apply()
{
    if (! m_changed) {
        return;
    }

    m_changed = false;

    // apply config to views
    m_plug->applyConfig(
        gbEnableShading->isChecked(),
        kcbViewShade->color(),
        kcbEditShade->color(),
        cmbMode->itemData(cmbMode->currentIndex()).toBool(),
        cmbSort->itemData(cmbSort->currentIndex()).toInt(),
        cbShowFullPath->checkState() == Qt::Checked
    );
}

void KateFileTreeConfigPage::reset()
{
    const KateFileTreePluginSettings &settings = m_plug->settings();

    gbEnableShading->setChecked(settings.shadingEnabled());
    kcbEditShade->setColor(settings.editShade());
    kcbViewShade->setColor(settings.viewShade());
    cmbSort->setCurrentIndex(cmbSort->findData(settings.sortRole()));
    cmbMode->setCurrentIndex(settings.listMode());
    cbShowFullPath->setCheckState(settings.showFullPathOnRoots() ? Qt::Checked : Qt::Unchecked);

    m_changed = false;
}

void KateFileTreeConfigPage::defaults()
{
    // m_plug->settings().revertToDefaults() ??
    // not sure the above is ever needed...
    reset();
}

void KateFileTreeConfigPage::slotMyChanged()
{
    m_changed = true;
    emit changed();
}

