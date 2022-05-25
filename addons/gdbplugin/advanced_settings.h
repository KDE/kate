// Description: Advanced settings dialog for gdb
//
//
// SPDX-FileCopyrightText: 2012 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#ifndef ADVANCED_SETTINGS_H
#define ADVANCED_SETTINGS_H

#include "ui_advanced_settings.h"
#include <QDialog>
#include <QStringList>

class AdvancedGDBSettings : public QDialog, public Ui::AdvancedGDBSettings
{
    Q_OBJECT
public:
    enum CustomStringOrder { GDBIndex = 0, LocalRemoteIndex, RemoteBaudIndex, SoAbsoluteIndex, SoRelativeIndex, SrcPathsIndex, CustomStartIndex };

    AdvancedGDBSettings(QWidget *parent = nullptr);
    ~AdvancedGDBSettings() override;

    static QJsonObject upgradeConfigV4_5(const QStringList &cfgs);
    static QStringList commandList(const QJsonObject &config);
    const QJsonObject configs() const;

    void setConfigs(const QJsonObject &cfgs);

    const static QString F_GDB;
    const static QString F_SRC_PATHS;

private:
    static void setComboText(QComboBox *combo, const QString &str);

private Q_SLOTS:
    void slotBrowseGDB();

    void slotSetSoPrefix();

    void slotAddSoPath();
    void slotDelSoPath();

    void slotAddSrcPath();
    void slotDelSrcPath();

    void slotLocalRemoteChanged();
};

#endif
