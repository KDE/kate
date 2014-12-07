// Description: Advanced settings dialog for gdb
//
//
// Copyright (c) 2012 Kåre Särs <kare.sars@iki.fi>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#include "advanced_settings.h"

#include <QFileDialog>

AdvancedGDBSettings::AdvancedGDBSettings(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    connect(u_gdbBrowse, SIGNAL(clicked()), this, SLOT(slotBrowseGDB()));
    connect(u_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(u_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
}

AdvancedGDBSettings::~AdvancedGDBSettings()
{
}

const QStringList AdvancedGDBSettings::configs() const
{
    QStringList tmp;

    tmp << u_gdbCmd->text();
    switch(u_localRemote->currentIndex()) {
        case 1:
            tmp << QStringLiteral("target remote %1:%2").arg(u_tcpHost->text()).arg(u_tcpPort->text());
            tmp << QString();
            break;
        case 2:
            tmp << QStringLiteral("target remote %1").arg(u_ttyPort->text());
            tmp << QStringLiteral("set remotebaud %1").arg(u_baudCombo->currentText());
            break;
        default:
            tmp << QString();
            tmp << QString();
    }
    if (!u_soAbsPrefix->text().isEmpty()) {
        tmp << QStringLiteral("set solib-absolute-prefix %1").arg(u_soAbsPrefix->text());
    }
    else {
        tmp << QString();
    }
    if (!u_soSearchPath->text().isEmpty()) {
        tmp << QStringLiteral("set solib-search-path %1").arg(u_soSearchPath->text());
    }
    else {
        tmp << QString();
    }
    tmp << u_customInit->toPlainText().split(QLatin1Char('\n'));

    return tmp;
}

void AdvancedGDBSettings::setConfigs(const QStringList &cfgs)
{
    // clear all info
    u_gdbCmd->setText(QStringLiteral("gdb"));
    u_localRemote->setCurrentIndex(0);
    u_soAbsPrefix->clear();
    u_soSearchPath->clear();
    u_customInit->clear();
    u_tcpHost->setText(QString());
    u_tcpPort->setText(QString());
    u_ttyPort->setText(QString());
    u_baudCombo->setCurrentIndex(0);

    // GDB
    if (cfgs.count() <= GDBIndex) return;
    u_gdbCmd->setText(cfgs[GDBIndex]);

    // Local / Remote
    if (cfgs.count() <= LocalRemoteIndex) return;

    int start;
    int end;
    if (cfgs[LocalRemoteIndex].isEmpty()) {
        u_localRemote->setCurrentIndex(0);
        u_remoteStack->setCurrentIndex(0);
    }
    else if (cfgs[LocalRemoteIndex].contains(QLatin1Char(':'))) {
        u_localRemote->setCurrentIndex(1);
        u_remoteStack->setCurrentIndex(1);
        start = cfgs[LocalRemoteIndex].lastIndexOf(QLatin1Char(' '));
        end = cfgs[LocalRemoteIndex].indexOf(QLatin1Char(':'));
        u_tcpHost->setText(cfgs[LocalRemoteIndex].mid(start+1, end-start-1));
        u_tcpPort->setText(cfgs[LocalRemoteIndex].mid(end+1));
    }
    else {
        u_localRemote->setCurrentIndex(2);
        u_remoteStack->setCurrentIndex(2);
        start = cfgs[LocalRemoteIndex].lastIndexOf(QLatin1Char(' '));
        u_ttyPort->setText(cfgs[LocalRemoteIndex].mid(start+1));

        start = cfgs[RemoteBaudIndex].lastIndexOf(QLatin1Char(' '));
        setComboText(u_baudCombo, cfgs[RemoteBaudIndex].mid(start+1));
    }

    // Solib absolute path
    if (cfgs.count() <= SoAbsoluteIndex) return;
    start = 26; // "set solib-absolute-prefix "
    u_soAbsPrefix->setText(cfgs[SoAbsoluteIndex].mid(start));

    // Solib search path
    if (cfgs.count() <= SoRelativeIndex) return;
    start = 22; // "set solib-search-path "
    u_soSearchPath->setText(cfgs[SoRelativeIndex].mid(start));

    // Custom init
    for (int i=CustomStartIndex; i<cfgs.count(); i++) {
        u_customInit->appendPlainText(cfgs[i]);
    }
}

void AdvancedGDBSettings::slotBrowseGDB()
{
    u_gdbCmd->setText(QFileDialog::getOpenFileName(this, QString(), u_gdbCmd->text(), QStringLiteral("application/x-executable")));
    if (u_gdbCmd->text().isEmpty()) {
        u_gdbCmd->setText(QStringLiteral("gdb"));
    }
}

void AdvancedGDBSettings::setComboText(QComboBox *combo, const QString &str)
{
    if (!combo) return;

    for (int i=0; i<combo->count(); i++) {
        if (combo->itemText(i) == str) {
            combo->setCurrentIndex(i);
            return;
        }
    }
    // The string was not found -> add it
    combo->addItem(str);
    combo->setCurrentIndex(combo->count()-1);
}
